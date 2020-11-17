/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfIdentityEncoding.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfDictionary.h"
#include "base/PdfLocale.h"

#include "PdfFont.h"

#include <sstream>
#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;
using namespace PoDoFo;

PdfIdentityEncoding::PdfIdentityEncoding( int nFirstChar, int nLastChar, bool bAutoDelete, PdfObject *pToUnicode )
    : PdfEncoding( nFirstChar, nLastChar, pToUnicode ), m_bAutoDelete( bAutoDelete )
{
    // create a unique ID
    std::ostringstream oss;
    oss << "/Identity-H" << nFirstChar << "_" << nLastChar;

    m_id = PdfName( oss.str() );
}

void PdfIdentityEncoding::AddToDictionary( PdfDictionary & rDictionary ) const
{
    rDictionary.AddKey( "Encoding", PdfName("Identity-H") );
}

char32_t PdfIdentityEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() || nIndex > this->GetLastChar() )
	    PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );

    return (char32_t)nIndex;
}

string PdfIdentityEncoding::ConvertToUnicode(const string_view& rEncodedString) const
{
    if( IsToUnicodeLoaded() )
    {
        return PdfEncoding::ConvertToUnicode(rEncodedString);
    }
    else
    {
        /* Identity-H means 1-1 mapping */	  
        return (string)rEncodedString;
    }
}

string PdfIdentityEncoding::ConvertToEncoding(const string_view& str) const
{
    if( IsToUnicodeLoaded() )
    {
        return PdfEncoding::ConvertToEncoding(str);
    }
    else
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    return { };
}
