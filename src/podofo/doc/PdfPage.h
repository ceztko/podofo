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

#ifndef _PDF_PAGE_H_
#define _PDF_PAGE_H_

#include <map>

#include "podofo/base/PdfDefines.h"

#include "podofo/base/PdfArray.h"
#include "podofo/base/PdfCanvas.h"
#include "podofo/base/PdfRect.h"

#include "PdfAnnotation.h"
#include "PdfContents.h"
#include "PdfElement.h"
#include "PdfField.h"

namespace PoDoFo {

class PdfDocument;
class PdfDictionary;
class PdfVecObjects;
class PdfInputStream;

typedef std::map<PdfObject *,PdfAnnotation*>  TMapAnnotationDirect;
typedef TMapAnnotationDirect::iterator        TIMapAnnotation;
typedef TMapAnnotationDirect::const_iterator  TCIMapAnnotationDirect;

/** PdfPage is one page in the pdf document. 
 *  It is possible to draw on a page using a PdfPainter object.
 *  Every document needs at least one page.
 */
class PODOFO_DOC_API PdfPage : public PdfElement, public PdfCanvas {
 public:
    /** Create a new PdfPage object.
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent add the page to this parent
     */
    PdfPage( const PdfRect & rSize, PdfDocument* pParent );

    /** Create a new PdfPage object.
     *  \param rSize a PdfRect specifying the size of the page (i.e the /MediaBox key) in PDF units
     *  \param pParent add the page to this parent
     */
    PdfPage( const PdfRect & rSize, PdfVecObjects* pParent );
 
    /** Create a PdfPage based on an existing PdfObject
     *  \param pObject an existing PdfObject
     *  \param listOfParents a list of PdfObjects that are
     *                       parents of this page and can be 
     *                       queried for inherited attributes.
     *                       The last object in the list is the
     *                       most direct parent of this page.
     */
    PdfPage( PdfObject* pObject, const std::deque<PdfObject*> & listOfParents );

    virtual ~PdfPage();

    /** Get the current page size in PDF Units
     *  \returns a PdfRect containing the page size available for drawing
     */
    PdfRect GetRect() const override;

    /** Get the current page rotation
     * \param teta counterclockwise rotation in radians
     * \returns true if the page has a rotation
     */
    bool HasRotation(double& teta) const override;

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page width in PDF Units
     *
     * \returns true if successfull, false otherwise
     *
     */
    bool SetPageWidth(int newWidth);

    // added by Petr P. Petrov 21 Febrary 2010
    /** Set the current page height in PDF Units
     *
     * \returns true if successfull, false otherwise
     *
     */
    bool SetPageHeight(int newHeight);

    /** Set the mediabox in PDF Units
    *  \param rSize a PdfRect specifying the mediabox of the page (i.e the /TrimBox key) in PDF units
    */
    void SetMediaBox(const PdfRect & rSize);

    /** Set the trimbox in PDF Units
     *  \param rSize a PdfRect specifying the trimbox of the page (i.e the /TrimBox key) in PDF units
     */
	void SetTrimBox( const PdfRect & rSize );

	/** Page number inside of the document. The  first page
     *  has the number 1, the last page has the number 
     *  PdfPagesTree:GetTotalNumberOfPages()
     *
     *  \returns the number of the page inside of the document
     *
     *  \see PdfPagesTree:GetTotalNumberOfPages()
     */
    size_t GetPageNumber() const;

    /** Creates a PdfRect with the page size as values which is needed to create a PdfPage object
     *  from an enum which are defined for a few standard page sizes.
     *
     *  \param ePageSize the page size you want
     *  \param bLandscape create a landscape pagesize instead of portrait (by exchanging width and height)
     *  \returns a PdfRect object which can be passed to the PdfPage constructor
     */
    static PdfRect CreateStandardPageSize( const EPdfPageSize ePageSize, bool bLandscape = false );

    /** Get access to the contents object of this page.
     *  If you want to draw onto the page, you have to add 
     *  drawing commands to the stream of the Contents object.
     *  \returns a contents object
     */
    PdfObject* GetContents() const override;

    /** Get access to the resources object of this page.
     *  This is most likely an internal object.
     *  \returns a resources object
     */
    inline PdfObject* GetResources() const override;

    /** Get the current MediaBox (physical page size) in PDF units.
     *  \returns PdfRect the page box
     */
    const PdfRect GetMediaBox() const { return GetPageBox( "MediaBox" ); }

    /** Get the current CropBox (visible page size) in PDF units.
     *  \returns PdfRect the page box
     */
    const PdfRect GetCropBox() const { return GetPageBox( "CropBox" ); }

    /** Get the current TrimBox (cut area) in PDF units.
     *  \returns PdfRect the page box
     */
    const PdfRect GetTrimBox() const { return GetPageBox( "TrimBox" ); }

