// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "XDeferCustomPageResourcesDT.xaml.h"

namespace Tests::Native::External::Framework::XDefer {
    CustomPageResourcesDT::CustomPageResourcesDT()
    {
        ::Microsoft::UI::Xaml::Application::LoadComponent(
            this,
            ref new ::Windows::Foundation::Uri(L"ms-appx:///XDeferCustomPageResourcesDT.xaml"),
            ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
    }

    CustomPageResourcesDT::~CustomPageResourcesDT()
    {
    }
}
