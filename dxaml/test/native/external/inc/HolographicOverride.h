// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {
        class HolographicOverride
        {
        public:
            HolographicOverride()
            {
                RunOnUIThread([&]()
                {
                   test_infra::TestServices::WindowHelper->SetIsHolographic(true);
                });
            }

            ~HolographicOverride()
            {
                RunOnUIThread([&]()
                {
                    test_infra::TestServices::WindowHelper->SetIsHolographic(false);
                });
            }
        };
    }
} } } }
