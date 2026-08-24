#pragma once
#include "App.xaml.g.h"

namespace winrt::XAMLPerf_MinApp_Cpp_WUX::implementation
{
    struct App : AppT<App>
    {
        App();

        void OnLaunched(winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs const&);
        void OnNavigationFailed(IInspectable const&, winrt::Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const&);
    };
}
