/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include <algorithm>
#include <deque>
#include <iostream>


#include "PdfDocument.h"

#include "base/PdfDefinesPrivate.h"

#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfImmediateWriter.h"
#include "base/PdfObject.h"
#include "base/PdfStream.h"
#include "base/PdfVecObjects.h"

#include "PdfAcroForm.h"
#include "PdfDestination.h"
#include "PdfFileSpec.h"
#include "PdfFont.h"
#include "PdfFontMetrics.h"
#include "PdfInfo.h"
#include "PdfNamesTree.h"
#include "PdfOutlines.h"
#include "PdfPage.h"
#include "PdfPagesTree.h"
#include "PdfXObject.h"


using namespace std;

using namespace PoDoFo;

PdfDocument::PdfDocument(bool bEmpty) :
    m_vecObjects(*this),
    m_pCatalog(nullptr),
    m_fontCache( &m_vecObjects )
{
    if( !bEmpty ) 
    {
        m_pTrailer.reset(new PdfObject()); // The trailer is NO part of the vector of objects
        m_pTrailer->SetDocument( *this );
        m_pCatalog = m_vecObjects.CreateObject("Catalog");
        
        m_pInfo.reset(new PdfInfo( &m_vecObjects ));
        
        m_pTrailer->GetDictionary().AddKey( "Root", m_pCatalog->GetIndirectReference() );
        m_pTrailer->GetDictionary().AddKey( "Info", m_pInfo->GetObject()->GetIndirectReference() );

        InitPagesTree();
    }
}

PdfDocument::~PdfDocument()
{
    this->Clear();
}

void PdfDocument::Clear() 
{
    m_fontCache.EmptyCache();
    m_vecObjects.Clear();
    m_pCatalog = nullptr;
}

void PdfDocument::InitPagesTree()
{
    PdfObject* pagesRootObj = m_pCatalog->GetIndirectKey( PdfName( "Pages" ) );
    if ( pagesRootObj ) 
    {
        m_pPagesTree.reset(new PdfPagesTree(pagesRootObj));
    }
    else
    {
        m_pPagesTree.reset(new PdfPagesTree(&m_vecObjects));
        m_pCatalog->GetDictionary().AddKey( "Pages", m_pPagesTree->GetObject()->GetIndirectReference() );
    }
}

PdfObject* PdfDocument::GetNamedObjectFromCatalog( const char* pszName ) const 
{
    return m_pCatalog->GetIndirectKey( PdfName( pszName ) );
}

int PdfDocument::GetPageCount() const
{
    return m_pPagesTree->GetTotalNumberOfPages();
}

PdfPage* PdfDocument::GetPage( int nIndex ) const
{
    if( nIndex < 0 || nIndex >= m_pPagesTree->GetTotalNumberOfPages() )
    {
        PODOFO_RAISE_ERROR( EPdfError::PageNotFound );
    }

    return m_pPagesTree->GetPage( nIndex );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName,
                                  bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, 
                                  EFontCreationFlags eFontCreationFlags, 
                                  bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, false, false, bSymbolCharset, bEmbedd, eFontCreationFlags, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, 
                                  EFontCreationFlags eFontCreationFlags,
                                  bool bEmbedd, const char* pszFileName )
{
    return m_fontCache.GetFont( pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, eFontCreationFlags, pEncoding, pszFileName );
}

