#ifndef PDF_PUSH_BUTTON_H
#define PDF_PUSH_BUTTON_H

#include "PdfButton.h"

namespace PoDoFo
{
    /** A push button is a button which has no state and value
     *  but can toggle actions.
     */
    class PODOFO_DOC_API PdfPushButton : public PdfButton
    {
        friend class PdfField;
    private:
        /** Create a new PdfPushButton
         */
        PdfPushButton(PdfObject* pObject, PdfAnnotation* pWidget);

    public:
        /** Create a new PdfPushButton
         */
        PdfPushButton(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfPushButton
         */
        PdfPushButton(PdfPage* pPage, const PdfRect& rRect);

        /** Create a PdfPushButton from a PdfField
         *  \param rhs a PdfField that is a push button button
         *
         *  Raises an error if PdfField::GetType() != EPdfField::PushButton
         */
        PdfPushButton(const PdfField& rhs);

        /** Set the rollover caption of this button
         *  which is displayed when the cursor enters the field
         *  without the mouse button being pressed
         *
         *  \param rsText the caption
         */
        void SetRolloverCaption(const PdfString& rsText);

        /**
         *  \returns the rollover caption of this button
         */
        std::optional<PdfString> GetRolloverCaption() const;

        /** Set the alternate caption of this button
         *  which is displayed when the button is pressed.
         *
         *  \param rsText the caption
         */
        void SetAlternateCaption(const PdfString& rsText);

        /**
         *  \returns the rollover caption of this button
         */
        std::optional<PdfString> GetAlternateCaption() const;

    private:
        void Init();
    };
}

#endif // PDF_PUSH_BUTTON_H
