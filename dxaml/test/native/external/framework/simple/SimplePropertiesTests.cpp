// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "SimplePropertiesTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <Utils.h>
#include <SafeEventRegistration.h>
#include <TestEvent.h>
#include <WindowsNumerics.h>
#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace WEX::Logging;
using namespace WEX::Common;

namespace wfn = ::Windows::Foundation::Numerics;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        auto s_noThrowTimeout = std::chrono::milliseconds(500);

        bool SimplePropertiesTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool SimplePropertiesTests::ClassCleanup()
        {
            return true;
        }

        bool SimplePropertiesTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool SimplePropertiesTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)

// TODO:  Dynamically enable velocity instead of skipping test
#define VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS if (!Feature_XamlMotionSystemHoldbacks::IsEnabled()) { LOG_OUTPUT(L"Motions holdbacks velocity feature disabled, skipping test"); return; }

        void SimplePropertiesTests::BasicFunctionality()
        {
            VELOCITY_TESTGUARD_MOTIONS_HOLDBACKS

            TestCleanupWrapper cleanup;

            auto changedRegistration1 = CreateSafeEventRegistration(Canvas, TranslationChanged);
            auto changedRegistration2 = CreateSafeEventRegistration(Canvas, TranslationChanged);

            Canvas^ canvas1;
            auto changedEvent1 = std::make_shared<Event>();

            Canvas^ canvas2;
            auto changedEvent2 = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                canvas1 = ref new Canvas();
                canvas2 = ref new Canvas();

                changedRegistration1.Attach(
                    canvas1,
                    ref new wf::TypedEventHandler<UIElement^, Platform::Object^>([&](UIElement^ sender, Platform::Object^ args)
                {
                    changedEvent1->Set();
                    VERIFY_ARE_EQUAL(safe_cast<Canvas^>(sender), canvas1);
                    VERIFY_IS_NULL(args);
                }));

                changedRegistration2.Attach(
                    canvas2,
                    ref new wf::TypedEventHandler<UIElement^, Platform::Object^>([&](UIElement^ sender, Platform::Object^ args)
                {
                    changedEvent2->Set();
                    VERIFY_ARE_EQUAL(safe_cast<Canvas^>(sender), canvas2);
                    VERIFY_IS_NULL(args);
                }));

                // setting to default value should have no effect
                canvas1->Translation = {0, 0, 0};
            });

            // no change
            changedEvent1->WaitForNoThrow(s_noThrowTimeout);
            changedEvent2->WaitForNoThrow(s_noThrowTimeout);

            RunOnUIThread([&]()
            {
                // set canvas1 to non-default
                canvas1->Translation = {100, 200, 300};
            });

            // only canvas1 fires
            changedEvent1->WaitForDefault();
            changedEvent2->WaitForNoThrow(s_noThrowTimeout);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(wfn_::float3(100, 200, 300), canvas1->Translation);
                // now set canvas2
                canvas2->Translation = {300, 200, 100};
            });

            // only canvas2 fires
            changedEvent1->WaitForNoThrow(s_noThrowTimeout);
            changedEvent2->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(wfn_::float3(300, 200, 100), canvas2->Translation);
                // set canvas1 to the same value as current
                canvas1->Translation = {100, 200, 300};
            });

            // no change
            changedEvent1->WaitForNoThrow(s_noThrowTimeout);
            changedEvent2->WaitForNoThrow(s_noThrowTimeout);

            RunOnUIThread([&]()
            {
                // set to default, but from a different value
                canvas1->Translation = {0, 0, 0};
            });

            // canvas1 changed
            changedEvent1->WaitForDefault();
            changedEvent2->WaitForNoThrow(s_noThrowTimeout);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(wfn_::float3(0, 0, 0), canvas1->Translation);

                // remove handler for canvas1
                changedRegistration1.Detach();
            });

            RunOnUIThread([&]()
            {
                canvas1->Translation = {10, 0, 0};
                canvas2->Translation = {10, 10, 0};
            });

            // canvas1 does not raise, 2 does
            changedEvent1->WaitForNoThrow(s_noThrowTimeout);
            changedEvent2->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(wfn_::float3(10, 0, 0), canvas1->Translation);
                VERIFY_ARE_EQUAL(wfn_::float3(10, 10, 0), canvas2->Translation);
            });
        }
#endif
    }
} } } }
