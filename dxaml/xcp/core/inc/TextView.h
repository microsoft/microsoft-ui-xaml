// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef TEXT_VIEW_H
#define TEXT_VIEW_H

struct HWTextRenderParams;
class D2DTextDrawingContext;
struct IJupiterTextSelection;

//------------------------------------------------------------------------
//
//  Interface:  ITextView
//
//  Query interface used to access layout results for text display
//  elements.
//
//------------------------------------------------------------------------
struct ITextView
{
    virtual _Check_return_ HRESULT TextRangeToTextBounds(
        _In_ XUINT32 startOffset,
        _In_ XUINT32 endOffset,
        _Out_ XUINT32 *pcRectangles,
        _Out_writes_(*pcRectangles) XRECTF **ppRectangles
    ) = 0;

    virtual _Check_return_ HRESULT TextSelectionToTextBounds(
        _In_                        IJupiterTextSelection  *pSelection,   // Range of text that we want bounding rectangles for.
        _Out_                       XUINT32         *pcRectangles, // The size of array of bounding rectangles.
        _Outptr_result_buffer_(*pcRectangles) XRECTF   **ppRectangles  // An array of bounding rectangles that represent the range of characters within the selection.
    ) = 0;

    virtual _Check_return_ HRESULT IsAtInsertionPosition(
        _In_  XUINT32 iTextPosition,
        _Out_ bool  *pfIsAtInsertionPosition
    ) = 0;

    virtual _Check_return_ HRESULT PixelPositionToTextPosition(
        _In_      XPOINTF      pixelCoordinate,
        _In_      XUINT32      bIncludeNewline,
        _Out_     XUINT32     *piTextPosition,
        _Out_opt_ TextGravity *peGravity
    ) = 0;

    virtual _Check_return_ HRESULT TextPositionToPixelPosition(
        _In_      XUINT32      iTextPosition,
        _In_      TextGravity  eGravity,
        _Out_opt_ XFLOAT      *pePixelOffset,      // Relative to origin of line
        _Out_opt_ XFLOAT      *peCharacterTop,     // Relative to TextView top
        _Out_opt_ XFLOAT      *peCharacterHeight,
        _Out_opt_ XFLOAT      *peLineTop,          // Relative to TextView top
        _Out_opt_ XFLOAT      *peLineHeight,
        _Out_opt_ XFLOAT      *peLineBaseline,
        _Out_opt_ XFLOAT      *peLineOffset        // Padding and alignment offset
    ) = 0;

    virtual XUINT32 GetContentStartPosition() = 0;

    virtual XUINT32 GetContentLength() = 0;

    virtual int GetAdjustedPosition(int charIndex) = 0;
    virtual int GetCharacterIndex(int position) = 0;

    virtual _Check_return_ HRESULT GetUIScopeForPosition(
        _In_ XUINT32 iTextPosition,
        _In_ TextGravity eGravity,
        _Outptr_ CFrameworkElement **ppUIScope
    ) = 0;

    virtual _Check_return_ HRESULT ContainsPosition(
        _In_ XUINT32 iTextPosition,
        _In_ TextGravity gravity,
        _Out_ bool *pContains
    ) = 0;
};

#endif
