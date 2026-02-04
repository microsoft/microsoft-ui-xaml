// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ModeContainer.h"

#include "InputInterceptor.h"
#include "InputInterceptorUnitTests.h"
#include <CxxMockTaef.h>
#include "Mocks.h"

using namespace CxxMock;
using namespace AccessKeys;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

        MOCK_CLASS(MockModeContainer, Base)
            STUB_METHOD(HRESULT, EvaluateAccessKeyMode, 2(const InputMessage*, bool*))
            STUB_METHOD(bool, GetIsActive, 0)
            STUB_METHOD(bool, ShouldForciblyExitAKMode, 0)
        END_MOCK

        MOCK_CLASS(MockScopeTree, Base)
            STUB_METHOD(HRESULT, ProcessCharacter, 2(const wchar_t, bool*))
            STUB_METHOD(HRESULT, TryExitScope, 1(bool*))
        END_MOCK

        MOCK_CLASS(MockTreeAnalyzer, Base)
            STUB_METHOD(HRESULT, DoesTreeContainAKElement, 1(bool&))
        END_MOCK

        void InputInterceptorUnitTests::VerifyHandledWithCorrectMessage()
        {
            //In order for ModeContainer to consider AccessKey Mode enabled, an Alt key would need to be pressed (Keydown)
            InputMessage message = { };
            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Menu;

            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .Once()
                .With(eq<wchar_t>(static_cast<wchar_t>(message.m_platformKeyCode)))
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);

            MockModeContainer mockModeContainer;
            BeginOrderedCalls();
                Expect(mockModeContainer, EvaluateAccessKeyMode)
                    .Once()
                    .With(eq<InputMessage*>(&message))
                    .SetOutValue<1>(true)
                    .ReturnValue(S_OK);
            EndOrderedCalls();
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(false);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            bool handled = false;
            VERIFY_SUCCEEDED_WITHMOCKS(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_TRUE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::VerifyNotHandledWithIncorrectMessage()
        {
            MockScopeTree mockTree;

            InputMessage message = { };

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .With(eq<InputMessage*>(&message))
                .SetOutValue<1>(false)
                .ReturnValue(S_OK);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(false);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            bool handled = true;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_FALSE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::AccessKeyNotHandledWhenAltPressedTwice()
        {

            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .Twice()
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);

            MockModeContainer mockModeContainer;

            bool handled = false;

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            InputMessage message1 = {};
            message1.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Menu;

            {
                MockModeContainer tempModeContainer;
                mockModeContainer = tempModeContainer;

                Expect(mockModeContainer, EvaluateAccessKeyMode)
                    .With(eq<InputMessage*>(&message1))
                    .SetOutValue<1>(true)
                    .ReturnValue(S_OK);
                Expect(mockModeContainer, GetIsActive)
                    .ReturnValue(true);

                Expect(mockModeContainer, ShouldForciblyExitAKMode)
                    .ReturnValue(false);

                VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message1, &handled));

                VERIFY_IS_TRUE(handled);
                VERIFY_EXPECTATIONS(mockModeContainer);
            }

            InputMessage message2 = message1;
            message2.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Menu;

            {
                MockModeContainer tempModeContainer;
                mockModeContainer = tempModeContainer;

                Expect(mockModeContainer, EvaluateAccessKeyMode)
                    .With(eq<InputMessage*>(&message2))
                    .SetOutValue<1>(true)
                    .ReturnValue(S_OK);
                Expect(mockModeContainer, GetIsActive)
                    .ReturnValue(true);

                Expect(mockModeContainer, ShouldForciblyExitAKMode)
                    .ReturnValue(false);

                VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message2, &handled));

                VERIFY_IS_TRUE(handled);
                VERIFY_EXPECTATIONS(mockModeContainer);
            }

            //AK mode should now be disabled
            InputMessage message3 = message2;
            message3.m_platformKeyCode = wsy::VirtualKey::VirtualKey_A;

            {
                MockModeContainer tempModeContainer;
                mockModeContainer = tempModeContainer;

                Expect(mockModeContainer, EvaluateAccessKeyMode)
                    .With(eq<InputMessage*>(&message3))
                    .SetOutValue<1>(false)
                    .ReturnValue(S_OK);
                Expect(mockModeContainer, GetIsActive)
                    .ReturnValue(false);

                Expect(mockModeContainer, ShouldForciblyExitAKMode)
                    .ReturnValue(false);

                VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message3, &handled));

                VERIFY_IS_FALSE(handled);
                VERIFY_EXPECTATIONS(mockModeContainer);
            }

            VERIFY_EXPECTATIONS(mockTree);

        }

        void InputInterceptorUnitTests::InputIsAcceptedWhenInAKMode()
        {
            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .Once()
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(false);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            InputMessage message = { };

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_A;

            bool handled = false;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message,&handled));

            VERIFY_IS_TRUE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::VerifyPressingRightAltDoesNotEnterAKMode()
        {
            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .ReturnValue(S_OK)
                .CallCount(0);

            InputMessage message = { };

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Menu;

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .With(eq<InputMessage*>(&message))
                .SetOutValue<1>(false)
                .ReturnValue(S_OK);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(false)
                .ReturnValue(false);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            bool handled = true;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_FALSE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::MultipleInputIsAcceptedWhenInAKMode()
        {
            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .CallCount(eq(3)) // Should be replaced with 3 calls to .With(eq<wchar_t>( ))
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(true);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            InputMessage message = { };

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_A;

            bool handled = false;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_TRUE(handled);

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_B;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_TRUE(handled);

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_C;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &handled));

            VERIFY_IS_TRUE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::ExceptionalResultIsReported()
        {
            MockScopeTree mockTree;

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .Once()
                .SetOutValue<1>(true)
                .ReturnValue(E_NOTIMPL);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(true);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            InputMessage message = { };

            bool handled = true;
            HRESULT hr = S_OK;
            hr = akInputInterceptor.TryProcessInputForAccessKey(&message, &handled);
            VERIFY_ARE_EQUAL(E_NOTIMPL, hr);
            VERIFY_IS_FALSE(handled);
            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }

        void InputInterceptorUnitTests::EscapeKeyNotHandled()
        {
            // Intent of this test is to check the situation where an escape key is initially received by this component.  In this case
            // we do not handle the escape key and allow the UI to do the handling.  (If the UI does not handle the key, we then invoke additional
            // Logic - an aspect not tested with this unit test).
            MockScopeTree mockTree;
            Expect(mockTree, ProcessCharacter)
                .ReturnValue(S_OK)
                .Once();

            MockModeContainer mockModeContainer;
            Expect(mockModeContainer, EvaluateAccessKeyMode)
                .SetOutValue<1>(true)
                .ReturnValue(S_OK);
            Expect(mockModeContainer, GetIsActive)
                .ReturnValue(true);

            Expect(mockModeContainer, ShouldForciblyExitAKMode)
                .ReturnValue(false);

            MockTreeAnalyzer treeAnalyzer;
            Expect(treeAnalyzer, DoesTreeContainAKElement)
                .SetOutValue<0>(true)
                .ReturnValue(S_OK);

            AKInputInterceptor<MockModeContainer, MockScopeTree, MockTreeAnalyzer> akInputInterceptor(mockModeContainer, mockTree, treeAnalyzer);

            InputMessage message;

            message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Escape;

            bool normalCaseHandled = false;
            VERIFY_SUCCEEDED(akInputInterceptor.TryProcessInputForAccessKey(&message, &normalCaseHandled));
            VERIFY_IS_FALSE(normalCaseHandled);

            VERIFY_EXPECTATIONS(mockTree);
            VERIFY_EXPECTATIONS(mockModeContainer);
        }
    }
}}}}
