/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
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

#ifndef _PDF_FONT_METRICS_H_
#define _PDF_FONT_METRICS_H_

#include "podofo/base/PdfDefines.h"
#include "podofo/base/Pdf3rdPtyForwardDecl.h"
#include "podofo/base/PdfString.h"
#include "podofo/base/PdfEncoding.h"

namespace PoDoFo {

class PdfArray;
class PdfObject;
class PdfVariant;

/**
 * This abstract class provides access
 * to fontmetrics informations.
 */
class PODOFO_DOC_API PdfFontMetrics
{
protected:
    PdfFontMetrics( EPdfFontType eFontType, const char* pszFilename, const char* pszSubsetPrefix );

public:
    virtual ~PdfFontMetrics();

    /** Create a width array for this font which is a required part
     *  of every font dictionary.
     *  \param var the final width array is written to this PdfVariant
     *  \param nFirst first character to be in the array
     *  \param nLast last character code to be in the array
     *  \param pEncoding encoding for correct character widths. If not passed default (latin1) encoding is used
     */
    virtual void GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding = nullptr ) const = 0;

    /** Get the width of a single glyph id
     *
     *  \param nGlyphId id of the glyph
     *  \returns the width of a single glyph id
     */
    virtual double GetGlyphWidth( int nGlyphId ) const = 0;

    /** Get the width of a single named glyph
     *
     *  \param pszGlyphname name of the glyph
     *  \returns the width of a single named glyph
     */
    virtual double GetGlyphWidth( const char* pszGlyphname ) const = 0;

    /** Create the bounding box array as required by the PDF reference
     *  so that it can be written directly to a PDF file.
     *
     *  \param array write the bounding box to this array.
     */
    virtual void GetBoundingBox( PdfArray & array ) const = 0;

    /** Retrieve the line spacing for this font
     *  \returns the linespacing in PDF units
     */
    virtual double GetLineSpacing() const = 0;

    /** Get the width of the underline for the current
     *  font size in PDF units
     *  \returns the thickness of the underline in PDF units
     */
    virtual double GetUnderlineThickness() const = 0;

    /** Return the position of the underline for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetUnderlinePosition() const = 0;

    /** Return the position of the strikeout for the current font
     *  size in PDF units
     *  \returns the underline position in PDF units
     */
    virtual double GetStrikeOutPosition() const = 0;

    /** Get the width of the strikeout for the current
     *  font size in PDF units
     *  \returns the thickness of the strikeout in PDF units
     */
    virtual double GetStrikeOutThickness() const = 0;

    /** Get the ascent of this font in PDF
     *  units for the current font size.
     *
     *  \returns the ascender for this font
     *
     *  \see GetAscent
     */
    virtual double GetAscent() const = 0;

    /** Get the descent of this font in PDF
     *  units for the current font size.
     *  This value is usually negative!
     *
     *  \returns the descender for this font
     *
     *  \see GetDescent
     */
    virtual double GetDescent() const = 0;

    /** Get a pointer to the path of the font file.
     *  \returns a zero terminated string containing the filename of the font file
     */
    const char* GetFilename() const;

    /** Get a pointer to the actual font data - if it was loaded from memory.
     *  \returns a binary buffer of data containing the font data
     */
    virtual const char* GetFontData() const = 0;

    /** Get the length of the actual font data - if it was loaded from memory.
     *  \returns a the length of the font data
     */
    virtual size_t GetFontDataLen() const = 0;

    /** Get a string with the postscript name of the font.
     *  \returns the postscript name of the font or nullptr string if no postscript name is available.
     */
    virtual const char* GetFontname() const = 0;

    /**
     * \returns nullptr or a 6 uppercase letter and "+" sign prefix
     *          used for font subsets
     */
    const char* GetSubsetFontnamePrefix() const;

    /** Get the weight of this font.
     *  Used to build the font dictionay
     *  \returns the weight of this font (500 is normal).
     */
    virtual unsigned int GetWeight() const = 0;

    /** Get the italic angle of this font.
     *  Used to build the font dictionay
     *  \returns the italic angle of this font.
     */
    virtual int GetItalicAngle() const = 0;

    /**
     *  \returns the fonttype of the loaded font
     */
    inline EPdfFontType GetFontType() const { return m_eFontType; }

    /** Get the glyph id for a unicode character
     *  in the current font.
     *
     *  \param lUnicode the unicode character value
     *  \returns the glyhph id for the character or 0 if the glyph was not found.
     */
    virtual long GetGlyphId( long lUnicode ) const = 0;

    /** Symbol fonts do need special treatment in a few cases.
     *  Use this method to check if the current font is a symbol
     *  font. Symbold fonts are detected by checking
     *  if they use FT_ENCODING_MS_SYMBOL as internal encoding.
     *
     * \returns true if this is a symbol font
     */
    virtual bool IsSymbol() const = 0;

    /** Try to detect the internal fonttype from
     *  the file extension of a fontfile.
     *
     *  \param pszFilename must be the filename of a font file
     *
     *  \return font type
     */
    static EPdfFontType FontTypeFromFilename( const char* pszFilename );

 protected:
    /**
     *  Set the fonttype.
     *  \param eFontType fonttype
     */
     inline void SetFontType(EPdfFontType eFontType) { m_eFontType = eFontType; }

 protected:
    std::string   m_sFilename;
    EPdfFontType  m_eFontType;
    std::string   m_sFontSubsetPrefix;
    std::vector<double> m_vecWidth;
};

};

#endif // _PDF_FONT_METRICS_H_

