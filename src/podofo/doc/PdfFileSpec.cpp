/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfFileSpec.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfInputStream.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"

#include <sstream>

namespace PoDoFo {

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, bool bEmbedd, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, bEmbedd, bStripPath );
}

PdfFileSpec::PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfVecObjects* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}


PdfFileSpec::PdfFileSpec( const char* pszFilename, const unsigned char* data, ptrdiff_t size, PdfDocument* pParent, bool bStripPath)
    : PdfElement( "Filespec", pParent )
{
    Init( pszFilename, data, size, bStripPath );
}

PdfFileSpec::PdfFileSpec( PdfObject* pObject )
    : PdfElement( "Filespec", pObject )
{
}

void PdfFileSpec::Init( const char* pszFilename, bool bEmbedd, bool bStripPath ) 
{
    PdfObject* pEmbeddedStream;
    PdfString filename( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath ) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename );

    if( bEmbedd ) 
    {
        PdfDictionary ef;

        pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
        this->EmbeddFile( pEmbeddedStream, pszFilename );

        ef.AddKey( "F",  pEmbeddedStream->GetIndirectReference() );

        this->GetObject()->GetDictionary().AddKey( "EF", ef );
    }
}

void PdfFileSpec::Init( const char* pszFilename, const unsigned char* data, ptrdiff_t size, bool bStripPath ) 
{
    PdfObject* pEmbeddedStream;
    PdfString filename( MaybeStripPath( pszFilename, true) );

    this->GetObject()->GetDictionary().AddKey( "F", this->CreateFileSpecification( MaybeStripPath( pszFilename, bStripPath) ) );
    this->GetObject()->GetDictionary().AddKey( "UF", filename );

    PdfDictionary ef;

    pEmbeddedStream = this->CreateObject( "EmbeddedFile" );
    this->EmbeddFileFromMem( pEmbeddedStream, data, size );

    ef.AddKey( "F",  pEmbeddedStream->GetIndirectReference() );

    this->GetObject()->GetDictionary().AddKey( "EF", ef );
}

PdfString PdfFileSpec::CreateFileSpecification( const char* pszFilename ) const
{
    std::ostringstream str;
    size_t                nLen = strlen( pszFilename );
    char buff[5];

    // Construct a platform independent file specifier
    
    for( size_t i=0;i<nLen;i++ ) 
    {
        char ch = pszFilename[i];
        if (ch == ':' || ch == '\\')
            ch = '/';
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
             ch == '_') {
            str.put( ch & 0xFF );
        } else if (ch == '/') {
            str.put( '\\' );
            str.put( '\\' );
            str.put( '/' );
        } else {
            sprintf(buff, "%02X", ch & 0xFF);
            str << buff;
        }
    }

    return PdfString( str.str() );
}

void PdfFileSpec::EmbeddFile( PdfObject* pStream, const char* pszFilename ) const
{
    size_t size = io::FileSize(pszFilename);

    PdfFileInputStream stream( pszFilename );
    pStream->GetOrCreateStream().Set( &stream );

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey("Size", static_cast<int64_t>(size));
    // TODO: CreationDate and ModDate
    pStream->GetDictionary().AddKey("Params", params );
}

const char *PdfFileSpec::MaybeStripPath( const char* pszFilename, bool bStripPath ) const
{
    if (!bStripPath)
    {
        return pszFilename;
    }

    const char *lastFrom = pszFilename;
    while (pszFilename && *pszFilename)
    {
        if (
            #ifdef WIN32
            *pszFilename == ':' || *pszFilename == '\\' ||
            #endif // WIN32
            *pszFilename == '/')
        {
            lastFrom = pszFilename + 1;
        }

        pszFilename++;
    }

    return lastFrom;
}

void PdfFileSpec::EmbeddFileFromMem( PdfObject* pStream, const unsigned char* data, ptrdiff_t size ) const
{
    PdfMemoryInputStream memstream(reinterpret_cast<const char*>(data),size);
    pStream->GetOrCreateStream().Set( &memstream );

    // Add additional information about the embedded file to the stream
    PdfDictionary params;
    params.AddKey( "Size", static_cast<int64_t>(size) );
    pStream->GetDictionary().AddKey("Params", params );
}

const PdfString & PdfFileSpec::GetFilename(bool canUnicode) const
{
    if( canUnicode && this->GetObject()->GetDictionary().HasKey( "UF" ) )
{
        return this->GetObject()->GetDictionary().GetKey( "UF" )->GetString();
    }

    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
    {
        return this->GetObject()->GetDictionary().GetKey( "F" )->GetString();
    }

    PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
}


};
