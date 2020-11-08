#include "PdfListBox.h"

using namespace PoDoFo;

PdfListBox::PdfListBox(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdChoiceField(EPdfField::ListBox, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfListBox::PdfListBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdChoiceField(EPdfField::ListBox, pWidget, pDoc, insertInAcroform)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

PdfListBox::PdfListBox(PdfPage* pPage, const PdfRect& rRect)
    : PdChoiceField(EPdfField::ListBox, pPage, rRect)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}
