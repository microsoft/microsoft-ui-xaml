// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParagraphNode.h"
#include "BlockLayoutEngine.h"
#include "BlockLayoutHelpers.h"
#include "ParagraphDrawingContext.h"
#include "ParagraphNodeBreak.h"
#include "PageNode.h"

using namespace DirectUI;
using namespace RichTextServices;

template class DynamicArray<LineMetrics>;

//---------------------------------------------------------------------------
//
// ParagraphNode::ParagraphNode
//
//---------------------------------------------------------------------------
ParagraphNode::ParagraphNode(
    _In_ BlockLayoutEngine *pBlockLayoutEngine,
    _In_opt_ CParagraph *pParagraph,
    _In_opt_ ContainerNode *pParentNode
    ) : BlockNode(pBlockLayoutEngine, pParagraph, pParentNode),
    m_textSource(pParagraph, do_pointer_cast<CFrameworkElement>(pBlockLayoutEngine->GetOwner())),
    m_pTextRunCache(NULL),
    m_pPageNode(NULL),
    m_untrimmedDesiredWidth(0)
{
    m_textSource.SetParagraphNode(this);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::~ParagraphNode
//
//---------------------------------------------------------------------------
ParagraphNode::~ParagraphNode()
{
    ReleaseInterface(m_pTextRunCache);
    DeleteLineCache();
    RemoveEmbeddedElements();
    delete m_pDrawingContext;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::GetBaselineAlignmentOffset
//
//---------------------------------------------------------------------------
XFLOAT ParagraphNode::GetBaselineAlignmentOffset() const
{
    XFLOAT baselineOffset = GetContentRenderingOffset().y;
    if (!IsMeasureDirty() && m_lines.GetCount() > 0)
    {
        baselineOffset = m_lines[0].BaselineOffset;
    }
    return baselineOffset;
}

//---------------------------------------------------------------------------
//
// PageNode::AddElement
//
//  Synopsis:
//      Adds the given embedded element to the host, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::AddElement(
    _In_ CInlineUIContainer *pContainer
    )
{
    if (m_pPageNode == NULL)
    {
        // TODO: Lookup IEmbeddedElementHost from parent in better way, this only works for non-nested Paragraph.
        m_pPageNode = static_cast<PageNode *>(m_pParentNode);
        ASSERT(m_pPageNode != NULL);
    }

    // Add the element to the page node, giving this ParagraphNode as context
    // so if the node is deleted, etc. the PageNode will be able to find elements associated with it.
    IFC_RETURN(m_pPageNode->AddElement(pContainer, this));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::CanAddElement
//
//  Synopsis:
//      Gets a value indicating whether the host can support adding elements,
//      IEmbeddedElementHost override. Should be queried before calling
//      AddElement. The value may depend on the state of the host, e.g.
//      ParagraphNode only permits elements to be added during Measure.
//
//---------------------------------------------------------------------------
bool ParagraphNode::CanAddElement() const
{
    return IsMeasureInProgress();
}

//---------------------------------------------------------------------------
//
//  ParagraphNode::RemoveElement
//
//  Synopsis:
//      Removes the given embedded element from the host, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::RemoveElement(
    _In_ CInlineUIContainer *pContainer
    )
{
    // ParagraphNode wouldn't be asked to remove an element it hadn't added,
    // so page node lookup must have happened.
    ASSERT(m_pPageNode != NULL);

    IFC_RETURN(m_pPageNode->RemoveElement(pContainer, this));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  ParagraphNode::UpdateElementPosition
//
//  Synopsis:
//      Updates the given embedded element position, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::UpdateElementPosition(
    _In_ CInlineUIContainer *pContainer,
    _In_ const XPOINTF &position
    )
{
    XPOINTF positionInPage;

    // ParagraphNode wouldn't be asked to update position of an element it hadn't added,
    // so page node lookup must have happened.
    ASSERT(m_pPageNode != NULL);

    // Offsets are relative to this node, so transform them relative to the root, which is the page node.
    IFC_RETURN(TransformOffsetToRoot(position, &positionInPage));

    IFC_RETURN(m_pPageNode->UpdateElementPosition(pContainer, this, positionInPage));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  ParagraphNode::GetElementPosition
//
//  Synopsis:
//      Retrieves the embedded element position in host space, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::GetElementPosition(
    _In_ CInlineUIContainer *pContainer,
    _Out_ XPOINTF    *pPosition
    )
{
    XPOINTF positionInPage;

    // ParagraphNode wouldn't be asked to update position of an element it hadn't added,
    // so page node lookup must have happened.
    ASSERT(m_pPageNode != NULL);

    IFC_RETURN(m_pPageNode->GetElementPosition(pContainer, this, &positionInPage));

    // Offsets will be relative to the page, transform it to this node's coordinate system.
    IFC_RETURN(TransformOffsetFromRoot(positionInPage, &positionInPage));

    *pPosition = positionInPage;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  ParagraphNode::GetAvailableMeasureSize
//
//  Synopsis:
//      Retrieves the available size of the host, IEmbeddedElementHost override.
//
//---------------------------------------------------------------------------
const XSIZEF & ParagraphNode::GetAvailableMeasureSize() const
{
    return m_prevAvailableSize;
}

_Check_return_ HRESULT ParagraphNode::IsAtInsertionPosition(
    _In_ XUINT32 position,
    _Out_ bool *pIsAtInsertionPosition
    )
{
    HRESULT hr = S_OK;
    XUINT32 positionInParagraph = GetPositionInParagraph(position);
    XUINT32 lineIndex = GetLineIndexFromPosition(positionInParagraph, NULL);
    TextFormatter *pTextFormatter = NULL;
    TextParagraphProperties *pParagraphProperties = NULL;
    TextLine *pTextLine  = NULL;
    TextLine *pLocalTextLine  = NULL;

    ASSERT(lineIndex < m_lines.GetCount());
    LineMetrics lineMetrics = m_lines[lineIndex];

    if (!lineMetrics.HasMultiCharacterClusters)
    {
       // Line does not have clusters. Use simple path to check for basic surrogates and CRLF pair.
        bool isInSurrogateCRLF = false;
        IFC(m_textSource.IsInSurrogateCRLF(positionInParagraph, &isInSurrogateCRLF));
        *pIsAtInsertionPosition = !isInSurrogateCRLF;
    }
    else
    {
        // Line has clusters, we need to format it and do a more detailed check.
        pTextLine = lineMetrics.Line;

        if (pTextLine == NULL)
        {
            IFC(FormatLineAtIndex(
                    lineIndex,
                    BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner()),
                    lineMetrics.Rect.Width,
                    &pTextFormatter,
                    &pParagraphProperties,
                    &pLocalTextLine));

            pTextLine = pLocalTextLine;
        }

        IFC(IsAtInsertionPositionInTextLine(
            positionInParagraph,
            lineMetrics.FirstCharIndex,
            pTextLine,
            pIsAtInsertionPosition));
    }

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pParagraphProperties);
    ReleaseInterface(pLocalTextLine);
    RRETURN(hr);
}

