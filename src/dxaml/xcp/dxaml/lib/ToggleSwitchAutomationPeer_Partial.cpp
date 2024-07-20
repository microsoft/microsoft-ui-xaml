// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ToggleSwitchAutomationPeer.g.h"
#include "ToggleSwitch.g.h"
#include "ToggleButtonAutomationPeer.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ToggleSwitchAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IToggleSwitch* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IToggleSwitchAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IToggleSwitchAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ToggleSwitch*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ToggleSwitchAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ToggleSwitchAutomationPeer class.
ToggleSwitchAutomationPeer::ToggleSwitchAutomationPeer()
{
}

// Deconstructor
ToggleSwitchAutomationPeer::~ToggleSwitchAutomationPeer()
{
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_Toggle)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ToggleSwitchAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ToggleSwitch")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    // We return Button as ControlType as that's closest and follows same as Toggle Button, there is no ControlType exclusively for ToggleSwitch
    *pReturnValue = xaml_automation_peers::AutomationControlType_Button;
    RRETURN(S_OK);
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(returnValue);
    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TOGGLESWITCH, returnValue));
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    // If the control has an explicitly set UIA Name, use that
    // Otherwise we construct a Name from the Header text and the On/OffContent
    wrl_wrappers::HString baseName;
    IFC_RETURN(ToggleSwitchAutomationPeerGenerated::GetNameCore(baseName.GetAddressOf()));

    if (!baseName.IsEmpty())
    {
        IFC_RETURN(baseName.CopyTo(returnValue));
    }
    else
    {
        ctl::ComPtr<xaml::IUIElement> spOwner;
        IFC_RETURN(get_Owner(&spOwner));

        wrl_wrappers::HString headerText;
        ctl::ComPtr<IInspectable> spHeader;
        IFC_RETURN(spOwner.Cast<ToggleSwitch>()->get_Header(&spHeader));
        if (spHeader)
        {
            IFC_RETURN(FrameworkElement::GetStringFromObject(spHeader.Get(), headerText.GetAddressOf()));
        }

        wrl_wrappers::HString onOffContentText; // A string representation of the OnContent (is IsOn is true) or the OffContent (if IsOn is false).
        ctl::ComPtr<IInspectable> spOnOffContent;

        BOOLEAN isOn = FALSE;
        IFC_RETURN(spOwner.Cast<ToggleSwitch>()->get_IsOn(&isOn));
        if (isOn)
        {
            // We only want to include the OnContent if custom content has been provided.
            // The default value of OnContent is the string "On", but including this in the UIA Name adds no value, since this information is
            // already included in the ToggleState. Narrator reads out both the ToggleState and the Name (We don't want it to read "On On ToggleSwitch").
            bool hasCustomOnContent = !(spOwner.Cast<ToggleSwitch>()->GetHandle()->IsPropertyDefaultByIndex(KnownPropertyIndex::ToggleSwitch_OnContent));
            if (hasCustomOnContent)
            {
                IFC_RETURN(spOwner.Cast<ToggleSwitch>()->get_OnContent(&spOnOffContent));
            }
        }
        else
        {
            // As above, we only include custom OffContent.
            bool hasCustomOffContent = !(spOwner.Cast<ToggleSwitch>()->GetHandle()->IsPropertyDefaultByIndex(KnownPropertyIndex::ToggleSwitch_OffContent));
            if (hasCustomOffContent)
            {
                IFC_RETURN(spOwner.Cast<ToggleSwitch>()->get_OffContent(&spOnOffContent));
            }
        }

        if (spOnOffContent)
        {
            IFC_RETURN(FrameworkElement::GetStringFromObject(spOnOffContent.Get(), onOffContentText.GetAddressOf()));
        }

        if (!headerText.IsEmpty() && !onOffContentText.IsEmpty())
        {
            // Return the header text followed by the on/off content separated by a space:
            wrl_wrappers::HString space;
            IFC_RETURN(space.Set(L" "));

            wrl_wrappers::HString fullString;
            IFC_RETURN(headerText.Concat(space, fullString));
            IFC_RETURN(fullString.Concat(onOffContentText, fullString));

            IFC_RETURN(fullString.CopyTo(returnValue));
        }
        else if (!headerText.IsEmpty())
        {
            IFC_RETURN(headerText.CopyTo(returnValue));
        }
        else
        {
            // onOffContentText might be empty, but that's ok.
            IFC_RETURN(onOffContentText.CopyTo(returnValue));
        }
    }

    return S_OK;
}

IFACEMETHODIMP ToggleSwitchAutomationPeer::GetClickablePointCore(_Out_ wf::Point* returnValue)
{
    ctl::ComPtr<xaml::IUIElement> owner;
    IFC_RETURN(get_Owner(&owner));
    IFC_RETURN(owner.Cast<ToggleSwitch>()->AutomationGetClickablePoint(returnValue));
    return S_OK;
}

_Check_return_ HRESULT ToggleSwitchAutomationPeer::ToggleImpl()
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
    IFC((static_cast<ToggleSwitch*>(pOwner))->AutomationToggleSwitchOnToggle());

Cleanup:
    ReleaseInterface(pOwner);

    RRETURN(hr);
}

_Check_return_ HRESULT ToggleSwitchAutomationPeer::get_ToggleStateImpl(_Out_ xaml_automation::ToggleState* pReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isOn = FALSE;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<ToggleSwitch*>(pOwner))->get_IsOn(&isOn));

    if(isOn)
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_On;
    }
    else
    {
        *pReturnValue = xaml_automation::ToggleState::ToggleState_Off;
    }

Cleanup:
    ReleaseInterface(pOwner);

    RRETURN(hr);
}

_Check_return_ HRESULT ToggleSwitchAutomationPeer::RaiseToggleStatePropertyChangedEvent(
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
    
    ASSERT(oldValue != xaml_automation::ToggleState::ToggleState_Indeterminate);
    ASSERT(newValue != xaml_automation::ToggleState::ToggleState_Indeterminate);
    
    if(oldValue != newValue)
    {
        IFC(CValueBoxer::BoxEnumValue(&valueOld, oldValue));
        IFC(CValueBoxer::BoxEnumValue(&valueNew, newValue));

        IFC(CoreImports::AutomationRaiseAutomationPropertyChanged(static_cast<CAutomationPeer*>(GetHandle()), UIAXcp::APAutomationProperties::APToggleStateProperty, valueOld, valueNew));
    }

Cleanup:
    RRETURN(hr);
}
