// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockView.h"
#include "PageNode.h"
#include "TextBounds.h"
#include "FlowDirection.h"
#include "XamlText.h"
#include "TextBlockViewHelpers.h"

// The offset adjustment to use for the start of each inline.  Offsets provided to TextRangeToTextBounds
// should be adjusted by this amount to get the appropriate rects.  Offsets output from other methods should
// already account for this adjustment.
static const int PlaceHolderPositionsForInlines = 2;

RichTextBlockView::RichTextBlockView(
    _In_ PageNode *pPageNode
    ) :
    m_pPageNode(pPageNode)
{
}

_Check_return_ HRESULT RichTextBlockView::TextRangeToTextBounds(
    _In_ XUINT32 startOffset,
    _In_ XUINT32 endOffset,
    _Out_ XUINT32 *pcRectangles,
    _Outptr_result_buffer_(*pcRectangles) XRECTF **ppRectangles
    )
{
    HRESULT hr = S_OK;
    *pcRectangles = 0;
    *ppRectangles = nullptr;
    XUINT32 length;
    xvector<RichTextServices::TextBounds> bounds;
    XRECTF *pBounds = nullptr;

    ASSERT(endOffset >= startOffset);
    length = endOffset - startOffset;

    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty() &&
        length > 0)
    {
        // Snap the position to page bounds.
        XUINT32 startInPage = 0;
        XUINT32 pageStart = m_pPageNode->GetStartPosition();
        XUINT32 pageLength = m_pPageNode->GetContentLength();

        // Eliminate no-overlap cases.
        // Start offset is inclusive for bounds calculation, so <= check is correct here.
        if ((startOffset + length) <= pageStart ||
            startOffset >= (pageStart + pageLength))
        {
            length = 0;
        }
        else
        {
            if (startOffset < pageStart)
            {
                startInPage = 0;
                length -= (pageStart - startOffset);
            }
            else
            {
                startInPage = startOffset - pageStart;
            }
            length = MIN(length, pageLength - startInPage);
        }

        if (length > 0)
        {
            IFC(m_pPageNode->GetTextBounds(
                startInPage,
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
        else
        {
            *pcRectangles = 0;
            *ppRectangles = nullptr;
        }
    }
    else
    {
        *pcRectangles = 0;
        *ppRectangles = nullptr;
    }

Cleanup:
    delete[] pBounds;
    return hr;
}

// Gets the physical bounds for the range occupied by a TextSelection.
_Check_return_ HRESULT RichTextBlockView::TextSelectionToTextBounds(
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

_Check_return_ HRESULT RichTextBlockView::IsAtInsertionPosition(
    _In_  XUINT32 iTextPosition,
    _Out_ bool  *pfIsAtInsertionPosition
)
{
    XUINT32 pageLocalPosition = 0;

    // Other RichTextBlockView APIs adjust for gravity to match the page node. IsAtInsertionPosition does not because
    // if it's not within the page's content it's OK to return false and also because there's no concept of Insertion
    // for RichTextBlock.
    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        if (TransformPositionToPage(iTextPosition, &pageLocalPosition))
        {
            IFC_RETURN(m_pPageNode->IsAtInsertionPosition(
                pageLocalPosition,
                pfIsAtInsertionPosition));
        }
        else
        {
            *pfIsAtInsertionPosition = FALSE;
        }
    }
    else
    {
        *pfIsAtInsertionPosition = FALSE;
    }

    return S_OK;
}

_Check_return_ HRESULT RichTextBlockView::PixelPositionToTextPosition(
    _In_ XPOINTF pixelCoordinate,
    _In_ XUINT32 bIncludeNewline,
    _Out_ XUINT32 *piTextPosition,
    _Out_opt_ TextGravity *peGravity
)
{
    XUINT32 pageLocalPosition = 0;

    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        IFC_RETURN(m_pPageNode->PixelPositionToTextPosition(pixelCoordinate,
                                                     &pageLocalPosition,
                                                     peGravity));
        pageLocalPosition = TransformPositionFromPage(pageLocalPosition);
    }

    *piTextPosition = pageLocalPosition;

    return S_OK;
}

_Check_return_ HRESULT RichTextBlockView::TextPositionToPixelPosition(
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
    XUINT32 pageLocalPosition = 0;
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

    // If the page has no break and the position is the last position on the page, it corresponds to the end of
    // the text container. In this case the view is considered to contain it. For pixel position, treat it as though it
    // has backward gravity, i.e. is the trailing edge of the last position on the page.
    if (iTextPosition == (m_pPageNode->GetStartPosition() + m_pPageNode->GetContentLength()) &&
        m_pPageNode->GetBreak() == nullptr &&
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

    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        if (TransformPositionToPage(startOffset, &pageLocalPosition))
        {
            IFC_RETURN(m_pPageNode->GetTextBounds(
                pageLocalPosition,
                length,
                &bounds));

            if (bounds.size() > 0)
            {
                ASSERT(bounds.size() == 1);

                if (pePixelOffset)
                {
                    // Use values from first bounds rect.
                    *pePixelOffset = bounds[0].rect.X;

                    if (((gravity & CharacterBackward) && bounds[0].flowDirection == m_pPageNode->GetFlowDirection())
                        || (((gravity & CharacterBackward) == 0) && bounds[0].flowDirection != m_pPageNode->GetFlowDirection()))
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

    return S_OK;
}

_Check_return_ HRESULT RichTextBlockView::GetUIScopeForPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity eGravity,
    _Outptr_ CFrameworkElement **ppUIScope
)
{
    XUINT32 pageLocalPosition = 0;
    XUINT32 startOffset = 0;
    TextGravity gravity = eGravity;

    // If the page has no break and the position is the last position on the page, it corresponds to the end of
    // the text container. In this case the view is considered to contain it. For GetUIScope, treat it as though it
    // has backward gravity, i.e. is the trailing edge of the last position on the page.
    if (iTextPosition == (m_pPageNode->GetStartPosition() + m_pPageNode->GetContentLength()) &&
        m_pPageNode->GetBreak() == nullptr &&
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

    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        if (TransformPositionToPage(startOffset, &pageLocalPosition))
        {
            *ppUIScope = m_pPageNode->GetPageOwner();
        }
        else
        {
            // This may be called for a position at the end of the block collection which would be just outside the page.
            *ppUIScope = nullptr;
        }
    }
    else
    {
        *ppUIScope = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT RichTextBlockView::ContainsPosition(
    _In_ XUINT32 iTextPosition,
    _In_ TextGravity gravity,
    _Out_ bool *pContains
    )
{
    XUINT32 pageLocalPosition;
    XUINT32 position = 0;
    *pContains = FALSE;

    // If the page has no break and the position is the last position on the page, it corresponds to the end of
    // the text container. In this case the view is considered to contain it. For Contains, treat it as though it
    // has backward gravity, i.e. is the trailing edge of the last position on the page.
    if (iTextPosition == (m_pPageNode->GetStartPosition() + m_pPageNode->GetContentLength()) &&
        m_pPageNode->GetBreak() == nullptr &&
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

    if (!m_pPageNode->IsMeasureDirty() &&
        !m_pPageNode->IsArrangeDirty())
    {
        *pContains =  TransformPositionToPage(position, &pageLocalPosition);
    }

    return S_OK;
}

XUINT32 RichTextBlockView::GetContentStartPosition()
{
    return m_pPageNode->GetStartPosition();
}

XUINT32 RichTextBlockView::GetContentLength()
{
    return m_pPageNode->GetContentLength();
}

int RichTextBlockView::GetAdjustedPosition(int charIndex)
{
    int charCount = charIndex;
    int adjustedPosition = 0;

    CRichTextBlock *owningRichTextBlock;

    if (m_pPageNode->GetPageOwner()->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
    {
        owningRichTextBlock = static_cast<CRichTextBlock*>(m_pPageNode->GetPageOwner());
    }
    else
    {
        CRichTextBlockOverflow *owningOverflow = static_cast<CRichTextBlockOverflow*>(m_pPageNode->GetPageOwner());
        owningRichTextBlock = static_cast<CRichTextBlock*>(owningOverflow->GetMaster());
    }

    // Save the starting and ending positions of the specific RTB or RTBO currently being highlighted.
    int blockStartPosition = GetContentStartPosition();
    int blockEndPosition = blockStartPosition + m_pPageNode->GetContentLength();

    // Correct out of bound indexes to valid ones
    if (charIndex < 0)
    {
        adjustedPosition = PlaceHolderPositionsForInlines;
    }
    else
    {
        CDependencyObject *previousBlock = nullptr;
        auto blockCollection = owningRichTextBlock->m_pBlocks->GetCollection();
        for (auto block : blockCollection)
        {
            // Account for the offset at the end of each paragraph before searching the next one
            if (previousBlock)
            {
                adjustedPosition += PlaceHolderPositionsForInlines;
            }
            // Go through each inline to see if the position we're looking for is in it
            auto inlines = do_pointer_cast<CParagraph>(block)->GetInlineCollection();
            bool adjustedPositionFound = TextBlockViewHelpers::AdjustPositionByCharacterCount(inlines, /* Inout */ charCount, /* Inout */ adjustedPosition);
            if (adjustedPositionFound)
            {
                // Don't need to search through any other paragraphs
                break;
            }
            previousBlock = block;
        }
    }

    // Since specified bounds may be outside of the RTB/RTBO that is
    // currently being rendered, snap to those limits.
    // If both bounds are outside of the RTB/RTBO, no highlight will
    // render since the endOffset is decremented after it is returned,
    // and will be smaller than the startOffset.
    if (adjustedPosition < blockStartPosition)
    {
        adjustedPosition = blockStartPosition;
    }
    if (adjustedPosition > blockEndPosition)
    {
        adjustedPosition = blockEndPosition;
    }

    return adjustedPosition;
}

int RichTextBlockView::GetCharacterIndex(int position)
{
    int adjustedPosition = position;
    int charIndex = 0;

    CRichTextBlock *owningRichTextBlock;

    if (m_pPageNode->GetPageOwner()->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
    {
        owningRichTextBlock = static_cast<CRichTextBlock*>(m_pPageNode->GetPageOwner());
    }
    else
    {
        CRichTextBlockOverflow *owningOverflow = static_cast<CRichTextBlockOverflow*>(m_pPageNode->GetPageOwner());
        owningRichTextBlock = static_cast<CRichTextBlock*>(owningOverflow->GetMaster());
    }

    // Correct out of bound indexes to valid ones
    if (position < 0)
    {
        charIndex = 0;
    }
    else
    {
        CDependencyObject *previousBlock = nullptr;
        auto blockCollection = owningRichTextBlock->m_pBlocks->GetCollection();
        for (auto block : blockCollection)
        {
            // Account for the offset at the end of each paragraph before searching the next one
            if (previousBlock)
            {
                adjustedPosition -= PlaceHolderPositionsForInlines;
            }
            // Go through each inline to see if the position we're looking for is in it
            auto inlines = static_cast<CParagraph*>(block)->GetInlineCollection();
            bool charIndexFound = TextBlockViewHelpers::AdjustCharacterIndexByPosition(inlines, /* Inout */ charIndex, /* Inout */ adjustedPosition);
            if (charIndexFound)
            {
                // Don't need to search through any other paragraphs
                break;
            }
            previousBlock = block;
        }
    }

    return charIndex;
}

bool RichTextBlockView::TransformPositionToPage(
    _In_ XUINT32 position,
    _Out_ XUINT32 *pPosition
    ) const
{
    XUINT32 pageLocalPosition;
    XUINT32 pageStart = m_pPageNode->GetStartPosition();

    if (position >= pageStart)
    {
        pageLocalPosition = position - pageStart;
        if (pageLocalPosition < m_pPageNode->GetContentLength())
        {
            *pPosition = pageLocalPosition;
            return true;
        }
    }
    return false;
}

XUINT32 RichTextBlockView::TransformPositionFromPage(
    _In_ XUINT32 position
    ) const
{
    XUINT32 pageStart = m_pPageNode->GetStartPosition();
    return (position + pageStart);
}

/*static*/
_Check_return_ HRESULT RichTextBlockView::GetBoundsCollectionForElement(
    _In_ ITextView* textView,
    _In_ CTextElement* textElement,
    _Out_ XUINT32* rectangleCount,
    _Outptr_result_buffer_(*rectangleCount) XRECTF** rectangleArray)
{
    *rectangleCount = 0;
    *rectangleArray = nullptr;
    xref_ptr<CTextPointerWrapper> contentStart;
    xref_ptr<CTextPointerWrapper> contentEnd;
    XINT32 contentStartOffset;
    XINT32 contentEndOffset;

    IFC_RETURN(textElement->GetContentStart(contentStart.ReleaseAndGetAddressOf()));
    IFC_RETURN(textElement->GetContentEnd(contentEnd.ReleaseAndGetAddressOf()));
    IFC_RETURN(contentStart->GetOffset(&contentStartOffset));
    IFC_RETURN(contentEnd->GetOffset(&contentEndOffset));

    IFC_RETURN(textView->TextRangeToTextBounds(
        contentStartOffset,
        contentEndOffset,
        rectangleCount,
        rectangleArray));

    return S_OK;
}

