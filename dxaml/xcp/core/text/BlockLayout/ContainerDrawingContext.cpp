// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContainerDrawingContext.h"
#include "ContainerNode.h"
#include <ContentRenderer.h>

//---------------------------------------------------------------------------
//
// Initializes a new instance of the ContainerDrawingContext class
//
//---------------------------------------------------------------------------
ContainerDrawingContext::ContainerDrawingContext(
    _In_ ContainerNode *pNode
    )
    : m_pNode(pNode)
{
}

//---------------------------------------------------------------------------
//
// Renders content in PC rendering walk.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ContainerDrawingContext::HWRenderContent(
    _In_ IContentRenderer* pContentRenderer
    )
{
    BlockNode *pChild = m_pNode->GetFirstChild();
    XFLOAT childYOffset = 0.0f;

    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);

    // Call GenerateEdges on DrawingContext of each child. Adjust transform per child to reflect
    // offset of previous children.
    while (pChild != NULL)
    {
        CMILMatrix offsetTransform(TRUE);
        offsetTransform.SetDy(childYOffset);

        CTransformToRoot localTransformToRoot = *(pContentRenderer->GetTransformToRoot());
        localTransformToRoot.Prepend(m_transform);
        localTransformToRoot.Prepend(offsetTransform);

        TransformAndClipStack transformsAndClips;
        IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
        transformsAndClips.PrependTransform(m_transform);
        transformsAndClips.PrependTransform(offsetTransform);

        TransformToRoot2DOverride transformOverride(pContentRenderer, &localTransformToRoot);
        localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

        ASSERT(pChild->GetDrawingContext() != NULL);
        IFC_RETURN(pChild->GetDrawingContext()->HWRenderContent(pContentRenderer));

        // Increase child's y offset by the last child's height and reset
        // child transform to identity so the correct world transform is transferred to the child.
        childYOffset += pChild->GetRenderSize().height;

        pChild = pChild->GetNext();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Ensures D2D resources.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ContainerDrawingContext::D2DEnsureResources(
    _In_ const D2DPrecomputeParams &cp,
    _In_ const CMILMatrix *pRenderTransform
    )
{
    BlockNode *pChild = m_pNode->GetFirstChild();
    CMILMatrix childTransform(TRUE);
    XFLOAT childYOffset = 0.0f;

    // Call D2DEnsureResources on DrawingContext of each child. Adjust transform per child to reflect
    // offset of previous children.
    while (pChild != NULL)
    {
        childTransform.SetDy(childYOffset);
        childTransform.Append(m_transform);
        childTransform.Append(*pRenderTransform);

        ASSERT(pChild->GetDrawingContext() != NULL);
        IFC_RETURN(pChild->GetDrawingContext()->D2DEnsureResources(cp, &childTransform));

        // Increase child's y offset by the last child's height and reset
        // child transform to identity so the correct world transform is transferred to the child.
        childYOffset += pChild->GetRenderSize().height;
        childTransform.SetToIdentity();

        pChild = pChild->GetNext();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Renders content using D2D.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ContainerDrawingContext::D2DRenderContent(
    _In_ const SharedRenderParams &sharedRP,
    _In_ const D2DRenderParams &d2dRP,
    _In_ XFLOAT opacity
    )
{
    BlockNode *pChild = m_pNode->GetFirstChild();
    CMILMatrix childTransform(TRUE);
    SharedRenderParams localSharedRP = sharedRP;
    localSharedRP.pCurrentTransform = &childTransform;
    XFLOAT childYOffset = 0.0f;

    // Call D2DEnsureResources on DrawingContext of each child. Adjust transform per child to reflect
    // offset of previous children.
    while (pChild != NULL)
    {
        childTransform.SetDy(childYOffset);
        childTransform.Append(m_transform);
        childTransform.Append(*sharedRP.pCurrentTransform);

        ASSERT(pChild->GetDrawingContext() != NULL);
        IFC_RETURN(pChild->GetDrawingContext()->D2DRenderContent(localSharedRP, d2dRP, opacity));

        // Increase child's y offset by the last child's height and reset
        // child transform to identity so the correct world transform is transferred to the child.
        childYOffset += pChild->GetRenderSize().height;
        childTransform.SetToIdentity();

        pChild = pChild->GetNext();
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Invalidates render walk caches.
//
//---------------------------------------------------------------------------
void ContainerDrawingContext::InvalidateRenderCache()
{
    BlockNode *pChild = m_pNode->GetFirstChild();
    while (pChild != NULL)
    {
        if (pChild->GetDrawingContext() != NULL)
        {
            pChild->GetDrawingContext()->InvalidateRenderCache();
        }
        pChild = pChild->GetNext();
    }
}

void ContainerDrawingContext::AppendForegroundHighlightInfo(
    uint32_t count,
    _In_reads_(count) const XRECTF* pRectangles,
    _In_ CSolidColorBrush* foregroundBrush,
    uint32_t startIndex, // Index of first rect within the array that corresponds to this node.
    _In_ const XPOINTF& nodeOffset, // Offset of the node in page space, used to compute relative offsets within a node.
    _Out_ uint32_t* pAdvanceCount // Number of rectangles advanced by this node.
    )
{
    BlockNode *pChild = m_pNode->GetFirstChild();

    // The container's content rendering offset if the offset at which children start rendering.
    // Additionally, each child is translated by the previous children's height.
    XPOINTF thisOffset = nodeOffset;
    XPOINTF childOffset;
    XPOINTF contentRenderingOffset = m_pNode->GetContentRenderingOffset();
    XUINT32 advanceCount = 0;
    thisOffset.x += contentRenderingOffset.x;
    thisOffset.y += contentRenderingOffset.y;

    ASSERT(count == 0 ||
           startIndex < count);

    if (count > 0)
    {
        while (pChild != NULL &&
            startIndex < count)
        {
            childOffset = thisOffset;

            // Include the left & top margins in the child offset. We'll look up the selection highlight bounds when
            // drawing, which goes through BlockNode::Draw and includes the left & top margins. (0, 0) should be where
            // the content starts drawing.
            childOffset.x += pChild->GetContentRenderingOffset().x;
            childOffset.y += pChild->GetContentRenderingOffset().y;

            XUINT32 childAdvanceCount = 0;

            // At this time, child should have a drawing context.
            ASSERT(pChild->GetDrawingContext() != NULL);

            pChild->GetDrawingContext()->AppendForegroundHighlightInfo(
                count,
                pRectangles,
                foregroundBrush,
                startIndex,
                childOffset,
                &childAdvanceCount);

            // Increase child's y offset by the last child's height and reset
            // child transform to identity so the correct world transform is transferred to the child.
            thisOffset.y += pChild->GetRenderSize().height;

            // Advance the start index.
            advanceCount += childAdvanceCount;
            startIndex += childAdvanceCount;

            ASSERT(startIndex <= count);

            pChild = pChild->GetNext();
        }
    }

    *pAdvanceCount = advanceCount;
}

//---------------------------------------------------------------------------
//
// Clears selection highlight context from children.
//
//---------------------------------------------------------------------------
void ContainerDrawingContext::ClearForegroundHighlightInfo()
{
    BlockNode *pChild = m_pNode->GetFirstChild();
    while (pChild != NULL)
    {
        if (pChild->GetDrawingContext())
        {
            pChild->GetDrawingContext()->ClearForegroundHighlightInfo();
        }
        pChild = pChild->GetNext();
    }
}

void ContainerDrawingContext::SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground)
{
    BlockNode *pChild = m_pNode->GetFirstChild();

    while (pChild != nullptr)
    {
        auto drawingContext = pChild->GetDrawingContext();

        if (drawingContext)
        {
            drawingContext->SetBackPlateConfiguration(backPlateActive, useHyperlinkForeground);
        }

        pChild = pChild->GetNext();
    }
}

void ContainerDrawingContext::SetControlEnabled(bool enabled)
{
    BlockNode *pChild = m_pNode->GetFirstChild();

    while (pChild != nullptr)
    {
        auto drawingContext = pChild->GetDrawingContext();

        if (drawingContext)
        {
            drawingContext->SetControlEnabled(enabled);
        }

        pChild = pChild->GetNext();
    }
}
