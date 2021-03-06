/***************************************************************************
 *   Copyriht (C) 2006 by Dominik Seichter                                *
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

#include "PdfPagesTree.h"

#include "base/PdfDefinesPrivate.h"

#include <algorithm>
#include <sstream>

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"
#include "base/PdfOutputDevice.h"

#include "PdfPage.h"

#include <iostream>
namespace PoDoFo {

PdfPagesTree::PdfPagesTree( PdfVecObjects* pParent )
    : PdfElement(*pParent, "Pages"),
      m_cache( 0 )
{
    GetObject()->GetDictionary().AddKey( "Kids", PdfArray() );
    GetObject()->GetDictionary().AddKey( "Count", PdfObject( static_cast<int64_t>(0) ) );
}

PdfPagesTree::PdfPagesTree( PdfObject* pPagesRoot )
    : PdfElement(*pPagesRoot),
      m_cache( GetChildCount( pPagesRoot ) )
{
    if( !this->GetObject() ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }
}

PdfPagesTree::~PdfPagesTree() 
{
    m_cache.ClearCache();
}

int PdfPagesTree::GetTotalNumberOfPages() const
{
    return GetChildCount( GetObject() );
}

PdfPage* PdfPagesTree::GetPage( int nIndex )
{
    // if you try to get a page past the end, return nullptr
    // we use >= since nIndex is 0 based
    if ( nIndex >= GetTotalNumberOfPages() )
        return nullptr;

    // Take a look into the cache first
    PdfPage* pPage = m_cache.GetPage( nIndex );
    if( pPage )
        return pPage;

    // Not in cache -> search tree
    PdfObjectList lstParents;
    PdfObject* pObj = this->GetPageNode(nIndex, this->GetRoot(), lstParents);
    if( pObj ) 
    {
        pPage = new PdfPage( pObj, lstParents );
        m_cache.AddPageObject( nIndex, pPage );
        return pPage;
    }

    return nullptr;
}

PdfPage* PdfPagesTree::GetPage( const PdfReference & ref )
{
    // We have to search through all pages,
    // as this is the only way
    // to instantiate the PdfPage with a correct list of parents
    for( int i=0;i<this->GetTotalNumberOfPages();i++ ) 
    {
        PdfPage* pPage = this->GetPage( i );
        if( pPage && pPage->GetObject()->GetIndirectReference() == ref ) 
            return pPage;
    }
    
    return nullptr;
}


void PdfPagesTree::InsertPage( int nAfterPageIndex, PdfPage* inPage )
{
    this->InsertPage( nAfterPageIndex, inPage->GetObject() );
}

void PdfPagesTree::InsertPage( int nAfterPageIndex, PdfObject* pPage )
{
    bool bInsertBefore = false;

    if((int)EPdfPageInsertionPoint::InsertBeforeFirstPage == nAfterPageIndex )
    {
        bInsertBefore = true;
        nAfterPageIndex = 0;
    }
    else if( nAfterPageIndex < 0 ) 
    {
        // Only EPdfPageInsertionPoint::InsertBeforeFirstPage is valid here
        PdfError::LogMessage( ELogSeverity::Information,
                              "Invalid argument to PdfPagesTree::InsertPage: %i (Only EPdfPageInsertionPoint::InsertBeforeFirstPage is valid here).",
                              nAfterPageIndex );
        return;
    }

    //printf("Fetching page node: %i\n", nAfterPageIndex);
    PdfObjectList lstParents;
    PdfObject* pPageBefore = nullptr;
    //printf("Searching page=%i\n", nAfterPageIndex );
    if( this->GetTotalNumberOfPages() != 0 ) // no GetPageNode call w/o pages
    {
        pPageBefore = this->GetPageNode( nAfterPageIndex, this->GetRoot(),
                                        lstParents );
    }
    //printf("pPageBefore=%p lstParents=%i\n", pPageBefore,lstParents.size() );
    if( !pPageBefore || lstParents.size() == 0 ) 
    {
        if( this->GetTotalNumberOfPages() != 0 ) 
        {
            PdfError::LogMessage( ELogSeverity::Critical,
                                  "Cannot find page %i or page %i has no parents. Cannot insert new page.",
                                  nAfterPageIndex, nAfterPageIndex );
            return;
        }
        else
        {
            // We insert the first page into an empty pages tree
            PdfObjectList lstPagesTree;
            lstPagesTree.push_back( this->GetObject() );
            // Use -1 as index to insert before the empty kids array
            InsertPageIntoNode( this->GetObject(), lstPagesTree, -1, pPage );
        }
    }
    else
    {
        PdfObject* pParent = lstParents.back();
        //printf("bInsertBefore=%i\n", bInsertBefore );
        int nKidsIndex = bInsertBefore  ? -1 : this->GetPosInKids( pPageBefore, pParent );
        //printf("Inserting into node: %p at pos %i\n", pParent, nKidsIndex );

        InsertPageIntoNode( pParent, lstParents, nKidsIndex, pPage );
    }

    m_cache.InsertPage( (bInsertBefore && nAfterPageIndex == 0) ? (int)EPdfPageInsertionPoint::InsertBeforeFirstPage : nAfterPageIndex );
}

void PdfPagesTree::InsertPages( int nAfterPageIndex, const std::vector<PdfObject*>& vecPages )
{
    bool bInsertBefore = false;
    if((int)EPdfPageInsertionPoint::InsertBeforeFirstPage == nAfterPageIndex )
    {
        bInsertBefore = true;
        nAfterPageIndex = 0;
    }
    else if( nAfterPageIndex < 0 ) 
    {
        // Only EPdfPageInsertionPoint::InsertBeforeFirstPage is valid here
        PdfError::LogMessage( ELogSeverity::Information,
                              "Invalid argument to PdfPagesTree::InsertPage: %i (Only EPdfPageInsertionPoint::InsertBeforeFirstPage is valid here).",
                              nAfterPageIndex );
        return;
    }

    PdfObjectList lstParents;
    PdfObject* pPageBefore = nullptr;
    if( this->GetTotalNumberOfPages() != 0 ) // no GetPageNode call w/o pages
    {
        pPageBefore = this->GetPageNode( nAfterPageIndex, this->GetRoot(),
                                        lstParents );
    }
    if( !pPageBefore || lstParents.size() == 0 ) 
    {
        if( this->GetTotalNumberOfPages() != 0 ) 
        {
            PdfError::LogMessage( ELogSeverity::Critical,
                                  "Cannot find page %i or page %i has no parents. Cannot insert new page.",
                                  nAfterPageIndex, nAfterPageIndex );
            return;
        }
        else
        {
            // We insert the first page into an empty pages tree
            PdfObjectList lstPagesTree;
            lstPagesTree.push_back( this->GetObject() );
            // Use -1 as index to insert before the empty kids array
            InsertPagesIntoNode( this->GetObject(), lstPagesTree, -1, vecPages );
        }
    }
    else
    {
        PdfObject* pParent = lstParents.back();
        int nKidsIndex = bInsertBefore  ? -1 : this->GetPosInKids( pPageBefore, pParent );

        InsertPagesIntoNode( pParent, lstParents, nKidsIndex, vecPages );
    }

    m_cache.InsertPages( (bInsertBefore && nAfterPageIndex == 0) ? (int)EPdfPageInsertionPoint::InsertBeforeFirstPage : nAfterPageIndex,  (int)vecPages.size() );
}

PdfPage* PdfPagesTree::CreatePage( const PdfRect & rSize )
{
    PdfPage* pPage = new PdfPage( rSize, GetRoot()->GetDocument() );

    InsertPage( this->GetTotalNumberOfPages() - 1, pPage );
    m_cache.AddPageObject( this->GetTotalNumberOfPages(), pPage );
    
    return pPage;
}

PdfPage* PdfPagesTree::InsertPage( const PdfRect & rSize, int atIndex)
{
    PdfPage* pPage = new PdfPage( rSize, GetRoot()->GetDocument());

    int pageCount;
    if ( atIndex < 0 )
        atIndex = 0;
    else if ( atIndex > ( pageCount = this->GetTotalNumberOfPages() ) )
        atIndex = pageCount;

    InsertPage( atIndex - 1, pPage );
    m_cache.AddPageObject( atIndex, pPage );

    return pPage;
}

void PdfPagesTree::CreatePages( const std::vector<PdfRect>& vecSizes )
{
    std::vector<PdfPage*> vecPages;
    std::vector<PdfObject*> vecObjects;
    for (std::vector<PdfRect>::const_iterator it = vecSizes.begin(); it != vecSizes.end(); ++it)
    {
        PdfPage* pPage = new PdfPage( (*it), GetRoot()->GetDocument());
        vecPages.push_back( pPage );
        vecObjects.push_back( pPage->GetObject() );
    }

    InsertPages( this->GetTotalNumberOfPages() - 1, vecObjects );
    m_cache.AddPageObjects( this->GetTotalNumberOfPages(), vecPages );
}

void PdfPagesTree::DeletePage( int nPageNumber )
{
    // Delete from cache
    m_cache.DeletePage( nPageNumber );
    
    // Delete from pages tree
    PdfObjectList lstParents;
    PdfObject* pPageNode = this->GetPageNode( nPageNumber, this->GetRoot(), lstParents );

    if( !pPageNode ) 
    {
        PdfError::LogMessage( ELogSeverity::Information,
                              "Invalid argument to PdfPagesTree::DeletePage: %i - Page not found",
                              nPageNumber );
        PODOFO_RAISE_ERROR( EPdfError::PageNotFound );
    }

    if( lstParents.size() > 0 ) 
    {
        PdfObject* pParent = lstParents.back();
        int nKidsIndex = this->GetPosInKids( pPageNode, pParent );
        
        DeletePageFromNode( pParent, lstParents, nKidsIndex, pPageNode );
    }
    else
    {
        PdfError::LogMessage( ELogSeverity::Error,
                              "PdfPagesTree::DeletePage: Page %i has no parent - cannot be deleted.",
                              nPageNumber );
        PODOFO_RAISE_ERROR( EPdfError::PageNotFound );
    }
}

PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pParent, 
                                      PdfObjectList & rLstParents ) 
{
    if( !pParent ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    if( !pParent->GetDictionary().HasKey( PdfName("Kids") ) )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidKey );
    }

    
    const PdfObject* pObj = pParent->GetIndirectKey( "Kids" );
    if( pObj == nullptr || !pObj->IsArray() )
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidDataType );
    }

    const PdfArray & rKidsArray = pObj->GetArray(); 
    const size_t numKids = GetChildCount(pParent);

    // use <= since nPageNum is 0-based
    if( static_cast<int>(numKids) <= nPageNum ) 
    {
        PdfError::LogMessage( ELogSeverity::Critical,
	    "Cannot retrieve page %i from a document with only %i pages.",
                              nPageNum, static_cast<int>(numKids) );
        return nullptr;
    }

    //printf("Fetching: %i %i\n", numKids, nPageNum );

    // We have to traverse the tree
    //
    // BEWARE: There is no valid shortcut for tree traversal.
    // Even if eKidsArray.size()==numKids, this does not imply that
    // eKidsArray can be accessed with the index of the page directly.
    // The tree could have an arbitrary complex structure because
    // internal nodes with no leaves (page objects) are not forbidden
    // by the PDF spec.
    for (auto &child : rKidsArray)
    {
        if(!child.IsReference() )
        {
            PdfError::LogMessage( ELogSeverity::Critical, "Requesting page index %i. Invalid datatype in kids array: %s", 
                                  nPageNum, child.GetDataTypeString());
            return nullptr;
        }

        PdfObject* pChild = GetRoot()->GetDocument()->GetObjects().GetObject(child.GetReference());
        if (!pChild) 
        {
            PdfError::LogMessage( ELogSeverity::Critical, "Requesting page index %i. Child not found: %s", 
                                  nPageNum, child.GetReference().ToString().c_str());
            return nullptr;
        }

        if (this->IsTypePages(pChild))
        {
            int childCount = GetChildCount(pChild);
            if (childCount < nPageNum + 1) // Pages are 0 based, but count is not
            {
                // skip this page tree node
                // and go to the next child in rKidsArray
                nPageNum -= childCount;
            }
            else
            {
                // page is in the subtree of pChild
                // => call GetPageNode() recursively

                rLstParents.push_back(pParent);

                if (std::find(rLstParents.begin(), rLstParents.end(), pChild)
                    != rLstParents.end()) // cycle in parent list detected, fend
                { // off security vulnerability similar to CVE-2017-8054 (infinite recursion)
                    std::ostringstream oss;
                    oss << "Cycle in page tree: child in /Kids array of object "
                        << (*(rLstParents.rbegin()))->GetIndirectReference().ToString()
                        << " back-references to object " << pChild->GetIndirectReference()
                        .ToString() << " one of whose descendants the former is.";
                    PODOFO_RAISE_ERROR_INFO(EPdfError::PageNotFound, oss.str());
                }

                return this->GetPageNode(nPageNum, pChild, rLstParents);
            }
        }
        else if (this->IsTypePage(pChild))
        {
            if (0 == nPageNum)
            {
                // page found
                rLstParents.push_back(pParent);
                return pChild;
            }

            // Skip a normal page
            if (nPageNum > 0)
                nPageNum--;
        }
		else
		{
                    const PdfReference & rLogRef = pChild->GetIndirectReference();
                    uint32_t nLogObjNum = rLogRef.ObjectNumber();
                    uint16_t nLogGenNum = rLogRef.GenerationNumber();
		    PdfError::LogMessage( ELogSeverity::Critical,
                                          "Requesting page index %i. "
                        "Invalid datatype referenced in kids array: %s\n"
                        "Reference to invalid object: %i %i R", nPageNum,
                        pChild->GetDataTypeString(), nLogObjNum, nLogGenNum);
                    return nullptr;
                }
        }

    return nullptr;
}

bool PdfPagesTree::IsTypePage(const PdfObject* pObject) const 
{
    if( !pObject )
        return false;

    if( pObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
        return true;

    return false;
}

bool PdfPagesTree::IsTypePages(const PdfObject* pObject) const 
{
    if( !pObject )
        return false;

    if( pObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Pages" ) )
        return true;

    return false;
}

int PdfPagesTree::GetChildCount( const PdfObject* pNode ) const
{
    if( !pNode ) 
        return 0;

    const PdfObject *pCount = pNode->GetIndirectKey( "Count" );
    if (pCount != 0)
    {
        return (pCount->GetDataType() == PoDoFo::EPdfDataType::Number) ?
            static_cast<int>(pCount->GetNumber()) : 0;
    }
    else
    {
        return 0;
    }
}

int PdfPagesTree::GetPosInKids( PdfObject* pPageObj, PdfObject* pPageParent )
{
    if (pPageParent == nullptr)
        return -1;

    const PdfArray & rKids = pPageParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();

    int index = 0;
    for (auto& child : rKids)
    {
        if (child.GetReference() == pPageObj->GetIndirectReference())
            return index;

        index++;
    }

    return -1;
}

void PdfPagesTree::InsertPageIntoNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                                       int nIndex, PdfObject* pPage )
{
    if( !pParent || !pPage ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // 1. Add the reference of the new page to the kids array of pParent
    // 2. Increase count of every node in lstParents (which also includes pParent)
    // 3. Add Parent key to the page

    // 1. Add reference
    const PdfArray oldKids = pParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    PdfArray newKids;

    newKids.reserve( oldKids.GetSize() + 1 );

    if( nIndex < 0 ) 
    {
        newKids.push_back( pPage->GetIndirectReference() );
    }

    int i = 0;
    for (auto& child : oldKids)
    {
        newKids.push_back(child);

        if (i == nIndex)
            newKids.push_back(pPage->GetIndirectReference());

        i++;
    }

    /*
    PdfVariant var2( newKids );
    std::string str2;
    var2.ToString(str2);
    printf("newKids= %s\n", str2.c_str() );
    */

    pParent->GetDictionary().AddKey( PdfName("Kids"), newKids );
 
    // 2. increase count
    PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        this->ChangePagesCount( *itParents, 1 );

        ++itParents;
    } 

    // 3. add parent key to the page
    pPage->GetDictionary().AddKey( PdfName("Parent"), pParent->GetIndirectReference() );
}

