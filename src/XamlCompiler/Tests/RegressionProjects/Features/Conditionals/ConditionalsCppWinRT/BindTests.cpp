// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "BindTests.h"

using namespace winrt::ConditionalsCppWinRT;
using namespace winrt::ConditionalControls;

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Data;
using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Navigation;

namespace winrt::ConditionalsCppWinRT::implementation
{
    BindTests::BindTests()
    {
        model = winrt::ConditionalControls::Model();
        InitializeComponent();
    }


    void BindTests::Click_V2(::Windows::Foundation::IInspectable const& sender, wux::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        clickV2results().Text(obj.V2Property());
    }

    void BindTests::Click_V3(::Windows::Foundation::IInspectable const& sender, wux::RoutedEventArgs const&)
    {
        IVersionedProperties obj = (IVersionedProperties)sender.try_as<IVersionedProperties>();
        clickV2results().Text(obj.V3Property());
    }
}
