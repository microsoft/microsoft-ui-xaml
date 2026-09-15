// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Animal.h"
#include "Animal.g.cpp"
#include <winrt/Microsoft.UI.Xaml.Controls.h>

namespace winrt::BindTestbedCppWinRTModel::implementation
{
    Animal::Animal(hstring const& name, hstring const& color)
        : m_name(name), m_color(color)
    {
    }

    hstring Animal::Name()
    {
        return m_name;
    }

    hstring Animal::Color()
    {
        return m_color;
    }

    void Animal::Click_RegularArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        auto dlg = winrt::Microsoft::UI::Xaml::Controls::ContentDialog();
        dlg.Content(box_value(L"Regular arguments clicked on Animal"));
        dlg.CloseButtonText(L"Ok");
        dlg.XamlRoot(sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>().XamlRoot());

        dlg.ShowAsync();
    }
    void Animal::Click_NoArgsOnAnimal()
    {
        auto dlg = winrt::Microsoft::UI::Xaml::Controls::ContentDialog();
        dlg.Content(box_value(L"No argument Clicked on Animal"));
        dlg.CloseButtonText(L"Ok");

        dlg.ShowAsync();
    }
    void Animal::Click_BaseArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Microsoft::UI::Xaml::RoutedEventArgs const& /*e*/)
    {
        auto dlg = winrt::Microsoft::UI::Xaml::Controls::ContentDialog();
        dlg.Content(box_value(L"Base argument clicked on Animal"));
        dlg.CloseButtonText(L"Ok");
        dlg.XamlRoot(sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>().XamlRoot());

        dlg.ShowAsync();
    }
    void Animal::Click_OverloadedArgsOnAnimal(winrt::Windows::Foundation::IInspectable const& sender, winrt::Windows::Foundation::IInspectable const& /*e*/)
    {
        auto dlg = winrt::Microsoft::UI::Xaml::Controls::ContentDialog();
        dlg.Content(box_value(L"Overloaded argument clicked on Animal"));
        dlg.CloseButtonText(L"Ok");
        dlg.XamlRoot(sender.as<winrt::Microsoft::UI::Xaml::Controls::Button>().XamlRoot());

        dlg.ShowAsync();
    }
}