_Check_return_ HRESULT ParagraphNode::PixelPositionToTextPosition(
    _In_ const XPOINTF& pixelPosition,
    _Out_ XUINT32 *pTextPosition,
    _Out_opt_ TextGravity *pGravity
    )
{
    HRESULT hr = S_OK;
    XPOINTF lineOffset;
    TextFormatter *pTextFormatter = NULL;
    TextParagraphProperties *pParagraphProperties = NULL;
    TextLine *pTextLine = NULL;
    TextLine *pLocalTextLine = NULL;
    XPOINTF contentOffset = GetContentRenderingOffset();
    XPOINTF linePixel = { pixelPosition.x - contentOffset.x, pixelPosition.y - contentOffset.y };
    XUINT32 lineIndex = GetLineIndexFromPoint(linePixel, &lineOffset);
    CharacterHit characterHit;
    XUINT32 characterIndex;
    XFLOAT characterDistance = 0.0f;

    ASSERT(lineIndex < m_lines.GetCount());
    LineMetrics lineMetrics = m_lines[lineIndex];

    // Line has clusters, we need to format it and do a more detailed check.
    pTextLine = lineMetrics.Line;

    // Hit testing should only occur after Arrange, so we don't expect to have a line.
    if (pTextLine == NULL)
    {
        IFC(FormatLineAtIndex(
                lineIndex,
                BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner()),
                lineMetrics.Rect.Width,
                &pTextFormatter,
                &pParagraphProperties,
                &pLocalTextLine));

        pTextLine = pLocalTextLine;
    }


    // If the detected paragraph direction doesn't align with specificed paragraph direction,
    // the logical coordinate must be adjusted to match the visual coordinate.
    if (pTextLine->AlignmentFollowsReadingOrder())
    {
        characterDistance = linePixel.x - lineMetrics.Rect.X;
    }
    else
    {
        characterDistance = lineMetrics.Rect.Width - linePixel.x;
    }

    IFC(RichTextServicesHelper::MapTxErr(pTextLine->GetCharacterHitFromDistance(characterDistance, &characterHit)));
    characterIndex = static_cast<XUINT32>(characterHit.firstCharacterIndex + characterHit.trailingLength);

    // Convert the paragraph-relative character index to one relative to this node, since hit testing through layout nodes
    // will only compute node-relative offsets, it doesn't know where a paragraoh has broken.
    *pTextPosition = GetLocalPositionFromParagraphPosition(characterIndex);
    if (pGravity != NULL)
    {
        // TODO: This is a lossy conversion - at line beginning/end we may need other gravity types. Fix it.
        *pGravity = ((characterHit.trailingLength == 0) ? LineForwardCharacterForward : LineForwardCharacterBackward);

        // If the returned position is after the last character in this line,
        // it maps to the first character of the next line.
        // Indicate this by adding LineBackward to the gravity, which means that the position
        // should be considered in the line above (before the line break)
        if (*pTextPosition >= (lineMetrics.FirstCharIndex + lineMetrics.Length))
        {
            *pGravity = static_cast<TextGravity>(*pGravity | LineBackward);
        }
    }

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pParagraphProperties);
    ReleaseInterface(pLocalTextLine);
    RRETURN(hr);
}