#if defined(WIN32)
PdfFont* PdfDocument::CreateFont( const wchar_t* pszFontName, bool bSymbolCharset, const PdfEncoding * const pEncoding, 
                                  bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, false, false, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                  const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( pszFontName, bBold, bItalic, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const LOGFONTA &logFont, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( logFont, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateFont( const LOGFONTW &logFont, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( logFont, bEmbedd, pEncoding );
}
#endif // WIN32

PdfFont* PdfDocument::CreateFontSubset( const char* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                        const PdfEncoding * const pEncoding, const char* pszFileName )
{
    return m_fontCache.GetFontSubset( pszFontName, bBold, bItalic, bSymbolCharset, pEncoding, pszFileName );
}

#ifdef WIN32
PdfFont* PdfDocument::CreateFontSubset( const wchar_t* pszFontName, bool bBold, bool bItalic, bool bSymbolCharset,
                                        const PdfEncoding * const pEncoding)
{

    (void)pszFontName;
    (void)bBold;
    (void)bItalic;
    (void)bSymbolCharset;
    (void)pEncoding;
    PODOFO_RAISE_ERROR_INFO( EPdfError::Unknown, "Subsets are not yet implemented for unicode on windows." );
}
#endif // WIN32

PdfFont* PdfDocument::CreateFont( FT_Face face, bool bSymbolCharset, const PdfEncoding * const pEncoding, bool bEmbedd )
{
    return m_fontCache.GetFont( face, bSymbolCharset, bEmbedd, pEncoding );
}

PdfFont* PdfDocument::CreateDuplicateFontType1( PdfFont * pFont, const char * pszSuffix )
{
	return m_fontCache.GetDuplicateFontType1( pFont, pszSuffix );
}

PdfPage* PdfDocument::CreatePage( const PdfRect & rSize )
{
    return m_pPagesTree->CreatePage( rSize );
}

void PdfDocument::CreatePages( const std::vector<PdfRect>& vecSizes )
{
    m_pPagesTree->CreatePages( vecSizes );
}

PdfPage* PdfDocument::InsertPage( const PdfRect & rSize, int atIndex)
{
	return m_pPagesTree->InsertPage( rSize, atIndex );
}

void PdfDocument::EmbedSubsetFonts()
{
	m_fontCache.EmbedSubsetFonts();
}

const PdfDocument & PdfDocument::Append( const PdfDocument & rDoc, bool bAppendAll )
{
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Ulrich Arnold 30.7.2009: Because GetNextObject uses m_nObjectCount instead 
    //                          of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    //                          make sure the free objects are already present before appending to
	//                          prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = rDoc.GetObjects().GetFreeObjects().begin();
    while( itFree != rDoc.GetObjects().GetFreeObjects().end() )
    {
        m_vecObjects.AddFreeObject( PdfReference( (*itFree).ObjectNumber() + difference, (*itFree).GenerationNumber() ) );

        ++itFree;
    }

	// append all objects first and fix their references
    TCIVecObjects it = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        PdfReference reference(static_cast<uint32_t>((*it)->GetIndirectReference().ObjectNumber() + difference), (*it)->GetIndirectReference().GenerationNumber());
        PdfObject* pObj = new PdfObject();
        pObj->SetIndirectReference(reference);
        m_vecObjects.AddObject(pObj);
        *pObj = **it;

        PdfError::LogMessage( ELogSeverity::Information,
                              "Fixing references in %i %i R by %i", pObj->GetIndirectReference().ObjectNumber(), pObj->GetIndirectReference().GenerationNumber(), difference );
        FixObjectReferences( pObj, difference );

        ++it;
    }

    if( bAppendAll )
    {
        const PdfName inheritableAttributes[] = {
            PdfName("Resources"),
            PdfName("MediaBox"),
            PdfName("CropBox"),
            PdfName("Rotate"),
            PdfName::KeyNull
        };

        // append all pages now to our page tree
        for(int i=0;i<rDoc.GetPageCount();i++ )
        {
            PdfPage *pPage = rDoc.GetPage( i );
            if (pPage == nullptr)
            {
                std::ostringstream oss;
                oss << "No page " << i << " (the first is 0) found.";
                PODOFO_RAISE_ERROR_INFO( EPdfError::PageNotFound, oss.str() );
            }
            PdfObject*    pObj  = m_vecObjects.GetObject( PdfReference( pPage->GetObject()->GetIndirectReference().ObjectNumber() + difference, pPage->GetObject()->GetIndirectReference().GenerationNumber() ) );
            if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
                pObj->GetDictionary().RemoveKey( "Parent" );

            // Deal with inherited attributes
            const PdfName* pInherited = inheritableAttributes;
            while( pInherited->GetLength() != 0 ) 
            {
                const PdfObject* pAttribute = pPage->GetInheritedKey( *pInherited ); 
                if( pAttribute != nullptr )
                {
                    PdfObject attribute( *pAttribute );
                    FixObjectReferences( &attribute, difference );
                    pObj->GetDictionary().AddKey( *pInherited, attribute );
                }

                ++pInherited;
            }

            m_pPagesTree->InsertPage( this->GetPageCount()-1, pObj );
        }
        
        // append all outlines
        PdfOutlineItem* pRoot       = this->GetOutlines();
        PdfOutlines*    pAppendRoot = const_cast<PdfDocument&>(rDoc).GetOutlines( PoDoFo::ePdfDontCreateObject );
        if( pAppendRoot && pAppendRoot->First() ) 
        {
            // only append outlines if appended document has outlines
            while( pRoot && pRoot->Next() ) 
                pRoot = pRoot->Next();
            
            PdfReference ref( pAppendRoot->First()->GetObject()->GetIndirectReference().ObjectNumber() + difference, pAppendRoot->First()->GetObject()->GetIndirectReference().GenerationNumber() );
            pRoot->InsertChild( new PdfOutlines( m_vecObjects.GetObject( ref ) ) );
        }
    }
    
    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

const PdfDocument &PdfDocument::InsertExistingPageAt( const PdfDocument & rDoc, int nPageIndex, int nAtIndex)
{
	/* copy of PdfDocument::Append, only restricts which page to add */
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());


    // Ulrich Arnold 30.7.2009: Because GetNextObject uses m_nObjectCount instead 
    //                          of m_vecObjects.GetSize()+m_vecObjects.GetFreeObjects().size()+1
    //                          make sure the free objects are already present before appending to
	//                          prevent overlapping obj-numbers

    // create all free objects again, to have a clean free object list
    TCIPdfReferenceList itFree = rDoc.GetObjects().GetFreeObjects().begin();
    while( itFree != rDoc.GetObjects().GetFreeObjects().end() )
    {
        m_vecObjects.AddFreeObject( PdfReference( (*itFree).ObjectNumber() + difference, (*itFree).GenerationNumber() ) );

        ++itFree;
    }

	// append all objects first and fix their references
    TCIVecObjects it = rDoc.GetObjects().begin();
    while( it != rDoc.GetObjects().end() )
    {
        PdfReference reference(static_cast<uint32_t>((*it)->GetIndirectReference().ObjectNumber() + difference), (*it)->GetIndirectReference().GenerationNumber());

        PdfObject* pObj = new PdfObject();
        pObj->SetIndirectReference(reference);
        m_vecObjects.AddObject(pObj);
        *pObj = **it;

        PdfError::LogMessage( ELogSeverity::Information,
                              "Fixing references in %i %i R by %i", pObj->GetIndirectReference().ObjectNumber(), pObj->GetIndirectReference().GenerationNumber(), difference );
        FixObjectReferences( pObj, difference );

        ++it;
    }

    const PdfName inheritableAttributes[] = {
        PdfName("Resources"),
        PdfName("MediaBox"),
        PdfName("CropBox"),
        PdfName("Rotate"),
        PdfName::KeyNull
    };

    // append all pages now to our page tree
    for(int i=0;i<rDoc.GetPageCount();i++ )
    {
        if (i != nPageIndex) {
            continue;
        }

        PdfPage*      pPage = rDoc.GetPage( i );
        PdfObject*    pObj  = m_vecObjects.GetObject( PdfReference( pPage->GetObject()->GetIndirectReference().ObjectNumber() + difference, pPage->GetObject()->GetIndirectReference().GenerationNumber() ) );
        if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Parent" ) )
            pObj->GetDictionary().RemoveKey( "Parent" );

        // Deal with inherited attributes
        const PdfName* pInherited = inheritableAttributes;
        while( pInherited->GetLength() != 0 ) 
        {
	    const PdfObject* pAttribute = pPage->GetInheritedKey( *pInherited ); 
	    if( pAttribute )
	    {
	        PdfObject attribute( *pAttribute );
	        FixObjectReferences( &attribute, difference );
	        pObj->GetDictionary().AddKey( *pInherited, attribute );
	    }

	    ++pInherited;
        }

        m_pPagesTree->InsertPage( nAtIndex <= 0 ? (int)EPdfPageInsertionPoint::InsertBeforeFirstPage : nAtIndex - 1, pObj );
    }

    // append all outlines
    PdfOutlineItem* pRoot       = this->GetOutlines();
    PdfOutlines*    pAppendRoot = const_cast<PdfDocument&>(rDoc).GetOutlines( PoDoFo::ePdfDontCreateObject );
    if( pAppendRoot && pAppendRoot->First() ) 
    {
        // only append outlines if appended document has outlines
        while( pRoot && pRoot->Next() ) 
	    pRoot = pRoot->Next();
    
        PdfReference ref( pAppendRoot->First()->GetObject()->GetIndirectReference().ObjectNumber() + difference, pAppendRoot->First()->GetObject()->GetIndirectReference().GenerationNumber() );
        pRoot->InsertChild( new PdfOutlines( m_vecObjects.GetObject( ref ) ) );
    }
    
    // TODO: merge name trees
    // ToDictionary -> then iteratate over all keys and add them to the new one
    return *this;
}

PdfRect PdfDocument::FillXObjectFromDocumentPage( PdfXObject * pXObj, const PdfDocument & rDoc, int nPage, bool bUseTrimBox )
{
    unsigned int difference = static_cast<unsigned int>(m_vecObjects.GetSize() + m_vecObjects.GetFreeObjects().size());
    Append( rDoc, false );
    PdfPage* pPage = rDoc.GetPage( nPage );

    return FillXObjectFromPage( pXObj, pPage, bUseTrimBox, difference );
}

PdfRect PdfDocument::FillXObjectFromExistingPage( PdfXObject * pXObj, int nPage, bool bUseTrimBox )
{
    PdfPage* pPage = GetPage( nPage );

    return FillXObjectFromPage( pXObj, pPage, bUseTrimBox, 0 );
}

PdfRect PdfDocument::FillXObjectFromPage( PdfXObject * pXObj, const PdfPage * pPage, bool bUseTrimBox, unsigned int difference )
{
    // TODO: remove unused objects: page, ...

    PdfObject*    pObj  = m_vecObjects.GetObject( PdfReference( pPage->GetObject()->GetIndirectReference().ObjectNumber() + difference, pPage->GetObject()->GetIndirectReference().GenerationNumber() ) );
    PdfRect       box  = pPage->GetMediaBox();

    // intersect with crop-box
    box.Intersect( pPage->GetCropBox() );

    // intersect with trim-box according to parameter
    if ( bUseTrimBox )
        box.Intersect( pPage->GetTrimBox() );

    // link resources from external doc to x-object
    if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Resources" ) )
        pXObj->GetObject()->GetDictionary().AddKey( "Resources" , pObj->GetDictionary().GetKey( "Resources" ) );

    // copy top-level content from external doc to x-object
    if( pObj->IsDictionary() && pObj->GetDictionary().HasKey( "Contents" ) )
    {
        // get direct pointer to contents
        PdfObject* pContents;
        if( pObj->GetDictionary().GetKey( "Contents" )->IsReference() )
            pContents = m_vecObjects.GetObject( pObj->GetDictionary().GetKey( "Contents" )->GetReference() );
        else
            pContents = pObj->GetDictionary().GetKey( "Contents" );

        if( pContents->IsArray() )
        {
            // copy array as one stream to xobject
            PdfArray pArray = pContents->GetArray();

            PdfObject*  pObj = pXObj->GetObject();
            PdfStream &pObjStream = pObj->GetOrCreateStream();

            TVecFilters vFilters;
            vFilters.push_back( EPdfFilter::FlateDecode );
            pObjStream.BeginAppend( vFilters );

            TIVariantList it;
            for(it = pArray.begin(); it != pArray.end(); it++)
            {
                if ( it->IsReference() )
                {
                    // TODO: not very efficient !!
                    PdfObject*  pObj = GetObjects().GetObject( it->GetReference() );

                    while (pObj != nullptr)
                    {
                        if (pObj->IsReference())    // Recursively look for the stream
                        {
                            pObj = GetObjects().GetObject( pObj->GetReference() );
                        }
                        else if (pObj->HasStream())
                        {
                            PdfStream &pcontStream = pObj->GetOrCreateStream();

                            unique_ptr<char> pcontStreamBuffer;
                            size_t pcontStreamLength;
                            pcontStream.GetFilteredCopy(pcontStreamBuffer, pcontStreamLength);
                            pObjStream.Append(pcontStreamBuffer.get(), pcontStreamLength);
                            break;
                        }
                        else
                        {
                            PODOFO_RAISE_ERROR( EPdfError::InvalidStream );
                            break;
                        }
                    }
                }
                else
                {
                    string str;
                    it->ToString( str );
                    pObjStream.Append( str );
                    pObjStream.Append( " " );
                }
            }
            pObjStream.EndAppend();
        }
        else if( pContents->HasStream() )
        {
            // copy stream to xobject
            PdfObject* pObj = pXObj->GetObject();
            PdfStream &pObjStream = pObj->GetOrCreateStream();
            PdfStream &pcontStream = pContents->GetOrCreateStream();

            TVecFilters vFilters;
            vFilters.push_back( EPdfFilter::FlateDecode );
            pObjStream.BeginAppend( vFilters );
            unique_ptr<char> pcontStreamBuffer;
            size_t pcontStreamLength;
            pcontStream.GetFilteredCopy(pcontStreamBuffer, pcontStreamLength);
            pObjStream.Append(pcontStreamBuffer.get(), pcontStreamLength);
            pObjStream.EndAppend();
        }
        else
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
    }

    return box;
}

void PdfDocument::FixObjectReferences( PdfObject* pObject, int difference )
{
    if( !pObject ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( pObject->IsDictionary() )
    {
        TKeyMap::iterator it = pObject->GetDictionary().begin();

        while( it != pObject->GetDictionary().end() )
        {
            if( it->second.IsReference() )
            {
                it->second = PdfObject(PdfReference( it->second.GetReference().ObjectNumber() + difference,
                                              it->second.GetReference().GenerationNumber() ));
            }
            else if( it->second.IsDictionary() || 
                     it->second.IsArray() )
            {
                FixObjectReferences( &it->second, difference );
            }

            ++it;
        }
    }
    else if( pObject->IsArray() )
    {
        PdfArray::iterator it = pObject->GetArray().begin();

        while( it != pObject->GetArray().end() )
        {
            if( (*it).IsReference() )
            {
                *it = PdfObject(PdfReference( (*it).GetReference().ObjectNumber() + difference,
                                      (*it).GetReference().GenerationNumber() ));

            }
            else if( (*it).IsDictionary() || 
                     (*it).IsArray() )
                FixObjectReferences( &(*it), difference );

            ++it;
        }
    }
    else if( pObject->IsReference() )
    {
        *pObject = PdfObject(PdfReference(pObject->GetReference().ObjectNumber() + difference,
                pObject->GetReference().GenerationNumber() ));
    }
}

EPdfPageMode PdfDocument::GetPageMode( void ) const
{
    // PageMode is optional; the default value is UseNone
    EPdfPageMode thePageMode = EPdfPageMode::UseNone;
    
    PdfObject* pageModeObj = m_pCatalog->GetIndirectKey( PdfName( "PageMode" ) );
    if ( pageModeObj != nullptr ) {
        PdfName pmName = pageModeObj->GetName();
        
        if( PdfName( "UseNone" ) == pmName )
            thePageMode = EPdfPageMode::UseNone ;
        else if( PdfName( "UseThumbs" ) == pmName )
            thePageMode = EPdfPageMode::UseThumbs ;
        else if( PdfName( "UseOutlines" ) == pmName )
            thePageMode = EPdfPageMode::UseBookmarks ;
        else if( PdfName( "FullScreen" ) == pmName )
            thePageMode = EPdfPageMode::FullScreen ;
        else if( PdfName( "UseOC" ) == pmName )
            thePageMode = EPdfPageMode::UseOC ;
        else if( PdfName( "UseAttachments" ) == pmName )
            thePageMode = EPdfPageMode::UseAttachments ;
        else
            PODOFO_RAISE_ERROR( EPdfError::InvalidName );
    }
    
    return thePageMode ;
}

void PdfDocument::SetPageMode( EPdfPageMode inMode )
{
    switch ( inMode ) {
        default:
        case EPdfPageMode::DontCare:	
            // m_pCatalog->RemoveKey( PdfName( "PageMode" ) );
            // this value means leave it alone!
            break;
            
        case EPdfPageMode::UseNone:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseNone" ) );
            break;
            
        case EPdfPageMode::UseThumbs:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseThumbs" ) );
            break;
            
        case EPdfPageMode::UseBookmarks:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseOutlines" ) );
            break;
            
        case EPdfPageMode::FullScreen:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "FullScreen" ) );
            break;
            
        case EPdfPageMode::UseOC:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseOC" ) );
            break;
            
        case EPdfPageMode::UseAttachments:
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageMode" ), PdfName( "UseAttachments" ) );
            break;
    }
}

void PdfDocument::SetUseFullScreen( void )
{
    // first, we get the current mode
    EPdfPageMode	curMode = GetPageMode();
    
    // if current mode is anything but "don't care", we need to move that to non-full-screen
    if ( curMode != EPdfPageMode::DontCare )
        SetViewerPreference( PdfName( "NonFullScreenPageMode" ), PdfObject( *(m_pCatalog->GetIndirectKey( PdfName( "PageMode" ) )) ) );
    
    SetPageMode( EPdfPageMode::FullScreen );
}

void PdfDocument::SetViewerPreference( const PdfName& whichPref, const PdfObject & valueObj )
{
    PdfObject* prefsObj = m_pCatalog->GetIndirectKey( PdfName( "ViewerPreferences" ) );
    if ( prefsObj == nullptr )
    {
        // make me a new one and add it
        PdfDictionary	vpDict;
        vpDict.AddKey( whichPref, valueObj );
        
        m_pCatalog->GetDictionary().AddKey( PdfName( "ViewerPreferences" ), PdfObject( vpDict ) );
    }
    else
    {
        // modify the existing one
        prefsObj->GetDictionary().AddKey( whichPref, valueObj );
    }
}

void PdfDocument::SetViewerPreference( const PdfName& whichPref, bool inValue )
{
    SetViewerPreference( whichPref, PdfObject( inValue ) );
}

void PdfDocument::SetHideToolbar( void )
{
    SetViewerPreference( PdfName( "HideToolbar" ), true );
}

void PdfDocument::SetHideMenubar( void )
{
    SetViewerPreference( PdfName( "HideMenubar" ), true );
}

void PdfDocument::SetHideWindowUI( void )
{
    SetViewerPreference( PdfName( "HideWindowUI" ), true );
}

void PdfDocument::SetFitWindow( void )
{
    SetViewerPreference( PdfName( "FitWindow" ), true );
}

void PdfDocument::SetCenterWindow( void )
{
    SetViewerPreference( PdfName( "CenterWindow" ), true );
}

void PdfDocument::SetDisplayDocTitle( void )
{
    SetViewerPreference( PdfName( "DisplayDocTitle" ), true );
}

void PdfDocument::SetPrintScaling( PdfName& inScalingType )
{
    SetViewerPreference( PdfName( "PrintScaling" ), inScalingType );
}

void PdfDocument::SetBaseURI( const std::string& inBaseURI )
{
    PdfDictionary	uriDict;
    uriDict.AddKey( PdfName( "Base" ), new PdfObject( PdfString( inBaseURI ) ) );
    m_pCatalog->GetDictionary().AddKey( PdfName( "URI" ), new PdfObject( uriDict ) );
}

void PdfDocument::SetLanguage( const std::string& inLanguage )
{
    m_pCatalog->GetDictionary().AddKey( PdfName( "Lang" ), new PdfObject( PdfString( inLanguage ) ) );
}

void PdfDocument::SetBindingDirection( PdfName& inDirection )
{
    SetViewerPreference( PdfName( "Direction" ), inDirection );
}

void PdfDocument::SetPageLayout( EPdfPageLayout inLayout )
{
    switch ( inLayout ) {
        default:
        case EPdfPageLayout::Ignore:
            break;	// means do nothing
        case EPdfPageLayout::Default:			
            m_pCatalog->GetDictionary().RemoveKey( PdfName( "PageLayout" ) );
            break;
        case EPdfPageLayout::SinglePage:		
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "SinglePage" ) );
            break;
        case EPdfPageLayout::OneColumn:		
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "OneColumn" ) );
            break;
        case EPdfPageLayout::TwoColumnLeft:	
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoColumnLeft" ) );
            break;
        case EPdfPageLayout::TwoColumnRight: 	
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoColumnRight" ) );
            break;
        case EPdfPageLayout::TwoPageLeft: 	
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoPageLeft" ) );
            break;
        case EPdfPageLayout::TwoPageRight: 	
            m_pCatalog->GetDictionary().AddKey( PdfName( "PageLayout" ), PdfName( "TwoPageRight" ) );
            break;
    }
}

