// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "AttachedPropSampleExtensions.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct AttachedPropSampleExtensions : AttachedPropSampleExtensionsT<AttachedPropSampleExtensions>
    {
        AttachedPropSampleExtensions() = default;

        static Microsoft::UI::Xaml::DependencyProperty MyAttachedPropProperty();
        static Microsoft::UI::Xaml::DependencyObject GetMyAttachedProp(Microsoft::UI::Xaml::DependencyObject const& target);
        static void SetMyAttachedProp(Microsoft::UI::Xaml::DependencyObject const& target, Microsoft::UI::Xaml::DependencyObject const& value);

    private:
        static  Microsoft::UI::Xaml::DependencyProperty s_myAttachedPropProperty;
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct AttachedPropSampleExtensions : AttachedPropSampleExtensionsT<AttachedPropSampleExtensions, implementation::AttachedPropSampleExtensions>
    {
    };
}