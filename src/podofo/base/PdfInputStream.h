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

#ifndef PDF_INPUT_STREAM_H
#define PDF_INPUT_STREAM_H

#include <fstream>

#include "PdfDefines.h"

namespace PoDoFo {

class PdfInputDevice;

/** An interface for reading blocks of data from an 
 *  a data source.
 */     
class PODOFO_API PdfInputStream
{
public:
    PdfInputStream();
    virtual ~PdfInputStream() { };

    /** Read data from the input stream
     *
     *  \param pBuffer    the data will be stored into this buffer
     *  \param lLen       the size of the buffer and number of bytes
     *                    that will be read
     *  \param eof        true if the stream reached eof
     *
     *  \returns the number of bytes read
     */
    size_t Read(char* pBuffer, size_t lLen, bool &eof);

public:
    inline bool Eof() const { return m_eof; }

protected:
    virtual size_t ReadImpl( char* pBuffer, size_t lLen, bool& eof) = 0;

private:
    bool m_eof;
};

/** An input stream that reads data from a file
 */
class PODOFO_API PdfFileInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *  
     *  \param pszFilename the filename of the file to read
     */
    PdfFileInputStream(const std::string_view& filename);

    ~PdfFileInputStream();

protected:
    size_t ReadImpl(char* pBuffer, size_t lLen, bool& eof) override;

private:
    std::ifstream m_stream;
};

/** An input stream that reads data from a memory buffer
 */
class PODOFO_API PdfMemoryInputStream : public PdfInputStream
{
public:
    /** Open a file for reading data
     *  
     *  \param pBuffer buffer to read from
     *  \param lBufferLen length of the buffer
     */
    PdfMemoryInputStream( const char* pBuffer, size_t lBufferLen );
    ~PdfMemoryInputStream();

protected:
    size_t ReadImpl( char* pBuffer, size_t lLen, bool& eof) override;

private:
    const char* m_pBuffer;
    size_t m_lBufferLen;
};

/** An input stream that reads data from an input device
 */
class PODOFO_API PdfDeviceInputStream : public PdfInputStream
{
public:
    /** 
     *  Read from an alread opened input device
     * 
     *  \param pDevice an input device
     */
    PdfDeviceInputStream( PdfInputDevice* pDevice );

protected:
    size_t ReadImpl( char* pBuffer, size_t lLen, bool& eof) override;;

private:
    PdfInputDevice* m_pDevice;
};

};

#endif // PDF_INPUT_STREAM_H
