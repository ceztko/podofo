#ifndef PDF_LISTBOX_H
#define PDF_LISTBOX_H

#include "PdfChoiceField.h"

namespace PoDoFo
{
    /** A list box
     */
    class PODOFO_DOC_API PdfListBox : public PdChoiceField
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
    };
}

#endif // PDF_LISTBOX_H
