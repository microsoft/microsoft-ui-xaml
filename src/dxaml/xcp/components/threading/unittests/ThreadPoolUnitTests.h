// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Threading {

        class ThreadPoolUnitTests : public WEX::TestClass<ThreadPoolUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ThreadPoolUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            BEGIN_TEST_METHOD(RunAsyncFreeThreaded)
                TEST_METHOD_PROPERTY(L"Description", L"Tests running an async job from the thread pool.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateTimerFreeThreaded)
                TEST_METHOD_PROPERTY(L"Description", L"Tests running a timer from the thread pool.")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(RunAsyncUIThread)
                TEST_METHOD_PROPERTY(L"Description", L"Tests running an async job from the thread pool which will run on the UI thread.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateTimerUIThread)
                TEST_METHOD_PROPERTY(L"Description", L"Tests running a timer from the thread pool which will run on the UI thread.")
            END_TEST_METHOD()
        };
    }}
} } } }
