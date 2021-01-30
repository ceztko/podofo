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

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <vector>

#include <utfcpp/utf8.h>

#include "PdfPainter.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfColor.h"
#include "base/PdfDictionary.h"
#include "base/PdfFilter.h"
#include "base/PdfName.h"
#include "base/PdfRect.h"
#include "base/PdfStream.h"
#include "base/PdfString.h"
#include "base/PdfLocale.h"

#include "PdfContents.h"
#include "PdfExtGState.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfImage.h"
#include "PdfMemDocument.h"
#include "PdfShadingPattern.h"
#include "PdfTilingPattern.h"
#include "PdfXObject.h"

using namespace std;
using namespace PoDoFo;

#define BEZIER_POINTS 13

/* 4/3 * (1-cos 45<-,A0<-(B)/sin 45<-,A0<-(B = 4/3 * sqrt(2) - 1 */
#define ARC_MAGIC    0.552284749
#define PI           3.141592654

static const long clPainterHighPrecision    = 15L;
static const long clPainterDefaultPrecision = 3L;

string ExpandTabsPrivate(const string_view& str, int tabWidth, unsigned nTabCnt, char cTab, char cSpace);

static inline bool IsNewLineChar(char32_t ch)
{
    return ch == '\n';
}

static inline bool IsSpaceChar(char32_t ch)
{
	return isspace( ch ) != 0;
}

PdfPainter::PdfPainter(EPdfPainterFlags flags)
: m_flags(flags), m_stream( nullptr ), m_canvas( nullptr ), m_pFont( nullptr ),
  m_nTabWidth( 4 ), m_curColor( PdfColor( 0.0, 0.0, 0.0 ) ),
  m_isTextOpen( false ), m_tmpStream(), m_curPath(), m_isCurColorICCDepend( false ), m_CSTag()
{
    m_tmpStream.flags( std::ios_base::fixed );
    m_tmpStream.precision( clPainterDefaultPrecision );
    PdfLocaleImbue(m_tmpStream);

    m_curPath.flags( std::ios_base::fixed );
    m_curPath.precision( clPainterDefaultPrecision );
    PdfLocaleImbue(m_curPath);

    lpx  = 
    lpy  = 
    lpx2 = 
    lpy2 = 
    lpx3 = 
    lpy3 = 
    lcx  =
    lcy  = 
    lrx  = 
    lry  = 0.0;
    currentTextRenderingMode = EPdfTextRenderingMode::Fill;
}

PdfPainter::~PdfPainter() noexcept(false)
{
	// Throwing exceptions in C++ destructors is not allowed.
	// Just log the error.
    // PODOFO_RAISE_LOGIC_IF( m_pCanvas, "FinishPage() has to be called after a page is completed!" );
    // Note that we can't do this for the user, since FinishPage() might
    // throw and we can't safely have that in a dtor. That also means
    // we can't throw here, but must abort.
    if (m_stream != nullptr)
    {
        PdfError::LogMessage(ELogSeverity::Error,
            "PdfPainter::~PdfPainter(): FinishPage() has to be called after a page is completed!");
    }

    PODOFO_ASSERT( !m_stream );
}

void PdfPainter::SetCanvas( PdfCanvas* canvas )
{
    // Ignore setting the same canvas twice
    if( m_canvas == canvas)
        return;

    finishDrawing();

    m_canvas = canvas;
    m_stream = nullptr;
    currentTextRenderingMode = EPdfTextRenderingMode::Fill;
}

void PdfPainter::FinishDrawing()
{
    try
    {
        finishDrawing();
    }
    catch (PdfError & e)
    {
        // clean up, even in case of error
        m_stream = nullptr;
        m_canvas = nullptr;

        throw e;
    }

    m_stream = nullptr;
    m_canvas = nullptr;
    currentTextRenderingMode = EPdfTextRenderingMode::Fill;
}

void PdfPainter::finishDrawing()
{
	if ( m_stream )
    {
        if ((m_flags & EPdfPainterFlags::NoSaveRestorePrior) == EPdfPainterFlags::NoSaveRestorePrior)
        {
            // GetLength() must be called before BeginAppend()
            if (m_stream->GetLength() == 0)
            {
                m_stream->BeginAppend(false);
            }
            else
            {
                m_stream->BeginAppend(false);
                // there is already content here - so let's assume we are appending
                // as such, we MUST put in a "space" to separate whatever we do.
                m_stream->Append("\n");
            }
        }
        else
        {
            PdfMemoryOutputStream memstream;
            if ( m_stream->GetLength() != 0 )
                m_stream->GetFilteredCopy( &memstream );

            size_t length = memstream.GetLength();
            if ( length == 0 )
            {
                m_stream->BeginAppend( false );
            }
            else
            {
                m_stream->BeginAppend( true );
                m_stream->Append( "q\n" );
                m_stream->Append(memstream.GetBuffer(), length );
                m_stream->Append( "Q\n" );
            }
        }

        if ((m_flags & EPdfPainterFlags::NoSaveRestore) == EPdfPainterFlags::NoSaveRestore)
        {
            m_stream->Append(m_tmpStream.str());
        }
        else
        {
            m_stream->Append("q\n");
            m_stream->Append(m_tmpStream.str());
            m_stream->Append("Q\n");
        }

        m_stream->EndAppend();
    }

    // Reset temporary stream
    m_tmpStream.str("");
}

void PdfPainter::SetStrokingShadingPattern( const PdfShadingPattern & rPattern )
{
    CheckStream();

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->GetIndirectReference(), PdfName("Pattern") );

    m_tmpStream << "/Pattern CS /" << rPattern.GetIdentifier().GetString() << " SCN" << std::endl;
}

void PdfPainter::SetShadingPattern( const PdfShadingPattern & rPattern )
{
    CheckStream();

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->GetIndirectReference(), PdfName("Pattern") );

    m_tmpStream << "/Pattern cs /" << rPattern.GetIdentifier().GetString() << " scn" << std::endl;
}

void PdfPainter::SetStrokingTilingPattern( const PdfTilingPattern & rPattern )
{
    CheckStream();

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->GetIndirectReference(), PdfName("Pattern") );

    m_tmpStream << "/Pattern CS /" << rPattern.GetIdentifier().GetString() << " SCN" << std::endl;
}

void PdfPainter::SetStrokingTilingPattern( const std::string &rPatternName )
{
    CheckStream();

    m_tmpStream << "/Pattern CS /" << rPatternName << " SCN" << std::endl;
}

