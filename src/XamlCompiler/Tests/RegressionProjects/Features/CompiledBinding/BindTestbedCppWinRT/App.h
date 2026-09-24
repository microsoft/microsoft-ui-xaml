#pragma once

// Adding SDK headers that link in XamlTypeInfo.g.cpp here, to test that
// all other generated code includes all its dependencies. These header
// names cannot be inferred by the Xaml Compiler, so we can't automatically
// add them to XamlTYpeInfo.g.cpp.
#include "winrt/Microsoft.UI.Xaml.Controls.Primitives.h"
#include "winrt/Microsoft.UI.Xaml.Documents.h"
#include "winrt/Microsoft.UI.Xaml.Input.h"
#include "winrt/Microsoft.UI.Xaml.Media.h"
#include "winrt/Microsoft.UI.Xaml.Shapes.h"

#include "App.xaml.g.h"

namespace winrt::BindTestbed::implementation
{
    struct App : public AppT<App>
    {
    public:
        App();

        static BindTestbedModel::DataModel Model;
        static BindTestbedModel::DOModel DOModel;
        //TODO: Convert BindTestbedModelCX to C++/WinRT
        //static BindTestbedCXModel::ModelCX ModelCX

        void OnLaunched(wux::LaunchActivatedEventArgs const& e);
        void OnSuspending(IInspectable const& sender, wa::SuspendingEventArgs const& e);
        void OnNavigationFailed(IInspectable const& sender, wux::Navigation::NavigationFailedEventArgs const& e);
    };
}