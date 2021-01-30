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

#include "PdfFontCache.h" 

#ifdef WIN32
#include <WindowsLeanMean.h>
#endif // WIN32

#include <algorithm>

#include "base/PdfDefinesPrivate.h"
#include "base/PdfDictionary.h"
#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include "PdfDifferenceEncoding.h"
#include "PdfFont.h"
#include "PdfFontFactory.h"
#include "PdfFontMetricsFreetype.h"
#include "PdfFontMetricsBase14.h"
#include "PdfFontTTFSubset.h"
#include "PdfFontType1.h"

#include <utfcpp/utf8.h>

#include <ft2build.h>
#include <freetype/freetype.h>

using namespace std;

namespace PoDoFo {

#ifdef WIN32
//This function will recieve the device context for the ttc font, it will then extract necessary tables,and create the correct buffer.
//On error function return false
static bool GetFontFromCollection(HDC &hdc, char *&buffer, unsigned int &bufferLen)
{
    const DWORD ttcf_const = 0x66637474;
    unsigned int fileLen = GetFontData(hdc, ttcf_const, 0, 0, 0);
    unsigned int ttcLen = GetFontData(hdc, 0, 0, 0, 0);
    if (fileLen == GDI_ERROR || ttcLen == GDI_ERROR)
    {
        return false;
    }

    char *fileBuffer = (char*)podofo_malloc(fileLen);
    if (!fileBuffer)
    {
        PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
    }

    char *ttcBuffer = (char*)podofo_malloc(ttcLen);
    if (!ttcBuffer)
    {
        podofo_free(fileBuffer);
        PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
    }

    if (GetFontData(hdc, ttcf_const, 0, fileBuffer, fileLen) == GDI_ERROR)
    {
        podofo_free(fileBuffer);
        podofo_free(ttcBuffer);
        return false;
    }

    if (GetFontData(hdc, 0, 0, ttcBuffer, ttcLen) == GDI_ERROR)
    {
        podofo_free(fileBuffer);
        podofo_free(ttcBuffer);
        return false;
    }

    uint16_t numTables = FROM_BIG_ENDIAN(*(uint16_t*)(ttcBuffer + 4));
    unsigned outLen = 12 + 16 * numTables;
    char *entry = ttcBuffer + 12;
    int table;

    //us: see "http://www.microsoft.com/typography/otspec/otff.htm"
    for (table = 0; table < numTables; table++)
    {
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(entry + 12));
        length = (length + 3) & ~3;
        entry += 16;
        outLen += length;
    }
    char *outBuffer = (char*)podofo_malloc(outLen);
    if (!outBuffer)
    {
        podofo_free(fileBuffer);
        podofo_free(ttcBuffer);
        PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
    }
    // copy font header and table index (offsets need to be still adjusted)
    memcpy(outBuffer, ttcBuffer, 12 + 16 * numTables);
    uint32_t dstDataOffset = 12 + 16 * numTables;

    // process tables
    char *srcEntry = ttcBuffer + 12;
    char *dstEntry = outBuffer + 12;
    for (table = 0; table < numTables; table++)
    {
        // read source entry
        uint32_t offset = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 8));
        uint32_t length = FROM_BIG_ENDIAN(*(uint32_t*)(srcEntry + 12));
        length = (length + 3) & ~3;

        // adjust offset
        // U can use FromBigEndian() also to convert _to_ big endian
        *(uint32_t*)(dstEntry + 8) = FROM_BIG_ENDIAN(dstDataOffset);

        //copy data
        memcpy(outBuffer + dstDataOffset, fileBuffer + offset, length);
        dstDataOffset += length;

        // adjust table entry pointers for loop
        srcEntry += 16;
        dstEntry += 16;
    }

    podofo_free(fileBuffer);
    podofo_free(ttcBuffer);
    buffer = outBuffer;
    bufferLen = outLen;
    return true;
}