PdfOutlines* PdfDocument::GetOutlines( bool bCreate )
{
    PdfObject* pObj;

    if( !m_pOutlines )
    {
        pObj = GetNamedObjectFromCatalog( "Outlines" );
        if( !pObj ) 
        {
            if ( !bCreate )
                return nullptr;
            
            m_pOutlines.reset(new PdfOutlines( &m_vecObjects ));
            m_pCatalog->GetDictionary().AddKey( "Outlines", m_pOutlines->GetObject()->GetIndirectReference() );
        }
        else if ( pObj->GetDataType() != EPdfDataType::Dictionary )
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }
        else
            m_pOutlines.reset(new PdfOutlines(pObj));
    }        
    
    return m_pOutlines.get();
}

PdfNamesTree* PdfDocument::GetNamesTree( bool bCreate )
{
    PdfObject* pObj;

    if( !m_pNamesTree )
    {
        pObj = GetNamedObjectFromCatalog( "Names" );
        if( !pObj ) 
        {
            if ( !bCreate )
                return nullptr;

            PdfNamesTree tmpTree ( &m_vecObjects );
            pObj = tmpTree.GetObject();
            m_pCatalog->GetDictionary().AddKey( "Names", pObj->GetIndirectReference() );
            m_pNamesTree.reset(new PdfNamesTree( pObj, m_pCatalog));
        }
        else if ( pObj->GetDataType() != EPdfDataType::Dictionary )
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }
        else
            m_pNamesTree.reset(new PdfNamesTree(pObj, m_pCatalog));
    }        
    
    return m_pNamesTree.get();
}

