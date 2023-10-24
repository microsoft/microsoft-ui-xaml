// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CalendarDatePickerAutomationPeer.g.h"
#include "CalendarDatePicker.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//CalendarDatePickerAutomationPeerFactory::CreateInstanceWithOwnerImpl
IMPLEMENT_CONTROL_AUTOMATIONPEERFACTORY_CREATEINSTANCE(CalendarDatePicker)

// Initializes a new instance of the CalendarDatePickerAutomationPeer class.
CalendarDatePickerAutomationPeer::CalendarDatePickerAutomationPeer()
{
}

// Deconstructor
CalendarDatePickerAutomationPeer::~CalendarDatePickerAutomationPeer()
{
}

IFACEMETHODIMP CalendarDatePickerAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke 
        || patternInterface == xaml_automation_peers::PatternInterface_Value)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(CalendarDatePickerAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP CalendarDatePickerAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    RRETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"CalendarDatePicker")).CopyTo(returnValue));
}

IFACEMETHODIMP CalendarDatePickerAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Button;
    RRETURN(S_OK);
}

IFACEMETHODIMP CalendarDatePickerAutomationPeer::GetLocalizedControlTypeCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_CALENDARDATEPICKER, returnValue));
    
    return S_OK;
}

_Check_return_ HRESULT  CalendarDatePickerAutomationPeer::InvokeImpl()
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    BOOLEAN bIsEnabled;

    IFC(IsEnabled(&bIsEnabled));
    if (!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<CalendarDatePicker*>(pOwner))->put_IsCalendarOpen(TRUE));

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT CalendarDatePickerAutomationPeer::get_IsReadOnlyImpl(_Out_ BOOLEAN* value)
{
    *value = TRUE;
    return S_OK;
}

_Check_return_ HRESULT CalendarDatePickerAutomationPeer::get_ValueImpl(_Out_ HSTRING* value)
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    IFC_RETURN(get_Owner(&spOwner));
    
    auto ownerItem = spOwner.AsOrNull<CalendarDatePicker>();
    IFCPTR_RETURN(ownerItem);

    IFC_RETURN(ownerItem->GetCurrentFormattedDate(value));
    
    return S_OK;
}

_Check_return_ HRESULT CalendarDatePickerAutomationPeer::SetValueImpl(HSTRING value)
{
    return E_NOTIMPL;
}