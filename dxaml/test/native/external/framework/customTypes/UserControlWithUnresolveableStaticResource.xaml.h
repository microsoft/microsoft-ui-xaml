// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "UserControlWithUnresolveableStaticResource.g.h"

namespace Tests { namespace Native { namespace External { namespace Framework {
    namespace ResourceDictionary
    {
        public ref class UserControlWithUnresolveableStaticResource sealed 
        {
        public:
            UserControlWithUnresolveableStaticResource() 
            {
                // The generated InitializeComponent() doesn't work for our tests because
                // we don't binplace .xaml/.xbf in the expected locations so we manually
                // call LoadComponent() with a hand-crafted URI.
                if (_contentLoaded)
                {
                    return;
                }
                _contentLoaded = true;
                ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///UserControlWithUnresolveableStaticResource.xaml");

                try
                {
                    ::Microsoft::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
                }
                catch (Platform::COMException^)
                {
                    // Swallow the exception to prevent this control from being leaked
                }
            }
        };
    }
} } } }
