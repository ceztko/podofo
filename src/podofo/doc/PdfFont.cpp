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

#include "PdfFont.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfEncoding.h"
#include "base/PdfInputStream.h"
#include "base/PdfStream.h"
#include "base/PdfWriter.h"
#include "base/PdfLocale.h"

#include "PdfFontMetrics.h"
#include "PdfPage.h"

#include <stdlib.h>
#include <string.h>
#include <sstream>

using namespace std;
using namespace PoDoFo;

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfVecObjects* pParent )
    : PdfElement( "Font", pParent ), m_pEncoding( pEncoding ), 
      m_pMetrics( pMetrics ), m_bBold( false ), m_bItalic( false ), m_isBase14( false ), m_bIsSubsetting( false )

{
    this->InitVars();
}

PdfFont::PdfFont( PdfFontMetrics* pMetrics, const PdfEncoding* const pEncoding, PdfObject* pObject )
    : PdfElement( "Font", pObject ),
      m_pEncoding( pEncoding ), m_pMetrics( pMetrics ),
      m_bBold( false ), m_bItalic( false ), m_isBase14( false ), m_bIsSubsetting( false )

{
    this->InitVars();

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    ostringstream out;
    PdfLocaleImbue(out);
    out << "PoDoFoFt" << this->GetObject()->GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );
}

PdfFont::~PdfFont()
{
    if (m_pMetrics)
        delete m_pMetrics;
    if( m_pEncoding && m_pEncoding->IsAutoDelete() )
        delete m_pEncoding;
}

void PdfFont::InitVars()
{
    ostringstream out;
    PdfLocaleImbue(out);

    // Peter Petrov 24 Spetember 2008
    m_bWasEmbedded = false;

    m_bUnderlined = false;
    m_bStrikedOut = false;

    // Implementation note: the identifier is always
    // Prefix+ObjectNo. Prefix is /Ft for fonts.
    out << "Ft" << this->GetObject()->GetIndirectReference().ObjectNumber();
    m_Identifier = PdfName( out.str().c_str() );

	

    // replace all spaces in the base font name as suggested in 
    // the PDF reference section 5.5.2#
    int curPos = 0;
    std::string sTmp = m_pMetrics->GetFontname();
    const char* pszPrefix = m_pMetrics->GetSubsetFontnamePrefix();
    if( pszPrefix ) 
    {
	std::string sPrefix = pszPrefix;
	sTmp = sPrefix + sTmp;
    }

    for(unsigned int i = 0; i < sTmp.size(); i++)
    {
        if(sTmp[i] != ' ')
            sTmp[curPos++] = sTmp[i];
    }
    sTmp.resize(curPos);
    m_BaseFont = PdfName( sTmp.c_str() );
}

inline char ToHex( const char byte )
{
    static const char* s_pszHex = "0123456789ABCDEF";

    return s_pszHex[byte % 16];
}
void PdfFont::WriteStringToStream(const string_view& rsString, PdfStream* pStream)
{
    if (m_pEncoding == nullptr)
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    stringstream ostream;
    WriteStringToStream( rsString, ostream );
    pStream->Append( ostream.str() );
}

void PdfFont::WriteStringToStream(const string_view& rsString, ostream& rStream)
{
    auto encoded = m_pEncoding->ConvertToEncoding(rsString);
    size_t lLen = 0;

    unique_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( EPdfFilter::ASCIIHexDecode );
    unique_ptr<char> buffer;
    pFilter->Encode(encoded.data(), encoded.size(), buffer, &lLen);

    rStream << "<";
    rStream.write(buffer.get(), lLen );
    rStream << ">";
}

// Peter Petrov 5 January 2009
void PdfFont::EmbedFont()
{
    if (!m_bWasEmbedded)
    {
        // Now we embed the font

        // Now we set the flag
        m_bWasEmbedded = true;
    }
}

void PdfFont::EmbedSubsetFont()
{
	//virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO( EPdfError::NotImplemented, "Subsetting not implemented for this font type." );
}

