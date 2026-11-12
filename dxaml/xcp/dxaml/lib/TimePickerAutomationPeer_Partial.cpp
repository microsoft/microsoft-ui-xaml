// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimePickerAutomationPeer.g.h"
#include "TimePicker.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT TimePickerAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ITimePicker* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ITimePickerAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::ITimePickerAutomationPeer> spInstance = NULL;
    ctl::ComPtr<IInspectable> spInner = NULL;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<TimePicker*>(pOwner)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::ITimePickerAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<TimePickerAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP TimePickerAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"TimePicker")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP TimePickerAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;
    RRETURN(S_OK);
}

// The TimePicker's own peer is a Group that wraps a single focusable/invokable child
// (the FlyoutButton). The FlyoutButton already carries the full automation name
// (parent name + selected value + localized control type), which is what focus-based
// screen readers announce and what voice-command experiences target. Exposing the same
// name on this containing group as well caused Narrator to announce it twice (GitHub
// issue #8296). Removing the group from the Control and Content views leaves it as a
// structural element only: its GetNameCore is still honored (so AutomationElement.Name
// keeps returning the label), but it is no longer announced as a separate node, so the
// name is read exactly once.
IFACEMETHODIMP TimePickerAutomationPeer::IsControlElementCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;
    RRETURN(S_OK);
}

IFACEMETHODIMP TimePickerAutomationPeer::IsContentElementCore(_Out_ BOOLEAN* returnValue)
{
    *returnValue = FALSE;
    RRETURN(S_OK);
}

IFACEMETHODIMP TimePickerAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    XUINT32 pLength = 0;

    IFC_RETURN(TimePickerAutomationPeerGenerated::GetNameCore(returnValue));
    
    pLength = WindowsGetStringLen(*returnValue);
    if(pLength == 0)
    {
        ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;
        ctl::ComPtr<IInspectable> spHeaderAsInspectable;

        DELETE_STRING(*returnValue);
        *returnValue = nullptr;

        IFC_RETURN(get_Owner(&spOwnerAsUIE));

        IFC_RETURN(spOwnerAsUIE.Cast<TimePicker>()->get_Header(&spHeaderAsInspectable));
        if(spHeaderAsInspectable)
        {
            IFC_RETURN(FrameworkElement::GetStringFromObject(spHeaderAsInspectable.Get(), returnValue));
            pLength = WindowsGetStringLen(*returnValue);
        }

        if(pLength == 0)
        {
            DELETE_STRING(*returnValue);
            *returnValue = nullptr;
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_AP_TIMEPICKER, returnValue));
        }
    }

    return S_OK;
}