static bool GetDataFromHFONT( HFONT hf, char** outFontBuffer, unsigned int& outFontBufferLen, const LOGFONTW* inFont )
{
    (void)inFont;
    HDC hdc = GetDC(0);
    if ( hdc == nullptr )
        return false;
    HGDIOBJ oldFont = SelectObject(hdc, hf);    // Petr Petrov (22 December 2009)

    bool ok = false;

    // try get data from true type collection
    char *buffer = nullptr;
    const DWORD ttcf_const = 0x66637474;
    unsigned int bufferLen = GetFontData(hdc, 0, 0, 0, 0);
    unsigned int ttcLen = GetFontData(hdc, ttcf_const, 0, 0, 0);

	if (bufferLen != GDI_ERROR && ttcLen == GDI_ERROR)
    {
        buffer = (char *) podofo_malloc( bufferLen );
		if (!buffer)
		{
			PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
		}

        ok = GetFontData(hdc, 0, 0, buffer, (DWORD)bufferLen) != GDI_ERROR;
    }
	else if (bufferLen != GDI_ERROR)
    {
        ok = GetFontFromCollection(hdc, buffer, bufferLen);
    }

    // clean up
    SelectObject(hdc,oldFont);
    ReleaseDC(0,hdc);
    if(ok)
    {
        // on success set result buffer
        *outFontBuffer = buffer;
        outFontBufferLen = bufferLen;
    }
    else if(buffer)
    {
        // on failure free local buffer
        podofo_free(buffer);
    }
    return ok;
}

static bool GetDataFromLPFONT( const LOGFONTW* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    bool ok = false;
    HFONT hf = CreateFontIndirectW(inFont);
    if(hf)
    {
        ok = GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen, inFont );
        DeleteObject(hf);
    }
    return ok;
}

static bool GetDataFromLPFONT( const LOGFONTA* inFont, char** outFontBuffer, unsigned int& outFontBufferLen )
{
    bool ok = false;
    HFONT hf = CreateFontIndirectA(inFont);
    if(hf)
    {
        LOGFONTW inFontW;
        GetObjectW(hf, sizeof(LOGFONTW), &inFontW);
        ok = GetDataFromHFONT( hf, outFontBuffer, outFontBufferLen, &inFontW);
        DeleteObject(hf);
    }
    return ok;
}
#endif // WIN32

PdfFontCache::PdfFontCache( PdfVecObjects* pParent )
    : m_pParent( pParent )
{
#if defined(PODOFO_HAVE_FONTCONFIG)
    m_fontConfig = PdfFontConfigWrapper::GetInstance();
#endif
    Init();
}

PdfFontCache::~PdfFontCache()
{
    this->EmptyCache();

    if( m_ftLibrary ) 
    {
        FT_Done_FreeType( m_ftLibrary );
        m_ftLibrary = nullptr;
    }
}

void PdfFontCache::Init(void)
{
    m_sSubsetBasename[0] = 0;
    char *p = m_sSubsetBasename;
    int ii;
    for (ii = 0; ii < SUBSET_BASENAME_LEN; ii++, p++) {
        *p = 'A';
    }
    p[0] = '+';
    p[1] = 0;

    m_sSubsetBasename[0]--;

    // Initialize all the fonts stuff
    if( FT_Init_FreeType( &m_ftLibrary ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::FreeType );
    }
}

void PdfFontCache::EmptyCache() 
{
    TISortedFontList itFont = m_vecFonts.begin();

    while( itFont != m_vecFonts.end() )
    {
        delete (*itFont).m_pFont;
        ++itFont;
    }

    itFont = m_vecFontSubsets.begin();
    while( itFont != m_vecFontSubsets.end() )
    {
        delete (*itFont).m_pFont;
        ++itFont;
    }

    m_vecFonts.clear();
    m_vecFontSubsets.clear();
}

