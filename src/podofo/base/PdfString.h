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

#ifndef PDF_STRING_H
#define PDF_STRING_H

#include "PdfDefines.h"

#include <memory>
#include <string>
#include <string_view>

#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

class PdfOutputDevice;

/** A string that can be written to a PDF document.
 *  If it contains binary data it is automatically 
 *  converted into a hex string, otherwise a normal PDF 
 *  string is written to the document.
 *
 *  PdfStrings representing text are encoded either in PDFDocEncoding
 *  (ISO Latin1) or UTF-16BE.
 *
 *  PoDoFo contains methods to convert between these 
 *  encodings. For convenience conversion to UTF-8
 *  is possible too. Please note that strings are
 *  always stored as UTF-16BE or ISO Latin1 (PdfDocEncoding)
 *  in the PDF file.
 *
 *  UTF-16BE strings have to start with the bytes 0xFE 0xFF
 *  to be recognized by PoDoFo as unicode strings.
 *
 *
 *  PdfString is an implicitly shared class. As a reason
 *  it is very fast to copy PdfString objects.
 *
 *  The internal string buffer is guaranteed to be always terminated 
 *  by 2 zero ('\0') bytes.
 */
class PODOFO_API PdfString : public PdfDataType
{
    friend class PdfTokenizer;
public:
    /** Create an empty and invalid string 
     */
    PdfString();

    PdfString(const char * str);

    /** Construct a new PdfString from a utf-8 string
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString(const std::string_view& view);

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     */
    PdfString( const PdfString & rhs );

    // Delete constructor with nullptr
    PdfString(nullptr_t) = delete;

    /** Construct a new PdfString from an UTF-8 encoded string.
     *
     *  \param view a buffer
     *  \param hex true if the string should be written as hex string
     */
    static PdfString FromRaw(const std::string_view &view, bool hex = true);

    /** Set hex-encoded data as the strings data.
     *  \param pszHex must be hex-encoded data.
     *  \param lLen   length of the hex-encoded data.
     *  \param pEncrypt if !NULL, assume the hex data is encrypted and should be decrypted after hex-decoding.
     */
    static PdfString FromHexData(const std::string_view& view, PdfEncrypt* pEncrypt = nullptr);

    /** Check if this is a hex string.
     *  
     *  If true the data will be hex-encoded when the string is written to
     *  a PDF file.
     *
     *  \returns true if this is a hex string.
     *  \see GetString() will return the raw string contents (not hex-encoded)
     */
    inline bool IsHex () const { return m_isHex; }

    /**
     * PdfStrings are either PdfDocEncoded, or unicode encoded (UTF-16BE or UTF-8) strings.
     *
     * This function returns true if this is a unicode string object.
     *
     * \returns true if this is a unicode string.
     */
    bool IsUnicode() const;

    /** The contents of the string as UTF-8 string.
     *
     *  The string's contents are always returned as
     *  UTF-8 by this function. Works for unicode strings 
     *  and for non-unicode strings.
     *
     *  This is the preferred way to access the string's contents.
     *
     *  \returns the string's contents always as UTF-8
     */
    const std::string & GetString() const;

    const std::string& GetRawData() const;

    /** The length of the string data returned by GetString() 
     *  in bytes not including terminating zero ('\0') bytes.
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    size_t GetLength() const;

    /** Write this PdfString in PDF format to a PdfOutputDevice.
     *  
     *  \param pDevice the output device.
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object,
     *                  or nullptr to not encrypt this object
     */
    void Write(PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt) const;

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString & operator=( const PdfString & rhs );

    /** Comparison operator
     *
     *  UTF-8 and strings of the same data compare equal. Whether
     *  the string will be written out as hex is not considered - only the real "text"
     *  is tested for equality.
     *
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents 
     */
    bool operator==( const PdfString & rhs ) const;
    bool operator==(const char* str) const;
    bool operator==(const std::string& str) const;
    bool operator==(const std::string_view& view) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if strings have different contents
     */
    bool operator!=(const PdfString& rhs) const;
    bool operator!=(const char* str) const;
    bool operator!=(const std::string& str) const;
    bool operator!=(const std::string_view& view) const;

private:
    PdfString(const std::shared_ptr<std::string> &data, bool isHex);

    /** Construct a new PdfString from a 0-terminated string.
     * 
     *  The input string will be copied.
     *  if m_bHex is true the copied data will be hex-encoded.
     *
     *  \param pszString the string to copy, must not be nullptr
     *  \param lLen length of the string data to copy
     *  
     */
    void initFromUtf8String(const std::string_view& view);
    void evaluateString() const;
    bool isValidText() const;
    static bool canPerformComparison(const PdfString& lhs, const PdfString& rhs);

private:
    enum class StringState : uint8_t
    {
        RawBuffer,
        PdfDocEncoding,
        Unicode
    };

private:
    std::shared_ptr<std::string> m_data;
    StringState m_state;
    bool m_isHex;    // This string is converted to hex during writing it out
};

}

#endif // PDF_STRING_H
