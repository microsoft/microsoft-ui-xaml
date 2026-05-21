// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowLoseFocusTest.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <SafeEventRegistration.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Foundation {
                    namespace Input {
                        namespace Focus {


                            Platform::String^ WindowLoseFocusTest::GetResourcesPath() const
                            {
                                return GetPackageFolder() + L"resources\\native\\foundation\\input\\focus\\";
                            }

                            bool WindowLoseFocusTest::ClassSetup()
                            {
                                CommonTestSetupHelper::CommonTestClassSetup();
                                return true;
                            }

                            bool WindowLoseFocusTest::TestCleanup()
                            {
                                TestServices::WindowHelper->VerifyTestCleanup();
                                return true;
                            }

                            bool IsSameColor(Color expected, Color actual)
                            {
                                return (expected.R == actual.R
                                    && expected.G == actual.G
                                    && expected.B == actual.B
                                    && expected.A == actual.A);
                            }

                            //------------------------------------------------------------------------
                            // Test case: Puts focus on button, then creates second window, focuses second window,
                            //            removing focus from button, checks, returns focus to primary window,
                            //            restores focus visuals on button, checks.
                            //------------------------------------------------------------------------
                            void WindowLoseFocusTest::LoseFocusVisualsOnControlWhenWindowLosesFocus()
                            {
                                TestCleanupWrapper cleanup;

                                auto gotFocusEvent = std::make_shared<Event>();
                                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

                                auto windowActivatedEvent = std::make_shared<Event>();
                                auto windowActivatedRegistration = CreateSafeEventRegistration(Window, Activated);

                                RunOnUIThread([&]()
                                {
                                    TestServices::WindowHelper->WindowContent = nullptr;
                                });

                                Panel^ root = safe_cast<Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"WindowLoseFocusTest.xaml"));
                                xaml_controls::Button^ Button1;
                                bool isFocused = false;

                                RunOnUIThread([&]()
                                {
                                    Button1 = safe_cast<xaml_controls::Button^>(root->FindName(L"Button1"));
                                    TestServices::WindowHelper->WindowContent = root;

                                    windowActivatedRegistration.Attach(Window::Current, [&]()
                                    {
                                        LOG_OUTPUT(L"Window activated");
                                        windowActivatedEvent->Set();
                                    });

                                    gotFocusRegistration.Attach(Button1, [&]()
                                    {
                                        LOG_OUTPUT(L"btn got focus");
                                        gotFocusEvent->Set();
                                    });
                                });

                                //Puts Keyboard focus onto the button.
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::KeyboardHelper->Tab();
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);
                                TestServices::WindowHelper->WaitForIdle();

                                //Checks that the chrome focus and VSM focus visual changes are there.
                                RunOnUIThread([&]()
                                {
                                    VERIFY_ARE_EQUAL(Button1->FocusState, FocusState::Keyboard);
                                    Grid^ RootGrid = safe_cast<Grid^>(xaml_media::VisualTreeHelper::GetChild(Button1, 0));
                                    auto brush = safe_cast<xaml_media::SolidColorBrush^>(RootGrid->Background);
                                    VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Green));
                                    VERIFY_IS_TRUE(isFocused);
                                });

                                //Sends an windows key input, causing the window to lose focus.
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::KeyboardHelper->PressKeySequence("$d$_lwin#$u$_lwin");
                                TestServices::WindowHelper->WaitForIdle();

                                windowActivatedEvent->WaitForDefault();
                                windowActivatedEvent->Reset();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);

                                //Verifies that the chrome focus and VSM focus visuals have been removed.
                                RunOnUIThread([&]()
                                {
                                    Grid^ RootGrid = safe_cast<Grid^>(xaml_media::VisualTreeHelper::GetChild(Button1, 0));
                                    auto brush = safe_cast<xaml_media::SolidColorBrush^>(RootGrid->Background);
                                    VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Orange));
                                    VERIFY_IS_FALSE(isFocused);
                                });

                                //Refocuses primary screen
                                TestServices::WindowHelper->WaitForIdle();
                                windowActivatedEvent->Reset();
                                gotFocusEvent->Reset();

                                Sleep(1000);
                                //There is a difference between how the XBox and Desktop shells bring back the background app back into foreground from Home
                                //On Desktop, we follow the sequence: (FGApp) LWin -> Start -> (Start) Escape -> FGApp
                                //On XBox, we follow the sequence: (FGApp) LWin -> Home -> (Home) Enter -> FGApp
                                if (TestServices::Utilities->IsXBox)
                                {
                                    TestServices::KeyboardHelper->Enter();
                                }
                                else
                                {
                                    TestServices::KeyboardHelper->Escape();
                                }
                                TestServices::WindowHelper->WaitForIdle();
                                windowActivatedEvent->WaitForDefault();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);
                                gotFocusEvent->WaitForDefault();

                                //Checks that the button has regained focus and is displaying all appropriate visuals.
                                RunOnUIThread([&]()
                                {
                                    VERIFY_ARE_EQUAL(Button1->FocusState, FocusState::Keyboard);
                                    Grid^ RootGrid = safe_cast<Grid^>(xaml_media::VisualTreeHelper::GetChild(Button1, 0));
                                    auto brush = safe_cast<xaml_media::SolidColorBrush^>(RootGrid->Background);
                                    VERIFY_IS_TRUE(IsSameColor(brush->Color, Microsoft::UI::Colors::Green));
                                    VERIFY_IS_TRUE(isFocused);
                                });

                                TestServices::WindowHelper->WaitForIdle();
                            }

                            void WindowLoseFocusTest::LoseFocusVisualsOnHyperlinkWhenWindowLosesFocus()
                            {
                                TestCleanupWrapper cleanup;
                                Hyperlink^ hyperlink1 = nullptr;
                                StackPanel^ root = nullptr;
                                bool isFocused = false;

                                auto lostFocusEvent = std::make_shared<Event>();
                                auto lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, LostFocus);

                                auto gotFocusEvent = std::make_shared<Event>();
                                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

                                auto windowActivatedEvent = std::make_shared<Event>();
                                auto windowActivatedRegistration = CreateSafeEventRegistration(Window, Activated);

                                RunOnUIThread([&]()
                                {
                                    root = safe_cast<StackPanel^>(xaml_markup::XamlReader::Load(
                                        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                                        L"    <TextBlock x:Name='txBlk' Margin='20,20,0,0' Width='350' HorizontalAlignment='Left' FontSize='20'>"
                                        L"        <Hyperlink x:Name='Hyperlink1' NavigateUri = 'http://www.bing.com' >Bing</Hyperlink>"
                                        L"    </TextBlock>"
                                        L"</StackPanel>"));

                                    TestServices::WindowHelper->WindowContent = root;
                                    hyperlink1 = safe_cast<Hyperlink^>(root->FindName(L"Hyperlink1"));

                                    windowActivatedRegistration.Attach(Window::Current, [&]()
                                    {
                                        LOG_OUTPUT(L"Window activated");
                                        windowActivatedEvent->Set();
                                    });
                                });

                                //Puts Keyboard focus onto the hyperlink.
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::KeyboardHelper->Tab();
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);

                                //Checks that the hyperlink has received focus.
                                RunOnUIThread([&]()
                                {
                                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink1));
                                    VERIFY_IS_TRUE(isFocused);
                                });

                                //Sends an window key input, causing the window to lose focus.
                                TestServices::WindowHelper->WaitForIdle();
                                TestServices::KeyboardHelper->PressKeySequence("$d$_lwin#$u$_lwin");
                                TestServices::WindowHelper->WaitForIdle();

                                windowActivatedEvent->WaitForDefault();
                                windowActivatedEvent->Reset();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);

                                //Checks that the hyperlink has lost focus.
                                RunOnUIThread([&]()
                                {
                                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink1));
                                    VERIFY_IS_FALSE(isFocused);
                                });

                                //Refocuses primary window
                                TestServices::WindowHelper->WaitForIdle();

                                //When the window is about to come back, we need to wait for it to send activtions/etc. There is currently no
                                //good way to do this, so we've defaulted to just sleeping for a second.
                                Sleep(1000);
                                //There is a difference between how the XBox and Desktop shells bring back the background app back into foreground from Home
                                //On Desktop, we follow the sequence: (FGApp) LWin -> Start -> (Start) Escape -> FGApp
                                //On XBox, we follow the sequence: (FGApp) LWin -> Home -> (Home) Enter -> FGApp
                                if (TestServices::Utilities->IsXBox)
                                {
                                    TestServices::KeyboardHelper->Enter();
                                }
                                else
                                {
                                    TestServices::KeyboardHelper->Escape();
                                }
                                TestServices::WindowHelper->WaitForIdle();
                                windowActivatedEvent->WaitForDefault();
                                TestServices::Utilities->IsWindowFocused(&isFocused, TestServices::WindowHelper->WindowContent->XamlRoot);

                                //Checks that the hyperlink has received focus once more.
                                RunOnUIThread([&]()
                                {
                                    VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink1));
                                    VERIFY_IS_TRUE(isFocused);
                                });

                                TestServices::WindowHelper->WaitForIdle();
                            }
                        }
                    }
                }
            }
        }
    }
}
