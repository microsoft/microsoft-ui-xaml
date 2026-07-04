// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ViewModel.g.h"
#include <winrt/Microsoft.UI.Xaml.h>

namespace winrt::WinUICppRuntimeComponent::implementation
{
    struct ViewModel : ViewModelT<ViewModel>
    {
        ViewModel();

        winrt::hstring Text();
        void Text(winrt::hstring value);

        bool BoolProperty();
        void BoolProperty(bool value);

    private:
        static void RegisterDependencyProperties();

        static Microsoft::UI::Xaml::DependencyProperty s_textProperty;
        static Microsoft::UI::Xaml::DependencyProperty s_boolPropertyProperty;
    };
}

namespace winrt::WinUICppRuntimeComponent::factory_implementation
{
    struct ViewModel : ViewModelT<ViewModel, implementation::ViewModel>
    {
    };
}