PdfFont* PdfFontCache::GetFont( PdfObject* pObject )
{
    TCISortedFontList it = m_vecFonts.begin();
    const PdfReference & ref = pObject->GetIndirectReference(); 

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->GetObject()->GetIndirectReference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetObject()->GetIndirectReference() == ref ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a new font
    PdfFont* pFont = PdfFontFactory::CreateFont( &m_ftLibrary, pObject );
    if( pFont ) 
    {
        TFontCacheElement element;
        element.m_pFont     = pFont;
        element.m_bBold     = pFont->IsBold();
        element.m_bItalic   = pFont->IsItalic();
        element.m_sFontName = pFont->GetFontMetrics()->GetFontname();
        element.m_pEncoding = nullptr;
        element.m_bIsSymbolCharset = pFont->GetFontMetrics()->IsSymbol();
        m_vecFonts.push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                bool bEmbedd, EFontCreationFlags eFontCreationFlags,
                                const PdfEncoding * const pEncoding, 
                                const char* pszFileName)
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont = nullptr;
    PdfFontMetrics*   pMetrics = nullptr;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
               TFontCacheElement( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding ) );

        
    if( it.first == it.second )
    {
        if ( (eFontCreationFlags & EFontCreationFlags::AutoSelectBase14) == EFontCreationFlags::AutoSelectBase14
             && PODOFO_Base14FontDef_FindBuiltinData(pszFontName) )
        {
            EPdfFontFlags eFlags = EPdfFontFlags::Normal;
            if( bBold )
            {
                if( bItalic )
                {
                    eFlags = EPdfFontFlags::BoldItalic;
                }
                else
                {
                    eFlags = EPdfFontFlags::Bold;
                }
            }
            else if( bItalic )
                eFlags = EPdfFontFlags::Italic;

            pFont = PdfFontFactory::CreateBase14Font(pszFontName, eFlags,
                        pEncoding, m_pParent);
            if( pFont ) 
            {
                TFontCacheElement element;
                element.m_pFont     = pFont;
                element.m_bBold     = pFont->IsBold();
                element.m_bItalic   = pFont->IsItalic();
                element.m_sFontName = pszFontName;
                element.m_pEncoding = pEncoding;
                element.m_bIsSymbolCharset = bSymbolCharset;

                // Do a sorted insert, so no need to sort again
                //rvecContainer.insert( itSorted, element ); 
                m_vecFonts.insert( it.first, element );
                
             }

        }

        if (!pFont)
        {
            bool bSubsetting = (eFontCreationFlags & EFontCreationFlags::Type1Subsetting) != EFontCreationFlags::None;
            std::string sPath;
            if ( pszFileName == nullptr )
                sPath = this->GetFontPath( pszFontName, bBold, bItalic );
            else
                sPath = pszFileName;
            
            if( sPath.empty() )
            {
#ifdef WIN32
                pFont = GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding, bSubsetting  );
#endif // WIN32
            }
            else
            {
                pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, sPath.c_str(), bSymbolCharset, bSubsetting ? genSubsetBasename() : nullptr );
                pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
                           bEmbedd, bBold, bItalic, pszFontName, pEncoding, bSubsetting );
            }

        }
    }
    else
        pFont = (*it.first).m_pFont;  

    return pFont;
}

#ifdef WIN32
PdfFont* PdfFontCache::GetFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    size_t lMaxLen = wcslen(pszFontName) * 5;

    if (lMaxLen == 0) 
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Font name is empty");
        
    char* pmbFontName = static_cast<char*>(podofo_malloc(lMaxLen));
    if (!pmbFontName)
    {
        PODOFO_RAISE_ERROR(EPdfError::OutOfMemory);
    }
    if( wcstombs(pmbFontName, pszFontName, lMaxLen) == static_cast<size_t>(-1) )
    {
        podofo_free(pmbFontName);
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Conversion to multibyte char failed");
    }

    TFontCacheElement element;
    element.m_bBold = bBold;
    element.m_bItalic = bItalic;
    element.m_pEncoding = pEncoding;
    element.m_sFontName = pmbFontName;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), element );
    
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const LOGFONTA &logFont, 
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont*          pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
         TFontCacheElement( logFont.lfFaceName, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, logFont.lfCharSet == SYMBOL_CHARSET, pEncoding ) );
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, logFont, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}

PdfFont* PdfFontCache::GetFont( const LOGFONTW &logFont, 
                                bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PODOFO_ASSERT( pEncoding );

    PdfFont * pFont;
    std::pair<TISortedFontList,TCISortedFontList> it;

    string fontname;
    utf8::utf16to8((char16_t *)logFont.lfFaceName, (char16_t *)logFont.lfFaceName + LF_FACESIZE, std::back_inserter(fontname));

    it = std::equal_range(m_vecFonts.begin(), m_vecFonts.end(),
         TFontCacheElement(fontname, logFont.lfWeight >= FW_BOLD ? true : false,
             logFont.lfItalic ? true : false, logFont.lfCharSet == SYMBOL_CHARSET, pEncoding));
    if( it.first == it.second )
        return GetWin32Font( it.first, m_vecFonts, logFont, bEmbedd, pEncoding );
    else
        pFont = (*it.first).m_pFont;
    
    return pFont;
}
#endif // WIN32

