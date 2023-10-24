// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Used as a root for connected animations.  The element itself is not used
// we just do this to create a visual that the connected animations can
// be tied to in their own layer.

#include "precomp.h"
#include "ConnectedAnimationRoot.h"

// These constants need to be kept in sync with test code, see WinRTMockDComp.cpp
static const wchar_t* s_connectedAnimationHostVisualTag = L"_XAML_DEBUG_TAG_ConnectedAnimationHostVisual";
static const wchar_t* s_connectedAnimationSnapshotVisualTag = L"_XAML_DEBUG_TAG_ConnectedAnimationSnapshotVisual";

CConnectedAnimationRoot::CConnectedAnimationRoot(_In_ CCoreServices *pCore) :
    CFrameworkElement(pCore)
{
};

_Check_return_ HRESULT CConnectedAnimationRoot::RemoveConnectedAnimationVisual(_In_ ixp::IVisual* animationVisual)
{
    wrl::ComPtr<IUnknown> rootVisual;
    IFC_RETURN(GetHandOffVisual(&rootVisual));

    wrl::ComPtr<ixp::IContainerVisual> containerVisual;
    IFC_RETURN(rootVisual.As(&containerVisual));

    wrl::ComPtr<ixp::IVisualCollection> childCollection;
    IFC_RETURN(containerVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));

    // Remove() will return E_INVALIDARG if the visual was not in the collection.
    // That's not a fatal error, so we'll ignore it if so - that's just a no-op.
    HRESULT hr = childCollection->Remove(animationVisual);

    if (SUCCEEDED(hr))
    {
        TraceDCompRemoveFromTreeInfo(
            reinterpret_cast<XUINT64>(animationVisual)
        );
    }
    else if (hr != E_INVALIDARG)
    {
        IFC_RETURN(hr);
    }

    return S_OK;
}

_Check_return_ HRESULT CConnectedAnimationRoot::CreateConnectedAnimationVisual(_Outptr_ ixp::IVisual** animationVisual)
{
    wrl::ComPtr<IUnknown> rootVisual;
    IFC_RETURN(GetHandOffVisual(&rootVisual));

    wrl::ComPtr<ixp::IVisual> visual;
    {
        wrl::ComPtr<ixp::ISpriteVisual> tempVisual;
        IFC_RETURN(GetContext()->m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor()->CreateSpriteVisual(&tempVisual));
        IFC_RETURN(tempVisual.As(&visual));
        SetDebugTag(visual.Get(), s_connectedAnimationSnapshotVisualTag);
    }

    TraceDCompCreateVisualInfo1(
        NULL, // Comp Node
        0,  // VisualIndex, currently nothing consumes this
        reinterpret_cast<XUINT64>(visual.Get())
    );

    wrl::ComPtr<ixp::IContainerVisual> containerVisual;
    IFC_RETURN(rootVisual.As(&containerVisual));

    wrl::ComPtr<ixp::IVisualCollection> childCollection;
    IFC_RETURN(containerVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));
    IFC_RETURN(childCollection->InsertAtTop(visual.Get()));

    TraceDCompAppendChildInfo(
        reinterpret_cast<XUINT64>(rootVisual.Get()),
        reinterpret_cast<XUINT64>(visual.Get())
    );

    *animationVisual = visual.Detach();
    return S_OK;
}

void CConnectedAnimationRoot::SetDebugTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* debugTag)
{
    if (DCompTreeHost::VisualDebugTagsEnabled())
    {
        xref_ptr<WUComp::ICompositionObject> compositionObject;
        VERIFYHR(visual->QueryInterface(IID_PPV_ARGS(compositionObject.ReleaseAndGetAddressOf())));
        xref_ptr<WUComp::ICompositionPropertySet> compositionPropertySet;
        IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

        // Property sets don't support strings as property values, so store our tag as a scalar value.
        IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(debugTag).Get(), 0.0f));
    }
}

const std::vector<xref_ptr<CUIElement>>& CConnectedAnimationRoot::GetUnloadingElements()
{
    return GetContext()->GetConnectedAnimationServiceNoRef()->GetUnloadingElements();
}

