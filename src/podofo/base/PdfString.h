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

#ifndef _PDF_STRING_H_
#define _PDF_STRING_H_

#include "PdfDefines.h"
#include "PdfDataType.h"
#include "PdfRefCountedBuffer.h"

namespace PoDoFo {

#define PDF_STRING_BUFFER_SIZE 24

class PdfEncoding;
class PdfOutputDevice;

enum class EPdfStringConversion
{
    Strict,
    Lenient
};


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

    /** Construct a new PdfString from a std::string. 
     *  The input string will be copied.
     *  If the first to bytes of the string are 0xFE and 0xFF
     *  this string is treated as UTF-16BE encoded unicode string.
     *
     *  \param sString the string to copy
     *  \param pEncoding the encoding of this string, if it is no unicode string.
     *         This is ignored for unicode strings. If nullptr, PdfDocEncoding will be used as a default.
     */
    PdfString( const std::string& sString, const PdfEncoding * const pEncoding = nullptr );

    /** Construct a new PdfString from a 0-terminated C-style string.
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     *  \param pEncoding the encoding of this string, if it is no unicode string.
     *         This is ignored for unicode strings. If nullptr, PdfDocEncoding will be used as a default.
     */
    PdfString( const char* pszString, const PdfEncoding * const pEncoding = nullptr );

#ifdef WIN32
    /** Construct a new PdfString from a 0-terminated C-style string.
     *  The input string will be copied.
     *
     *  \param pszString the string to copy
     */
    PdfString(const wchar_t* pszString);
    PdfString( const wchar_t* pszString, size_t lLen);
#endif

    /** Construct a new PdfString from a string. 
     *  The input string will be copied.
     *  If the first two bytes of the string are 0xFE and 0xFF
     *  this string is treated as UTF-16BE encoded unicode string.
     *
     *  \param pszString the string to copy
     *  \param lLen length of the string data to encode
     *  \param bHex if true the data will be hex-encoded during writing out the string and IsHex() will return true.
     *  \param pEncoding the encoding of this string, if it is no unicode string.
     *         This is ignored for unicode strings. If nullptr, PdfDocEncoding will be used as a default.
     */
    PdfString( const char* pszString, size_t lLen, bool bHex = false, const PdfEncoding * const pEncoding = nullptr );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 an UTF-8 encoded string.
     */
    PdfString( const pdf_utf8* pszStringUtf8 );

    /** Construct a new PdfString from an UTF-16BE encoded zero-terminated C-style string.
     *
     *  \param pszStringUtf16 an UTF-16BE encoded string.
     */
    PdfString(const pdf_utf16be* pszStringUtf16);

    /** Construct a new PdfString from an UTF-8 encoded string.
     *  
     *  The string is converted to UTF-16BE internally.
     *
     *  \param pszStringUtf8 a UTF-8 encoded string.
     *  \param lLen number of bytes to convert
     */
    PdfString( const pdf_utf8* pszStringUtf8, size_t lLen );

    /** Construct a new PdfString from an UTF-16BE encoded zero-terminated string.
     *
     *  \param pszStringUtf16 a UTF-16BE encoded string.
     *  \param lLen number of words to convert
     */
    PdfString( const pdf_utf16be* pszStringUtf16, size_t lLen );

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     */
    PdfString( const PdfString & rhs );

    /** Construct a new PdfString from an UTF-8 encoded string.
     *
     *  \param str an UTF-8 encoded string.
     */
    static PdfString FromUtf8String( const std::string &str );

    /** Construct a new hex PdfString from a buffer
     *
     *  \param pbuffer char buffer
     *  \param lLen length of the buffer
     */
    static PdfString CreateHexString( const char* pbuffer, size_t lLen );

    /** Construct a new hex PdfString from a buffer
     *
     *  \param buffer string containing the buffer
     */
    static PdfString CreateHexString( const std::string &buffer );

    /** The string is valid if no error in the constructor has occurred.
     *  The default constructor PdfString() creates an invalid string, as do
     *  other constructors when passed a nullptr char* or nullptr wchar_t*.
     *  PdfString::StringNull uses the default constructor so is also invalid.
     *  If it is valid it is safe to call all the other member functions.
     *  \returns true if this is a valid initialized PdfString
     */
    bool IsValid() const;

