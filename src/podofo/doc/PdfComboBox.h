#ifndef PDF_COMBOBOX_H
#define PDF_COMBOBOX_H

#include "PdfChoiceField.h"

namespace PoDoFo
{
    /** A combo box with a drop down list of items.
     */
    class PODOFO_DOC_API PdfComboBox : public PdChoiceField
    {
        friend class PdfField;
    private:
        PdfComboBox(PdfObject* pObject, PdfAnnotation* pWidget);

    public:
        /** Create a new PdfComboBox
         */
        PdfComboBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfTextField
         */
        PdfComboBox(PdfPage* pPage, const PdfRect& rRect);

        /** Create a PdfComboBox from a PdfField
         *
         *  \param rhs a PdfField that is a PdfComboBox
         *
         *  Raises an error if PdfField::GetType() != EPdfField::ComboBox
         */
        PdfComboBox(const PdfField& rhs);

        /**
         * Sets the combobox to be editable
         *
         * \param bEdit if true the combobox can be edited by the user
         *
         * By default a combobox is not editable
         */
        void SetEditable(bool bEdit);

        /**
         *  \returns true if this is an editable combobox
         */
        bool IsEditable() const;

    };
}

#endif // PDF_COMBOBOX_H
