// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Animal.g.h"

namespace winrt::BindTestbedCppWinRTModel::implementation
{
    struct Animal : AnimalT<Animal>
    {
        Animal() = default;

        Animal(hstring const& name, hstring const& color);
        hstring Name();
        hstring Color();
        void Click_RegularArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Click_NoArgsOnAnimal();
        void Click_BaseArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& e);
        void Click_OverloadedArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& e);

    private:
        hstring m_name;
        hstring m_color;
    };
}
namespace winrt::BindTestbedCppWinRTModel::factory_implementation
{
    struct Animal : AnimalT<Animal, implementation::Animal>
    {
    };
}