PdfFont* PdfFontCache::GetFont( FT_Face face, bool bSymbolCharset, bool bEmbedd, const PdfEncoding * const pEncoding )
{
    PdfFont*          pFont;
    PdfFontMetrics*   pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    std::string sName = FT_Get_Postscript_Name( face );
    if( sName.empty() )
    {
        PdfError::LogMessage( ELogSeverity::Critical, "Could not retrieve fontname for font!" );
        return nullptr;
    }

    bool bBold   = ((face->style_flags & FT_STYLE_FLAG_BOLD)   != 0);
    bool bItalic = ((face->style_flags & FT_STYLE_FLAG_ITALIC) != 0);

    it = std::equal_range( m_vecFonts.begin(), m_vecFonts.end(), 
               TFontCacheElement( sName.c_str(), bBold, bItalic, bSymbolCharset, pEncoding ) );
    if( it.first == it.second )
    {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, face, bSymbolCharset );
        pFont    = this->CreateFontObject( it.first, m_vecFonts, pMetrics, 
                       bEmbedd, bBold, bItalic, sName.c_str(), pEncoding );
    }
    else
        pFont = (*it.first).m_pFont;

    return pFont;
}

PdfFont* PdfFontCache::GetDuplicateFontType1( PdfFont * pFont, const char* pszSuffix )
{
    TCISortedFontList it = m_vecFonts.begin();

    std::string id = pFont->GetIdentifier().GetString();
    id += pszSuffix;

    // Search if the object is a cached normal font
    while( it != m_vecFonts.end() )
    {
        if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Search if the object is a cached font subset
    it = m_vecFontSubsets.begin();
    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->GetIdentifier() == id ) 
            return (*it).m_pFont;

        ++it;
    }

    // Create a copy of the font
    PODOFO_ASSERT( pFont->GetFontMetrics()->GetFontType() == EPdfFontType::Type1Pfb );
    PdfFontMetrics* pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pFont->GetFontMetrics()->GetFilename(), pFont->GetFontMetrics()->IsSymbol() );
    PdfFont* newFont = new PdfFontType1( static_cast<PdfFontType1 *>(pFont), pMetrics, pszSuffix, m_pParent );
    if( newFont ) 
    {
        std::string name = newFont->GetFontMetrics()->GetFontname();
        name += pszSuffix;
        TFontCacheElement element;
        element.m_pFont     = newFont;
        element.m_bBold     = newFont->IsBold();
        element.m_bItalic   = newFont->IsItalic();
        element.m_sFontName = name;
        element.m_pEncoding = newFont->GetEncoding();
          element.m_bIsSymbolCharset = pFont->GetFontMetrics()->IsSymbol();
        m_vecFonts  .push_back( element );
        
        // Now sort the font list
        std::sort( m_vecFonts.begin(), m_vecFonts.end() );
    }

    return newFont;
}

PdfFont* PdfFontCache::GetFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                      const PdfEncoding * const pEncoding,
                      const char* pszFileName )
{
    PdfFont*        pFont = 0;
    PdfFontMetrics* pMetrics;
    std::pair<TISortedFontList,TCISortedFontList> it;

    // WARNING: The characters are completely ignored right now!

    it = std::equal_range( m_vecFontSubsets.begin(), m_vecFontSubsets.end(), 
               TFontCacheElement( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding ) );
    if( it.first == it.second )
    {
        std::string sPath; 
        if( pszFileName == nullptr || *pszFileName == 0) 
        {
            sPath = this->GetFontPath( pszFontName, bBold, bItalic );
            if( sPath.empty() )
            {
#ifdef WIN32
                return GetWin32Font( it.first, m_vecFontSubsets, pszFontName, bBold, bItalic, bSymbolCharset, true, pEncoding, true );
#else       
                PdfError::LogMessage( ELogSeverity::Critical, "No path was found for the specified fontname: %s", pszFontName );
                return nullptr;
#endif // WIN32
            }
        }
        else {
            sPath = pszFileName;
        }
        
        pMetrics = PdfFontMetricsFreetype::CreateForSubsetting( &m_ftLibrary, sPath.c_str(), bSymbolCharset, genSubsetBasename() );
        pFont = this->CreateFontObject( it.first, m_vecFontSubsets, pMetrics, 
                                        true, bBold, bItalic, pszFontName, pEncoding, true );
    }
    else
        pFont = (*it.first).m_pFont;
    
    
    return pFont;
}

