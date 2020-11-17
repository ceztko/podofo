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

#ifndef PDF_FONT_H
#define PDF_FONT_H

#include <ostream>
#include <podofo/base/PdfTextState.h>
#include <podofo/base/PdfDefines.h>
#include <podofo/base/PdfName.h>
#include <podofo/base/PdfEncodingFactory.h>
#include "PdfElement.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfObject;
class PdfPage;
class PdfWriter;



/** Before you can draw text on a PDF document, you have to create
 *  a font object first. You can reuse this font object as often
 *  as you want.
 *
 *  Use PdfDocument::CreateFont to create a new font object.
 *  It will choose a correct subclass using PdfFontFactory.
 *
 *  This is only an abstract base class which is implemented
 *  for different font formats.
 */
class PODOFO_DOC_API PdfFont : public PdfElement
{
    friend class PdfFontFactory;

public:

    /** Create a new PdfFont object which will introduce itself
     *  automatically to every page object it is used on.
     *
     *  The font has a default font size of 12.0pt.
     *
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pParent parent of the font object
     *
     */
    PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent );

    /** Create a PdfFont based on an existing PdfObject
     *  \param pMetrics pointer to a font metrics object. The font in the PDF
     *         file will match this fontmetrics object. The metrics object is
     *         deleted along with the font.
     *  \param pEncoding the encoding of this font. The font will take ownership of this object
     *                   depending on pEncoding->IsAutoDelete()
     *  \param pObject an existing PdfObject
     */
    PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject );

    virtual ~PdfFont();

    /** Write a string to a PdfStream in a format so that it can
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param rsString a unicode or ansi string which will be displayed
     *  \param pStream the string will be appended to pStream without any leading
     *                 or following whitespaces.
     */
    void WriteStringToStream(const std::string_view& rsString, PdfStream * pStream);

    /** Write a string to a PdfStream in a format so that it can
     *  be used with this font.
     *  This is used by PdfPainter::DrawText to display a text string.
     *  The following PDF operator will be Tj
     *
     *  \param rsString a unicode or ansi string which will be displayed
     *  \param rStream the string will be appended to the stream without any leading
     *                 or following whitespaces.
     */
    virtual void WriteStringToStream(const std::string_view& rsString, std::ostream & rStream );

    // Peter Petrov 24 September 2008
    /** Embeds the font into PDF page
     *
     */
    virtual void EmbedFont();

    /** Remember the glyphs used in the string in case of subsetting
     *
     *  \param sText the text string which should be printed (is not allowed to be NULL!)
     *  \param lStringLen draw only lLen characters of pszText
     *
     *  Only call if IsSubsetting() returns true. Might throw an exception otherwise.
     *
     *  \see IsSubsetting
     */
    virtual void AddUsedSubsettingGlyphs(const std::string_view& sText, size_t lStringLen);

    /** Remember the glyphname in case of subsetting
     *
     *  \param pszGlyphName Name of the glyph to remember
     */
    virtual void AddUsedGlyphname( const char * pszGlyphName );

    /** Embeds pending subset-font into PDF page
     *  Only call if IsSubsetting() returns true. Might throw an exception otherwise.
     *
     *  \see IsSubsetting
     */
    virtual void EmbedSubsetFont();

    /** Retrieve the width of a given text string in PDF units when
     *  drawn with the current font
     *  \param pszText a text string of which the width should be calculated
     *  \param nLength if != 0 only the width of the nLength first characters is calculated
     *  \returns the width in PDF units
     */
    double StringWidth(const std::string_view& view, const PdfTextState& state) const;

    /** Retrieve the width of the given character in PDF units in the current font
     *  \param c character
     *  \returns the width in PDF units
     */
    double CharWidth(char32_t ch, const PdfTextState& state) const;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    double GetLineSpacing(const PdfTextState& state) const;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    double GetUnderlineThickness(const PdfTextState& state) const;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetUnderlinePosition(const PdfTextState& state) const;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    double GetStrikeOutPosition(const PdfTextState& state) const;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    double GetStrikeOutThickness(const PdfTextState& state) const;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    double GetAscent(const PdfTextState& state) const;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    double GetDescent(const PdfTextState& state) const;

public:
    /** Check if this is a subsetting font.
     * \returns true if this is a subsetting font
     */
    inline bool IsSubsetting() const { return m_bIsSubsetting; }

    /** Set the underlined property of the font
     *  \param bUnder if true any text drawn with this font
     *                by a PdfPainter will be underlined.
     *  Default is false
     */
    inline void SetUnderlined(bool bUnder) { m_bUnderlined = bUnder; }

    /** \returns true if the font is underlined
     *  \see IsBold
     *  \see IsItalic
     */
    inline bool IsUnderlined() const { return m_bUnderlined; }

    /** \returns true if this font is bold
     *  \see IsItalic
     *  \see IsUnderlined
     */
    inline bool IsBold() const { return m_bBold; }

    /** \returns true if this font is italic
     *  \see IsBold
     *  \see IsUnderlined
     */
    inline bool IsItalic() const { return m_bItalic; }

    /** Set the strikeout property of the font
     *  \param bStrikeOut if true any text drawn with this font
     *                    by a PdfPainter will be strikedout.
     *  Default is false
     */
    inline void SetStrikeOut(bool bStrikeOut) { m_bStrikedOut = bStrikeOut; }

    /** \returns true if the font is striked out
     */
    inline bool IsStrikeOut() const { return m_bStrikedOut; }

    /** Returns the identifier of this font how it is known
     *  in the pages resource dictionary.
     *  \returns PdfName containing the identifier (e.g. /Ft13)
     */
    inline const PdfName& GetIdentifier() const { return m_Identifier; }

    /** Returns a reference to the fonts encoding
     *  \returns a PdfEncoding object.
     */
    inline const PdfEncoding* GetEncoding() const { return m_pEncoding; }

    /** Returns a handle to the fontmetrics object of this font.
     *  This can be used for size calculations of text strings when
     *  drawn using this font.
     *  \returns a handle to the font metrics object
     */
    inline const PdfFontMetrics* GetFontMetrics() const { return m_pMetrics; }

protected:
    /** Get the base font name of this font
     *
     *  \returns the base font name
     */
    inline const PdfName& GetBaseFont() const { return m_BaseFont; }


    const PdfEncoding* const m_pEncoding;
    PdfFontMetrics*          m_pMetrics;

    bool  m_bBold;
    bool  m_bItalic;
    bool  m_bUnderlined;
    bool  m_bStrikedOut;

    bool  m_bWasEmbedded;
    bool m_isBase14;
    bool m_bIsSubsetting;
    PdfName m_Identifier;

    /** Used to specify if this represents a bold font
     *  \param bBold if true this is a bold font.
     *
     *  \see IsBold
     *
     *  This can be called by PdfFontFactory to tell this font
     *  object that it belongs to a bold font.
     */
    virtual void SetBold( bool bBold );

    /** Used to specify if this represents an italic font
     *  \param bItalic if true this is an italic font.
     *
     *  \see IsItalc
     *
     *  This can be called by PdfFontFactory to tell this font
     *  object that it belongs to an italic font.
     */
    virtual void SetItalic( bool bItalic );


 private:
    /** default constructor, not implemented
     */
    PdfFont(void);
    /** copy constructor, not implemented
     */
    PdfFont(const PdfFont& rhs);

    /** Initialize all variables
    */
    void InitVars();
 
    PdfName m_BaseFont;
};

};

#endif // PDF_FONT_H

