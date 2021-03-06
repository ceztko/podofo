/***************************************************************************
 *   Copyright (C) 2006 by Dominik Seichter                                *
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

#include "PdfOutlines.h"

#include "base/PdfDefinesPrivate.h"

#include <doc/PdfDocument.h>
#include "base/PdfArray.h"
#include "base/PdfDictionary.h"
#include "base/PdfObject.h"

#include "PdfAction.h"
#include "PdfDestination.h"

namespace PoDoFo {

PdfOutlineItem::PdfOutlineItem( const PdfString & sTitle, const PdfDestination & rDest, 
                                PdfOutlineItem* pParentOutline, PdfVecObjects* pParent )
    : PdfElement(*pParent),
      m_pParentOutline( pParentOutline ), m_pPrev( nullptr ), m_pNext( nullptr ), 
      m_pFirst( nullptr ), m_pLast( nullptr ), m_pDestination( nullptr ), m_pAction( nullptr )
{
    if( pParentOutline )
        this->GetObject()->GetDictionary().AddKey( "Parent", pParentOutline->GetObject()->GetIndirectReference() );

    this->SetTitle( sTitle );
    this->SetDestination( rDest );
}

PdfOutlineItem::PdfOutlineItem( const PdfString & sTitle, const PdfAction & rAction, 
                                PdfOutlineItem* pParentOutline, PdfVecObjects* pParent )
    : PdfElement(*pParent),
      m_pParentOutline( pParentOutline ), m_pPrev( nullptr ), m_pNext( nullptr ), 
      m_pFirst( nullptr ), m_pLast( nullptr ), m_pDestination( nullptr ), m_pAction( nullptr )
{
    if( pParentOutline )
        this->GetObject()->GetDictionary().AddKey( "Parent", pParentOutline->GetObject()->GetIndirectReference() );

    this->SetTitle( sTitle );
    this->SetAction( rAction );
}

PdfOutlineItem::PdfOutlineItem( PdfObject* pObject, PdfOutlineItem* pParentOutline, PdfOutlineItem* pPrevious )
    : PdfElement(*pObject), m_pParentOutline( pParentOutline ), m_pPrev( pPrevious ), 
      m_pNext( nullptr ), m_pFirst( nullptr ), m_pLast( nullptr ), m_pDestination( nullptr ), m_pAction( nullptr )
{
    PdfReference first, next;

    if( this->GetObject()->GetDictionary().HasKey( "First" ) )
    {
        first    = this->GetObject()->GetDictionary().GetKey("First")->GetReference();
        m_pFirst = new PdfOutlineItem( pObject->GetDocument()->GetObjects().GetObject( first ), this, nullptr );
    }

    if( this->GetObject()->GetDictionary().HasKey( "Next" ) )
    {
        next     = this->GetObject()->GetDictionary().GetKey("Next")->GetReference();
        PdfObject* pObj = pObject->GetDocument()->GetObjects().GetObject( next );

        m_pNext  = new PdfOutlineItem( pObj, pParentOutline, this );
    }
    else
    {
        // if there is no next key,
        // we have to set ourself as the last item of the parent
        if( m_pParentOutline )
            m_pParentOutline->SetLast( this );
    }
}

PdfOutlineItem::PdfOutlineItem( PdfVecObjects* pParent )
    : PdfElement(*pParent, "Outlines"), m_pParentOutline( nullptr ), m_pPrev( nullptr ),
      m_pNext( nullptr ), m_pFirst( nullptr ), m_pLast( nullptr ), m_pDestination( nullptr ), m_pAction( nullptr )
{
}

PdfOutlineItem::~PdfOutlineItem()
{
    delete m_pNext;
    delete m_pFirst;
}

PdfOutlineItem* PdfOutlineItem::CreateChild( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, this, &this->GetObject()->GetDocument()->GetObjects());

    this->InsertChildInternal( pItem, false );

    return pItem;
}

void PdfOutlineItem::InsertChild( PdfOutlineItem* pItem )
{
    this->InsertChildInternal( pItem, true );
}

void PdfOutlineItem::InsertChildInternal( PdfOutlineItem* pItem, bool bCheckParent )
{
    PdfOutlineItem* pItemToCheckParent = pItem;
    PdfOutlineItem* pRoot = nullptr;
    PdfOutlineItem* pRootOfThis = nullptr;

    if ( !pItemToCheckParent )
        return;

    if( bCheckParent )
    {
        while( pItemToCheckParent )
        {
            while( pItemToCheckParent->GetParentOutline() )
                pItemToCheckParent = pItemToCheckParent->GetParentOutline();

            if( pItemToCheckParent == pItem ) // item can't have a parent
            {
                pRoot = pItem; // needed later, "root" can mean "standalone" here
                break;         // for performance in standalone or doc-merge case
            }

            if( !pRoot )
            {
                pRoot = pItemToCheckParent;
                pItemToCheckParent = this;
            }
            else
            {
                pRootOfThis = pItemToCheckParent;
                pItemToCheckParent = nullptr;
            }
        }

        if( pRoot == pRootOfThis ) // later nullptr if check skipped for performance
            PODOFO_RAISE_ERROR( EPdfError::OutlineItemAlreadyPresent );
    }

    if( m_pLast )
    {
        m_pLast->SetNext( pItem );
        pItem->SetPrevious( m_pLast );
    }

    m_pLast = pItem;

    if( !m_pFirst )
        m_pFirst = m_pLast;

    this->GetObject()->GetDictionary().AddKey( "First", m_pFirst->GetObject()->GetIndirectReference() );
    this->GetObject()->GetDictionary().AddKey( "Last",  m_pLast->GetObject()->GetIndirectReference() );
}

PdfOutlineItem* PdfOutlineItem::CreateNext ( const PdfString & sTitle, const PdfDestination & rDest )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rDest, m_pParentOutline, &this->GetObject()->GetDocument()->GetObjects());

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( pItem );
        pItem->SetNext( m_pNext );
    }

    m_pNext = pItem;
    m_pNext->SetPrevious( this );

    this->GetObject()->GetDictionary().AddKey( "Next", m_pNext->GetObject()->GetIndirectReference() );

    if( m_pParentOutline && !m_pNext->Next() ) 
        m_pParentOutline->SetLast( m_pNext );

    return m_pNext;
}

PdfOutlineItem* PdfOutlineItem::CreateNext ( const PdfString & sTitle, const PdfAction & rAction )
{
    PdfOutlineItem* pItem = new PdfOutlineItem( sTitle, rAction, m_pParentOutline, &this->GetObject()->GetDocument()->GetObjects());

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( pItem );
        pItem->SetNext( m_pNext );
    }

    m_pNext = pItem;
    m_pNext->SetPrevious( this );

    this->GetObject()->GetDictionary().AddKey( "Next", m_pNext->GetObject()->GetIndirectReference() );

    if( m_pParentOutline && !m_pNext->Next() ) 
        m_pParentOutline->SetLast( m_pNext );

    return m_pNext;
}

void PdfOutlineItem::SetPrevious( PdfOutlineItem* pItem )
{
    m_pPrev = pItem;
    if( m_pPrev )
        this->GetObject()->GetDictionary().AddKey( "Prev", m_pPrev->GetObject()->GetIndirectReference() );
    else
        this->GetObject()->GetDictionary().RemoveKey( "Prev" );
}

void PdfOutlineItem::SetNext( PdfOutlineItem* pItem )
{
    m_pNext = pItem;
    if( m_pNext )
        this->GetObject()->GetDictionary().AddKey( "Next", m_pNext->GetObject()->GetIndirectReference() );
    else
        this->GetObject()->GetDictionary().RemoveKey( "Next" );
}

void PdfOutlineItem::SetLast( PdfOutlineItem* pItem )
{
    m_pLast = pItem;
    if( m_pLast )
        this->GetObject()->GetDictionary().AddKey( "Last",  m_pLast->GetObject()->GetIndirectReference() );
    else 
        this->GetObject()->GetDictionary().RemoveKey( "Last" );
}

void PdfOutlineItem::SetFirst( PdfOutlineItem* pItem )
{
    m_pFirst = pItem;
    if( m_pFirst )
        this->GetObject()->GetDictionary().AddKey( "First",  m_pFirst->GetObject()->GetIndirectReference() );
    else 
        this->GetObject()->GetDictionary().RemoveKey( "First" );
}

void PdfOutlineItem::Erase()
{
    while( m_pFirst )
    {
        // erase will set a new first
        // if it has a next item
        m_pFirst->Erase();
    }

    if( m_pPrev ) 
    {
        m_pPrev->SetNext( m_pNext );
    }

    if( m_pNext ) 
    {
        m_pNext->SetPrevious( m_pPrev );
    }

    if( !m_pPrev && m_pParentOutline && this == m_pParentOutline->First() )
        m_pParentOutline->SetFirst( m_pNext );

    if( !m_pNext && m_pParentOutline && this == m_pParentOutline->Last() )
        m_pParentOutline->SetLast( m_pPrev );

    m_pNext = nullptr;
    delete this;
}

void PdfOutlineItem::SetDestination( const PdfDestination & rDest )
{
    delete m_pDestination;
    m_pDestination = nullptr;

    rDest.AddToDictionary( this->GetObject()->GetDictionary() );
}

PdfDestination* PdfOutlineItem::GetDestination( PdfDocument* pDoc )
{
    if( !m_pDestination )
    {
        PdfObject*	dObj = this->GetObject()->GetIndirectKey( "Dest" );
        if ( !dObj ) 
            return nullptr;
    
        m_pDestination = new PdfDestination( dObj, pDoc );
    }

    return m_pDestination;
}

void PdfOutlineItem::SetAction( const PdfAction & rAction )
{
    delete m_pAction;
    m_pAction = nullptr;

    rAction.AddToDictionary( this->GetObject()->GetDictionary() );
}

PdfAction* PdfOutlineItem::GetAction( void )
{
    if( !m_pAction )
    {
        PdfObject*	dObj = this->GetObject()->GetIndirectKey( "A" );
        if ( !dObj ) 
            return nullptr;
    
        m_pAction = new PdfAction( dObj );
    }

    return m_pAction;
}

void PdfOutlineItem::SetTitle( const PdfString & sTitle )
{
    this->GetObject()->GetDictionary().AddKey( "Title", sTitle );
}

const PdfString & PdfOutlineItem::GetTitle() const
{
    return this->GetObject()->GetIndirectKey( "Title" )->GetString();
}

void PdfOutlineItem::SetTextFormat( EPdfOutlineFormat eFormat )
{
    this->GetObject()->GetDictionary().AddKey( "F", static_cast<int64_t>(eFormat) );
}

EPdfOutlineFormat PdfOutlineItem::GetTextFormat() const
{
    if( this->GetObject()->GetDictionary().HasKey( "F" ) )
        return static_cast<EPdfOutlineFormat>(this->GetObject()->GetIndirectKey( "F" )->GetNumber());

    return EPdfOutlineFormat::Default;
}

void PdfOutlineItem::SetTextColor( double r, double g, double b )
{
    PdfArray color;
    color.push_back( r );
    color.push_back( g );
    color.push_back( b );

    this->GetObject()->GetDictionary().AddKey( "C", color );
}


double PdfOutlineItem::GetTextColorRed() const
{
    if( this->GetObject()->GetDictionary().HasKey( "C" ) )
        return this->GetObject()->GetIndirectKey( "C" )->GetArray()[0].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorGreen() const
{
    if( this->GetObject()->GetDictionary().HasKey( "C" ) )
        return this->GetObject()->GetIndirectKey( "C" )->GetArray()[1].GetReal();

    return 0.0;
}

double PdfOutlineItem::GetTextColorBlue() const
{
    if( this->GetObject()->GetDictionary().HasKey( "C" ) )
        return this->GetObject()->GetIndirectKey( "C" )->GetArray()[2].GetReal();

    return 0.0;
}

PdfOutlines::PdfOutlines( PdfVecObjects* pParent )
    : PdfOutlineItem( pParent )
{
}

PdfOutlines::PdfOutlines( PdfObject* pObject )
    : PdfOutlineItem( pObject, nullptr, nullptr )
{
}

PdfOutlineItem* PdfOutlines::CreateRoot( const PdfString & sTitle )
{
    return this->CreateChild( sTitle, PdfDestination(&GetObject()->GetDocument()->GetObjects()));
}

};