    /** Check if this is a hex string.
     *  
     *  If true the data will be hex-encoded when the string is written to
     *  a PDF file.
     *
     *  \returns true if this is a hex string.
     *  \see GetString() will return the raw string contents (not hex-encoded)
     */
    inline bool IsHex () const { return m_bHex; }

    /**
     * PdfStrings are either Latin1-encoded or UTF-16BE-encoded unicode strings.
     *
     * This function returns true if this is a unicode string object.
     *
     * \returns true if this is a unicode string.
     */
    inline bool IsUnicode() const { return m_bUnicode; }

    /** The contents of the string can be read by this function.
     *
     *  The returned data is never hex-encoded and may contain '\0' bytes.
     *
     *  If IsUnicode() returns true, the return value
     *  points to a UTF-16BE string buffer with GetCharacterLength()
     *  characters (GetLength() bytes without the terminating '\0').
     *  Better use GetUnicode() in this case.
     *) 
     *  \returns the string's contents which are guaranteed to be zero-terminated
     *           but might also contain '\0' bytes in the string.
     *  \see IsHex
     *  \see IsUnicode
     *  \see GetLength
     */
    const char* GetString() const;

    /** The contents of the string can be read by this function.
     *
     *  The returned data is never hex-encoded any may contain '\0' bytes.
     *
     *  If IsUnicode() returns true, the return value points to a UTF-16BE
     *  string with GetCharacterLength() characters (GetLength() bytes without
     *  the terminating '\0' bytes).
     *
     *  If IsUnicode() returns false, the return value
     *  points to a PdfDocEncoding string buffer with GetLength()
     *  characters (so may well not be valid as UTF-16BE).
     *  Better use GetString() in this case.
     * 
     *  \returns the string's contents which are guaranteed to be zero-terminated
     *           but might also contain '\0' bytes in the string,
     *           returns nullptr if PdfString::IsValid() returns false.
     *
     *  \see IsHex
     *  \see IsUnicode
     *  \see GetUnicodeLength
     */
    const pdf_utf16be* GetUnicode() const;

    /** The contents of the string as UTF-8 string.
     *
     *  The string's contents are always returned as
     *  UTF-8 by this function. Works for unicode strings 
     *  and for non-unicode strings.
     *
     *  This is the preferred way to access the string's contents.
     *
     *  \returns the string's contents always as UTF-8,
     *           returns nullptr if PdfString::IsValid() returns false
     */
    const std::string & GetStringUtf8() const;

#ifdef WIN32
    /** The contents of the string as wide-character string.
     *
     *  \returns the string contents as wide-character string.
     *           returns an empty string if PdfString::IsValid() returns false
     */
    const std::wstring GetStringW() const;
#endif // WIN32

    /** The length of the string data returned by GetString() 
     *  in bytes not including terminating zero ('\0') bytes.
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    size_t GetLength() const;

    /** The length of the string data returned by GetUnicode() 
     *  in characters not including the terminating zero ('\0') bytes.
     *
     *  \returns the length of the string,
     *           returns zero if PdfString::IsValid() returns false
     *
     *  \see GetCharacterLength to determine the number of characters in the string
     */
    size_t GetUnicodeLength() const;

    /** Get the number of characters in the string.
     *  
     *  This function returns the correct number of characters in the string
     *  for unicode and ANSI strings. Always use this method if you want to 
     *  know the number of characters in the string
     *  as GetLength() will return the number of bytes used for unicode strings!
     *
     * 
     *  \returns the number of characters in the string,
     *           returns zero if PdfString::IsValid() returns false
     */
    size_t GetCharacterLength() const;

    /** Write this PdfString in PDF format to a PdfOutputDevice.
     *  
     *  \param pDevice the output device.
     *  \param eWriteMode additional options for writing this object
     *  \param pEncrypt an encryption object which is used to encrypt this object,
     *                  or nullptr to not encrypt this object
     */
    void Write ( PdfOutputDevice& pDevice, EPdfWriteMode eWriteMode, const PdfEncrypt* pEncrypt ) const override;