double PdfFont::StringWidth(const string_view& view, const PdfTextState& state) const
{
    //// Use PdfFontMetrics::StringWidth()
    return 0.0;
}

double PdfFont::CharWidth(char32_t ch, const PdfTextState& state) const
{
    return 0.0;

    /* FreeType
    FT_Error ftErr;
    double   dWidth = 0.0;

    if( static_cast<int>(c) < PODOFO_WIDTH_CACHE_SIZE )
    {
        dWidth = m_vecWidth[static_cast<unsigned int>(c)];
    }
    else
    {
        ftErr = FT_Load_Char( m_pFace, static_cast<FT_UInt>(c), FT_LOAD_NO_SCALE | FT_LOAD_NO_BITMAP );
        if( ftErr )
            return dWidth;

        dWidth = m_pFace->glyph->metrics.horiAdvance * 1000.0 / m_pFace->units_per_EM;
    }

    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale()) / 1000.0 +
        static_cast<double>( this->GetFontSize() * this->GetFontScale() * this->GetFontCharSpace() / 100.0);
    */

    /*
    return dWidth * static_cast<double>(this->GetFontSize() * this->GetFontScale()) / 1000.0 +
        static_cast<double>(this->GetFontSize() * this->GetFontScale() * this->GetFontCharSpace() / 100.0);
    */

    /*
    if( c >= m_nFirst && c <= m_nLast
        && c - m_nFirst < m_width.GetSize())
    {
        double dWidth = m_width[c - m_nFirst].GetReal();

        return (dWidth * m_matrix[0] * m_fFontSize + m_fFontCharSpace) * m_fFontScale / 100.0;

    }

    if( m_missingWidth != NULL )
        return m_missingWidth->GetReal ();
    else
        return m_dDefWidth;
}

double PdfFontMetricsObject::UnicodeCharWidth( unsigned short c ) const
{
    if( c >= m_nFirst && c <= m_nLast
        && c - m_nFirst < m_width.GetSize())
    {
        double dWidth = m_width[c - m_nFirst].GetReal();

        return (dWidth * m_matrix[0] * m_fFontSize + m_fFontCharSpace) * m_fFontScale / 100.0;
    }

    if( m_missingWidth != NULL )
        return m_missingWidth->GetReal ();
    else
        return m_dDefWidth;
    */
}

double PdfFont::GetLineSpacing(const PdfTextState& state) const
{
    return m_pMetrics->GetLineSpacing() * state.GetFontSize();
}

double PdfFont::GetUnderlineThickness(const PdfTextState& state) const
{
    return m_pMetrics->GetUnderlineThickness() * state.GetFontSize();
}

double PdfFont::GetUnderlinePosition(const PdfTextState& state) const
{
    return m_pMetrics->GetUnderlinePosition() * state.GetFontSize();
}

double PdfFont::GetStrikeOutPosition(const PdfTextState& state) const
{
    return m_pMetrics->GetStrikeOutPosition() * state.GetFontSize();
}

double PdfFont::GetStrikeOutThickness(const PdfTextState& state) const
{
    return m_pMetrics->GetStrikeOutThickness() * state.GetFontSize();
}

double PdfFont::GetAscent(const PdfTextState& state) const
{
    return m_pMetrics->GetAscent() * state.GetFontSize();
}

double PdfFont::GetDescent(const PdfTextState& state) const
{
    return m_pMetrics->GetDescent() * state.GetFontSize();
}

void PdfFont::AddUsedSubsettingGlyphs(const string_view&, size_t)
{
    //virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Subsetting not implemented for this font type.");
}

void PdfFont::AddUsedGlyphname(const char*)
{
    //virtual function is only implemented in derived class
    PODOFO_RAISE_ERROR_INFO(EPdfError::NotImplemented, "Subsetting not implemented for this font type.");
}

void PdfFont::SetBold(bool bBold)
{
    m_bBold = bBold;
}

void PdfFont::SetItalic(bool bItalic)
{
    m_bItalic = bItalic;
}
