// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        // Covers VisualStateGroup.CurrentStateChanging/CurrentStateChanged as observed by app code, both when
        // the VisualStateManager changes state and when a custom VisualStateManager raises them explicitly.
        class VisualStateGroupEventsTests : public WEX::TestClass<VisualStateGroupEventsTests>
        {
        public:
            BEGIN_TEST_CLASS(VisualStateGroupEventsTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CurrentStateChangingRaisedBeforeChangedOnGoToState)
                TEST_METHOD_PROPERTY(L"Description", L"GoToState raises CurrentStateChanging (then CurrentStateChanged) with the old/new states, the control and the template root as sender.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CurrentStateChangingHandlerNotInvokedAfterRemoval)
                TEST_METHOD_PROPERTY(L"Description", L"After a CurrentStateChanging handler is removed it is not invoked, while CurrentStateChanged still is.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CustomVsmRaiseCurrentStateChangingInvokesGroupHandlers)
                TEST_METHOD_PROPERTY(L"Description", L"VisualStateManager.RaiseCurrentStateChanging invokes the group's handlers once, synchronously, with the supplied arguments.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CustomVsmRaiseCurrentStateChangedInvokesGroupHandlers)
                TEST_METHOD_PROPERTY(L"Description", L"VisualStateManager.RaiseCurrentStateChanged invokes the group's handlers once, synchronously, with the supplied arguments.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CustomVsmRaiseForControlWithoutTemplateIsIgnored)
                TEST_METHOD_PROPERTY(L"Description", L"Raising for a control whose template has not been applied does not invoke handlers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CustomVsmRaiseWithNullNewStateFails)
                TEST_METHOD_PROPERTY(L"Description", L"Raising with a null new state fails with E_INVALIDARG and does not invoke handlers.")
            END_TEST_METHOD()
        };

} } } } }
