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

#ifndef _PDF_DATE_H_
#define _PDF_DATE_H_

#include <chrono>

#include "PdfDefines.h"
#include <podofo/compat/optional>
#include "PdfString.h"

namespace PoDoFo {

/** This class is a date datatype as specified in the PDF 
 *  reference. You can easily convert from Unix time_t to
 *  the PDF time representation and back. Dates like these
 *  are used for example in the PDF info dictionary for the
 *  creation time and date of the PDF file.
 *
 *  PdfDate objects are immutable.
 *
 *  From the PDF reference:
 *
 *  PDF defines a standard date format, which closely follows 
 *  that of the international standard ASN.1 (Abstract Syntax
 *  Notation One), defined in ISO/IEC 8824 (see the Bibliography). 
 *  A date is a string of the form
 *  (D:YYYYMMDDHHmmSSOHH'mm')
 */
class PODOFO_API PdfDate
{
public:
    /** Create a PdfDate object with the current date and time.
     */
    PdfDate();

    /** Create a PdfDate with a specified date and time
     *  \param t the date and time of this object
     *
     *  \see IsValid()
     */
    PdfDate(const std::chrono::seconds &secondsFromEpoch, const std::optional<std::chrono::minutes> &offsetFromUTC);

    /** Create a PdfDate with a specified date and time
     *  \param szDate the date and time of this object 
     *         in PDF format. It has to be a string of 
     *         the format  (D:YYYYMMDDHHmmSSOHH'mm').
     */
    PdfDate( const PdfString & sDate );

    /** \returns the date and time of this PdfDate in 
     *  seconds since epoch.
     */
    const std::chrono::seconds & GetSecondsFromEpoch() const { return m_secondsFromEpoch; }

    const std::optional<std::chrono::minutes> & GetMinutesFromUtc() const { return m_minutesFromUtc; }

    /** The value returned by this function can be used in any PdfObject
     *  where a date is needed
     */
    PdfString ToString() const;

    /** The value returned is a W3C compliant date representation
     */
    PdfString ToStringW3C() const;

private:
    /** Creates the internal string representation from
     *  a time_t value and writes it to m_szDate.
     */
    PdfString createStringRepresentation(bool w3cstring) const;

    /** Parse fixed length number from string
     *  \param in string to read number from
     *  \param length of number to read 
     *  \param min minimal value of number
     *  \param max maximal value of number
     *  \param ret parsed number
     */
    bool ParseFixLenNumber(const char *&in, unsigned int length, int min, int max, int &ret);

private:
    std::chrono::seconds m_secondsFromEpoch;
    std::optional<std::chrono::minutes> m_minutesFromUtc;
};

};

#endif
