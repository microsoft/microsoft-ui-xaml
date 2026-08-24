//
// pch.h
// Header for standard system include files.
//

#pragma once

// Avoid adding SDK headers here, to test that generated code
// includes all its dependencies.
#include <unknwn.h>
#undef GetCurrentTime

// References

#include "winrt/BindTestbedModel.h"
#ifdef DESKTOP
    #include "winrt/BindTestbedModel.BindTestbedModelDesktop_XamlTypeInfo.h"
#else
    #include "winrt/BindTestbedModel.BindTestbedModel_XamlTypeInfo.h"
#endif

namespace winrt::BindTestbed::implementation
{
    namespace wa = ::Windows::ApplicationModel;
    namespace wf = ::Windows::Foundation;
    namespace wfc = ::Windows::Foundation::Collections;
    namespace wux = Microsoft::UI::Xaml;
    namespace wuxc = Microsoft::UI::Xaml::Controls;
}

// Model
#include "CastingModel.h"
#include "MainModel.h"

inline winrt::Microsoft::UI::Xaml::DependencyProperty RegisterDependencyProperty(
    winrt::hstring const& name,
    winrt::Windows::UI::Xaml::Interop::TypeName const& propertyType,
    winrt::Windows::UI::Xaml::Interop::TypeName const& ownerType,
    winrt::Microsoft::UI::Xaml::PropertyMetadata const& metadata)
{
#ifdef DESKTOP
    struct ensure_initialize { ensure_initialize() { winrt::init_apartment(winrt::apartment_type::single_threaded); } } static init;
#else
    struct ensure_initialize { ensure_initialize() { winrt::init_apartment(winrt::apartment_type::multi_threaded); } } static init;
#endif
    return winrt::Microsoft::UI::Xaml::DependencyProperty::Register(name, propertyType, ownerType, metadata);
}

namespace winrt
{
    // say developer really wants to be able to bind certain enums to hstring
    inline hstring to_hstring(Microsoft::UI::Xaml::Visibility const& value)
    {
        return hstring{ value == Microsoft::UI::Xaml::Visibility::Visible ? L"Visible" : L"Hidden" };
    }
    inline hstring to_hstring(::Windows::UI::Color const& value)
    {
        std::wstring color = L"";
        color.append(std::to_wstring(value.A));
        color.append(std::to_wstring(value.R));
        color.append(std::to_wstring(value.G));
        color.append(std::to_wstring(value.B));
        return hstring{ color.data() };
    }

    // For scenarios where IInspectable boxed into string
    inline hstring to_hstring(::Windows::Foundation::IInspectable const& value)
    {
        return unbox_value<hstring>(value);
    }
}