#ifndef PDF_BUTTON_H
#define PDF_BUTTON_H

#include "PdfField.h"

namespace PoDoFo
{
    class PODOFO_DOC_API PdfButton : public PdfField
    {
        friend class PdfField;
    protected:
        enum
        {
            ePdfButton_NoToggleOff = 0x0004000,
            ePdfButton_Radio = 0x0008000,
            ePdfButton_PushButton = 0x0010000,
            ePdfButton_RadioInUnison = 0x2000000
        };

        /** Create a new PdfButton
        */
        PdfButton(EPdfField eField, PdfAnnotation* pWidget, PdfDocument& doc, bool insertInAcrofrom);

        /** Create a new PdfButton
        */
        PdfButton(EPdfField eField, PdfObject* pObject, PdfAnnotation* pWidget);

        /** Create a new PdfButton
         */
        PdfButton(EPdfField eField, PdfPage* pPage, const PdfRect& rRect);

    public:
        /**
         * \returns true if this is a pushbutton
         */
        bool IsPushButton() const;

        /**
         * \returns true if this is a checkbox
         */
        bool IsCheckBox() const;

        /**
         * \returns true if this is a radiobutton
         */
        bool IsRadioButton() const;

        /** Set the normal caption of this button
         *
         *  \param rsText the caption
         */
        void SetCaption(const PdfString& rsText);

        /**
         *  \returns the caption of this button
         */
        std::optional<PdfString> GetCaption() const;
    };
}

#endif // PDF_BUTTON_H
