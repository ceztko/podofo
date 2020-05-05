/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfString.h"

#include <stdexcept>
#include <utfcpp/utf8.h>

#include "PdfEncrypt.h"
#include "PdfEncoding.h"
#include "PdfEncodingFactory.h"
#include "PdfFilter.h"
#include "PdfTokenizer.h"
#include "PdfOutputDevice.h"
#include "PdfDefinesPrivate.h"

using namespace std;
using namespace PoDoFo;

enum class StringEncoding
{
    utf8,
    utf16be,
    utf16le,
    PdfDocEncoding
};

static const char* genStrEscMap();
static StringEncoding GetEncoding(const string_view& view);

static const char* const m_escMap = genStrEscMap(); // Mapping of escape sequences to their value

PdfString::PdfString()
    : m_data(std::make_shared<string>()), m_state(StringState::PdfDocEncoding), m_isHex(false)
{
}

PdfString::PdfString(const char* str)
    : m_isHex(false)
{
    initFromUtf8String({ str, std::strlen(str) });
}

PdfString::PdfString(const string_view& view)
    : m_isHex(false)
{
    initFromUtf8String(view);
}

PdfString::PdfString( const PdfString & rhs )
{
    this->operator=( rhs );
}

PdfString::PdfString(const shared_ptr<string>& data, bool isHex)
    : m_data(data), m_state(StringState::RawBuffer), m_isHex(isHex)
{
}

PdfString PdfString::FromRaw(const string_view &view, bool isHex)
{
    return PdfString(std::make_shared<string>(view), isHex);
}

PdfString PdfString::FromHexData(const string_view& view, PdfEncrypt* pEncrypt )
{
    size_t lLen = view.size();
    auto buffer = std::make_shared<string>();
    buffer->resize(lLen % 2 ? (lLen + 1) >> 1 : lLen >> 1);

    char val;
    char cDecodedByte = 0;
    bool bLow = true;
    for(size_t i = 0; i < lLen; i++)
    {
        char ch = view[i];
        if( PdfTokenizer::IsWhitespace(ch) )
            continue;

        val = PdfTokenizer::GetHexValue(ch);
        if (bLow)
        {
            cDecodedByte = val & 0x0F;
            bLow = false;
        }
        else
        {
            cDecodedByte = (cDecodedByte << 4) | val;
            bLow = true;
            buffer->push_back(cDecodedByte);
        }
    }

    if( !bLow ) 
    {
        // an odd number of bytes was read,
        // so the last byte is 0
        buffer->push_back(cDecodedByte);
    }

    buffer->shrink_to_fit();

    if (pEncrypt)
    {
        throw std::runtime_error("Untested after utf-8 migration");
        // CHECK-ME: Why outBufferLen is not the actual size of the buffer
        size_t outBufferLen = buffer->size() - pEncrypt->CalculateStreamOffset();
        auto decrypted = std::make_shared<string>();
        decrypted->resize(outBufferLen + 16 - (outBufferLen % 16));
        pEncrypt->Decrypt(reinterpret_cast<const unsigned char*>(buffer->data()),
            buffer->size(), reinterpret_cast<unsigned char*>(decrypted->data()), outBufferLen);

        return PdfString(decrypted, true);
    }
    else
    {
        return PdfString(buffer, true);
    }
}

