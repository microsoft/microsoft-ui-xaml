// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

        class AsyncImageDecoderTests : public WEX::TestClass<AsyncImageDecoderTests>
        {
        public:
            BEGIN_TEST_CLASS(AsyncImageDecoderTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(InitialState)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the initial state of the AsyncImageDecoder object.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeDecodeSizeStatic)
                TEST_METHOD_PROPERTY(L"Description", L"Update decode params for static image.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeDecodeSizeAnimatedPlaying)
                TEST_METHOD_PROPERTY(L"Description", L"Update decode params for animated image that is playing.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeDecodeSizeAnimatedStopped)
                TEST_METHOD_PROPERTY(L"Description", L"Update decode params for animated image that is stopped.")
            END_TEST_METHOD()
        };
    }}
} } } }
