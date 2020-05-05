#ifndef PDF_LISTBOX_H
#define PDF_LISTBOX_H

#include "PdfChoiceField.h"

namespace PoDoFo
{
    /** A list box
     */
    class PODOFO_DOC_API PdfListBox : public PdfListField
    {
        friend class PdfField;
    private:
        PdfListBox(PdfObject* pObject, PdfAnnotation* pWidget);

    public:
        /** Create a new PdfListBox
         */
        PdfListBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfListBox
         */
        PdfListBox(PdfPage* pPage, const PdfRect& rRect);

        /** Create a PdfListBox from a PdfField
         *
         *  \param rhs a PdfField that is a PdfComboBox
         *
         *  Raises an error if PdfField::GetType() != EPdfField::ListBox
         */
        PdfListBox(const PdfField& rhs);

    };
}

#endif // PDF_LISTBOX_H
