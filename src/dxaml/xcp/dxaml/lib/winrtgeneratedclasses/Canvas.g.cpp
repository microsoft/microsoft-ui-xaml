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

#include "Canvas.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::Canvas::Canvas()
{
}

DirectUI::Canvas::~Canvas()
{
}

HRESULT DirectUI::Canvas::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::Canvas)))
    {
        *ppObject = static_cast<DirectUI::Canvas*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICanvas)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICanvas*>(this);
    }
    else
    {
        RRETURN(DirectUI::Panel::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.

HRESULT DirectUI::CanvasFactory::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICanvasFactory)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICanvasFactory*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICanvasStatics)))
    {
        *ppObject = static_cast<ABI::Microsoft::UI::Xaml::Controls::ICanvasStatics*>(this);
    }
    else
    {
        RRETURN(ctl::BetterAggregableCoreObjectActivationFactory::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}


// Factory methods.
IFACEMETHODIMP DirectUI::CanvasFactory::CreateInstance(_In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInner, _Outptr_ ABI::Microsoft::UI::Xaml::Controls::ICanvas** ppInstance)
{

#if DBG
    // We play some games with reinterpret_cast and assuming that the GUID type table is accurate - which is somewhat sketchy, but
    // really good for binary size.  This code is a sanity check that the games we play are ok.
    const GUID uuidofGUID = __uuidof(ABI::Microsoft::UI::Xaml::Controls::ICanvas);
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

// Attached properties.
_Check_return_ HRESULT DirectUI::CanvasFactory::GetLeftStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_Left, pLength));
}

_Check_return_ HRESULT DirectUI::CanvasFactory::SetLeftStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_Left, length));
}


IFACEMETHODIMP DirectUI::CanvasFactory::get_LeftProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Canvas_Left, ppValue));
}


IFACEMETHODIMP DirectUI::CanvasFactory::GetLeft(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength)
{
    RRETURN(GetLeftStatic(pElement, pLength));
}

IFACEMETHODIMP DirectUI::CanvasFactory::SetLeft(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length)
{
    RRETURN(SetLeftStatic(pElement, length));
}
_Check_return_ HRESULT DirectUI::CanvasFactory::GetTopStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_Top, pLength));
}

_Check_return_ HRESULT DirectUI::CanvasFactory::SetTopStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_Top, length));
}


IFACEMETHODIMP DirectUI::CanvasFactory::get_TopProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Canvas_Top, ppValue));
}


IFACEMETHODIMP DirectUI::CanvasFactory::GetTop(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ DOUBLE* pLength)
{
    RRETURN(GetTopStatic(pElement, pLength));
}

IFACEMETHODIMP DirectUI::CanvasFactory::SetTop(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, DOUBLE length)
{
    RRETURN(SetTopStatic(pElement, length));
}
_Check_return_ HRESULT DirectUI::CanvasFactory::GetZIndexStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ INT* pValue)
{
    RRETURN(DependencyObject::GetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_ZIndex, pValue));
}

_Check_return_ HRESULT DirectUI::CanvasFactory::SetZIndexStatic(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, INT value)
{
    RRETURN(DependencyObject::SetAttachedValueByKnownIndex(static_cast<DirectUI::UIElement*>(pElement), KnownPropertyIndex::Canvas_ZIndex, value));
}


IFACEMETHODIMP DirectUI::CanvasFactory::get_ZIndexProperty(_Out_ ABI::Microsoft::UI::Xaml::IDependencyProperty** ppValue)
{
    RRETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::Canvas_ZIndex, ppValue));
}


IFACEMETHODIMP DirectUI::CanvasFactory::GetZIndex(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, _Out_ INT* pValue)
{
    RRETURN(GetZIndexStatic(pElement, pValue));
}

IFACEMETHODIMP DirectUI::CanvasFactory::SetZIndex(_In_ ABI::Microsoft::UI::Xaml::IUIElement* pElement, INT value)
{
    RRETURN(SetZIndexStatic(pElement, value));
}

// Static properties.

// Static methods.

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_Canvas()
    {
        RRETURN(ctl::ActivationFactoryCreator<CanvasFactory>::CreateActivationFactory());
    }
}
