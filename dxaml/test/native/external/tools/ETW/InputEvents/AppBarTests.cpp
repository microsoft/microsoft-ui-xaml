// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "TraceConsumerSession.h"
#include "MUX-ETWEvents.h"
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace ETW { namespace InputEvents {

    bool AppBarTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool AppBarTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //

    void AppBarTests::AppBarEvents()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto spOpenedEvent = std::make_shared<Event>();
        auto spClosedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            appBar = ref new xaml_controls::AppBar();

            openedRegistration.Attach(appBar, ref new wf::EventHandler<Platform::Object^>([spOpenedEvent](Platform::Object^ sender, Platform::Object^ e) {
                spOpenedEvent->Set();
            }));

            closedRegistration.Attach(appBar, ref new wf::EventHandler<Platform::Object^>([spClosedEvent](Platform::Object^ sender, Platform::Object^ e) {
                spClosedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TraceConsumerSession traceSession(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER);
        TraceConsumer::EnableTracingByEventId(AppBarOpenBegin_value);
        TraceConsumer::EnableTracingByEventId(AppBarOpenEnd_value);
        TraceConsumer::EnableTracingByEventId(AppBarClosedBegin_value);
        TraceConsumer::EnableTracingByEventId(AppBarClosedEnd_value);
        // Verify open/close for top appbar.
        LOG_OUTPUT(L"Verify open/close for top appbar.");
        RunOnUIThread([&]()
        {
            page->TopAppBar = appBar;
            appBar->IsOpen = true;
        });
        spOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        spClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->TopAppBar = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify open/close for bottom appbar.
        LOG_OUTPUT(L"Verify open/close for bottom appbar.");
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
            appBar->IsOpen = true;
        });
        spOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        spClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        traceSession.Stop();

        // Verify each event fired twice, one for top and one for bottom
        TraceConsumer::VerifyEventTraced(AppBarOpenBegin_value, 2);
        TraceConsumer::VerifyEventTraced(AppBarOpenEnd_value, 2);
        TraceConsumer::VerifyEventTraced(AppBarClosedBegin_value, 2);
        TraceConsumer::VerifyEventTraced(AppBarClosedEnd_value, 2);

    }

} } } } } } }// Microsoft::UI::Xaml::Tests::Tools::ETW::InputEvents