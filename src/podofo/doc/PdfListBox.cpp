#include "PdfListBox.h"

using namespace PoDoFo;

PdfListBox::PdfListBox(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfListField(EPdfField::ListBox, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfListBox::PdfListBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfListField(EPdfField::ListBox, pWidget, pDoc, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfPage* pPage, const PdfRect& rRect)
    : PdfListField(EPdfField::ListBox, pPage, rRect)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(const PdfField& rhs)
    : PdfListField(rhs)
{
    if (this->GetType() != EPdfField::ListBox)
    {
        PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Field cannot be converted into a PdfTextField");
    }
}
