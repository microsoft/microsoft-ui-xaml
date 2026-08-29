// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThumbAutomationPeer.g.h"
#include "Thumb.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ThumbAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IThumb* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IThumbAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IThumbAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<Thumb*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ThumbAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the ButtonAutomationPeer class.
ThumbAutomationPeer::ThumbAutomationPeer()
{
}

// Deconstructor
ThumbAutomationPeer::~ThumbAutomationPeer()
{
}

IFACEMETHODIMP ThumbAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Thumb")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ThumbAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Thumb;
    RRETURN(S_OK);
}
