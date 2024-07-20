// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <corep.h>
#include <depends.h>
#include <DOCollection.h>
#include <framework.h>
#include <ContentRenderer.h>
#include <hwwalk.h>
#include <TextBoxHelpers.h>
#include <TextView.h>
#include <matrix.h>
#include <compositortree.h>
#include <rendertypes.h>
#include <SolidColorBrush.h>
#include <TextRange.h>
#include <TextRangeCollection.h>
#include <TextHighlighter.h>
#include <TextHighlighterCollection.h>
#include <TextHighlightMerge.h>
#include "TextHighlightRenderer.h"
#include "RichTextBlockView.h"
#include "HighlightRegion.h"

namespace
{
    using IterateHighlighterCallback =
        std::function<
            _Check_return_ HRESULT(
                CSolidColorBrush* /* foregroundBrush */,
                CSolidColorBrush* /* backgroundBrush */,
                uint32_t /* highlightRectCount */,
                XRECTF* /* highlightRects */
            )>;

    _Check_return_ HRESULT
    GetDefaultHighlighterBrushes(
        _In_ CCoreServices* coreServices,
        _Outref_ xref_ptr<CSolidColorBrush>& foregroundBrush,
        _Outref_ xref_ptr<CSolidColorBrush>& backgroundBrush
        )
    {
        xref_ptr<CDependencyObject> defaultForegroundBrushDO;
        IFC_RETURN(coreServices->LookupThemeResource(
            XSTRING_PTR_EPHEMERAL(L"TextControlHighlighterForeground"),
            defaultForegroundBrushDO.ReleaseAndGetAddressOf()));
        ASSERT(defaultForegroundBrushDO != nullptr);
        defaultForegroundBrushDO->SimulateFreeze();
        foregroundBrush = static_sp_cast<CSolidColorBrush>(std::move(defaultForegroundBrushDO));

        xref_ptr<CDependencyObject> defaultBackgroundBrushDO;
        IFC_RETURN(coreServices->LookupThemeResource(
            XSTRING_PTR_EPHEMERAL(L"TextControlHighlighterBackground"),
            defaultBackgroundBrushDO.ReleaseAndGetAddressOf()));
        ASSERT(defaultBackgroundBrushDO != nullptr);
        defaultBackgroundBrushDO->SimulateFreeze();
        backgroundBrush = static_sp_cast<CSolidColorBrush>(std::move(defaultBackgroundBrushDO));

        return S_OK;
    }

    _Check_return_ HRESULT
    IterateMergedHighlighters(
        _In_ CCoreServices* coreServices,
        _In_ CTextHighlighterCollection* textHighlighters,
        _In_ std::vector<HighlightRegion>& textSelections,
        _In_ ITextView* textView,
        IterateHighlighterCallback callback
        )
    {
#if DBG
        auto textLength = static_cast<int>(textView->GetContentLength());
#endif

        // Merge all the highlighters so that there is no overlap in ranges and
        // earlier items in the collection get overwritten by later ones.
        TextHighlightMerge merge;

        if (textHighlighters)
        {
            xref_ptr<CSolidColorBrush> defaultForegroundBrush;
            xref_ptr<CSolidColorBrush> defaultBackgroundBrush;
            IFC_RETURN(GetDefaultHighlighterBrushes(
                coreServices,
                defaultForegroundBrush,
                defaultBackgroundBrush));

            for (const auto& textHighlighterDO : textHighlighters->GetCollection())
            {
                auto textHighlighter = do_pointer_cast<CTextHighlighter>(textHighlighterDO);
                ASSERT(textHighlighter);

                CSolidColorBrush* highlightForegroundBrush;
                if (textHighlighter->m_foreground)
                {
                    highlightForegroundBrush = do_pointer_cast<CSolidColorBrush>(textHighlighter->m_foreground);
                }
                else
                {
                    highlightForegroundBrush = defaultForegroundBrush;
                }

                CSolidColorBrush* highlightBackgroundBrush;
                if (textHighlighter->m_background)
                {
                    highlightBackgroundBrush = do_pointer_cast<CSolidColorBrush>(textHighlighter->m_background);
                }
                else
                {
                    highlightBackgroundBrush = defaultBackgroundBrush;
                }

                ASSERT(textHighlighter->GetRanges());
                for (const auto& textRange : textHighlighter->GetRanges()->GetCollection())
                {
                    // Resolve the highlight rects

                    // Adjust for content offsets in each inline and paragraph. This is also accounted for in textLength.
                    int startOffset = textView->GetAdjustedPosition(textRange.startIndex);
                    int endOffset = textView->GetAdjustedPosition(textRange.startIndex + textRange.length) - 1;
                    ASSERT((endOffset - startOffset) < textLength);

                    // Only highlight valid ranges
                    if ((startOffset >= 0) &&
                        (endOffset >= 0) &&
                        (startOffset <= endOffset))
                    {
                        merge.AddRegion(
                            std::make_shared<HighlightRegion>(
                                startOffset,
                                endOffset,
                                highlightForegroundBrush,
                                highlightBackgroundBrush));
                    }
                }
            }
        }

        if (!textSelections.empty())
        {
            for (HighlightRegion selection : textSelections)
            {
                if (selection.startIndex >= 0 &&
                    selection.endIndex >= 0 &&
                    selection.startIndex <= selection.endIndex)
                {
                    merge.AddRegion(
                        std::make_shared<HighlightRegion>(
                            selection.startIndex,
                            selection.endIndex,
                            selection.foregroundBrush,
                            selection.backgroundBrush));
                }
            }
        }

        // Iterate over the merged collection
        // Note that the merge algorithm works with inclusive ranges [Start,End]
        // Whereas TextRangeToTextBounds works on [Start,End), so 1 is incremented
        // to the endIndex to compensate.  Additionally, endOffset was appropriately adjusted
        // based on the length of the range when the regions were added to the merge algorithm.
        for (const auto& mergeMapItem : merge)
        {
            auto highlightRegion = mergeMapItem.second.get();

            // Get the highlight rects
            uint32_t rectangleCount = 0;
            XRECTF* rectanglesRaw = nullptr;
            IFC_RETURN(textView->TextRangeToTextBounds(
                highlightRegion->startIndex,
                highlightRegion->endIndex + 1,
                &rectangleCount,
                &rectanglesRaw));
            std::unique_ptr<XRECTF[]> rectangles(rectanglesRaw);

            IFC_RETURN(callback(
                highlightRegion->foregroundBrush,
                highlightRegion->backgroundBrush,
                rectangleCount,
                rectangles.get()));
        }

        return S_OK;
    }
}

