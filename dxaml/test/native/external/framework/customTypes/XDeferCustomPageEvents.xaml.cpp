// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XDeferCustomPageEvents.xaml.h"
#include <TestEvent.h>

namespace Tests::Native::External::Framework::XDefer {

    std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spLoadedEvent0;
    std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spUnloadedEvent0;
    std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spLoadedEvent1;
    std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spUnloadedEvent1;

    CustomPageEvents::CustomPageEvents()
    {
        s_spLoadedEvent0 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
        s_spUnloadedEvent0 = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();

        ::Microsoft::UI::Xaml::Application::LoadComponent(
            this,
            ref new ::Windows::Foundation::Uri(L"ms-appx:///XDeferCustomPageEvents.xaml"),
            ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
    }

    CustomPageEvents::~CustomPageEvents()
    {
        s_spLoadedEvent0 = nullptr;
        s_spUnloadedEvent0 = nullptr;
    }

    void CustomPageEvents::button_Loaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spLoadedEvent0->Set();
    }

    void CustomPageEvents::button_Unloaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args)
    {
        s_spUnloadedEvent0->Set();
    }

    void CustomPageEvents::reset_button_field()
    {
        button = nullptr;
    }
}