void PdfString::Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const
{
    // Strings in PDF documents may contain \0 especially if they are encrypted
    // this case has to be handled!

    // Now we are not encrypting the empty strings (was access violation)!
    string tempBuffer;
    string_view dataview;
    bool isUnicode = IsUnicode();
    if (isUnicode)
    {
        // Prepend utf-16 BOM
        tempBuffer.push_back(static_cast<char>(0xFE));
        tempBuffer.push_back(static_cast<char>(0xFF));
        tempBuffer.append(m_data->data(), m_data->size());
        dataview = string_view(tempBuffer);
    }
    else
    {
        dataview = string_view(*m_data);
    }

    if( pEncrypt && dataview.size() > 0)
    {
        size_t nOutputBufferLen = pEncrypt->CalculateStreamLength(dataview.size());
        string encrypted;
        encrypted.resize(nOutputBufferLen);
        pEncrypt->Encrypt(reinterpret_cast<const unsigned char*>(dataview.data()), dataview.size(), reinterpret_cast<unsigned char*>(encrypted.data()), nOutputBufferLen);
        encrypted.swap(tempBuffer);
        dataview = string_view(tempBuffer);
    }

    pDevice.Print( m_isHex ? "<" : "(" );
    if(dataview.size() > 0)
    {
        const char* pBuf = dataview.data();
        size_t lLen = dataview.size();

        if( m_isHex ) 
        {
            char data[2];
            while( lLen-- )
            {
                data[0]  = (*pBuf & 0xF0) >> 4;
                data[0] += (data[0] > 9 ? 'A' - 10 : '0');
                
                data[1]  = (*pBuf & 0x0F);
                data[1] += (data[1] > 9 ? 'A' - 10 : '0');
                
                pDevice.Write( data, 2 );
                
                ++pBuf;
            }
        }
        else
        {
            while( lLen-- ) 
            {
                const char & cEsc = m_escMap[static_cast<unsigned char>(*pBuf)];
                if( cEsc != 0 ) 
                {
                    pDevice.Write( "\\", 1 );
                    pDevice.Write( &cEsc, 1 );
                }
                else 
                {
                    pDevice.Write( &*pBuf, 1 );
                }

                ++pBuf;
            }
        }
    }

    pDevice.Print( m_isHex ? ">" : ")" );
}

bool PdfString::IsUnicode() const
{
    evaluateString();
    return m_state == StringState::Unicode;
}

const string& PdfString::GetString() const
{
    evaluateString();
    return *m_data;
}

size_t PdfString::GetLength() const
{
    evaluateString();
    return m_data->length();
}

const PdfString & PdfString::operator=( const PdfString & rhs )
{
    this->m_data = rhs.m_data;
    this->m_state = rhs.m_state;
    this->m_isHex = rhs.m_isHex;
    return *this;
}

bool PdfString::operator==( const PdfString & rhs ) const
{
    if (this == &rhs)
        return true;

    if (!canPerformComparison(*this, rhs))
        return false;

    if (this->m_data == rhs.m_data)
        return true;

    return *this->m_data == *rhs.m_data;
}

bool PdfString::operator==(const char* str) const
{
    return operator==(string_view(str, std::strlen(str)));
}

bool PdfString::operator==(const string& str) const
{
    return operator==((string_view)str);
}

bool PdfString::operator==(const string_view& view) const
{
    if (!isValidText())
        return false;

    return *m_data == view;
}

bool PdfString::operator!=(const PdfString& rhs) const
{
    if (this == &rhs)
        return false;

    if (!canPerformComparison(*this, rhs))
        return true;

    if (this->m_data == rhs.m_data)
        return false;

    return *this->m_data != *rhs.m_data;
}

bool PdfString::operator!=(const char* str) const
{
    return operator!=(string_view(str, std::strlen(str)));
}

bool PdfString::operator!=(const string& str) const
{
    return operator!=((string_view)str);
}

bool PdfString::operator!=(const string_view& view) const
{
    if (!isValidText())
        return true;

    return *m_data != view;
}

void PdfString::initFromUtf8String(const string_view &view)
{
    if (view.data() == nullptr)
        throw runtime_error("String is null");

    if (view.length() == 0)
    {
        m_data = make_shared<string>();
        m_state = StringState::PdfDocEncoding;
        return;
    }

    m_data = std::make_shared<string>(view);

    bool isPdfDocEncodingEqual;
    if (PdfDocEncoding::CheckValidUTF8ToPdfDocEcondingChars(view, isPdfDocEncodingEqual))
        m_state = StringState::PdfDocEncoding;
    else
        m_state = StringState::Unicode;
}

