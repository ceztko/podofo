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

#ifndef _PDF_TOKENIZER_H_
#define _PDF_TOKENIZER_H_

#include "PdfDefines.h"
#include "PdfRefCountedBuffer.h"
#include "PdfRefCountedInputDevice.h"

#include <deque>
#include <sstream>

namespace PoDoFo {

class PdfEncrypt;
class PdfVariant;

enum class EPdfTokenType
{
    Unknown = 0,
    Delimiter,
    Literal,
};

typedef std::pair<std::string,EPdfTokenType> TTokenizerPair;
typedef std::deque<TTokenizerPair>           TTokenizerQueque;
typedef TTokenizerQueque::iterator           TITokenizerQueque;
typedef TTokenizerQueque::const_iterator     TCITokenizerQueque;


/**
 * A simple tokenizer for PDF files and PDF content streams
 */
class PODOFO_API PdfTokenizer
{
    friend class PdfParser;
    friend class PdfParserObject;

protected:
    // This enum differs from regular PdfDataType in the sense
    // it enumerates only data types that can be determined literally
    // by the tokenization and specify better if the strings literals
    // are regular or hex strings
    enum class EPdfLiteralDataType
    {
        Unknown = 0,
        Bool,
        Number,
        Real,
        String,
        HexString,
        Name,
        Array,
        Dictionary,
        Null,
        Reference,
    };

public:
    PdfTokenizer();
    PdfTokenizer(const PdfRefCountedBuffer& rBuffer);
    ~PdfTokenizer();

    /** Reads the next token from the current file position
     *  ignoring all comments.
     *
     *  \param[out] pszToken On true return, set to a pointer to the read
     *                     token (a nullptr-terminated C string). The pointer is
     *                     to memory owned by PdfTokenizer and must NOT be
     *                     freed.  The contents are invalidated on the next
     *                     call to tryReadNextToken(..) and by the destruction of
     *                     the PdfTokenizer. Undefined on false return.
     *
     *  \param[out] peType On true return, if not nullptr the type of the read token
     *                     will be stored into this parameter. Undefined on false
     *                     return.
     * 
     *  \returns           True if a token was read, false if there are no
     *                     more tokens to read.
     */
    bool TryReadNextToken(const PdfRefCountedInputDevice& device, std::string_view& pszToken, EPdfTokenType* peType = nullptr);

    /** Reads the next token from the current file position
     *  ignoring all comments and compare the passed token
     *  to the read token.
     *
     *  If there is no next token available, throws UnexpectedEOF.
     *
     *  \param pszToken a token that is compared to the 
     *                  read token
     *
     *  \returns true if the read token equals the passed token.
     */
    bool IsNextToken(const PdfRefCountedInputDevice& device, const std::string_view& pszToken);

    /** Read the next number from the current file position
     *  ignoring all comments.
     *
     *  Raises NoNumber exception if the next token is no number, and
     *  UnexpectedEOF if no token could be read. No token is consumed if
     *  NoNumber is thrown.
     *
     *  \returns a number read from the input device.
     */
    int64_t ReadNextNumber(const PdfRefCountedInputDevice& device);

    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an UnexpectedEOF exception if there is no variant left in the
     *  file.
     *
     *  \param rVariant write the read variant to this value
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(const PdfRefCountedInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt = nullptr);

    /** Returns true if the given character is a whitespace 
     *  according to the pdf reference
     *
     *  \returns true if it is a whitespace character otherwise false
     */
    static bool IsWhitespace(const unsigned char ch);

    /** Returns true if the given character is a delimiter
     *  according to the pdf reference
     *
     *  \returns true if it is a delimiter character otherwise false
     */
    static bool IsDelimiter(const unsigned char ch);

    /**
     * True if the passed character is a regular character according to the PDF
     * reference (Section 3.1.1, Character Set); ie it is neither a white-space
     * nor a delimeter character.
     */
    static bool IsRegular(const unsigned char ch);

    /**
     * True if the passed character is within the generally accepted "printable"
     * ASCII range.
     */
    static bool IsPrintable(const unsigned char ch);

