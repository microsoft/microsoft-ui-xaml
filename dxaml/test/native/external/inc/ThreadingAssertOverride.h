// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {
        class ThreadingAssertOverride
        {
        public:
            ThreadingAssertOverride()
            {
                RunOnUIThread([&]()
                {
                   WEX::Logging::Log::Comment(L"Disabling threading assert");
                   test_infra::TestServices::WindowHelper->SetThreadingAssertOverride(false);
                });
            }

            ~ThreadingAssertOverride()
            {
                RunOnUIThread([&]()
                {
                    WEX::Logging::Log::Comment(L"Enabling threading assert");
                    test_infra::TestServices::WindowHelper->SetThreadingAssertOverride(true);
                });
            }
        };
    }
} } } }
