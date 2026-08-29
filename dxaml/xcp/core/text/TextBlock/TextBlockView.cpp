// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBlockView.h"
#include "BlockNode.h"
#include "TextBounds.h"
#include "FlowDirection.h"
#include "ParagraphTextSource.h"
#include "XamlText.h"
#include "TextBlockViewHelpers.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

// The offset adjustment to use for the start of inlines.  Offsets provided to TextRangeToTextBounds
// should be adjusted by this amount to get the appropriate rects.  Offsets output from other methods should
// already account for this adjustment.
static const XUINT32 PlaceHolderPositionsForInlines = 2;

TextBlockView::TextBlockView(
        _In_ CTextBlock *pTextBlock
    ) :
    m_pTextBlock(pTextBlock)
{
}

// Gets the physical bounds for a character range within the element.
_Check_return_ HRESULT TextBlockView::TextRangeToTextBounds(
    _In_ XUINT32 startOffset,
    _In_ XUINT32 endOffset,
    _Out_ XUINT32 *pcRectangles, 
    _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles
    )
{
    HRESULT hr = S_OK;
    *pcRectangles = 0;
    *ppRectangles = nullptr;
    xvector<RichTextServices::TextBounds> bounds;
    XRECTF *pBounds = nullptr;
    XUINT32 length;
    ASSERT(endOffset >= startOffset);
    length = endOffset - startOffset;
    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        BlockNode *pPageNode = m_pTextBlock->GetPageNode();

        if (!pPageNode->IsMeasureDirty() &&
            !pPageNode->IsArrangeDirty() &&
            length > 0)
        {
            // Snap the position to page bounds. TextBlock is always a single page 
            // which starts at 0, so only the end offset may need adjustment.
            XUINT32 pageLength = pPageNode->GetContentLength();
        
            // Eliminate no-overlap cases.
            // Start offset is inclusive for bounds calculation, so <= check is correct here.
            if (startOffset >= pageLength)
            {
                length = 0;
            }
            else if (endOffset >= pageLength)
            {
                length = pageLength - startOffset;
            }
        
            if (length > 0)
            {
                IFC(pPageNode->GetTextBounds(
                    startOffset,
                    length, 
                    &bounds));
        
                if (bounds.size() > 0)
                {
                    pBounds = new XRECTF[bounds.size()];
                }
        
                RichTextServices::TextBounds textBounds;
                for (XUINT32 i = 0; i < bounds.size(); i++)
                {
                    IFC(bounds.get_item(i, textBounds));
                    pBounds[i] = textBounds.rect;
                }
        
                *ppRectangles = pBounds;
                pBounds = nullptr;
                *pcRectangles = bounds.size();
            }
        }
    }
    else
    {
        ASSERT(m_pTextBlock->GetTextMode() == TextMode::DWriteLayout);
        if (length > 0)
        {
            Microsoft::WRL::ComPtr<IDWriteTextLayout> dwriteTextLayout;
            IFC(m_pTextBlock->GetDWriteTextLayout(&dwriteTextLayout));

            // The length sometimes contains reserved spaces so it could be > 0 for empty TextBlock.
            if (dwriteTextLayout == nullptr)
            {
                // Empty TextBlock
                ASSERT(m_pTextBlock->GetText().GetCount() == 0);
                goto Cleanup;
            }
            
            DWRITE_TEXT_METRICS textMetrics = {};
            IFC(dwriteTextLayout->GetMetrics(&textMetrics));
            XUINT32 lineCount = textMetrics.lineCount;
            XUINT32 maxBidiReorderingDepth = textMetrics.maxBidiReorderingDepth;
            XUINT32 actualHitTestMetricsCount = 0;
            std::vector<DWRITE_HIT_TEST_METRICS> hitTestMetrics(lineCount * maxBidiReorderingDepth);

            if (startOffset >= PlaceHolderPositionsForInlines)
            {
                startOffset = startOffset - PlaceHolderPositionsForInlines;
                endOffset = endOffset - PlaceHolderPositionsForInlines;
            }
            
            hr = dwriteTextLayout->HitTestTextRange(startOffset, endOffset - startOffset, 0, 0, hitTestMetrics.data(), lineCount*maxBidiReorderingDepth, &actualHitTestMetricsCount);
            // Try once more by resizing hitTestMetrics to the actual size before giving up
            if (hr == E_NOT_SUFFICIENT_BUFFER)
            {
                hitTestMetrics.resize(actualHitTestMetricsCount);
                IFC(dwriteTextLayout->HitTestTextRange(startOffset, endOffset - startOffset, 0, 0, hitTestMetrics.data(), actualHitTestMetricsCount, &actualHitTestMetricsCount));
            }
            else
            {
                IFC(hr);
            }
            if (actualHitTestMetricsCount > 0)
            {
                CMILMatrix contentRenderTransform;
                IFC(m_pTextBlock->GetContentRenderTransform(&contentRenderTransform));
                pBounds = new XRECTF[actualHitTestMetricsCount];
                for (XUINT32 i = 0; i < actualHitTestMetricsCount; i++)
                {
                    pBounds[i].X = hitTestMetrics[i].left;
                    pBounds[i].Y = hitTestMetrics[i].top;
                    pBounds[i].Width = hitTestMetrics[i].width;
                    pBounds[i].Height = hitTestMetrics[i].height;
                    // Transform to the screen pixel space.
                    contentRenderTransform.TransformBounds(&pBounds[i], &pBounds[i]);
                }
                *ppRectangles = pBounds;
                 pBounds = nullptr;
                *pcRectangles = actualHitTestMetricsCount;
            }
        }
    }

