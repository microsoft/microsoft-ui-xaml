#pragma once
#include "App.xaml.g.h"

namespace winrt::XAMLPerf_TextBlockApp_Cpp_MUX::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(Microsoft::UI::Xaml::LaunchActivatedEventArgs const&);
        void OnNavigationFailed(IInspectable const&, Microsoft::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
