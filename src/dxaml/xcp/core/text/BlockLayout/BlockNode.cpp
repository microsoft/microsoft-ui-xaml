// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BlockNode.h"
#include "BlockLayoutEngine.h"
#include "BlockNodeBreak.h"
#include "ContainerNode.h"
#include "BlockLayoutHelpers.h"
#include "DrawingContext.h"

BlockNode::BlockNode(
    _In_ BlockLayoutEngine *pBlockLayoutEngine,
    _In_opt_ CDependencyObject *pElement,
    _In_opt_ ContainerNode *pParentNode
    ) :
    m_pBlockLayoutEngine(pBlockLayoutEngine),
    m_pElement(pElement),
    m_pParentNode(pParentNode),
    m_pBreak(NULL),
    m_pPreviousBreak(NULL),
    m_pNext(NULL),
    m_pPrevious(NULL),
    m_length(0),
    m_pDrawingContext(NULL),
    m_isContentDirty(FALSE),
    m_isMeasureDirty(TRUE),
    m_isArrangeDirty(TRUE),
    m_isMeasureInProgress(FALSE),
    m_isArrangeInProgress(FALSE),
    m_isDrawInProgress(FALSE),
    m_isEmptyContentAllowed(FALSE),
    m_measureBottomless(FALSE),
    m_isMeasureBypassed(FALSE),
    m_isArrangeBypassed(FALSE),
    m_cachedMaxLines(0),
    m_measuredLines(0)
{
    ASSERT(pBlockLayoutEngine != NULL);

    m_desiredSize.width  = 0;
    m_desiredSize.height = 0;
    m_renderSize.width   = 0;
    m_renderSize.height  = 0;
    m_prevAvailableSize.width = XFLOAT_INF;
    m_prevAvailableSize.height = XFLOAT_INF;
    EmptyRectF(&m_margin);
}

BlockNode::~BlockNode()
{
    ReleaseInterface(m_pBreak);
    ReleaseInterface(m_pPreviousBreak);
}

