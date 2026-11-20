// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ParserTests.CustomSelectors.h"
#include "CustomListViewRoot.g.h"

namespace Tests { namespace Native { namespace External { namespace Framework { namespace Parser {
    namespace Selectors
    {
        // The ctor loads the compiled XAML with itself as the provided root, like InitializeComponent.
        public ref class CustomListViewRoot sealed
        {
        public:
            CustomListViewRoot()
            {
                if (_contentLoaded)
                {
                    return;
                }
                _contentLoaded = true;
                ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///CustomListViewRoot.xaml");
                ::Microsoft::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Microsoft::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Nested);
            }
        };
    }
} } } } }
