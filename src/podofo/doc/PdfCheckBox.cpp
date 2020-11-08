#include "PdfCheckBox.h"

using namespace PoDoFo;


PdfCheckBox::PdfCheckBox(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfButton(EPdfField::CheckBox, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfCheckBox::PdfCheckBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::CheckBox, pWidget, pDoc, insertInAcroform)
{
}

PdfCheckBox::PdfCheckBox(PdfPage* pPage, const PdfRect& rRect)
    : PdfButton(EPdfField::CheckBox, pPage, rRect)
{
}

void PdfCheckBox::AddAppearanceStream(const PdfName& rName, const PdfReference& rReference)
{
    if (!GetFieldObject()->GetDictionary().HasKey(PdfName("AP")))
        GetFieldObject()->GetDictionary().AddKey(PdfName("AP"), PdfDictionary());

    if (!GetFieldObject()->GetDictionary().GetKey(PdfName("AP"))->GetDictionary().HasKey(PdfName("N")))
        GetFieldObject()->GetDictionary().GetKey(PdfName("AP"))->GetDictionary().AddKey(PdfName("N"), PdfDictionary());

    GetFieldObject()->GetDictionary().GetKey(PdfName("AP"))->
        GetDictionary().GetKey(PdfName("N"))->GetDictionary().AddKey(rName, rReference);
}

void PdfCheckBox::SetAppearanceChecked(const PdfXObject& rXObject)
{
    this->AddAppearanceStream(PdfName("Yes"), rXObject.GetObject()->GetIndirectReference());
}

void PdfCheckBox::SetAppearanceUnchecked(const PdfXObject& rXObject)
{
    this->AddAppearanceStream(PdfName("Off"), rXObject.GetObject()->GetIndirectReference());
}

void PdfCheckBox::SetChecked(bool bChecked)
{
    GetFieldObject()->GetDictionary().AddKey(PdfName("V"), (bChecked ? PdfName("Yes") : PdfName("Off")));
    GetFieldObject()->GetDictionary().AddKey(PdfName("AS"), (bChecked ? PdfName("Yes") : PdfName("Off")));
}

bool PdfCheckBox::IsChecked() const
{
    PdfDictionary dic = GetFieldObject()->GetDictionary();

    if (dic.HasKey(PdfName("V"))) {
        PdfName name = dic.GetKey(PdfName("V"))->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
    }
    else if (dic.HasKey(PdfName("AS"))) {
        PdfName name = dic.GetKey(PdfName("AS"))->GetName();
        return (name == PdfName("Yes") || name == PdfName("On"));
    }

    return false;
}
