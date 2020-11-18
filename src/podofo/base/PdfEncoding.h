/***************************************************************************
 *   Copyright (C) 2007 by Dominik Seichter                                *
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

#ifndef _PDF_ENCODING_H_
#define _PDF_ENCODING_H_

#include <mutex>
#include <unordered_map>

#include "PdfDefines.h"
#include "PdfName.h"
#include "PdfString.h"
#include "PdfVariant.h"

namespace PoDoFo {

class PdfDictionary;
class PdfFont;
class PdfObject;
class PdfStream;

/** 
 * A PdfEncoding is in PdfFont to transform a text string
 * into a representation so that it can be displayed in a
 * PDF file.
 *
 * PdfEncoding can also be used to convert strings from a
 * PDF file back into a PdfString.
 */
class PODOFO_API PdfEncoding
{
protected:

    struct CodeIdentity
    {
        // RangeSize example <cd> -> 1, <00cd> -> 2
        unsigned RangeSize;
        uint32_t Code;
    };


    struct HashCodeIdentity
    {
        std::size_t operator()(const CodeIdentity &identity) const
        {
            return identity.RangeSize << 24 | identity.Code;
        };
    };

    struct EqualCodeIdentity
    {
        bool operator()(const CodeIdentity &lhs, const CodeIdentity &rhs) const
        {
            return lhs.RangeSize == rhs.RangeSize && lhs.Code == rhs.Code;
        }
    };

    // pp. 474-475 of PdfReference 1.7 "The value of dstString can be a string of up to 512 bytes"
    typedef std::unordered_map<CodeIdentity, std::string, HashCodeIdentity, EqualCodeIdentity> UnicodeMap;

protected:
    /** 
     *  Create a new PdfEncoding.
     *
     *  \param nFirstChar the first supported character code 
     *                    (either a byte value in the current encoding or a unicode value)
     *  \param nLastChar the last supported character code, must be larger than nFirstChar 
     *                    (either a byte value in the current encoding or a unicode value)
     *  \param toUnicode /ToUnicode object, if available
     *
     */
    PdfEncoding( int nFirstChar, int nLastChar, const PdfObject* toUnicode = nullptr );

    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
    virtual const PdfName & GetID() const = 0;

public:
    virtual ~PdfEncoding();

    /** Comparison operator.
     *
     *  \param rhs the PdfEncoding to which this encoding should be compared
     *
     *  \returns true if both encodings are the same.
     */
    bool operator==( const PdfEncoding & rhs ) const;

    /** Comparison operator.
     *
     *  \param rhs the PdfEncoding to which this encoding should be compared
     *
     *  \returns true if this encoding is less than the specified.
     */
    bool operator<( const PdfEncoding & rhs ) const;

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    virtual void AddToDictionary( PdfDictionary & rDictionary ) const = 0;

    std::string ConvertToUnicode(const PdfString& rEncodedString) const;

    /** Convert a string that is encoded with this encoding
     *  to an unicode PdfString.
     *
     *  \param rEncodedString a string encoded by this encoding. 
     *         Usually this string was read from a content stream.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an unicode PdfString.
     */
    virtual std::string ConvertToUnicode(const std::string_view& rEncodedString) const;

    /** Convert a unicode PdfString to a string encoded with this encoding.
     *
     *  \param rString an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfRefCountedBuffer. The PdfRefCountedBuffer is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned buffer must not be a unicode string.
     */
    virtual std::string ConvertToEncoding(const std::string_view& rString) const;

    virtual bool IsAutoDelete() const = 0;

    virtual bool IsSingleByteEncoding() const = 0;

    /** 
     * \returns the first character code that is defined for this encoding
     */
    inline int GetFirstChar() const { return m_nFirstCode; }

    /** 
     * \returns the last character code that is defined for this encoding
     */
    inline int GetLastChar() const { return m_nLastCode; }

