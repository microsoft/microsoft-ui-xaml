// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class AutomationElementIdentifiers : 
        public xaml_automation::IAutomationElementIdentifiers,
        public xaml_automation::IAutomationElementIdentifiersStatics,
        public ctl::AbstractActivationFactory
    {

        BEGIN_INTERFACE_MAP(AutomationElementIdentifiers, ctl::AbstractActivationFactory)
            INTERFACE_ENTRY(AutomationElementIdentifiers,  xaml_automation::IAutomationElementIdentifiers)
            INTERFACE_ENTRY(AutomationElementIdentifiers,  xaml_automation::IAutomationElementIdentifiersStatics)
        END_INTERFACE_MAP(AutomationElementIdentifiers, ctl::AbstractActivationFactory)
            
        public:

            // Override Close on debug builds only so we can release our reference to any properties that were created.
#if XCP_MONITOR
            IFACEMETHOD(Close()) override;
#endif
            // Properties.
            IFACEMETHOD(get_AcceleratorKeyProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_AccessKeyProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_AutomationIdProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_BoundingRectangleProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ClassNameProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ClickablePointProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ControlTypeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_HasKeyboardFocusProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_HelpTextProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsContentElementProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsControlElementProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsEnabledProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsKeyboardFocusableProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsOffscreenProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsPasswordProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsRequiredForFormProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ItemStatusProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ItemTypeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LabeledByProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LocalizedControlTypeProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_NameProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_OrientationProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LiveSettingProperty)(_Out_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_ControlledPeersProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_PositionInSetProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_SizeOfSetProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LevelProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_AnnotationsProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LandmarkTypeProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_LocalizedLandmarkTypeProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsPeripheralProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsDataValidForFormProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FullDescriptionProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_DescribedByProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FlowsToProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_FlowsFromProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_HeadingLevelProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_IsDialogProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);
            IFACEMETHOD(get_CultureProperty)(_Outptr_ xaml_automation::IAutomationProperty** ppValue);

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            AutomationElementIdentifiers();
            ~AutomationElementIdentifiers() override;

        private:

            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAcceleratorKeyProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAccessKeyProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAutomationIdProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spBoundingRectangleProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spClassNameProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spClickablePointProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spControlTypeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spHasKeyboardFocusProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spHelpTextProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsContentElementProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsControlElementProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsEnabledProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsKeyboardFocusableProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsOffscreenProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsPasswordProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsRequiredForFormProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spItemStatusProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spItemTypeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLabeledByProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLocalizedControlTypeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spNameProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spOrientationProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLiveSettingProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spControlledPeersProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spPositionInSetProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spSizeOfSetProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLevelProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spAnnotationsProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLandmarkTypeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spLocalizedLandmarkTypeProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsPeripheralProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spIsDataValidForFormProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFullDescriptionProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spDescribedByProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFlowsToProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_spFlowsFromProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_HeadingLevelProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_isDialogProperty;
            ctl::ComPtr<xaml_automation::IAutomationProperty> m_CultureProperty;
    };
}
