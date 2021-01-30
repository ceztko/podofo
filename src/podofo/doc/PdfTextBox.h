#ifndef PDF_TEXTBOX_H
#define PDF_TEXTBOX_H

#include "PdfField.h"

namespace PoDoFo
{
    /** A text field in a PDF file.
     *
     *  Users can enter text into a text field.
     *  Single and multi line text is possible,
     *  as well as richtext. The text can be interpreted
     *  as path to a file which is going to be submitted.
     */
    class PODOFO_DOC_API PdfTextBox : public PdfField
    {
        friend class PdfField;
    private:
        enum
        {
            ePdfTextField_MultiLine = 0x0001000,
            ePdfTextField_Password = 0x0002000,
            ePdfTextField_FileSelect = 0x0100000,
            ePdfTextField_NoSpellcheck = 0x0400000,
            ePdfTextField_NoScroll = 0x0800000,
            ePdfTextField_Comb = 0x1000000,
            ePdfTextField_RichText = 0x2000000
        };

    private:
        PdfTextBox(PdfObject* pObject, PdfAnnotation* pWidget);

    public:
        /** Create a new PdfTextBox
         */
        PdfTextBox(PdfAnnotation* pWidget, PdfDocument& pDoc, bool insertInAcroform);

        /** Create a new PdfTextBox
         */
        PdfTextBox(PdfPage* pPage, const PdfRect& rRect);

        /** Create a PdfTextBox from a PdfField
         *
         *  \param rhs a PdfField that is a PdfTextField
         *
         *  Raises an error if PdfField::GetType() != EPdfField::TextField
         */
        PdfTextBox(const PdfField& rhs);

        /** Sets the text contents of this text field.
         *
         *  \param rsText the text of this field
         */
        void SetText(const PdfString& rsText);

        /**
         *  \returns the text contents of this text field
         */
        PdfString GetText() const;

        /** Sets the max length in characters of this textfield
         *  \param nMaxLen the max length of this textfields in characters
         */
        void SetMaxLen(int64_t nMaxLen);

        /**
         * \returns the max length of this textfield in characters or -1
         *          if no max length was specified
         */
        int64_t GetMaxLen() const;

        /**
         *  Create a multi-line text field that can contains multiple lines of text.
         *  \param bMultiLine if true a multi line field is generated, otherwise
         *                    the text field can contain only a single line of text.
         *
         *  The default is to create a single line text field.
         */
        void SetMultiLine(bool bMultiLine);

        /**
         * \returns true if this text field can contain multiple lines of text
         */
        bool IsMultiLine() const;

        /**
         *  Create a password text field that should not echo entered
         *  characters visibly to the screen.
         *
         *  \param bPassword if true a password field is created
         *
         *  The default is to create no password field
         */
        void SetPasswordField(bool bPassword);

        /**
         * \returns true if this field is a password field that does
         *               not echo entered characters on the screen
         */
        bool IsPasswordField() const;

        /**
         *  Create a file selection field.
         *  The entered contents are treated as filename to a file
         *  whose contents are submitted as the value of the field.
         *
         *  \param bFile if true the contents are treated as a pathname
         *               to a file to submit
         */
        void SetFileField(bool bFile);

        /**
         * \returns true if the contents are treated as filename
         */
        bool IsFileField() const;

        /**
         *  Enable/disable spellchecking for this text field
         *
         *  \param bSpellcheck if true spellchecking will be enabled
         *
         *  Text fields are spellchecked by default
         */
        void SetSpellcheckingEnabled(bool bSpellcheck);

        /**
         *  \returns true if spellchecking is enabled for this text field
         */
        bool IsSpellcheckingEnabled() const;

        /**
         *  Enable/disable scrollbars for this text field
         *
         *  \param bScroll if true scrollbars will be enabled
         *
         *  Text fields have scrollbars by default
         */
        void SetScrollBarsEnabled(bool bScroll);

        /**
         *  \returns true if scrollbars are enabled for this text field
         */
        bool IsScrollBarsEnabled() const;

        /**
         *  Divide the text field into max-len equal
         *  combs.
         *
         *  \param bCombs if true enable division into combs
         *
         *  By default combs are disabled. Requires the max-len
         *  property to be set.
         *
         *  \see SetMaxLen
         */
        void SetCombs(bool bCombs);

        /**
         * \returns true if the text field has a division into equal combs set on it
         */
        bool IsCombs() const;

        /**
         * Creates a richtext field.
         *
         * \param bRichText if true creates a richtext field
         *
         * By default richtext is disabled.
         */
        void SetRichText(bool bRichText);

        /**
         * \returns true if this is a richtext text field
         */
        bool IsRichText() const;

    private:
        void Init();
    };
}

#endif // PDF_TEXTBOX_H
