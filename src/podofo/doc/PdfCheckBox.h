#ifndef PDF_CHECKBOX_H
#define PDF_CHECKBOX_H

#include "PdfButton.h"
#include "PdfXObject.h"

namespace PoDoFo
{
    /** A checkbox can be checked or unchecked by the user
     */
    class PODOFO_DOC_API PdfCheckBox : public PdfButton
    {
        friend class PdfField;
    private:
        /** Create a new PdfCheckBox
         */
        PdfCheckBox(PdfObject* pObject, PdfAnnotation* pWidget);
    public:
        /** Create a new PdfRadioButton
         */
        PdfCheckBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfCheckBox
         */
        PdfCheckBox(PdfPage* pPage, const PdfRect& rRect);

        /** Set the appearance stream which is displayed when the checkbox
         *  is checked.
         *
         *  \param rXObject an xobject which contains the drawing commands for a checked checkbox
         */
        void SetAppearanceChecked(const PdfXObject& rXObject);

        /** Set the appearance stream which is displayed when the checkbox
         *  is unchecked.
         *
         *  \param rXObject an xobject which contains the drawing commands for an unchecked checkbox
         */
        void SetAppearanceUnchecked(const PdfXObject& rXObject);

        /** Sets the state of this checkbox
         *
         *  \param bChecked if true the checkbox will be checked
         */
        void SetChecked(bool bChecked);

        /**
         * \returns true if the checkbox is checked
         */
        bool IsChecked() const;

    private:

        /** Add a appearance stream to this checkbox
         *
         *  \param rName name of the appearance stream
         *  \param rReference reference to the XObject containing the appearance stream
         */
        void AddAppearanceStream(const PdfName& rName, const PdfReference& rReference);
    };
}

#endif // PDF_CHECKBOX_H