PdfAcroForm* PdfDocument::GetAcroForm( bool bCreate, EPdfAcroFormDefaulAppearance eDefaultAppearance )
{
    PdfObject* pObj;

    if( !m_pAcroForms )
    {
        pObj = GetNamedObjectFromCatalog( "AcroForm" );
        if( !pObj ) 
        {
            if ( !bCreate )
                return nullptr;
            
            m_pAcroForms.reset(new PdfAcroForm(this, eDefaultAppearance));
            m_pCatalog->GetDictionary().AddKey( "AcroForm", m_pAcroForms->GetObject()->GetIndirectReference() );
        }
        else if ( pObj->GetDataType() != EPdfDataType::Dictionary )
        {
            PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
        }
        else
            m_pAcroForms.reset(new PdfAcroForm(this, pObj, eDefaultAppearance));
    }        
    
    return m_pAcroForms.get();
}

void PdfDocument::AddNamedDestination( const PdfDestination& rDest, const PdfString & rName )
{
    PdfNamesTree* nameTree = GetNamesTree();
    nameTree->AddValue( PdfName("Dests"), rName, rDest.GetObject()->GetIndirectReference() );
}

void PdfDocument::AttachFile( const PdfFileSpec & rFileSpec )
{
    PdfNamesTree* pNames = this->GetNamesTree( true );

    if(pNames == nullptr)
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );

    pNames->AddValue( "EmbeddedFiles", rFileSpec.GetFilename(false), rFileSpec.GetObject()->GetIndirectReference() );
}
    
