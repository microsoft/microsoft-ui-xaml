// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationElementIdentifiers.h"
#include "AutomationProperty.g.h"

DirectUI::AutomationElementIdentifiers::AutomationElementIdentifiers()
{
}

DirectUI::AutomationElementIdentifiers::~AutomationElementIdentifiers()
{
}

HRESULT DirectUI::AutomationElementIdentifiers::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_automation::IAutomationElementIdentifiersStatics)))
    {
        *ppObject = static_cast<xaml_automation::IAutomationElementIdentifiersStatics*>(this);
    }
    else 
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_AcceleratorKeyProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAcceleratorKeyProperty, AutomationPropertiesEnum::AcceleratorKeyProperty));
    m_spAcceleratorKeyProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_AccessKeyProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAccessKeyProperty, AutomationPropertiesEnum::AccessKeyProperty));
    m_spAccessKeyProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_AutomationIdProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAutomationIdProperty, AutomationPropertiesEnum::AutomationIdProperty));
    m_spAutomationIdProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_BoundingRectangleProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spBoundingRectangleProperty, AutomationPropertiesEnum::BoundingRectangleProperty));
    m_spBoundingRectangleProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_ClassNameProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spClassNameProperty, AutomationPropertiesEnum::ClassNameProperty));
    m_spClassNameProperty.CopyTo(ppValue);

    return S_OK;
}


HRESULT DirectUI::AutomationElementIdentifiers::get_ClickablePointProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spClickablePointProperty, AutomationPropertiesEnum::ClickablePointProperty));
    m_spClickablePointProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_ControlTypeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spControlTypeProperty, AutomationPropertiesEnum::ControlTypeProperty));
    m_spControlTypeProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_HasKeyboardFocusProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spHasKeyboardFocusProperty, AutomationPropertiesEnum::ControlTypeProperty));
    m_spHasKeyboardFocusProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_HelpTextProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spHelpTextProperty, AutomationPropertiesEnum::HelpTextProperty));
    m_spHelpTextProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsContentElementProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsContentElementProperty, AutomationPropertiesEnum::IsContentElementProperty));
    m_spIsContentElementProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsControlElementProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsControlElementProperty, AutomationPropertiesEnum::IsControlElementProperty));
    m_spIsControlElementProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsEnabledProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsEnabledProperty, AutomationPropertiesEnum::IsEnabledProperty));
    m_spIsEnabledProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsKeyboardFocusableProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsKeyboardFocusableProperty, AutomationPropertiesEnum::IsKeyboardFocusableProperty));
    m_spIsKeyboardFocusableProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsOffscreenProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsOffscreenProperty, AutomationPropertiesEnum::IsOffscreenProperty));
    m_spIsOffscreenProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsPasswordProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsPasswordProperty, AutomationPropertiesEnum::IsPasswordProperty));
    m_spIsPasswordProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsRequiredForFormProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsRequiredForFormProperty, AutomationPropertiesEnum::IsRequiredForFormProperty));
    m_spIsRequiredForFormProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_ItemStatusProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsRequiredForFormProperty, AutomationPropertiesEnum::ItemStatusProperty));
    m_spIsRequiredForFormProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_ItemTypeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spItemTypeProperty, AutomationPropertiesEnum::ItemTypeProperty));
    m_spItemTypeProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LabeledByProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLabeledByProperty, AutomationPropertiesEnum::LabeledByProperty));
    m_spLabeledByProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LocalizedControlTypeProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLocalizedControlTypeProperty, AutomationPropertiesEnum::LocalizedControlTypeProperty));
    m_spLocalizedControlTypeProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_NameProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spNameProperty, AutomationPropertiesEnum::NameProperty));
    m_spNameProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_OrientationProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spOrientationProperty, AutomationPropertiesEnum::OrientationProperty));
    m_spOrientationProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LiveSettingProperty(_Out_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLiveSettingProperty, AutomationPropertiesEnum::LiveSettingProperty));
    m_spLiveSettingProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_ControlledPeersProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spControlledPeersProperty, AutomationPropertiesEnum::ControlledPeersProperty));
    m_spControlledPeersProperty.CopyTo(ppValue);

    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_PositionInSetProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spPositionInSetProperty, AutomationPropertiesEnum::PositionInSetProperty));
    m_spPositionInSetProperty.CopyTo(ppValue);
    return S_OK;
}


