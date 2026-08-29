// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarToggleButton {

    class AppBarToggleButtonAutomationIntegrationTests : public WEX::TestClass<AppBarToggleButtonAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(AppBarToggleButtonAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"b914a3e3-0d78-4274-88b9-46a4773bb21b")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Automationproperties for AppBarToggleButton.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyHasNoUIAControlChildren)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that AppBarToggleButton does not report any UIA children that are control elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyToggleExecutesCommand)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that toggling AppBarToggleButton causes the attached Command to fire.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()
    };

} } } } } }