Cleanup:
    delete[] pBounds;
    RRETURN (hr);
}

// Gets the physical bounds for the range occupied by a TextSelection.
_Check_return_ HRESULT TextBlockView::TextSelectionToTextBounds(
    _In_ IJupiterTextSelection *pSelection,   
    _Out_ XUINT32 *pcRectangles, 
    _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles  
)
{
    *pcRectangles = 0;
    *ppRectangles = nullptr;
    XUINT32 startOffset = 0;
    XUINT32 endOffset = 0;
    CTextPosition startPosition;
    CTextPosition endPosition;

    // Obtain selection start and end offset, and get range bounds from the PageNode.
    IFC_RETURN(pSelection->GetStartTextPosition(&startPosition));
    IFC_RETURN(pSelection->GetEndTextPosition(&endPosition));

    IFC_RETURN(startPosition.GetOffset(&startOffset));
    IFC_RETURN(endPosition.GetOffset(&endOffset));

    IFC_RETURN(TextRangeToTextBounds(startOffset, endOffset, pcRectangles, ppRectangles));

    return S_OK;
}

 _Check_return_ HRESULT TextBlockView::IsAtInsertionPosition(
    _In_  XUINT32 iTextPosition,
    _Out_ bool  *pfIsAtInsertionPosition
)
{
    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        BlockNode *pPageNode = m_pTextBlock->GetPageNode();
        
        if (!pPageNode->IsMeasureDirty() &&
            !pPageNode->IsArrangeDirty() &&
            iTextPosition < pPageNode->GetContentLength())
        {
            IFC_RETURN(pPageNode->IsAtInsertionPosition(
                iTextPosition,
                pfIsAtInsertionPosition)); 
        }
        else
        {
            *pfIsAtInsertionPosition = FALSE;
        }
    }
    else
    {
        XUINT32 textLength = m_pTextBlock->GetText().GetCount();
        if (iTextPosition < textLength + 2*PlaceHolderPositionsForInlines)
        {
            if (iTextPosition < PlaceHolderPositionsForInlines || iTextPosition >= textLength + PlaceHolderPositionsForInlines)
            {
               // For begin and end postions, they are always insertion positions.
               *pfIsAtInsertionPosition = TRUE;
               return S_OK;
            }
            Microsoft::WRL::ComPtr<IDWriteTextLayout> dwriteTextLayout;
            IFC_RETURN(m_pTextBlock->GetDWriteTextLayout(&dwriteTextLayout));
            ASSERT(dwriteTextLayout);

            // Get all the clusters of the text. 
            // TODO: If getting the clusters turns out to be expensive, consider caching it.
            XUINT32 count = 0;
            std::vector<DWRITE_CLUSTER_METRICS> clusters(textLength);
            IFC_RETURN(
                dwriteTextLayout->GetClusterMetrics(
                    clusters.data(),
                    static_cast<UINT32>(clusters.size()),
                    &count
                    )
                );
             // No cluster then just need to check the Surrogate pairs.
            if (count == textLength)
            {
                bool isInSurrogateCRLF = false;
                IFC_RETURN(ParagraphTextSource::IsInSurrogateCRLF(m_pTextBlock->m_pInlines, iTextPosition - PlaceHolderPositionsForInlines, &isInSurrogateCRLF));
                *pfIsAtInsertionPosition = !isInSurrogateCRLF;
            }
            // Has multi-character clusters.
            else
            {
                XUINT32 sum = 0;
                // Add up the clusters' length until the text postion, if they are aligned, then the text postion is not inside a cluster.
                for (XUINT32 i = 0; i < count; i++)
                {
                    if (sum == iTextPosition - PlaceHolderPositionsForInlines)
                    {
                        *pfIsAtInsertionPosition = TRUE;
                        break;
                    }
                    else if (sum > iTextPosition - PlaceHolderPositionsForInlines)
                    {
                        // Text Position is in middle of a cluster, not an insertion position.
                        *pfIsAtInsertionPosition = FALSE;
                        break;
                    }
                    sum += clusters[i].length;
                }
            }
        }
        else
        {
            *pfIsAtInsertionPosition = FALSE;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TextBlockView::PixelPositionToTextPosition(
    _In_ XPOINTF pixelCoordinate, 
    _In_ XUINT32 bIncludeNewline,
    _Out_ XUINT32 *piTextPosition,
    _Out_opt_ TextGravity *peGravity
)
{
    XUINT32 pageLocalPosition = 0;

    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        BlockNode *pPageNode = m_pTextBlock->GetPageNode();
        
        if (!pPageNode->IsMeasureDirty() &&
            !pPageNode->IsArrangeDirty())
        {
            IFC_RETURN(pPageNode->PixelPositionToTextPosition(pixelCoordinate,
                                                       &pageLocalPosition,
                                                       peGravity)); 
        }
    }
    else
    {
        ASSERT(m_pTextBlock->GetTextMode() == TextMode::DWriteLayout);
        if (!m_pTextBlock->GetIsMeasureDirty() && !m_pTextBlock->GetIsArrangeDirty())
        {
            TextGravity gravity = LineForwardCharacterForward;
            Microsoft::WRL::ComPtr<IDWriteTextLayout> dwriteTextLayout;
            IFC_RETURN(m_pTextBlock->GetDWriteTextLayout(&dwriteTextLayout));
            if (dwriteTextLayout == nullptr)
            {
                // Empty TextBlock always return 0.
                ASSERT(m_pTextBlock->GetText().GetCount() == 0);
                *piTextPosition = 0;
                *peGravity = gravity;
                return S_OK;
            }
            CMILMatrix contentRenderTransform;
            IFC_RETURN(m_pTextBlock->GetContentRenderTransform(&contentRenderTransform));

            // We need to transfrom the screen pixel to logical pixel before passing it to DWrite for hit-testing.
            XPOINTF points[1] = {pixelCoordinate};
            if (contentRenderTransform.Invert())
            {
                contentRenderTransform.Transform(points, points, 1);
            }

            BOOL isTrailingHit;
            BOOL isInside;
            DWRITE_HIT_TEST_METRICS hitTestMetrics = {};
            IFC_RETURN(dwriteTextLayout->HitTestPoint(points[0].x, points[0].y, &isTrailingHit, &isInside, &hitTestMetrics)); 
            pageLocalPosition = hitTestMetrics.textPosition + PlaceHolderPositionsForInlines + hitTestMetrics.length - 1  + static_cast<XUINT32>(isTrailingHit);
            if (isTrailingHit)
            {
                DWRITE_TEXT_METRICS textMetrics = {};
                IFC_RETURN(dwriteTextLayout->GetMetrics(&textMetrics));
                XUINT32 actualLineCount = 0;
                std::vector<DWRITE_LINE_METRICS> lineInformation(textMetrics.lineCount);
                IFC_RETURN(dwriteTextLayout->GetLineMetrics(lineInformation.data(), lineInformation.size(), &actualLineCount));

                gravity = CharacterBackward;
                XUINT32 endOfLineCount = 0;
                for (XUINT32 i = 0; i < actualLineCount; i++)
                {
                    endOfLineCount += lineInformation[i].length;
                    // If the postion is at the end of the line, we need to set gravity LineBackward.
                    if (hitTestMetrics.textPosition == endOfLineCount -1)
                    {
                        gravity = LineBackwardCharacterBackward;
                        break;
                    }
                    else if (hitTestMetrics.textPosition < endOfLineCount -1)
                    {
                        break;
                    }
                }
            }
            *peGravity = gravity;
        }
    }

    *piTextPosition = pageLocalPosition;
    return S_OK;
}

XUINT32 TextBlockView::GetContentStartPosition()
{
    return 0;
}

XUINT32 TextBlockView::GetContentLength()
{
    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        return m_pTextBlock->GetPageNode()->GetContentLength();
    }
    else
    {
        return m_pTextBlock->GetText().GetCount() + 2*PlaceHolderPositionsForInlines;
    }
}

