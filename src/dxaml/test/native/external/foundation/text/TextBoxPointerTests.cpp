// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxPointerTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "TextBoxGenericTests.h"
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;
using namespace Microsoft::UI::Xaml::Input;
using namespace ::Windows::UI::ViewManagement;
using namespace ::Windows::Foundation;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        bool TextBoxPointerTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool TextBoxPointerTests::ClassCleanup()
        {
            return true;
        }

        bool TextBoxPointerTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool TextBoxPointerTests::TestCleanup()
        {
            TestServices::WindowHelper->WaitForIdle();
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ TextBoxPointerTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\text\\");
        }

        void TextBoxPointerTests::CheckTextBoxPointerOver()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto textBoxPointerEnteredEvent = std::make_shared<Event>();
            auto textBoxPointerEnteredRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, PointerEntered);
            auto textBoxPointerExitedEvent = std::make_shared<Event>();
            auto textBoxPointerExitedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, PointerExited);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxPointerTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                textBoxPointerEnteredRegistration.Attach(
                    textBox,
                    ref new PointerEventHandler(
                    [textBoxPointerEnteredEvent](Platform::Object^, PointerRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Pointer entered textbox.");
                    textBoxPointerEnteredEvent->Set();
                }));
                textBoxPointerExitedRegistration.Attach(
                    textBox,
                    ref new PointerEventHandler(
                    [textBoxPointerExitedEvent](Platform::Object^, PointerRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Pointer exited textbox.");
                    textBoxPointerExitedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Make sure mouse pointer is away from textbox...");
            TestServices::InputHelper->MoveMouse(button);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Move mouse pointer over textbox...");
            TestServices::InputHelper->MoveMouse(textBox);
            textBoxPointerEnteredEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree when textbox has the pointer over...");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            LOG_OUTPUT(L"Move mouse pointer away from textbox...");
            TestServices::InputHelper->MoveMouse(button);
            textBoxPointerExitedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Recording DComp tree when textbox does not have the pointer over...");
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

            TestServices::WindowHelper->WaitForIdle();
        }

         void TextBoxPointerTests::CheckTextBoxFocusInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxPointerTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button control GotFocus handler.");
                    buttonGotFocusEvent->Set();
                }));

                textboxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textboxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                }));
             });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Touch inside the textbox and gradually move out, it should not have the focus.");
            for (INT iMove = 0; iMove <= 100; iMove++)
            {
                TestServices::InputHelper->DynamicPressCenter(textBox, 0, iMove, PointerFinger::Finger1);
            }
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);

            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_FALSE(textboxGotFocusEvent->HasFired()); // make sure textbox does not have the focus

            LOG_OUTPUT(L"Touch outside of the textbox and gradually move in, it should have the focus.");
            for (INT iMove = 100; iMove >= 0; iMove--)
            {
                TestServices::InputHelper->DynamicPressCenter(textBox, 0, iMove, PointerFinger::Finger1);
            }
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);

            textboxGotFocusEvent->WaitForDefault(); //should have the focus now
            textboxGotFocusEvent->Reset();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });
            buttonGotFocusEvent->WaitForDefault();

            {
                TestServices::WindowHelper->WaitForIdle();

                LOG_OUTPUT(L"Press mouse button inside the textbox and gradually move out, it should have the focus.");
                TestServices::InputHelper->DragFromCenter(textBox, 0 /*relX*/, -60 /*relY*/, 0.1 /*velocityFactor*/);
                TestServices::WindowHelper->WaitForIdle();
                textboxGotFocusEvent->WaitForDefault();
                textboxGotFocusEvent->Reset();

                // tap button to remove focus on textbox
                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                    button->Focus(FocusState::Pointer);
                });
                buttonGotFocusEvent->WaitForDefault();
                TestServices::WindowHelper->WaitForIdle();

                // Reverse the action, drag from outside to inside element
                LOG_OUTPUT(L"Press mouse button outside of the textbox and gradually move in, it should not have the focus.");
                TestServices::InputHelper->DragToCenter(textBox, 0 /*relX*/, -60 /*relY*/, 0.1 /*velocityFactor*/);
                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_FALSE(textboxGotFocusEvent->HasFired());
            }

            RunOnUIThread([&]()
            {
                // Prevent the text control from getting focus again so input pane remains hidden.
                textBox->IsEnabled = false;
            });

            TestServices::WindowHelper->WaitForIdle();

        }

        void TextBoxPointerTests::CheckTextBoxFocusWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            CheckTextBoxFocusInternal();
        }

        void TextBoxPointerTests::CheckGamepadInputOnDesktop()
        {
            TestCleanupWrapper cleanup;

            InputDevice device = InputDevice::Gamepad;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxPointerTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                }));

                textboxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textboxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button");
                button->Focus(FocusState::Pointer);
            });
            buttonGotFocusEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            // Move focus to the textbox
            CommonInputHelper::Down(device);

            //SIP should not raise so no cleanup needed. This used to crash, only verifying that it doesn't
            CommonInputHelper::Accept(device);
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxPointerTests::PreventKeyboardDisplayOnProgrammaticFocus()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            InputPane^ inputPane;
            int inputPaneAnimationTime = 1000; // Input Pane takes about 1 sec to show/hide.
            ::Windows::Foundation::EventRegistrationToken inputPaneShowToken1;
            ::Windows::Foundation::EventRegistrationToken inputPaneShowToken2;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto SIPShowingEvent = std::make_shared<Event>();
            auto SIPHidingEvent = std::make_shared<Event>();

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxPointerTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);

                textBox->PreventKeyboardDisplayOnProgrammaticFocus = true;
                textBox->Height = 600;

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button control GotFocus handler.");
                    buttonGotFocusEvent->Set();
                }));

                textboxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textboxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                }));
             });

            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Focus the Button to discard the input pane.");
                TestServices::InputHelper->Tap(button);
            });
            buttonGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                inputPane = InputPane::GetForCurrentView();
                inputPaneShowToken1 = inputPane->Showing += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                   SIPShowingEvent->Set();
                });
                inputPaneShowToken2 = inputPane->Hiding += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                   SIPHidingEvent->Set();
                });
            });
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"1.Set Focus programmaticaly on the TextBox, the SIP should not pop up");
                textBox->Focus(FocusState::Programmatic);

            });

            // The SIP should not pop up
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            VERIFY_IS_FALSE(SIPShowingEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"2.Tap the TextBox, the SIP should pop up.");
                TestServices::InputHelper->Tap(textBox);
            });

            //The SIP should pop up
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            VERIFY_IS_TRUE(SIPShowingEvent->HasFired());
            SIPShowingEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"3.Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });

            //The SIP should now close
            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->WaitForDefault();
            SIPHidingEvent->WaitForDefault();
            SIPHidingEvent->Reset();
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            SIPShowingEvent->Reset();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            // Set Programmatic focus again
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"4.Set Focus programmaticaly on the TextBox, the SIP should not pop up");
                textBox->Focus(FocusState::Programmatic);
            });

            // The SIP should not pop up
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            VERIFY_IS_FALSE(SIPShowingEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"5.Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });

            //The SIP should now remain close
            buttonGotFocusEvent->WaitForDefault();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            // Set Programmatic focus again
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"6.Set Focus programmaticaly on the TextBox, the SIP should not pop up");
                textBox->Focus(FocusState::Programmatic);
            });

            // The SIP should not pop up
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            VERIFY_IS_FALSE(SIPShowingEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            // Tap on textbox, SIP should popup
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"7.Tap the TextBox, the SIP should pop up.");
                TestServices::InputHelper->Tap(textBox);
            });

            //The SIP should pop up
            SIPShowingEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"8.Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });
            //The SIP should now close
            TestServices::WindowHelper->WaitForIdle();
            buttonGotFocusEvent->WaitForDefault();
            SIPHidingEvent->WaitForDefault();
            SIPHidingEvent->Reset();
            SIPShowingEvent->WaitForNoThrow(std::chrono::milliseconds(inputPaneAnimationTime));
            SIPShowingEvent->Reset();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            // Tap on textbox, SIP should popup
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"9.Tap the TextBox, the SIP should pop up.");
                TestServices::InputHelper->Tap(textBox);
            });

            //The SIP should pop up
            SIPShowingEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // tap button to remove focus on textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"10.Focus the Button to discard the input pane.");
                button->Focus(FocusState::Pointer);
            });
            buttonGotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                // Prevent the text control from getting focus again so input pane remains hidden.
                textBox->IsEnabled = false;
                inputPane->Showing -= inputPaneShowToken1;
                inputPane->Hiding -= inputPaneShowToken2;
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxPointerTests::PreventEditFocusLoss()
        {
            wf::EventRegistrationToken inputPaneHidingToken = {};
            TestCleanupWrapper cleanup([&inputPaneHidingToken]()
            {
                RunOnUIThread([&inputPaneHidingToken]()
                {
                    InputPane::GetForCurrentView()->Hiding -= inputPaneHidingToken;
                    inputPaneHidingToken = {};
                });
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            InputPane^ inputPane;

            auto buttonGotFocusEvent = std::make_shared<Event>();
            auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
            auto textboxGotFocusEvent = std::make_shared<Event>();
            auto textboxGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
            auto SIPHidingEvent = std::make_shared<Event>();

            xaml_controls::TextBox^ textBox = nullptr;
            xaml_controls::Button^ button = nullptr;
            xaml_controls::ITextBoxPriv2^ textBoxPriv = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"TextBoxPointerTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                textBox = safe_cast<xaml_controls::TextBox^>(rootStackPanel->FindName("textBox"));
                VERIFY_IS_NOT_NULL(textBox);
                button = safe_cast<xaml_controls::Button^>(rootStackPanel->FindName("button"));
                VERIFY_IS_NOT_NULL(button);


                textBox->Height = 100;
                textBoxPriv = dynamic_cast<xaml_controls::ITextBoxPriv2^>(textBox);
                textBoxPriv->PreventEditFocusLoss = true;

                buttonGotFocusRegistration.Attach(
                    button,
                    ref new xaml::RoutedEventHandler(
                    [buttonGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button GotFocus.");
                    buttonGotFocusEvent->Set();
                }));

                textboxGotFocusRegistration.Attach(
                    textBox,
                    ref new xaml::RoutedEventHandler(
                    [textboxGotFocusEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Textbox GotFocus.");
                    textboxGotFocusEvent->Set();
                }));

                inputPane = InputPane::GetForCurrentView();
                inputPaneHidingToken = inputPane->Hiding += ref new TypedEventHandler<InputPane^, InputPaneVisibilityEventArgs^>([&](InputPane^ pane, InputPaneVisibilityEventArgs^ e)
                {
                    SIPHidingEvent->Set();
                });
             });

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);
            TestServices::WindowHelper->WaitForIdle();

            // Move focus to the textbox
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"1.Set Focus programmaticaly on the TextBox");
                textBox->Focus(FocusState::Programmatic);

            });
            TestServices::WindowHelper->WaitForIdle();

            // inject GamePad key A to enable the PreventEditFocusLoss private API
            CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"2.Set Focus programmaticaly on the Button, SIP should not dismiss");
                button->Focus(FocusState::Programmatic);

            });

            SIPHidingEvent->WaitForNoThrow(std::chrono::milliseconds(2000));
            VERIFY_IS_FALSE(SIPHidingEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"3.Set Focus programmaticaly on the TextBox");
                textBox->Focus(FocusState::Programmatic);

            });

            TestServices::WindowHelper->WaitForIdle();

            //remove edit focus and reset state
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"4.Dismiss SIP");
                textBoxPriv->PreventEditFocusLoss = false;
                textBoxPriv->ForceEditFocusLoss();

            });
            LOG_OUTPUT(L"Wait for SIP hiding event.");
            SIPHidingEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        void TextBoxPointerTests::CheckFiresManipulationEvents()
        {
            TestCleanupWrapper cleanup;
            TextBoxGenericTests<xaml_controls::TextBox>::CanFireManipulationEvents();
        }

    } }
} } } }