void PdfPagesTree::InsertPagesIntoNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                                       int nIndex, const std::vector<PdfObject*>& vecPages )
{
    if( !pParent || !vecPages.size() ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // 1. Add the reference of the new page to the kids array of pParent
    // 2. Increase count of every node in lstParents (which also includes pParent)
    // 3. Add Parent key to the page

    // 1. Add reference
    const PdfArray oldKids = pParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    PdfArray newKids;
    newKids.reserve( oldKids.GetSize() + vecPages.size() );

    bool bIsPushedIn = false;
    int i=0;
    for (auto &oldKid : oldKids)
    {
        if ( !bIsPushedIn && (nIndex < i) )    // Pushing before
        {
            for (std::vector<PdfObject*>::const_iterator itPages=vecPages.begin(); itPages!=vecPages.end(); ++itPages)
            {
                newKids.push_back( (*itPages)->GetIndirectReference() );    // Push all new kids at once
            }
            bIsPushedIn = true;
        }
        newKids.push_back(oldKid);    // Push in the old kids
        ++i;
    }

    // If new kids are still not pushed in then they may be appending to the end
    if ( !bIsPushedIn && ( (nIndex + 1) == static_cast<int>(oldKids.size())) ) 
    {
        for (std::vector<PdfObject*>::const_iterator itPages=vecPages.begin(); itPages!=vecPages.end(); ++itPages)
        {
            newKids.push_back( (*itPages)->GetIndirectReference() );    // Push all new kids at once
        }
        bIsPushedIn = true;
    }

    pParent->GetDictionary().AddKey( PdfName("Kids"), newKids );
 

    // 2. increase count
    for ( PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin(); itParents != rlstParents.rend(); ++itParents )
    {
        this->ChangePagesCount( *itParents, (int)vecPages.size() );
    } 

    // 3. add parent key to each of the pages
    for (std::vector<PdfObject*>::const_iterator itPages=vecPages.begin(); itPages!=vecPages.end(); ++itPages)
    {
        (*itPages)->GetDictionary().AddKey( PdfName("Parent"), pParent->GetIndirectReference() );
    }
}

void PdfPagesTree::DeletePageFromNode( PdfObject* pParent, const PdfObjectList & rlstParents, 
                                       int nIndex, PdfObject* pPage )
{
    if( !pParent || !pPage ) 
    {
        PODOFO_RAISE_ERROR( EPdfError::InvalidHandle );
    }

    // 1. Delete the reference from the kids array of pParent
    // 2. Decrease count of every node in lstParents (which also includes pParent)
    // 3. Remove empty page nodes

    // TODO: Tell cache to free page object

    // 1. Delete reference
    this->DeletePageNode( pParent, nIndex ) ;

    // 2. Decrease count
    PdfObjectList::const_reverse_iterator itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        this->ChangePagesCount( *itParents, -1 );

        ++itParents;
    } 

    // 3. Remove empty pages nodes
    itParents = rlstParents.rbegin();
    while( itParents != rlstParents.rend() )
    {
        // Never delete root node
        if( IsEmptyPageNode( *itParents ) && *itParents != GetRoot() ) 
        {
            PdfObject* pParentOfNode = *(itParents + 1);
            int nKidsIndex = this->GetPosInKids( *itParents, pParentOfNode );
            DeletePageNode( pParentOfNode, nKidsIndex );

            // Delete empty page nodes
            this->GetObject()->GetDocument()->GetObjects().RemoveObject( (*itParents)->GetIndirectReference() );
        }

        ++itParents;
    } 
}

