// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace Private
{

_Check_return_ HRESULT
AutomationHelper::RaiseEventIfListener(
    _In_ xaml::IUIElement* pUIElement,
    _In_ xaml::Automation::Peers::AutomationEvents eventId)
{
    HRESULT hr = S_OK;

    BOOLEAN bListener = FALSE;
    IFC(ListenerExistsHelper(eventId, &bListener));

    if (bListener)
    {
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
        IFC(CreatePeerForElement(pUIElement, &spAutomationPeer));

        if (spAutomationPeer)
        {
            IFC(spAutomationPeer->RaiseAutomationEvent(eventId));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::SetAutomationFocusIfListener(
    _In_ xaml::IUIElement* pUIElement)
{
    BOOLEAN bListener = FALSE;
    IFC_RETURN(ListenerExistsHelper(xaml_automation_peers::AutomationEvents_AutomationFocusChanged, &bListener));

    if (bListener)
    {
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerPrivate> spAutomationPeerPrivate;

        IFC_RETURN(CreatePeerForElement(pUIElement, &spAutomationPeer));
        if (spAutomationPeer)
        {
            IFC_RETURN(spAutomationPeer.As(&spAutomationPeerPrivate));

            if (spAutomationPeerPrivate)
            {
                IFC_RETURN(spAutomationPeerPrivate->SetAutomationFocus());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AutomationHelper::ConvertPropertyToEnum(
    _In_opt_ xaml::Automation::IAutomationProperty* pAutomationProperty,
    _Out_ AutomationPropertyEnum* pEnumProperty)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::IAutomationElementIdentifiersStatics> spAutomationElementIds;
    wrl::ComPtr<xaml::Automation::ISelectionItemPatternIdentifiersStatics> spSelectionItemPatternIds;
    wrl::ComPtr<xaml::Automation::IAutomationProperty> spCompareProperty;

    if (pAutomationProperty == nullptr)
    {
        *pEnumProperty = AutomationPropertyEnum::EmptyProperty;
        goto Cleanup;
    }

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationElementIdentifiers).Get(),
        &spAutomationElementIds));

    IFC(spAutomationElementIds->get_NameProperty(&spCompareProperty));
    if (spCompareProperty.Get() == pAutomationProperty)
    {
        *pEnumProperty = AutomationPropertyEnum::NameProperty;
        goto Cleanup;
    }

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_SelectionItemPatternIdentifiers).Get(),
        &spSelectionItemPatternIds));

    IFC(spSelectionItemPatternIds->get_IsSelectedProperty(&spCompareProperty));
    if (spCompareProperty.Get() == pAutomationProperty)
    {
        *pEnumProperty = AutomationPropertyEnum::IsSelectedProperty;
        goto Cleanup;
    }

    *pEnumProperty = AutomationPropertyEnum::NotSupported;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::RaisePropertyChanged(
    _In_ xaml::IUIElement* pUIElement,
    _In_ xaml::Automation::IAutomationProperty* pAutomationProperty,
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue,
    _In_ BOOLEAN checkIfListenerExists /* = FALSE by default */)
{
    HRESULT hr = S_OK;

    BOOLEAN raiseEvent = TRUE;

    if(checkIfListenerExists)
    {
        IFC(ListenerExistsHelper(xaml::Automation::Peers::AutomationEvents_PropertyChanged, &raiseEvent));
    }

    if (raiseEvent)
    {
        wrl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spAutomationPeer;
        IFC(CreatePeerForElement(pUIElement, &spAutomationPeer));

        if (spAutomationPeer)
        {
            IFC(spAutomationPeer->RaisePropertyChangedEvent(pAutomationProperty, pOldValue, pNewValue));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::RaisePropertyChangedIfListener(
    _In_ xaml::IUIElement* pUIElement,
    _In_ xaml::Automation::IAutomationProperty* pAutomationProperty,
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue)
{
    RRETURN(RaisePropertyChanged(
        pUIElement,
        pAutomationProperty,
        pOldValue,
        pNewValue,
        TRUE /* checkIfListenerExists */));
}

_Check_return_ HRESULT
AutomationHelper::ListenerExistsHelper(
    _In_ xaml::Automation::Peers::AutomationEvents eventId,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::Peers::IAutomationPeerStatics> spAPStatics;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_AutomationPeer).Get(),
        &spAPStatics));

    IFC(spAPStatics->ListenerExists(eventId, pReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::CreatePeerForElement(
    _In_ xaml::IUIElement* pUIElement,
    _Outptr_result_maybenull_ xaml::Automation::Peers::IAutomationPeer** ppReturnValue)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerStatics> spFEAPStatics;

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
        &spFEAPStatics));

    IFC(spFEAPStatics->CreatePeerForElement(pUIElement, ppReturnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::GetPlainText(
    _In_ IInspectable* pObject,
    _Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spObject(pObject);
    wrl::ComPtr<wf::IStringable> spString;
    wrl::ComPtr<wf::IPropertyValue> spPV;
    wrl::ComPtr<xaml::Data::ICustomPropertyProvider> spCPP;

    *returnValue = nullptr;

    if(SUCCEEDED(spObject.As(&spString)))
    {
        IFC(spString->ToString(returnValue));
    }

    if(!(*returnValue) && SUCCEEDED(spObject.As(&spPV)))
    {
        wf::PropertyType propertyType;
        IFC(spPV->get_Type(&propertyType));

        if (ValueConversionHelpers::CanConvertValueToString(propertyType))
        {
            IFC(ValueConversionHelpers::ConvertValueToString(spPV.Get(), propertyType, returnValue));
        }
    }

    if (!(*returnValue) && SUCCEEDED(spObject.As(&spCPP)))
    {
        IFC(spCPP->GetStringRepresentation(returnValue));
    }

    if(!(*returnValue))
    {
        IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"")).CopyTo(returnValue));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::SetElementAutomationName(
    _In_ xaml::IDependencyObject* pDO,
    _In_ HSTRING string)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::IAutomationPropertiesStatics> spAutomationStatics;
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
        &spAutomationStatics));

    IFC(spAutomationStatics->SetName(pDO, string));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::SetElementAutomationId(
    _In_ xaml::IDependencyObject* pDO,
    _In_ HSTRING string)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::Automation::IAutomationPropertiesStatics> spAutomationStatics;
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
        &spAutomationStatics));

    IFC(spAutomationStatics->SetAutomationId(pDO, string));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AutomationHelper::CopyAutomationProperties(
    _In_ xaml::IDependencyObject* pDOSource,
    _In_ xaml::IDependencyObject* pDOTarget)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::IAutomationPropertiesStatics> spAutomationStatics;
    HSTRING strAcceleratorKey = NULL;
    HSTRING strAccessKey = NULL;
    HSTRING strAutomationId = NULL;
    HSTRING strHelpText = NULL;
    BOOLEAN isRequiredForForm = NULL;
    HSTRING strItemStatus = NULL;
    HSTRING strItemType = NULL;
    HSTRING strName = NULL;
    wrl::ComPtr<xaml::IUIElement> spLabeledBy;
    xaml::Automation::Peers::AutomationLiveSetting liveSetting;
    xaml::Automation::Peers::AccessibilityView accessibilityView;

    // Get AutomationPropertiesStatic
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_AutomationProperties).Get(),
        &spAutomationStatics));

    // AcceleratorKey Property
    IFC(spAutomationStatics->GetAcceleratorKey(pDOSource, &strAcceleratorKey));
    if (strAcceleratorKey)
    {
        IFC(spAutomationStatics->SetAcceleratorKey(pDOTarget, strAcceleratorKey));
    }

    // AccessKey Property
    IFC(spAutomationStatics->GetAccessKey(pDOSource, &strAccessKey));
    if (strAccessKey)
    {
        IFC(spAutomationStatics->SetAccessKey(pDOTarget, strAccessKey));
    }

    // AutomationId Property
    IFC(spAutomationStatics->GetAutomationId(pDOSource, &strAutomationId));
    if (strAutomationId)
    {
        IFC(spAutomationStatics->SetAutomationId(pDOTarget, strAutomationId));
    }

    // HelpText Property
    IFC(spAutomationStatics->GetHelpText(pDOSource, &strHelpText));
    if (strHelpText)
    {
        IFC(spAutomationStatics->SetHelpText(pDOTarget, strHelpText));
    }

    // IsRequiredForForm Property
    IFC(spAutomationStatics->GetIsRequiredForForm(pDOSource, &isRequiredForForm));
    if (isRequiredForForm)
    {
        IFC(spAutomationStatics->SetIsRequiredForForm(pDOTarget, isRequiredForForm));
    }

    // ItemStatus Property
    IFC(spAutomationStatics->GetItemStatus(pDOSource, &strItemStatus));
    if (strItemStatus)
    {
        IFC(spAutomationStatics->SetItemStatus(pDOTarget, strItemStatus));
    }

    // ItemType Property
    IFC(spAutomationStatics->GetItemType(pDOSource, &strItemType));
    if (strItemType)
    {
        IFC(spAutomationStatics->SetItemType(pDOTarget, strItemType));
    }

    // Name Property
    IFC(spAutomationStatics->GetName(pDOSource, &strName));
    if (strName)
    {
        IFC(spAutomationStatics->SetName(pDOTarget, strName));
    }

    // LabeledBy Property
    IFC(spAutomationStatics->GetLabeledBy(pDOSource, &spLabeledBy));
    if (spLabeledBy)
    {
        IFC(spAutomationStatics->SetLabeledBy(pDOTarget, spLabeledBy.Get()));
    }

    // LiveSetting Property
    IFC(spAutomationStatics->GetLiveSetting(pDOSource, &liveSetting));
    if (liveSetting != xaml::Automation::Peers::AutomationLiveSetting::AutomationLiveSetting_Off)
    {
        IFC(spAutomationStatics->SetLiveSetting(pDOTarget, liveSetting));
    }

    // AccessibilityView Property
    IFC(spAutomationStatics->GetAccessibilityView(pDOSource, &accessibilityView));
    if (accessibilityView != xaml::Automation::Peers::AccessibilityView::AccessibilityView_Content)
    {
        IFC(spAutomationStatics->SetAccessibilityView(pDOTarget, accessibilityView));
    }

Cleanup:
    RRETURN(hr);
}

}