_Check_return_ HRESULT
TextHighlightRenderer::HWRenderCollection(
    _In_ CCoreServices* coreServices,
    _In_ CTextHighlighterCollection* textHighlighters,
    _In_ std::vector<HighlightRegion>& textSelections,
    _In_ ITextView* textView,
    _In_ IContentRenderer* contentRenderer,
    TextHighlightRenderer::ForegroundRenderingCallback foregroundRenderingCallback
    )
{
    if (((textHighlighters != nullptr && textHighlighters->GetCount() > 0) || 
        (!textSelections.empty())) &&
        (textView != nullptr))
    {
        IFC_RETURN(IterateMergedHighlighters(
            coreServices,
            textHighlighters,
            textSelections,
            textView,
            [contentRenderer, foregroundRenderingCallback]
            (CSolidColorBrush* foregroundBrush,
             CSolidColorBrush* backgroundBrush,
             uint32_t highlightRectCount,
             XRECTF* highlightRects)
        {
            IFC_RETURN(HWHighlightRect(
                contentRenderer,
                backgroundBrush,
                highlightRectCount,
                highlightRects));

            IFC_RETURN(foregroundRenderingCallback(
                foregroundBrush,
                highlightRectCount,
                highlightRects));

            return S_OK;
        }));
    }

    return S_OK;
}

_Check_return_ HRESULT
TextHighlightRenderer::HWHighlightRect(
    _In_ IContentRenderer* contentRenderer,
    _In_ CSolidColorBrush* highlightBrush,
    uint32_t numHighlightRects,
    _In_reads_(numHighlightRects) XRECTF* highlightRects
    )
{
    CMILMatrix renderParamTransform;
    contentRenderer->GetRenderParams()->pTransformsAndClipsToCompNode->Get2DTransformInLeafmostProjection(&renderParamTransform);
    auto allowsSnapToPixels = CTextBoxHelpers::TransformAllowsSnapToPixels(&renderParamTransform);

    for (uint32_t i = 0; i < numHighlightRects; i++)
    {
        XRECTF rect = highlightRects[i];

        // Check the applicable RenderTransform to see if it's practical to try to snap to pixels.
        if (allowsSnapToPixels)
        {
            IFC_RETURN(CTextBoxHelpers::SnapRectToPixel(&renderParamTransform, 0, SelectionRectangle, &rect));
        }

        IFC_RETURN(contentRenderer->RenderSolidColorRectangle(
            rect,
            highlightBrush));
    }

    return S_OK;
}