void PdfFontCache::EmbedSubsetFonts()
{
    TCISortedFontList it = m_vecFontSubsets.begin();

    while( it != m_vecFontSubsets.end() )
    {
        if( (*it).m_pFont->IsSubsetting() )
        {
            (*it).m_pFont->EmbedSubsetFont();
        }

        ++it;
    }
}

#ifdef WIN32
PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                     bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting )
{
    LOGFONTW lf;
    
    lf.lfHeight         = 0;
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = bBold ? FW_BOLD : 0;
    lf.lfItalic         = bItalic;
    lf.lfUnderline      = 0;
    lf.lfStrikeOut      = 0;
    lf.lfCharSet           = bSymbolCharset ? SYMBOL_CHARSET : DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    if (strlen(pszFontName) >= LF_FACESIZE)
        return nullptr;
    
    memset(&(lf.lfFaceName), 0, LF_FACESIZE);
    //strcpy( lf.lfFaceName, pszFontName );
    /*int destLen =*/ MultiByteToWideChar (0, 0, pszFontName, -1, lf.lfFaceName, LF_FACESIZE);

     return GetWin32Font(itSorted, vecContainer, lf, bEmbedd, pEncoding, pSubsetting);
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, 
                                     const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                     bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting )
{
    LOGFONTW    lf;
    
    lf.lfHeight         = 0;
    lf.lfWidth          = 0;
    lf.lfEscapement     = 0;
    lf.lfOrientation    = 0;
    lf.lfWeight         = bBold ? FW_BOLD : 0;
    lf.lfItalic         = bItalic;
    lf.lfUnderline      = 0;
    lf.lfStrikeOut      = 0;
    lf.lfCharSet           = bSymbolCharset ? SYMBOL_CHARSET : DEFAULT_CHARSET;
    lf.lfOutPrecision   = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    lf.lfQuality        = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    size_t lFontNameLen = wcslen(pszFontName);
    if (lFontNameLen >= LF_FACESIZE)
        return nullptr;
    if (lFontNameLen == 0)
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "Font name is empty");
    
    memset(&(lf.lfFaceName), 0, LF_FACESIZE);
    wcscpy( static_cast<wchar_t*>(lf.lfFaceName), pszFontName );
    
    return GetWin32Font(itSorted, vecContainer, lf, bEmbedd, pEncoding, pSubsetting);
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTA &logFont,
                                bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting)
{
    char*        pBuffer = nullptr;
    unsigned int nLen;

    if( !GetDataFromLPFONT( &logFont, &pBuffer, nLen ) )
        return nullptr;
    
    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = nullptr;
    try {
         pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen, logFont.lfCharSet == SYMBOL_CHARSET, pSubsetting ? genSubsetBasename() : nullptr );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
              bEmbedd, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, logFont.lfFaceName, pEncoding, pSubsetting );
    } catch( PdfError & error ) {
        podofo_free( pBuffer );
        throw error;
    }
    
    podofo_free( pBuffer );
    return pFont;
}

