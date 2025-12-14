// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "SwapChainPanelOwner.h"
#include "RootScale.h"
#include "LoadLibraryAbs.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basic ctor
//
//------------------------------------------------------------------------
CSwapChainPanel::CSwapChainPanel(_In_ CCoreServices *pCore)
    : CGrid(pCore)
    , m_pSwapChainElement(NULL)
    , m_ppRenderChildren(NULL)
    , m_renderChildrenCount(0)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Basic dtor
//
//------------------------------------------------------------------------
CSwapChainPanel::~CSwapChainPanel()
{
    if (m_pSwapChainElement)
    {
        IGNOREHR(m_pSwapChainElement->RemoveParent(this));
        ReleaseInterface(m_pSwapChainElement);
    }

    SAFE_DELETE_ARRAY(m_ppRenderChildren);
}

bool CSwapChainPanel::HasContent() const
{
    return m_pSwapChainElement != NULL && m_pSwapChainElement->HasContent();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Throw exception on setting unsupported property values on SCBP
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
        case KnownPropertyIndex::Panel_Background:
            IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                GetContext(),
                AG_E_SWAPCHAINPANEL_ERROR_SETUNSUPPORTEDPROPERTY,
                args.m_pDP->GetName()));
            break;
        }
    }

    IFC_RETURN(CGrid::SetValue(args));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Entry point to change the swapchain on the CSwapChainPanel
