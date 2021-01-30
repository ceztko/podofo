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

#ifndef _PDF_FONT_TTF_SUBSET_H_
#define _PDF_FONT_TTF_SUBSET_H_

#include <string>
#include <vector>
#include <map>
#include <set>

#include "podofo/base/PdfDefines.h"
#include "PdfFontMetrics.h"

namespace PoDoFo {

class PdfInputDevice;
class PdfOutputDevice;

/**
 * Internal enum specifying the type of a fontfile.
 */
enum class EFontFileType
{
    TTF,    ///< TrueType Font
    TTC,    ///< TrueType Collection
    OTF,    ///< OpenType Font
    Unknown ///< Unknown
};

// This code is based heavily on code by ZhangYang 
// (张杨.国际) <zhang_yang@founder.com>
//
// (Do not put this into doxygen documentation blocks
//  as this will break latex documentation generation)

/**
 * This class is able to build a new TTF font with only
 * certain glyphs from an existing font.
 *
 */
class PODOFO_DOC_API PdfFontTTFSubset
{
public:
    /** Create a new PdfFontTTFSubset from an existing 
     *  TTF font file.
     *
     *  @param pszFontFileName path to a TTF file
     *  @param pMetrics font metrics object for this font
     *  @param nFaceIndex index of the face inside of the font
     */
    PdfFontTTFSubset( const char* pszFontFileName, PdfFontMetrics* pMetrics, unsigned short nFaceIndex = 0 );

    /** Create a new PdfFontTTFSubset from an existing 
     *  TTF font file using an input device.
     *
     *  @param pDevice a PdfInputDevice
     *  @param pMetrics font metrics object for this font
     *  @param eType the type of the font
     *  @param nFaceIndex index of the face inside of the font
     */
    PdfFontTTFSubset( PdfInputDevice* pDevice, PdfFontMetrics* pMetrics, EFontFileType eType, unsigned short nFaceIndex = 0 );

    ~PdfFontTTFSubset();

    /**
     * Actually generate the subsetted font
     *
     * @param pOutputDevice write the font to this device
     */
    void BuildFont( PdfRefCountedBuffer& outputBuffer, const std::set<char32_t>& usedChars, std::vector<unsigned char>& cidSet );

private:
    /** copy constructor, not implemented
     */
    PdfFontTTFSubset(const PdfFontTTFSubset& rhs);
    /** assignment operator, not implemented
     */
    PdfFontTTFSubset& operator=(const PdfFontTTFSubset& rhs) = delete;

    void Init();
    
    /** Get the offset of a specified table. 
     *  @param pszTableName name of the table
     */
    unsigned GetTableOffset(unsigned tag);

    void GetNumberOfTables();
    void GetNumberOfGlyphs();
    void SeeIfLongLocaOrNot();
    void InitTables();
    void GetStartOfTTFOffsets();

    /** Get sz bytes from the offset'th bytes of the input file
     *
     */
    void GetData(unsigned offset, void* address, unsigned sz);


    /** Information of TrueType tables.
     */
    class TTrueTypeTable {
    public:
        TTrueTypeTable()
            : tag( 0L ), checksum( 0L ), length( 0L ), offset( 0L )
        {
        }
        
        uint32_t tag;
        uint32_t checksum;
        uint32_t length;
        uint32_t offset;
    };

    /** GlyphData contains the glyph address relative 
     *  to the beginning of the glyf table.
     */
    class TGlyphData
    {
    public:
        TGlyphData()
            : glyphLength( 0L ), glyphAddress( 0L )
        {
        }
        
        uint32_t glyphLength;
        uint32_t glyphAddress;	//In the original truetype file.
    };

    typedef unsigned short GID;
    typedef unsigned CodePoint;
    typedef std::map<GID, TGlyphData> GlyphMap;
    typedef std::map<CodePoint, GID> CodePointToGid;

    class CMapv4Range
    {
    public:
        CMapv4Range()
            : endCode( 0 ), startCode( 0 ), delta( 0 ), offset( 0 )
        {
        }
        
	    unsigned short endCode;
	    unsigned short startCode;
	    short delta;
	    unsigned short offset;
    };

    typedef std::vector<CMapv4Range> CMapRanges;

    class CMap {
    public:
        CMap()
            : segCount( 0 )
        {
        }
        
	    unsigned short segCount;
	    CMapRanges ranges;
	    std::vector<unsigned short> glyphArray;
    };

    class GlyphContext
    {
    public:
        GlyphContext()
            : ulGlyfTableOffset( 0 ), ulLocaTableOffset( 0 ), contourCount( 0 ) , shortOffset( 0 )
        {
        }
        
        unsigned ulGlyfTableOffset;
        unsigned ulLocaTableOffset;
        /* Used internaly during recursive load */
        TGlyphData glyphData;
        int16_t contourCount;
        uint16_t shortOffset;
    };

    void BuildUsedCodes(CodePointToGid& usedCodes, const std::set<char32_t>& usedChars );
    void LoadGlyphs(GlyphContext& ctx, const CodePointToGid& usedCodes);
    void LoadGID(GlyphContext& ctx, GID gid);
    void LoadCompound(GlyphContext& ctx, unsigned offset);
    void CreateCmapTable( const CodePointToGid& usedCodes );
    void FillGlyphArray(const CodePointToGid& usedCodes, GID gid, unsigned short count);
    unsigned GetCmapTableSize();
    unsigned WriteCmapTable(char*);
    unsigned GetGlyphTableSize();
    unsigned WriteGlyphTable(char* bufp, unsigned ulGlyphTableOffset);
    unsigned GetLocaTableSize();
    unsigned WriteLocaTable(char* bufp);
    unsigned GetHmtxTableSize();
    unsigned CalculateSubsetSize();
    void WriteTables(PdfRefCountedBuffer& fontData);

private:
    PdfFontMetrics* m_pMetrics;                ///< FontMetrics object which is required to convert unicode character points to glyph ids
    EFontFileType   m_eFontFileType;
    bool	        m_bIsLongLoca;
    
    uint16_t m_numTables;
    uint16_t m_numGlyphs;
    uint16_t m_numHMetrics;
    
    std::vector<TTrueTypeTable> m_vTable;
    GlyphMap m_mGlyphMap;
    CMap m_sCMap;
    
    /* temp storage during load */

    unsigned short  m_faceIndex;

    uint32_t m_ulStartOfTTFOffsets;	///< Start address of the truetype offset tables, differs from ttf to ttc.

    PdfInputDevice* m_pDevice;                  ///< Read data from this input device
    const bool      m_bOwnDevice;               ///< If the input device is owned by this object
};

}; /* PoDoFo */

#endif /* _PDF_FONT_TRUE_TYPE_H_ */
