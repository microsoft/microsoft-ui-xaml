#pragma once

#include "App.xaml.g.h"

namespace winrt::DisableXamlGeneratedMainNoCtorCpp::implementation
{
    struct App : AppT<App>
    {
        // This sample intentionally has NO parameterless constructor. The developer
        // supplies their own entry point (see program.cpp) and constructs the App with
        // this parameterized constructor. Because DISABLE_XAML_GENERATED_MAIN is defined,
        // the XamlCompiler must NOT emit the parameterless constructor call in the
        // generated wXamlGeneratedMain() helper.
        App(int launchId);

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);

    private:
        winrt::Microsoft::UI::Xaml::Window window{ nullptr };
    };
}