void PdfPainter::SetTilingPattern( const PdfTilingPattern & rPattern )
{
    CheckStream();

    this->AddToPageResources( rPattern.GetIdentifier(), rPattern.GetObject()->GetIndirectReference(), PdfName("Pattern") );

    m_tmpStream << "/Pattern cs /" << rPattern.GetIdentifier().GetString() << " scn" << std::endl;
}

void PdfPainter::SetTilingPattern( const std::string &rPatternName )
{
    CheckStream();

    m_tmpStream << "/Pattern cs /" << rPatternName << " scn" << std::endl;
}

void PdfPainter::SetStrokingColor( const PdfColor & rColor )
{
    CheckStream();

    switch( rColor.GetColorSpace() ) 
    {
        default: 
        case EPdfColorSpace::DeviceRGB:
            m_tmpStream << rColor.GetRed()   << " "
                  << rColor.GetGreen() << " "
                  << rColor.GetBlue() 
                  << " RG" << std::endl;
            break;
        case EPdfColorSpace::DeviceCMYK:
            m_tmpStream << rColor.GetCyan()    << " " 
                  << rColor.GetMagenta() << " " 
                  << rColor.GetYellow()  << " " 
                  << rColor.GetBlack() 
                  << " K" << std::endl;
            break;
        case EPdfColorSpace::DeviceGray:
            m_tmpStream << rColor.GetGrayScale() << " G" << std::endl;
            break;
        case EPdfColorSpace::Separation:
			m_canvas->AddColorResource( rColor );
			m_tmpStream << "/ColorSpace" << PdfName( rColor.GetName() ).GetEscapedName() << " CS " << rColor.GetDensity() << " SCN" << std::endl;
            break;
        case EPdfColorSpace::CieLab:
			m_canvas->AddColorResource( rColor );
			m_tmpStream << "/ColorSpaceCieLab" << " CS " 
				  << rColor.GetCieL() << " " 
                  << rColor.GetCieA() << " " 
                  << rColor.GetCieB() <<
				  " SCN" << std::endl;
            break;
        case EPdfColorSpace::Unknown:
        case EPdfColorSpace::Indexed:
        {
            PODOFO_RAISE_ERROR( EPdfError::CannotConvertColor );
        }
    }
}

void PdfPainter::SetColor( const PdfColor & rColor )
{
    CheckStream();

    m_isCurColorICCDepend = false;

    m_curColor = rColor;
    switch( rColor.GetColorSpace() ) 
    {
        default: 
        case EPdfColorSpace::DeviceRGB:
            m_tmpStream << rColor.GetRed()   << " "
                  << rColor.GetGreen() << " "
                  << rColor.GetBlue() 
                  << " rg" << std::endl;
            break;
        case EPdfColorSpace::DeviceCMYK:
            m_tmpStream << rColor.GetCyan()    << " " 
                  << rColor.GetMagenta() << " " 
                  << rColor.GetYellow()  << " " 
                  << rColor.GetBlack() 
                  << " k" << std::endl;
            break;
        case EPdfColorSpace::DeviceGray:
            m_tmpStream << rColor.GetGrayScale() << " g" << std::endl;
            break;
        case EPdfColorSpace::Separation:
			m_canvas->AddColorResource( rColor );
            m_tmpStream << "/ColorSpace" << PdfName( rColor.GetName() ).GetEscapedName() << " cs " << rColor.GetDensity() << " scn" << std::endl;
            break;
        case EPdfColorSpace::CieLab:
			m_canvas->AddColorResource( rColor );
			m_tmpStream << "/ColorSpaceCieLab" << " cs " 
				  << rColor.GetCieL() << " " 
                  << rColor.GetCieA() << " " 
                  << rColor.GetCieB() <<
				  " scn" << std::endl;
			break;
        case EPdfColorSpace::Unknown:
        case EPdfColorSpace::Indexed:
        {
            PODOFO_RAISE_ERROR( EPdfError::CannotConvertColor );
        }
    }
}

void PdfPainter::SetStrokeWidth( double dWidth )
{
    CheckStream();

    m_tmpStream << dWidth << " w" << std::endl;
}

void PdfPainter::SetStrokeStyle( EPdfStrokeStyle eStyle, const char* pszCustom, bool inverted, double scale, bool subtractJoinCap)
{
    bool have = false;

    CheckStream();

    if (eStyle != EPdfStrokeStyle::Custom) {
        m_tmpStream << "[";
    }

    if (inverted && eStyle != EPdfStrokeStyle::Solid && eStyle != EPdfStrokeStyle::Custom) {
       m_tmpStream << "0 ";
    }

    switch( eStyle )
    {
        case EPdfStrokeStyle::Solid:
            have = true;
            break;
        case EPdfStrokeStyle::Dash:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_tmpStream << "6 2";
            } else {
                if (subtractJoinCap) {
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0;
                } else {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0;
                }
            }
            break;
        case EPdfStrokeStyle::Dot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_tmpStream << "2 2";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_tmpStream << 0.001 << " " << 2.0 * scale << " " << 0 << " " << 2.0 * scale;
                } else {
                   m_tmpStream << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case EPdfStrokeStyle::DashDot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_tmpStream << "3 2 1 2";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                } else {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case EPdfStrokeStyle::DashDotDot:
            have = true;
            if (scale >= 1.0 - 1e-5 && scale <= 1.0 + 1e-5) {
                m_tmpStream << "3 1 1 1 1 1";
            } else {
                if (subtractJoinCap) {
                    // zero length segments are drawn anyway here
                    m_tmpStream << scale * 2.0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0 << " " << 0 << " " << scale * 2.0;
                } else {
                    m_tmpStream << scale * 3.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0 << " " << scale * 1.0;
                }
            }
            break;
        case EPdfStrokeStyle::Custom:
            have = pszCustom != nullptr;
            if (have)
                m_tmpStream << pszCustom;
            break;
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidStrokeStyle );
        }
    }

    if( !have )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidStrokeStyle );
    }
    
    if (inverted && eStyle != EPdfStrokeStyle::Solid && eStyle != EPdfStrokeStyle::Custom) {
        m_tmpStream << " 0";
    }

    if (eStyle != EPdfStrokeStyle::Custom) {
        m_tmpStream << "] 0";
    }

    m_tmpStream << " d" << std::endl;
}

void PdfPainter::SetLineCapStyle( EPdfLineCapStyle eCapStyle )
{
    CheckStream();

    m_tmpStream << static_cast<int>(eCapStyle) << " J" << std::endl;
}

