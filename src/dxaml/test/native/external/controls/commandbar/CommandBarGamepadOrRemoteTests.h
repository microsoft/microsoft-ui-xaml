// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Controls { namespace CommandBar {

        class CommandBarGamepadOrRemoteTests : public WEX::TestClass<CommandBarGamepadOrRemoteTests>
        {
        public:
            BEGIN_TEST_CLASS(CommandBarGamepadOrRemoteTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SimpleNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies simple focus transition works within elements in a commandbar using gamepad.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_CLASS_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_RS5) // RS4 test failure: CommandBarGamepadOrRemoteTests::SimpleNavigation
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusTrapInOverflow_CommandBarInTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the only way to leave the overflow region of an inline commandbar using gamepad is via the expand button, or by making a selection.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusTrapInOverflow_BottomCommandBar)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the only way to leave the overflow region of a bottom commandbar using gamepad is via the expand button, or by making a selection.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusNotTrappedInOverflow_ExpandButton)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that after entering the a commandbar overflow it is possible to horizontally navigate from the expand button to the primary commandbar items using gamepad.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusTrapInCommandBarWhenOverflowIsOpen)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that you can't navigate out the command bar when the overflow is open by attempting to navigate left of the left most primary commandbar item with gamepad.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExitOverflow_BottomCommandBar)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that once you enter a bottom commandbar overflow, you can exit it by pressing Gamepad B and focus is restored to the Expand Button.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ExitOverflow_SelectingItemInOverflow)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that selecting an item in overflow with the gamepad sets the focus on to the Expand Button.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotAllowFocusWrapInOverflow_TopCommandBar)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Gamepad doesn't allow the focus wrapping in overflow of a Top CommandBar.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotAllowFocusWrapInOverflow_BottomCommandBar)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Gamepad doesn't allow the focus wrapping in overflow of a Bottom CommandBar.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            void DoNotAllowFocusWrapInOverflow(bool isTopCommandBar);

            Microsoft::UI::Xaml::UIElement^ SetupTest(Platform::String^ xamlFile);
            inline Platform::String^ GetResourcesPath() const;
        };

    } }
} } } }
