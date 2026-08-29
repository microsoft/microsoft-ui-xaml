// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextView.h"

class RichTextBlock;
class PageNode;
struct ILinkedTextContainer;

//---------------------------------------------------------------------------
//
//  RichTextBlockView
//
//  Implements ITextView querying methods for RichTextBlock.
//
//---------------------------------------------------------------------------
class RichTextBlockView final : public ITextView
{
public:
    RichTextBlockView(
        _In_ PageNode *pPageNode
    );
    ~RichTextBlockView() {};

    _Check_return_ HRESULT TextRangeToTextBounds(
        _In_ XUINT32 startOffset,
        _In_ XUINT32 endOffset,
        _Out_ XUINT32 *pcRectangles,
        _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles
    ) override;
    _Check_return_ HRESULT TextSelectionToTextBounds(
        _In_ IJupiterTextSelection *pSelection,
        _Out_ XUINT32 *pcRectangles,
        _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles
    ) override;
    _Check_return_ HRESULT IsAtInsertionPosition(
        _In_  XUINT32 iTextPosition,
        _Out_ bool  *pfIsAtInsertionPosition
    ) override;
    _Check_return_ HRESULT PixelPositionToTextPosition(
        _In_      XPOINTF      pixelCoordinate,
        _In_      XUINT32      bIncludeNewline,
        _Out_     XUINT32     *piTextPosition,
        _Out_opt_ TextGravity *peGravity
    ) override;
    _Check_return_ HRESULT TextPositionToPixelPosition(
        _In_      XUINT32      iTextPosition,
        _In_      TextGravity  eGravity,
        _Out_opt_ XFLOAT      *pePixelOffset,
        _Out_opt_ XFLOAT      *peCharacterTop,
        _Out_opt_ XFLOAT      *peCharacterHeight,
        _Out_opt_ XFLOAT      *peLineTop,
        _Out_opt_ XFLOAT      *peLineHeight,
        _Out_opt_ XFLOAT      *peLineBaseline,
        _Out_opt_ XFLOAT      *peLineOffset
    ) override;

    XUINT32 GetContentStartPosition() override;
    XUINT32 GetContentLength() override;

    int GetAdjustedPosition(int charIndex) override;
    int GetCharacterIndex(int position) override;

    _Check_return_ HRESULT GetUIScopeForPosition(
        _In_ XUINT32 iTextPosition,
        _In_ TextGravity eGravity,
        _Outptr_ CFrameworkElement **ppUIScope
    ) override;
    _Check_return_ HRESULT ContainsPosition(
        _In_ XUINT32 iTextPosition,
        _In_ TextGravity gravity,
        _Out_ bool *pContains
    ) override;

    static _Check_return_ HRESULT GetBoundsCollectionForElement(
        _In_ ITextView* textView,
        _In_ CTextElement* textElement,
        _Out_ XUINT32* rectangleCount,
        _Outptr_result_buffer_(*rectangleCount) XRECTF** rectangleArray);

private:
    PageNode *m_pPageNode;

    // Gets a page-relative offset from an external offset passed to the page and vice versa.
    // This is necessary because query methods can be called from a linked text view
    // with an arbitrary offset.
    // TransformToPage returns bool because the position may be on the page at all. TransformFromPage
    // is only called for a position on the page, so it will always succeed.
    bool TransformPositionToPage(
        _In_ XUINT32 position,
        _Out_ XUINT32 *pPosition) const;
    XUINT32 TransformPositionFromPage(_In_ XUINT32 position) const;
};



