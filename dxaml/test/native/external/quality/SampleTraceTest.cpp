// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SampleTraceTest.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FileLoader.h"
#include "TraceConsumerSession.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;
using namespace Concurrency;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Quality {
        bool SampleTraceTest::ClassSetup()
        {


            // It's very important to call EnsureInitialized on TestServices
            // from ClassSetup. This method will wait for the window to be
            // activated on launch, which avoids a race condition that will block
            // input from being routed to the app. It will also wait for the
            // debugger to attach when the waitForDebugger runtime parameter is
            // specified.
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool SampleTraceTest::TestCleanup()
        {
            //
            // It's very important to have your test clean up the window contents
            // when it completes. When creating new tests be sure to copy this
            // method over or implement it in a similar way. By cleaning
            // up the window content and waiting for the page to go idle you ensure
            // that if your test fails while the UI element tree is being torn down
            // that the failure is associated with your test and doesn't occur
            // nondeterministically in the future. By waiting for the page to go
            // idle you ensure that all transitions have completed and that jupiter
            // is in a 'tabula rasa' state for the next test.
            //
            // Use the TestCleanupWrapper in each test method to handle cleanup, even
            // in cases of failure or repeated runs. Use VerifyTestCleanup here to
            // ensure that the test was cleaned up correctly.
            //
            TestServices::WindowHelper->VerifyTestCleanup();

            return true;
        }

        // Test to verify FirstUiThreadFrameEnd event is traced
        void SampleTraceTest::TestMethod()
        {
            TestCleanupWrapper cleanup;

            // TraceConsumer::Start has to be called within the test method
            TraceConsumerSession session;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->SetupSimulatedAppPage();
            });

            TestServices::WindowHelper->WaitForIdle();

            // verify traces
            // If test runs in a loop, the trace should occur only the first iteration.
            session.Stop();
            TraceConsumer::VerifyEventTraced("FirstUiThreadFrameEnd", testMethodExecutedCount++ == 0 ? 1 : 0);
        }
    }
} } } }
