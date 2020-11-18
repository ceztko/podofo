/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
 *                                                                         *
 *   Pdf CMAP encoding by kalyan                                           *
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

#include "PdfCMapEncoding.h"
#include "base/PdfDefinesPrivate.h"
#include "base/PdfEncodingFactory.h"
#include "base/PdfObject.h"
#include "base/PdfVariant.h"
#include "base/PdfLocale.h"
#include "base/PdfStream.h"
#include "base/PdfContentsTokenizer.h"


#include <iostream>
#include <stack>
#include <iomanip>
#include <string>

using namespace std;
using namespace PoDoFo;

PdfCMapEncoding::PdfCMapEncoding (const PdfObject& obj, const PdfObject * pToUnicode) :
    PdfEncoding(0x0000, 0xFFFF, pToUnicode == nullptr ? pToUnicode : &obj),
    m_baseEncoding( EBaseEncoding::Font )
{
    // TODO: If /ToUnicode is absent, and CID font is not identity, use the CID font's predefined character collection
    // (/CIDSystemInfo<</Registry(XXX)/Ordering(XXX)/Supplement 0>>)
}

void PdfCMapEncoding::AddToDictionary(PdfDictionary& rDictionary) const
{
    throw runtime_error("Not implemented");
}

const PdfEncoding* PdfCMapEncoding::GetBaseEncoding() const
{
    const PdfEncoding* pEncoding = nullptr;

    switch( m_baseEncoding ) 
    {
        case EBaseEncoding::WinAnsi:
            pEncoding = PdfEncodingFactory::GlobalWinAnsiEncodingInstance();
            break;

        case EBaseEncoding::MacRoman:
            pEncoding = PdfEncodingFactory::GlobalMacRomanEncodingInstance();
            break;

        case EBaseEncoding::MacExpert:
        case EBaseEncoding::Font:
        default:
            break;
    }

    if( !pEncoding ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    return pEncoding;
}

bool PdfCMapEncoding::IsSingleByteEncoding() const
{
	return false;
}

bool PdfCMapEncoding::IsAutoDelete() const
{
    return true;
}

char32_t PdfCMapEncoding::GetCharCode( int nIndex ) const
{
    if( nIndex < this->GetFirstChar() ||
       nIndex > this->GetLastChar() )
    {
        PODOFO_RAISE_ERROR( EPdfError::ValueOutOfRange );
    }
    
    return static_cast<char32_t>(nIndex);
}

const PdfName & PdfCMapEncoding::GetID() const
{
    PODOFO_RAISE_ERROR( EPdfError::NotImplemented );
}