PdfFont* PdfFontCache::GetWin32Font( TISortedFontList itSorted, TSortedFontList & vecContainer, const LOGFONTW &logFont,
                                bool bEmbedd, const PdfEncoding * const pEncoding, bool pSubsetting)
{
    size_t lFontNameLen = wcslen(logFont.lfFaceName);
    if (lFontNameLen >= LF_FACESIZE)
        return nullptr;

    size_t lMaxLen = lFontNameLen * 5;
    char* pmbFontName = static_cast<char*>(podofo_malloc(lMaxLen));
    if( !pmbFontName )
    {
        PODOFO_RAISE_ERROR( EPdfError::OutOfMemory );
    }

    if( wcstombs( pmbFontName, logFont.lfFaceName, lMaxLen ) == static_cast<size_t>(-1) )
    {
        podofo_free( pmbFontName );
        PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "Conversion to multibyte char failed" );
    }

    char*        pBuffer = nullptr;
    unsigned int nLen;
    if (!GetDataFromLPFONT(&logFont, &pBuffer, nLen))
        return nullptr;

    PdfFontMetrics* pMetrics;
    PdfFont*        pFont = nullptr;
    try {
        pMetrics = new PdfFontMetricsFreetype( &m_ftLibrary, pBuffer, nLen, logFont.lfCharSet == SYMBOL_CHARSET, pSubsetting ? genSubsetBasename() : nullptr );
        pFont    = this->CreateFontObject( itSorted, vecContainer, pMetrics, 
              bEmbedd, logFont.lfWeight >= FW_BOLD ? true : false, logFont.lfItalic ? true : false, pmbFontName, pEncoding, pSubsetting );
        podofo_free( pmbFontName );
        pmbFontName = nullptr;
    } catch( PdfError & error ) {
        podofo_free( pmbFontName );
        pmbFontName = nullptr;
        podofo_free( pBuffer );
        throw error;
    }
    
    podofo_free( pBuffer );
    return pFont;
}

#endif // WIN32

std::string PdfFontCache::GetFontPath( const char* pszFontName, bool bBold, bool bItalic )
{
#if defined(PODOFO_HAVE_FONTCONFIG)
    std::string sPath = m_fontConfig->GetFontConfigFontPath( pszFontName, bBold, bItalic );
#else
    (void)pszFontName;
    (void)bBold;
    (void)bItalic;
    std::string sPath = "";
#endif
    return sPath;
}

PdfFont* PdfFontCache::CreateFontObject( TISortedFontList itSorted, TSortedFontList & rvecContainer, 
                     PdfFontMetrics* pMetrics, bool bEmbedd, bool bBold, bool bItalic, 
                     const char* pszFontName, const PdfEncoding * const pEncoding, bool bSubsetting ) 
{
    PdfFont* pFont;

    try {
        EPdfFontFlags nFlags = EPdfFontFlags::Normal;

        if ( bSubsetting )
            nFlags |= EPdfFontFlags::Subsetting;
        
        if( bEmbedd )
            nFlags |= EPdfFontFlags::Embedded;
        
        if( bBold ) 
            nFlags |= EPdfFontFlags::Bold;

        if( bItalic )
            nFlags |= EPdfFontFlags::Italic;
        
        pFont    = PdfFontFactory::CreateFontObject( pMetrics, nFlags, pEncoding, m_pParent );

        if( pFont ) 
        {
            TFontCacheElement element;
            element.m_pFont     = pFont;
            element.m_bBold     = pFont->IsBold();
            element.m_bItalic   = pFont->IsItalic();
            element.m_sFontName = pszFontName;
            element.m_pEncoding = pEncoding;
            element.m_bIsSymbolCharset = pMetrics->IsSymbol();
            
            // Do a sorted insert, so no need to sort again
            rvecContainer.insert( itSorted, element );
        }
    } catch( PdfError & e ) {
        e.AddToCallstack( __FILE__, __LINE__ );
        e.PrintErrorMsg();
        PdfError::LogMessage( ELogSeverity::Error, "Cannot initialize font: %s", pszFontName ? pszFontName : "" );
        return nullptr;
    }
    
    return pFont;
}

const char *PdfFontCache::genSubsetBasename(void)
{
    int ii = 0;
    while(ii < SUBSET_BASENAME_LEN)
    {
        m_sSubsetBasename[ii]++;
        if (m_sSubsetBasename[ii] <= 'Z')
        {
            break;
        }

        m_sSubsetBasename[ii] = 'A';
        ii++;
    }

    return m_sSubsetBasename;
}

#ifdef PODOFO_HAVE_FONTCONFIG

void PdfFontCache::SetFontConfigWrapper( PdfFontConfigWrapper * pFontConfig )
{
    if ( m_fontConfig == pFontConfig )
        return;

    if ( pFontConfig )
        m_fontConfig = pFontConfig;
    else
        m_fontConfig = PdfFontConfigWrapper::GetInstance();
}

#endif // PODOFO_HAVE_FONTCONFIG

};
