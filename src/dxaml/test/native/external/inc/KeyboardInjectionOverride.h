// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class KeyboardInjectionIgnoreEventWaitOverride
        {
        public:

            KeyboardInjectionIgnoreEventWaitOverride(KeyboardWaitKind waitKind)
            {
                TestServices::KeyboardHelper->SetWaitKind(waitKind);
            }

            KeyboardInjectionIgnoreEventWaitOverride() : KeyboardInjectionIgnoreEventWaitOverride(KeyboardWaitKind::Sleep)
            {
            }

            ~KeyboardInjectionIgnoreEventWaitOverride()
            {
                TestServices::KeyboardHelper->SetWaitKind(KeyboardWaitKind::Default);
            }
        };
    }
} } } }