void PdfPainter::SetLineJoinStyle( EPdfLineJoinStyle eJoinStyle )
{
    CheckStream();

    m_tmpStream << static_cast<int>(eJoinStyle) << " j" << std::endl;
}

void PdfPainter::SetFont( PdfFont* pFont )
{
    CheckStream();

    if( !pFont )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_pFont = pFont;
}

void PdfPainter::SetTextRenderingMode( EPdfTextRenderingMode mode )
{
    CheckStream();

    if (mode == currentTextRenderingMode) {
        return;
    }

    currentTextRenderingMode = mode;
    if (m_isTextOpen)
        SetCurrentTextRenderingMode();
}

void PdfPainter::SetCurrentTextRenderingMode( void )
{
    CheckStream();

    m_tmpStream << (int) currentTextRenderingMode << " Tr" << std::endl;
}

void PdfPainter::SetClipRect( double dX, double dY, double dWidth, double dHeight )
{
    CheckStream();

    m_tmpStream << dX << " "
          << dY << " "
          << dWidth << " "
          << dHeight        
          << " re W n" << std::endl;

	 m_curPath
			 << dX << " "
          << dY << " "
          << dWidth << " "
          << dHeight        
          << " re W n" << std::endl;
}

void PdfPainter::SetMiterLimit(double value)
{
    CheckStream();

    m_tmpStream << value << " M" << std::endl;
}

void PdfPainter::DrawLine( double dStartX, double dStartY, double dEndX, double dEndY )
{
    CheckStream();

	 m_curPath.str("");
    m_curPath
		    << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY        
          << " l" << std::endl;

    m_tmpStream << dStartX << " "
          << dStartY
          << " m "
          << dEndX << " "
          << dEndY        
          << " l S" << std::endl;
}

void PdfPainter::Rectangle( double dX, double dY, double dWidth, double dHeight,
                           double dRoundX, double dRoundY )
{ 
    CheckStream();

    if ( static_cast<int>(dRoundX) || static_cast<int>(dRoundY) ) 
    {
        double x = dX, y = dY, 
               w = dWidth, h = dHeight,
               rx= dRoundX, ry = dRoundY;
        double b = 0.4477f;

        MoveTo(x + rx, y);
        LineTo(x + w - rx, y);
        CubicBezierTo(x + w - rx * b, y, x + w, y + ry * b, x + w, y + ry);
        LineTo(x + w, y + h - ry);
        CubicBezierTo(x + w, y + h - ry * b, x + w - rx * b, y + h, x + w - rx, y + h);
        LineTo(x + rx, y + h);
        CubicBezierTo(x + rx * b, y + h, x, y + h - ry * b, x, y + h - ry);
        LineTo(x, y + ry);
        CubicBezierTo(x, y + ry * b, x + rx * b, y, x + rx, y);
    } 
    else 
    {
        m_curPath 
				  << dX << " "
              << dY << " "
              << dWidth << " "
              << dHeight        
              << " re" << std::endl;

        m_tmpStream << dX << " "
            << dY << " "
            << dWidth << " "
            << dHeight        
              << " re" << std::endl;
    }
}

void PdfPainter::Ellipse( double dX, double dY, double dWidth, double dHeight )
{
    double dPointX[BEZIER_POINTS];
    double dPointY[BEZIER_POINTS];
    int    i;

    CheckStream();

    ConvertRectToBezier( dX, dY, dWidth, dHeight, dPointX, dPointY );

    m_curPath
			 << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    m_tmpStream << dPointX[0] << " "
          << dPointY[0]
          << " m" << std::endl;

    for( i=1;i<BEZIER_POINTS; i+=3 )
    {
        m_curPath
				  << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;

        m_tmpStream << dPointX[i] << " "
              << dPointY[i] << " "
              << dPointX[i+1] << " "
              << dPointY[i+1] << " "
              << dPointX[i+2] << " "
              << dPointY[i+2]    
              << " c" << std::endl;
    }
}

void PdfPainter::Circle( double dX, double dY, double dRadius )
{
    CheckStream();

    /* draw four Bezier curves to approximate a circle */
    MoveTo( dX + dRadius, dY );
    CubicBezierTo( dX + dRadius, dY + dRadius*ARC_MAGIC,
             dX + dRadius*ARC_MAGIC, dY + dRadius,
             dX, dY + dRadius );
    CubicBezierTo( dX - dRadius*ARC_MAGIC, dY + dRadius,
            dX - dRadius, dY + dRadius*ARC_MAGIC,
            dX - dRadius, dY );
    CubicBezierTo( dX - dRadius, dY - dRadius*ARC_MAGIC,
            dX - dRadius*ARC_MAGIC, dY - dRadius,
            dX, dY - dRadius );
    CubicBezierTo( dX + dRadius*ARC_MAGIC, dY - dRadius,
            dX + dRadius, dY - dRadius*ARC_MAGIC,
            dX + dRadius, dY );
    Close();
}

void PdfPainter::DrawText(double dX, double dY, const string_view& sText)
{
    this->DrawText(dX, dY, sText, sText.length());
}

void PdfPainter::DrawText(double dX, double dY, const string_view& sText, size_t lStringLen)
{
    CheckStream();

    if( !m_pFont || !m_canvas )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // Peter Petrov 25 September 2008
    //m_pFont->EmbedFont();

    auto sString = this->ExpandTabs( sText, lStringLen );
    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->GetIndirectReference(), PdfName("Font") );
    if( m_pFont->IsSubsetting() )
    {
        m_pFont->AddUsedSubsettingGlyphs( sText, lStringLen );
    }

    if( m_pFont->IsUnderlined() || m_pFont->IsStrikeOut())
    {
        this->Save();
        this->SetCurrentStrokingColor();
		
        // Draw underline
        this->SetStrokeWidth( m_pFont->GetUnderlineThickness(m_textState) );
        if( m_pFont->IsUnderlined() )
        {
            this->DrawLine( dX,
                dY + m_pFont->GetUnderlinePosition(m_textState),
                dX + m_pFont->StringWidth( sString, m_textState),
                dY + m_pFont->GetUnderlinePosition(m_textState) );
        }

        // Draw strikeout
        this->SetStrokeWidth( m_pFont->GetStrikeOutThickness(m_textState) );
        if( m_pFont->IsStrikeOut() )
        {
            this->DrawLine( dX,
                dY + m_pFont->GetStrikeOutPosition(m_textState),
                dX + m_pFont->StringWidth( sString, m_textState),
                dY + m_pFont->GetStrikeOutPosition(m_textState) );
        }

        this->Restore();
    }
    
    m_tmpStream << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetString()
          << " "  << m_textState.GetFontSize()
          << " Tf" << std::endl;

    if (currentTextRenderingMode != EPdfTextRenderingMode::Fill) {
        SetCurrentTextRenderingMode();
    }

    m_tmpStream << m_textState.GetFontScale() * 100 << " Tz" << std::endl;
    m_tmpStream << m_textState.GetCharSpace() * (double)m_textState.GetFontSize() / 100.0 << " Tc" << std::endl;

    m_tmpStream << dX << std::endl
          << dY << std::endl << "Td ";

    m_pFont->WriteStringToStream( sString, m_tmpStream );

    /*
    char* pBuffer;
    std::unique_ptr<PdfFilter> pFilter = PdfFilterFactory::Create( EPdfFilter::ASCIIHexDecode );
    pFilter->Encode( sString.GetString(), sString.GetLength(), &pBuffer, &lLen );

    m_oss.write( pBuffer, lLen );
    podofo_free( pBuffer );
    */

    m_tmpStream << " Tj\nET\n";
}

