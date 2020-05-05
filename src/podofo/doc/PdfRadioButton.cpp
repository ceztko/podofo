#include "PdfRadioButton.h"

using namespace PoDoFo;

PdfRadioButton::PdfRadioButton(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfButton(EPdfField::RadioButton, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfRadioButton::PdfRadioButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfButton(EPdfField::RadioButton, pWidget, pDoc, insertInAcroform)
{
}

PdfRadioButton::PdfRadioButton(PdfPage* pPage, const PdfRect& rRect)
    : PdfButton(EPdfField::RadioButton, pPage, rRect)
{
}

PdfRadioButton::PdfRadioButton(const PdfField& rhs)
    : PdfButton(rhs)
{
}
