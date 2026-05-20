// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Test {
        class InfrastructureIntegrationTests : public WEX::TestClass<InfrastructureIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(InfrastructureIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"478e7719-b8f0-434b-b29a-e7911a52ae17;e63becda-2a21-4186-9066-a5c3d52f5603")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateWindowContentAccessor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the WindowHelper getter/setters operate as expected.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateWaitForIdle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that wait for idle successfully waits.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateSetupSimulatedAppPage)
                TEST_METHOD_PROPERTY(L"Description", L"Validates setting up a simulated app page creates the correct structure.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTap)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the end-to-end Tap infrastructure is functional.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateFlick)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the end-to-end Flick infrastructure is functional.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateTab)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the end-to-end keyboard tab infrastructure is functional.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePressKeySequence)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the end-to-end keyboard press key sequence infrastructure is functional.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()
        };
    }
} } } }