_Check_return_ HRESULT ParagraphNode::GetTextBounds(
    _In_ XUINT32 start,
    _In_ XUINT32 length,
    _Inout_ xvector<RichTextServices::TextBounds> *pBounds
    )
{
    HRESULT hr = S_OK;
    TextFormatter *pTextFormatter = NULL;
    TextParagraphProperties *pParagraphProperties = NULL;
    TextLine *pTextLine = NULL;
    XUINT32 remainingLength = length;
    TextBounds *pLineBounds = NULL;
    XUINT32 count;
    XPOINTF lineOffset;
    XPOINTF contentOffset = GetContentRenderingOffset();

    // Start and length should be snapped to paragraph bounds so we can just iterate through lines.
    ASSERT(length > 0);
    ASSERT(start + length <= m_length);

    // Start+Length is inclusive, so end index is start + length - 1.
    XUINT32 startPositionInParagraph = GetPositionInParagraph(start);
    XUINT32 startLineIndex = GetLineIndexFromPosition(startPositionInParagraph, &lineOffset);
    XUINT32 endPositionInParagraph = GetPositionInParagraph(start + length - 1);
    XUINT32 endLineIndex = GetLineIndexFromPosition(endPositionInParagraph, NULL);

    ASSERT(startLineIndex < m_lines.GetCount());
    ASSERT(endLineIndex < m_lines.GetCount());
    ASSERT(startLineIndex <= endLineIndex);

    TextTrimming textTrimming = BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner());

    for (XUINT32 i = startLineIndex; i <= endLineIndex; i++)
    {
        LineMetrics lineMetrics = m_lines[i];
        XUINT32 lineEndIndex = lineMetrics.FirstCharIndex + lineMetrics.Length;

        ASSERT(remainingLength > 0);

        start = (i == startLineIndex) ? startPositionInParagraph : lineMetrics.FirstCharIndex;
        length = MIN(remainingLength, lineEndIndex - start);

        IFC(FormatLineAtIndex(
                i,
                textTrimming,
                lineMetrics.Rect.Width,
                &pTextFormatter,
                &pParagraphProperties,
                &pTextLine));

        IFC(RichTextServicesHelper::MapTxErr(pTextLine->GetTextBounds(start, length, &count, &pLineBounds)));

        IFC(pBounds->reserve(pBounds->size() + count));
        for (XUINT32 j = 0; j < count; j++)
        {
            // If the detected paragraph direction doesn't align with specificed paragraph direction,
            // the logical coordinate must be adjusted to match the visual coordinate.
            if (pTextLine->AlignmentFollowsReadingOrder())
            {
                pLineBounds[j].rect.X += (contentOffset.x + lineMetrics.Rect.X);
            }
            else
            {
                pLineBounds[j].rect.X = lineMetrics.Rect.Width - (pLineBounds[j].rect.Width + pLineBounds[j].rect.X + contentOffset.x);
            }

            // In y-dimension, bounds should always correspond to the full line advance, including and line height/ stacking strategy properties
            // applied, regardless of the text bounds within the line.
            pLineBounds[j].rect.Y += (contentOffset.y + lineOffset.y);
            pLineBounds[j].rect.Height = lineMetrics.VerticalAdvance;

            IFC(pBounds->push_back(pLineBounds[j]));
        }

        ASSERT(remainingLength >= length);
        remainingLength -= length;
        lineOffset.y += lineMetrics.VerticalAdvance;
        ReleaseInterface(pTextLine);
        delete[] pLineBounds;
        pLineBounds = NULL;
    }

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pParagraphProperties);
    ReleaseInterface(pTextLine);
    delete[] pLineBounds;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::MeasureCore
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::MeasureCore(
    _In_ XSIZEF availableSize,
    _In_ XUINT32 paragraphMaxLines,
    _In_ bool allowEmptyContent,
    _In_ bool measureBottomless,
    _In_ bool suppressTopMargin,
    _In_opt_ BlockNodeBreak *pPreviousBreak
    )
{
    HRESULT hr = S_OK;
    TextParagraphProperties *pParagraphProperties = NULL;
    TextLineBreak *pPreviousLineBreak = NULL;
    TextLine *pTextLine = NULL;
    ParagraphNodeBreak *pPreviousParagraphBreak = NULL;
    ParagraphNodeBreak *pBreak = NULL;
    bool endOfParagraph = false;
    XFLOAT lineOffset = 0;
    TextFormatter *pTextFormatter = NULL;
    LineStackingStrategy lineStackingStrategy;
    XFLOAT defaultFontBaseline;
    XFLOAT defaultFontLineAdvance;
    XFLOAT defaultLineHeight;
    XUINT32 firstCharIndex = 0;
    bool addLineToMetrics = true;
    bool firstLine = true;
    TextTrimming textTrimming = BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner());

    if (pPreviousBreak != NULL)
    {
        pPreviousParagraphBreak = static_cast<ParagraphNodeBreak *>(pPreviousBreak);
    }

    IFC(EnsureTextCaches(pPreviousParagraphBreak));
    IFC(BlockLayoutHelpers::GetTextFormatter(m_pBlockLayoutEngine->GetOwner(), &pTextFormatter));
    IFC(BlockLayoutHelpers::GetParagraphProperties(m_pElement, m_pBlockLayoutEngine->GetOwner(), &pParagraphProperties));

    if (IsContentDirty())
    {
        // Currently this shouldn't ever be the case since we have no incremental content invalidation and PageNode
        // deletes all its children when content is dirty.
        if (m_pTextRunCache != NULL)
        {
            m_pTextRunCache->Clear();
        }
    }
    DeleteLineCache();
    RemoveEmbeddedElements();
    InvalidateArrange();
    m_cachedMaxLines = paragraphMaxLines;

    m_desiredSize.width = m_desiredSize.height = 0;
    m_untrimmedDesiredWidth = 0;
    m_length = 0;
    ReleaseInterface(m_pBreak);

    // Get line stacking info.
    IFC(BlockLayoutHelpers::GetLineStackingInfo(
        m_pElement,
        m_pBlockLayoutEngine->GetOwner(),
        &lineStackingStrategy,
        &defaultFontBaseline,
        &defaultFontLineAdvance,
        &defaultLineHeight));

    if (pPreviousParagraphBreak != NULL)
    {
        pPreviousLineBreak = pPreviousParagraphBreak ->GetLineBreak();
        firstCharIndex = pPreviousParagraphBreak->GetBreakIndex();
    }

    // We always want to measure at least one line, whether we add it or not.
    ASSERT(availableSize.height >= 0.0f);

    while ( !endOfParagraph &&
            (lineOffset <= availableSize.height) &&
            (paragraphMaxLines == 0 || m_lines.GetCount() < paragraphMaxLines))
    {
        LineMetrics lineMetrics;
        lineMetrics.LineBreak = pPreviousLineBreak;
        addLineToMetrics = TRUE;
        AddRefInterface(pPreviousLineBreak);

        IFC(RichTextServicesHelper::MapTxErr(pTextFormatter->FormatLine(
            &m_textSource,
            firstCharIndex,
            availableSize.width,
            pParagraphProperties,
            lineMetrics.LineBreak,
            m_pTextRunCache,
            &pTextLine)));

        endOfParagraph = (pTextLine->GetTextLineBreak() == NULL);

        ApplyLineStackingStrategy(
            lineStackingStrategy,
            defaultFontBaseline,
            defaultFontLineAdvance,
            defaultLineHeight,
            pTextLine,
            firstLine,
            endOfParagraph,
            &lineMetrics.VerticalAdvance,
            &lineMetrics.Rect.Y);

        lineMetrics.FirstCharIndex = firstCharIndex;
        lineMetrics.Length = pTextLine->GetLength();
        lineMetrics.Rect.Width = pTextLine->GetWidth();
        lineMetrics.Rect.Height = pTextLine->GetHeight();
        lineMetrics.BaselineOffset = pTextLine->GetBaseline() + lineMetrics.Rect.Y;
        lineMetrics.HasMultiCharacterClusters = pTextLine->HasMultiCharacterClusters();
        lineMetrics.Line = pTextLine;

        // If empty content is allowed and the current line overflows the available height,
        // do not add it to the line array and exit formatting.
        // If empty content is not allowed we need to add at least one line.
        if (lineOffset + lineMetrics.VerticalAdvance > availableSize.height)
        {
            // Create break. No need to release the break break or line from metrics, they
            // will be released in Cleanup if not reset to NULL.
            if (!allowEmptyContent)
            {
                if (!endOfParagraph)
                {
                    // If allowEmptyContent == FALSE, we have to add this line even if it doesn't fit,
                    // we'll break after it assuming we're not at EOP.
                    // For the bottomless measure case, we also keep the line if it doesn't fit
                    // completely, assuming TextTrimming is not applied - if it is, paragraph ellipsis will be shown.
                    // This is an optimization - in theory bottomless measure could measure all
                    // content, but that's unnecessary given that all of it will not be rendered.
                    pBreak = new ParagraphNodeBreak(firstCharIndex + lineMetrics.Length,
                                                            pTextLine->GetTextLineBreak(),
                                                            m_pTextRunCache);
                }
            }
            else
            {
                // If empty content is allowed, break at the line's start. In paginated mode
                // or bottomless mode with text trimming, this line should also not be part of
                // metrics - it will not be rendered at all, or hit testable.
                // In bottomless mode without trimming, produce the break at the same point - the line technically
                // didn't fit, but add it to metrics since we want to render a partially clipped line.
                pBreak = new ParagraphNodeBreak(firstCharIndex,
                                                        pPreviousLineBreak,
                                                        m_pTextRunCache);

                if (!measureBottomless ||
                    textTrimming != DirectUI::TextTrimming::None)
                {
                    addLineToMetrics = FALSE;
                }
            }
        }

        // If the line fits in available space i.e. no break, or we have to add it,
        // even if there is a break since we're not allowing empty content,
        // complete the addition of line metrics and update paragraph metrics.
        if (addLineToMetrics)
        {
            IFC(m_lines.Add(lineMetrics));
            m_untrimmedDesiredWidth = MAX(m_untrimmedDesiredWidth, lineMetrics.Rect.Width);
            m_desiredSize.height += lineMetrics.VerticalAdvance;
            m_length += lineMetrics.Length;

            if (textTrimming != DirectUI::TextTrimming::None)
            {
                // Trimming modes can always fit to whatever constraint is provided, so don't
                // return a larger width than the constraint.  (This is similar to the wrapping
                // scenarios).  If we ask for more space than the constraint in Measure then
                // FrameworkElement will give us back that unclipped desired size in Arrange
                // and there's no way for us to trim or align to the actual arrange slot width.
                m_desiredSize.width = MIN(m_untrimmedDesiredWidth, availableSize.width);
            }
            else
            {
                m_desiredSize.width = m_untrimmedDesiredWidth;
            }

            // If paragraphMaxLines has been reached we end formatting
            if (paragraphMaxLines != 0 && m_lines.GetCount() >= paragraphMaxLines)
            {
                // Don't introduce an additional paragraph break if this is the end of the paragraph.
                if (!endOfParagraph && pBreak == NULL)
                {
                    pBreak = new ParagraphNodeBreak(firstCharIndex + lineMetrics.Length,
                                                            pTextLine->GetTextLineBreak(),
                                                            m_pTextRunCache);
                }
            }

            // If we have not produced a break, we will format the next line, so we update
            // the previous break value. If we have produced a break we're not going to format
            // any more lines but we don't want to let go of the previous break at the end of formatting,
            // since its part of metrics.
            pPreviousLineBreak = (pBreak == NULL) ? pTextLine->GetTextLineBreak() : NULL;
            pTextLine = NULL;
        }

        firstCharIndex += lineMetrics.Length;
        lineOffset += lineMetrics.VerticalAdvance;
        allowEmptyContent = TRUE;
        firstLine = FALSE;
    }

    m_measuredLines = m_lines.GetCount();
    m_pBreak = pBreak;
    pBreak = NULL;

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pBreak);
    ReleaseInterface(pParagraphProperties);
    ReleaseInterface(pPreviousLineBreak);
    ReleaseInterface(pTextLine);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::ArrangeCore
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::ArrangeCore(
    _In_ XSIZEF finalSize
    )
{
    HRESULT hr = S_OK;
    XFLOAT lineOffset = 0.0f;
    DirectUI::TextAlignment textAlignment = DirectUI::TextAlignment::Left;
    TextTrimming textTrimming = BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner());
    ParagraphDrawingContext *pDrawingContext = NULL;
    TextDrawingContext *pTextDrawingContext = NULL;
    TextFormatter *pTextFormatter = NULL;
    TextParagraphProperties *pParagraphProperties = NULL;
    XFLOAT renderWidth = 0.0f;

    ASSERT(!IsInfiniteF(finalSize.width));
    ASSERT(!IsInfiniteF(finalSize.height));

    // Clear all drawing instructions from the drawing context.
    // The drawing context is not used in Arrange, but should be cleared here since Arrange implies a Render to follow,
    // and if no Render follows we don't want stale content on the drawing context. Additionally, DrawingContext
    // may be needed for callers to push some state to it between Arrange and Render. By the time Render is called
    // the DC should be initialized and ready with any necessary state.
    if (m_pDrawingContext == NULL)
    {
        pDrawingContext = new ParagraphDrawingContext(m_pBlockLayoutEngine->GetOwner()->GetContext(), this);
        m_pDrawingContext = pDrawingContext;
    }
    else
    {
        pDrawingContext = static_cast<ParagraphDrawingContext *>(m_pDrawingContext);
    }
    IFC(pDrawingContext->GetTextDrawingContext(&pTextDrawingContext));

    IFC(BlockLayoutHelpers::GetTextAlignment(m_pElement, m_pBlockLayoutEngine->GetOwner(), &textAlignment));

    // Embedded elements will be given their visual offsets here based on available space when lines are arranged.
    // Visibility should be reset before this happens.
    MarkEmbeddedElementsInvisible();

    // Reset to assume there's no trimmed line before checking if there is.
    m_hasTrimmedLine = false;

    for (XUINT32 lineIndex = 0, lineCount = m_lines.GetCount(); lineIndex < lineCount; lineIndex++)
    {
        LineMetrics& lineMetrics = m_lines[lineIndex];
        TextLine *pTextLine = lineMetrics.Line;
        XRECTF lineBounds;

        // Since we need trimmed width to calculate alignment offset, format lines in Arrange ahead of Render.
        // Render always expects to follow Arrange and will expect lines to remain, and then delete them.
        IFC(FormatLineAtIndex(
                lineIndex,
                textTrimming,
                finalSize.width,
                &pTextFormatter,
                &pParagraphProperties,
                &pTextLine));
        lineMetrics.Line = pTextLine;

        lineMetrics.Rect.Width = pTextLine->GetWidth();
        lineMetrics.Rect.X = CalculateLineOffset(textAlignment, pTextLine, finalSize.width);

        renderWidth = MAX(renderWidth, lineMetrics.Rect.Width);

        // Arrange the line with its final position within this paragraph.
        lineBounds = lineMetrics.Rect;
        lineBounds.Y += lineOffset;
        pTextLine->Arrange(lineBounds);

        lineOffset += lineMetrics.VerticalAdvance;
    }

    // Render width is the MAX of the arrange width and the render width.  In TextTrimming=None
    // scenarios, arrangeWidth >= renderWidth because we reported the width we needed in measure
    // and that's what we'll be given back in Arrange.  For TextTrimming=Clip we can always
    // fit the arrange width provided, so again arrangeWidth >= renderWidth.  But for
    // TextTrimming=Character/WordEllipsis we will try our best to fit the provided arrange width.
    // But if the arrange width is narrow, and isn't enough to fit the character+ellipsis then
    // our renderWidth will exceed the arrangeWidth.  We need to bubble that excess up to
    // FrameworkElement so that it can apply a layout clip in these scenarios.
    m_renderSize.width = MAX(finalSize.width, renderWidth);

    // Render size height is the MIN of desired size height and final size height - if there's
    // more than enough room to render, paragraph will just render all its content.
    m_renderSize.height = MIN(m_desiredSize.height, finalSize.height);

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pParagraphProperties);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::RenderCore
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::DrawCore(
    _In_ bool forceDraw
    )
{
    HRESULT hr = S_OK;
    XFLOAT lineOffset = 0.0f;
    bool skipRemainingLines = false;
    bool isRightToLeft = false;
    XUINT32 lineCharacterIndex = 0;
    TextDrawingContext *pTextDrawingContext = NULL;
    TextTrimming textTrimming = BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner());
    TextFormatter *pTextFormatter = NULL;
    TextParagraphProperties *pParagraphProperties = NULL;
    bool isColorFontEnabled = BlockLayoutHelpers::GetIsColorFontEnabled(m_pBlockLayoutEngine->GetOwner());


    if (BlockLayoutHelpers::GetFlowDirection(m_pBlockLayoutEngine->GetOwner()) == RichTextServices::FlowDirection::RightToLeft)
    {
        isRightToLeft = TRUE;
    }

    // DrawingContext should be created and cleared in Arrange.
    ASSERT(m_pDrawingContext != NULL);
    IFC((static_cast<ParagraphDrawingContext *>(m_pDrawingContext))->GetTextDrawingContext(&pTextDrawingContext));
    pTextDrawingContext->Clear();

    pTextDrawingContext->SetIsColorFontEnabled(isColorFontEnabled);

    for (XUINT32 lineIndex = 0, lineCount = m_lines.GetCount(); lineIndex < lineCount; lineIndex++)
    {
        LineMetrics &lineMetrics = m_lines[lineIndex];
        XPOINTF origin = { lineMetrics.Rect.X, lineOffset + lineMetrics.Rect.Y };
        bool skipLine = false;
        lineCharacterIndex = lineMetrics.FirstCharIndex;

        // Draw only lines within the viewport area. Assume that anything for which there are metrics can be
        // rendered, even partially. If a line should not be rendered in this node, it will not be in the metrics.
        if (origin.y > m_renderSize.height)
        {
            // At the bottom of the viewport we can also skip all remaining lines.
            skipLine = TRUE;
            skipRemainingLines = TRUE;
        }

        if (!skipLine)
        {
            // Line must exist in the metrics at this time, either created during Measure or Arrange.
            TextLine *pTextLine = lineMetrics.Line;

            // Since we need trimmed width to calculate alignment offset, format lines in Arrange ahead of Render.
            // Render always expects to follow Arrange and will expect lines to remain, and then delete them.
            if (pTextLine == NULL)
            {
                IFC(FormatLineAtIndex(
                    lineIndex,
                    textTrimming,
                    lineMetrics.Rect.Width,
                    &pTextFormatter,
                    &pParagraphProperties,
                    &pTextLine));
                lineMetrics.Line = pTextLine;
            }

            pTextDrawingContext->SetLineInfo(m_renderSize.width, isRightToLeft, origin.y, lineMetrics.VerticalAdvance);
            IFC(RichTextServicesHelper::MapTxErr(pTextLine->Draw(pTextDrawingContext, origin, m_renderSize.width)));

            if (textTrimming != DirectUI::TextTrimming::None)
            {
                // If paragraph ellipsis will be shown on this line, we can skip all remaining lines.
                if (ShouldShowParagraphEllipsisOnCurrentLine(
                    (lineIndex < lineCount - 1) ? FALSE : TRUE))
                {
                    skipRemainingLines = TRUE;
                }
            }
        }

        lineOffset += lineMetrics.VerticalAdvance;

        // Release the cached line.
        ReleaseInterface(lineMetrics.Line);

        if (skipRemainingLines)
        {
            break;
        }
    }