int TextBlockView::GetAdjustedPosition(int charIndex)
{
    int charCount = charIndex;
    int adjustedPosition = 0;

    // Correct out of bound indexes to valid ones
    if (charIndex < 0)
    {
        adjustedPosition = PlaceHolderPositionsForInlines;
    }
    else if ((unsigned)charIndex >= (GetContentLength() - (int)PlaceHolderPositionsForInlines))
    {
        adjustedPosition = GetContentLength() - (int)PlaceHolderPositionsForInlines;
    }
    else 
    {
        if (m_pTextBlock->GetTextMode() == TextMode::Normal)
        {
            // Go through each inline to see if the charIndex we're looking for is in it.
            auto inlines = m_pTextBlock->GetInlineCollection();
            VERIFY(TextBlockViewHelpers::AdjustPositionByCharacterCount(inlines, /* Inout */ charCount, /* Inout */ adjustedPosition));
        }
        else
        {
            adjustedPosition = charIndex + PlaceHolderPositionsForInlines;
            
        }
        ASSERT(adjustedPosition <= (int)GetContentLength() - (int)PlaceHolderPositionsForInlines);
    }

    return adjustedPosition;
}

int TextBlockView::GetCharacterIndex(int position)
{
    int positionCount = position;
    int characterIndex = 0;

    // Correct out of bound indexes to valid ones
    if (position < PlaceHolderPositionsForInlines)
    {
        characterIndex = 0;
    }
    else if (position >= (int)GetContentLength())
    {
        characterIndex = GetContentLength() - (int)PlaceHolderPositionsForInlines;
    }
    else
    {
        if (m_pTextBlock->GetTextMode() == TextMode::Normal)
        {
            // Go through each inline to see if the charIndex we're looking for is in it.
            auto inlines = m_pTextBlock->GetInlineCollection();
            VERIFY(TextBlockViewHelpers::AdjustCharacterIndexByPosition(inlines, /* Inout */ characterIndex, /* Inout */ positionCount));
        }
        else
        {
            characterIndex = position - PlaceHolderPositionsForInlines;

        }
        ASSERT(characterIndex <= (int)GetContentLength() - (int)PlaceHolderPositionsForInlines);
    }

    return characterIndex;
}

