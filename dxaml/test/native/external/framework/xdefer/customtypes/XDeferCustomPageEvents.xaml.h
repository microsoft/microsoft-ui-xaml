// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XDeferCustomPageEvents.g.h"

namespace Microsoft::UI::Xaml::Tests::Common {
    class Event;
} 

namespace Private::Tests::Framework::XDefer {

    extern std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spLoadedEvent0;
    extern std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spUnloadedEvent0;
    extern std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spLoadedEvent1;
    extern std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> s_spUnloadedEvent1;

    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class CustomPageEvents sealed
    {
    public:
        CustomPageEvents();
        virtual ~CustomPageEvents();
        void button_Loaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
        void button_Unloaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
        void reset_button_field();
    };

}