void PdfPainter::BeginText( double dX, double dY )
{
    CheckStream();

    if( !m_pFont || !m_canvas ||  m_isTextOpen)
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    this->AddToPageResources( m_pFont->GetIdentifier(), m_pFont->GetObject()->GetIndirectReference(), PdfName("Font") );

    m_tmpStream << "BT" << std::endl << "/" << m_pFont->GetIdentifier().GetString()
          << " "  << m_textState.GetFontSize()
          << " Tf" << std::endl;

    if (currentTextRenderingMode != EPdfTextRenderingMode::Fill) {
        SetCurrentTextRenderingMode();
    }

    m_tmpStream << m_textState.GetFontScale() * 100 << " Tz" << std::endl;
    m_tmpStream << m_textState.GetCharSpace() * (double)m_textState.GetFontSize() / 100.0 << " Tc" << std::endl;

    m_tmpStream << dX << " " << dY << " Td" << std::endl ;

	m_isTextOpen = true;
}

void PdfPainter::MoveTextPos( double dX, double dY )
{
    CheckStream();

    if( !m_pFont || !m_canvas || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_tmpStream << dX << " " << dY << " Td" << std::endl ;
}

void PdfPainter::AddText(const string_view& sText)
{
	AddText(sText, sText.length());
}

void PdfPainter::AddText(const string_view& sText, size_t lStringLen)
{
    CheckStream();

    if( !m_pFont || !m_canvas || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    auto sString = this->ExpandTabs( sText, lStringLen );
    if( m_pFont->IsSubsetting() )
    {
        m_pFont->AddUsedSubsettingGlyphs( sText, lStringLen );
    }

	// TODO: Underline and Strikeout not yet supported
	m_pFont->WriteStringToStream(sString, m_tmpStream);

    m_tmpStream << " Tj\n";
}

void PdfPainter::EndText()
{
    CheckStream();

    if( !m_pFont || !m_canvas || !m_isTextOpen )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_tmpStream << "ET\n";
	m_isTextOpen = false;
}

void PdfPainter::DrawMultiLineText(double dX, double dY, double dWidth, double dHeight, const string_view& rsText,
                                   EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical, bool bClip, bool bSkipSpaces )
{
    throw std::runtime_error("Untested after utf-8 migration");
    CheckStream();

    if( !m_pFont || !m_canvas )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
    
    if( dWidth <= 0.0 || dHeight <= 0.0 ) // nonsense arguments
        return;

    this->Save();
    if( bClip ) 
    {
        this->SetClipRect( dX, dY, dWidth, dHeight );
    }

    auto sString = this->ExpandTabs( rsText );

	vector<string> vecLines = GetMultiLineTextAsLines( dWidth, sString, bSkipSpaces );
    double dLineGap = m_pFont->GetLineSpacing(m_textState) - m_pFont->GetAscent(m_textState) + m_pFont->GetDescent(m_textState);
    // Do vertical alignment
    switch( eVertical ) 
    {
        default:
	    case EPdfVerticalAlignment::Top:
            dY += dHeight; break;
        case EPdfVerticalAlignment::Bottom:
            dY += m_pFont->GetLineSpacing(m_textState) * vecLines.size(); break;
        case EPdfVerticalAlignment::Center:
            dY += (dHeight - 
                   ((dHeight - (m_pFont->GetLineSpacing(m_textState) * vecLines.size()))/2.0));
            break;
    }

    dY -= (m_pFont->GetAscent(m_textState) + dLineGap / (2.0));

    vector<string>::const_iterator it = vecLines.begin();
    while( it != vecLines.end() )
    {
        if (it->length() != 0)
            this->DrawTextAligned( dX, dY, dWidth, *it, eAlignment );

        dY -= m_pFont->GetLineSpacing(m_textState);
        ++it;
    }
    this->Restore();
}

vector<string> PdfPainter::GetMultiLineTextAsLines(double dWidth, const string_view& rsText, bool bSkipSpaces)
{
    throw runtime_error("Untested after utf-8 migration");
    CheckStream();

    if( !m_pFont || !m_canvas )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
     
    if( dWidth <= 0.0 ) // nonsense arguments
	    return vector<string>();
    
    if( rsText.length() == 0 ) // empty string
        return vector<string>(1, (string)rsText);
	        
	const char* const stringBegin = rsText.data();
    const char* pszLineBegin = stringBegin;
    const char* pszCurrentCharacter = stringBegin;
    const char* pszStartOfCurrentWord  = stringBegin;
    bool startOfWord = true;
    double dCurWidthOfLine = 0.0;
	vector<string> vecLines;

    // do simple word wrapping
    while( *pszCurrentCharacter ) 
    {
        if( IsNewLineChar( *pszCurrentCharacter ) ) // hard-break! 
        {
            vecLines.push_back(string({ pszLineBegin, (size_t)(pszCurrentCharacter - pszLineBegin) }));

            pszLineBegin = pszCurrentCharacter + 1;// skip the line feed
            startOfWord = true;
            dCurWidthOfLine = 0.0;
        }
        else if( IsSpaceChar( *pszCurrentCharacter ) )
        {
            if( dCurWidthOfLine > dWidth )
            {
                // The previous word does not fit in the current line.
                // -> Move it to the next one.
                if( pszStartOfCurrentWord > pszLineBegin )
                {
                    vecLines.push_back(string({ pszLineBegin, (size_t)(pszStartOfCurrentWord - pszLineBegin) }));
                }
                else
                {
                    vecLines.push_back(string({ pszLineBegin, (size_t)(pszCurrentCharacter - pszLineBegin) }));
                    if (bSkipSpaces)
                    {
                        // Skip all spaces at the end of the line
                        while( IsSpaceChar( *( pszCurrentCharacter + 1 ) ) )
                            pszCurrentCharacter++;

                        pszStartOfCurrentWord = pszCurrentCharacter + 1;
                    }
                    else
                    {
                        pszStartOfCurrentWord = pszCurrentCharacter;
                    }
                    startOfWord=true;
                }
                pszLineBegin = pszStartOfCurrentWord;

                if (!startOfWord)
                {
                    dCurWidthOfLine = m_pFont->StringWidth( 
                        { pszStartOfCurrentWord, (size_t)(pszCurrentCharacter - pszStartOfCurrentWord) },
                        m_textState);
                }
                else
                {
                    dCurWidthOfLine = 0.0;
                }
            }
            else if( ( dCurWidthOfLine + m_pFont->CharWidth( *pszCurrentCharacter, m_textState) ) > dWidth )
            {
                vecLines.push_back(string({ pszLineBegin, (size_t)(pszCurrentCharacter - pszLineBegin) }));
                if( bSkipSpaces )
                {
                    // Skip all spaces at the end of the line
                    while( IsSpaceChar( *( pszCurrentCharacter + 1 ) ) )
                        pszCurrentCharacter++;

                    pszStartOfCurrentWord = pszCurrentCharacter + 1;
                }
                else
                {
                    pszStartOfCurrentWord = pszCurrentCharacter;
                }
                pszLineBegin = pszStartOfCurrentWord;
                startOfWord = true;
                dCurWidthOfLine = 0.0;
            }
            else 
            {           
                dCurWidthOfLine += m_pFont->CharWidth( *pszCurrentCharacter, m_textState);
            }

            startOfWord = true;
        }
        else
        {
            if (startOfWord)
            {
                pszStartOfCurrentWord = pszCurrentCharacter;
                startOfWord = false;
            }
            //else do nothing

            if ((dCurWidthOfLine + m_pFont->CharWidth(*pszCurrentCharacter, m_textState)) > dWidth)
            {
                if ( pszLineBegin == pszStartOfCurrentWord )
                {
                    // This word takes up the whole line.
                    // Put as much as possible on this line.                    
                    if (pszLineBegin == pszCurrentCharacter)
                    {
                        vecLines.push_back(string({ pszCurrentCharacter, 1 }));
                        pszLineBegin = pszCurrentCharacter + 1;
                        pszStartOfCurrentWord = pszCurrentCharacter + 1;
                        dCurWidthOfLine = 0;
                    }
                    else
                    {
                        vecLines.push_back(string({ pszLineBegin, (size_t)(pszCurrentCharacter - pszLineBegin) }));
                        pszLineBegin = pszCurrentCharacter;
                        pszStartOfCurrentWord = pszCurrentCharacter;
                        dCurWidthOfLine = m_pFont->CharWidth(*pszCurrentCharacter, m_textState);
                    }
                }
                else
                {
                    // The current word does not fit in the current line.
                    // -> Move it to the next one.                    
                    vecLines.push_back(string({ pszLineBegin, (size_t)(pszStartOfCurrentWord - pszLineBegin) }));
                    pszLineBegin = pszStartOfCurrentWord;
                    dCurWidthOfLine = m_pFont->StringWidth({ pszStartOfCurrentWord, (size_t)((pszCurrentCharacter - pszStartOfCurrentWord) + 1) }, m_textState);
                }
            }
            else 
            {
                dCurWidthOfLine += m_pFont->CharWidth(*pszCurrentCharacter, m_textState);
            }
        }
        ++pszCurrentCharacter;
    }

    if( (pszCurrentCharacter - pszLineBegin) > 0 ) 
    {
        if( dCurWidthOfLine > dWidth && pszStartOfCurrentWord > pszLineBegin )
        {
            // The previous word does not fit in the current line.
            // -> Move it to the next one.
            vecLines.push_back(string({ pszLineBegin, (size_t)(pszStartOfCurrentWord - pszLineBegin) }));
            pszLineBegin = pszStartOfCurrentWord;
        }
        //else do nothing

        if( pszCurrentCharacter - pszLineBegin > 0 ) 
        {
            vecLines.push_back(string({ pszLineBegin, (size_t)(pszCurrentCharacter - pszLineBegin) }));
        }
        //else do nothing
    }

    return vecLines;
}

void PdfPainter::DrawTextAligned(double dX, double dY, double dWidth, const string_view& rsText, EPdfAlignment eAlignment)
{
    CheckStream();

    if( !m_pFont || !m_canvas )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( dWidth <= 0.0 ) // nonsense arguments
        return;

    switch( eAlignment ) 
    {
        default:
        case EPdfAlignment::Left:
            break;
        case EPdfAlignment::Center:
            dX += (dWidth - m_pFont->StringWidth( rsText, m_textState ) ) / 2.0;
            break;
        case EPdfAlignment::Right:
            dX += (dWidth - m_pFont->StringWidth( rsText, m_textState) );
            break;
    }

    this->DrawText( dX, dY, rsText );
}

void PdfPainter::DrawImage( double dX, double dY, const PdfImage* pObject, double dScaleX, double dScaleY )
{
    this->DrawXObject( dX, dY, pObject, 
                       dScaleX * pObject->GetRect().GetWidth(), 
                       dScaleY * pObject->GetRect().GetHeight() );
}

void PdfPainter::DrawXObject( double dX, double dY, const PdfXObject* pObject, double dScaleX, double dScaleY )
{
    CheckStream();

    if( !pObject )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // use OriginalReference() as the XObject might have been written to disk
    // already and is not in memory anymore in this case.
    this->AddToPageResources( pObject->GetIdentifier(), pObject->GetObjectReference(), "XObject" );

	std::streamsize oldPrecision = m_tmpStream.precision(clPainterHighPrecision);
    m_tmpStream << "q" << std::endl
          << dScaleX << " 0 0 "
          << dScaleY << " "
          << dX << " " 
          << dY << " cm" << std::endl
          << "/" << pObject->GetIdentifier().GetString() << " Do" << std::endl << "Q" << std::endl;
	m_tmpStream.precision(oldPrecision);
}

void PdfPainter::ClosePath()
{
    CheckStream();

	 m_curPath << "h" << std::endl;

     m_tmpStream << "h\n";
}

void PdfPainter::LineTo( double dX, double dY )
{
    CheckStream();
    
	 m_curPath
			 << dX << " "
          << dY
          << " l" << std::endl;

    m_tmpStream << dX << " "
          << dY
          << " l" << std::endl;
}

void PdfPainter::MoveTo( double dX, double dY )
{
    CheckStream();
    
    m_curPath
        << dX << " "
        << dY
        << " m" << std::endl;

    m_tmpStream << dX << " "
          << dY
          << " m" << std::endl;
}

void PdfPainter::CubicBezierTo( double dX1, double dY1, double dX2, double dY2, double dX3, double dY3 )
{
    CheckStream();

	 m_curPath
         << dX1 << " "
         << dY1 << " "
         << dX2 << " "
         << dY2 << " "
         << dX3 << " "
         << dY3 
         << " c" << std::endl;

    m_tmpStream << dX1 << " "
          << dY1 << " "
          << dX2 << " "
          << dY2 << " "
          << dX3 << " "
          << dY3 
          << " c" << std::endl;
}

void PdfPainter::HorizontalLineTo( double inX )
{
    LineTo( inX, lpy3 );
}

void PdfPainter::VerticalLineTo( double inY )
{
    LineTo( lpx3, inY );
}

void PdfPainter::SmoothCurveTo( double inX2, double inY2, double inX3, double inY3 )
{
    double
        px, py, px2 = inX2, 
        py2 = inY2, 
        px3 = inX3, py3 = inY3;

    // compute the reflective points (thanks Raph!)
    px = 2 * lcx - lrx;
    py = 2 * lcy - lry;

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = px2;    lry = py2;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::QuadCurveTo( double inX1, double inY1, double inX3, double inY3 )
{
    double px = inX1, py = inY1, 
           px2, py2, 
           px3 = inX3, py3 = inY3;

    /* raise quadratic bezier to cubic    - thanks Raph!
        http://www.icce.rug.nl/erikjan/bluefuzz/beziers/beziers/beziers.html
    */
    px = (lcx + 2 * px) * (1.0 / 3.0);
    py = (lcy + 2 * py) * (1.0 / 3.0);
    px2 = (px3 + 2 * px) * (1.0 / 3.0);
    py2 = (py3 + 2 * py) * (1.0 / 3.0);

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = px2;    lry = py2;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::SmoothQuadCurveTo( double inX3, double inY3 )
{
    double px, py, px2, py2, 
           px3 = inX3, py3 = inY3;

    double xc, yc; /* quadratic control point */
    xc = 2 * lcx - lrx;
    yc = 2 * lcy - lry;

    /* generate a quadratic bezier with control point = xc, yc */
    px = (lcx + 2 * xc) * (1.0 / 3.0);
    py = (lcy + 2 * yc) * (1.0 / 3.0);
    px2 = (px3 + 2 * xc) * (1.0 / 3.0);
    py2 = (py3 + 2 * yc) * (1.0 / 3.0);

    lpx = px; lpy = py; lpx2 = px2; lpy2 = py2; lpx3 = px3; lpy3 = py3;
    lcx = px3;    lcy = py3;    lrx = xc;    lry = yc;    // thanks Raph!

    CubicBezierTo( px, py, px2, py2, px3, py3 );
}

void PdfPainter::ArcTo( double inX, double inY, double inRadiusX, double inRadiusY,
                       double    inRotation, bool inLarge, bool inSweep)
{
    double px = inX, py = inY;
    double rx = inRadiusX, ry = inRadiusY, rot = inRotation;
    int    large = ( inLarge ? 1 : 0 ),
           sweep = ( inSweep ? 1 : 0 );

    double sin_th, cos_th;
    double a00, a01, a10, a11;
    double x0, y0, x1, y1, xc, yc;
    double d, sfactor, sfactor_sq;
    double th0, th1, th_arc;
    int i, n_segs;

    sin_th     = sin (rot * (PI / 180.0));
    cos_th     = cos (rot * (PI / 180.0));
    a00        = cos_th / rx;
    a01        = sin_th / rx;
    a10        = -sin_th / ry;
    a11        = cos_th / ry;
    x0         = a00 * lcx + a01 * lcy;
    y0         = a10 * lcx + a11 * lcy;
    x1         = a00 * px + a01 * py;
    y1         = a10 * px + a11 * py;
    /* (x0, y0) is current point in transformed coordinate space.
     (x1, y1) is new point in transformed coordinate space.

     The arc fits a unit-radius circle in this space.
    */
    d = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
    sfactor_sq = 1.0 / d - 0.25;
    if (sfactor_sq < 0) sfactor_sq = 0;
    sfactor = sqrt (sfactor_sq);
    if (sweep == large) sfactor = -sfactor;
    xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
    yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);
    /* (xc, yc) is center of the circle. */

    th0 = atan2 (y0 - yc, x0 - xc);
    th1 = atan2 (y1 - yc, x1 - xc);

    th_arc = th1 - th0;
    if (th_arc < 0 && sweep)
        th_arc += 2 * PI;
    else if (th_arc > 0 && !sweep)
        th_arc -= 2 * PI;

    n_segs = static_cast<int>(ceil (fabs (th_arc / (PI * 0.5 + 0.001))));

    for (i = 0; i < n_segs; i++) {
        double nth0 = th0 + (double)i * th_arc / n_segs,
               nth1 = th0 + ((double)i + 1) * th_arc / n_segs;
        double nsin_th = 0.0,
                ncos_th = 0.0;
        double na00 = 0.0, 
               na01 = 0.0, 
               na10 = 0.0, 
               na11 = 0.0;
        double nx1 = 0.0, 
               ny1 = 0.0, 
               nx2 = 0.0, 
               ny2 = 0.0, 
               nx3 = 0.0,
               ny3 = 0.0;
        double t   = 0.0;
        double th_half = 0.0;

        nsin_th = sin (rot * (PI / 180.0));
        ncos_th = cos (rot * (PI / 180.0)); 
        /* inverse transform compared with rsvg_path_arc */
        na00 = ncos_th * rx;
        na01 = -nsin_th * ry;
        na10 = nsin_th * rx;
        na11 = ncos_th * ry;

        th_half = 0.5 * (nth1 - nth0);
        t = (8.0 / 3.0) * sin (th_half * 0.5) * sin (th_half * 0.5) / sin (th_half);
        nx1 = xc + cos (nth0) - t * sin (nth0);
        ny1 = yc + sin (nth0) + t * cos (nth0);
        nx3 = xc + cos (nth1);
        ny3 = yc + sin (nth1);
        nx2 = nx3 + t * sin (nth1);
        ny2 = ny3 - t * cos (nth1);
        nx1 = na00 * nx1 + na01 * ny1;
        ny1 = na10 * nx1 + na11 * ny1;
        nx2 = na00 * nx2 + na01 * ny2;
        ny2 = na10 * nx2 + na11 * ny2;
        nx3 = na00 * nx3 + na01 * ny3;
        ny3 = na10 * nx3 + na11 * ny3;
        CubicBezierTo( nx1, 
                       ny1,
                       nx2, 
                       ny2, 
                       nx3, 
                       ny3 );
    }

    lpx = lpx2 = lpx3 = px; lpy = lpy2 = lpy3 = py;
    lcx = px;    lcy = py;    lrx = px;    lry = py;    // thanks Raph!
}

// Peter Petrov 5 January 2009 was delivered from libHaru
bool PdfPainter::Arc(double dX, double dY, double dRadius, double dAngle1, double dAngle2)
{
    bool cont_flg = false;

    bool ret = true;

    if (dAngle1 >= dAngle2 || (dAngle2 - dAngle1) >= 360.0f)
        return false;

    while (dAngle1 < 0.0f || dAngle2 < 0.0f) {
        dAngle1 = dAngle1 + 360.0f;
        dAngle2 = dAngle2 + 360.0f;
    }

    for (;;) {
        if (dAngle2 - dAngle1 <= 90.0f)
            return InternalArc (dX, dY, dRadius, dAngle1, dAngle2, cont_flg);
        else {
            double tmp_ang = dAngle1 + 90.0f;

            ret = InternalArc (dX, dY, dRadius, dAngle1, tmp_ang, cont_flg);
            if (!ret)
                return ret;

            dAngle1 = tmp_ang;
        }

        if (dAngle1 >= dAngle2)
            break;

        cont_flg = true;
    }

    return true;
}

bool PdfPainter::InternalArc(
              double    x,
              double    y,
              double    ray,
              double    ang1,
              double    ang2,
              bool      cont_flg)
{
    bool ret = true;

    double rx0, ry0, rx1, ry1, rx2, ry2, rx3, ry3;
    double x0, y0, x1, y1, x2, y2, x3, y3;
    double delta_angle = (90.0f - static_cast<double>(ang1 + ang2) / 2.0f) / 180.0f * PI;
    double new_angle = static_cast<double>(ang2 - ang1) / 2.0f / 180.0f * PI;

    rx0 = ray * cos (new_angle);
    ry0 = ray * sin (new_angle);
    rx2 = (ray * 4.0f - rx0) / 3.0f;
    ry2 = ((ray * 1.0f - rx0) * (rx0 - ray * 3.0f)) / (3.0 * ry0);
    rx1 = rx2;
    ry1 = -ry2;
    rx3 = rx0;
    ry3 = -ry0;

    x0 = rx0 * cos (delta_angle) - ry0 * sin (delta_angle) + x;
    y0 = rx0 * sin (delta_angle) + ry0 * cos (delta_angle) + y;
    x1 = rx1 * cos (delta_angle) - ry1 * sin (delta_angle) + x;
    y1 = rx1 * sin (delta_angle) + ry1 * cos (delta_angle) + y;
    x2 = rx2 * cos (delta_angle) - ry2 * sin (delta_angle) + x;
    y2 = rx2 * sin (delta_angle) + ry2 * cos (delta_angle) + y;
    x3 = rx3 * cos (delta_angle) - ry3 * sin (delta_angle) + x;
    y3 = rx3 * sin (delta_angle) + ry3 * cos (delta_angle) + y;

    if (!cont_flg) {
        MoveTo(x0,y0);
    }

    CubicBezierTo( x1, 
                   y1,
                   x2, 
                   y2, 
                   x3, 
                   y3 );

    //attr->cur_pos.x = (HPDF_REAL)x3;
    //attr->cur_pos.y = (HPDF_REAL)y3;
    lcx = x3;
    lcy = y3;

    lpx = lpx2 = lpx3 = x3; 
    lpy = lpy2 = lpy3 = y3;
    lcx = x3;   
    lcy = y3;    
    lrx = x3;    
    lry = y3;   

    return ret;
}

void PdfPainter::Close()
{
    CheckStream();

    m_curPath << "h" << std::endl;

    m_tmpStream << "h\n";
}

void PdfPainter::Stroke()
{
    CheckStream();

    m_curPath.str("");

    m_tmpStream << "S\n";
}

void PdfPainter::Fill(bool useEvenOddRule)
{
    CheckStream();

    m_curPath.str("");

    if (useEvenOddRule)
        m_tmpStream << "f*\n";
    else
        m_tmpStream << "f\n";
}

void PdfPainter::FillAndStroke(bool useEvenOddRule)
{
    CheckStream();

    m_curPath.str("");

    if (useEvenOddRule)
        m_tmpStream << "B*\n";
    else
        m_tmpStream << "B\n";
}

void PdfPainter::Clip( bool useEvenOddRule )
{
    CheckStream();
    
    if ( useEvenOddRule )
        m_tmpStream << "W* n\n";
    else
        m_tmpStream << "W n\n";
}

void PdfPainter::EndPath(void)
{
    CheckStream();

    m_curPath << "n" << std::endl;

    m_tmpStream << "n\n";
}

void PdfPainter::Save()
{
    CheckStream();

    m_tmpStream << "q\n";
}

void PdfPainter::Restore()
{
    CheckStream();

    m_tmpStream << "Q\n";
}

void PdfPainter::AddToPageResources( const PdfName & rIdentifier, const PdfReference & rRef, const PdfName & rName )
{
    if( !m_canvas )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    m_canvas->AddResource( rIdentifier, rRef, rName );
}

void PdfPainter::ConvertRectToBezier( double dX, double dY, double dWidth, double dHeight, double pdPointX[], double pdPointY[] )
{
    // this function is based on code from:
    // http://www.codeguru.com/Cpp/G-M/gdi/article.php/c131/
    // (Llew Goodstadt)

    // MAGICAL CONSTANT to map ellipse to beziers
    //                          2/3*(sqrt(2)-1) 
    const double dConvert =     0.2761423749154;

    double dOffX    = dWidth  * dConvert;
    double dOffY    = dHeight * dConvert;
    double dCenterX = dX + (dWidth / 2.0); 
    double dCenterY = dY + (dHeight / 2.0); 

    pdPointX[0]  =                            //------------------------//
    pdPointX[1]  =                            //                        //
    pdPointX[11] =                            //        2___3___4       //
    pdPointX[12] = dX;                        //     1             5    //
    pdPointX[5]  =                            //     |             |    //
    pdPointX[6]  =                            //     |             |    //
    pdPointX[7]  = dX + dWidth;               //     0,12          6    //
    pdPointX[2]  =                            //     |             |    //
    pdPointX[10] = dCenterX - dOffX;          //     |             |    //
    pdPointX[4]  =                            //    11             7    //
    pdPointX[8]  = dCenterX + dOffX;          //       10___9___8       //
    pdPointX[3]  =                            //                        //
    pdPointX[9]  = dCenterX;                  //------------------------//

    pdPointY[2]  =
    pdPointY[3]  =
    pdPointY[4]  = dY;
    pdPointY[8]  =
    pdPointY[9]  =
    pdPointY[10] = dY + dHeight;
    pdPointY[7]  =
    pdPointY[11] = dCenterY + dOffY;
    pdPointY[1]  =
    pdPointY[5]  = dCenterY - dOffY;
    pdPointY[0]  =
    pdPointY[12] =
    pdPointY[6]  = dCenterY;
}

void PdfPainter::SetCurrentStrokingColor()
{
    if ( m_isCurColorICCDepend )
    {
        m_tmpStream << "/" << m_CSTag     << " CS ";
        m_tmpStream << m_curColor.GetRed()   << " "
              << m_curColor.GetGreen() << " "
              << m_curColor.GetBlue()
              << " SC" << std::endl;
    }
    else
    {
        SetStrokingColor( m_curColor );
    }
}

void PdfPainter::SetTransformationMatrix( double a, double b, double c, double d, double e, double f )
{
    CheckStream();

	// Need more precision for transformation-matrix !!
	std::streamsize oldPrecision = m_tmpStream.precision(clPainterHighPrecision);
    m_tmpStream << a << " "
          << b << " "
          << c << " "
          << d << " "
          << e << " "
          << f << " cm" << std::endl;
	m_tmpStream.precision(oldPrecision);
}

void PdfPainter::SetExtGState( PdfExtGState* inGState )
{
    CheckStream();

    this->AddToPageResources( inGState->GetIdentifier(), inGState->GetObject()->GetIndirectReference(), PdfName("ExtGState") );
    
    m_tmpStream << "/" << inGState->GetIdentifier().GetString()
          << " gs" << std::endl;
}

void PdfPainter::SetRenderingIntent( char* intent )
{
    CheckStream();

    m_tmpStream << "/" << intent
          << " ri" << std::endl;
}

void PdfPainter::SetDependICCProfileColor( const PdfColor &rColor, const std::string &pCSTag )
{
    m_isCurColorICCDepend = true;
    m_curColor = rColor;
    m_CSTag = pCSTag;

    m_tmpStream << "/" << m_CSTag << " cs ";
    m_tmpStream << rColor.GetRed()   << " "
          << rColor.GetGreen() << " "
          << rColor.GetBlue()
          << " sc" << std::endl;
}

string PdfPainter::ExpandTabs(const string_view& str, ssize_t lStringLen) const
{
    throw std::runtime_error("Untested after utf-8 migration");
    unsigned nTabCnt  = 0;
    const char32_t cTab     = 0x0900;
    const char32_t cSpace   = 0x2000;

    if( lStringLen < 0)
        lStringLen = str.length();

    for (ssize_t i = 0; i < lStringLen; i++)
    {
        if (str[i] == '\t')
            ++nTabCnt;
    }

    // if no tabs are found: bail out!
    if (nTabCnt == 0)
        return (string)str;
    
    return ExpandTabsPrivate(str, m_nTabWidth, nTabCnt, '\t', ' ');
}

void PdfPainter::CheckStream()
{
    if (m_stream != nullptr)
        return;

    PODOFO_RAISE_LOGIC_IF(m_canvas == nullptr, "Call SetCanvas() first before doing drawing operations.");
    m_stream = &m_canvas->GetStreamForAppending((EPdfStreamAppendFlags)(m_flags & (~EPdfPainterFlags::NoSaveRestore)));
}

void PdfPainter::SetPrecision(unsigned short inPrec)
{
    m_tmpStream.precision(inPrec);
}

unsigned short PdfPainter::GetPrecision() const
{
    return static_cast<unsigned short>(m_tmpStream.precision());
}

void PdfPainter::SetClipRect(const PdfRect& rRect)
{
    this->SetClipRect(rRect.GetLeft(), rRect.GetBottom(), rRect.GetWidth(), rRect.GetHeight());
}

void PdfPainter::Rectangle(const PdfRect& rRect, double dRoundX, double dRoundY)
{
    this->Rectangle(rRect.GetLeft(), rRect.GetBottom(),
        rRect.GetWidth(), rRect.GetHeight(),
        dRoundX, dRoundY);
}

void PdfPainter::DrawMultiLineText(const PdfRect& rRect, const string_view& rsText,
    EPdfAlignment eAlignment, EPdfVerticalAlignment eVertical, bool bClip, bool bSkipSpaces)
{
    this->DrawMultiLineText(rRect.GetLeft(), rRect.GetBottom(), rRect.GetWidth(), rRect.GetHeight(),
        rsText, eAlignment, eVertical, bClip, bSkipSpaces);
}

string ExpandTabsPrivate(const string_view& str, int tabWidth, unsigned nTabCnt, char cTab, char cSpace)
{
    throw std::runtime_error("Untested after utf-8 migration");
    const char* pszText = str.data();
    size_t lStringLen = str.length();
    size_t lLen = lStringLen + nTabCnt * (tabWidth - 1);
    string ret;
    ret.resize(lLen);
    char* pszTab = ret.data();

    int i = 0;
    while (lStringLen--)
    {
        if (*pszText == cTab)
        {
            for (int z = 0; z < tabWidth; z++)
                pszTab[i + z] = cSpace;

            i += tabWidth;
        }
        else
            pszTab[i++] = *pszText;

        ++pszText;
    }

    return ret;
}
