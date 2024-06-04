// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FullWindowMediaRoot.g.h"
#include "FullWindowMediaRootAutomationPeer.g.h"

using namespace DirectUI;

IFACEMETHODIMP FullWindowMediaRoot::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    ctl::ComPtr<FullWindowMediaRootAutomationPeer> spAutomationPeer;
    IFC_RETURN(ActivationAPI::ActivateAutomationInstance(KnownTypeIndex::FullWindowMediaRootAutomationPeer, GetHandle(), spAutomationPeer.GetAddressOf()));
    IFC_RETURN(spAutomationPeer->put_Owner(this));
    *ppAutomationPeer = spAutomationPeer.Detach();
    return S_OK;
}

