#ifndef PDF_RADIO_BUTTON_H
#define PDF_RADIO_BUTTON_H

#include "PdfButton.h"

namespace PoDoFo
{
    /** A radio button
     * TODO: This is just a stub
     */
    class PODOFO_DOC_API PdfRadioButton : public PdfButton
    {
        friend class PdfField;
    private:
        /** Create a new PdfRadioButton
         */
        PdfRadioButton(PdfObject* pObject, PdfAnnotation* pWidget);
    public:
        /** Create a new PdfRadioButton
         */
        PdfRadioButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfRadioButton
         */
        PdfRadioButton(PdfPage* pPage, const PdfRect& rRect);
    };
}

#endif // PDF_RADIO_BUTTON_H