PdfFileSpec* PdfDocument::GetAttachment( const PdfString & rName )
{
    PdfNamesTree* pNames = this->GetNamesTree();
    
    if(pNames == nullptr)
        return nullptr;
    
    PdfObject* pObj = pNames->GetValue( "EmbeddedFiles", rName );
    
    if(pObj == nullptr)
        return nullptr;
    
    return new PdfFileSpec(pObj);
}

void PdfDocument::SetInfo(std::unique_ptr<PdfInfo>& pInfo)
{
    if (pInfo == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pInfo = std::move(pInfo);
}

void PdfDocument::SetTrailer(std::unique_ptr<PdfObject>&  pObject )
{
    if (pObject == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    m_pTrailer = std::move(pObject);
    // Set owner so that GetIndirectKey will work
    m_pTrailer->SetDocument( *this );
}

FT_Library PdfDocument::GetFontLibrary() const
{
    return this->m_fontCache.GetFontLibrary();
}

#ifdef PODOFO_HAVE_FONTCONFIG

void PdfDocument::SetFontConfigWrapper(PdfFontConfigWrapper* pFontConfig)
{
    m_fontCache.SetFontConfigWrapper(pFontConfig);
}

#endif // PODOFO_HAVE_FONTCONFIG


PdfObject& PdfDocument::GetCatalog()
{
    if (m_pCatalog == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pCatalog;
}

const PdfObject& PdfDocument::GetCatalog() const
{
    if (m_pCatalog == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pCatalog;
}

PdfPagesTree& PdfDocument::GetPagesTree() const
{
    if (m_pPagesTree == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pPagesTree;
}

PdfObject& PdfDocument::GetTrailer()
{
    if (m_pTrailer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pTrailer;
}

const PdfObject& PdfDocument::GetTrailer() const
{
    if (m_pTrailer == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pTrailer;
}

PdfInfo& PdfDocument::GetInfo() const
{
    if (m_pInfo == nullptr)
        PODOFO_RAISE_ERROR(EPdfError::NoObject);

    return *m_pInfo;
}
