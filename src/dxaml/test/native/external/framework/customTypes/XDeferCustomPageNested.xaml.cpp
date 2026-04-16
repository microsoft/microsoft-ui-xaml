// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XDeferCustomPageNested.xaml.h"
#include "XDeferCustomPageEvents.xaml.h"
#include <TestEvent.h>

namespace Tests::Native::External::Framework::XDefer {
    CustomPageNested::CustomPageNested()
    {
        s_spLoadedEvent0 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        s_spUnloadedEvent0 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        s_spLoadedEvent1 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        s_spUnloadedEvent1 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();

        ::Microsoft::UI::Xaml::Application::LoadComponent(
            this,
            ref new ::Windows::Foundation::Uri(L"ms-appx:///XDeferCustomPageNested.xaml"),
            ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
    }

    CustomPageNested::~CustomPageNested()
    {
        s_spLoadedEvent0 = nullptr;
        s_spUnloadedEvent0 = nullptr;
        s_spLoadedEvent1 = nullptr;
        s_spUnloadedEvent1 = nullptr;
    }

    void CustomPageNested::level1Grid_Loaded(::Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spLoadedEvent0->Set();
    }

    void CustomPageNested::level1Grid_Unloaded(::Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spUnloadedEvent0->Set();
    }

    void CustomPageNested::level2Button_Loaded(::Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spLoadedEvent1->Set();
    }

    void CustomPageNested::level2Button_Unloaded(::Platform::Object^ sender, Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spUnloadedEvent1->Set();
    }
}
