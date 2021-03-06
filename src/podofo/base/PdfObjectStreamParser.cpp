/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *   Copyright (C) 2020 by Francesco Pretto                                *
 *   ceztko@gmail.com                                                      *
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

#include "PdfDefinesPrivate.h"
#include "PdfObjectStreamParser.h"

#include "PdfDictionary.h"
#include "PdfEncrypt.h"
#include "PdfInputDevice.h"
#include "PdfParserObject.h"
#include "PdfStream.h"
#include "PdfVecObjects.h"

#include <limits>
#include <algorithm>

#ifndef VERBOSE_DEBUG_DISABLED
#include <iostream>
#endif

using namespace std;
using namespace PoDoFo;

PdfObjectStreamParser::PdfObjectStreamParser(PdfParserObject* pParser, PdfVecObjects* pVecObjects, const PdfRefCountedBuffer & rBuffer, PdfEncrypt* pEncrypt )
    : m_pParser( pParser ), m_vecObjects( pVecObjects ), m_buffer( rBuffer ), m_pEncrypt( pEncrypt )
{

}

void PdfObjectStreamParser::Parse(ObjectIdList const & list)
{
    int64_t lNum   = m_pParser->GetDictionary().GetKeyAsNumber( "N", 0 );
    int64_t lFirst = m_pParser->GetDictionary().GetKeyAsNumber( "First", 0 );
    
    unique_ptr<char> buffer;
    size_t lBufferLen;
    m_pParser->GetOrCreateStream().GetFilteredCopy(buffer, lBufferLen);

    this->ReadObjectsFromStream(buffer.get(), lBufferLen, lNum, lFirst, list );
    m_pParser = nullptr;
}

void PdfObjectStreamParser::ReadObjectsFromStream( char* pBuffer, size_t lBufferLen, int64_t lNum, int64_t lFirst, ObjectIdList const & list)
{
    PdfRefCountedInputDevice device( pBuffer, lBufferLen );
    PdfTokenizer tokenizer(m_buffer );
    PdfVariant var;
    int i = 0;

    while( static_cast<int64_t>(i) < lNum )
    {
        const int64_t lObj = tokenizer.ReadNextNumber(device);
        const int64_t lOff = tokenizer.ReadNextNumber(device);
        size_t pos = device.Device()->Tell();

        if( lFirst >= std::numeric_limits<int64_t>::max() - lOff )
        {
            PODOFO_RAISE_ERROR_INFO( EPdfError::BrokenFile,
                                    "Object position out of max limit" );
        }

        // move to the position of the object in the stream
        device.Device()->Seek( static_cast<std::streamoff>(lFirst + lOff) );

		// use a second tokenizer here so that anything that gets dequeued isn't left in the tokenizer that reads the offsets and lengths
	    PdfTokenizer variantTokenizer(m_buffer);
		if (m_pEncrypt && (m_pEncrypt->GetEncryptAlgorithm() == EPdfEncryptAlgorithm::AESV2
			|| m_pEncrypt->GetEncryptAlgorithm() == EPdfEncryptAlgorithm::RC4V2))
        {
            variantTokenizer.ReadNextVariant(device, var); // Stream is already decrypted
        }
        else
        {
            variantTokenizer.ReadNextVariant(device, var, m_pEncrypt);
        }

		bool should_read = std::find(list.begin(), list.end(), lObj) != list.end();
#ifndef VERBOSE_DEBUG_DISABLED
        std::cerr << "ReadObjectsFromStream STREAM=" << m_pParser->GetIndirectReference().ToString() <<
			", OBJ=" << lObj <<
			", " << (should_read ? "read" : "skipped") << std::endl;
#endif
		if (should_read)
        {
            // The generation number of an object stream and of any
            // compressed object is implicitly zero
            PdfReference reference(static_cast<uint32_t>(lObj), 0);
            auto obj = new PdfObject(var);
            m_vecObjects->PushObject(reference, obj);
		}

        // move back to the position inside of the table of contents
        device.Device()->Seek( pos );

        ++i;
    }
}