void PdfPagesTree::DeletePageNode( PdfObject* pParent, int nIndex ) 
{
    PdfArray kids = pParent->GetDictionary().GetKey( PdfName("Kids") )->GetArray();
    kids.erase( kids.begin() + nIndex );
    pParent->GetDictionary().AddKey( PdfName("Kids"), kids );
}

int PdfPagesTree::ChangePagesCount( PdfObject* pPageObj, int nDelta )
{
    // Increment or decrement inPagesDict's Count by inDelta, and return the new count.
    // Simply return the current count if inDelta is 0.
    int	cnt = GetChildCount( pPageObj );
    if( 0 != nDelta ) 
    {
        cnt += nDelta ;
        pPageObj->GetDictionary().AddKey( "Count", PdfVariant( static_cast<int64_t>(cnt) ) );
    }

    return cnt ;
}

bool PdfPagesTree::IsEmptyPageNode( PdfObject* pPageNode ) 
{
    long lCount = GetChildCount( pPageNode );
    bool bKidsEmpty = true;

    if( pPageNode->GetDictionary().HasKey( PdfName("Kids") ) )
    {
        bKidsEmpty = pPageNode->GetDictionary().GetKey( PdfName("Kids") )->GetArray().empty();
    }

    return ( lCount == 0L || bKidsEmpty );
}