    /** Get the unicode character code for this encoding
     *  at the position nIndex. nIndex is a position between
     *  GetFirstChar() and GetLastChar()
     *
     *  \param nIndex character code at position index
     *  \returns unicode character code 
     * 
     *  \see GetFirstChar 
     *  \see GetLastChar
     *
     *  Will throw an exception if nIndex is out of range.
     */
    virtual char32_t GetCharCode( int nIndex ) const = 0;

public:
    bool IsToUnicodeLoaded() const { return m_bToUnicodeIsLoaded; }

private:
    static std::string HandleBaseFontString(const PdfString& str);
    static void HandleBaseFontRange(UnicodeMap& map, uint32_t srcCodeLo, const std::string& dstCodeLo, unsigned codeSize, unsigned rangeSize);

protected:
    static uint32_t GetCodeFromVariant(const PdfVariant &var);
    static uint32_t GetCodeFromVariant(const PdfVariant &var, unsigned &codeSize);
    static std::string convertToEncoding(const std::string_view& rString, const UnicodeMap &map);
    static std::string convertToUnicode(const std::string_view& rString, const UnicodeMap &map, unsigned maxCodeRangeSize);
    static void ParseCMapObject(const PdfStream& stream, UnicodeMap &map, char32_t &firstChar, char32_t &lastChar, unsigned &maxCodeRangeSize);

private:
     bool m_bToUnicodeIsLoaded; // If true, ToUnicode has been parsed
     char32_t m_nFirstCode;     // The first defined character code
     char32_t m_nLastCode;      // The last defined character code
     unsigned m_maxCodeRangeSize;    // Size of in bytes of the bigger code range
     UnicodeMap m_toUnicode;
};

/**
 * A common base class for standard PdfEncoding which are
 * known by name.
 *
 *  - PdfDocEncoding (only use this for strings which are not printed 
 *                    in the document. This is for meta data in the PDF).
 *  - MacRomanEncoding
 *  - WinAnsiEncoding
 *  - MacExpertEncoding
 *  - StandardEncoding
 *  - SymbolEncoding
 *  - ZapfDingbatsEncoding
 *
 *  \see PdfWinAnsiEncoding
 *  \see PdfMacRomanEncoding
 *  \see PdfMacExportEncoding
 *..\see PdfStandardEncoding
 *  \see PdfSymbolEncoding
 *  \see PdfZapfDingbatsEncoding
 *
 */
class PODOFO_API PdfSimpleEncoding : public PdfEncoding
{
public:
    /*
     *  Create a new simple PdfEncoding which uses 1 byte.
     *
     *  \param rName the name of a standard PdfEncoding
     *
     *  As of now possible values for rName are:
     *  - MacRomanEncoding
     *  - WinAnsiEncoding
     *  - MacExpertEncoding
     *  - StandardEncoding
     *  - SymbolEncoding
     *  - ZapfDingbatsEncoding
     *
     *  \see PdfWinAnsiEncoding
     *  \see PdfMacRomanEncoding
     *  \see PdfMacExportEncoding
     *  \see PdfStandardEncoding
     *  \see PdfSymbolEncoding
     *  \see PdfZapfDingbatsEncoding
     *
     *  This will allocate a table of 65535 short values
     *  to make conversion from unicode to encoded strings
     *  faster. As this requires a lot of memory, make sure that
     *  only one object of a certain encoding exists at one
     *  time, which is no problem as all methods are const anyways!
     *
     */
    PdfSimpleEncoding( const PdfName & rName );

    ~PdfSimpleEncoding();

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    void AddToDictionary( PdfDictionary & rDictionary ) const override;

    /** Convert a string that is encoded with this encoding
     *  to an unicode PdfString.
     *
     *  \param rEncodedString a string encoded by this encoding. 
     *         Usually this string was read from a content stream.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an unicode PdfString.
     */
    std::string ConvertToUnicode(const std::string_view& rEncodedString) const override;

