// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RadioButtonAutomationPeer.g.h"
#include "RadioButton.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT RadioButtonAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IRadioButton* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IRadioButtonAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IRadioButtonAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<RadioButton*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<RadioButtonAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the RadioButtonAutomationPeer class.
RadioButtonAutomationPeer::RadioButtonAutomationPeer()
{
}

// Deconstructor
RadioButtonAutomationPeer::~RadioButtonAutomationPeer()
{
}

IFACEMETHODIMP RadioButtonAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_SelectionItem)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        *ppReturnValue = NULL;
    }

    RRETURN(hr);
}

IFACEMETHODIMP RadioButtonAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"RadioButton")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP RadioButtonAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_RadioButton;
    RRETURN(S_OK);
}

_Check_return_ HRESULT RadioButtonAutomationPeer::SelectImpl()
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
    IFC((static_cast<RadioButton*>(pOwner))->AutomationRadioButtonOnToggle());

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT RadioButtonAutomationPeer::AddToSelectionImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsChecked = FALSE;
    wf::IReference<bool>* pIsCheckedReference = NULL;
    
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RadioButton*>(pOwner))->get_IsChecked(&pIsCheckedReference));
    if (pIsCheckedReference)
    {
        IFC(pIsCheckedReference->get_Value(&bIsChecked));
    }

    if(!bIsChecked)
    {
        IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pIsCheckedReference);
    RRETURN(hr);
}

_Check_return_ HRESULT RadioButtonAutomationPeer::RemoveFromSelectionImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsChecked = FALSE;
    wf::IReference<bool>* pIsCheckedReference = NULL;
    
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RadioButton*>(pOwner))->get_IsChecked(&pIsCheckedReference));
    if (pIsCheckedReference)
    {
        IFC(pIsCheckedReference->get_Value(&bIsChecked));
    }

    if(bIsChecked)
    {
        IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
        IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pIsCheckedReference);
    RRETURN(hr);
}


_Check_return_ HRESULT RadioButtonAutomationPeer::get_IsSelectedImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    wf::IReference<bool>* pIsCheckedReference = NULL;
    BOOLEAN bIsChecked = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<RadioButton*>(pOwner))->get_IsChecked(&pIsCheckedReference));
    if (pIsCheckedReference)
    {
        IFC(pIsCheckedReference->get_Value(&bIsChecked));
    }

    *pValue = bIsChecked;

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pIsCheckedReference);

    RRETURN(hr);
}

_Check_return_ HRESULT RadioButtonAutomationPeer::get_SelectionContainerImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue)
{
    *ppReturnValue = NULL;
    RRETURN(S_OK);
}

// IsSelected Property Changed Event to UIAutomation Clients
_Check_return_ HRESULT RadioButtonAutomationPeer::RaiseIsSelectedPropertyChangedEvent(
        _In_ BOOLEAN bOldValue, 
        _In_ BOOLEAN bNewValue)
{
    HRESULT hr = S_OK;

    if (bOldValue != bNewValue)
    {
        CValue valueOld;
        CValue valueNew;

        IFC(CValueBoxer::BoxValue(&valueOld, bOldValue));
        IFC(CValueBoxer::BoxValue(&valueNew, bNewValue));

        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APIsSelectedProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}
