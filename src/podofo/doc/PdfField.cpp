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

#include "PdfField.h"

#include "base/PdfDefinesPrivate.h"

#include "PdfAcroForm.h"
#include "PdfDocument.h"
#include "PdfPainter.h"
#include "PdfPage.h"
#include "PdfStreamedDocument.h"
#include "PdfXObject.h"
#include "PdfSignature.h"
#include "PdfPushButton.h"
#include "PdfRadioButton.h"
#include "PdfCheckBox.h"
#include "PdfTextBox.h"
#include "PdfComboBox.h"
#include "PdfListBox.h"

#include <sstream>

using namespace std;
using namespace PoDoFo;

void getFullName(const PdfObject* obj, bool escapePartialNames, string &fullname);

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect)
    : m_eField( eField )    
{
    m_pWidget = pPage->CreateAnnotation( EPdfAnnotation::Widget, rRect );
    m_pObject = m_pWidget->GetObject();
    Init(pPage->GetDocument().GetAcroForm());
}

PdfField::PdfField( EPdfField eField, PdfAnnotation* pWidget, PdfDocument &pDoc, bool insertInAcroform)
    : m_eField(eField), m_pObject(pWidget ? pWidget->GetObject() : nullptr), m_pWidget( pWidget )
{
    auto parent = pDoc.GetAcroForm();
    if (m_pObject == nullptr)
        m_pObject = parent->GetDocument()->GetObjects().CreateObject();

    Init(insertInAcroform ? parent : nullptr);
}

PdfField::PdfField( EPdfField eField, PdfPage* pPage, const PdfRect & rRect, PdfDocument* pDoc, bool bAppearanceNone)
    :  m_eField( eField )
{
   m_pWidget = pPage->CreateAnnotation( EPdfAnnotation::Widget, rRect );
   m_pObject = m_pWidget->GetObject();

   Init(pDoc->GetAcroForm(true,
			  bAppearanceNone ? 
			  EPdfAcroFormDefaulAppearance::None
			  : EPdfAcroFormDefaulAppearance::BlackText12pt ));
}

PdfField::PdfField( EPdfField eField, PdfObject *pObject, PdfAnnotation *pWidget )
    : m_eField( eField ), m_pObject( pObject ), m_pWidget( pWidget )
{
}

PdfField * PdfField::CreateField( PdfObject *pObject )
{
    return createField(GetFieldType(*pObject), pObject, nullptr );
}

PdfField * PdfField::CreateField( PdfAnnotation *pWidget )
{
    PdfObject *pObject = pWidget->GetObject();
    return createField(GetFieldType(*pObject), pObject, pWidget);
}

PdfField * PdfField::CreateChildField()
{
    return createChildField(nullptr, PdfRect());
}

PdfField * PdfField::CreateChildField(PdfPage &page, const PdfRect &rect)
{
    return createChildField(&page, rect);
}

PdfField * PdfField::createChildField(PdfPage *page, const PdfRect &rect)
{
    EPdfField type = GetType();
    auto doc = m_pObject->GetDocument();
    PdfField *field;
    PdfObject *childObj;
    if (page == nullptr)
    {
        childObj = doc->GetObjects().CreateObject();
        field = createField(type, childObj, nullptr);
    }
    else
    {
        PdfAnnotation *annot = page->CreateAnnotation(EPdfAnnotation::Widget, rect);
        childObj = annot->GetObject();
        field = createField(type, childObj, annot);
    }

    auto &dict = m_pObject->GetDictionary();
    auto kids = dict.FindKey("Kids");
    if (kids == nullptr)
        kids = &dict.AddKey("Kids", PdfArray());

    auto &arr = kids->GetArray();
    arr.push_back(childObj->GetIndirectReference());
    childObj->GetDictionary().AddKey("Parent", m_pObject->GetIndirectReference());
    return field;
}

PdfField * PdfField::createField(EPdfField type, PdfObject *pObject, PdfAnnotation *pWidget )
{
    switch ( type )
    {
    case EPdfField::Unknown:
        return new PdfField( pObject, pWidget );
    case EPdfField::PushButton:
        return new PdfPushButton( pObject, pWidget );
    case EPdfField::CheckBox:
        return new PdfCheckBox( pObject, pWidget );
    case EPdfField::RadioButton:
        return new PdfRadioButton( pObject, pWidget );
    case EPdfField::TextField:
        return new PdfTextBox( pObject, pWidget );
    case EPdfField::ComboBox:
        return new PdfComboBox( pObject, pWidget );
    case EPdfField::ListBox:
        return new PdfListBox( pObject, pWidget );
    case EPdfField::Signature:
        return new PdfSignature( pObject, pWidget );
    default:
        PODOFO_RAISE_ERROR(EPdfError::InvalidEnumValue);
    }
}