    /** Copy an existing PdfString 
     *  \param rhs another PdfString to copy
     *  \returns this object
     */
    const PdfString & operator=( const PdfString & rhs );

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare with
     *  \returns this object
     */
    bool operator>( const PdfString & rhs ) const;

    /** Compare two PdfString objects
     *  \param rhs another PdfString to compare with
     *  \returns this object
     */
    bool operator<( const PdfString & rhs ) const;

    /** Comparison operator
     *
     *  UTF-8 and UTF-16BE encoded strings of the same data compare equal. Whether
     *  the string will be written out as hex is not considered - only the real "text"
     *  is tested for equality.
     *
     *  \param rhs compare to this string object
     *  \returns true if both strings have the same contents 
     */
    bool operator==( const PdfString & rhs ) const;

    /** Comparison operator
     *  \param rhs compare to this string object
     *  \returns true if strings have different contents
     */
    bool operator!=(const PdfString& rhs) const { return !operator==(rhs); }

    /** Converts this string to a unicode string
     *  
     *  If IsUnicode() returns true a copy of this string is returned,
     *  otherwise the string data is converted to UTF-16BE and returned.
     *
     *  \returns a unicode version of this string,
     *           returns *this if if PdfString::IsValid() returns false
     */
    PdfString ToUnicode() const;

	 /** Returns internal buffer; do not free it, it's owned by the PdfString
	  *
	  * \returns internal buffer; do not free it, it's owned by the PdfString
      *          (zero size buffer, internally nullptr, if PdfString::IsValid()
      *          returns false).
	  */
	 PdfRefCountedBuffer &GetBuffer(void);
     const PdfRefCountedBuffer &GetBuffer(void) const;

    static const PdfString StringNull;

    /** Swap the bytes in the buffer (UTF-16BE -> UTF-16LE)
     *  \param pBuf buffer
     *  \param lLen length of buffer
     */
    static void SwapBytes(char* pBuf, size_t lLen);

private:
    /** Set hex-encoded data as the strings data.
     *  \param pszHex must be hex-encoded data.
     *  \param lLen   length of the hex-encoded data.
     *  \param pEncrypt if !nullptr, assume the hex data is encrypted and should be decrypted after hex-decoding.
     */
    void SetHexData(const char* pszHex, size_t lLen, PdfEncrypt* pEncrypt = nullptr);

    /** Construct a new PdfString from a 0-terminated string.
     * 
     *  The input string will be copied.
     *  if m_bHex is true the copied data will be hex-encoded.
     *
     *  \param pszString the string to copy, must not be nullptr
     *  \param lLen length of the string data to copy
     *  
     */
    void Init( const char* pszString, size_t lLen );

    /** Construct a new PdfString from a UTF-8 string. 
     * 
     *  The input string will be copied and converted to UTF-16BE.
     *
     *  \param pszStringUtf8 the string to copy, must not be nullptr
     *  \param lLen number of bytes of the string data to copy
     *  
     */
    void InitFromUtf8( const pdf_utf8* pszStringUtf8, size_t lLen );

    /** Initialise the data member containing a
     *  UTF-8 version of this string.
     *
     *  This is only done once and only if necessary.
     */
    void InitUtf8();

#ifdef WIN32
    void setFromWchar_t(const wchar_t* pszString, size_t lLen);
#endif
 private:
    static const char        s_pszUnicodeMarker[];   ///< The unicode marker used to indicate unicode strings in PDF
    static const char*       s_pszUnicodeMarkerHex;  ///< The unicode marker converted to hex
    static const pdf_utf16be s_cPdfDocEncoding[256]; ///< conversion table from PDFDocEncoding to UTF-16
    static const char * const m_escMap;              ///< Mapping of escape sequences to their value

 private:
    PdfRefCountedBuffer m_buffer;                    ///< String data (always binary), may contain '\0' bytes

    bool                m_bHex;                      ///< This string is converted to hex during writing it out
    bool                m_bUnicode;                  ///< This string contains unicode data

    std::string         m_sUtf8;                     ///< The UTF-8 version of the string's contents.
    const PdfEncoding*  m_pEncoding;                 ///< Encoding for non-unicode strings. nullptr for unicode strings.
};

};


#endif // _PDF_STRING_H_
