// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "CommandBarTemplateSettings.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::CommandBarTemplateSettings::CommandBarTemplateSettings()
{
}

DirectUI::CommandBarTemplateSettings::~CommandBarTemplateSettings()
{
}

HRESULT DirectUI::CommandBarTemplateSettings::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::CommandBarTemplateSettings)))
    {
        *ppObject = static_cast<DirectUI::CommandBarTemplateSettings*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::Primitives::ICommandBarTemplateSettings)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::Primitives::ICommandBarTemplateSettings*>(this);
    }
    else
    {
        RRETURN(DirectUI::DependencyObject::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_ContentHeight(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_ContentHeight, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_ContentHeight(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_ContentHeight, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_EffectiveOverflowButtonVisibility(_Out_ ABI::Microsoft::UI::Xaml::Visibility* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_EffectiveOverflowButtonVisibility, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_EffectiveOverflowButtonVisibility(ABI::Microsoft::UI::Xaml::Visibility value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_EffectiveOverflowButtonVisibility, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_NegativeOverflowContentHeight(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_NegativeOverflowContentHeight, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_NegativeOverflowContentHeight(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_NegativeOverflowContentHeight, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentClipRect(_Out_ ABI::Windows::Foundation::Rect* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentClipRect, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentClipRect(ABI::Windows::Foundation::Rect value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentClipRect, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentCompactYTranslation(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentCompactYTranslation, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentCompactYTranslation(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentCompactYTranslation, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentHeight(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHeight, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentHeight(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHeight, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentHiddenYTranslation(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHiddenYTranslation, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentHiddenYTranslation(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHiddenYTranslation, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentHorizontalOffset(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHorizontalOffset, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentHorizontalOffset(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentHorizontalOffset, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentMaxHeight(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMaxHeight, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentMaxHeight(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMaxHeight, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentMaxWidth(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMaxWidth, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentMaxWidth(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMaxWidth, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentMinimalYTranslation(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMinimalYTranslation, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentMinimalYTranslation(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMinimalYTranslation, value));
}
IFACEMETHODIMP DirectUI::CommandBarTemplateSettings::get_OverflowContentMinWidth(_Out_ DOUBLE* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMinWidth, pValue));
}
_Check_return_ HRESULT DirectUI::CommandBarTemplateSettings::put_OverflowContentMinWidth(DOUBLE value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CommandBarTemplateSettings_OverflowContentMinWidth, value));
}

// Events.

// Methods.

// Factory methods.

// Dependency properties.













// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_CommandBarTemplateSettings()
    {
        RRETURN(ctl::ActivationFactoryCreator<CommandBarTemplateSettingsFactory>::CreateActivationFactory());
    }
}
