/***************************************************************************
 *   Copyright (C) 2010 by Dominik Seichter                                *
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

#include "PdfFontMetricsBase14.h"

#include "base/PdfDefinesPrivate.h"
#include "base/PdfArray.h"

#include "PdfFontFactoryBase14Data.h"

using namespace PoDoFo;


PdfFontMetricsBase14::PdfFontMetricsBase14(const char      *mfont_name,
                                           const PODOFO_CharData  *mwidths_table,
                                           bool              mis_font_specific,
                                           int16_t         mascent,
                                           int16_t         mdescent,
                                           uint16_t        mx_height,
                                           uint16_t        mcap_height,
                                           int16_t         mstrikeout_pos,
                                           int16_t         munderline_pos,
                                           const PdfRect &  mbbox)
    : PdfFontMetrics( EPdfFontType::Type1Base14, "", nullptr),
      font_name(mfont_name),
      widths_table(mwidths_table),
      is_font_specific(mis_font_specific),
      ascent(mascent), 
      descent(mdescent),
      x_height(mx_height), 
      cap_height(mcap_height), 
      bbox(mbbox), 
      m_bSymbol(is_font_specific)
{			
    m_nWeight             = 500;
    m_nItalicAngle        = 0;
    m_dLineSpacing        = 0.0;
    m_dUnderlineThickness = 0.05;
    m_dStrikeOutThickness = m_dUnderlineThickness;
    units_per_EM          = 1000;
    m_dPdfAscent          = ascent * 1000 / units_per_EM;
    m_dPdfDescent         = descent * 1000 / units_per_EM;
    
    m_dAscent             = ascent;
    m_dDescent            = descent;
    m_dUnderlinePosition  = static_cast<double>(munderline_pos) / units_per_EM;
    m_dStrikeOutPosition  = static_cast<double>(mstrikeout_pos) / units_per_EM;

    // calculate the line spacing now, as it changes only with the font size
    m_dLineSpacing        = (static_cast<double>(ascent + abs(descent)) / units_per_EM);
    m_dAscent             = static_cast<double>(ascent) /  units_per_EM;
    m_dDescent            = static_cast<double>(descent) /  units_per_EM;
}

PdfFontMetricsBase14::~PdfFontMetricsBase14()
{
}

double PdfFontMetricsBase14::GetGlyphWidth( int nGlyphId ) const 
{
    return widths_table[static_cast<unsigned int>(nGlyphId)].width; 
}

double PdfFontMetricsBase14::GetGlyphWidth( const char* ) const 
{
    return 0.0;
}

double PdfFontMetricsBase14::GetLineSpacing() const
{
    return m_dLineSpacing;
}

double PdfFontMetricsBase14::GetUnderlineThickness() const
{
    return m_dUnderlineThickness;
}

double PdfFontMetricsBase14::GetUnderlinePosition() const
{
    return m_dUnderlinePosition;
}

double PdfFontMetricsBase14::GetStrikeOutPosition() const
{
    return m_dStrikeOutPosition;
}

double PdfFontMetricsBase14::GetStrikeOutThickness() const
{
    return m_dStrikeOutThickness;
}

double PdfFontMetricsBase14::GetAscent() const
{
    return m_dAscent;
}

double PdfFontMetricsBase14::GetDescent() const
{
    return m_dDescent;
}

const char* PdfFontMetricsBase14::GetFontname() const
{
    return font_name;
}

unsigned int PdfFontMetricsBase14::GetWeight() const
{
    return m_nWeight;
}

int PdfFontMetricsBase14::GetItalicAngle() const
{
    return m_nItalicAngle;
}

long PdfFontMetricsBase14::GetGlyphIdUnicode( long lUnicode ) const
{
    long lGlyph = 0;
    long lSwappedUnicode = ((lUnicode & 0xFF00) >> 8) | ((lUnicode & 0x00FF) << 8);

    // Handle symbol fonts!
    /*
      if( m_bSymbol ) 
      {
      lUnicode = lUnicode | 0xf000;
      }
    */
    
    for(int i = 0; widths_table[i].unicode != 0xFFFF ; ++i)
    {
        if( widths_table[i].unicode == lUnicode ||
            widths_table[i].unicode == lSwappedUnicode )
        {
            lGlyph = i; //widths_table[i].char_cd ;
            break;
        }
    }
    
    //FT_Get_Char_Index( m_face, lUnicode );
	
    return lGlyph;
}

long PdfFontMetricsBase14::GetGlyphId( long charId ) const
{
    long lGlyph = 0;
    
    // Handle symbol fonts!
    /*
      if( m_bSymbol ) 
      {
      charId = charId | 0xf000;
      }
    */
    
    for(int i = 0; widths_table[i].unicode != 0xFFFF  ; ++i)
    {
        if (widths_table[i].char_cd == charId) 
        {
            lGlyph = i; //widths_table[i].char_cd ;
            break;
        }
    }
    
    //FT_Get_Char_Index( m_face, lUnicode );

    return lGlyph;
}

bool PdfFontMetricsBase14::IsSymbol() const
{
    return m_bSymbol;
}

void PdfFontMetricsBase14::GetBoundingBox( PdfArray & array ) const
{
    array.Clear();
    array.push_back( PdfVariant( bbox.GetLeft() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetBottom() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetWidth() * 1000.0 / units_per_EM ) );
    array.push_back( PdfVariant( bbox.GetHeight() * 1000.0 / units_per_EM ) );
    
    return;
}

void PdfFontMetricsBase14::GetWidthArray( PdfVariant & var, unsigned int nFirst, unsigned int nLast, const PdfEncoding* pEncoding ) const
{
    unsigned int i;
    PdfArray     list;
    
    for( i=nFirst;i<=nLast;i++ )
    {
        if (pEncoding != nullptr)
        {
            unsigned short shCode = pEncoding->GetCharCode(i);

            list.push_back(PdfObject( (int64_t)this->GetGlyphWidth(this->GetGlyphIdUnicode(shCode) )));
        }
        else
        {
            list.push_back( PdfVariant(  double(widths_table[i].width)  ) );
        }
    }
    
    var = PdfVariant( list );
}

const char* PdfFontMetricsBase14::GetFontData() const
{
    return nullptr;
}

size_t PdfFontMetricsBase14::GetFontDataLen() const
{
    return 0;
}
