/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 *                                                                         *
 *   In addition, as a special exception, the copyright holders give       *
 *   permission to link the code of portions of this program with the      *
 *   OpenSSL library under certain conditions as described in each         *
 *   individual source file, and distribute linked combinations            *
 *   including the two.                                                    *
 *   You must obey the GNU General Public License in all respects          *
 *   for all of the code used other than OpenSSL.  If you modify           *
 *   file(s) with this exception, you may extend this exception to your    *
 *   version of the file(s), but you are not obligated to do so.  If you   *
 *   do not wish to do so, delete this exception statement from your       *
 *   version.  If you delete this exception statement from all source      *
 *   files in the program, then also delete it here.                       *
 ***************************************************************************/

#include "PdfImmediateWriter.h"

#include "PdfFileStream.h"
#include "PdfMemStream.h"
#include "PdfObject.h"
#include "PdfXRef.h"
#include "PdfXRefStream.h"
#include "PdfDefinesPrivate.h"

using namespace PoDoFo;

PdfImmediateWriter::PdfImmediateWriter(PdfVecObjects& pVecObjects, const PdfObject& pTrailer,
        PdfOutputDevice& pDevice, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode ) :
    PdfWriter( pVecObjects, pTrailer),
    m_attached(true),
    m_pDevice(&pDevice),
    m_pLast( nullptr ),
    m_bOpenStream( false )
{
    // register as observer for PdfVecObjects
    GetObjects().Attach( this );
    // register as stream factory for PdfVecObjects
    GetObjects().SetStreamFactory( this );

    PdfString identifier;
    this->CreateFileIdentifier(identifier, pTrailer);
    SetIdentifier(identifier);

    // setup encryption
    if( pEncrypt )
    {
        this->SetEncrypted( *pEncrypt );
        pEncrypt->GenerateEncryptionKey(GetIdentifier());
    }

    // start with writing the header
    this->SetPdfVersion( eVersion );
    this->SetWriteMode( eWriteMode );
    this->WritePdfHeader(*m_pDevice);

    m_pXRef.reset(GetUseXRefStream() ? new PdfXRefStream(*this, GetObjects()) : new PdfXRef(*this));

}

PdfImmediateWriter::~PdfImmediateWriter()
{
    if(m_attached)
        GetObjects().Detach( this );
}

EPdfWriteMode PdfImmediateWriter::GetWriteMode() const
{
    return PdfWriter::GetWriteMode();
}

EPdfVersion PdfImmediateWriter::GetPdfVersion() const
{
    return PdfWriter::GetPdfVersion();
}

void PdfImmediateWriter::WriteObject( const PdfObject* pObject )
{
    const int endObjLenght = 7;

    this->FinishLastObject();

    m_pXRef->AddInUseObject( pObject->GetIndirectReference(), m_pDevice->Tell());
    pObject->Write(*m_pDevice, this->GetWriteMode(), GetEncrypt());
    // Make sure, no one will add keys now to the object
    const_cast<PdfObject*>(pObject)->SetImmutable(true);

    // Let's cheat a bit:
    // pObject has written an "endobj\n" as last data to the file.
    // we simply overwrite this string with "stream\n" which 
    // has excatly the same length.
    m_pDevice->Seek( m_pDevice->Tell() - endObjLenght );
    m_pDevice->Print( "stream\n" );
    m_pLast = const_cast<PdfObject*>(pObject);
}

void PdfImmediateWriter::Finish()
{
    // write all objects which are still in RAM
    this->FinishLastObject();

    // setup encrypt dictionary
    if( GetEncrypt() )
    {
        // Add our own Encryption dictionary
        SetEncryptObj(GetObjects().CreateDictionaryObject());
        GetEncrypt()->CreateEncryptionDictionary(GetEncryptObj()->GetDictionary() );
    }

    this->WritePdfObjects(*m_pDevice, GetObjects(), *m_pXRef);

    // write the XRef
    uint64_t lXRefOffset = static_cast<uint64_t>( m_pDevice->Tell() );
    m_pXRef->Write(*m_pDevice);

    // FIX-ME: The following is already done by PdfXRef now
    PODOFO_RAISE_ERROR(EPdfError::NotImplemented);

    // XRef streams contain the trailer in the XRef
    if( !GetUseXRefStream() )
    {
        PdfObject trailer;
        
        // if we have a dummy offset we write also a prev entry to the trailer
        FillTrailerObject( trailer, m_pXRef->GetSize(), false );
        
        m_pDevice->Print("trailer\n");
        trailer.Write(*m_pDevice, this->GetWriteMode(), nullptr);
    }
    
    m_pDevice->Print( "startxref\n%" PDF_FORMAT_UINT64 "\n%%%%EOF\n", lXRefOffset );
    m_pDevice->Flush();

    // we are done now
    GetObjects().Detach( this );
    m_attached = false;
}

PdfStream* PdfImmediateWriter::CreateStream( PdfObject* pParent )
{
    return m_bOpenStream ? 
        static_cast<PdfStream*>(new PdfMemStream( pParent )) :
        static_cast<PdfStream*>(new PdfFileStream( pParent, m_pDevice ));
}

void PdfImmediateWriter::FinishLastObject()
{
    if( m_pLast ) 
    {
        m_pDevice->Print( "\nendstream\n" );
        m_pDevice->Print( "endobj\n" );
        
        GetObjects().RemoveObject( m_pLast->GetIndirectReference(), false );
        m_pLast = nullptr;
    }
}

void PdfImmediateWriter::BeginAppendStream( const PdfStream* pStream )
{
    const PdfFileStream* pFileStream = dynamic_cast<const PdfFileStream*>(pStream );
    if( pFileStream ) 
    {
        // Only one open file stream is allowed at a time
        PODOFO_ASSERT( !m_bOpenStream );
        m_bOpenStream = true;

        if( GetEncrypt() )
            const_cast<PdfFileStream*>(pFileStream)->SetEncrypted(GetEncrypt());
    }
}
    
void PdfImmediateWriter::EndAppendStream( const PdfStream* pStream )
{
    const PdfFileStream* pFileStream = dynamic_cast<const PdfFileStream*>(pStream );
    if( pFileStream ) 
    {
        // A PdfFileStream has to be opened before
        PODOFO_ASSERT( m_bOpenStream );
        m_bOpenStream = false;
    }
}
