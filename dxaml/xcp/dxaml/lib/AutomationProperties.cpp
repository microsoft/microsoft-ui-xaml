// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationProperties.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Initializes a new instance of the AutomationProperties class.
DirectUI::AutomationProperties::AutomationProperties()
{
}


_Check_return_ HRESULT DirectUI::AutomationProperties::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(xaml_automation::IAutomationPropertiesStatics)))
    {
        *ppObject = static_cast<xaml_automation::IAutomationPropertiesStatics*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(xaml_automation::IAutomationPropertiesStatics2)))
    {
        *ppObject = static_cast<xaml_automation::IAutomationPropertiesStatics2*>(this);
    }
    else
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAcceleratorKeyPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AcceleratorKey);
    return S_OK;
}

_Check_return_ HRESULT DirectUI::AutomationProperties::get_AcceleratorKeyProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AcceleratorKey, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAcceleratorKey(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetAcceleratorKeyStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAcceleratorKeyStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AcceleratorKey, pValue));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::SetAcceleratorKeyStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AcceleratorKey, value));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetAcceleratorKey(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AcceleratorKey, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAccessKeyPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AccessKey);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_AccessKeyProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AccessKey, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAccessKey(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetAccessKeyStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAccessKeyStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessKey, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetAccessKey(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return SetAccessKeyStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::SetAccessKeyStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessKey, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAutomationIdPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AutomationId);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_AutomationIdProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AutomationId, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAutomationId(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetAutomationIdStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAutomationIdStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AutomationId, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetAutomationId(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return AutomationProperties::SetAutomationIdStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::SetAutomationIdStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AutomationId, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetHelpTextPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_HelpText);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_HelpTextProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_HelpText, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetHelpText(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetHelpTextStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetHelpTextStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_HelpText, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetHelpText(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_HelpText, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::SetHelpTextStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_HelpText, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsRequiredForFormPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_IsRequiredForForm);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_IsRequiredForFormProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_IsRequiredForForm, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetIsRequiredForForm(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return AutomationProperties::GetIsRequiredForFormStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsRequiredForFormStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsRequiredForForm, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetIsRequiredForForm(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsRequiredForForm, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetItemStatusPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_ItemStatus);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_ItemStatusProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_ItemStatus, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetItemStatus(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetItemStatusStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetItemStatusStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ItemStatus, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetItemStatus(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ItemStatus, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetItemTypePropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_ItemType);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_ItemTypeProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_ItemType, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetItemType(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetItemTypeStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetItemTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ItemType, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetItemType(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ItemType, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLabeledByPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LabeledBy);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LabeledByProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_LabeledBy, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLabeledBy(_In_ xaml::IDependencyObject* element, _Outptr_ xaml::IUIElement** pValue)
{
    return AutomationProperties::GetLabeledByStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLabeledByStatic(_In_ xaml::IDependencyObject* element, _Outptr_ xaml::IUIElement** pValue)
{
    return (DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::DependencyObject*>(element), KnownPropertyIndex::AutomationProperties_LabeledBy, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLabeledBy(_In_ xaml::IDependencyObject* element, _In_ xaml::IUIElement* value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LabeledBy, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetNamePropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Name);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_NameProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_Name, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetName(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetNameStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetNameStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Name, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetName(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return AutomationProperties::SetNameStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::SetNameStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Name, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLiveSettingPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LiveSetting);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LiveSettingProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_LiveSetting, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLiveSetting(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    return AutomationProperties::GetLiveSettingStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLiveSettingStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLiveSetting* returnValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LiveSetting, returnValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLiveSetting(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationLiveSetting value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LiveSetting, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAccessibilityViewPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_AccessibilityViewProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AccessibilityView, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAccessibilityView(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AccessibilityView* returnValue)
{
    return AutomationProperties::GetAccessibilityViewStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAccessibilityViewStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AccessibilityView* returnValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, returnValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetAccessibilityView(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AccessibilityView value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetControlledPeersPropertyStatic(_Outptr_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_ControlledPeers);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_ControlledPeersProperty(_Outptr_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_ControlledPeers, ppValue));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetControlledPeersStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::UIElement*>** returnValue)
{
    DependencyObject* pDO;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spBox;

    IFCPTR_RETURN(element);
    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    pDO = static_cast<DependencyObject*>(element);
    IFC_RETURN(pDO->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ControlledPeers, spBox.GetAddressOf()));

    if (spBox)
    {
        *returnValue = spBox.Detach();
    }
    else
    {
        ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spElements;
        IFC_RETURN(ctl::make(&spElements));
        IFC_RETURN(pDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_ControlledPeers, ctl::iinspectable_cast(spElements.Get())));
        *returnValue = spElements.Detach();
    }

    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetControlledPeers(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::UIElement*>** returnValue)
{
    return GetControlledPeersStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetPositionInSetPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_PositionInSet);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_PositionInSetProperty(_Out_ xaml::IDependencyProperty **ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_PositionInSet, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetPositionInSet(_In_ xaml::IDependencyObject *element, _Out_ INT *value)
{
    return AutomationProperties::GetPositionInSetStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetPositionInSetStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_PositionInSet, value));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetPositionInSet(_In_ xaml::IDependencyObject *element, _In_ INT value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_PositionInSet, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetSizeOfSetPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_SizeOfSet);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_SizeOfSetProperty(_Out_ xaml::IDependencyProperty **ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_SizeOfSet, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetSizeOfSet(_In_ xaml::IDependencyObject *element, _Out_ INT *value)
{
    return AutomationProperties::GetSizeOfSetStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetSizeOfSetStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_SizeOfSet, value));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetSizeOfSet(_In_ xaml::IDependencyObject *element, _In_ INT value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_SizeOfSet, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLevelPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Level);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LevelProperty(_Out_ xaml::IDependencyProperty **ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_Level, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLevel(_In_ xaml::IDependencyObject *element, _Out_ INT *value)
{
    return AutomationProperties::GetLevelStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLevelStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Level, value));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLevel(_In_ xaml::IDependencyObject *element, _In_ INT value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Level, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAnnotationsPropertyStatic(_Outptr_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Annotations);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_AnnotationsProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_Annotations, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAnnotations(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml_automation::AutomationAnnotation*>** ppValue)
{
    return AutomationProperties::GetAnnotationsStatic(element, ppValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAnnotationsStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml_automation::AutomationAnnotation*>** ppValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Annotations, ppValue));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLandmarkTypePropertyStatic(_Outptr_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LandmarkType);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LandmarkTypeProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_LandmarkType, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLandmarkType(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLandmarkType *pValue)
{
    return AutomationProperties::GetLandmarkTypeStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLandmarkTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLandmarkType *pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LandmarkType, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLandmarkType(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationLandmarkType value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LandmarkType, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLocalizedLandmarkTypePropertyStatic(_Outptr_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LocalizedLandmarkType);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LocalizedLandmarkTypeProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_LocalizedLandmarkType, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLocalizedLandmarkType(_In_ xaml::IDependencyObject* element, _Out_ HSTRING *pValue)
{
    return AutomationProperties::GetLocalizedLandmarkTypeStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLocalizedLandmarkTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LocalizedLandmarkType, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLocalizedLandmarkType(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LocalizedLandmarkType, value));
}

// IsPeripheral
_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsPeripheralPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_IsPeripheral);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_IsPeripheralProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_IsPeripheral, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetIsPeripheral(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return AutomationProperties::GetIsPeripheralStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsPeripheralStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsPeripheral, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetIsPeripheral(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsPeripheral, value));
}

// IsDataValidForForm
_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsDataValidForFormPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_IsDataValidForForm);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_IsDataValidForFormProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_IsDataValidForForm, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetIsDataValidForForm(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return AutomationProperties::GetIsDataValidForFormStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsDataValidForFormStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsDataValidForForm, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetIsDataValidForForm(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsDataValidForForm, value));
}

// LocalizedControlType
_Check_return_ HRESULT DirectUI::AutomationProperties::GetLocalizedControlTypePropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_LocalizedControlType);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_LocalizedControlTypeProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_LocalizedControlType, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetLocalizedControlType(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetLocalizedControlTypeStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetLocalizedControlTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LocalizedControlType, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetLocalizedControlType(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_LocalizedControlType, value));
}

// FullDescription
_Check_return_ HRESULT DirectUI::AutomationProperties::GetFullDescriptionPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_FullDescription);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_FullDescriptionProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_FullDescription, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetFullDescription(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return AutomationProperties::GetFullDescriptionStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetFullDescriptionStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FullDescription, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetFullDescription(_In_ xaml::IDependencyObject* element, _In_ HSTRING value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FullDescription, value));
}

// GetDescribedBy
static _Check_return_ HRESULT GetDescribedByPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_DescribedBy);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_DescribedByProperty(_Outptr_ xaml::IDependencyProperty** returnValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_DescribedBy, returnValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetDescribedBy(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    return AutomationProperties::GetDescribedByStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetDescribedByStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    DependencyObject* pDO;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spBox;

    IFCPTR_RETURN(element);
    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    pDO = static_cast<DependencyObject*>(element);
    IFC_RETURN(pDO->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_DescribedBy, spBox.GetAddressOf()));

    if (spBox)
    {
        *returnValue = spBox.Detach();
    }
    else
    {
        ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spElements;
        IFC_RETURN(ctl::make(&spElements));
        IFC_RETURN(pDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_DescribedBy, ctl::iinspectable_cast(spElements.Get())));
        *returnValue = spElements.Detach();
    }

    return S_OK;
}

// GetFlowsTo
static _Check_return_ HRESULT GetFlowsToPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_FlowsTo);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_FlowsToProperty(_Outptr_ xaml::IDependencyProperty** returnValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_FlowsTo, returnValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetFlowsTo(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    return AutomationProperties::GetFlowsToStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetFlowsToStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    DependencyObject* pDO;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spBox;

    IFCPTR_RETURN(element);
    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    pDO = static_cast<DependencyObject*>(element);
    IFC_RETURN(pDO->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FlowsTo, spBox.GetAddressOf()));

    if (spBox)
    {
        *returnValue = spBox.Detach();
    }
    else
    {
        ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spElements;
        IFC_RETURN(ctl::make(&spElements));
        IFC_RETURN(pDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FlowsTo, ctl::iinspectable_cast(spElements.Get())));
        *returnValue = spElements.Detach();
    }

    return S_OK;
}

// GetFlowsFrom
static _Check_return_ HRESULT GetFlowsFromPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_FlowsFrom);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_FlowsFromProperty(_Outptr_ xaml::IDependencyProperty** returnValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_FlowsFrom, returnValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetFlowsFrom(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    return AutomationProperties::GetFlowsFromStatic(element, returnValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetFlowsFromStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue)
{
    DependencyObject* pDO;
    ctl::ComPtr<wfc::IVector<xaml::DependencyObject*>> spBox;

    IFCPTR_RETURN(element);
    IFCPTR_RETURN(returnValue);
    *returnValue = nullptr;

    pDO = static_cast<DependencyObject*>(element);
    IFC_RETURN(pDO->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FlowsFrom, spBox.GetAddressOf()));

    if (spBox)
    {
        *returnValue = spBox.Detach();
    }
    else
    {
        ctl::ComPtr<TrackerCollection<xaml::DependencyObject*>> spElements;
        IFC_RETURN(ctl::make(&spElements));
        IFC_RETURN(pDO->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_FlowsFrom, ctl::iinspectable_cast(spElements.Get())));
        *returnValue = spElements.Detach();
    }

    return S_OK;
}

