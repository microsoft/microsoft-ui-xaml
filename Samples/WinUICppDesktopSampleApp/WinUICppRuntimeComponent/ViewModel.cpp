// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ViewModel.h"
#if __has_include("ViewModel.g.cpp")
#include "ViewModel.g.cpp"
#endif

#include <winrt/Microsoft.UI.Xaml.h>

using namespace winrt;
using namespace Microsoft::UI::Xaml;

DependencyProperty InitializeDependencyProperty(
    const std::wstring_view& propertyNameString,
    const std::wstring_view& propertyTypeNameString,
    const std::wstring_view& ownerTypeNameString,
    const winrt::Windows::Foundation::IInspectable& defaultValue,
    const PropertyChangedCallback& propertyChangedCallback = nullptr)
{
    auto propertyType = Windows::UI::Xaml::Interop::TypeName();
    propertyType.Name = propertyTypeNameString;
    propertyType.Kind = Windows::UI::Xaml::Interop::TypeKind::Metadata;

    auto ownerType = Windows::UI::Xaml::Interop::TypeName();
    ownerType.Name = ownerTypeNameString;
    ownerType.Kind = Windows::UI::Xaml::Interop::TypeKind::Metadata;

    auto propertyMetadata = PropertyMetadata(defaultValue, propertyChangedCallback);
    return DependencyProperty::Register(propertyNameString, propertyType, ownerType, propertyMetadata);
}

namespace winrt::WinUICppRuntimeComponent::implementation
{
    Microsoft::UI::Xaml::DependencyProperty ViewModel::s_textProperty{ nullptr };
    Microsoft::UI::Xaml::DependencyProperty ViewModel::s_boolPropertyProperty{ nullptr };

    ViewModel::ViewModel()
    {
        RegisterDependencyProperties();
    }

    hstring ViewModel::Text()
    {
        return unbox_value<hstring>(GetValue(s_textProperty));
    }

    void ViewModel::Text(hstring value)
    {
        SetValue(s_textProperty, box_value(value));
    }

    bool ViewModel::BoolProperty()
    {
        return unbox_value<bool>(GetValue(s_boolPropertyProperty));
    }

    void ViewModel::BoolProperty(bool value)
    {
        SetValue(s_boolPropertyProperty, box_value(value));
    }

    void ViewModel::RegisterDependencyProperties()
    {
        if (!s_textProperty)
        {
            s_textProperty = InitializeDependencyProperty(
                L"Text",
                name_of<hstring>(),
                name_of<WinUICppRuntimeComponent::ViewModel>(),
                box_value(L""));
        }

        if (!s_boolPropertyProperty)
        {
            s_boolPropertyProperty = InitializeDependencyProperty(
                L"BoolProperty",
                name_of<bool>(),
                name_of<WinUICppRuntimeComponent::ViewModel>(),
                box_value(false));
        }
    }
}
