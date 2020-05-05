#include "PdfButton.h"

using namespace std;
using namespace PoDoFo;

PdfButton::PdfButton(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& doc, bool insertInAcrofrom)
    : PdfField(eField, pWidget, doc, insertInAcrofrom)
{
}

PdfButton::PdfButton(EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfField(eField, pObject, pWidget)
{
}

PdfButton::PdfButton(EPdfField eField, PdfPage* pPage, const PdfRect& rRect)
    : PdfField(eField, pPage, rRect)
{
}

bool PdfButton::IsPushButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false);
}

bool PdfButton::IsCheckBox() const
{
    return (!this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false) &&
        !this->GetFieldFlag(static_cast<int>(ePdfButton_PushButton), false));
}

bool PdfButton::IsRadioButton() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfButton_Radio), false);
}

void PdfButton::SetCaption(const PdfString& rsText)
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey(PdfName("CA"), rsText);
}

optional<PdfString> PdfButton::GetCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(false);
    if (pMK && pMK->GetDictionary().HasKey(PdfName("CA")))
        return pMK->GetDictionary().GetKey(PdfName("CA"))->GetString();

    return { };
}
