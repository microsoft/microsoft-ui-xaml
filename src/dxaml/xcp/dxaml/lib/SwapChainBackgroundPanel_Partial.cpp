// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SwapChainBackgroundPanel.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Give the user access to the ISwapChainBackgroundPanelNative pure COM interface
//      using this QI override.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT SwapChainBackgroundPanel::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ISwapChainBackgroundPanelNative)))
    {
        *ppObject = static_cast<ISwapChainBackgroundPanelNative *>(this);
    }
    else if (iid == __uuidof(ISwapChainPanelOwner))
    {
        *ppObject = static_cast<ISwapChainPanelOwner *>(this);
    }
    else
    {
        return DirectUI::SwapChainBackgroundPanelGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Implementation of the ISwapChainBackgroundPanelNative interface
//      Allows a user to set an IDXGISwapChain on this SwapChainBackgroundPanel
//
//------------------------------------------------------------------------
IFACEMETHODIMP
SwapChainBackgroundPanel::SetSwapChain(_In_opt_ IDXGISwapChain* pSwapChain)
{
    HRESULT hr = S_OK;

    IUnknown *pSwapChainIUnknown = NULL;
    IDXGISwapChain1 *pSwapChain1 = NULL;
    DXGI_SWAP_CHAIN_DESC1 desc;

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

        //
        // Reject swap chains with premultipled/straight alpha mode. Blending XAML content
        // with swap chain background does not make sense in our model.
        //
        IFC(pSwapChain1->GetDesc1(&desc));
        if (desc.AlphaMode != DXGI_ALPHA_MODE_UNSPECIFIED &&
            desc.AlphaMode != DXGI_ALPHA_MODE_IGNORE)
        {
            IFC(ErrorHelper::OriginateError(AgError(AG_E_SWAPCHAINBACKGROUNDPANEL_UNSUPPORTED_ALPHAMODE)));
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
SwapChainBackgroundPanel::GetSwapChainPanel()
{
    return static_cast<CSwapChainPanel*>(GetHandle());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure that SwapChainBackgroundPanel's parent is public root of visual tree
//      or a Page element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SwapChainBackgroundPanel::OnTreeParentUpdated(
    _In_opt_ CDependencyObject *pNewParentCore,
    BOOLEAN isParentAlive)
{
    HRESULT hr = S_OK;

    if (pNewParentCore)
    {
        ctl::ComPtr<DependencyObject> spNewParent;
        IFC(DXamlCore::GetCurrent()->GetPeer(pNewParentCore, &spNewParent));

        CDependencyObject *pVisualRoot = pNewParentCore->GetPublicRootVisual();

        BOOLEAN isVisualRoot = (pVisualRoot == this->GetHandle());

        bool isParentPage = ctl::is<xaml_controls::IPage>(static_cast<IDependencyObject*>(spNewParent.Get()));

        BOOLEAN isPageParentNullOrVisualRoot = isParentPage &&
                                               (spNewParent->GetHandle()->GetParentInternal() == NULL || spNewParent->GetHandle()->GetParentInternal() == pVisualRoot);

        if (!(isVisualRoot || (isParentPage && isPageParentNullOrVisualRoot)))
        {
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_FAIL, ERROR_SWAPCHAINBACKGROUNDPANEL_MUSTBEROOTVISUAL_OR_CHILDOFPAGE));
        }

        if (isParentPage)
        {
            IFC(CoreImports::Page_ValidatePropertiesIfSwapChainBackgroundPanelChild(
                static_cast<CCoreServices*>(DXamlCore::GetCurrent()->GetHandle()),
                static_cast<CUIElement*>(spNewParent->GetHandle())
                ));
        }
    }

Cleanup:
    RRETURN(hr);
}
