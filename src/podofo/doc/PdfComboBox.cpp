#include "PdfComboBox.h"

using namespace PoDoFo;

PdfComboBox::PdfComboBox(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdChoiceField(EPdfField::ComboBox, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfComboBox::PdfComboBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdChoiceField(EPdfField::ComboBox, pWidget, pDoc, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(PdfPage* pPage, const PdfRect& rRect)
    : PdChoiceField(EPdfField::ComboBox, pPage, rRect)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), true);
}

PdfComboBox::PdfComboBox(const PdfField& rhs)
    : PdChoiceField(rhs)
{
    if (this->GetType() != EPdfField::ComboBox)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Field cannot be converted into a PdfTextField");
    }
}

void PdfComboBox::SetEditable(bool bEdit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Edit), bEdit);
}

bool PdfComboBox::IsEditable() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Edit), false);
}
