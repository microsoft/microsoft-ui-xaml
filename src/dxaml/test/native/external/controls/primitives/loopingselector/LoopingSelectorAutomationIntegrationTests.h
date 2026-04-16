// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <UIAutomation.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Primitives { namespace LoopingSelector {

    class LoopingSelectorAutomationIntegrationTests : public WEX::TestClass<LoopingSelectorAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(LoopingSelectorAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for LoopingSelector.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyPatternsRS1)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that LoopingSelector and LoopingSelectorItem implement the expected UIA patterns in RS1.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUIAClientCanScroll)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the UIA client can scroll the LoopingSelector.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectionIsSeparateFromScrolling)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that scrolling with the UIA client does not select an item in the LoopingSelector.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectionAfterLooping)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that selection is correct after looping through all the items")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyIndexInGroupAfterRecycling)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"Description", L"Verify that index and count are correct after containers get recycled")
        END_TEST_METHOD()

    private:
        void VerifyPatterns(bool expectExpandCollapsePattern);

        static xaml_primitives::LoopingSelector^ SetupLoopingSelectorAutomationTest(int itemCount);
        static xaml_primitives::LoopingSelectorItem^ GetLoopingSelectorItemWithContent(xaml_primitives::LoopingSelector^ loopingSelector, int content);

        static WCHAR s_automationName[];
        static WCHAR s_automationId[];
        static WCHAR s_automationItemStatus[];
        static CONTROLTYPEID s_automationTypeId;

    };

} } } } } } }
