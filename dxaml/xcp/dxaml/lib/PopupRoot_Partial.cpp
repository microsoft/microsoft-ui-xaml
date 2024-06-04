// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PopupRoot.g.h"
#include "PopupRootAutomationPeer.g.h"

using namespace DirectUI;


// Create PopupRootAutomationPeer to represent the PopupRoot.
IFACEMETHODIMP PopupRoot::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;

    BOOLEAN bLightDismiss;

    IFC(IsTopmostPopupInLightDismissChain(&bLightDismiss));
    if (bLightDismiss)
    {
        ctl::ComPtr<PopupRootAutomationPeer> spAutomationPeer;
        IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::PopupRootAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
        IFC(spAutomationPeer->put_Owner(this));
        *ppAutomationPeer = spAutomationPeer.Detach();
    }
    else
    {
        hr = S_FALSE;
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT PopupRoot::CloseTopmostPopup()
{
    return CoreImports::PopupRoot_CloseTopmostPopup(GetHandle(), TRUE);
}

_Check_return_ HRESULT PopupRoot::IsTopmostPopupInLightDismissChain(_Out_ BOOLEAN* pbIsInLightDismissChain)
{
    *pbIsInLightDismissChain = !!static_cast<CPopupRoot*>(GetHandle())->IsTopmostPopupInLightDismissChain();
    return S_OK;
}
