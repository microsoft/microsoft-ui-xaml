// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <WindowsGraphicsDeviceManager.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
CSwapChainElement::CSwapChainElement(_In_ CCoreServices *pCore)
    : CUIElement(pCore)
    , m_compositionSet(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CSwapChainElement::~CSwapChainElement()
{
}

bool CSwapChainElement::HasContent() const
{
    return m_pISwapChain != nullptr
        || m_pISwapChainSurface != nullptr
        || m_useTransparentVisual;
}

IUnknown* CSwapChainElement::GetSwapChain() const
{
    return m_pISwapChain.Get();
}

WUComp::ICompositionSurface* CSwapChainElement::GetSwapChainSurface() const
{
    return m_pISwapChainSurface.Get();
}

bool CSwapChainElement::GetUseTransparentVisual() const
{
    return m_useTransparentVisual;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces the underlying swap chain or swapchain handle, null to clear
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainElement::SetSwapChain(_In_opt_ IUnknown * pISwapChain)
{
    HRESULT hr = S_OK;

    BOOL dirty = TRUE;
    BOOL compositionChanged;
    CDependencyObject *pParentNoRef = GetParentInternal(false /*publicOnly*/);

    // If our new swapchain is null, then this might be the application trying to cleanup DX12 resources so they can reopen
    // a lost device.  To facilitate this, we will immediately release the content (releasing the underlying swapchain)
    // so that the app doesn't have to wait for the next tick.  Note, we can't use the device lost path in this case because
    // XAML device lost does not clear swapchains since they may have been created against a device that is still valid (e.g.
    // software device).
    if (pISwapChain == nullptr && GetCompositionPeer() != nullptr)
    {
        HWCompLeafNode* leafNode = GetCompositionPeer()->GetContentNode();
        if (leafNode != nullptr)
        {
            xref_ptr<WUComp::IVisual> visual = leafNode->GetWUCVisual();

            if (visual)
            {
                xref_ptr<WUComp::ISpriteVisual> spriteVisual;

                IFC(visual->QueryInterface(IID_PPV_ARGS(spriteVisual.ReleaseAndGetAddressOf())));
                IFC(spriteVisual->put_Brush(nullptr));
            }
        }
    }

    // First check if need to set or clear composition
    m_useTransparentVisual = false;
    IFC(UpdateRequiresComposition((pISwapChain != nullptr), compositionChanged));

    // If composition stays same, determine if content is still clean
    if (!compositionChanged && (pISwapChain == m_pISwapChain.Get()))
    {
        // SCBP is special.  The app may change swapchain rotation or resize the buffers,
        // then call SetSwapChain again with the same swapchain.  In this case we need
        // to update the DComp stretch-to-fit scale transform by dirtying/redrawing/updating it.
        // SCP does not stretch-to-fit so we only have to do this for SCBP.
        dirty = pParentNoRef->OfTypeByIndex<KnownTypeIndex::SwapChainBackgroundPanel>();
    }
    else // only need to update data member when dirty
    {
        m_pISwapChain = pISwapChain;
        m_pISwapChainSurface = nullptr;
    }

    if (dirty)
    {
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        CUIElement::NWSetContentDirty(pParentNoRef, DirtyFlags::Render | DirtyFlags::Bounds);
    }

Cleanup:
    ASSERT(m_pISwapChainSurface == nullptr);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replaces the underlying swap chain handle or swap chain, null to clear
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSwapChainElement::SetSwapChainHandle(_In_opt_ HANDLE swapChainHandle)
{
    BOOL compositionChanged = FALSE;
    BOOL handleNotNull = (swapChainHandle != nullptr);

    // First check if need to set or clear composition
    m_useTransparentVisual = false;
    IFC_RETURN(UpdateRequiresComposition(handleNotNull, compositionChanged));

    // if composition changed or handle is not null, set content dirty
    if (compositionChanged || handleNotNull)
    {
        m_pISwapChain = nullptr; // in case previously SwapChain set
        m_pISwapChainSurface = nullptr; // in case we fail

        if (handleNotNull)
        {
            // It is possible that an application could set this prior to a renderwalk, so we
            // need to ensure that our resources have finished being created.
            IFC_RETURN(GetContext()->GetBrowserHost()->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());

            IFC_RETURN(GetDCompTreeHost()->CreateCompositionSurfaceForHandle(swapChainHandle, m_pISwapChainSurface.ReleaseAndGetAddressOf()));
        }

        CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
        CUIElement::NWSetContentDirty(GetParentInternal(false), DirtyFlags::Render | DirtyFlags::Bounds);
    }

    ASSERT(m_pISwapChain == nullptr);
    return S_OK;
}

void CSwapChainElement::SetUseTransparentVisualIfNeeded()
{
    // If we already have swap chain content, don't mark this SwapChainElement as needing a transparent visual.
    if (!HasContent())
    {
        m_useTransparentVisual = true;

        BOOL compositionChanged = FALSE;
        IFCFAILFAST(UpdateRequiresComposition(true, compositionChanged));

        if (compositionChanged)
        {
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
            CUIElement::NWSetContentDirty(GetParentInternal(false), DirtyFlags::Render | DirtyFlags::Bounds);
        }
    }
}

//----------------------------------------------------------------------------------------
//
//  Synopsis:
//      Set or unset SwapChain composition and update m_compositionSet flag and interfaces
//----------------------------------------------------------------------------------------

_Check_return_ HRESULT
CSwapChainElement::UpdateRequiresComposition(_In_ bool requiresComposition, _Out_ BOOL &compositionChanged)
{
    compositionChanged = FALSE;
    if (requiresComposition != m_compositionSet)
    {
        if (requiresComposition)
        {
            IFC_RETURN(SetRequiresComposition(
                CompositionRequirement::SwapChainContent,
                IndependentAnimationType::None
                ));
            m_compositionSet = TRUE;
        }
        else
        {
            UnsetRequiresComposition(
                CompositionRequirement::SwapChainContent,
                IndependentAnimationType::None
                );
            m_compositionSet = FALSE;
        }
        compositionChanged = TRUE;
    }

    return S_OK;
}


