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

#include "AppBarButton.g.h"
#include "AppBarButtonTemplateSettings.g.h"
#include "IconElement.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::AppBarButtonGenerated::AppBarButtonGenerated()
{
}

DirectUI::AppBarButtonGenerated::~AppBarButtonGenerated()
{
}

HRESULT DirectUI::AppBarButtonGenerated::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::AppBarButton)))
    {
        *ppObject = static_cast<DirectUI::AppBarButton*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarButton)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarButton*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICommandBarElement)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICommandBarElement*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICommandBarLabeledElement)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICommandBarLabeledElement*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICommandBarOverflowElement)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICommandBarOverflowElement*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ISubMenuOwner)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ISubMenuOwner*>(this);
    }
    else
    {
        RRETURN(DirectUI::Button::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_DynamicOverflowOrder(_Out_ INT* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_DynamicOverflowOrder, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_DynamicOverflowOrder(INT value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_DynamicOverflowOrder, value));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_Icon(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_Icon, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_Icon(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IIconElement* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_Icon, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_IsCompact(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_IsCompact, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_IsCompact(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_IsCompact, value));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_IsInOverflow(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<AppBarButton*>(this)->get_IsInOverflowImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_IsSubMenuOpen(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<AppBarButton*>(this)->get_IsSubMenuOpenImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_IsSubMenuPositionedAbsolutely(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<AppBarButton*>(this)->get_IsSubMenuPositionedAbsolutelyImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_KeyboardAcceleratorTextOverride(_Out_ HSTRING* pValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(pValue);
    *pValue={};
    IFC(CheckThread());
    IFC(static_cast<AppBarButton*>(this)->get_KeyboardAcceleratorTextOverrideImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_KeyboardAcceleratorTextOverride(_In_opt_ HSTRING value)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->put_KeyboardAcceleratorTextOverrideImpl(value));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_Label(_Out_ HSTRING* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_Label, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_Label(_In_opt_ HSTRING value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_Label, value));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_LabelPosition(_Out_ ABI::Microsoft::UI::Xaml::Controls::CommandBarLabelPosition* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_LabelPosition, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_LabelPosition(ABI::Microsoft::UI::Xaml::Controls::CommandBarLabelPosition value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_LabelPosition, value));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_ParentOwner(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::ISubMenuOwner** ppValue)
{
    HRESULT hr = S_OK;
    ARG_VALIDRETURNPOINTER(ppValue);
    *ppValue={};
    IFC(CheckThread());
    IFC(static_cast<AppBarButton*>(this)->get_ParentOwnerImpl(ppValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_ParentOwner(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::ISubMenuOwner* pValue)
{
    HRESULT hr = S_OK;
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->put_ParentOwnerImpl(pValue));
Cleanup:
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_TemplateSettings(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IAppBarButtonTemplateSettings** ppValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_TemplateSettings, ppValue));
}
_Check_return_ HRESULT DirectUI::AppBarButtonGenerated::put_TemplateSettings(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::Primitives::IAppBarButtonTemplateSettings* pValue)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_TemplateSettings, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::get_UseOverflowStyle(_Out_ BOOLEAN* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::AppBarButton_UseOverflowStyle, pValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::put_UseOverflowStyle(BOOLEAN value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::AppBarButton_UseOverflowStyle, value));
}

// Events.

// Methods.
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::CancelCloseSubMenu()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_CancelCloseSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->CancelCloseSubMenuImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_CancelCloseSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::ClosePeerSubMenus()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_ClosePeerSubMenus", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->ClosePeerSubMenusImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_ClosePeerSubMenus", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::CloseSubMenu()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_CloseSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->CloseSubMenuImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_CloseSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::CloseSubMenuTree()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_CloseSubMenuTree", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->CloseSubMenuTreeImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_CloseSubMenuTree", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::DelayCloseSubMenu()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_DelayCloseSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->DelayCloseSubMenuImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_DelayCloseSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::GetHasBottomLabel(_Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_GetHasBottomLabel", 0);
    }
    ARG_VALIDRETURNPOINTER(pResult);
    *pResult={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->GetHasBottomLabelImpl(pResult));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_GetHasBottomLabel", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::GetHasRightLabel(_Out_ BOOLEAN* pResult)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_GetHasRightLabel", 0);
    }
    ARG_VALIDRETURNPOINTER(pResult);
    *pResult={};
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->GetHasRightLabelImpl(pResult));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_GetHasRightLabel", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::OpenSubMenu(ABI::Windows::Foundation::Point position)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_OpenSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->OpenSubMenuImpl(position));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_OpenSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::PositionSubMenu(ABI::Windows::Foundation::Point position)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_PositionSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->PositionSubMenuImpl(position));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_PositionSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::PrepareSubMenu()
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_PrepareSubMenu", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->PrepareSubMenuImpl());
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_PrepareSubMenu", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::RaiseAutomationPeerExpandCollapse(BOOLEAN isOpen)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_RaiseAutomationPeerExpandCollapse", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->RaiseAutomationPeerExpandCollapseImpl(isOpen));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_RaiseAutomationPeerExpandCollapse", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::SetDefaultLabelPosition(ABI::Microsoft::UI::Xaml::Controls::CommandBarDefaultLabelPosition defaultLabelPosition)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_SetDefaultLabelPosition", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->SetDefaultLabelPositionImpl(defaultLabelPosition));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_SetDefaultLabelPosition", hr);
    }
    RRETURN(hr);
}
IFACEMETHODIMP DirectUI::AppBarButtonGenerated::SetSubMenuDirection(BOOLEAN isSubMenuDirectionUp)
{
    HRESULT hr = S_OK;
    if (EventEnabledApiFunctionCallStart())
    {
        XamlTelemetry::PublicApiCall(true, reinterpret_cast<uint64_t>(this), "AppBarButton_SetSubMenuDirection", 0);
    }
    
    IFC(CheckThread());
    IFC(DefaultStrictApiCheck(this));
    IFC(static_cast<AppBarButton*>(this)->SetSubMenuDirectionImpl(isSubMenuDirectionUp));
Cleanup:
    if (EventEnabledApiFunctionCallStop())
    {
        XamlTelemetry::PublicApiCall(false, reinterpret_cast<uint64_t>(this), "AppBarButton_SetSubMenuDirection", hr);
    }
    RRETURN(hr);
}

HRESULT DirectUI::AppBarButtonFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarButtonFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarButtonFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarButtonStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::IAppBarButtonStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::AppBarButtonFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::IAppBarButton** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::IAppBarButton);
    const GUID metadataAPIGUID = MetadataAPI::GetClassInfoByIndex(GetTypeIndex())->GetGuid();
    const KnownTypeIndex typeIndex = GetTypeIndex();

    if(uuidofGUID != metadataAPIGUID)
    {
        XAML_FAIL_FAST();
    }
