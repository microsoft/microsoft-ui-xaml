// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <UIThreadScheduler.h>

//------------------------------------------------------------------------
//
//  Method:   CRootVisual::~CRootVisual
//
//------------------------------------------------------------------------
CRootVisual::~CRootVisual()
{
    // The RootVisual is never added to the live tree, so it gets removed from the PC scene (on Leave()).
    // It does have render data, however - the background color brush. Ensure that gets cleaned up.
    if (IsInPCScene())
    {
        ClearPCRenderData();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the default background brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::InitInstance()
{
    HRESULT hr = S_OK;

    CREATEPARAMETERS cp(GetContext());

    CSolidColorBrush *pBackgroundBrush = NULL;

    IFC(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(&pBackgroundBrush), &cp));
    IFC(SetValueByKnownIndex(KnownPropertyIndex::Panel_Background, pBackgroundBrush));

Cleanup:
    ReleaseInterfaceNoNULL(pBackgroundBrush);

    RRETURN(hr);
}

bool CRootVisual::AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const
{
    return (iListenerType == REQUEST_INTERNAL) || __super::AllowsHandlerWhenNotLive(iListenerType, eventIndex);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the color of the background brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::SetBackgroundColor(XUINT32 backgroundColor)
{
    ASSERT(m_pBackground != NULL && m_pBackground->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>());

    CValue val;
    val.SetColor(backgroundColor);

    RRETURN(m_pBackground->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, val));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overriding CFrameworkElement virtual to add specific logic to measure pass.
//      This behavior is the same as that of the Canvas
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    desiredSize.width = 0;
    desiredSize.height = 0;

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if(pChild)
        {
            // measure child to the plugin size
            IFC_RETURN(pChild->Measure(availableSize));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overriding CFrameworkElement virtual to add specific logic to arrange pass.
//      The root visual always arranges the children with the finalSize. This ensures that
//      children of the root visual are always arranged at the plugin size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    auto children = GetUnsortedChildren();
    UINT32 count = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < count; ++childIndex)
    {
        CUIElement* pChild = children[childIndex];
        if (!pChild) continue;

        XFLOAT x = pChild->GetOffsetX();
        XFLOAT y = pChild->GetOffsetY();

        if (pChild->GetIsArrangeDirty() || pChild->GetIsOnArrangeDirtyPath())
        {
            IFC(pChild->EnsureLayoutStorage());

            XRECTF childRect = { x, y, finalSize.width, finalSize.height };

            IFC(pChild->Arrange(childRect));
        }
    }

Cleanup:
    newFinalSize = finalSize;

    RRETURN(hr);

}

//------------------------------------------------------------------------
//
//  Synopsis:
//      RootVisual does not support layout clipping - skip to base implementation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::UpdateLayoutClip(bool forceClipToRenderSize)
{
    ASSERT(!forceClipToRenderSize);
    return CUIElement::UpdateLayoutClip(forceClipToRenderSize);
}

void CRootVisual::CleanupCompositionResources()
{
    // Unmark the element as the root of the render walk.
    UnsetRequiresComposition(
        CompositionRequirement::RootElement,
        IndependentAnimationType::None
        );

    // Also tear down all the composition related resources in our entire subtree, including primitives.
    LeavePCSceneRecursive();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the scene becomes dirty and it propagates up to the root element.
//      This override ensures the UI thread will tick again to render the updated scene.
//
//------------------------------------------------------------------------
void
CRootVisual::NWPropagateDirtyFlag(DirtyFlags flags)
{
    // The browser host and/or frame scheduler can be NULL during shutdown.
    IXcpBrowserHost *pBH = GetContext()->GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();

        // There's no need to schedule another tick for tree changes during ticking.
        // Rendering the frame is the last step of ticking, so any changes that occur
        // during the tick will be picked up in the current frame.
        if (pFrameScheduler != NULL && !pFrameScheduler->IsInTick())
        {
            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::RootVisualDirty));
        }
    }

    CPanel::NWPropagateDirtyFlag(flags);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the VisualTree that is associated with this root.
