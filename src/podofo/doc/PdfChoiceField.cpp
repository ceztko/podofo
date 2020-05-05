#include "PdfChoiceField.h"

using namespace std;
using namespace PoDoFo;


PdfListField::PdfListField(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform)
    : PdfField(eField, pWidget, pDoc, insertInAcroform)
{
}

PdfListField::PdfListField(EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget)
    : PdfField(eField, pObject, pWidget)
{
}

PdfListField::PdfListField(EPdfField eField, PdfPage* pPage, const PdfRect& rRect)
    : PdfField(eField, pPage, rRect)
{
}

void PdfListField::InsertItem(const PdfString& rsValue, const optional<PdfString>& rsDisplayName)
{
    PdfVariant var;
    PdfArray   opt;

    if (!rsDisplayName.has_value())
    {
        var = rsValue;
    }
    else
    {
        PdfArray array;
        array.push_back(rsValue);
        array.push_back(rsDisplayName.value());
        var = array;
    }

    if (GetFieldObject()->GetDictionary().HasKey(PdfName("Opt")))
        opt = GetFieldObject()->GetDictionary().GetKey(PdfName("Opt"))->GetArray();

    // TODO: Sorting
    opt.push_back(var);
    GetFieldObject()->GetDictionary().AddKey(PdfName("Opt"), opt);

    /*
    m_pObject->GetDictionary().AddKey( PdfName("V"), rsValue );

    PdfArray array;
    array.push_back( 0L );
    m_pObject->GetDictionary().AddKey( PdfName("I"), array );
    */
}

void PdfListField::RemoveItem(int nIndex)
{
    PdfArray   opt;

    if (GetFieldObject()->GetDictionary().HasKey(PdfName("Opt")))
        opt = GetFieldObject()->GetDictionary().GetKey(PdfName("Opt"))->GetArray();

    if (nIndex < 0 || nIndex > static_cast<int>(opt.size()))
    {
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
    }

    opt.erase(opt.begin() + nIndex);
    GetFieldObject()->GetDictionary().AddKey(PdfName("Opt"), opt);
}

PdfString PdfListField::GetItem(int nIndex) const
{
    PdfObject* opt = GetFieldObject()->GetDictionary().FindKey("Opt");
    if (opt == NULL)
        PODOFO_RAISE_ERROR(EPdfError::InvalidHandle);

    PdfArray& optArray = opt->GetArray();
    if (nIndex < 0 || nIndex >= optArray.GetSize())
    {
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
    }

    PdfObject& item = optArray[nIndex];
    if (item.IsArray())
    {
        PdfArray& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
        }
        else
            return itemArray.FindAt(0).GetString();
    }

    return item.GetString();
}

optional<PdfString> PdfListField::GetItemDisplayText(int nIndex) const
{
    PdfObject* opt = GetFieldObject()->GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return { };

    PdfArray& optArray = opt->GetArray();
    if (nIndex < 0 || nIndex >= static_cast<int>(optArray.size()))
    {
        PODOFO_RAISE_ERROR(EPdfError::ValueOutOfRange);
    }

    PdfObject& item = optArray[nIndex];
    if (item.IsArray())
    {
        PdfArray& itemArray = item.GetArray();
        if (itemArray.size() < 2)
        {
            PODOFO_RAISE_ERROR(EPdfError::InvalidDataType);
        }
        else
            return itemArray.FindAt(1).GetString();
    }

    return item.GetString();
}

size_t PdfListField::GetItemCount() const
{
    PdfObject* opt = GetFieldObject()->GetDictionary().FindKey("Opt");
    if (opt == NULL)
        return 0;

    return opt->GetArray().size();
}

void PdfListField::SetSelectedIndex(int nIndex)
{
    AssertTerminalField();
    PdfString selected = this->GetItem(nIndex);
    GetFieldObject()->GetDictionary().AddKey("V", selected);
}

int PdfListField::GetSelectedIndex() const
{
    AssertTerminalField();
    PdfObject* valueObj = GetFieldObject()->GetDictionary().FindKey("V");
    if (valueObj == nullptr || !valueObj->IsString())
        return -1;

    PdfString value = valueObj->GetString();
    PdfObject* opt = GetFieldObject()->GetDictionary().FindKey("Opt");
    if (opt == nullptr)
        return -1;

    PdfArray& optArray = opt->GetArray();
    for (int i = 0; i < optArray.GetSize(); i++)
    {
        auto& found = optArray.FindAt(i);
        if (found.IsString())
        {
            if (found.GetString() == value)
                return i;
        }
        else if (found.IsArray())
        {
            auto& arr = found.GetArray();
            if (arr.FindAt(0).GetString() == value)
                return i;
        }
        else
        {
            PODOFO_RAISE_ERROR_INFO(EPdfError::InvalidDataType, "Choice field item has invaid data type");
        }
    }

    return -1;
}

bool PdfListField::IsComboBox() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Combo), false);
}

void PdfListField::SetSpellcheckingEnabled(bool bSpellcheck)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), !bSpellcheck);
}

bool PdfListField::IsSpellcheckingEnabled() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_NoSpellcheck), true);
}

void PdfListField::SetSorted(bool bSorted)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_Sort), bSorted);
}

bool PdfListField::IsSorted() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_Sort), false);
}

void PdfListField::SetMultiSelect(bool bMulti)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), bMulti);
}

bool PdfListField::IsMultiSelect() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_MultiSelect), false);
}

void PdfListField::SetCommitOnSelectionChange(bool bCommit)
{
    this->SetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), bCommit);
}

bool PdfListField::IsCommitOnSelectionChange() const
{
    return this->GetFieldFlag(static_cast<int>(ePdfListField_CommitOnSelChange), false);
}