    /** Get the current BleedBox (extra area for printing purposes) in PDF units.
     *  \returns PdfRect the page box
     */
    const PdfRect GetBleedBox() const { return GetPageBox( "BleedBox" ); }

    /** Get the current ArtBox in PDF units.
     *  \returns PdfRect the page box
     */
    const PdfRect GetArtBox() const { return GetPageBox( "ArtBox" ); }

    /** Get the current page rotation (if any), it's a clockwise rotation
     *  \returns int 0, 90, 180 or 270
     */
    int GetRotationRaw() const;

    /** Set the current page rotation.
     *  \param iRotation Rotation to set to the page. Valid value are 0, 90, 180, 270.
     */
    void SetRotationRaw(int nRotation);
        
    /** Get the number of annotations associated with this page
     * \ returns int number of annotations
     */
    size_t GetAnnotationCount() const;

    /** Create a new annotation to this page.
     *  \param eType the type of the annotation
     *  \param rRect rectangle of the annotation on the page
     *
     *  \returns the annotation object which is owned by the PdfPage
     */
    PdfAnnotation* CreateAnnotation( EPdfAnnotation eType, const PdfRect & rRect );

    /** Get the annotation with index index of the current page.
     *  \param index the index of the annotation to retrieve
     *
     *  \returns a annotation object. The annotation object is owned by the PdfPage.
     *
     *  \see GetAnnotationCount
     */
    PdfAnnotation* GetAnnotation(size_t index);

    /** Delete the annotation with index index from this page.
     *  \param index the index of the annotation to delete
     *
     *  \see GetAnnotationCount
     */
    void DeleteAnnotation(size_t index);

    /** Delete the annotation with the given object
     *  \param annotObj the object of an annotation
     *
     *  \see GetAnnotationCount
     */
    void DeleteAnnotation( PdfObject &annotObj );

    /** Get an element from the pages resources dictionary,
     *  using a type (category) and a key.
     *
     *  \param rType the type of resource to fetch (e.g. /Font, or /XObject)
     *  \param rKey the key of the resource
     *
     *  \returns the object of the resource or nullptr if it was not found
     */
    PdfObject* GetFromResources( const PdfName & rType, const PdfName & rKey );

    /** Method for getting a value that can be inherited
     *  Possible names that can be inherited according to 
     *  the PDF specification are: Resources, MediaBox, CropBox and Rotate
     *  
     *  \returns PdfObject - the result of the key fetching or nullptr
     */
    inline const PdfObject* GetInheritedKey( const PdfName & rName ) const; 

    /** Set an ICC profile for this page
     *
     *  \param pszCSTag a ColorSpace tag
     *  \param pStream an input stream from which the ICC profiles data can be read
     *  \param nColorComponents the number of colorcomponents of the ICC profile (expected is 1, 3 or 4 components)
     *  \param eAlternateColorSpace an alternate colorspace to use if the ICC profile cannot be used
     *
     *  \see PdfPainter::SetDependICCProfileColor()
     */
    void SetICCProfile( const char* pszCSTag, PdfInputStream* pStream, int64_t nColorComponents,
                                EPdfColorSpace eAlternateColorSpace = EPdfColorSpace::DeviceRGB );
 private:
     PdfStream & GetStreamForAppending(EPdfStreamAppendFlags flags) override;

    /**
     * Initialize a new page object.
     * m_pContents must be initialized before calling this!
     *
     * @param rSize page size
     */
    void InitNewPage( const PdfRect & rSize );

    /**
     * Create the internal PdfContents object.
     * Call this before accessing m_pContents as
     * the object is only created if needed.
     */
    void EnsureContentsCreated() const;

    /** Get the bounds of a specified page box in PDF units.
     * This function is internal, since there are wrappers for all standard boxes
     *  \returns PdfRect the page box
     */
    const PdfRect GetPageBox( const char* inBox ) const;
    
    /** Method for getting a key value that could be inherited (such as the boxes, resources, etc.)
     *  \returns PdfObject - the result of the key fetching or nullptr
     */
    const PdfObject* GetInheritedKeyFromObject( const char* inKey, const PdfObject* inObject, int depth = 0 ) const;

private:
    PdfArray * GetAnnotationsArray() const;
    PdfArray & GetOrCreateAnnotationsArray();

 private:
    PdfContents*   m_pContents;
    PdfObject*     m_pResources;

    TMapAnnotationDirect m_mapAnnotations;
};

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline PdfObject* PdfPage::GetResources() const
{
    return m_pResources;
}

// -----------------------------------------------------
// 
// -----------------------------------------------------
inline const PdfObject* PdfPage::GetInheritedKey( const PdfName & rName ) const
{
    return this->GetInheritedKeyFromObject( rName.GetString().c_str(), this->GetObject() );
}

};

#endif // _PDF_PAGE_H_
