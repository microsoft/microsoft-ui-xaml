// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RootCustomResourceDialog.g.h"

namespace Tests { namespace Native { namespace External { namespace Framework {
    namespace ResourceLoading
    {
        // Faithful repro. This is a markup-compiled (genxbf) ContentDialog
        // subclass whose ROOT element sets PrimaryButtonText via a {CustomResource} markup
        // extension, mirroring the exact reported scenario. The ctor loads the compiled XAML
        // with itself as the provided root, just like the framework-generated InitializeComponent.
        public ref class RootCustomResourceDialog sealed
        {
        public:
            RootCustomResourceDialog()
            {
                if (_contentLoaded)
                {
                    return;
                }
                _contentLoaded = true;
                ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///RootCustomResourceDialog.xaml");
                ::Microsoft::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
            }
        };
    }
} } } }