_Check_return_ HRESULT TextBlockView::TextPositionToPixelPosition(
    _In_      XUINT32      iTextPosition,
    _In_      TextGravity  eGravity,
    _Out_opt_ XFLOAT      *pePixelOffset,      // Relative to origin of line
    _Out_opt_ XFLOAT      *peCharacterTop,     // Relative to TextView top
    _Out_opt_ XFLOAT      *peCharacterHeight,
    _Out_opt_ XFLOAT      *peLineTop,          // Relative to TextView top
    _Out_opt_ XFLOAT      *peLineHeight,
    _Out_opt_ XFLOAT      *peLineBaseline,
    _Out_opt_ XFLOAT      *peLineOffset        // Padding and alignment offset
)
{
    XUINT32 startOffset = 0;
    XUINT32 length = 1;
    xvector<RichTextServices::TextBounds> bounds;
    TextGravity gravity = eGravity;

    if (pePixelOffset)
    {
        *pePixelOffset = 0;
    }
    if (peCharacterTop)
    {
        *peCharacterTop = 0;
    }
    if (peCharacterHeight)
    {
        *peCharacterHeight = 0;
    }
    if (peLineTop)
    {
        *peLineTop = 0;
    }
    if (peLineHeight)
    {
        *peLineHeight = 0;
    }
    if (peLineBaseline)
    {
        *peLineBaseline = 0;
    }
    if (peLineOffset)
    {
        *peLineOffset = 0;
    }

    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        BlockNode *pPageNode = m_pTextBlock->GetPageNode();
        // If the page has no break and the position is the last position on the page, it corresponds to the end of
        // the text container. In this case the view is considered to contain it. For pixel position, treat it as though it
        // has backward gravity, i.e. is the trailing edge of the last position on the page.
        if (iTextPosition == pPageNode->GetContentLength() &&
            pPageNode->GetBreak() == nullptr && 
            !(gravity & CharacterBackward))
        {
            gravity = LineForwardCharacterBackward;
        }
        
        if (gravity & CharacterBackward &&
            iTextPosition > 0)
        {
            startOffset = iTextPosition - 1;
        }
        else
        {
            startOffset = iTextPosition;
        }
        
        if (!pPageNode->IsMeasureDirty() &&
            !pPageNode->IsArrangeDirty() && 
            startOffset < pPageNode->GetContentLength())
        {        
            IFC_RETURN(pPageNode->GetTextBounds(
                startOffset,
                length, 
                &bounds));
            
            if (bounds.size() > 0)
            {
                ASSERT(bounds.size() == 1);
        
                if (pePixelOffset)
                {
                    // Use values from first bounds rect.
                    *pePixelOffset = bounds[0].rect.X;
        
                    if (((gravity & CharacterBackward) && bounds[0].flowDirection == pPageNode->GetFlowDirection())
                        || (((gravity & CharacterBackward) == 0) && bounds[0].flowDirection != pPageNode->GetFlowDirection()))
                    {
                        *pePixelOffset += bounds[0].rect.Width;
                    }
                }
        
                if (peLineTop)
                {
                    *peLineTop = bounds[0].rect.Y;
                }
        
                if (peLineHeight)
                {
                    *peLineHeight = bounds[0].rect.Height;
                }       
            }        
        } // Else: If the position is not on the page, don't return anything. Callers should check Contains.    
    }
    else
    {
        Microsoft::WRL::ComPtr<IDWriteTextLayout> dwriteTextLayout;
        IFC_RETURN(m_pTextBlock->GetDWriteTextLayout(&dwriteTextLayout));

        if (dwriteTextLayout == nullptr)
        {
            // Empty TextBlock always return 0.
            ASSERT(m_pTextBlock->GetText().GetCount() == 0);
            return S_OK;
        }

        DWRITE_HIT_TEST_METRICS hitTestMetrics = {};
        if (iTextPosition >= PlaceHolderPositionsForInlines)
        {
            iTextPosition = iTextPosition - PlaceHolderPositionsForInlines;
        }
        if ((gravity & CharacterBackward) && iTextPosition > 0)
        {
            --iTextPosition;
        }
        
        XPOINTF logicalCoordinate;
        IFC_RETURN(dwriteTextLayout->HitTestTextPosition(iTextPosition, false, &logicalCoordinate.x, &logicalCoordinate.y, &hitTestMetrics)); 

        if(gravity & CharacterBackward)
        {
            // When character gravity backward, we need to include the character width to show the selection grippers at the correct place.
            if (hitTestMetrics.bidiLevel % 2 == 0) 
            {
                logicalCoordinate.x += hitTestMetrics.width;
            }
            else
            {
                logicalCoordinate.x -= hitTestMetrics.width;
            }
        }

        CMILMatrix contentRenderTransform;
        IFC_RETURN(m_pTextBlock->GetContentRenderTransform(&contentRenderTransform));
        // We need to transfrom the logical pixel to screen pixel
        XPOINTF points[1] = {logicalCoordinate};
        contentRenderTransform.Transform(points, points, 1);
        *pePixelOffset = points[0].x;
        *peLineTop = points[0].y;
        *peLineHeight = hitTestMetrics.height;
    }
    return S_OK;
}

