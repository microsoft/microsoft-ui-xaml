// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace CommandBar {

    class CommandBarAutomationIntegrationTests : public WEX::TestClass<CommandBarAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(CommandBarAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for CommandBar.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAutomationWindowPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that a CommandBar implements the Window UIA pattern when CommandBar is opened.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPositionAndSize)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that AppBarButtons and AppBarToggleButtons in a CommandBar report correct values for PositionInSet and SizeOfSet.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyNonTabStopAppBarButtonsAreStillKeyboardFocusable)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that AppBarButtons with IsTabStop set to false still show up as keyboard focusable.")
        END_TEST_METHOD()

    private:
        Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;

        static WCHAR s_automationName[];
        static WCHAR s_automationId[];
    };

} } } } } }
