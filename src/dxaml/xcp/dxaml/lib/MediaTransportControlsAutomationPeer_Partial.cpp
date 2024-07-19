// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaTransportControlsAutomationPeer.g.h"
#include "MediaTransportControls.g.h"
using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT MediaTransportControlsAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IMediaTransportControls* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IMediaTransportControlsAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IMediaTransportControlsAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<IUIElement> ownerAsUIE;
    ctl::ComPtr<DirectUI::MediaTransportControls> ownerAsMTC;
    ctl::ComPtr<DirectUI::MediaTransportControlsAutomationPeer> mtcAP;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    IFC(ownerAsUIE.As(&ownerAsMTC));

    IFC(ActivateInstance(pOuter, ownerAsMTC->GetHandle(), &spInner));
    
    IFC(ctl::do_query_interface(spInstance, spInner.Get()));
    IFC(spInstance.As(&mtcAP));
    IFC(mtcAP->put_Owner(ownerAsUIE.Get()));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
        spInner = nullptr;
    }

    *ppInstance = spInstance.Detach();
    spInstance = nullptr;

Cleanup:
    return hr;
}

IFACEMETHODIMP MediaTransportControlsAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{   
    return wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MediaTransportControls")).CopyTo(returnValue);
}

IFACEMETHODIMP MediaTransportControlsAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    *pReturnValue = xaml_automation_peers::AutomationControlType_Custom;
    return hr;
}