// Culture
_Check_return_ HRESULT DirectUI::AutomationProperties::GetCulturePropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_Culture);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_CultureProperty(_Out_ xaml::IDependencyProperty **ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_Culture, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetCulture(_In_ xaml::IDependencyObject *element, _Out_ INT *value)
{
    return AutomationProperties::GetCultureStatic(element, value);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetCultureStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Culture, value));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetCulture(_In_ xaml::IDependencyObject *element, _In_ INT value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_Culture, value));
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetHeadingLevelPropertyStatic(_Outptr_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_HeadingLevel);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_HeadingLevelProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_HeadingLevel, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetHeadingLevel(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationHeadingLevel *pValue)
{
    return AutomationProperties::GetHeadingLevelStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetHeadingLevelStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationHeadingLevel *pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_HeadingLevel, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetHeadingLevel(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationHeadingLevel value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_HeadingLevel, value));
}

// IsDialog
_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsDialogPropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_IsDialog);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_IsDialogProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_IsDialog, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetIsDialog(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return AutomationProperties::GetIsDialogStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetIsDialogStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue)
{
    return (static_cast<DependencyObject*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsDialog, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetIsDialog(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value)
{
    return AutomationProperties::SetIsDialogStatic(element, value);
}

/*static*/ _Check_return_ HRESULT DirectUI::AutomationProperties::SetIsDialogStatic(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value)
{
    return (static_cast<DependencyObject*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_IsDialog, value));
}

// AutomationControlType
_Check_return_ HRESULT DirectUI::AutomationProperties::GetAutomationControlTypePropertyStatic(_Out_ const CDependencyProperty** ppValue)
{
    *ppValue = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AutomationControlType);
    return S_OK;
}

IFACEMETHODIMP DirectUI::AutomationProperties::get_AutomationControlTypeProperty(_Out_ xaml::IDependencyProperty** ppValue)
{
    return (MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AutomationProperties_AutomationControlType, ppValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::GetAutomationControlType(_In_ xaml::IUIElement* element, _Out_ xaml_automation_peers::AutomationControlType* pValue)
{
    return AutomationProperties::GetAutomationControlTypeStatic(element, pValue);
}

_Check_return_ HRESULT DirectUI::AutomationProperties::GetAutomationControlTypeStatic(_In_ xaml::IUIElement* element, _Out_ xaml_automation_peers::AutomationControlType* pValue)
{
    return (static_cast<UIElement*>(element)->GetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AutomationControlType, pValue));
}

IFACEMETHODIMP DirectUI::AutomationProperties::SetAutomationControlType(_In_ xaml::IUIElement* element, _In_ xaml_automation_peers::AutomationControlType value)
{
    return (static_cast<UIElement*>(element)->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AutomationControlType, value));
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_AutomationProperties()
    {
        return (ctl::ActivationFactoryCreator<AutomationProperties>::CreateActivationFactory());
    }
}

