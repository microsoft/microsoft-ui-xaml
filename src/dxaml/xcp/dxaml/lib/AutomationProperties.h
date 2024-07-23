// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class AutomationProperties :
        public xaml_automation::IAutomationProperties,
        public xaml_automation::IAutomationPropertiesStatics,
        public xaml_automation::IAutomationPropertiesStatics2,
        public ctl::AbstractActivationFactory
    {
        BEGIN_INTERFACE_MAP(AutomationProperties, ctl::AbstractActivation)
            INTERFACE_ENTRY(AutomationProperties, xaml_automation::IAutomationProperties)
            INTERFACE_ENTRY(AutomationProperties, xaml_automation::IAutomationPropertiesStatics)
            INTERFACE_ENTRY(AutomationProperties, xaml_automation::IAutomationPropertiesStatics2)
        END_INTERFACE_MAP(AutomationProperties, ctl::AbstractActivationFactory)

    public:
        // Properties.

        // Dependency properties.

        // Attached properties.
        static _Check_return_ HRESULT GetAcceleratorKeyPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        _Check_return_ IFACEMETHOD(get_AcceleratorKeyProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAcceleratorKey)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetAcceleratorKeyStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetAcceleratorKey)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT SetAcceleratorKeyStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value);
        static _Check_return_ HRESULT GetAccessKeyPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_AccessKeyProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAccessKey)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetAccessKeyStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetAccessKey)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT SetAccessKeyStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value);
        static _Check_return_ HRESULT GetAutomationIdPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_AutomationIdProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAutomationId)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetAutomationIdStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetAutomationId)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT SetAutomationIdStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value);
        static _Check_return_ HRESULT GetHelpTextPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_HelpTextProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHelpText)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetHelpTextStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetHelpText)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT SetHelpTextStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value);
        static _Check_return_ HRESULT GetIsRequiredForFormPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_IsRequiredForFormProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsRequiredForForm)(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue) override;
        static _Check_return_ HRESULT GetIsRequiredForFormStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetIsRequiredForForm)(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value) override;
        static _Check_return_ HRESULT GetItemStatusPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_ItemStatusProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetItemStatus)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetItemStatusStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetItemStatus)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT GetItemTypePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_ItemTypeProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetItemType)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetItemTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetItemType)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT GetLabeledByPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LabeledByProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLabeledBy)(_In_ xaml::IDependencyObject* element, _Outptr_ xaml::IUIElement** pValue) override;
        static _Check_return_ HRESULT GetLabeledByStatic(_In_ xaml::IDependencyObject* element, _Outptr_ xaml::IUIElement** pValue);
        IFACEMETHOD(SetLabeledBy)(_In_ xaml::IDependencyObject* element, _In_ xaml::IUIElement* value) override;
        static _Check_return_ HRESULT GetNamePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_NameProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetName)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetNameStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetName)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT SetNameStatic(_In_ xaml::IDependencyObject* element, _In_ HSTRING value);
        static _Check_return_ HRESULT GetLiveSettingPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LiveSettingProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLiveSetting)(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLiveSetting* returnValue) override;
        static _Check_return_ HRESULT GetLiveSettingStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLiveSetting* returnValue);
        IFACEMETHOD(SetLiveSetting)(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationLiveSetting value) override;
        static _Check_return_ HRESULT GetAccessibilityViewPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_AccessibilityViewProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAccessibilityView)(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AccessibilityView* returnValue) override;
        static _Check_return_ HRESULT GetAccessibilityViewStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AccessibilityView* returnValue);
        IFACEMETHOD(SetAccessibilityView)(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AccessibilityView value) override;
        static _Check_return_ HRESULT GetControlledPeersPropertyStatic(_Outptr_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_ControlledPeersProperty)(_Outptr_ xaml::IDependencyProperty** ppValue) override;
        static _Check_return_ HRESULT GetControlledPeersStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::UIElement*>** returnValue);
        IFACEMETHOD(GetControlledPeers)(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::UIElement*>** returnValue) override;
        static _Check_return_ HRESULT GetPositionInSetPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_PositionInSetProperty)(_Out_ xaml::IDependencyProperty **value) override;
        IFACEMETHOD(GetPositionInSet)(_In_ xaml::IDependencyObject *element, _Out_ INT *value) override;
        static _Check_return_ HRESULT GetPositionInSetStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value);
        IFACEMETHOD(SetPositionInSet)(_In_ xaml::IDependencyObject *element, _In_ INT value) override;
        static _Check_return_ HRESULT GetSizeOfSetPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_SizeOfSetProperty)(_Out_ xaml::IDependencyProperty **value) override;
        IFACEMETHOD(GetSizeOfSet)(_In_ xaml::IDependencyObject *element, _Out_ INT *value) override;
        static _Check_return_ HRESULT GetSizeOfSetStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value);
        IFACEMETHOD(SetSizeOfSet)(_In_ xaml::IDependencyObject *element, _In_ INT value) override;
        static _Check_return_ HRESULT GetLevelPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LevelProperty)(_Out_ xaml::IDependencyProperty **value) override;
        IFACEMETHOD(GetLevel)(_In_ xaml::IDependencyObject *element, _Out_ INT *value) override;
        static _Check_return_ HRESULT GetLevelStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value);
        IFACEMETHOD(SetLevel)(_In_ xaml::IDependencyObject *element, _In_ INT value) override;
        static _Check_return_ HRESULT GetAnnotationsPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_AnnotationsProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAnnotations)(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml_automation::AutomationAnnotation*>** pValue) override;
        static _Check_return_ HRESULT GetAnnotationsStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml_automation::AutomationAnnotation*>** pValue);
        static _Check_return_ HRESULT GetLandmarkTypePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LandmarkTypeProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLandmarkType)(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLandmarkType *pValue) override;
        static _Check_return_ HRESULT GetLandmarkTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationLandmarkType *pValue);
        IFACEMETHOD(SetLandmarkType)(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationLandmarkType value) override;
        static _Check_return_ HRESULT GetLocalizedLandmarkTypePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LocalizedLandmarkTypeProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLocalizedLandmarkType)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetLocalizedLandmarkTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetLocalizedLandmarkType)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT GetIsPeripheralPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_IsPeripheralProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsPeripheral)(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue) override;
        static _Check_return_ HRESULT GetIsPeripheralStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetIsPeripheral)(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value) override;
        static _Check_return_ HRESULT GetIsDataValidForFormPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_IsDataValidForFormProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsDataValidForForm)(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue) override;
        static _Check_return_ HRESULT GetIsDataValidForFormStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetIsDataValidForForm)(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value) override;
        static _Check_return_ HRESULT GetLocalizedControlTypePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_LocalizedControlTypeProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetLocalizedControlType)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetLocalizedControlTypeStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetLocalizedControlType)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT GetFullDescriptionPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_FullDescriptionProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetFullDescription)(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue) override;
        static _Check_return_ HRESULT GetFullDescriptionStatic(_In_ xaml::IDependencyObject* element, _Out_ HSTRING* pValue);
        IFACEMETHOD(SetFullDescription)(_In_ xaml::IDependencyObject* element, _In_ HSTRING value) override;
        static _Check_return_ HRESULT GetDescribedByPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_DescribedByProperty)(_Outptr_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetDescribedBy)(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue) override;
        static _Check_return_ HRESULT GetDescribedByStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue);
        static _Check_return_ HRESULT GetFlowsToPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_FlowsToProperty)(_Outptr_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetFlowsTo)(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue) override;
        static _Check_return_ HRESULT GetFlowsToStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue);
        static _Check_return_ HRESULT GetFlowsFromPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_FlowsFromProperty)(_Outptr_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetFlowsFrom)(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue) override;
        static _Check_return_ HRESULT GetFlowsFromStatic(_In_ xaml::IDependencyObject* element, _Outptr_ wfc::IVector<xaml::DependencyObject*>** returnValue);
        static _Check_return_ HRESULT GetCulturePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_CultureProperty)(_Out_ xaml::IDependencyProperty **value) override;
        IFACEMETHOD(GetCulture)(_In_ xaml::IDependencyObject *element, _Out_ INT *value) override;
        static _Check_return_ HRESULT GetCultureStatic(_In_ xaml::IDependencyObject* element, _Out_ INT *value);
        IFACEMETHOD(SetCulture)(_In_ xaml::IDependencyObject *element, _In_ INT value) override;
        static _Check_return_ HRESULT GetHeadingLevelPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_HeadingLevelProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetHeadingLevel)(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationHeadingLevel *pValue) override;
        static _Check_return_ HRESULT GetHeadingLevelStatic(_In_ xaml::IDependencyObject* element, _Out_ xaml_automation_peers::AutomationHeadingLevel *pValue);
        IFACEMETHOD(SetHeadingLevel)(_In_ xaml::IDependencyObject* element, _In_ xaml_automation_peers::AutomationHeadingLevel value) override;
        static _Check_return_ HRESULT GetIsDialogPropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_IsDialogProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetIsDialog)(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue) override;
        static _Check_return_ HRESULT GetIsDialogStatic(_In_ xaml::IDependencyObject* element, _Out_ BOOLEAN* pValue);
        IFACEMETHOD(SetIsDialog)(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value) override;
        static _Check_return_ HRESULT SetIsDialogStatic(_In_ xaml::IDependencyObject* element, _In_ BOOLEAN value);
        static _Check_return_ HRESULT GetAutomationControlTypePropertyStatic(_Out_ const CDependencyProperty** ppValue);
        IFACEMETHOD(get_AutomationControlTypeProperty)(_Out_ xaml::IDependencyProperty** ppValue) override;
        IFACEMETHOD(GetAutomationControlType)(_In_ xaml::IUIElement* element, _Out_ xaml_automation_peers::AutomationControlType* pValue) override;
        static _Check_return_ HRESULT GetAutomationControlTypeStatic(_In_ xaml::IUIElement* element, _Out_ xaml_automation_peers::AutomationControlType* pValue);
        IFACEMETHOD(SetAutomationControlType)(_In_ xaml::IUIElement* element, _In_ xaml_automation_peers::AutomationControlType value) override;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
        AutomationProperties();
    };

}