    /**
     * Get the hex value from a static map of a given hex character (0-9, A-F, a-f).
     *
     * \param ch hex character
     *
     * \returns hex value or HEX_NOT_FOUND if invalid
     *
     * \see HEX_NOT_FOUND
     */
    static int GetHexValue(const unsigned char ch);

    /**
     * Constant which is returned for invalid hex values.
     */
    static constexpr unsigned int HEX_NOT_FOUND = std::numeric_limits<unsigned int>::max();

protected:
    /** Read the next variant from the current file position
     *  ignoring all comments.
     *
     *  Raises an exception if there is no variant left in the file.
     *
     *  \param pszToken a token that has already been read
     *  \param eType type of the passed token
     *  \param rVariant write the read variant to this value
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadNextVariant(const PdfRefCountedInputDevice& device, const std::string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt );
    bool TryReadNextVariant(const PdfRefCountedInputDevice& device, const std::string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant, PdfEncrypt* pEncrypt);

    /** Add a token to the queue of tokens.
     *  tryReadNextToken() will return all enqueued tokens first before
     *  reading new tokens from the input device.
     *
     *  \param pszToken string of the token
     *  \param eType type of the token
     *
     *  \see tryReadNextToken
     */
    void EnqueueToken(const std::string_view& pszToken, EPdfTokenType eType);

    /** Determine the possible datatype of a token.
     *  Numbers, reals, bools or nullptr values are parsed directly by this function
     *  and saved to a variant.
     *
     *  \returns the expected datatype
     */
    EPdfLiteralDataType DetermineDataType(const PdfRefCountedInputDevice& device, const std::string_view& pszToken, EPdfTokenType eType, PdfVariant& rVariant );

    /** Read a dictionary from the input device
     *  and store it into a variant.
     * 
     *  \param rVariant store the dictionary into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadDictionary(const PdfRefCountedInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt );

    /** Read an array from the input device
     *  and store it into a variant.
     * 
     *  \param rVariant store the array into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadArray(const PdfRefCountedInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt );

    /** Read a string from the input device
     *  and store it into a variant.
     * 
     *  \param rVariant store the string into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadString(const PdfRefCountedInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt );

    /** Read a hex string from the input device
     *  and store it into a variant.
     *
     *  \param rVariant store the hex string into this variable
     *  \param pEncrypt an encryption object which is used to decrypt strings during parsing
     */
    void ReadHexString(const PdfRefCountedInputDevice& device, PdfVariant& rVariant, PdfEncrypt* pEncrypt);

    /** Read a name from the input device
     *  and store it into a variant.
     * 
     *  Throws UnexpectedEOF if there is nothing to read.
     *
     *  \param rVariant store the name into this variable
     */
    void ReadName(const PdfRefCountedInputDevice& device, PdfVariant& rVariant );

    PdfRefCountedBuffer& GetBuffer() { return m_buffer; }

private:
    bool tryReadDataType(const PdfRefCountedInputDevice& device, EPdfLiteralDataType eDataType, PdfVariant& rVariant, PdfEncrypt* pEncrypt);

    /** Read a hex string from the input device
     *  and store it into a vector.
     *
     *  \param rVecBuffer store the hex string into this variable
     */
    void readHexString(const PdfRefCountedInputDevice& device, std::vector<char>& rVecBuffer);

private:
    // 256-byte array mapping character ordinal values to a truth value
    // indicating whether or not they are whitespace according to the PDF
    // standard.
    static const char * const s_delimiterMap;
    static const char * const s_whitespaceMap;
    static const char s_octMap[]; ///< Map of bool values, if a certain char
                                  ///< is a valid octal digit
    static const char * const s_escMap; ///< Mapping of escape sequences to their value
    static const char * const s_hexMap; ///< Mapping of hex characters to their value

    PdfRefCountedBuffer m_buffer;
    TTokenizerQueque m_deqQueque;

    // A vector which is used as a buffer to read strings.
    // It is a member of the class to avoid reallocations while parsing.
    std::vector<char> m_vecBuffer; // we use a vector instead of a string
                                   // because we might read a unicode
                                   // string which is allowed to contain 0 bytes.

    /// An istringstream which is used
    /// to read double values instead of strtod
    /// which is locale depend.
    std::istringstream m_doubleParser;
};

};

#endif // _PDF_TOKENIZER_H_
