// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class ParagraphNode;

//---------------------------------------------------------------------------
//
//  ParagraphTextSource
//
//  TextSource for Paragraph element. Can also be used for TextBlock since
//  both Paragraph and TextBlock are based on InlineCollection content model.
//
//---------------------------------------------------------------------------
class ParagraphTextSource : public RichTextServices::TextSource
{
public:
    // Creates an instance of this ParagraphTextSource. If the owning control is
    // a TextBlock, pContent may be null since the TextBlock acts as a paragraph in this case.
    ParagraphTextSource(
        _In_opt_ CParagraph *pParagraph,
        _In_ CFrameworkElement *pContentOwner
        );
    ~ParagraphTextSource() override;

    // Fetches one run of text.
    RichTextServices::Result::Enum GetTextRun(
        _In_ XUINT32 characterIndex,
        _Outptr_ RichTextServices::TextRun **ppTextRun
        ) override;

    // Gets the embedded object host for the TextSource in current formatting context.
    IEmbeddedElementHost *GetEmbeddedElementHost() const override;

    // Sets the ParagraphNode that uses this TextSource for formatting.
    void SetParagraphNode(_In_ ParagraphNode *pNode);

    // Checks if a position is within a surrogate pair or CRLF sequence.
    _Check_return_ HRESULT IsInSurrogateCRLF(
        _In_ XUINT32 characterIndex,
        _Out_ bool *pIsInSurrogateCRLF
        ) const;

    static _Check_return_ HRESULT IsInSurrogateCRLF(
        _In_ CInlineCollection* inlines,
        _In_ XUINT32 characterIndex,
        _Out_ bool *pIsInSurrogateCRLF
        );

private:
    CParagraph *m_pParagraph;
    CFrameworkElement *m_pContentOwner;
    ParagraphNode *m_pParagraphNode;

    // Checks for formatting control characters that require special types of runs.
    static _Check_return_ HRESULT SplitAtFormatControlCharacter(
        _In_ XUINT32 characterIndex,
        _In_reads_(length) const WCHAR *pCharacters,
        _In_ XUINT32 length,
        _In_opt_ RichTextServices::TextRunProperties *pTextProperties,
        _Outptr_ RichTextServices::TextRun **ppTextRun
        );

    static bool IsFormatControlCharacter(WCHAR character)
    {
        // Currently XAML new lines and tabs are the only formatting control characters.
        return (IsXamlNewline(character) ||
                character == UNICODE_CHARACTER_TABULATION);
    }

    // Gets DirectionalControl change caused by a TextElement if it has a flow direction different than
    // its parent TextElement.
    _Check_return_ HRESULT GetDirectionalControl(
        _In_ CTextElement* pTextElement,
        _In_ bool isCloseNesting,
        _Out_ RichTextServices::DirectionalControl::Enum *pControl
    );

    // Gets the font context for this TextSource.
    CFontContext *GetFontContext();
};



