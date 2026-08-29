// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleButtonAutomationPeer.g.h"
#include "ToggleButton.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ToggleButtonAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::IToggleButton* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IToggleButtonAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IToggleButtonAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ToggleButton*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ToggleButtonAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ToggleButtonAutomationPeer class.
ToggleButtonAutomationPeer::ToggleButtonAutomationPeer()
{
}

// Deconstructor
ToggleButtonAutomationPeer::~ToggleButtonAutomationPeer()
{
}

IFACEMETHODIMP ToggleButtonAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Toggle)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ToggleButtonAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleButtonAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ToggleButton")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleButtonAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Button;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ToggleButtonAutomationPeer::ToggleImpl()
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
    IFC((static_cast<ToggleButton*>(pOwner))->AutomationToggleButtonOnToggle());

Cleanup:
    ReleaseInterface(pOwner);

    RRETURN(hr);
}

_Check_return_ HRESULT ToggleButtonAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    HRESULT hr = S_OK;
    wf::IReference<bool>* pIsCheckedReference = NULL;
    BOOLEAN bIsChecked = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ToggleButton*>(pOwner))->get_IsChecked(&pIsCheckedReference));

    if(pIsCheckedReference == NULL)
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_Indeterminate;
    }
    else
    {
        IFC(pIsCheckedReference->get_Value(&bIsChecked));
        if(bIsChecked)
        {
            *pReturnValue = xaml_automation::ToggleState::ToggleState_On;
        }
        else
        {
            *pReturnValue = xaml_automation::ToggleState::ToggleState_Off;
        }
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pIsCheckedReference);

    RRETURN(hr);
}

_Check_return_ HRESULT ToggleButtonAutomationPeer::RaiseToggleStatePropertyChangedEvent(
        _In_ IInspectable* pOldValue, 
        _In_ IInspectable* pNewValue)
{
    HRESULT hr = S_OK;
    xaml_automation::ToggleState oldValue;
    xaml_automation::ToggleState newValue;
    CValue valueOld;
    CValue valueNew;
    
    IFC(ToggleButtonAutomationPeer::ConvertToToggleState(pOldValue, &oldValue))
    IFC(ToggleButtonAutomationPeer::ConvertToToggleState(pNewValue, &newValue))
    
    if(oldValue != newValue)
    {
        IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
        IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));

        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}

// Convert the Boolean in Inspectable to the ToggleState Enum, if the Inspectable is NULL that corresponds to Indeterminate state.
_Check_return_ HRESULT ToggleButtonAutomationPeer::ConvertToToggleState(_In_ IInspectable* pValue, _Out_ xaml_automation::ToggleState* pToggleState)
{
    HRESULT hr = S_OK;
    BOOLEAN bValue = FALSE;
    
    *pToggleState = xaml_automation::ToggleState::ToggleState_Indeterminate;

    if (pValue)
    {
        IFC(ctl::do_get_value(bValue, pValue));
        
        if(bValue)
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_On;
        } 
        else
        {
            *pToggleState = xaml_automation::ToggleState::ToggleState_Off;
        }
    }

Cleanup:
    RRETURN(hr);    
}