HRESULT DirectUI::AutomationElementIdentifiers::get_SizeOfSetProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spSizeOfSetProperty, AutomationPropertiesEnum::SizeOfSetProperty));
    m_spSizeOfSetProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LevelProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLevelProperty, AutomationPropertiesEnum::LevelProperty));
    m_spLevelProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_AnnotationsProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spAnnotationsProperty, AutomationPropertiesEnum::AnnotationsProperty));
    m_spAnnotationsProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LandmarkTypeProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLandmarkTypeProperty, AutomationPropertiesEnum::LandmarkTypeProperty));
    m_spLandmarkTypeProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_LocalizedLandmarkTypeProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spLocalizedLandmarkTypeProperty, AutomationPropertiesEnum::LocalizedLandmarkTypeProperty));
    m_spLocalizedLandmarkTypeProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsPeripheralProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsPeripheralProperty, AutomationPropertiesEnum::IsPeripheralProperty));
    m_spIsPeripheralProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsDataValidForFormProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spIsDataValidForFormProperty, AutomationPropertiesEnum::IsDataValidForFormProperty));
    m_spIsDataValidForFormProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_FullDescriptionProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFullDescriptionProperty, AutomationPropertiesEnum::FullDescriptionProperty));
    m_spFullDescriptionProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_DescribedByProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spDescribedByProperty, AutomationPropertiesEnum::DescribedByProperty));
    m_spDescribedByProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_FlowsToProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFlowsToProperty, AutomationPropertiesEnum::FlowsToProperty));
    m_spFlowsToProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_FlowsFromProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_spFlowsFromProperty, AutomationPropertiesEnum::FlowsFromProperty));
    m_spFlowsFromProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_HeadingLevelProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_HeadingLevelProperty, AutomationPropertiesEnum::HeadingLevelProperty));
    m_HeadingLevelProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_IsDialogProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_isDialogProperty, AutomationPropertiesEnum::IsDialogProperty));
    m_isDialogProperty.CopyTo(ppValue);
    return S_OK;
}

HRESULT DirectUI::AutomationElementIdentifiers::get_CultureProperty(_Outptr_ xaml_automation::IAutomationProperty** ppValue)
{
    IFCPTR_RETURN(ppValue);
    IFC_RETURN(AutomationProperty::EnsureProperty(m_CultureProperty, AutomationPropertiesEnum::CultureProperty));
    m_CultureProperty.CopyTo(ppValue);
    return S_OK;
}

#if XCP_MONITOR
IFACEMETHODIMP DirectUI::AutomationElementIdentifiers::Close()
{
    m_spAcceleratorKeyProperty.Reset();
    m_spAccessKeyProperty.Reset();
    m_spAutomationIdProperty.Reset();
    m_spBoundingRectangleProperty.Reset();
    m_spClassNameProperty.Reset();
    m_spClickablePointProperty.Reset();
    m_spControlTypeProperty.Reset();
    m_spHasKeyboardFocusProperty.Reset();
    m_spHelpTextProperty.Reset();
    m_spIsContentElementProperty.Reset();
    m_spIsControlElementProperty.Reset();
    m_spIsEnabledProperty.Reset();
    m_spIsKeyboardFocusableProperty.Reset();
    m_spIsOffscreenProperty.Reset();
    m_spIsPasswordProperty.Reset();
    m_spIsRequiredForFormProperty.Reset();
    m_spItemStatusProperty.Reset();
    m_spItemTypeProperty.Reset();
    m_spLabeledByProperty.Reset();
    m_spLocalizedControlTypeProperty.Reset();
    m_spNameProperty.Reset();
    m_spOrientationProperty.Reset();
    m_spLiveSettingProperty.Reset();
    m_spControlledPeersProperty.Reset();
    m_spPositionInSetProperty.Reset();
    m_spSizeOfSetProperty.Reset();
    m_spLevelProperty.Reset();
    m_spAnnotationsProperty.Reset();
    m_spLandmarkTypeProperty.Reset();
    m_spLocalizedLandmarkTypeProperty.Reset();
    m_spIsPeripheralProperty.Reset();
    m_spIsDataValidForFormProperty.Reset();
    m_spFullDescriptionProperty.Reset();
    m_spDescribedByProperty.Reset();
    m_spFlowsToProperty.Reset();
    m_spFlowsFromProperty.Reset();
    m_HeadingLevelProperty.Reset();
    m_isDialogProperty.Reset();
    return S_OK;
}
#endif