//
//  Note:
//      This does not add ref the visual tree.  The visual tree is responsible
//      for setting this to NULL if the root is no longer associated with it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::SetAssociatedVisualTree(_In_ VisualTree *pVisualTree)
{
    m_pVisualTree = pVisualTree;
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the popup root that is associated with this root.
//
//------------------------------------------------------------------------
CPopupRoot* CRootVisual::GetAssociatedPopupRootNoRef() const
{
    if (m_pVisualTree)
    {
        return m_pVisualTree->GetPopupRoot();
    }
    else
    {
        return GetContext()->GetMainPopupRoot();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the layout manager that is associated with this root.
//
//------------------------------------------------------------------------
CLayoutManager* CRootVisual::GetAssociatedLayoutManager() const
{
    if (m_pVisualTree)
    {
        return m_pVisualTree->GetLayoutManager();
    }
    else
    {
        return GetContext()->GetMainLayoutManager();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the public root that is associated with this hidden root.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRootVisual::GetAssociatedPublicRootNoRef(_Outptr_ CUIElement** ppPublicRoot) const
{
    IFCPTR_RETURN(ppPublicRoot);

    if (m_pVisualTree)
    {
        *ppPublicRoot = m_pVisualTree->GetPublicRootVisual();
    }
    else
    {
        auto core = GetContext();
        IFCEXPECT_ASSERT_RETURN(core);
        *ppPublicRoot = static_cast<CUIElement*>(core->getVisualRoot());
    }

    return S_OK;
}

CUIElement* CRootVisual::GetRootScrollViewerOrCanvas()
{
    if (m_pVisualTree != nullptr)
    {
        CUIElement* rootScrollViewer = m_pVisualTree->GetRootScrollViewer();
        if (rootScrollViewer != nullptr)
        {
            return rootScrollViewer;
        }

        CUIElement* rootCanvas = m_pVisualTree->GetPublicRootVisual();
        ASSERT(rootCanvas == nullptr || rootCanvas->OfTypeByIndex<KnownTypeIndex::Canvas>());
        return rootCanvas;
    }

    return nullptr;
}

CFullWindowMediaRoot* CRootVisual::GetFullWindowMediaRoot()
{
    if (m_pVisualTree != nullptr)
    {
        return m_pVisualTree->GetFullWindowMediaRoot();
    }

    return nullptr;
}

_Check_return_ HRESULT CRootVisual::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* bounds)
{
    //
    // If there are open parented popups, then the element tree looks something like:
    //
    //      <RootVisual>
    //          <PopupRoot>
    //              <Grid ChildOfPopup />
    //          </PopupRoot>
    //          <Border PublicRoot>
    //              <Canvas>
    //                  <Popup Child="ChildOfPopup" />
    //              </Canvas>
    //          </Border>
    //      </RootVisual>
    //
    // If the Canvas gets a different render transform, it'll propagate bounds dirty up the tree to the Border
    // and the RootVisual. But it also moved the position of the Popup and the Grid, which means the
    // PopupRoot's bounds should also be marked dirty (since it contains the Grid).
    //
    // There's no mechanism for propagating dirty flags down the tree, so we'll miss this Popup scenario.
    // The workaround is to always assume that the PopupRoot's bounds are dirty whenever the RootVisual's
    // bounds are dirty. That guarantees that we'll recompute the PopupRoot's bounds, and that we'll correctly
    // pass a point to it for the next bounds pass.
    //
    CPopupRoot* popupRoot = GetAssociatedPopupRootNoRef();

    if (popupRoot != nullptr)
    {
        popupRoot->InvalidateChildBounds();
    }

    IFC_RETURN(__super::GenerateChildOuterBounds(hitTestParams, bounds));

    return S_OK;
}
