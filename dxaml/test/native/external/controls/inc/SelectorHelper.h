// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class SelectorHelper
    {
    public:
        // Verify the selected Index on the Selector.
        static void VerifySelectedIndex(xaml_primitives::Selector^ selector, int expected)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Selected Index = %d", selector->SelectedIndex);
                VERIFY_ARE_EQUAL(expected, selector->SelectedIndex);
            });
        }
    };

} } } } }
