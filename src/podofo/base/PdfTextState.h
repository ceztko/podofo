/***************************************************************************
 *   Copyright (C) 2005 by Dominik Seichter                                *
 *   Copyright (C) 2018-2020 by Francesco Pretto                           *
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
#ifndef PDF_TEXT_STATE_H
#define PDF_TEXT_STATE_H

#include "podofo/base/PdfDefines.h"

namespace PoDoFo
{
    class PODOFO_DOC_API PdfTextState
    {
    public:
        PdfTextState();

    public:
        /** Set the font size of this metrics object for width and height
         *  calculations.
         *  This is typically called from PdfFont for you.
         *
         *  \param fSize font size in points
         */
        inline void SetFontSize(double fSize) { m_FontSize = fSize; }

        /** Retrieve the current font size of this metrics object
         *  \returns the current font size
         */
        inline double GetFontSize() const { return m_FontSize; }

        /** Set the horizontal scaling of the font for compressing (< 100) and expanding (>100)
         *  This is typically called from PdfFont for you.
         *
         *  \param fScale scaling in percent
         */
        inline void SetFontScale(double fScale) { m_FontScale = fScale; }

        /** Retrieve the current horizontal scaling of this metrics object
         *  \returns the current font scaling
         */
        inline double GetFontScale() const { return m_FontScale; }

        /** Set the character spacing of this metrics object
         *  \param fCharSpace character spacing in percent
         */
        inline void SetCharSpace(double fCharSpace) { m_CharSpace = fCharSpace; }

        /** Retrieve the current character spacing of this metrics object
         *  \returns the current font character spacing
         */
        inline double GetCharSpace() const { return m_CharSpace; }

        /** Set the word spacing of this metrics object
         *  \param fWordSpace word spacing in PDF units
         */
        inline void SetWordSpace(double fWordSpace) { m_WordSpace = fWordSpace; }

        /** Retrieve the current word spacing of this metrics object
         *  \returns the current font word spacing in PDF units
         */
        inline double GetWordSpace() const { return m_WordSpace; }

    private:
        double m_FontSize;
        double m_FontScale;
        double m_CharSpace;
        double m_WordSpace;
    };
}

#endif // PDF_TEXT_STATE_H
