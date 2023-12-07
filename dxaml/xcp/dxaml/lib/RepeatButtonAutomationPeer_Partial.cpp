// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RepeatButtonAutomationPeer.g.h"
#include "RepeatButton.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT RepeatButtonAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IRepeatButton* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IRepeatButtonAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRepeatButtonAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<RepeatButton*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<RepeatButtonAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the RepeatButtonAutomationPeer class.
RepeatButtonAutomationPeer::RepeatButtonAutomationPeer()
{
}

// Deconstructor
RepeatButtonAutomationPeer::~RepeatButtonAutomationPeer()
{
}

IFACEMETHODIMP RepeatButtonAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(RepeatButtonAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RepeatButtonAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RepeatButton")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RepeatButtonAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Button;
    RRETURN(S_OK);
}

_Check_return_ HRESULT RepeatButtonAutomationPeer::InvokeImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ButtonBase*>(pOwner))->ProgrammaticClick());

Cleanup:
    ReleaseInterface(pOwner);

    RRETURN(hr);
}
