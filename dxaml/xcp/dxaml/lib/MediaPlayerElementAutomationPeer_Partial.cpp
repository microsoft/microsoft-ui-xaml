// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaPlayerElementAutomationPeer.g.h"
#include "MediaPlayerElement.g.h"
#include "Panel.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT MediaPlayerElementAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IMediaPlayerElement* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IMediaPlayerElementAutomationPeer** ppInstance)
{
    ctl::ComPtr<xaml_automation_peers::IMediaPlayerElementAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;
    ctl::ComPtr<DirectUI::MediaPlayerElement> spMPE;
    ctl::ComPtr<DirectUI::MediaPlayerElementAutomationPeer> spMPE_AP;

    IFCPTR_RETURN(ppInstance);
    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);
    IFCPTR_RETURN(owner);
    
    IFC_RETURN(ctl::do_query_interface(spOwnerAsUIE, owner));
    IFC_RETURN(spOwnerAsUIE.As(&spMPE));

    IFC_RETURN(ActivateInstance(pOuter, spMPE->GetHandle(),&spInner));

    IFC_RETURN(spInner.As<xaml_automation_peers::IMediaPlayerElementAutomationPeer>(&spInstance));
    IFC_RETURN(spInstance.As(&spMPE_AP));    
    IFC_RETURN(spMPE_AP->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;

}

IFACEMETHODIMP MediaPlayerElementAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MediaPlayerElementAutomationPeer")).CopyTo(returnValue));
    return S_OK;
}

IFACEMETHODIMP MediaPlayerElementAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_MEDIAPLAYERELEMENT, returnValue));
    return S_OK;
}