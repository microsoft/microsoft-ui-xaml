// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {
        class SetContactTest : public WEX::TestClass<SetContactTest>
        {
        public:
            BEGIN_TEST_CLASS(SetContactTest)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"aa6364d2-41fe-4bec-a849-584f1f309baf;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Basics)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the most basic scenario of manually starting a pan manipulation")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Validation1)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TryStartDM fails if used for mouse input")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EdgeCase1)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that TryStartDM returns false if ManipulationMode doesn't contain System")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EdgeCase2)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that both the app and XAML can try to start a manipulation and the manipulation still happens without error")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EdgeCase3)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that calling TryStartDM multiple times manipulates correctly and gracefully no-ops")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EdgeCase4)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that removing a UIelement from the tree mid-manipulation does not crash")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;

            xaml::FrameworkElement^ SetupUI(
                _In_ Platform::String^ filename
                );

        };


    } } }
} } } }
