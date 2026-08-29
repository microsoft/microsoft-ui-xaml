// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutoSuggestBoxAutomationPeer.g.h"
#include "AutoSuggestBoxAutomationPeer_Partial.h"
#include "AutoSuggestBox.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;


_Check_return_ HRESULT 
AutoSuggestBoxAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IAutoSuggestBox* pOwner,
    _Outptr_ xaml_automation_peers::IAutoSuggestBoxAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutoSuggestBoxAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* pOwnerAsUIE = NULL;

    IFCPTR(ppInstance);
    *ppInstance = NULL;

    IFCPTR(pOwner);

    IFC(ctl::do_query_interface(pOwnerAsUIE, pOwner));

    IFC(ActivationAPI::ActivateAutomationInstance(
            KnownTypeIndex::AutoSuggestBoxAutomationPeer, 
            static_cast<AutoSuggestBox*>(pOwner)->GetHandle(),
            nullptr /* pOuter */,
            &pInner));

    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<AutoSuggestBoxAutomationPeer*>(pInstance)->put_Owner(pOwnerAsUIE));

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pOwnerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the ButtonAutomationPeer class.
AutoSuggestBoxAutomationPeer::AutoSuggestBoxAutomationPeer()
{
}

// Deconstructor
AutoSuggestBoxAutomationPeer::~AutoSuggestBoxAutomationPeer()
{
}

IFACEMETHODIMP 
AutoSuggestBoxAutomationPeer::GetClassNameCore(
    _Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"AutoSuggestBox")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP 
AutoSuggestBoxAutomationPeer::GetAutomationControlTypeCore(
    _Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;
    RRETURN(S_OK);
}

IFACEMETHODIMP
AutoSuggestBoxAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC_RETURN(__super::GetPatternCore(patternInterface, returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT AutoSuggestBoxAutomationPeer::InvokeImpl()
{
    ctl::ComPtr<xaml::IUIElement> owner;
    IFC_RETURN(get_Owner(&owner));

    IFC_RETURN(owner.Cast<AutoSuggestBox>()->ProgrammaticSubmitQuery());

    return S_OK;
}