/*
PdfObject* PdfPagesTree::GetPageNode( int nPageNum, PdfObject* pPagesObject, 
                                      std::deque<PdfObject*> & rListOfParents )
{
    // recurse through the pages tree nodes
    PdfObject* pObj            = nullptr;

    if( !pPagesObject->GetDictionary().HasKey( "Kids" ) )
        return nullptr;

    pObj = pPagesObject->GetDictionary().GetKey( "Kids" );
    if( !pObj->IsArray() )
        return nullptr;

    PdfArray&	kidsArray = pObj->GetArray();
    size_t	numKids   = kidsArray.size();
    size_t      kidsCount = GetChildCount( pPagesObject );

    // All parents of the page node will be added to this lists,
    // so that the PdfPage can later access inherited attributes
    rListOfParents.push_back( pPagesObject );

    // the pages tree node represented by pPagesObject has only page nodes in its kids array,
    // or pages nodes with a kid count of 1, so we can speed things up
    // by going straight to the desired node
    if ( numKids == kidsCount )
    {
        if( nPageNum >= static_cast<int>(kidsArray.size()) )
        {
            PdfError::LogMessage( ELogSeverity::Critical, "Requesting page index %i from array of size %i", nPageNum, kidsArray.size() );
            nPageNum--;
        }

        PdfVariant pgVar = kidsArray[ nPageNum ];
        while ( true ) 
        {
            if ( pgVar.IsArray() ) 
            {
                // Fixes some broken PDFs who have trees with 1 element kids arrays
                return GetPageNodeFromTree( nPageNum, pgVar.GetArray(), rListOfParents );
            }
            else if ( !pgVar.IsReference() )
                return nullptr;	// can't handle inline pages just yet...

            PdfObject* pgObject = GetRoot()->GetOwner()->GetObject( pgVar.GetReference() );
            // make sure the object is a /Page and not a /Pages with a single kid
            if ( pgObject->GetDictionary().GetKeyAsName( PdfName( "Type" ) ) == PdfName( "Page" ) )
                return pgObject;

            // it's a /Pages with a single kid, so dereference and try again...
            if( !pgObject->GetDictionary().HasKey( "Kids" ) )
                return nullptr;

            rListOfParents.push_back( pgObject );
            pgVar = *(pgObject->GetDictionary().GetKey( "Kids" ));
        }
    } 
    else 
    {
        return GetPageNodeFromTree( nPageNum, kidsArray, rListOfParents );
    }

    // we should never exit from here - we should always have been able to return a page from above
    // PODOFO_ASSERT( false ) ;
    return nullptr;
}
*/

};
