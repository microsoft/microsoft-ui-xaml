// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <UIAutomation.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MenuFlyout {

    class MenuFlyoutAutomationIntegrationTests : public WEX::TestClass<MenuFlyoutAutomationIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(MenuFlyoutAutomationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test") //DCPP: Crash in Microsoft.UI.Input.dll!UIAutomationIslandForwarder::EnsureAutomationHostProvider(HWND__ * hwnd)
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyAutomationProperties)
            TEST_METHOD_PROPERTY(L"Description", L"Verify AutomationProperties for MenuFlyout.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutMenuSubItemAutomation)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Automation of MenuFlyout Sub Item.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFlyoutMenuSplitItemAutomation)
            TEST_METHOD_PROPERTY(L"Description", L"Verify Automation of MenuFlyout Split Item.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOverridingAutomationNameForSplitMenuItem)
            TEST_METHOD_PROPERTY(L"Description", L"Verify overriding automation name for split menu item overrides the automation name for primary and secondary buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySplitMenuItemAutomationTreeStructure)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that SplitMenuFlyoutItem automation tree has secondary button as next sibling with correct position and size of set.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyItemIsNotReadWhenCollapsed)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that MenuFlyout items are read when visible, and not when they are collapsed.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesSizeOfSetAndPositionInSetExcludeSeparatorsAndCollapsedItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that MenuFlyoutSeparators are excluded when determing position in set and size of set.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyScrollItemPattern)
            TEST_METHOD_PROPERTY(L"Description", L"Verify ScrollItemPattern on MenuFlyoutItem.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySizeAndPositionInSetCanBeOverridden)
            TEST_METHOD_PROPERTY(L"Description", L"Verify that the app author can set the PositionInSet and SizeOfSet AutomationProperties to override the default values.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

    private:
        static WCHAR s_automationName[];
        static WCHAR s_automationId[];
        static WCHAR s_automationItemStatus[];
        static CONTROLTYPEID s_automationTypeId;

        const float DefaultHorizontalOffset{ -50 };
        const float DefaultVerticalOffset{ 50 };
    };

} } } } } }
