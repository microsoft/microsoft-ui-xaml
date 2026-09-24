// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// pch.h
// Header for standard system include files.
//

#pragma once

#include "hstring.h"
#include "windows.h"

#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.UI.h"
#include "winrt/Windows.UI.Popups.h"
#include "winrt/Microsoft.UI.Xaml.h"
#include "winrt/Microsoft.UI.Xaml.Controls.h"
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Data.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Xaml.Interop.h"
#include "winrt/Microsoft.UI.Xaml.Markup.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Xaml.Navigation.h"
#include "winrt/Microsoft.UI.Xaml.Shapes.h"

// References
#include "winrt/BindTestbedModel.h"
#include "winrt/BindTestbedCXModel.h"

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
    winrt::Microsoft::UI::Xaml::Interop::TypeName const& propertyType,
    winrt::Microsoft::UI::Xaml::Interop::TypeName const& ownerType,
    winrt::Microsoft::UI::Xaml::PropertyMetadata const& metadata)
{
    struct ensure_initialize { ensure_initialize() { winrt::init_apartment(); } } static init;
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