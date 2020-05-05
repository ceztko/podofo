#include "PdfPushButton.h"

using namespace std;
using namespace PoDoFo;

PdfPushButton::PdfPushButton(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfButton(EPdfField::PushButton, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfPushButton::PdfPushButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::PushButton, pWidget, pDoc, insertInAcroform)
{
    Init();
}

PdfPushButton::PdfPushButton(PdfPage* pPage, const PdfRect& rRect)
    : PdfButton(EPdfField::PushButton, pPage, rRect)
{
    Init();
}

PdfPushButton::PdfPushButton(const PdfField& rhs)
    : PdfButton(rhs)
{
    if (this->GetType() != EPdfField::CheckBox)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Field cannot be converted into a PdfPushButton");
    }
}

void PdfPushButton::Init()
{
    // make a push button
    this->SetFieldFlag(static_cast<int>(ePdfButton_PushButton), true);
}

void PdfPushButton::SetRolloverCaption(const PdfString& rsText)
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey(PdfName("RC"), rsText);
}

optional<PdfString>  PdfPushButton::GetRolloverCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(false);
    if (pMK && pMK->GetDictionary().HasKey(PdfName("RC")))
        return pMK->GetDictionary().GetKey(PdfName("RC"))->GetString();

    return { };
}

void PdfPushButton::SetAlternateCaption(const PdfString& rsText)
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(true);
    pMK->GetDictionary().AddKey(PdfName("AC"), rsText);

}

optional<PdfString>  PdfPushButton::GetAlternateCaption() const
{
    PdfObject* pMK = this->GetAppearanceCharacteristics(false);
    if (pMK && pMK->GetDictionary().HasKey(PdfName("AC")))
        return pMK->GetDictionary().GetKey(PdfName("AC"))->GetString();

    return { };
}