EPdfField PdfField::GetFieldType(const PdfObject & rObject)
{
    EPdfField eField = EPdfField::Unknown;

    // ISO 32000:2008, Section 12.7.3.1, Table 220, Page #432.
    const PdfObject *pFT = rObject.GetDictionary().FindKeyParent("FT");
    if (!pFT)
        return EPdfField::Unknown;

    const PdfName fieldType = pFT->GetName();
    if (fieldType == PdfName("Btn"))
    {
        int64_t flags;
        PdfField::GetFieldFlags( rObject , flags );

        if ( ( flags & PdfButton::ePdfButton_PushButton ) == PdfButton::ePdfButton_PushButton)
        {
            eField = EPdfField::PushButton;
        }
        else if ( ( flags & PdfButton::ePdfButton_Radio ) == PdfButton::ePdfButton_Radio)
        {
            eField = EPdfField::RadioButton;
        }
        else
        {
            eField = EPdfField::CheckBox;
        }
    }
    else if (fieldType == PdfName("Tx"))
    {
        eField = EPdfField::TextField;
    }
    else if (fieldType == PdfName("Ch"))
    {
        int64_t flags;
        PdfField::GetFieldFlags(rObject, flags);

        if ( ( flags & PdfListField::ePdfListField_Combo ) == PdfListField::ePdfListField_Combo )
        {
            eField = EPdfField::ComboBox;
        }
        else
        {
            eField = EPdfField::ListBox;
        }
    }
    else if (fieldType == PdfName("Sig"))
    {
        eField = EPdfField::Signature;
    }

    return eField;
}

void PdfField::Init(PdfAcroForm *pParent)
{
    if (pParent != nullptr)
    {
        // Insert into the parents kids array
        PdfArray& fields = pParent->GetFieldsArray();
        fields.push_back(m_pObject->GetIndirectReference());
    }

    PdfDictionary &dict = m_pObject->GetDictionary();
    switch( m_eField ) 
    {
        case EPdfField::CheckBox:
            dict.AddKey(PdfName("FT"), PdfName("Btn"));
            break;
        case EPdfField::PushButton:
            dict.AddKey(PdfName("FT"), PdfName("Btn"));
            dict.AddKey("Ff", PdfObject((int64_t)PdfButton::ePdfButton_PushButton));
            break;
        case EPdfField::RadioButton:
            dict.AddKey( PdfName("FT"), PdfName("Btn") );
            dict.AddKey("Ff", PdfObject((int64_t)(PdfButton::ePdfButton_Radio | PdfButton::ePdfButton_NoToggleOff)));
            break;
        case EPdfField::TextField:
            dict.AddKey( PdfName("FT"), PdfName("Tx") );
            break;
        case EPdfField::ListBox:
            dict.AddKey(PdfName("FT"), PdfName("Ch"));
            break;
        case EPdfField::ComboBox:
            dict.AddKey(PdfName("FT"), PdfName("Ch"));
            dict.AddKey("Ff", PdfObject((int64_t)PdfListField::ePdfListField_Combo));
            break;
        case EPdfField::Signature:
            dict.AddKey( PdfName("FT"), PdfName("Sig") );
            break;

        case EPdfField::Unknown:
        default:
        {
            PODOFO_RAISE_ERROR( EPdfError::InternalLogic );
        }
        break;
    }
}

PdfField::PdfField( PdfObject* pObject, PdfAnnotation* pWidget )
    : m_eField( EPdfField::Unknown ), m_pObject( pObject ), m_pWidget( pWidget )
{
    m_eField = GetFieldType( *pObject );
}

PdfObject* PdfField::GetAppearanceCharacteristics( bool bCreate ) const
{
    PdfObject* pMK = nullptr;

    if( !m_pObject->GetDictionary().HasKey( PdfName("MK") ) && bCreate )
    {
        PdfDictionary dictionary;
        const_cast<PdfField*>(this)->m_pObject->GetDictionary().AddKey( PdfName("MK"), dictionary );
    }

    pMK = m_pObject->GetDictionary().GetKey( PdfName("MK") );

    return pMK;
}