Cleanup:
    BlockLayoutHelpers::ReleaseTextFormatter(m_pBlockLayoutEngine->GetOwner(), pTextFormatter);
    ReleaseInterface(pParagraphProperties);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::CanBypassMeasure
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::CanBypassMeasure(
    _In_ XSIZEF availableSize,
    _In_ XUINT32 paragraphMaxLines,
    _In_ bool allowEmptyContent,
    _In_ bool measureBottomless,
    _In_opt_ BlockNodeBreak *pPreviousBreak,
    _Out_ bool *pCanBypass
    )
{
    HRESULT hr = S_OK;
    TextParagraphProperties *pParagraphProperties = NULL;
    bool canBypassMeasure = false;

    IFC(BlockLayoutHelpers::GetParagraphProperties(m_pElement, m_pBlockLayoutEngine->GetOwner(), &pParagraphProperties));

    // Current bypass requires all "measure states" to be equal except size, similar to BlockNode bypass except with slightly relaxed restrictions on size equality.
    // Additional optimization might be possible - e.g. when switching from finite to bottomless or vice versa it may be OK to bypass if there is no break and all content
    // will still fit, but those are not critical scenarios and we forgo bypass there for simplifying the code. We can consider more sophisticated bypass logic if
    // perf for key scenarios warrants it.
    if (!IsMeasureDirty() &&
        (IsEmptyContentAllowed() == allowEmptyContent) &&
        (IsMeasureBottomless() == measureBottomless) &&
        (m_cachedMaxLines == paragraphMaxLines) &&
        BlockNodeBreak::Equals(pPreviousBreak, m_pPreviousBreak))
    {
        TextTrimming textTrimming = BlockLayoutHelpers::GetTextTrimming(m_pBlockLayoutEngine->GetOwner());

        // Detect if measure can be bypassed - paragraph implements additional measure bypass checks over BlockNode depending on wrapping, etc.
        // Measure might be bypassed if content has not been changed AND:
        // 1) Not wrapping AND Not clipping AND AvailableSize.Height <= PrevAvaiableSize.Height
        //    (width constraint can be ignored, since it does not have any effects on the desired size).
        //    If TextTrimming is not None, we always need to remeasure since the desired size is dependent on
        //    the(possibly changed) available size.
        // 2) Wrapping AND m_untrimmedDesiredWidth <= AvailableSize.Width <= PrevAvailableSize.Width AND
        //    AvailableSize.Height <= PreviousAvailableSize.Height FOR BOTTOMLESS SCENARIOS E.G. TEXTBLOCK.
        //    When breaking, e.g. for linking/pages we need to re measure if height is smaller, since we will
        //    break differently.
        // NOTE: If we add justification support, need to check for width equity.
        if ((pParagraphProperties->GetTextWrapping() == DirectUI::TextWrapping::NoWrap && textTrimming == DirectUI::TextTrimming::None) ||
            (m_untrimmedDesiredWidth <= availableSize.width && availableSize.width <= m_prevAvailableSize.width))
        {
            if (m_pBreak != NULL)
            {
                // If there is a break for this paragraph, then height constraint needs to be
                // the same to break at the same place.
                canBypassMeasure = IsCloseReal(availableSize.height, m_prevAvailableSize.height);
            }
            else
            {
                // If there was no break, we can bypass as long as the desired height will fit
                // in the available space.
                canBypassMeasure = (m_desiredSize.height  <= availableSize.height);
            }
        }

        if (canBypassMeasure)
        {
            // If measure is being bypassed, it means also that arrange could be bypassed if the final size is not changing.
            // However if TextTrimming is enabled, we cannot bypass the arrange, if previous available width is
            // different than the current available width. This is due to the fact that available size is used as collapsing width.
            if (textTrimming != DirectUI::TextTrimming::None &&
                !IsCloseReal(availableSize.width, m_prevAvailableSize.width))
            {
                InvalidateArrange();
            }
        }
    }

    *pCanBypass = canBypassMeasure;

Cleanup:
    ReleaseInterface(pParagraphProperties);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::DeleteLineCache
//
//---------------------------------------------------------------------------
void ParagraphNode::DeleteLineCache()
{
    for (XUINT32 lineIndex = 0, lineCount = m_lines.GetCount(); lineIndex < lineCount; lineIndex++)
    {
        ReleaseInterface(m_lines[lineIndex].LineBreak);
        ReleaseInterface(m_lines[lineIndex].Line);
    }
    m_lines.Clear();
    ReleaseInterface(m_pBreak);
}

//---------------------------------------------------------------------------
//
// ParagraphNode::EnsureTextCaches
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::EnsureTextCaches(
    _In_opt_ ParagraphNodeBreak *pPreviousBreak
    )
{
    HRESULT hr = S_OK;

    // Obtain a run cache from a previous break or create one if necessary.
    // If there is a run cache, either our own or from the break, assume all data in it is valid.
    // Invalidation is processed by a separate code path.
    if (pPreviousBreak != NULL)
    {
        // If there is a previous break, it must have a run cache.
        ReleaseInterface(m_pTextRunCache);
        m_pTextRunCache = pPreviousBreak->GetRunCache();
        m_pTextRunCache->AddRef();
    }
    else
    {
        // If there is no run cache, create one.
        if (m_pTextRunCache == NULL)
        {
            IFC(RichTextServicesHelper::MapTxErr(TextRunCache::Create(&m_pTextRunCache)));
        }
    }

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::FormatLineAtIndex
//
//---------------------------------------------------------------------------
HRESULT ParagraphNode::FormatLineAtIndex(
    _In_ XUINT32 lineIndex,
    _In_ TextTrimming textTrimming,
    _In_ XFLOAT layoutWidth,
    _Inout_ TextFormatter **ppTextFormatter,
    _Inout_ TextParagraphProperties **ppParagraphProperties,
    _Inout_ __pre_deref_except_maybenull TextLine **ppTextLine
    )
{
    HRESULT hr = S_OK;
    TextLine *pCollapsedTextLine = NULL;

    LineMetrics& lineMetrics = m_lines[lineIndex];

    if (*ppTextLine == NULL)
    {
        if (*ppTextFormatter == NULL)
        {
            IFC(BlockLayoutHelpers::GetTextFormatter(m_pBlockLayoutEngine->GetOwner(), ppTextFormatter));
            IFC(BlockLayoutHelpers::GetParagraphProperties(m_pElement, m_pBlockLayoutEngine->GetOwner(), ppParagraphProperties));
        }

        // Line should always be reformatted at the width at which it was first measured to ensure that the metrics cache is valid.
        IFC(RichTextServicesHelper::MapTxErr((*ppTextFormatter)->FormatLine(
            &m_textSource,
            lineMetrics.FirstCharIndex,
            m_prevAvailableSize.width,
            *ppParagraphProperties,
            lineMetrics.LineBreak,
            m_pTextRunCache,
            ppTextLine)));

        ASSERT(lineMetrics.Length == (*ppTextLine)->GetLength());
    }

    if ((textTrimming != DirectUI::TextTrimming::None) && !(*ppTextLine)->HasCollapsed())
    {
        IFC(TrimLineIfNecessary(
            (lineIndex < m_lines.GetCount() - 1) ? &m_lines[lineIndex + 1] : NULL,
            *ppTextLine,
            layoutWidth,
            textTrimming,
            &pCollapsedTextLine
            ));

        if (pCollapsedTextLine != NULL)
        {
            ReleaseInterface(*ppTextLine);
            *ppTextLine = pCollapsedTextLine;
            pCollapsedTextLine = NULL;
        }
    }

Cleanup:
    ReleaseInterface(pCollapsedTextLine);
    return hr;
}


//---------------------------------------------------------------------------
//
// ParagraphNode::CalculateLineOffset
//
//---------------------------------------------------------------------------
XFLOAT ParagraphNode::CalculateLineOffset(
    _In_ DirectUI::TextAlignment textAlignment,
    _In_ RichTextServices::TextLine *pTextLine,
    _In_ XFLOAT paragraphWidth
    )
{
    XFLOAT lineOffset = 0.0f;
    XFLOAT lineWidth = pTextLine->GetWidth();
    DirectUI::TextAlignment lineAlignment = textAlignment;

    if (textAlignment == DirectUI::TextAlignment::DetectFromContent)
    // if alignment is set to DetectFromContent, compare the set paragraph direction and the detected paragraph direction to determine the line alignment( need to flip and they mismatch)
    {
        if (pTextLine->GetParagrahDirection() == RichTextServices::FlowDirection::RightToLeft)
        {
            if (pTextLine->GetDetectedParagrahDirection() == RichTextServices::FlowDirection::RightToLeft)
            {
                lineAlignment = DirectUI::TextAlignment::Left;
            }
            else
            {
                lineAlignment = DirectUI::TextAlignment::Right;
            }
        }
        else
        {
            if (pTextLine->GetDetectedParagrahDirection() == RichTextServices::FlowDirection::RightToLeft)
            {
                lineAlignment = DirectUI::TextAlignment::Right;
            }
            else
            {
                lineAlignment = DirectUI::TextAlignment::Left;
            }
        }
    }
    else
    {
        // If the detected reading order is different from the default flow direction,
        // flip the textalignment to match the set flowdirection & textalignment combination.
        if (!pTextLine->AlignmentFollowsReadingOrder())
        {
            if (textAlignment == DirectUI::TextAlignment::Right)
            {
                lineAlignment = DirectUI::TextAlignment::Left;
            }
            else if ((textAlignment == DirectUI::TextAlignment::Left) ||
                (textAlignment == DirectUI::TextAlignment::Justify))
            {
                lineAlignment = DirectUI::TextAlignment::Right;
            }
        }
    }

    if (!IsInfiniteF(paragraphWidth))
    {
        switch (lineAlignment)
        {
        case DirectUI::TextAlignment::Right:
            lineOffset = paragraphWidth - lineWidth;
            break;

        case DirectUI::TextAlignment::Center:
            lineOffset = (paragraphWidth - lineWidth) / 2;
            break;

        default:
            break;
        }
    }
    return lineOffset;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::ApplyLineStackingStrategy
//
//---------------------------------------------------------------------------
void ParagraphNode::ApplyLineStackingStrategy(
    _In_ LineStackingStrategy lineStackingStrategy,
    _In_ XFLOAT fontBaseline,
    _In_ XFLOAT fontLineAdvance,
    _In_ XFLOAT lineHeight,
    _In_ RichTextServices::TextLine *pTextLine,
    _In_ bool firstLine,
    _In_ bool lastLine,
    _Out_ XFLOAT *pLineAdvance,
    _Out_ XFLOAT *pLineOffset
    )
{
    XFLOAT fontBaselineRatio = fontBaseline / fontLineAdvance;

    if (lineHeight <= 0)
    {
        lineStackingStrategy = DirectUI::LineStackingStrategy::MaxHeight;
    }

    switch (lineStackingStrategy)
    {
    case DirectUI::LineStackingStrategy::BlockLineHeight:
        // The 'BlockLineHeight' line stacking strategy simply uses
        // the block line metrics
        *pLineAdvance = lineHeight;
        *pLineOffset = (lineHeight - pTextLine->GetHeight()) * fontBaselineRatio;
        break;

    case DirectUI::LineStackingStrategy::BaselineToBaseline:
        // The 'BaselineToBaseline' line stacking strategy uses the block line
        // metrics on all lines except the first and last.
        *pLineAdvance = lineHeight;
        *pLineOffset = (lineHeight - pTextLine->GetHeight()) * fontBaselineRatio;

        if (firstLine)
        {
            *pLineAdvance -= *pLineOffset;
            *pLineOffset = 0.0f;
        }
        else if (lastLine)
        {
            *pLineAdvance -= (lineHeight - pTextLine->GetHeight()) * (1 - fontBaselineRatio);
        }
        break;

    default:
        *pLineAdvance = MAX(pTextLine->GetHeight(), lineHeight);
        *pLineOffset = 0.0f;
        break;
    }
}

//---------------------------------------------------------------------------
//
// ParagraphNode::TrimLineIfNecessary
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::TrimLineIfNecessary(
    _In_ const LineMetrics *pNextLineMetrics,
    _In_ RichTextServices::TextLine *pCurrentTextLine,
    _In_ XFLOAT collapsingWidth,
    _In_ TextTrimming collapsingStyle,
    _Outptr_result_maybenull_ RichTextServices::TextLine **ppCollapsedTextLine
    )
{
    HRESULT hr = S_OK;
    RichTextServices::TextCollapsingSymbol *pCollapsingSymbol = NULL;

    *ppCollapsedTextLine = NULL;

    XFLOAT lineWidth = pCurrentTextLine->GetWidth();
    bool trimmingNeeded = false;
    if (collapsingWidth < lineWidth)
    {
        CFrameworkElement *pTextOwner = static_cast<CFrameworkElement*>(m_pBlockLayoutEngine->GetOwner());
        if (pTextOwner->GetUseLayoutRounding())
        {
            //
            // If we're using layout rounding it's possible we've been given a slightly smaller width,
            // so check to see if our layout-rounded line width is actually less than the arrange width
            // before trimming.  If we choose not to apply trimming here it will be because we're only
            // off by a fraction of a pixel and our measure width really does still fit the provided
            // arrange width.  This means that we will still bubble up to FrameworkElement that our
            // render size is technically larger than the arrange width, but FrameworkElement LayoutRound()s
            // the render size we return before deciding if it should clip, and the same width that was
            // returned in measure gets rounded down to the same as the arrange width and no layout clip
            // needs to be applied.
            //

            float layoutRoundedLineWidth = pTextOwner->LayoutRound(lineWidth);
            if (collapsingWidth < layoutRoundedLineWidth)
            {
                trimmingNeeded = true;
            }
        }
        else
        {
            trimmingNeeded = true;
        }
    }

    // If the line being displayed has overflowed, collapse the line.
    if (   trimmingNeeded
        || ShouldShowParagraphEllipsisOnCurrentLine(pNextLineMetrics == NULL))
    {
        if ((collapsingStyle == DirectUI::TextTrimming::WordEllipsis)
            || (collapsingStyle == DirectUI::TextTrimming::CharacterEllipsis))
        {
            IFC(BlockLayoutHelpers::CreateCollapsingSymbol(m_pElement, m_pBlockLayoutEngine->GetOwner(), &pCollapsingSymbol));
        }

        IFC(RichTextServicesHelper::MapTxErr(pCurrentTextLine->Collapse(collapsingWidth, collapsingStyle, pCollapsingSymbol, ppCollapsedTextLine)));
        m_hasTrimmedLine = true;
    }

Cleanup:
    ReleaseInterface(pCollapsingSymbol);
    RRETURN(hr);
}

bool ParagraphNode::ShouldShowParagraphEllipsisOnCurrentLine(
    _In_ bool isLastLine
    )
{
    PageNode *pPageNode = NULL;
    bool showParagraphEllipsis = false;

    // If there is a break and this is the last line, paragraph ellipsis may need to be displayed depending
    // on the options for the page.
    // TODO: Lookup paragraph ellipsis options from page in better way, this only works for non-nested Paragraph.
    if (isLastLine &&
        m_pBreak != NULL)
    {
        pPageNode = static_cast<PageNode *>(m_pParentNode);
        ASSERT(pPageNode != NULL);
        showParagraphEllipsis = BlockLayoutHelpers::ShowParagraphEllipsisOnPage(pPageNode->GetPageOwner());
    }

    return showParagraphEllipsis;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::RemoveEmbeddedElements
//
//---------------------------------------------------------------------------
void ParagraphNode::RemoveEmbeddedElements()
{
    // If there were any embedded elements in this node, page node will be resolved already.
    if (m_pPageNode != NULL)
    {
        // Paragraph only recovers the page node for embedded element hosting.
        // On delete, ask PageNode to remove all embedded elements associated with this ParagraphNode.
        VERIFYHR(m_pPageNode->RemoveParagraphEmbeddedElements(this));
    }
}

//---------------------------------------------------------------------------
//
// ParagraphNode::MarkEmbeddedElementsInvisible
//
//---------------------------------------------------------------------------
void ParagraphNode::MarkEmbeddedElementsInvisible()
{
    // If there were any embedded elements in this node, page node will be resolved already.
    if (m_pPageNode != NULL)
    {
        // Paragraph only recovers the page node for embedded element hosting.
        // On delete, ask PageNode to remove all embedded elements associated with this ParagraphNode.
        m_pPageNode->MarkParagraphEmbeddedElementsInvisible(this);
    }
}

//---------------------------------------------------------------------------
//
// ParagraphNode::GetPositionInParagraph
//
//  Synopsis:
//      Gets the paragraph-relative offset from an offset local to this node.
//      This is determined using the previous break record, which indicates
//      where this node starts relative to the paragraph start. Line
//      formatting requires this offset.
//
//---------------------------------------------------------------------------
XUINT32 ParagraphNode::GetPositionInParagraph(
    _In_ XUINT32 localOffset
    ) const
{
    XUINT32 breakIndex = (m_pPreviousBreak == NULL) ? 0 : m_pPreviousBreak->GetBreakIndex();
    return localOffset + breakIndex;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::GetLocalPositionFromParagraphPosition
//
//  Synopsis:
//      Gets a local offset in the node from a paragraph-relative offset.
//      ParagraphNode needs to return only local offsets for query methods
//      since transformation to a global offset is done by the layout root.
//
//---------------------------------------------------------------------------
XUINT32 ParagraphNode::GetLocalPositionFromParagraphPosition(
    _In_ XUINT32 paragraphPosition
    ) const
{
    XUINT32 breakIndex = (m_pPreviousBreak == NULL) ? 0 : m_pPreviousBreak->GetBreakIndex();
    ASSERT(paragraphPosition >= breakIndex);
    return paragraphPosition - breakIndex;
}

XUINT32 ParagraphNode::GetLineIndexFromPosition(
    _In_ XUINT32 positionInParagraph,
    _Out_opt_ XPOINTF *pLineOffset
    ) const
{
    XPOINTF lineOffset = {0.0f, 0.0f};
    for (XUINT32 i = 0; i < m_lines.GetCount(); i++)
    {
        if (m_lines[i].FirstCharIndex <= positionInParagraph &&
            positionInParagraph < (m_lines[i].FirstCharIndex + m_lines[i].Length))
        {
            if (pLineOffset != NULL)
            {
                *pLineOffset = lineOffset;
            }
            return i;
        }
        lineOffset.y += m_lines[i].VerticalAdvance;
    }
    ASSERT(FALSE);
    return m_lines.GetCount();
}

XUINT32 ParagraphNode::GetLineIndexFromPoint(
    _In_ XPOINTF point,
    _Out_ XPOINTF *pLineOffset
    ) const
{
    XPOINTF lineOffset = {0.0f, 0.0f};
    XUINT32 i = 0;

    for (i = 0; i < m_lines.GetCount(); i++)
    {
        // The true line's start along Y direction is the advance-based offset with adjustment
        // for the line height/stacking strategy.
        XFLOAT lineStartY = lineOffset.y + m_lines[i].Rect.Y;

        if (point.y < lineStartY && i == 0 )
        {
            // Before start of first line - match to first line.
            break;
        }
        else if (point.y < (lineStartY + m_lines[i].VerticalAdvance))
        {
            break;
        }
        else if (i == m_lines.GetCount() - 1)
        {
            // Past the end of the last line - match to last line.
            break;
        }
        lineOffset.y += m_lines[i].VerticalAdvance;
    }
    *pLineOffset = lineOffset;
    return i;
}

//---------------------------------------------------------------------------
//
// ParagraphNode::IsAtInsertionPositionInTextLine
//
//  Synopsis:
//      Queries a TextLine to determine if the specified position is an
//      insertion position. This code path is required for lines with
//      multi-character clusters since they cannot use the simple surrogate/
//      CRLF check.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphNode::IsAtInsertionPositionInTextLine(
    _In_ XUINT32 position,
    _In_ XUINT32 lineStartIndex,
    _In_ TextLine *pTextLine,
    _Out_ bool *pIsAtInsertionPosition
    )
{
    HRESULT hr = S_OK;
    XUINT32 lineEndIndex;
    bool isAtInsertionPosition = true;
    CharacterHit characterHit;

    lineEndIndex = lineStartIndex + pTextLine->GetLength();

    ASSERT(lineStartIndex < lineEndIndex); // Empty line shouldn't have multi character cluster flag.
    ASSERT(lineStartIndex <= position &&
           position <= lineEndIndex);

    // Remove trailing newlines from consideration for insertion positions.
    lineEndIndex -= pTextLine->GetNewlineLength();
    if (lineEndIndex < position)
    {
        isAtInsertionPosition = FALSE;
    }
    else if (lineStartIndex < position &&
            position < lineEndIndex)
    {
        // Start/end text offsets are always insertion positions - do the work
        // to query only if we're not at either endpoint.

        // Infer insertion position by calling the caret next/prev to see
        // if we end up at the same place we started.
        characterHit.firstCharacterIndex = position;
        characterHit.trailingLength = 0;

        IFC(RichTextServicesHelper::MapTxErr(pTextLine->GetNextCaretCharacterHit(characterHit, &characterHit)));
        IFC(RichTextServicesHelper::MapTxErr(pTextLine->GetPreviousCaretCharacterHit(characterHit, &characterHit)));

        if (static_cast<XUINT32>(characterHit.firstCharacterIndex) != position ||
            characterHit.trailingLength != 0)
        {
            isAtInsertionPosition = FALSE;
        }
    }

    *pIsAtInsertionPosition = isAtInsertionPosition;

Cleanup:
    RRETURN(hr);
}