    /** Convert a unicode PdfString to a string encoded with this encoding.
     *
     *  \param rString an unicode PdfString.
     *  \param pFont the font for which this string is converted
     *
     *  \returns an encoded PdfRefCountedBuffer. The PdfRefCountedBuffer is treated as a series of bytes
     *           and is allowed to have 0 bytes. The returned buffer must not be a unicode string.
     */
    std::string ConvertToEncoding(const std::string_view& rString) const override;

    /** Get the unicode character code for this encoding
     *  at the position nIndex. nIndex is a position between
     *  GetFirstChar() and GetLastChar()
     *
     *  \param nIndex character code at position index
     *  \returns unicode character code 
     * 
     *  \see GetFirstChar 
     *  \see GetLastChar
     *
     *  Will throw an exception if nIndex is out of range.
     */
    char32_t GetCharCode( int nIndex ) const override;

    char GetUnicodeCharCode(char32_t unicodeValue) const;

    /**
     * PdfSimpleEncoding subclasses are usuylla not auto-deleted, as
     * they are allocated statically only once.
     *
     * \returns true if this encoding should be deleted automatically with the
     *          font.
     *
     * \see PdfFont::WinAnsiEncoding
     * \see PdfFont::MacRomanEncoding
     */
    inline bool IsAutoDelete() const override { return false; }

    /**
     *  \returns true if this is a single byte encoding with a maximum of 256 values.
     */
    inline bool IsSingleByteEncoding() const override { return true; }

    /** Get the name of this encoding.
     *
     *  \returns the name of this encoding.
     */
    inline const PdfName& GetName() const { return m_name; }

 private:
    /** Initialize the internal table of mappings from unicode code points
     *  to encoded byte values.
     */
    void InitEncodingTable();

 protected:

    /** Get a unique ID for this encoding
     *  which can used for comparisons!
     *
     *  \returns a unique id for this encoding!
     */
     inline const PdfName& GetID() const override { return m_name; }

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    virtual const char32_t* GetToUnicodeTable() const = 0;

 protected:
    std::mutex m_mutex;   ///< Mutex for the creation of the encoding table
    
 private:
    PdfName m_name;           ///< The name of the encoding
    char*   m_pEncodingTable; ///< The helper table for conversions into this encoding
};

/** 
 * The PdfDocEncoding is the default encoding for
 * all strings in PoDoFo which are data in the PDF
 * file.
 *
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::DocEncoding.
 *
 * \see PdfFont::DocEncoding
 */
class PODOFO_API PdfDocEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfDocEncoding
     */
    PdfDocEncoding()
        : PdfSimpleEncoding( PdfName("PdfDocEncoding") )
    { }

public:
    /** Check if the chars in the given utf-8 view are elegible for PdfDocEncofing conversion
     *
     * /param isPdfDocEncoding the given utf-8 string is coincident in PdfDocEncoding representation
     */
    static bool CheckValidUTF8ToPdfDocEcondingChars(const std::string_view &view, bool &isPdfDocEncodingEqual);
    static bool IsPdfDocEncodingCoincidentToUTF8(const std::string_view& view);
    static bool TryConvertUTF8ToPdfDocEncoding(const std::string_view& view, std::string &pdfdocencstr);
    static std::string ConvertUTF8ToPdfDocEncoding(const std::string_view& view);
    static std::string ConvertPdfDocEncodingToUTF8(const std::string_view& view, bool &isUTF8Equal);
    static void ConvertPdfDocEncodingToUTF8(const std::string_view& view, std::string &u8str, bool& isUTF8Equal);

public:
    static const std::unordered_map<uint16_t, char> & GetUTF8ToPdfEncodingMap();

protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

private:
    static const char32_t s_cEncoding[256]; ///< conversion table from DocEncoding to UTF16

};

/** 
 * The WinAnsi Encoding is the default encoding in PoDoFo for 
 * contents on PDF pages.
 *
 * It is also called CP-1252 encoding.
 * This class may be used as base for derived encodings.
 *
 * \see PdfWin1250Encoding
 *
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::WinAnsiEncoding.
 *
 * \see PdfFont::WinAnsiEncoding
 */
