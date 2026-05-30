// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class HubTest : public WEX::TestClass<HubTest>
        {
        public:
            BEGIN_TEST_CLASS(HubTest)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_CLASS_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // DCPP: RS4 test failure: DManip::HubTest::Basics fails due to crash in TraceConsumerSession
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Basics)
                TEST_METHOD_PROPERTY(L"Description", L"Validates Hub doesn't apply any DManip transforms to secondary content relationship Dependency Properties")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Intermittent test crash
            END_TEST_METHOD()

        private:
            xaml_controls::Hub^ SetupUI();

        };


    } } }
} } } }
