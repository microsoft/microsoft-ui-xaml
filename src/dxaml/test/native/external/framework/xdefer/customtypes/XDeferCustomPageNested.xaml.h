// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XDeferCustomPageNested.g.h"

namespace Private {
    namespace Tests {
        namespace Framework {
            namespace XDefer {
                [::Windows::Foundation::Metadata::WebHostHidden]
                public ref class CustomPageNested sealed
                {
                public:
                    CustomPageNested();
                    virtual ~CustomPageNested();
                    void level1Grid_Loaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
                    void level1Grid_Unloaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
                    void level2Button_Loaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
                    void level2Button_Unloaded(::Platform::Object^ sender, ::Microsoft::UI::Xaml::RoutedEventArgs^ args);
                };
            }
        }
    }
}
