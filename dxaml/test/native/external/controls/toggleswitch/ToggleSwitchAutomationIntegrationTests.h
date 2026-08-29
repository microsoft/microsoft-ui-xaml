// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <AutomationClient\AutomationClientManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ToggleSwitch {

    class ToggleSwitchAutomationIntegrationTests : public WEX::TestClass<ToggleSwitchAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ToggleSwitchAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationName)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that the ToggleSwitch Automation Name is correct")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetClickablePoint)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies the clickable point is the center of the ToggleSwitch thumb.")
        END_TEST_METHOD()

    };

} } } } } }