void PdfField::AssertTerminalField() const
{
    auto& dict = GetDictionary();
    if (dict.HasKey("Kids"))
        PODOFO_RAISE_ERROR_INFO(EPdfError::InternalLogic, "This method can be called only on terminal field. Ensure this field has "
            "not been retrieved from AcroFormFields collection or it's not a parent of terminal fields");
}

void PdfField::SetFieldFlag(int64_t lValue, bool bSet )
{
    int64_t lCur = 0;

    if( m_pObject->GetDictionary().HasKey( PdfName("Ff") ) )
        lCur = m_pObject->GetDictionary().GetKey( PdfName("Ff") )->GetNumber();
    
    if( bSet )
        lCur |= lValue;
    else
    {
        if( (lCur & lValue) == lValue )
            lCur ^= lValue;
    }

    m_pObject->GetDictionary().AddKey( PdfName("Ff"), lCur );
}

bool PdfField::GetFieldFlag(int64_t lValue, bool bDefault ) const
{
    int64_t flag;
    if ( !GetFieldFlags( *m_pObject, flag) )
        return bDefault;

    return ( flag & lValue ) == lValue;
}


bool PdfField::GetFieldFlags( const PdfObject & rObject, int64_t & lValue )
{
    const PdfDictionary &rDict = rObject.GetDictionary();

    const PdfObject *pFlagsObject = rDict.GetKey( "Ff" );
    PdfObject *pParentObect;
    if( pFlagsObject == nullptr && ( ( pParentObect = rObject.GetIndirectKey( "Parent" ) ) == nullptr
        || ( pFlagsObject = pParentObect->GetDictionary().GetKey( "Ff" ) ) == nullptr ) )
    {
        lValue = 0;
        return false;
    }

    lValue = pFlagsObject->GetNumber();
    return true;
}

void PdfField::SetHighlightingMode( EPdfHighlightingMode eMode )
{
    PdfName value;

    switch( eMode ) 
    {
        case EPdfHighlightingMode::None:
            value = PdfName("N");
            break;
        case EPdfHighlightingMode::Invert:
            value = PdfName("I");
            break;
        case EPdfHighlightingMode::InvertOutline:
            value = PdfName("O");
            break;
        case EPdfHighlightingMode::Push:
            value = PdfName("P");
            break;
        case EPdfHighlightingMode::Unknown:
        default:
            PODOFO_RAISE_ERROR( EPdfError::InvalidName );
            break;
    }

    m_pObject->GetDictionary().AddKey( PdfName("H"), value );
}

EPdfHighlightingMode PdfField::GetHighlightingMode() const
{
    EPdfHighlightingMode eMode = EPdfHighlightingMode::Invert;

    if( m_pObject->GetDictionary().HasKey( PdfName("H") ) )
    {
        PdfName value = m_pObject->GetDictionary().GetKey( PdfName("H") )->GetName();
        if( value == PdfName("N") )
            return EPdfHighlightingMode::None;
        else if( value == PdfName("I") )
            return EPdfHighlightingMode::Invert;
        else if( value == PdfName("O") )
            return EPdfHighlightingMode::InvertOutline;
        else if( value == PdfName("P") )
            return EPdfHighlightingMode::Push;
    }

    return eMode;
}

void PdfField::SetBorderColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBorderColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BC"), array );
}

void PdfField::SetBackgroundColorTransparent()
{
    PdfArray array;

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dGray )
{
    PdfArray array;
    array.push_back( dGray );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dRed, double dGreen, double dBlue )
{
    PdfArray array;
    array.push_back( dRed );
    array.push_back( dGreen );
    array.push_back( dBlue );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetBackgroundColor( double dCyan, double dMagenta, double dYellow, double dBlack )
{
    PdfArray array;
    array.push_back( dCyan );
    array.push_back( dMagenta );
    array.push_back( dYellow );
    array.push_back( dBlack );

    PdfObject* pMK = this->GetAppearanceCharacteristics( true );
    pMK->GetDictionary().AddKey( PdfName("BG"), array );
}

void PdfField::SetName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("T"), rsName );
}

optional<PdfString> PdfField::GetName() const
{
    PdfObject *name = m_pObject->GetDictionary().FindKeyParent( "T" );
    if( name == nullptr )
        return { };

    return name->GetString();
}

optional<PdfString> PdfField::GetNameRaw() const
{
    PdfObject *name = m_pObject->GetDictionary().GetKey("T");
    if (name == nullptr)
        return { };

    return name->GetString();
}

