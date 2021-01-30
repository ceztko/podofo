/***************************************************************************
 *   Copyright (C) 2011 by Dominik Seichter                                *
 *   domseichter@web.de                                                    *
 *                      by Petr Pytelka                                    *
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

#ifndef _PDF_SIGNATURE_FIELD_H_
#define _PDF_SIGNATURE_FIELD_H_

#include "PdfAnnotation.h"
#include "PdfField.h"
#include "podofo/base/PdfDate.h"
#include "podofo/base/PdfData.h"

namespace PoDoFo {

enum class EPdfCertPermission
{
    NoPerms = 1,
    FormFill = 2,
    Annotations = 3,
};

struct PdfSignatureBeacons
{
    PdfSignatureBeacons();
    std::string ContentsBeacon;
    std::string ByteRangeBeacon;
    std::shared_ptr<size_t> ContentsOffset;
    std::shared_ptr<size_t> ByteRangeOffset;
};

class PODOFO_DOC_API PdfSignature : public PdfField
{
public:

    /** Create a new PdfSignature
     */
    PdfSignature( PdfPage* pPage, const PdfRect & rRect);

    /** Create a new PdfSignature
     *  \param bInit creates a signature field with/without a /V key
     */
    PdfSignature( PdfAnnotation* pWidget, PdfDocument &pDoc, bool insertInAcroform);

    /** Creates a PdfSignature from an existing PdfAnnotation, which should
     *  be an annotation with a field type Sig.
     *	\param pObject the object
     *	\param pWidget the annotation to create from
     */
    PdfSignature( PdfObject* pObject, PdfAnnotation* pWidget );

    /** Set an appearance stream for this signature field
     *  to specify its visual appearance
     *  \param pObject an XObject
     *  \param eAppearance an appearance type to set
     *  \param state the state for which set it the pObject; states depend on the annotation type
     */
    void SetAppearanceStream(PdfXObject *pObject, EPdfAnnotationAppearance eAppearance = EPdfAnnotationAppearance::Normal, const PdfName & state = "" );

    /** Create space for signature
     *
     * Structure of the PDF file - before signing:
     * <</ByteRange[ 0 1234567890 1234567890 1234567890]/Contents<signatureData>
     * Have to be replaiced with the following structure:
     * <</ByteRange[ 0 count pos count]/Contents<real signature ...0-padding>
     *
     * \param filter /Filter for this signature
     * \param subFilter /SubFilter for this signature
     * \param beacons Shared sentinels that will updated
     *                during writing of the document
     */
    void PrepareForSigning(const std::string_view& filter,
        const std::string_view& subFilter,
        const PdfSignatureBeacons& beacons);

    /** Set the signer name
    *
    *  \param rsText the signer name
    */
    void SetSignerName(const PdfString & rsText);

    /** Set reason of the signature
     *
     *  \param rsText the reason of signature
     */
    void SetSignatureReason(const PdfString & rsText);

    /** Set location of the signature
     *
     *  \param rsText the location of signature
     */
    void SetSignatureLocation(const PdfString & rsText);

    /** Set the creator of the signature
     *
     *  \param creator the creator of the signature
     */
    void SetSignatureCreator( const PdfName & creator );

    /** Date of signature
     */
    void SetSignatureDate(const PdfDate &sigDate);

    /** Add certification dictionaries and references to document catalog.
     *
     *  \param pDocumentCatalog the catalog of current document
     *  \param perm document modification permission
     */
    void AddCertificationReference(PdfObject *pDocumentCatalog, EPdfCertPermission perm = EPdfCertPermission::NoPerms);

    /** Get the signer name
    *
    *  \returns the found signer object
    */
    const PdfObject * GetSignerName() const;

    /** Get the reason of the signature
    *
    *  \returns the found reason object
    */
    const PdfObject * GetSignatureReason() const;

    /** Get the location of the signature
    *
    *  \returns the found location object
    */
    const PdfObject * GetSignatureLocation() const;

    /** Get the date of the signature
    *
    *  \returns the found date object
    */
    const PdfObject * GetSignatureDate() const;

    /** Returns signature object for this signature field.
     *  It can be nullptr, when the signature field was created
     *  from an existing annotation and it didn't have set it.
     *
     *  \returns associated signature object, or nullptr
     */
    PdfObject* GetSignatureObject( void ) const;

    /** Ensures that the signature field has set a signature object.
     *  The function does nothing, if the signature object is already
     *  set. This is useful for cases when the signature field had been
     *  created from an existing annotation, which didn't have it set.
     */
    void EnsureSignatureObject( void );

private:
    void Init(PdfAcroForm &acroForm);

private:
    PdfObject* m_pSignatureObj;
};

}

#endif