_Check_return_ HRESULT TextBlockView::GetUIScopeForPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity eGravity,
    _Outptr_ CFrameworkElement **ppUIScope
)
{
    // For TextBlock, it's OK to always return the TextBlock as the UIScope. Even if the
    // position is not part of formatted content, its character rect, etc. will be empty/nonexistent
    // but the TextBlock can be considered its visual parent always.
    *ppUIScope = m_pTextBlock;
    return S_OK;
}

_Check_return_ HRESULT TextBlockView::ContainsPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity gravity,
    _Out_ bool *pContains
    )
{
    XUINT32 position = 0;
    *pContains = FALSE;

    if (m_pTextBlock->GetTextMode() == TextMode::Normal)
    {
        BlockNode *pPageNode = m_pTextBlock->GetPageNode();
        // If the page has no break and the position is the last position on the page, it corresponds to the end of
        // the text container. In this case the view is considered to contain it. For Contains, treat it as though it
        // has backward gravity, i.e. is the trailing edge of the last position on the page.
        if (iTextPosition == pPageNode->GetContentLength() &&
            pPageNode->GetBreak() == nullptr && 
            !(gravity & CharacterBackward))
        {
            gravity = LineForwardCharacterBackward;
        }
        
        if (gravity & CharacterBackward &&
            iTextPosition > 0)
        {
            position = iTextPosition - 1;
        }
        else
        {
            position = iTextPosition;
        }
        
        if (!pPageNode->IsMeasureDirty() &&
            !pPageNode->IsArrangeDirty() &&
            position < pPageNode->GetContentLength())
        {
            *pContains =  TRUE;
        }
    }
    else
    {
        // If  the position is the last position, it corresponds to the end of
        // the text container. In this case the view is considered to contain it. For Contains, treat it as though it
        // has backward gravity, i.e. is the trailing edge of the last position on the page.
        if (iTextPosition == m_pTextBlock->GetText().GetCount() + 2*PlaceHolderPositionsForInlines &&
            !(gravity & CharacterBackward))
        {
            gravity = LineForwardCharacterBackward;
        }
        
        if (gravity & CharacterBackward &&
            iTextPosition > 0)
        {
            position = iTextPosition - 1;
        }
        else
        {
            position = iTextPosition;
        }

        // Note measure/arrange may be dirty here due to the delayed Inline creation. 
        if (position < m_pTextBlock->GetText().GetCount() + 2*PlaceHolderPositionsForInlines)
        {
            *pContains =  TRUE;
        }
    }

    return S_OK;
}
