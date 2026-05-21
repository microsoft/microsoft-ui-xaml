// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Copyright (c) Microsoft Corporation.  All rights reserved.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus {
        class AllowFocusOnInteractionIntegrationTests : public WEX::TestClass<AllowFocusOnInteractionIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(AllowFocusOnInteractionIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(FocusAndSIPDoesNotChangeWhenAllowFocusDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that focus remains the same and that the SIP stays open when focus is attempting to change")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClickFiresButFocusDoesNotChange)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we still fire events although the focus is not on the element")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ControlReceivesAllowFocusOnInteractionThroughInheritance)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when a parent element has AlowFocusOnInteraction false, it propagates to its children.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ControlOverridesAllowFocusEvenWhenInherited)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when a parent element has AlowFocusOnInteraction set to false, but the child has it true, we use the child's value.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanStillSetFocusThroughKeyboardWhenAllowFocusFalse)
                TEST_METHOD_PROPERTY(L"Description", L"When we have set AllowFocusCorrectlyLeavesTree, we can still focus the element when we navigate via Keyboard")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppBarButtonDenyFocusByDefault)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the appbar buttons (AppBarButton/Toggle) sets AllowFocusOnInteraction to false by default")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AllowFocusCorrectlyLeavesTree)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when an element leaves the tree and the candiate element has AllowFocusCorrectlyLeavesTree set to falser, we still focus the element")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MenuFlyoutCorrectlyPropagatesProperty)
                TEST_METHOD_PROPERTY(L"Description", L"A Flyout is special in that it creates an internal class and forwards its values to that class. Verify that AllowFocusOnInteraction is propagated correctly")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppBarButtonBehavesNormallyOnKeyboard)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the appbar buttons behave normally when using keyboard input")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopupLightDismissBehaviorAllowFocusOnInteractionOff)
                TEST_METHOD_PROPERTY(L"Description", L"Verify control with AllowFocusOnInteraction disabled does not have focus set after popup is light dismissed using pointer")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopupLightDismissBehaviorAllowFocusOnInteractionOn)
                TEST_METHOD_PROPERTY(L"Description", L"Verify control with AllowFocusOnInteraction enabled does have focus set after popup is light dismissed using pointer")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

        private:
            void PopupLightDismissBehaviorHelper(bool allowFocusOnInteraction);

        };
    }
}}}}
