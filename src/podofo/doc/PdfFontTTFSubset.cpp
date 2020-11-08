/***************************************************************************
 *   Copyright (C) 2008 by Dominik Seichter                                *
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

#include "PdfFontTTFSubset.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfInputDevice.h"
#include "base/PdfOutputDevice.h"

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/tttables.h>
#include <freetype/tttags.h>

#include <cstring>
#include <iostream>
#include <algorithm>

using namespace std;
using namespace PoDoFo;

static constexpr unsigned LENGTH_HEADER12 = 12;
static constexpr unsigned LENGTH_OFFSETTABLE16 = 16;
static constexpr unsigned LENGTH_DWORD = 4;
static constexpr unsigned LENGTH_WORD = 2;

inline void TTFWriteUInt32(char *bufp, uint32_t value)
{
    bufp[0] = static_cast<char>(value >> 24);
    bufp[1] = static_cast<char>(value >> 16);
    bufp[2] = static_cast<char>(value >>  8);
    bufp[3] = static_cast<char>(value);
}

inline void TTFWriteUInt16(char *bufp, uint16_t value)
{
    bufp[0] = static_cast<char>(value >> 8);
    bufp[1] = static_cast<char>(value);
}

//Get the number of bytes to pad the ul, because of 4-byte-alignment.
static unsigned TableCheksum(const char* bufp, unsigned size);
static uint16_t xln2(uint16_t v);

static unsigned TableCheksum(const char* bufp, unsigned size)
{
    unsigned chksum = 0;
    unsigned v = 0;
    for(unsigned offset = 0; offset < size; offset += 4)
    {
        v = ((static_cast<unsigned>(bufp[offset + 0]) << 24) & 0xff000000) 
          | ((static_cast<unsigned>(bufp[offset + 1]) << 16) & 0x00ff0000) 
          | ((static_cast<unsigned>(bufp[offset + 2]) <<  8) & 0x0000ff00) 
          | ((static_cast<unsigned>(bufp[offset + 3])      ) & 0x000000ff);
        chksum += v;
    }
    return chksum;
}

PdfFontTTFSubset::PdfFontTTFSubset( const char* pszFontFileName, PdfFontMetrics* pMetrics, unsigned short nFaceIndex )
    : m_pMetrics( pMetrics ), 
      m_bIsLongLoca( false ), m_numTables( 0 ), m_numGlyphs( 0 ), m_numHMetrics( 0 ), m_faceIndex( nFaceIndex ), m_ulStartOfTTFOffsets( 0 ),
      m_bOwnDevice( true )
{
    //File type is now distinguished by ext, which might cause problems.
    const char* pname = pszFontFileName;
    const char* ext   = pname + strlen(pname) - 3;

    if (PoDoFo::compat::strcasecmp(ext,"ttf") == 0)
        m_eFontFileType = EFontFileType::TTF;
    else if (PoDoFo::compat::strcasecmp(ext,"ttc") == 0)
        m_eFontFileType = EFontFileType::TTC;
    else if (PoDoFo::compat::strcasecmp(ext,"otf") == 0)
        m_eFontFileType = EFontFileType::OTF;
    else
        m_eFontFileType = EFontFileType::Unknown;

    m_pDevice = new PdfInputDevice( (string_view)pszFontFileName );
}

PdfFontTTFSubset::PdfFontTTFSubset( PdfInputDevice* pDevice, PdfFontMetrics* pMetrics, EFontFileType eType, unsigned short nFaceIndex )
    : m_pMetrics( pMetrics ), m_eFontFileType( eType ),
      m_bIsLongLoca( false ), m_numTables( 0 ), m_numGlyphs( 0 ), m_numHMetrics( 0 ), m_faceIndex( nFaceIndex ), m_ulStartOfTTFOffsets( 0 ),
      m_pDevice( pDevice ), m_bOwnDevice( false )
{
}

PdfFontTTFSubset::~PdfFontTTFSubset()
{
    if( m_bOwnDevice )
    {
        delete m_pDevice;
        m_pDevice = nullptr;
    }
}

void PdfFontTTFSubset::Init()
{
    GetStartOfTTFOffsets();
    GetNumberOfTables();
    InitTables();
    GetNumberOfGlyphs();
    SeeIfLongLocaOrNot();
}

unsigned PdfFontTTFSubset::GetTableOffset( unsigned tag )
{
    std::vector<TTrueTypeTable>::const_iterator it = m_vTable.begin();

    for (; it != m_vTable.end(); it++)
    {
        if (it->tag == tag)
            return it->offset;
    }
    PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "table missing" );
}

void PdfFontTTFSubset::GetNumberOfGlyphs()
{
    unsigned ulOffset = GetTableOffset( TTAG_maxp );

    GetData(ulOffset + LENGTH_DWORD * 1 , &m_numGlyphs, LENGTH_WORD);
    m_numGlyphs = FROM_BIG_ENDIAN(m_numGlyphs);

    ulOffset = GetTableOffset( TTAG_hhea );

    GetData(ulOffset + LENGTH_WORD * 17, &m_numHMetrics, LENGTH_WORD);
    m_numHMetrics = FROM_BIG_ENDIAN(m_numHMetrics);
}

void PdfFontTTFSubset::InitTables()
{
    unsigned short tableMask = 0;
    TTrueTypeTable tbl;

    for (unsigned short i = 0; i < m_numTables; i++)
    {
        // Name of each table:
        GetData(m_ulStartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i, &tbl.tag, LENGTH_DWORD);
        tbl.tag = FROM_BIG_ENDIAN(tbl.tag);

        // Checksum of each table:
        GetData(m_ulStartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 1, &tbl.checksum, LENGTH_DWORD);
        tbl.checksum = FROM_BIG_ENDIAN(tbl.checksum);

        // Offset of each table:
        GetData(m_ulStartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 2, &tbl.offset, LENGTH_DWORD);
        tbl.offset = FROM_BIG_ENDIAN(tbl.offset);

        // Length of each table:
        GetData(m_ulStartOfTTFOffsets + LENGTH_HEADER12 + LENGTH_OFFSETTABLE16 * i + LENGTH_DWORD * 3, &tbl.length, LENGTH_DWORD);
        tbl.length = FROM_BIG_ENDIAN(tbl.length);

        switch(tbl.tag)
        {
            case TTAG_head:
                tableMask |= 0x0001;
                break;
            case TTAG_maxp:
                tableMask |= 0x0002; 
                break;
            case TTAG_hhea:
                // required to get numHMetrics
                tableMask |= 0x0004; 
                break;
            case TTAG_glyf:
                tableMask |= 0x0008; 
                break;
            case TTAG_loca:
                tableMask |= 0x0010; 
                break;
            case TTAG_hmtx:
                // advance width
                tableMask |= 0x0020; 
                break;
            case TTAG_cmap:
                // cmap table will be generated later
                tableMask |= 0x0100;
                break;
            case TTAG_post:
                if (tbl.length < 32)
                    tbl.tag = 0;
 
                // reduce table size, leter we will change format to 0x00030000
                tbl.length = 32;
                break;
            case TTAG_cvt:
            case TTAG_fpgm:
            case TTAG_OS2:
            case TTAG_prep:
            //case TTAG_name:
                break;
            default:
                // exclude all other tables
                tbl.tag = 0;
                break;
        }
        if (tbl.tag)
            m_vTable.push_back(tbl);		
    }
    if ((tableMask & 0x3f )!= 0x3f)
    {
        PODOFO_RAISE_ERROR_INFO( EPdfError::UnsupportedFontFormat, "Required TrueType table missing" );
    }
    if ((tableMask & 0x0100 ) == 0x00)
    {
        tbl.tag = TTAG_cmap;
        tbl.checksum = 0;
        tbl.offset = 0;
        tbl.length = 0;
        m_vTable.push_back(tbl);
    }
    m_numTables = static_cast<unsigned short>(m_vTable.size());
}

void PdfFontTTFSubset::GetStartOfTTFOffsets()
{
    switch (m_eFontFileType)
    {
        case EFontFileType::TTF:
        case EFontFileType::OTF:
            m_ulStartOfTTFOffsets = 0x0;
            break;
        case EFontFileType::TTC:
        {
            uint32_t ulnumFace;
            GetData(8, &ulnumFace, LENGTH_DWORD);
            ulnumFace = FROM_BIG_ENDIAN(ulnumFace);
	    
            GetData((m_faceIndex + 3) * LENGTH_DWORD, &m_ulStartOfTTFOffsets, LENGTH_DWORD);
            m_ulStartOfTTFOffsets = FROM_BIG_ENDIAN(m_ulStartOfTTFOffsets);
        }
        break;
        case EFontFileType::Unknown:
        default:
            PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "Invalid font type" );
    }
}

void PdfFontTTFSubset::GetNumberOfTables()
{
    GetData(m_ulStartOfTTFOffsets + LENGTH_DWORD * 1, &m_numTables, LENGTH_WORD);
    m_numTables = FROM_BIG_ENDIAN(m_numTables);
}

void PdfFontTTFSubset::SeeIfLongLocaOrNot()
{
    uint16_t usIsLong; //1 for long
    unsigned ulHeadOffset = GetTableOffset(TTAG_head);
    GetData(ulHeadOffset + 50, &usIsLong, LENGTH_WORD);
    usIsLong = FROM_BIG_ENDIAN(usIsLong);
	m_bIsLongLoca = (usIsLong == 0 ? false : true);
}

uint16_t xln2(uint16_t v)
{
    uint16_t e = 0;
    while (v >>= 1)
        ++e;

    return e;
}

void PdfFontTTFSubset::BuildUsedCodes(CodePointToGid& usedCodes, const std::set<pdf_utf16be>& usedChars )
{
    CodePoint codePoint;
    GID gid;

    for (std::set<pdf_utf16be>::const_iterator it = usedChars.begin(); it != usedChars.end(); ++it)
    {
        codePoint = *it;
        gid = static_cast<GID>( m_pMetrics->GetGlyphId( codePoint ) );
        usedCodes[codePoint] = gid;
    }
}
	
void PdfFontTTFSubset::LoadGlyphs(GlyphContext& ctx, const CodePointToGid& usedCodes)
{
    // For any fonts, assume that glyph 0 is needed.
    LoadGID(ctx, 0);
    for (CodePointToGid::const_iterator cit = usedCodes.begin(); cit != usedCodes.end(); ++cit)
        LoadGID(ctx, cit->second);

    m_numGlyphs = 0;
    GlyphMap::reverse_iterator it = m_mGlyphMap.rbegin();
    if (it != m_mGlyphMap.rend())
        m_numGlyphs = it->first;

    ++m_numGlyphs;
    if (m_numHMetrics > m_numGlyphs)
        m_numHMetrics = m_numGlyphs;

}
	
void PdfFontTTFSubset::LoadGID(GlyphContext& ctx, GID gid)
{
    if (gid < m_numGlyphs)
    {
        if (!m_mGlyphMap.count(gid))
        {
            if (m_bIsLongLoca)
            {
                GetData(ctx.ulLocaTableOffset + LENGTH_DWORD * gid, &ctx.glyphData.glyphAddress, LENGTH_DWORD);
                ctx.glyphData.glyphAddress = FROM_BIG_ENDIAN(ctx.glyphData.glyphAddress);

                GetData(ctx.ulLocaTableOffset + LENGTH_DWORD * (gid + 1), &ctx.glyphData.glyphLength, LENGTH_DWORD);
                ctx.glyphData.glyphLength = FROM_BIG_ENDIAN(ctx.glyphData.glyphLength);
            }
            else
            {
                GetData(ctx.ulLocaTableOffset + LENGTH_WORD * gid, &ctx.shortOffset, LENGTH_WORD);
                ctx.glyphData.glyphAddress = FROM_BIG_ENDIAN(ctx.shortOffset);
                ctx.glyphData.glyphAddress <<= 1;

                GetData(ctx.ulLocaTableOffset + LENGTH_WORD * (gid + 1), &ctx.shortOffset, LENGTH_WORD);
                ctx.glyphData.glyphLength = FROM_BIG_ENDIAN(ctx.shortOffset);
                ctx.glyphData.glyphLength <<= 1;
            }
            ctx.glyphData.glyphLength -= ctx.glyphData.glyphAddress;

            m_mGlyphMap[gid] = ctx.glyphData;

            GetData(ctx.ulGlyfTableOffset + ctx.glyphData.glyphAddress, &ctx.contourCount, LENGTH_WORD);
            ctx.contourCount = FROM_BIG_ENDIAN(ctx.contourCount);
            if (ctx.contourCount < 0)
            {
                // skeep over numberOfContours, xMin, yMin, xMax and yMax
                LoadCompound(ctx, ctx.glyphData.glyphAddress + 5 * LENGTH_WORD);
            }
        }
        return;
    }
    PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "GID out of range" );
}

void PdfFontTTFSubset::LoadCompound(GlyphContext& ctx, unsigned offset)
{
    uint16_t flags;
    uint16_t glyphIndex;
	
    const int ARG_1_AND_2_ARE_WORDS = 0x01;
    const int WE_HAVE_A_SCALE          = 0x08;
    const int MORE_COMPONENTS       = 0x20;
    const int WE_HAVE_AN_X_AND_Y_SCALE = 0x40;
    const int WE_HAVE_TWO_BY_TWO       = 0x80;

    while(true)
    {
        GetData(ctx.ulGlyfTableOffset + offset, &flags, LENGTH_WORD);
        flags = FROM_BIG_ENDIAN(flags);
                    
        GetData(ctx.ulGlyfTableOffset + offset + LENGTH_WORD, &glyphIndex, LENGTH_WORD);
        glyphIndex = FROM_BIG_ENDIAN(glyphIndex);

        LoadGID(ctx, glyphIndex);

        if (!(flags & MORE_COMPONENTS))
            break;

        offset += (flags & ARG_1_AND_2_ARE_WORDS) ? 4 * LENGTH_WORD : 3 * LENGTH_WORD;
        if (flags & WE_HAVE_A_SCALE)
            offset +=  LENGTH_WORD;
        else if (flags & WE_HAVE_AN_X_AND_Y_SCALE)
            offset +=  2 * LENGTH_WORD;
        else if (flags & WE_HAVE_TWO_BY_TWO)
            offset +=  4 * LENGTH_WORD;
    }
}

unsigned PdfFontTTFSubset::GetHmtxTableSize()
{
    unsigned tableLength = static_cast<unsigned>(m_numGlyphs + m_numHMetrics) << 1;
    return tableLength;
}
	
unsigned PdfFontTTFSubset::GetCmapTableSize()
{
    unsigned tableSize = 0;
    tableSize += m_sCMap.segCount * 4 * LENGTH_WORD + LENGTH_WORD;
    tableSize += (unsigned)m_sCMap.glyphArray.size() * LENGTH_WORD;
    return tableSize + 12 + 14;
}

void PdfFontTTFSubset::CreateCmapTable( const CodePointToGid& usedCodes )
{
    CMapRanges cmapRanges;
    CMapv4Range range;

    unsigned short arrayCount = 0;

    CodePointToGid::const_iterator cit = usedCodes.begin();
    while (cit != usedCodes.end())
    {
        range.endCode = range.startCode = static_cast<unsigned short>(cit->first);
        range.delta   = static_cast<short>( cit->second - cit->first );
        range.offset  = 0;

        while (++cit != usedCodes.end())
        {
            if ((range.endCode + 1u) != cit->first)
                break;

            ++range.endCode;
            if (!range.offset)
                range.offset = range.endCode + range.delta - cit->second;
        }

        if (range.offset)
            arrayCount += range.endCode - range.startCode + 1;

        m_sCMap.ranges.push_back(range);
    }
    m_sCMap.segCount = static_cast<unsigned short>(m_sCMap.ranges.size() + 1);
    // fill glyphArray
    if (arrayCount)
    {
        m_sCMap.glyphArray.reserve(arrayCount);
        unsigned short arrayOffset  = m_sCMap.segCount * LENGTH_WORD;
        for (CMapRanges::iterator it = m_sCMap.ranges.begin(); it != m_sCMap.ranges.end(); ++it)
        {
            if (it->offset)
            {
                it->offset = arrayOffset;
                FillGlyphArray(usedCodes, it->startCode, it->endCode - it->startCode + 1);
                arrayOffset += (it->endCode - it->startCode + 1) * LENGTH_WORD;
            }
            arrayOffset -= LENGTH_WORD;
        }
    }
	    
    // append final range
    range.endCode = range.startCode = static_cast<unsigned short>(~0u);
    range.offset  = range.delta = 0;

    m_sCMap.ranges.push_back(range);
}
    
void PdfFontTTFSubset::FillGlyphArray(const CodePointToGid& usedCodes, GID gid, unsigned short count)
{
    CodePointToGid::const_iterator it = usedCodes.lower_bound(gid);
    do
    {
        if (it == usedCodes.end())
            PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "Unexpected" );

        m_sCMap.glyphArray.push_back(it->second);
        ++it;
    } while(--count);
}

unsigned PdfFontTTFSubset::WriteCmapTable(char* bufp)
{
    unsigned offset = 12;
    // version and number of subtables
    TTFWriteUInt16(bufp + 0, 0);
    TTFWriteUInt16(bufp + 2, 1);
        
    // platformID, platformSpecificID and offset
    TTFWriteUInt16(bufp + 4, 3);
    TTFWriteUInt16(bufp + 6, 1);
    TTFWriteUInt32(bufp + 8, offset);

    // adjust bufp to begin of cmap format 4 table
    bufp += offset;

    // format, length, language
    TTFWriteUInt16(bufp +  0, 4);
    TTFWriteUInt16(bufp +  2, 0);
    TTFWriteUInt16(bufp +  4, 0);

    // segCountX2
    TTFWriteUInt16(bufp +  6, m_sCMap.segCount << 1);

    uint16_t es = xln2(m_sCMap.segCount);
    uint16_t sr = 1 << (es + 1);
    // searchRange
    TTFWriteUInt16(bufp +  8, sr);
    // entrySelector
    TTFWriteUInt16(bufp + 10, es);
    // rangeShift
    TTFWriteUInt16(bufp + 12, (m_sCMap.segCount << 1) - sr);

    // adjust offset to first array
    offset = 14;

    CMapRanges::const_iterator it;

    // write endCode array
    for (it = m_sCMap.ranges.begin(); it != m_sCMap.ranges.end(); ++it)
    {
        TTFWriteUInt16(bufp + offset, it->endCode);
        offset += LENGTH_WORD;
    }
    // write reservedPad
    TTFWriteUInt16(bufp + offset, 0);
    offset += LENGTH_WORD;
    // write startCode array
    for (it = m_sCMap.ranges.begin(); it != m_sCMap.ranges.end(); ++it)
    {
        TTFWriteUInt16(bufp + offset, it->startCode);
        offset += LENGTH_WORD;
    }
    // write idDelta array
    for (it = m_sCMap.ranges.begin(); it != m_sCMap.ranges.end(); ++it)
    {
        TTFWriteUInt16(bufp + offset, it->delta);
        offset += LENGTH_WORD;
    }
    // write idRangeOffset array
    for (it = m_sCMap.ranges.begin(); it != m_sCMap.ranges.end(); ++it)
    {
        TTFWriteUInt16(bufp + offset, it->offset);
        offset += LENGTH_WORD;
    }
    std::vector<unsigned short>::const_iterator uit;
    // write glyphIndexArray
    for (uit = m_sCMap.glyphArray.begin(); uit != m_sCMap.glyphArray.end(); ++uit)
    {
        TTFWriteUInt16(bufp + offset, *uit);
        offset += LENGTH_WORD;
    }
    // update length of this table
    TTFWriteUInt16(bufp + 2, offset);
    // return total length of cmap tables
    return offset + 12;
}


unsigned PdfFontTTFSubset::GetGlyphTableSize()
{
    unsigned glyphTableSize = 0;
    for(GlyphMap::const_iterator it = m_mGlyphMap.begin(); it != m_mGlyphMap.end(); ++it)
        glyphTableSize += it->second.glyphLength;

    return glyphTableSize;
}

unsigned PdfFontTTFSubset::WriteGlyphTable(char* bufp, unsigned ulGlyphTableOffset)
{
    unsigned offset = 0;
    for(GlyphMap::const_iterator it = m_mGlyphMap.begin(); it != m_mGlyphMap.end(); ++it)
    {
        if (it->second.glyphLength)
        {
            GetData( ulGlyphTableOffset + it->second.glyphAddress, bufp + offset, it->second.glyphLength);
            offset += it->second.glyphLength;
        }
    }
    return offset;
}
	
unsigned PdfFontTTFSubset::GetLocaTableSize()
{
    unsigned offset = static_cast<unsigned>(m_numGlyphs + 1);
    return (m_bIsLongLoca) ? offset << 2 : offset << 1;
}

unsigned PdfFontTTFSubset::WriteLocaTable(char* bufp)
{
    GID glyphIndex = 0;
    unsigned offset = 0;
    unsigned glyphAddress = 0;

    if (m_bIsLongLoca)
    {
        for(GlyphMap::const_iterator it = m_mGlyphMap.begin(); it != m_mGlyphMap.end(); ++it)
        {
            while(glyphIndex < it->first)
            {
                // set the glyph length to zero
                TTFWriteUInt32(bufp + offset, glyphAddress);
                offset += 4;
                ++glyphIndex;
            }

            TTFWriteUInt32(bufp + offset, glyphAddress);
            glyphAddress += it->second.glyphLength;
            offset += 4;
            ++glyphIndex;
        }

        TTFWriteUInt32(bufp + offset, glyphAddress);
        offset += 4;
    }
    else
    {
        for(GlyphMap::const_iterator it = m_mGlyphMap.begin(); it != m_mGlyphMap.end(); ++it)
        {
            while(glyphIndex < it->first)
            {
                TTFWriteUInt16(bufp + offset, static_cast<uint16_t>(glyphAddress >> 1));
                offset += 2;
                ++glyphIndex;
            }

            TTFWriteUInt16(bufp + offset, static_cast<uint16_t>(glyphAddress >> 1));
            glyphAddress += it->second.glyphLength;
            offset += 2;
            ++glyphIndex;
        }

        TTFWriteUInt16(bufp + offset, static_cast<uint16_t>(glyphAddress >> 1));
        offset += 2;
    }
    return offset;
}
        
unsigned PdfFontTTFSubset::CalculateSubsetSize()
{
    unsigned subsetLength = LENGTH_HEADER12 + m_numTables * LENGTH_OFFSETTABLE16;
    unsigned tableLength;

    for (std::vector<TTrueTypeTable>::iterator it = m_vTable.begin(); it != m_vTable.end(); it++)
    {
        switch(it->tag)
        {
            case TTAG_glyf:
                tableLength = GetGlyphTableSize();
                break;
            case TTAG_loca:
                tableLength = GetLocaTableSize();
                break;
            case TTAG_hmtx:
                tableLength = GetHmtxTableSize();
                break;
            case TTAG_cmap:
                tableLength = GetCmapTableSize();
                break;
            default:
                tableLength = it->length;
                break;
        }

        it->length = tableLength;
        subsetLength += (tableLength + 3) & ~3;
    }

    return subsetLength;
}

void PdfFontTTFSubset::WriteTables(PdfRefCountedBuffer& fontData)
{
    fontData.Resize( CalculateSubsetSize() );
    char *bufp = fontData.GetBuffer();
        
    // write TFF Offset table
    {
        uint16_t es = xln2(m_numTables);
        uint16_t sr = es << 4;

        TTFWriteUInt32(bufp + 0, 0x00010000);
        TTFWriteUInt16(bufp + 4, m_numTables);
        TTFWriteUInt16(bufp + 6, sr);
        TTFWriteUInt16(bufp + 8, es);
        es  = (m_numTables << 4) - sr;
        TTFWriteUInt16(bufp + 10, es);
    }

    unsigned headOffset = 0;
    unsigned dirOffset = LENGTH_HEADER12;
    unsigned tableOffset = dirOffset + m_numTables * LENGTH_OFFSETTABLE16;
    unsigned tableLength;
    unsigned totalLength;

    for (std::vector<TTrueTypeTable>::const_iterator it = m_vTable.begin(); it != m_vTable.end(); it++)
    {
        tableLength = 0;
        switch(it->tag)
        {
            case TTAG_head:
                headOffset = tableOffset;
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                TTFWriteUInt32(bufp + tableOffset + 8, 0);
                break;
            case TTAG_maxp:
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                TTFWriteUInt16(bufp + tableOffset + 4, m_numGlyphs);
                break;
            case TTAG_hhea:
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                TTFWriteUInt16(bufp + tableOffset + 34, m_numHMetrics);
                break;
            case TTAG_hmtx:
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                break;
            case TTAG_post:
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                TTFWriteUInt32(bufp + tableOffset, 0x00030000);
                memset(bufp + tableOffset + 16, 0, 16);
                break;
            case TTAG_glyf:
                tableLength = WriteGlyphTable(bufp + tableOffset, it->offset);
                break;
            case TTAG_loca:
                tableLength = WriteLocaTable(bufp + tableOffset);
                break;
            case TTAG_cmap:
                tableLength = WriteCmapTable(bufp + tableOffset);
                break;
            default:
                tableLength = it->length;
                GetData( it->offset, bufp + tableOffset, tableLength);
                break;
        }

        if (tableLength)
        {
            // write TFF Directory table entry
            totalLength = tableLength;
            while(totalLength & 3)
                bufp[tableOffset + totalLength++] = '\0';
		
            TTFWriteUInt32(bufp + dirOffset, it->tag);
            TTFWriteUInt32(bufp + dirOffset +  4, TableCheksum(bufp + tableOffset, totalLength));
            TTFWriteUInt32(bufp + dirOffset +  8, tableOffset);
            TTFWriteUInt32(bufp + dirOffset + 12, tableLength);

            tableOffset += totalLength;
            dirOffset += 16;
        }
    }

    // head table
    if (!headOffset)
        PODOFO_RAISE_ERROR_INFO( EPdfError::InternalLogic, "'head' table missing" );

    TTFWriteUInt32(bufp + headOffset + 8, TableCheksum(bufp, tableLength) - 0xB1B0AFBA);
}
	    
void PdfFontTTFSubset::BuildFont( PdfRefCountedBuffer& outputBuffer, const std::set<pdf_utf16be>& usedChars, std::vector<unsigned char>& cidSet )
{
    Init();

    GlyphContext context;
    context.ulGlyfTableOffset = GetTableOffset(TTAG_glyf);
    context.ulLocaTableOffset = GetTableOffset(TTAG_loca);
    {
        CodePointToGid usedCodes;

        BuildUsedCodes(usedCodes, usedChars);
        CreateCmapTable(usedCodes);
        LoadGlyphs(context, usedCodes);
    }

    if (m_numGlyphs)
    {
        cidSet.assign((m_numGlyphs + 7) >> 3, 0);
        GlyphMap::reverse_iterator rit = m_mGlyphMap.rbegin();
        if (rit != m_mGlyphMap.rend())
        {
            static const unsigned char bits[] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
            do
            {
                cidSet[rit->first >> 3] |= bits[rit->first & 7];
            } while(++rit != m_mGlyphMap.rend());    
        }
    }
    WriteTables(outputBuffer);
}

void PdfFontTTFSubset::GetData(unsigned offset, void* address, unsigned sz)
{
    m_pDevice->Seek( offset );
    m_pDevice->Read( static_cast<char*>(address), sz );
}