#endif

    // Can't just IFC(_RETURN) this because for some validate calls (those with multiple template parameters), the
    // preprocessor gets confused at the "," in the template type-list before the function's opening parenthesis.
    // So we'll use IFC_RETURN syntax with a local hr variable, kind of weirdly.
    const HRESULT hr = ctl::ValidateFactoryCreateInstanceWithBetterAggregableCoreObjectActivationFactory(pOuter, ppInner, reinterpret_cast<IUnknown**>(ppInstance), GetTypeIndex(), false /*isFreeThreaded*/);
    IFC_RETURN(hr);
    return S_OK;
}

// Dependency properties.
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_LabelProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_Label, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_IconProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_Icon, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_LabelPositionProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_LabelPosition, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_KeyboardAcceleratorTextOverrideProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_KeyboardAcceleratorTextOverride, ppValue));
}

IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_IsCompactProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_IsCompact, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_IsInOverflowProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_IsInOverflow, ppValue));
}
IFACEMETHODIMP DirectUI::AppBarButtonFactory::get_DynamicOverflowOrderProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::AppBarButton_DynamicOverflowOrder, ppValue));
}


// Attached properties.

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_AppBarButton()
    {
        RRETURN(ctl::ActivationFactoryCreator<AppBarButtonFactory>::CreateActivationFactory());
    }
}
