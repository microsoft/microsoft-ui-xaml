// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ButtonBaseAutomationPeer.g.h"
#include "ButtonBase.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ButtonBaseAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IButtonBase* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IButtonBaseAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IButtonBaseAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ButtonBase*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ButtonBaseAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ButtonBaseAutomationPeer class.
ButtonBaseAutomationPeer::ButtonBaseAutomationPeer()
{
}

// Deconstructor
ButtonBaseAutomationPeer::~ButtonBaseAutomationPeer()
{
}

IFACEMETHODIMP ButtonBaseAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    xaml::IUIElement* pOwner = NULL;
    IInspectable* pContent = NULL;

    IFC(ButtonBaseAutomationPeerGenerated::GetNameCore(returnValue));

    if (*returnValue == NULL)
    {
        IFC(get_Owner(&pOwner));
        IFCPTR(pOwner);
        IFC((static_cast<ButtonBase*>(pOwner))->get_Content(&pContent));
        IFC(IValueBoxer::UnboxValue(pContent, returnValue));
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pContent);

    RRETURN(hr);
}


