// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarLightDismiss.g.h"
#include "AppBarLightDismissAutomationPeer.g.h"
#include "ApplicationBarService.g.h"
#include "XamlRoot.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Create AppBarLightDismissAutomationPeer to represent the AppBarLightDismiss.
IFACEMETHODIMP AppBarLightDismiss::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<AppBarLightDismissAutomationPeer> spAutomationPeer;
    IFC(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::AppBarLightDismissAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();

Cleanup:
    RRETURN(hr);
}

HRESULT AppBarLightDismiss::AutomationClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IApplicationBarService> applicationBarService;
    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
    {
        IFC(xamlRoot->GetApplicationBarService(applicationBarService));
    }
    IFC(applicationBarService->CloseAllNonStickyAppBars());

Cleanup:
    RRETURN(hr);
}
