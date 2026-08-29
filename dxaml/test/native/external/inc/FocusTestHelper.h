// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <TestEvent.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

        class FocusTestHelper
        {
        public:
            template <class Element>
            static void EnsureFocus(_In_ Element^ element, _In_ xaml::FocusState focusState, uint32 Attempts = 1)
            {
                bool gotFocus = false;

                while (Attempts > 0 && !gotFocus)
                {
                    Attempts--;

                    // On desktop, check if there is any other window currently having the window focus
                    // If so, try to bring the test app to foreground before attempting focus on the element
                    if ( !test_infra::TestServices::Utilities->IsOneCore &&
                        !test_infra::TestServices::Utilities->IsXBox)
                    {
                        if (!test_infra::TestServices::WindowHelper->IsFocusedWindow)
                        {
                            LOG_OUTPUT(L"Test app does not have window focus, bring it to foreground and try again!");
                            test_infra::TestServices::WindowHelper->RestoreForegroundWindow();
                            test_infra::TestServices::WindowHelper->WaitForIdle();
                        }
                    }

                    auto gotFocusEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>(L"GotFocus");
                    // gotFocusEvent MUST be declared before gotFocusRegistration
                    // Otherwise the event handler could execute after gotFocusEvent has been destroyed.
                    auto gotFocusRegistration = CreateSafeEventRegistration(Element, GotFocus);

                    Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]
                    {
                        gotFocusRegistration.Attach(element, [&]()
                        {
                            LOG_OUTPUT(L"Element has received focus.");
                            gotFocusEvent->Set();
                        });

                        if (element->FocusState != xaml::FocusState::Unfocused && xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(element))
                        {
                            // The element is already focused
                            LOG_OUTPUT(L"Focus was already set on this element");
                            gotFocusEvent->Set();
                        }
                        else
                        {
                            LOG_OUTPUT(L"Setting focus to the element...");
                            element->Focus(focusState);
                        }
                    });

                    gotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(4000));
                    gotFocus = gotFocusEvent->HasFired();
                }

                if (!gotFocus)
                {
                    test_infra::TestServices::Utilities->CaptureScreen(L"FocusTestHelper");
                }
                VERIFY_IS_TRUE(gotFocus);
            };

            // This version is for use in Island or extra Window scenarios, where WindowHelper can't be used.
            template <class Element>
            static void EnsureFocusInIsland(_In_ Element^ element, _In_ xaml::FocusState focusState, uint32 Attempts = 1)
            {
                bool gotFocus = false;

                while (Attempts > 0 && !gotFocus)
                {
                    Attempts--;

                    auto gotFocusEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>(L"GotFocus");
                    // gotFocusEvent MUST be declared before gotFocusRegistration
                    // Otherwise the event handler could execute after gotFocusEvent has been destroyed.
                    auto gotFocusRegistration = CreateSafeEventRegistration(Element, GotFocus);

                    Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]
                    {
                        gotFocusRegistration.Attach(element, [&]()
                        {
                            LOG_OUTPUT(L"Element has received focus.");
                            gotFocusEvent->Set();
                        });

                        if (element->FocusState != xaml::FocusState::Unfocused && xaml_input::FocusManager::GetFocusedElement(element->XamlRoot)->Equals(element))
                        {
                            // The element is already focused
                            LOG_OUTPUT(L"Focus was already set on this element");
                            gotFocusEvent->Set();
                        }
                        else
                        {
                            LOG_OUTPUT(L"Setting focus to the element...");
                            element->Focus(focusState);
                        }
                    });

                    gotFocusEvent->WaitForNoThrow(std::chrono::milliseconds(4000));
                    gotFocus = gotFocusEvent->HasFired();
                }

                if (!gotFocus)
                {
                    test_infra::TestServices::Utilities->CaptureScreen(L"FocusTestHelper");
                }
                VERIFY_IS_TRUE(gotFocus);
            };
        };

        class FocusMonitor
        {
        public:
            FocusMonitor(_In_ Xaml::UIElement^ element)
                :m_fLostFocus(false)
            {
                RunOnUIThread([&]()
                {
                    m_element = element;
                    m_pLostFocusToken = element->LostFocus += ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                    {
                        LOG_OUTPUT(L"Element has lost focus.");
                        m_fLostFocus = true;
                    });
                });
            }

            ~FocusMonitor()
            {
                RunOnUIThread([&]()
                {
                    m_element->LostFocus -= m_pLostFocusToken;
                });
            }

            bool HasFocusLost() { return m_fLostFocus; }

        private:

            wf::EventRegistrationToken m_pLostFocusToken;
            Xaml::UIElement^ m_element;
            bool m_fLostFocus;
        };

} } } } }
