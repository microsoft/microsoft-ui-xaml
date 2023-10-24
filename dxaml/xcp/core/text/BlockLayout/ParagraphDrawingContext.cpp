// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParagraphDrawingContext.h"
#include "D2DTextDrawingContext.h"
#include <ContentRenderer.h>

//---------------------------------------------------------------------------
//
// Initializes a new instance of the ParagraphDrawingContext.
//
//---------------------------------------------------------------------------
ParagraphDrawingContext::ParagraphDrawingContext(
    _In_ CCoreServices *pCore,
    _In_ ParagraphNode *pNode
    )
    : m_pCore(pCore)
    , m_pTextDrawingContext(NULL)
    , m_pNode(pNode)
{
    XCP_WEAK(&m_pCore);
}

//---------------------------------------------------------------------------
//
// Release resources associated with the ParagraphDrawingContext.
//
//---------------------------------------------------------------------------
ParagraphDrawingContext::~ParagraphDrawingContext()
{
    ReleaseInterface(m_pTextDrawingContext);
}

_Check_return_ HRESULT
ParagraphDrawingContext::HWRenderContent(
    _In_ IContentRenderer* pContentRenderer
    )
{
    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);

    CTransformToRoot localTransformToRoot = *(pContentRenderer->GetTransformToRoot());
    localTransformToRoot.Prepend(m_transform);

    TransformAndClipStack transformsAndClips;
    IFC_RETURN(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(m_transform);

    TransformToRoot2DOverride transformOverride(pContentRenderer, &localTransformToRoot);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    ASSERT(m_pTextDrawingContext != NULL);

    IFC_RETURN(m_pTextDrawingContext->HWRender(pContentRenderer));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Ensures D2D resources
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphDrawingContext::D2DEnsureResources(
    _In_ const D2DPrecomputeParams &cp,
    _In_ const CMILMatrix *pRenderTransform
    )
{
    CMILMatrix localTransform = m_transform;
    localTransform.Append(*pRenderTransform);

    ASSERT(m_pTextDrawingContext != NULL);
    IFC_RETURN(m_pTextDrawingContext->D2DEnsureResources(cp, &localTransform));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Renders content using D2D
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphDrawingContext::D2DRenderContent(
    _In_ const SharedRenderParams &sharedRP,
    _In_ const D2DRenderParams &d2dRP,
    _In_ XFLOAT opacity
    )
{
    CMILMatrix localTransform = m_transform;
    localTransform.Append(*sharedRP.pCurrentTransform);
    SharedRenderParams localSharedRP = sharedRP;
    localSharedRP.pCurrentTransform = &localTransform;

    ASSERT(m_pTextDrawingContext != NULL);
    IFC_RETURN(m_pTextDrawingContext->D2DRender(localSharedRP, d2dRP, opacity));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Gets the TextDrawingContext which is capable of rendering text lines.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphDrawingContext::GetTextDrawingContext(
    _Outptr_ RichTextServices::TextDrawingContext **ppTextDrawingContext
    )
{
    if (m_pTextDrawingContext == NULL)
    {
        CTextCore *pTextCore;
        IFC_RETURN(m_pCore->GetTextCore(&pTextCore));
        m_pTextDrawingContext = new D2DTextDrawingContext(pTextCore);
    }

    *ppTextDrawingContext = m_pTextDrawingContext;

    return S_OK;
}

//---------------------------------------------------------------------------
//
// Invalidates render walk caches.
//
//---------------------------------------------------------------------------
void ParagraphDrawingContext::InvalidateRenderCache()
{
    if (m_pTextDrawingContext != NULL)
    {
        m_pTextDrawingContext->ClearGlyphRunCaches();
    }
}

void ParagraphDrawingContext::ClearForegroundHighlightInfo()
{
    m_pTextDrawingContext->ClearForegroundHighlightInfo();
}

void ParagraphDrawingContext::AppendForegroundHighlightInfo(
    uint32_t count,
    _In_reads_(count) const XRECTF* pRectangles,
    _In_ CSolidColorBrush* foregroundBrush,
    uint32_t startIndex,
    _In_ const XPOINTF& nodeOffset,
    _Out_ uint32_t* pAdvanceCount
    )
{
    XUINT32 startIndexInParagraph = 0;
    XUINT32 countInParagraph = 0;
    bool intersects = false;
    XUINT32 i = 0;

    // With current usage this call should always be made after Arrange so a TextDrawingContext should
    // be guaranteed.
    ASSERT(m_pTextDrawingContext != NULL);
    ASSERT(count == 0 ||
           startIndex < count);

    for (i = startIndex; i < count; i++)
    {
        // Examine each rectangle and determine if it's within the paragraph's render bounds.
        if (pRectangles[i].Y + pRectangles[i].Height/2 >= nodeOffset.y + m_pNode->GetRenderSize().height)
        {
            // If the rectangle starts below the node's end, we're done, and the next node should
            // be examined.
            break;
        }
        else if ((pRectangles[i].Y + pRectangles[i].Height/2) <= nodeOffset.y)
        {
            // If the rectangle ends before the node's start, keep going.
            continue;
        }
        else
        {
            // Otherwise we're within node bounds.
            if (!intersects)
            {
                // Intersection has not yet been detected, but is now. Set the startIndexInParagraph.
                startIndexInParagraph = i;
                intersects = TRUE;
            }
            countInParagraph++;
        }
    }

    // Push information about rects found within the node to the drawing context.
    if (countInParagraph > 0)
    {
        m_pTextDrawingContext->AppendForegroundHighlightInfo(
            count,
            const_cast<XRECTF *>(pRectangles),
            foregroundBrush,
            startIndexInParagraph,
            countInParagraph,
            nodeOffset);
    }

    *pAdvanceCount = (i - startIndex);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The method to clean up all the device related realizations
//      on this object.
//
//-----------------------------------------------------------------------------
void ParagraphDrawingContext::CleanupRealizations()
{
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->CleanupRealizations();
    }
    // The only way we could be here is through node's cleanup.
    // Hence no need call cleanup on node again.
}

void ParagraphDrawingContext::SetBackPlateConfiguration(bool backPlateActive, bool useHyperlinkForeground)
{
    if (m_pTextDrawingContext)
    {
        m_pTextDrawingContext->SetBackPlateConfiguration(backPlateActive, useHyperlinkForeground);
    }
}

void ParagraphDrawingContext::SetControlEnabled(bool enabled)
{
    if (m_pTextDrawingContext)
    {
        m_pTextDrawingContext->SetControlEnabled(enabled);
    }
}

