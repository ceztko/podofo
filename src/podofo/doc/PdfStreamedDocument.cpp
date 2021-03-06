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

#include "PdfStreamedDocument.h"

#include "base/PdfDefinesPrivate.h"

using namespace std;
using namespace PoDoFo;

PdfStreamedDocument::PdfStreamedDocument( PdfOutputDevice& pDevice, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : m_pWriter( nullptr ), m_pDevice( nullptr ), m_pEncrypt( pEncrypt ), m_bOwnDevice( false )
{
    Init(pDevice, eVersion, pEncrypt, eWriteMode);
}

PdfStreamedDocument::PdfStreamedDocument(const string_view& filename, EPdfVersion eVersion, PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode )
    : m_pWriter( nullptr ), m_pEncrypt( pEncrypt ), m_bOwnDevice( true )
{
    m_pDevice = new PdfOutputDevice(filename);
    Init(*m_pDevice, eVersion, pEncrypt, eWriteMode);
}

PdfStreamedDocument::~PdfStreamedDocument()
{
    delete m_pWriter;
    if( m_bOwnDevice )
        delete m_pDevice;
}

void PdfStreamedDocument::Init(PdfOutputDevice& pDevice, EPdfVersion eVersion, 
                                PdfEncrypt* pEncrypt, EPdfWriteMode eWriteMode)
{
    m_pWriter = new PdfImmediateWriter( this->GetObjects(), this->GetTrailer(), pDevice, eVersion, pEncrypt, eWriteMode );
}

void PdfStreamedDocument::Close()
{
    // TODO: Check if this works correctly
	// makes sure pending subset-fonts are embedded
	GetFontCache().EmbedSubsetFonts();
    
    this->GetObjects().Finish();
}

EPdfWriteMode PdfStreamedDocument::GetWriteMode() const
{
    return m_pWriter->GetWriteMode();
}

EPdfVersion PdfStreamedDocument::GetPdfVersion() const
{
    return m_pWriter->GetPdfVersion();
}

bool PdfStreamedDocument::IsLinearized() const
{
    // Linearization is currently not supported by PdfStreamedDocument
    return false;
}

bool PdfStreamedDocument::IsPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsPrintAllowed() : true;
}

bool PdfStreamedDocument::IsEditAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditAllowed() : true;
}

bool PdfStreamedDocument::IsCopyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsCopyAllowed() : true;
}

bool PdfStreamedDocument::IsEditNotesAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsEditNotesAllowed() : true;
}

bool PdfStreamedDocument::IsFillAndSignAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsFillAndSignAllowed() : true;
}

bool PdfStreamedDocument::IsAccessibilityAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsAccessibilityAllowed() : true;
}

bool PdfStreamedDocument::IsDocAssemblyAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsDocAssemblyAllowed() : true;
}

bool PdfStreamedDocument::IsHighPrintAllowed() const
{
    return m_pEncrypt ? m_pEncrypt->IsHighPrintAllowed() : true;
}
