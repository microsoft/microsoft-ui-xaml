// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLocalTransformBuilder.h"
#include <D3D11Device.h>
#include <WindowsGraphicsDeviceManager.h>
#include <RootScale.h>

_Check_return_ HRESULT
CRenderTargetElementData::Create(
    _In_ CUIElement* renderElement,
    int32_t scaledWidth,
    int32_t scaledHeight,
    _In_ ICoreAsyncAction* renderAction,
    _In_ RenderTargetBitmapImplBase* renderTargetBitmapImpl,
    _Outptr_ CRenderTargetElementData** renderTargetElementDataOut
    )
{
    xref_ptr<CRenderTargetElementData> renderTargetElementData;
    renderTargetElementData.attach(
        new CRenderTargetElementData(
            renderElement,
            renderAction,
            scaledWidth,
            scaledHeight,
            renderTargetBitmapImpl));

    IFC_RETURN(renderTargetElementData->Initialize());

    *renderTargetElementDataOut = renderTargetElementData.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ctor
//
//------------------------------------------------------------------------
CRenderTargetElementData::CRenderTargetElementData(
    _In_ CUIElement *pRenderElement,
    _In_ ICoreAsyncAction *pRenderAction,
    _In_ XINT32 scaledWidth,
    _In_ XINT32 scaledHeight,
    _In_ RenderTargetBitmapImplBase* renderTargetBitmapImpl) :
    m_pRenderElement(pRenderElement),
    m_pConnectionElement(NULL),
    m_pixelWidth(0),
    m_pixelHeight(0),
    m_layoutWidth(0),
    m_layoutHeight(0),
    m_pRenderAsyncAction(pRenderAction),
    m_pPrependTransformAndClip(NULL),
    m_scaleWidth(scaledWidth),
    m_scaleHeight(scaledHeight),
    m_renderTargetBitmapImplNoRef(renderTargetBitmapImpl)
{
    AddRefInterface(pRenderElement);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Dtor
//
//------------------------------------------------------------------------
CRenderTargetElementData::~CRenderTargetElementData()
{
    ResetState();

    // Detach the connection element from RenderTargetBitmapRoot,
    // if it was previously attached.
    if (m_pConnectionElement != NULL)
    {
        CCoreServices *pCoreNoRef = m_pRenderElement->GetContext();
        ASSERT(pCoreNoRef->IsInBackgroundTask());
        CRenderTargetBitmapRoot *pRenderTargetBitmapRoot = pCoreNoRef->GetMainRenderTargetBitmapRoot();
        ASSERT(pRenderTargetBitmapRoot != NULL && pRenderTargetBitmapRoot->IsElementAttached(m_pConnectionElement));
        VERIFYHR(pRenderTargetBitmapRoot->DetachElement(m_pConnectionElement));
        ReleaseInterface(m_pConnectionElement);
    }

    ReleaseInterface(m_pRenderElement);
    ReleaseInterface(m_pCompositorTreeHost);
    SAFE_DELETE(m_pPrependTransformAndClip);
    ReleaseRenderAsyncAction();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize method
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetElementData::Initialize()
{
    CCoreServices *pCoreNoRef = m_pRenderElement->GetContext();
    if (pCoreNoRef->IsInBackgroundTask())
    {
        CUIElement *pCurrent = m_pRenderElement;
        CUIElement *pConnectionElement = NULL;

        CRenderTargetBitmapRoot *pRenderTargetBitmapRoot = pCoreNoRef->GetMainRenderTargetBitmapRoot();
        ASSERT(pRenderTargetBitmapRoot != NULL);

        CUIElement *pPendingRootVisual = NULL;
        if (pCoreNoRef->GetDeployment() &&
            pCoreNoRef->GetDeployment()->m_pApplication)
        {
            pPendingRootVisual = pCoreNoRef->GetDeployment()->m_pApplication->m_pRootVisual;
        }

        // Determine if the element to be captured is not under root visual.
        // If so determine the ancestor element which needs to hooked up
        // under RenderTargetBitmapRoot and hook it.
        //
        // If any of the ancestors are deemed to be ineligible for capture,
        // then ingore. The error will be raised at a later point of time
        // along with live trees and foreground apps.
        while (pCurrent)
        {
            // Popup rendering in background task was never tested so disabling it here as it is not a required scenario.
            if (pCurrent->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
            {
                IFC_RETURN(E_INVALIDARG);
            }

            if (pCurrent->OfTypeByIndex<KnownTypeIndex::RootVisual>() ||
                !CRenderTargetBitmap::IsAncestorTypeEligible(pCurrent))
            {
                break;
            }

            // If the tree to be captured is a part
            // of pending Window's content then
            // skip adding it to the tree.
            if (pCurrent == pPendingRootVisual)
            {
                break;
            }

            CDependencyObject *pParent = pCurrent->GetParentInternal(false);
            if (pParent == NULL)
            {
                pConnectionElement = pCurrent;
                break;
            }
            if (pParent->OfTypeByIndex<KnownTypeIndex::RenderTargetBitmapRoot>())
            {
                ASSERT(pRenderTargetBitmapRoot->IsElementAttached(pCurrent));
                pConnectionElement = pCurrent;
                break;
            }
            if (!pParent->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                break;
            }

            pCurrent = static_cast<CUIElement *>(pParent);
        }

        // In background tasks hook non-live trees
        // under RenderTargetBitmapRoot.
        if (pConnectionElement != NULL)
        {
            IFC_RETURN(pRenderTargetBitmapRoot->AttachElement(pConnectionElement));
            SetInterface(m_pConnectionElement, pConnectionElement);
        }
    }


    IFC_RETURN(CompositorTreeHost::Create(&m_pCompositorTreeHost));
    IFC_RETURN(TransformAndClipStack::Create(NULL, &m_pPrependTransformAndClip));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the composition peer for the given element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetElementData::GetCompositionPeer(_In_ CUIElement *pElement, _Outptr_ HWCompTreeNode **ppCompositionPeer) const
{
    *ppCompositionPeer = NULL;
    RRETURN(m_mapUIElementNoRefToCompTreeNode.Get(pElement, *ppCompositionPeer));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures the composition peer for the given element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetElementData::EnsureCompositionPeer(
    _In_ CUIElement *pElement,
    _In_ HWCompTreeNode *pCompositionPeer,
    _In_opt_ HWCompTreeNode *pParentNode,
    _In_ HWCompNode *pPreviousSibling)
{
    HRESULT hr = S_OK;
    HWCompTreeNode *pCompTreeNode = NULL;
    HWCompTreeNode *pOriginalCompTreeNodeNoRef = NULL;
    IFC(GetCompositionPeer(pElement, &pOriginalCompTreeNodeNoRef));
    SetInterface(pCompTreeNode, pOriginalCompTreeNodeNoRef);
    IFC(CUIElement::EnsureCompositionPeer(
        pElement->GetDCompTreeHost(),
        pCompositionPeer,
        pParentNode,
        pPreviousSibling,
        nullptr /* previousSiblingVisual */,
        nullptr /* element */,
        &pCompTreeNode));

    if (pCompTreeNode != pOriginalCompTreeNodeNoRef)
    {
        ReleaseInterface(pOriginalCompTreeNodeNoRef);
        HWCompTreeNode *pOutNode = NULL;
        IGNOREHR(m_mapUIElementNoRefToCompTreeNode.Remove(pElement, pOutNode));
        IFC(m_mapUIElementNoRefToCompTreeNode.Add(pElement, pCompTreeNode));
        AddRefInterface(pCompTreeNode);
    }
Cleanup:
    ReleaseInterface(pCompTreeNode);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the prepend transform and pixel lengths.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRenderTargetElementData::UpdateMetrics()
{
    ASSERT(m_pRenderElement != NULL);
    ASSERT(m_scaleWidth >= 0);
    ASSERT(m_scaleHeight >= 0);

    CMILMatrix scaleTransform(TRUE);
    CMILMatrix zoomScaleTransform(TRUE);
    CMILMatrix localTransformInverse;
    XRECTF_RB elementBounds = {};
    XRECTF_RB elementBoundsScaled = {};
    XRECTF_RB elementBoundsZoomScaled = {};

    // For RTB the maximum texture size is the minimum of
    // max texture size supported by device and 4096.
    // We limit at 4096 as that should be sufficient for capturing the entire screen.
    CCoreServices *pCoreNoRef = m_pRenderElement->GetContext();
    XFLOAT maxTextureSize = static_cast<XFLOAT>(MIN(pCoreNoRef->GetMaxTextureSize(), 4096));

    XFLOAT elementWidth = 0;
    XFLOAT elementHeight = 0;

    if (!CUIElement::IsInTransform3DSubtree(m_pRenderElement, NULL))
    {
        IFC_RETURN(m_pRenderElement->GetInnerBounds(&elementBounds));

        elementWidth = elementBounds.right - elementBounds.left;
        elementHeight = elementBounds.bottom - elementBounds.top;
    }
    else
    {
        // TODO: https://task.ms/3310775: Ensure GetInnerBounds returns correct bounds
        // If we're in a 3D subtree, InnerBounds is not a valid measure of element size since
        // bounds are relative to the nearest 3D ancestor (rather than the parent, per 2D).
        // Instead, we'll explicitly request element size.
        // NOTE: this will cause elements rendered outside the {0, 0, Width, Height} bounding
        //       rectangle to be unaccounted for (and therefore not rendered correctly)
        const XSIZEF elementSize = m_pRenderElement->NWComputeElementSize();
        elementWidth = elementSize.width;
        elementHeight = elementSize.height;

        elementBounds.left = elementBounds.top = 0;
        elementBounds.right = elementWidth;
        elementBounds.bottom = elementHeight;
    }

    XFLOAT scaleX = 1;
    XFLOAT scaleY = 1;
    XFLOAT zoomScale = RootScale::GetRasterizationScaleForElement(m_pRenderElement);
    zoomScaleTransform.Scale(zoomScale,zoomScale);

    if (m_scaleWidth != 0 && elementWidth > 0)
    {
        scaleX = static_cast<XFLOAT>(m_scaleWidth)/elementWidth;
    }
    if (m_scaleHeight != 0 && elementHeight > 0)
    {
        scaleY = static_cast<XFLOAT>(m_scaleHeight)/elementHeight;
    }
    if (m_scaleWidth == 0)
    {
        scaleX = scaleY;
    }
    if (m_scaleHeight == 0)
    {
        scaleY = scaleX;
    }

    // Change the scale factors so as to restrict the texture
    // size to the maximum allowed.
    // 1) First we scale down to fit the width and maintain
    //     the requested aspect ratio.
    // 2) Then we check if the height fits.
    // 3) If it does then all is well.
    // 4) If it does not then we scale down the original scale
    //     factors to fit the height and maintain the requested
    //     aspect ratio. The width should definitely fit this time.
    XFLOAT originalScaleX = scaleX;
    XFLOAT originalScaleY = scaleY;
    if (elementWidth * scaleX * zoomScale > maxTextureSize)
    {
        scaleX = maxTextureSize / (elementWidth * zoomScale);
        scaleY = originalScaleY * scaleX / originalScaleX;
    }
    if (elementHeight * scaleY * zoomScale > maxTextureSize)
    {
        scaleY = maxTextureSize / (elementHeight * zoomScale);
        scaleX = originalScaleX * scaleY / originalScaleY;
    }
    scaleTransform.Scale(scaleX, scaleY);

    // When rendering, we don't care about properties set by composition on the handoff visual. That will prevent problems like
    // creating a tiny mask because there's a WUC scale animation going on.
    if (!m_pRenderElement->GetLocalTransform(TransformRetrievalOptions::None, &localTransformInverse))
    {
        if (!localTransformInverse.Invert())
        {
            // TODO: RTB Is this appropriate?
            IFC_RETURN(E_FAIL);
        }
    }
    else
    {
        localTransformInverse.SetToIdentity();
    }

    // Apply the scale transform first, then apply
    // tranlation if needed and then apply
    // the inverse of local transform on the element.
    m_pPrependTransformAndClip->Reset();
    m_pPrependTransformAndClip->PrependTransform(zoomScaleTransform);
    m_pPrependTransformAndClip->PrependTransform(scaleTransform);
    if (elementBounds.left != 0 || elementBounds.top != 0)
    {
        CMILMatrix translateTransform(true);
        translateTransform.SetDx(-elementBounds.left);
        translateTransform.SetDy(-elementBounds.top);
        m_pPrependTransformAndClip->PrependTransform(translateTransform);
    }

    // Scale the element bounds to determine the layout bounds.
    scaleTransform.TransformBounds(&elementBounds, &elementBoundsScaled);
    m_layoutWidth = static_cast<XUINT32>(XcpCeiling(elementBoundsScaled.right - elementBoundsScaled.left));
    m_layoutHeight = static_cast<XUINT32>(XcpCeiling(elementBoundsScaled.bottom - elementBoundsScaled.top));

    // Top level RTL transform as it would have been inherited from parent during full render walk.
    // Flip in place is required to properly display the render element when it's a parentless popup's child.
    if (m_pRenderElement->IsRightToLeft())
    {
        CMILMatrix parentRtlTransform(true);
        XamlLocalTransformBuilder rtlBuilder(&parentRtlTransform);
        rtlBuilder.ApplyFlowDirection(true /*flipRTL*/, true /*flipRTLInPlace*/, elementBoundsScaled.right - elementBoundsScaled.left);
        m_pPrependTransformAndClip->PrependTransform(parentRtlTransform);
    }

    // Inverse local transform to return the element back to (0,0) position in RTB
    // when combined with forward local transform during HW comp tree walk.
    m_pPrependTransformAndClip->PrependTransform(localTransformInverse);

    // Zoom scale the scaled element bounds to determine the pixel bounds.
    zoomScaleTransform.TransformBounds(&elementBoundsScaled,&elementBoundsZoomScaled);
    m_pixelWidth = static_cast<XUINT32>(XcpCeiling(elementBoundsZoomScaled.right - elementBoundsZoomScaled.left));
    m_pixelHeight = static_cast<XUINT32>(XcpCeiling(elementBoundsZoomScaled.bottom - elementBoundsZoomScaled.top));

    // Max texture size should already be accommodated
    // in the scale factors above. Here we compensate for
    // any math errors and strictly enforce the max texture size.
    m_pixelWidth = MIN(m_pixelWidth, static_cast<XUINT32>(maxTextureSize));
    m_pixelHeight = MIN(m_pixelHeight, static_cast<XUINT32>(maxTextureSize));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets completion on the render async action and releases its ref.
//
//------------------------------------------------------------------------
void CRenderTargetElementData::CompleteRenderAsyncAction(HRESULT resultHR)
{
    if (m_pRenderAsyncAction != NULL)
    {
        m_pRenderAsyncAction->CoreSetError(resultHR);
        m_pRenderAsyncAction->CoreFireCompletion();
        ReleaseRenderAsyncAction();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release the ref of the render async action
//
//------------------------------------------------------------------------
void CRenderTargetElementData::ReleaseRenderAsyncAction()
{
    if (m_pRenderAsyncAction != NULL)
    {
        m_pRenderAsyncAction->CoreReleaseRef();
        m_pRenderAsyncAction = NULL;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Saves a ref to the passed in realization.
//      This is realization would have been created specially for RenderTargetBitmap
//      and we need to keep it alive until we are done with the draw.
//
//------------------------------------------------------------------------
void CRenderTargetElementData::SaveHWRealization(_In_ HWRealization* const pRealization)
{
    m_realizations.push_back(pRealization);
    AddRefInterface(pRealization);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release references to all the saved realizations.
//
//------------------------------------------------------------------------
void
CRenderTargetElementData::ClearHWRealizations()
{
    for (HWRealization* realization : m_realizations)
    {
        ReleaseInterface(realization);
    }

    m_realizations.clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release references to all the comp nodes created for this RTB.
//
//------------------------------------------------------------------------
void
CRenderTargetElementData::ClearCompNodeMap()
{
    for (xchainedmap<CUIElement*, HWCompTreeNode*>::const_iterator it = m_mapUIElementNoRefToCompTreeNode.begin();
        it != m_mapUIElementNoRefToCompTreeNode.end();
        ++it)
    {
        HWCompTreeNode* pCompTreeNode = (*it).second;
        ReleaseInterface(pCompTreeNode);
    }
    m_mapUIElementNoRefToCompTreeNode.Clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases all the device related resources such as comp
//      nodes, realizations etc.
//
//------------------------------------------------------------------------
void
CRenderTargetElementData::CleanupDeviceRelatedResources()
{
    ClearCompNodeMap();
    ClearHWRealizations();
    m_pCompositorTreeHost->Cleanup();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the state so that the render target element operation
//      can be retried.
//
//------------------------------------------------------------------------
void
CRenderTargetElementData::ResetState()
{
    ClearCompNodeMap();
    ClearHWRealizations();
    if (m_pCompositorTreeHost != NULL)
    {
        m_pCompositorTreeHost->Cleanup();
    }
    if (m_pPrependTransformAndClip != NULL)
    {
        m_pPrependTransformAndClip->Reset();
    }
    m_layoutWidth = 0;
    m_layoutHeight = 0;
    m_pixelWidth = 0;
    m_pixelHeight = 0;
}