class PODOFO_API PdfWinAnsiEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfWinAnsiEncoding
     */
    PdfWinAnsiEncoding()
        : PdfSimpleEncoding( PdfName("WinAnsiEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

    /** Add this encoding object to a dictionary
     *  usually be adding an /Encoding key in font dictionaries.
     *  
     *  This method generates array of differences into /Encoding
     *  dictionary if called from derived class with
     *  different unicode table.
     *
     *  \param rDictionary add the encoding to this dictionary
     */
    void AddToDictionary( PdfDictionary & rDictionary ) const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from WinAnsiEncoding to UTF16

};

/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::MacRomanEncoding.
 *
 * \see PdfFont::MacRomanEncoding
 */
class PODOFO_API PdfMacRomanEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfMacRomanEncoding
     */
    PdfMacRomanEncoding()
        : PdfSimpleEncoding( PdfName("MacRomanEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from MacRomanEncoding to UTF16

};

/** 
 */
class PODOFO_API PdfMacExpertEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfMacExpertEncoding
     */
    inline PdfMacExpertEncoding()
        : PdfSimpleEncoding( PdfName("MacExpertEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from MacExpertEncoding to UTF16

};

// OC 13.08.2010 Neu: StandardEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::StandardEncoding.
 *
 * \see PdfFont::StandardEncoding
 */
class PODOFO_API PdfStandardEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfStandardEncoding
     */
    PdfStandardEncoding()
        : PdfSimpleEncoding( PdfName("StandardEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from StandardEncoding to UTF16

};

// OC 13.08.2010 Neu: SymbolEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::SymbolEncoding.
 *
 * \see PdfFont::SymbolEncoding
 */
class PODOFO_API PdfSymbolEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfSymbolEncoding
     */
    PdfSymbolEncoding()
        : PdfSimpleEncoding( PdfName("SymbolEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from SymbolEncoding to UTF16

};

// OC 13.08.2010 Neu: ZapfDingbatsEncoding
/** 
 * Do not allocate this class yourself, as allocations
 * might be expensive. Try using PdfFont::ZapfDingbats.
 *
 * \see PdfFont::ZapfDingbatsEncoding
 */
class PODOFO_API PdfZapfDingbatsEncoding : public PdfSimpleEncoding {
 public:
   
    /** Create a new PdfZapfDingbatsEncoding
     */
    PdfZapfDingbatsEncoding()
        : PdfSimpleEncoding( PdfName("ZapfDingbatsEncoding") )
    {

    }

 protected:

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from ZapfDingbatsEncoding to UTF16

};

/**
* WINDOWS-1250 encoding
*/
class PODOFO_API PdfWin1250Encoding : public PdfWinAnsiEncoding
{
 public:
   
    /** Create a new PdfWin1250Encoding
     */
    PdfWin1250Encoding()
    {
        m_id = "Win1250Encoding";
    }

 protected:

    inline const PdfName & GetID() const override
    {
        return m_id;
    }

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from Win1250Encoding to UTF16
    PdfName m_id;
};

/**
* ISO-8859-2 encoding
*/
class PODOFO_API PdfIso88592Encoding : public PdfWinAnsiEncoding
{
 public:
   
    /** Create a new PdfIso88592Encoding
     */
    PdfIso88592Encoding()
    {
        m_id = "Iso88592Encoding";
    }

 protected:

    inline const PdfName & GetID() const override
    {
        return m_id;
    }

    /** Gets a table of 256 short values which are the 
     *  big endian unicode code points that are assigned
     *  to the 256 values of this encoding.
     *
     *  This table is used internally to convert an encoded
     *  string of this encoding to and from unicode.
     *
     *  \returns an array of 256 big endian unicode code points
     */
    const char32_t* GetToUnicodeTable() const override;

 private:
    static const char32_t s_cEncoding[256]; ///< conversion table from Iso88592Encoding to UTF16
    PdfName m_id;
};

}; /* namespace PoDoFo */

#endif // _PDF_ENCODING_H_

