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

#ifndef _PDF_PARSER_OBJECT_H_
#define _PDF_PARSER_OBJECT_H_

#include "PdfDefines.h"
#include "PdfObject.h"
#include "PdfTokenizer.h"

namespace PoDoFo {

class PdfEncrypt;

/**
 * A PdfParserObject constructs a PdfObject from a PDF file.
 * Parsing starts always at the current file position.
 */
class PODOFO_API PdfParserObject : public PdfObject
{
public:
    /** Parse the object data from the given file handle starting at
     *  the current position.
     *  \param document document where to resolve object references
     *  \param rDevice an open reference counted input device which is positioned in
     *                 front of the object which is going to be parsed.
     *  \param rBuffer buffer to use for parsing to avoid reallocations
     *  \param lOffset the position in the device from which the object shall be read
     *                 if lOffset = -1, the object will be read from the current 
     *                 position in the file.
     */
    PdfParserObject(PdfDocument& document, const PdfRefCountedInputDevice & rDevice, const PdfRefCountedBuffer & rBuffer, ssize_t lOffset = -1 );

    /** Parse the object data for an internal object.
     *  You have to call ParseDictionaryKeys as next function call.
     *
     *  The following two parameters are used to avoid allocation of a new
     *  buffer in PdfSimpleParser.
     *
     *  \warning This constructor is for internal usage only!
     *
     *  \param rBuffer buffer to use for parsing to avoid reallocations
     */
    explicit PdfParserObject( const PdfRefCountedBuffer & rBuffer );

    /** Tries to free all memory allocated by this
     *  PdfObject (variables and streams) and reads
     *  it from disk again if it is requested another time.
     *
     *  This will only work if load on demand is used.
     *  If the object is dirty if will not be free'd.
     *
     *  \param bForce if true the object will be free'd
     *                even if IsDirty() returns true.
     *                So you will loose any changes made
     *                to this object.
     *
     *  \see IsLoadOnDemand
     *  \see IsDirty
     */
    void FreeObjectMemory(bool bForce = false);

    /** Parse the object data from the given file handle 
     *  If delayed loading is enabled, only the object and generation number
     *  is read now and everything else is read later.
     *
     *  \param pEncrypt an encryption dictionary which is used to decrypt 
     *                  strings and streams during parsing or nullptr if the PDF
     *                  file was not encrypted
     *  \param bIsTrailer wether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFile( PdfEncrypt* pEncrypt, bool bIsTrailer = false );

    void ForceStreamParse();

    /** Returns if this object has a stream object appended.
     *  which has to be parsed.
     *  \returns true if there is a stream
     */
    inline bool HasStreamToParse() const { return m_bStream; }

    /** \returns true if this PdfParser loads all objects at
     *                the time they are accessed for the first time.
     *                The default is to load all object immediately.
     *                In this case false is returned.
     */
    inline bool IsLoadOnDemand() const { return m_bLoadOnDemand; }

    /** Sets wether this object shall be loaded on demand
     *  when it's data is accessed for the first time.
     *  \param bDelayed if true the object is loaded delayed.
     */
    inline void SetLoadOnDemand( bool bDelayed ) { m_bLoadOnDemand = bDelayed; }

    /** Gets an offset in which the object beginning is stored in the file.
     *  Note the offset points just after the object identificator ("0 0 obj").
     *
     * \returns an offset in which the object is stored in the source device,
     *     or -1, if the object was created on demand.
     */
    inline ssize_t GetOffset() const { return m_lOffset; }

 protected:
    /** Load all data of the object if load object on demand is enabled.
     *  Reimplemented from PdfVariant. Do not call this directly, use
     *  DelayedLoad().
     */
    void DelayedLoadImpl() override;

    void DelayedLoadStreamImpl() override;

 private:
     /** Starts reading at the file position m_lStreamOffset and interprets all bytes
      *  as contents of the objects stream.
      *  It is assumed that the dictionary has a valid /Length key already.
      *
      *  Called from DelayedLoadStream(). Do not call directly.
      */
     void ParseStream();

    /** Initialize private members in this object with their default values
     */
    void InitPdfParserObject();

    /** Parse the object data from the given file handle 
     *  \param bIsTrailer wether this is a trailer dictionary or not.
     *                    trailer dictionaries do not have a object number etc.
     */
    void ParseFileComplete( bool bIsTrailer );

    void ReadObjectNumber();

private:
    PdfRefCountedInputDevice m_device;
    PdfRefCountedBuffer m_buffer;
    PdfTokenizer m_tokenizer;
    PdfEncrypt* m_pEncrypt;
    bool        m_bIsTrailer;

    // Should the object try to defer loading of its contents until needed?
    // If false, object contents will be loaded during ParseFile(...). Note that
    //          this still uses the delayed loading infrastructure.
    // If true, loading will be triggered the first time the information is needed by
    //          an external caller.
    // Outside callers should not be able to tell the difference between the two modes
    // of operation.
    bool m_bLoadOnDemand;
    ssize_t m_lOffset;
    bool m_bStream;
    size_t m_lStreamOffset;
};

};

#endif // _PDF_PARSER_OBJECT_H_