//---------------------------------------------------------------------------
//
// BlockNode::Measure
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockNode::Measure(
    _In_ XSIZEF availableSize,
    _In_ XUINT32 maxLines,
    _In_ MarginCollapsingState mcsIn,
    _In_ bool allowEmptyContent,
    _In_ bool measureBottomless,
    _In_ bool suppressTopMargin,
    _In_opt_ BlockNodeBreak *pPreviousBreak,
    _Out_opt_ MarginCollapsingState *pMcsOut
    )
{
    HRESULT hr = S_OK;
    bool canBypassMeasure = false;
    XTHICKNESS margin;

    // BlockLayoutEngine doesn't support Measure during Measure.
    ASSERT(!IsMeasureInProgress());

    // Perform margin collapsing.
    // Left and Right margin values are untouched. 
    // Bottom margin is always set to 0, since it is handled during margin collapsing by the next block.
    // Top margin is set to maximum of previous value from margin collapsing state and the currently applied margin. 
    // However, if this is the first block on the page (suppressTopMargin is true), margin is always set to 0.
    BlockLayoutHelpers::GetBlockMargin(m_pElement, &margin);
    m_margin.left   = margin.left;
    m_margin.right  = margin.right;
    m_margin.top    = suppressTopMargin ? 0.0f : MAX(margin.top, mcsIn);
    m_margin.bottom = 0.0f;
    if (pMcsOut != NULL)
    {
        *pMcsOut = margin.bottom;
    }

    // Extract collapsed margin values from the availableSize.
    availableSize.width  = MAX(0.0f, availableSize.width  - (m_margin.left + m_margin.right));
    availableSize.height = MAX(0.0f, availableSize.height - (m_margin.top + m_margin.bottom));

    IFC(CanBypassMeasure(availableSize, maxLines, allowEmptyContent, measureBottomless, pPreviousBreak, &canBypassMeasure));

    m_prevAvailableSize = availableSize;
    ReleaseInterface(m_pPreviousBreak);
    m_pPreviousBreak = pPreviousBreak;
    AddRefInterface(m_pPreviousBreak);
    m_isEmptyContentAllowed = allowEmptyContent;
    m_measureBottomless = measureBottomless;

    // BlockNode will execute MeasureCore if:
    // 1. Measure was marked dirty/invalid.
    // 2. Measure was not marked dirty, but cannot be bypassed as determined by bypass checks implemented by the node.
    if (!IsMeasureInProgress() &&
        !canBypassMeasure)
    {
        m_isMeasureInProgress = TRUE;

        IFC(MeasureCore(availableSize,
                        maxLines,
                        allowEmptyContent,
                        measureBottomless,
                        suppressTopMargin,
                        pPreviousBreak));
    }

Cleanup:
    if (hr == RichTextServices::INTERNAL_ERROR)
    {
        m_length = 0;
    }
    m_isMeasureInProgress = FALSE;
    // Invalidating content invalidates measure, and content dirty can be reset here.
    m_isContentDirty = FALSE;
    m_isMeasureDirty = FALSE;
    m_isMeasureBypassed = canBypassMeasure;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockNode::Arrange
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockNode::Arrange(
    _In_ XSIZEF finalSize
)
{
    HRESULT hr = S_OK;
    bool canBypassArrange = false;

    // BlockLayoutEngine doesn't support Arrange during Arrange.
    ASSERT(!IsArrangeInProgress());

    // Extract collapsed margin values from the availableSize.
    finalSize.width  = MAX(0.0f, finalSize.width  - (m_margin.left + m_margin.right));
    finalSize.height = MAX(0.0f, finalSize.height - (m_margin.top + m_margin.bottom));

    // Arrange cannot be bypassed if:
    // 1. Arrange was marked dirty/invalid.
    // 2. Arrange was not marked dirty, but cannot be bypassed as determined by bypass checks implemented by the node.
    // 3. Measure was not bypassed. Anything that was measured must be arranged. m_isMeasureBypassed tracks this 
    // internally instead of using the public InvalidateArrange call which propagates invalidations to children, etc.
    canBypassArrange = CanBypassArrange(finalSize) && m_isMeasureBypassed;

    if (!IsArrangeInProgress() &&
        !canBypassArrange)
    {
        m_isArrangeInProgress = TRUE;

        IFC(ArrangeCore(finalSize));
    }

Cleanup:
    m_isArrangeDirty = FALSE;
    m_isArrangeInProgress = FALSE;
    m_isArrangeBypassed = canBypassArrange;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockNode::Render
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockNode::Draw(
    _In_ bool forceDraw
        // Forces re-rendering even if normal bypass rules apply.
    )
{
    HRESULT hr = S_OK;

    // BlockLayoutEngine doesn't support Render during Render or any other stage pf layout.
    ASSERT(!IsMeasureInProgress() &&
           !IsArrangeInProgress() &&
           !IsDrawInProgress());

    // Render is only bypassed if Arrange was. There is no independent bypass for Render.
    // Re-rendering may be required in some cases even when Arrange is bypassed, e.g. high contrast selection.
    // Hence the forceRender flag.
    // Additionally, Measure and Arrange must always be valid. 
    if (!IsDrawInProgress() &&
        !IsMeasureDirty() &&
        !IsArrangeDirty() &&
        (forceDraw || !m_isArrangeBypassed))
    {
        m_isDrawInProgress = TRUE;

        ASSERT(m_pDrawingContext != NULL);

        // Content rendering offset has been already calculated during layout. 
        // Apply this offset to the drawing context to position rendered content appropriately.
        XPOINTF contentRenderingOffset = GetContentRenderingOffset();
        CMILMatrix offsetTransform(TRUE);
        offsetTransform.SetDx(contentRenderingOffset.x);
        offsetTransform.SetDy(contentRenderingOffset.y);
        m_pDrawingContext->SetTransform(offsetTransform);

        IFC(DrawCore(forceDraw));
        m_isArrangeBypassed = TRUE;
    }

Cleanup:
    m_isDrawInProgress = FALSE;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockNode::InvalidateContent
//
//---------------------------------------------------------------------------
void BlockNode::InvalidateContent()
{
    if (!IsMeasureInProgress())
    {
        m_isContentDirty = TRUE;
        m_isMeasureDirty = TRUE;
        m_isArrangeDirty = TRUE;
        BlockNode *pChildNode = GetFirstChild();
        while (pChildNode != NULL)
        {
            pChildNode->InvalidateContent();
            pChildNode = pChildNode->GetNext();
        }
    }
}

//---------------------------------------------------------------------------
//
// BlockNode::InvalidateMeasure
//
//---------------------------------------------------------------------------
void BlockNode::InvalidateMeasure()
{
    if (!IsMeasureInProgress())
    {
        m_isMeasureDirty = TRUE;
        m_isArrangeDirty = TRUE;
        BlockNode *pChildNode = GetFirstChild();
        while (pChildNode != NULL)
        {
            pChildNode->InvalidateMeasure();
            pChildNode = pChildNode->GetNext();
        }
    }
}

//---------------------------------------------------------------------------
//
// BlockNode::InvalidateArrange
//
//---------------------------------------------------------------------------
void BlockNode::InvalidateArrange()
{
    if (!IsArrangeInProgress())
    {
        m_isArrangeDirty = TRUE;
        BlockNode *pChildNode = GetFirstChild();
        while (pChildNode != NULL)
        {
            pChildNode->InvalidateArrange();
            pChildNode = pChildNode->GetNext();
        }
    }
}

//---------------------------------------------------------------------------
//
// BlockNode::TransformOffsetFromRoot
//
// Synopsis:
//      Transforms coordinates from root-relative to block-relative i.e.
//      from the coordinate system of the highest-level block parent, usually
//      PageNode, to the block's coordinate system.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockNode::TransformOffsetFromRoot(
    _In_ const XPOINTF& offset, 
    _Out_ XPOINTF *pTransformedOffset
    )
{
    XPOINTF origin = {0.0f, 0.0f};

    IFC_RETURN(TransformOffsetToRoot(origin, &origin));

    // Subtract the origin's transformed offset from the value passed in to get the relative offset.
    // Results are MAXed with 0 so we don't return a negative point, but don't bother to MIN with desired size, etc.
    // This is an internal API so we expect callers to be reasonable with input.
    (*pTransformedOffset).x = MAX(offset.x - origin.x, 0.0f);
    (*pTransformedOffset).y = MAX(offset.y - origin.y, 0.0f);

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockNode::TransformOffsetToRoot
//
// Synopsis:
//      Transforms coordinates from block-relative to root-relative i.e.
//      to the coordinate system of the highest-level block parent, usually
//      PageNode.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockNode::TransformOffsetToRoot(
    _In_ const XPOINTF& offset, 
    _Out_ XPOINTF *pTransformedOffset
    )
{
    HRESULT hr = S_OK;
    XPOINTF contentOffset = GetContentRenderingOffset();
    XPOINTF transformedOffset = {offset.x + contentOffset.x, offset.y + contentOffset.y };
    XPOINTF childOffset = {0.0f, 0.0f};
    ContainerNode *pParent = m_pParentNode;
    BlockNode *pChild = this;

    while (pParent != NULL)
    {
        childOffset = pParent->GetChildOffset(pChild);
        transformedOffset.x += childOffset.x;
        transformedOffset.y += childOffset.y;
        pChild = pParent;
        pParent = pParent->m_pParentNode;
    }

    *pTransformedOffset = transformedOffset;

    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockNode::GetContentRenderingOffset
//
// Synopsis:
//      Returns the pixel offset at which content is rendered at this node.
//      This is not an absolute offset or parent-relative offset. Those 
//      offsets are handled by the parent. This offset determines where
//      content is drawn within the block's coordinate space and is
//      meant to account for padding, border, etc.
//
//---------------------------------------------------------------------------
XPOINTF BlockNode::GetContentRenderingOffset() const
{
    XPOINTF offset = { m_margin.left, m_margin.top };
    return offset;
}

RichTextServices::FlowDirection::Enum BlockNode::GetFlowDirection() const
{
    return BlockLayoutHelpers::GetFlowDirection(m_pBlockLayoutEngine->GetOwner());
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The method to clean up all the device related realizations on this subtree.
//
//-----------------------------------------------------------------------------
void BlockNode::CleanupRealizations()
{
    BlockNode *pChild = GetFirstChild();
    while (pChild)
    {
        pChild->CleanupRealizations();
        pChild = pChild->GetNext();
    }
    if (m_pDrawingContext != nullptr)
    {
        m_pDrawingContext->CleanupRealizations();
    }
}

_Check_return_ HRESULT BlockNode::CanBypassMeasure(
    _In_ XSIZEF availableSize,
    _In_ XUINT32 maxLines,
    _In_ bool allowEmptyContent,
    _In_ bool measureBottomless,
    _In_opt_ BlockNodeBreak *pPreviousBreak,
    _Out_ bool *pCanBypass
    )
{
    // BlockNode is the base class and performs the most conservative layout bypass check since it
    // can't make any assumptions about layout behavior - strict equality of all parameters.
    HRESULT hr = S_OK;
    bool bypass = false;
    if (!IsMeasureDirty() &&
        (IsEmptyContentAllowed() == allowEmptyContent) &&
        (IsMeasureBottomless() == measureBottomless) &&
        IsCloseReal(m_prevAvailableSize.width, availableSize.width) &&
        (m_cachedMaxLines == maxLines) &&
        BlockNodeBreak::Equals(pPreviousBreak, m_pPreviousBreak))
    {
        if (m_pBreak != NULL)
        {
            // If there is a break for this paragraph, then height constraint needs to be
            // the same to break at the same place.
            bypass = IsCloseReal(availableSize.height, m_prevAvailableSize.height);
        }
        else
        {
            // If there was no break, we can bypass as long as the desired height will fit
            // in the available space.
            bypass = (m_desiredSize.height  <= availableSize.height); 
        }
    }
    
    *pCanBypass = bypass;

    RRETURN(hr);
}

bool BlockNode::CanBypassArrange(
    _In_ XSIZEF finalSize
    )
{
    // BlockNode is the base class and performs the most conservative layout bypass check since it
    // can't make any assumptions about layout behavior - strict equality of all parameters.
    return (!IsArrangeDirty() &&
            IsCloseReal(m_renderSize.width, finalSize.width) &&
            IsCloseReal(m_renderSize.height, finalSize.height));
}