string PdfField::GetFullName(bool escapePartialNames) const
{
    string fullName;
    getFullName(m_pObject, escapePartialNames, fullName);
    return fullName;
}

void PdfField::SetAlternateName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TU"), rsName );
}

optional<PdfString> PdfField::GetAlternateName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TU" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TU" ) )->GetString();

    return { };
}

void PdfField::SetMappingName( const PdfString & rsName )
{
    m_pObject->GetDictionary().AddKey( PdfName("TM"), rsName );
}

optional<PdfString> PdfField::GetMappingName() const
{
    if( m_pObject->GetDictionary().HasKey( PdfName("TM" ) ) )
        return m_pObject->GetDictionary().GetKey( PdfName("TM" ) )->GetString();

    return { };
}

void PdfField::AddAlternativeAction( const PdfName & rsName, const PdfAction & rAction ) 
{
    if( !m_pObject->GetDictionary().HasKey( PdfName("AA") ) ) 
        m_pObject->GetDictionary().AddKey( PdfName("AA"), PdfDictionary() );

    PdfObject* pAA = m_pObject->GetDictionary().GetKey( PdfName("AA") );
    pAA->GetDictionary().AddKey( rsName, rAction.GetObject()->GetIndirectReference() );
}

void PdfField::SetReadOnly(bool bReadOnly)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::ReadOnly), bReadOnly);
}

bool PdfField::IsReadOnly() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::ReadOnly), false);
}

void PdfField::SetRequired(bool bRequired)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::Required), bRequired);
}

bool PdfField::IsRequired() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::Required), false);
}

void PdfField::SetNoExport(bool bExport)
{
    this->SetFieldFlag(static_cast<int>(EPdfFieldFlags::NoExport), bExport);
}

bool PdfField::IsNoExport() const
{
    return this->GetFieldFlag(static_cast<int>(EPdfFieldFlags::NoExport), false);
}

PdfPage* PdfField::GetPage() const
{
    return m_pWidget->GetPage();
}

void PdfField::SetMouseEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("E"), rAction);
}

void PdfField::SetMouseLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("X"), rAction);
}

void PdfField::SetMouseDownAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("D"), rAction);
}

void PdfField::SetMouseUpAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("U"), rAction);
}

void PdfField::SetFocusEnterAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("Fo"), rAction);
}

void PdfField::SetFocusLeaveAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("BI"), rAction);
}

void PdfField::SetPageOpenAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PO"), rAction);
}

void PdfField::SetPageCloseAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PC"), rAction);
}

void PdfField::SetPageVisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PV"), rAction);
}

void PdfField::SetPageInvisibleAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("PI"), rAction);
}

void PdfField::SetKeystrokeAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("K"), rAction);
}

void PdfField::SetValidateAction(const PdfAction& rAction)
{
    this->AddAlternativeAction(PdfName("V"), rAction);
}

EPdfField PdfField::GetType() const
{
    return m_eField;
}

PdfAnnotation* PdfField::GetWidgetAnnotation() const
{
    return m_pWidget;
}

PdfObject* PdfField::GetFieldObject() const
{
    return m_pObject;
}

PdfDictionary& PdfField::GetDictionary()
{
    return m_pObject->GetDictionary();
}

const PdfDictionary& PdfField::GetDictionary() const
{
    return m_pObject->GetDictionary();
}

void getFullName(const PdfObject* obj, bool escapePartialNames, string& fullname)
{
    const PdfDictionary& dict = obj->GetDictionary();
    auto parent = dict.FindKey("Parent");;
    if (parent != nullptr)
        getFullName(parent, escapePartialNames, fullname);

    const PdfObject* nameObj = dict.GetKey("T");
    if (nameObj != nullptr)
    {
        string name = nameObj->GetString().GetString();
        if (escapePartialNames)
        {
            // According to ISO 32000-1:2008, "12.7.3.2 Field Names":
            // "Because the PERIOD is used as a separator for fully
            // qualified names, a partial name shall not contain a
            // PERIOD character."
            // In case the partial name still has periods (effectively
            // violating the standard and Pdf Reference) the fullname
            // would be unintelligible, let's escape them with double
            // dots "..", example "parent.partial..name"
            size_t currpos = 0;
            while ((currpos = name.find('.', currpos)) != std::string::npos)
            {
                name.replace(currpos, 1, "..");
                currpos += 2;
            }
        }

        if (fullname.length() == 0)
            fullname = name;
        else
            fullname.append(".").append(name);
    }
}

