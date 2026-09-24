// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "AttachedPropSampleExtensions.h"
#if __has_include("AttachedPropSampleExtensions.g.cpp")
#include "AttachedPropSampleExtensions.g.cpp"
#endif

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    Microsoft::UI::Xaml::DependencyProperty AttachedPropSampleExtensions::s_myAttachedPropProperty =
        Microsoft::UI::Xaml::DependencyProperty::RegisterAttached(
            L"MyAttachedProp",
            winrt::xaml_typename<Microsoft::UI::Xaml::DependencyProperty>(),
            winrt::xaml_typename<WinUICppDesktopSampleApp::AttachedPropSampleExtensions>(),
            Microsoft::UI::Xaml::PropertyMetadata{ nullptr });

    Microsoft::UI::Xaml::DependencyProperty AttachedPropSampleExtensions::MyAttachedPropProperty()
    {
        return s_myAttachedPropProperty;
    }
    Microsoft::UI::Xaml::DependencyObject AttachedPropSampleExtensions::GetMyAttachedProp(Microsoft::UI::Xaml::DependencyObject const& target)
    {
        return target.GetValue(s_myAttachedPropProperty).as<Microsoft::UI::Xaml::DependencyObject>();
    }
    void AttachedPropSampleExtensions::SetMyAttachedProp(Microsoft::UI::Xaml::DependencyObject const& target, Microsoft::UI::Xaml::DependencyObject const& value)
    {
        target.SetValue(s_myAttachedPropProperty, value);
    }
}