void PdfString::evaluateString() const
{
    switch (m_state)
    {
    case StringState::PdfDocEncoding:
    case StringState::Unicode:
        return;
    case StringState::RawBuffer:
    {
        auto encoding = GetEncoding(*m_data);
        switch (encoding)
        {
        case StringEncoding::utf16be:
        {
            string utf8;
            utf8::utf16to8(utf8::endianess::big_endian, m_data->data(), m_data->data() + m_data->size(), std::back_inserter(utf8));
            utf8.swap(*m_data);
            const_cast<PdfString&>(*this).m_state = StringState::Unicode;
            break;
        }
        case StringEncoding::utf16le:
        {
            string utf8;
            utf8::utf16to8(utf8::endianess::little_endian, m_data->data(), m_data->data() + m_data->size(), std::back_inserter(utf8));
            utf8.swap(*m_data);
            const_cast<PdfString&>(*this).m_state = StringState::Unicode;
            break;
        }
        case StringEncoding::utf8:
        {
            m_data->substr(3).swap(*m_data);
            const_cast<PdfString&>(*this).m_state = StringState::Unicode;
            break;
        }
        case StringEncoding::PdfDocEncoding:
        {
            bool isUTF8Equal;
            auto utf8 = PdfDocEncoding::ConvertPdfDocEncodingToUTF8(*m_data, isUTF8Equal);
            utf8.swap(*m_data);
            const_cast<PdfString&>(*this).m_state = StringState::PdfDocEncoding;
            break;
        }
        default:
            throw runtime_error("Unsupported");
        }

        return;
    }
    default:
        throw runtime_error("Unsupported");
    }
}

// Returns true only if same state or it's valid text string
bool PdfString::canPerformComparison(const PdfString& lhs, const PdfString& rhs)
{
    if (lhs.m_state == rhs.m_state)
        return true;

    if (lhs.isValidText() || rhs.isValidText())
        return true;

    return false;
}

const string& PdfString::GetRawData() const
{
    if (m_state != StringState::RawBuffer)
        throw runtime_error("The string buffer has been evaluated");

    return *m_data;
}

bool PdfString::isValidText() const
{
    return m_state == StringState::PdfDocEncoding || m_state == StringState::Unicode;
}

// Generate the escape character map at runtime
const char* genStrEscMap()
{
    static char g_StrEscMap[256] = { 0 };
    const long lAllocLen = 256;
    char* map = static_cast<char*>(g_StrEscMap);
    memset(map, 0, sizeof(char) * lAllocLen);
    map[static_cast<unsigned char>('\n')] = 'n'; // Line feed (LF)
    map[static_cast<unsigned char>('\r')] = 'r'; // Carriage return (CR)
    map[static_cast<unsigned char>('\t')] = 't'; // Horizontal tab (HT)
    map[static_cast<unsigned char>('\b')] = 'b'; // Backspace (BS)
    map[static_cast<unsigned char>('\f')] = 'f'; // Form feed (FF)
    map[static_cast<unsigned char>(')')] = ')';
    map[static_cast<unsigned char>('(')] = '(';
    map[static_cast<unsigned char>('\\')] = '\\';
    return map;
}

StringEncoding GetEncoding(const string_view& view)
{
    const char utf16beMarker[2] = { static_cast<char>(0xFE), static_cast<char>(0xFF) };
    if (view.size() >= sizeof(utf16beMarker) && memcmp(view.data(), utf16beMarker, sizeof(utf16beMarker)) == 0)
        return StringEncoding::utf16be;

    // NOTE: Little endian should not be officially supported
    const char utf16leMarker[2] = { static_cast<char>(0xFF), static_cast<char>(0xFE) };
    if (view.size() >= sizeof(utf16leMarker) && memcmp(view.data(), utf16leMarker, sizeof(utf16leMarker)) == 0)
        return StringEncoding::utf16le;

    const char utf8Marker[3] = { static_cast<char>(0xEF), static_cast<char>(0xBB), static_cast<char>(0xBF) };
    if (view.size() >= sizeof(utf8Marker) && memcmp(view.data(), utf8Marker, sizeof(utf8Marker)) == 0)
        return StringEncoding::utf8;

    return StringEncoding::PdfDocEncoding;
}
