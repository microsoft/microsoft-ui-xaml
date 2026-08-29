// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct ITextView;
class IContentRenderer;
class CCoreServices;
class CMILMatrix;
class CSolidColorBrush;
class CTextHighlighterCollection;
class CTextRange;
class HighlightRegion;

namespace TextHighlightRenderer
{
    // HWRenderCollection handles rendering the background implicitly.  It could almost
    // do the same for foreground except that there are two very similar but different methods for adding the
    // foreground highlight rects to either D2DTextDrawingContext or DrawingContext.  A callback is provided
    // instead to give the caller the opportunity to draw the foreground.
    // This offers the best of some bad choices.  It could handle itself internally but it would have to overload
    // and provide a lot of different functions.  It could attempt to do background rendering as well, but that has
    // two different methods.
    using ForegroundRenderingCallback =
        std::function<
            _Check_return_ HRESULT(
                CSolidColorBrush* /* foregroundBrush */,
                uint32_t /* highlightRectCount */,
                XRECTF* /* highlightRects */
            )>;

    _Check_return_ HRESULT HWRenderCollection(
        _In_ CCoreServices* coreServices,
        _In_ CTextHighlighterCollection* textHighlighterCollection,
        _In_ std::vector<HighlightRegion>& textSelections,
        _In_ ITextView* textView,
        _In_ IContentRenderer* contentRenderer,
        ForegroundRenderingCallback foregroundRenderingCallback
        );

    // TODO: Change TextSelectionManager to use HWHighlightRect
    _Check_return_ HRESULT HWHighlightRect(
        _In_ IContentRenderer* contentRenderer,
        _In_ CSolidColorBrush* highlightBrush,
        uint32_t highlightRectCount,
        _In_reads_(highlightRectCount) XRECTF* highlightRects
        );
};