// pSwapChain can be NULL, indicating the user is cleaning up and removing their swapchain
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::SetSwapChain(_In_opt_ IUnknown *pSwapChain)
{
    IFC_RETURN(EnsureSwapChainElement());
    IFC_RETURN(m_pSwapChainElement->SetSwapChain(pSwapChain));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Entry point to set Surface handle on the CSwapChainPanel
//      HANDLE can be NULL, indicating the user is cleaning up panel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::SetSwapChainHandle(_In_opt_ HANDLE swapChainHandle)
{
    IFC_RETURN(EnsureSwapChainElement());
    IFC_RETURN(m_pSwapChainElement->SetSwapChainHandle(swapChainHandle));

    return S_OK;
}

void CSwapChainPanel::SetUseTransparentVisualIfNeeded()
{
    IFCFAILFAST(EnsureSwapChainElement());
    m_pSwapChainElement->SetUseTransparentVisualIfNeeded();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to include the SwapChainElement in the children collection for rendering
//      and hit-testing.
//
//      The SwapChainPanel attaches its swap chain to the composition tree
//      via the SwapChainElement here. This exists purely to simplify the implementation.
//      The SwapChainElement maps to a special type of composition node. It would be
//      difficult to create a composition node for the SCBP that supports adding XAML
//      content (the children) in addition to a swap chain. It's much easier to separate
//      the content types by creating separate elements for them.
//
//      The SwapChainElement is inserted into the collection as the first child, before
//      all the publicly-accessible children, but it doesn't exist in the real element tree
//      otherwise.
//
//------------------------------------------------------------------------
void
CSwapChainPanel::GetChildrenInRenderOrder(
    _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
    _Out_ XUINT32 *puiChildCount
    )
{
    CUIElement **ppPublicChildren = NULL;
    XUINT32 publicChildrenCount = 0;
    CGrid::GetChildrenInRenderOrder(&ppPublicChildren, &publicChildrenCount);

    if (m_pSwapChainElement != NULL)
    {
        const XUINT32 renderChildrenCount = publicChildrenCount + 1;

        if (renderChildrenCount != m_renderChildrenCount)
        {
            SAFE_DELETE_ARRAY(m_ppRenderChildren);
        }

        if (m_ppRenderChildren == NULL)
        {
            m_ppRenderChildren = new CUIElement*[renderChildrenCount];
        }

        m_ppRenderChildren[0] = m_pSwapChainElement;
        for (XUINT32 publicIndex = 0; publicIndex < publicChildrenCount; ++publicIndex)
        {
            m_ppRenderChildren[publicIndex + 1] = ppPublicChildren[publicIndex];
        }

        m_renderChildrenCount = renderChildrenCount;

        *pppUIElements = m_ppRenderChildren;
        *puiChildCount = m_renderChildrenCount;
    }
    else
    {
        *pppUIElements = ppPublicChildren;
        *puiChildCount = publicChildrenCount;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Override to handle render data for the swap chain element child.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params)
{
    if (params.fIsLive)
    {
        // The swap chain element isn't added to the live tree, so it cannot rely on LeaveImpl cleaning up its
        // render data. Explicitly clean it up when the parent leaves. Any other time this element's render data
        // is cleaned (via LeavePCSceneRecursive) the child is cleaned correctly via GetChildrenInRenderOrder.
        if (m_pSwapChainElement != NULL)
        {
            m_pSwapChainElement->LeavePCSceneRecursive();
        }
    }

    IFC_RETURN(CGrid::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      helper function to create SwapChain Element  for the panel
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CSwapChainPanel::EnsureSwapChainElement()
{
    if (m_pSwapChainElement == NULL)
    {
        CREATEPARAMETERS cp(GetContext());
        IFC_RETURN(CSwapChainElement::Create(
            reinterpret_cast<CDependencyObject**>(&m_pSwapChainElement),
            &cp
            ));

        IFC_RETURN(m_pSwapChainElement->AddParent(
            this,
            FALSE /*isPublic*/,
            RENDERCHANGEDPFN(CUIElement::NWSetSubgraphDirty)
            ));
    }

    return S_OK;
}

namespace CoreImports
{
    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Core export for setting the swapchain
    //
    //------------------------------------------------------------------------
    _Check_return_ HRESULT SwapChainPanel_SetSwapChain(_In_ CSwapChainPanel *pSwapChainPanel, _In_opt_ IUnknown *pSwapChain)
    {
        IFC_RETURN(pSwapChainPanel->SetSwapChain(pSwapChain));

        return S_OK;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    // TODO:  Decide if we need SCP to know about swapchain transform and
    // include it in the computation of content bounds.
    if (HasContent())
    {
        pBounds->left = 0.0f;
        pBounds->top = 0.0f;
        pBounds->right = GetActualWidth();
        pBounds->bottom = GetActualHeight();
    }
    else
    {
        EmptyRectF(pBounds);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Hit-testing override behavior:  If we have a swapchain set then
//      we consider this a hit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    if (GetContext()->InvisibleHitTestMode())
    {
        // Normally we only get here if we passed all bounds checks, but this is not the case if InvisibleHitTestMode is
        // enabled. Here we're hit testing invisible elements, which skips all bounds checks. We don't want to blindly
        // return true here, because that would mean that this element will always be hit under InvisibleHitTestMode,
        // regardless of the incoming point. Instead, make an explicit check against this element's layout size.
        //
        // InvisibleHitTestMode is used by FindElementsInHostCoordinates with includeAllElements set to true, which the
        // VS designer uses to hit test elements that are collapsed or have no background.
        XRECTF layoutSize = { 0, 0, GetActualWidth(), GetActualHeight() };
        *pHit = DoesRectContainPoint(layoutSize, target);
    }
    else if (HasContent())
    {
        // TODO:  Decide if SCP should have more sophisticated hit-testing behavior.
        // We could hit-test the swapchain size but we'd also need to know the swapchain
        // transform to do this correctly.
        *pHit = true;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Hit-testing override behavior:  If we have a swapchain set then
//      we consider this a hit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    if (GetContext()->InvisibleHitTestMode())
    {
        // Normally we only get here if we passed all bounds checks, but this is not the case if InvisibleHitTestMode is
        // enabled. Here we're hit testing invisible elements, which skips all bounds checks. We don't want to blindly
        // return true here, because that would mean that this element will always be hit under InvisibleHitTestMode,
        // regardless of the incoming point. Instead, make an explicit check against this element's layout size.
        //
        // InvisibleHitTestMode is used by FindElementsInHostCoordinates with includeAllElements set to true, which the
        // VS designer uses to hit test elements that are collapsed or have no background.
        XRECTF layoutSize = { 0, 0, GetActualWidth(), GetActualHeight() };
        *pHit = target.IntersectsRect(layoutSize);
    }
    else if (HasContent())
    {
        // TODO:  Decide if SCP should have more sophisticated hit-testing behavior.
        // We could hit-test the swapchain size but we'd also need to know the swapchain
        // transform to do this correctly.
        *pHit = true;
    }

    RRETURN(hr);
}


//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CSwapChainPanel::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CGrid::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_pSwapChainElement != NULL)
    {
        m_pSwapChainElement->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    if (cleanupDComp)
    {
        auto guard = m_swapChainVisualLock.lock();
        m_swapChainVisual.Reset();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Entry point for public CompositionScaleX property getter.
//
//-----------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CSwapChainPanel::GetCompositionScaleX(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    CSwapChainPanel *pThis = static_cast<CSwapChainPanel*>(pObject);
    return pThis->GetCompositionScaleImpl(KnownPropertyIndex::SwapChainPanel_CompositionScaleX, pResult);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Entry point for public CompositionScaleY property getter.
//
//-----------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CSwapChainPanel::GetCompositionScaleY(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    CSwapChainPanel *pThis = static_cast<CSwapChainPanel*>(pObject);
    return pThis->GetCompositionScaleImpl(KnownPropertyIndex::SwapChainPanel_CompositionScaleY, pResult);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of getter for CompositionScaleX and CompositionScaleY properties.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::GetCompositionScaleImpl(
    KnownPropertyIndex propIndex,
    _Out_ CValue *pValue
    )
{
    XFLOAT compositionScaleX;
    XFLOAT compositionScaleY;

    if (!IsInPCScene())
    {
        // If this element is in a state of not being drawn, we will not have a
        // cached HWRealization.  In this case we'll recompute the value.
        IFC_RETURN(ComputeCompositionScale(&compositionScaleX, &compositionScaleY));
    }
    else
    {
        // Just return the cached value from our HWRealization.
        HWRealization *pHWRealization = GetHWRealizationCache();
        pHWRealization->GetRealizationScale(&compositionScaleX, &compositionScaleY);
    }
    switch (propIndex)
    {
        case KnownPropertyIndex::SwapChainPanel_CompositionScaleX:
            pValue->SetFloat(compositionScaleX);
            break;

        case KnownPropertyIndex::SwapChainPanel_CompositionScaleY:
            pValue->SetFloat(compositionScaleY);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a HWRealization, creating one and caching it if necessary.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::EnsureHWRealization(_Outptr_ HWRealization **ppHWRealization)
{
    HRESULT hr = S_OK;

    HWRealization *pHWRealization = GetHWRealizationCache();
    if (pHWRealization == NULL)
    {
        // Create a HWRealization.
        // This HWRealization doesn't have any actual resources,
        // it is just a facade for us to store and upate the composition scale.
        pHWRealization = new HWRealization(HWRealizationType::Facade);
        SetHWRealizationCache(pHWRealization);
    }
    else
    {
        AddRefInterface(pHWRealization);
    }
    *ppHWRealization = pHWRealization;

    RRETURN(hr);//RRETURN_REMOVAL
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the CompositionScaleX and CompositionScaleY values.
//      Used only in the scenario where the element has not been drawn.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::ComputeCompositionScale(
    _Out_ XFLOAT *pCompositionScaleX,
    _Out_ XFLOAT *pCompositionScaleY
    )
{
    ASSERT(!IsInPCScene());

    if (!IsActive())
    {
        const auto scale = RootScale::GetRasterizationScaleForElementWithFallback(this);

        // If this element is not in the live tree,
        // use the plateau scale as the composition scale
        *pCompositionScaleX = scale;
        *pCompositionScaleY = scale;
    }
    else
    {
        // Retrieve the realization transform.  This computes a transform to root
        // the same way that HWWalk computes it for HWRealization purposes.
        CTransformToRoot realizationTransform;
        IFC_RETURN(ComputeRealizationTransform(&realizationTransform));
        IFC_RETURN(realizationTransform.GetScaleDimensions(this, pCompositionScaleX, pCompositionScaleY));
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Updates and caches the CompositionScale matrix computed by the render walk.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::UpdateCompositionScale(
    _In_ const CMILMatrix& compositionScale,
    bool isTransformAnimating
    )
{
    HRESULT hr = S_OK;

    bool creatingHWRealization = (GetHWRealizationCache() == NULL);
    HWRealization *pHWRealization = NULL;

    IFC(EnsureHWRealization(&pHWRealization));

    if (pHWRealization->NeedsUpdate(&compositionScale, isTransformAnimating) || creatingHWRealization)
    {
        pHWRealization->UpdateRealizationParameters(&compositionScale, isTransformAnimating, 0, 0, false /* renderCollapsedMask */);

        // Raise the event to let listeners know the composition scale has changed.
        // This should only happen under 2 conditions:
        // 1) On the initial draw (can also happen if the element is collapsed then re-shown).
        // 2) If the composition scale changes, but not during an animation
        IFC(RaiseCompositionScaleChangedEvent());
    }

Cleanup:
    ReleaseInterfaceNoNULL(pHWRealization);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Rasies the CompositionScaleChanged event
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainPanel::RaiseCompositionScaleChangedEvent()
{
    HRESULT hr = S_OK;

    CEventArgs* pArgs = NULL;

    CEventManager *pEventManager = GetContext()->GetEventManager();
    EventHandle hEvent(KnownEventIndex::SwapChainPanel_CompositionScaleChanged);
    pArgs = new CEventArgs();
    pEventManager->Raise(hEvent, TRUE, this, pArgs);

    ReleaseInterfaceNoNULL(pArgs);

    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_ HRESULT CSwapChainPanel::ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize)
{
    IFC_RETURN(CGrid::ArrangeOverride(finalSize, newFinalSize));

    if (m_pSwapChainElement != nullptr)
    {
        // If this panel resizes, we also have to resize the visual hosting the swap chain. Mark the SwapChainElement
        // as dirty so we'll render it again.
        CUIElement::NWSetContentDirty(m_pSwapChainElement, DirtyFlags::Render);
    }

    return S_OK;
}

// MUC visuals are thread-safe, so we don't need to lock around calls into a MUC visual. We just need a lock around
// whether this SwapChainPanel has created and cached its MUC visual. 
wrl::ComPtr<ixp::ISpriteVisual> CSwapChainPanel::GetOrEnsureSwapChainVisual()
{
    auto guard = m_swapChainVisualLock.lock();

    if (m_swapChainVisual == nullptr)
    {
        IFCFAILFAST(GetDCompTreeHost()->GetCompositor()->CreateSpriteVisual(&m_swapChainVisual));
    }
    return m_swapChainVisual;
}

_Check_return_ HRESULT CSwapChainPanel::CreateInputPointerSource(
    _In_ ixp::InputPointerSourceDeviceKinds deviceKinds,
    _Outptr_ ixp::IInputPointerSource** ppInputPointerSource)
{
    wil::unique_hmodule inputModule;
    inputModule.reset(LoadLibraryExWAbs(L"Microsoft.UI.Input.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));
    ASSERT(inputModule != nullptr, L"error: NULL handle to module Microsoft.UI.Input.dll!");

    typedef HRESULT(WINAPI* PFN_GetActivationFactory)(HSTRING activatableClassId, IActivationFactory** ppFactory);
    PFN_GetActivationFactory pfnGetActivationFactory = (PFN_GetActivationFactory)GetProcAddress(inputModule.get(), "DllGetActivationFactory");
    ASSERT(pfnGetActivationFactory != nullptr, L"error: nullptr to pfnGetActivationFactory!");

    wrl::ComPtr<ixp::IInputPointerSourceStatics2> inputPointerSourceStatics;
    {
        wrl::ComPtr<IActivationFactory> factory;
        pfnGetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputPointerSource).Get(), &factory);
        IFCFAILFAST(factory.As(&inputPointerSourceStatics));

        wrl::ComPtr<ixp::ISpriteVisual> spriteVisual = GetOrEnsureSwapChainVisual();
        wrl::ComPtr<ixp::IVisual> visual;
        IFCFAILFAST(spriteVisual.As(&visual));

        // Ensure any previously configured input pointer sources on this visual are cleared out.
        // This is important if we previously created an input pointer source on this visual for 
        // a different thread. If one exists, this will dispose it.
        IFCFAILFAST(inputPointerSourceStatics->RemoveForVisual(visual.Get()));

        // Create a new input pointer source for this visual on the current thread.
        IFCFAILFAST(inputPointerSourceStatics->GetForVisual(visual.Get(), ppInputPointerSource));

        // Set the device kinds.
        wrl::ComPtr<ixp::IInputPointerSource2> inputPointerSource2;
        IFCFAILFAST((*ppInputPointerSource)->QueryInterface(IID_PPV_ARGS(&inputPointerSource2)));
        
        boolean success;
        IFCFAILFAST(inputPointerSource2->TrySetDeviceKinds(deviceKinds, &success));
        if (!success)
        {
            IFCFAILFAST(E_UNEXPECTED);
        }
    }

    return S_OK;
}