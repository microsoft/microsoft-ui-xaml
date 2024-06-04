// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SwapChainPanel.g.h"
#include <RootScale.h>
#include "Dispatcher.h"
#include "Callback.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize the SwapChainPanel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SwapChainPanel::Initialize()
{
    HRESULT hr = S_OK;

    IFC(__super::Initialize());

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Give the user access to the ISwapChainPanelNative pure COM interface
//      using this QI override.
//
//------------------------------------------------------------------------
HRESULT SwapChainPanel::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ISwapChainPanelNative2)))
    {
        *ppObject = static_cast<ISwapChainPanelNative2 *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ISwapChainPanelNative)))
    {
        *ppObject = static_cast<ISwapChainPanelNative *>(this);
    }
    else if (iid == __uuidof(ISwapChainPanelOwner))
    {
        *ppObject = static_cast<ISwapChainPanelOwner *>(this);
    }
    else
    {
        return DirectUI::SwapChainPanelGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      SetSwapChainHandle interface is exposed on ISwapChainPanelNative2 interface
//
//------------------------------------------------------------------------
IFACEMETHODIMP
SwapChainPanel::SetSwapChainHandle(_In_opt_ HANDLE swapChainHandle)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());
    IFC(static_cast<CSwapChainPanel*>(GetHandle())->SetSwapChainHandle(swapChainHandle));

Cleanup:

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of the ISwapChainPanelNative interface
//      Allows a user to set an IDXGISwapChain on this SwapChainPanel
//
//------------------------------------------------------------------------
IFACEMETHODIMP
SwapChainPanel::SetSwapChain(_In_opt_ IDXGISwapChain* pSwapChain)
{
    HRESULT hr = S_OK;

    IUnknown *pSwapChainIUnknown = NULL;
    IDXGISwapChain1 *pSwapChain1 = NULL;

    IFC(CheckThread());

    if (pSwapChain)
    {
        IFC(pSwapChain->QueryInterface(__uuidof(IUnknown), reinterpret_cast<void**>(&pSwapChainIUnknown)));

        //
        // Check for IDXGISwapChain1. Also, the swap chain must be created using CreateSwapChainForComposition().
        //
        HRESULT hr1 = pSwapChain->QueryInterface(__uuidof(IDXGISwapChain1), reinterpret_cast<void**>(&pSwapChain1));
        if (!SUCCEEDED(hr1))
        {
            IFC(ErrorHelper::OriginateError(AgError(AG_E_SWAPCHAINBACKGROUNDPANEL_UNSUPPORTED_SWAPCHAINTYPE)));
        }
    }

    IFC(CoreImports::SwapChainPanel_SetSwapChain(static_cast<CSwapChainPanel*>(GetHandle()), pSwapChainIUnknown));

Cleanup:
    ReleaseInterface(pSwapChainIUnknown);
    ReleaseInterface(pSwapChain1);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the CSwapChainPanel core object
//
//------------------------------------------------------------------------
CSwapChainPanel*
SwapChainPanel::GetSwapChainPanel()
{
    return static_cast<CSwapChainPanel*>(GetHandle());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to get peer of a certain type
//
//------------------------------------------------------------------------
template <typename T>
_Check_return_ HRESULT
TryGetPeer(_In_ CDependencyObject* pDO, _Out_ ctl::ComPtr<T>& spElement)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spPeer;
    IFC(DXamlCore::GetCurrent()->TryGetPeer(pDO, &spPeer));

    spElement = spPeer.Cast<T>();

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Entry point for the app to create and associate a CoreInput object
//      with their SCP, or disassociate the current CoreInput object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SwapChainPanel::CreateCoreIndependentInputSourceImpl(
    _In_ ixp::InputPointerSourceDeviceKinds deviceKinds,
    _Outptr_ ixp::IInputPointerSource** ppPointerInputSource)
{
    auto guard = m_Lock.lock();

    // This method must only be called from a non-UI thread
    if (SUCCEEDED(CheckThread()))
    {
        IFC_RETURN(RPC_E_WRONG_THREAD);
    }

    if (deviceKinds != ixp::InputPointerSourceDeviceKinds_None)
    {
        CSwapChainPanel* swapChainPanel = static_cast<CSwapChainPanel*>(GetHandle());
        IFC_RETURN(swapChainPanel->CreateInputPointerSource(deviceKinds, ppPointerInputSource));

        // Workaround for an IXP limitation.
        //
        // SwapChainPanel::CreateCoreIndependentInputSource is called off of the UI thread and allows an app to create
        // an independent input source to deliver pointer input off-thread. In order for the input source to deliver
        // anything, we must insert a visual that has content into the tree. Previously the app was able to set a fully
        // transparent swap chain as the content to handle scenarios where the app wants input but doesn't want anything
        // to show up on screen, but lifted IXP does not support transparency in swap chains. As a workaround, we create
        // a fully transparent solid color SpriteVisual and hook the independent input source up to that instead.
        //
        // The trickiness comes from the fact that CreateCoreIndependentInputSource is called off-thread. We want to set
        // up the transparent SpriteVisual when CreateCoreIndependentInputSource is called, except we're not on the UI
        // thread so we can't touch the tree. So instead, we queue a work item to revisit this SwapChainPanel on the UI
        // thread. If it doesn't have a SwapChainElement, we'll need to create one and hook it up to the tree via a fully
        // transparent SpriteVisual so that it can catch input for the independent pointer input observer that we just
        // created. If it already has a SwapChainElement with a swap chain hooked up, then we'll no-op.
        ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
        IFC_RETURN(GetXamlDispatcher(&spDispatcher));
        ctl::WeakRefPtr wpThis;
        IFC_RETURN(ctl::AsWeak(this, &wpThis));
        IFC_RETURN(spDispatcher->RunAsync(
            DirectUI::MakeCallback(
                this,
                &SwapChainPanel::SetUseTransparentVisualIfNeeded, 
                wpThis)));
    }
    else
    {
        *ppPointerInputSource = nullptr;
    }

    return S_OK;
}

_Check_return_ HRESULT SwapChainPanel::SetUseTransparentVisualIfNeeded(ctl::WeakRefPtr wpThis)
{
    auto localThis = wpThis.AsOrNull<SwapChainPanel>();
    if (localThis)
    {
        CSwapChainPanel* swapChainPanel = static_cast<CSwapChainPanel*>(GetHandle());
        swapChainPanel->SetUseTransparentVisualIfNeeded();
    }

    return S_OK;
}
