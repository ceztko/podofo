#include "PdfTextBox.h"

using namespace PoDoFo;

PdfTextBox::PdfTextBox(PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfField(EPdfField::TextField, pObject, pWidget)
{
    // NOTE: We assume initialization was performed in the given object
}

PdfTextBox::PdfTextBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfField(EPdfField::TextField, pWidget, pDoc, insertInAcroform)
{
    Init();
}

PdfTextBox::PdfTextBox(PdfPage* pPage, const PdfRect& rRect)
    : PdfField(EPdfField::TextField, pPage, rRect)
{
    Init();
}

void PdfTextBox::Init()
{
    if (!GetFieldObject()->GetDictionary().HasKey(PdfName("DS")))
        GetFieldObject()->GetDictionary().AddKey(PdfName("DS"), PdfString("font: 12pt Helvetica"));
}

void PdfTextBox::SetText(const PdfString& rsText)
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");

    // if rsText is longer than maxlen, truncate it
    int64_t nMax = this->GetMaxLen();
    if (nMax != -1 && rsText.GetLength() > (size_t)nMax)
        PODOFO_RAISE_ERROR_INFO(EPdfError::ValueOutOfRange, "Unable to set text larger MaxLen");

    GetFieldObject()->GetDictionary().AddKey(key, rsText);
}

PdfString PdfTextBox::GetText() const
{
    AssertTerminalField();
    PdfName key = this->IsRichText() ? PdfName("RV") : PdfName("V");
    PdfString str;

    auto found = GetFieldObject()->GetDictionary().FindKeyParent(key);
    if (found == nullptr)
        return str;

    return found->GetString();
}

void PdfTextBox::SetMaxLen(int64_t nMaxLen)
{
    GetFieldObject()->GetDictionary().AddKey(PdfName("MaxLen"), nMaxLen);
}

int64_t PdfTextBox::GetMaxLen() const
{
    auto found = GetFieldObject()->GetDictionary().FindKeyParent("MaxLen");
    if (found == nullptr)
        return -1;

    return found->GetNumber();
}

void PdfTextBox::SetMultiLine(bool bMultiLine)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), bMultiLine);
}

bool PdfTextBox::IsMultiLine() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_MultiLine), false);
}

void PdfTextBox::SetPasswordField(bool bPassword)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Password), bPassword);
}

bool PdfTextBox::IsPasswordField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Password), false);
}

void PdfTextBox::SetFileField(bool bFile)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), bFile);
}

bool PdfTextBox::IsFileField() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_FileSelect), false);
}

void PdfTextBox::SetSpellcheckingEnabled(bool bSpellcheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), !bSpellcheck);
}

bool PdfTextBox::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoSpellcheck), true);
}

void PdfTextBox::SetScrollBarsEnabled(bool bScroll)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), !bScroll);
}

bool PdfTextBox::IsScrollBarsEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_NoScroll), true);
}

void PdfTextBox::SetCombs(bool bCombs)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_Comb), bCombs);
}

bool PdfTextBox::IsCombs() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_Comb), false);
}

void PdfTextBox::SetRichText(bool bRichText)
{
    this->SetFieldFlag(static_cast<int>(ePdfTextField_RichText), bRichText);
}

bool PdfTextBox::IsRichText() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfTextField_RichText), false);
}
