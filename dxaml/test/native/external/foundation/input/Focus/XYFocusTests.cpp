// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <TreeHelper.h>
#include "XYFocusTests.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <CommonInputHelper.h>
#include <FocusTestHelper.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Focus { namespace XYFocus {

        Platform::String^ XYFocusTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\foundation\\input\\focus\\";
        }

        bool XYFocusTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool XYFocusTests::ClassCleanup()
        {
            return true;
        }

        bool XYFocusTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool XYFocusTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Microsoft::UI::Xaml::UIElement^ XYFocusTests::SetupTest(Platform::String^ xamlFile, Platform::String^ rootElementName)
        {
            ::Windows::Foundation::Size size(1000, 800);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + xamlFile));
            VERIFY_IS_NOT_NULL(rootGrid);

            Microsoft::UI::Xaml::UIElement^ rootElement = nullptr;
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(Grid, Loaded);

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"XYFocusTests: SetupTest(%s,%s)", xamlFile->Data(), rootElementName->Data());

                rootElement = safe_cast<Microsoft::UI::Xaml::UIElement^>(rootGrid->FindName(rootElementName));
                VERIFY_IS_NOT_NULL(rootElement);

                loadedRegistration.Attach(
                    rootGrid,
                    ref new RoutedEventHandler([loadedEvent](Platform::Object^ sender, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"[RootGrid]: Loaded");
                    loadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            return rootElement;
        }
        //-------------------------------------------------------------------------------
        // Test case: Renders AutoFocusScene.xaml
        //            Sets focus on the first focusable element, injects 10 right keys
        //            to get to the last focusable element in "Right" nav direction
        //            Verify focus by verifying the element in the last section has focus
        //-------------------------------------------------------------------------------
        void XYFocusTests::ClippedElementsTest()
        {
            TestCleanupWrapper cleanup;

            auto hubControl = (Microsoft::UI::Xaml::Controls::Hub^)SetupTest(L"AutoFocusScene.xaml", L"hub");

            auto loadedRegistration = CreateSafeEventRegistration(HubSection, Loaded);
            auto loadedEvent = std::make_shared<Event>();

            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto btn00 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(hubControl, L"btn00"));
                VERIFY_IS_NOT_NULL(btn00);

                LOG_OUTPUT(L"Setting focus on Button00");
                btn00->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);

                auto btn40 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(hubControl, L"btn40"));
                VERIFY_IS_NOT_NULL(btn40);

                gotFocusRegistration.Attach(btn40, ref new RoutedEventHandler([gotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"[btn40]: Got Focus Event Fired");
                    gotFocusEvent->Set();
                }));

            });
            TestServices::WindowHelper->WaitForIdle();

            for (int i = 0; i<10; i++)
            {
                TestServices::KeyboardHelper->GamepadDpadRight();
                TestServices::WindowHelper->WaitForIdle();
            }
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());
        }

        //----------------------------------------------------------------------------
        // Test case: Renders DistantElementsScene.xaml
        //            Sets focus on the first focusable element,
        //            Verifies that transitioning focus to a distant element works
        //----------------------------------------------------------------------------
        void XYFocusTests::LargeDistanceBetweenFocusableElements()
        {
            TestCleanupWrapper cleanup;

            auto gridControl = (Microsoft::UI::Xaml::Controls::Grid^)SetupTest(L"DistantElementsScene.xaml", L"rootGrid");
            auto targetgotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto targetgotFocusEvent = std::make_shared<Event>();

            xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

            RunOnUIThread([&]()
            {
                auto btn0 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(gridControl, L"btn0"));
                VERIFY_IS_NOT_NULL(btn0);

                LOG_OUTPUT(L"Setting focus on Button0");
                btn0->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);

                auto btn1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(gridControl, L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                gotFocusHandler = ref new xaml::RoutedEventHandler([targetgotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"[btn1]: GotFocus Event Fired");
                    targetgotFocusEvent->Set();
                });

                targetgotFocusRegistration.Attach(btn1, gotFocusHandler);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(targetgotFocusEvent->HasFired());
        }
        //--------------------------------------------------------------------------------
        // Test case: Renders FullScreenMediaScene.xaml
        //            Sets focus on the first focusable element,
        //            Verifies that Media transport controls are focusable using Auto-Focus
        //--------------------------------------------------------------------------------
        void XYFocusTests::MediaTransportControlsTest()
        {
        // TODO: Convert ME tests to MPE.
#if false
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(Size(400, 300));

            MediaElement^ mediaElement = nullptr;
            xaml_controls::Button^ playPauseButton = nullptr;
            xaml_controls::ContentControl^ rootContentControl = nullptr;
            auto loadedRegistration = CreateSafeEventRegistration(FrameworkElement, Loaded);
            auto openedRegistration = CreateSafeEventRegistration(MediaElement, MediaOpened);
            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);

            auto loadedEvent = std::make_shared<Event>();
            auto mediaOpenedEvent = std::make_shared<Event>();
            auto gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                rootContentControl = ref new xaml_controls::ContentControl();
                loadedRegistration.Attach(rootContentControl, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded event fired on ContentControl");
                    loadedEvent->Set();
                }));
                mediaElement = ref new xaml_controls::MediaElement();
                openedRegistration.Attach(
                    mediaElement,
                    ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Media Opened Event Fired");
                    mediaOpenedEvent->Set();
                }));
                rootContentControl->Content = mediaElement;
                TestServices::WindowHelper->WindowContent = rootContentControl;
            });
            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();
            VERIFY_IS_TRUE(loadedEvent->HasFired());

            LOG_OUTPUT(L"Set Source and show MediaTransportControls");
            RunOnUIThread([&]()
            {
                mediaElement->AreTransportControlsEnabled = true;
                mediaElement->IsLooping = true;
                auto testUri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\media\\blueframe_video.mp4");
                mediaElement->Source = testUri;
            });
            mediaOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(mediaOpenedEvent->HasFired());
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                playPauseButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(mediaElement, "PlayPauseButton"));
                VERIFY_IS_NOT_NULL(playPauseButton);
                LOG_OUTPUT(L"Setting focus on Play/Pause Button");
                playPauseButton->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);

                gotFocusRegistration.Attach(playPauseButton,
                    ref new RoutedEventHandler([gotFocusEvent](Platform::Object^ sender, RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"[MTCButton]: Got Focus Event Fired");
                    gotFocusEvent->Set();
                }));

                mediaElement->IsFullWindow = true;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadRight();
            TestServices::KeyboardHelper->GamepadDpadRight();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(gotFocusEvent->HasFired());
#endif
        }

        //----------------------------------------------------------------------------
        // Test case: Renders AutoFocusPage1.xaml
        //            Sets focus on the first focusable element, injects a down key
        //            to get to the last focusable element in the current page
        //            On Click, the page transitions to AutoFocusPage2.xaml
        //            Verifies that AutoFocus works correctly in the new page
        //----------------------------------------------------------------------------
        void XYFocusTests::PageNavigationTest()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Page^ page = nullptr;

            auto pageLoadedRegistration = CreateSafeEventRegistration(StackPanel, Loaded);
            auto pageLoadedEvent = std::make_shared<Event>();
            auto rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"AutoFocusPage1.xaml"));
            RunOnUIThread([&]()
            {
                page = TestServices::WindowHelper->SetupSimulatedAppPage();
                pageLoadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    pageLoadedEvent->Set();
                }));

                page->Content = rootStackPanel;
            });
            pageLoadedEvent->WaitForDefault();

            auto gotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto gotFocusEvent = std::make_shared<Event>();
            auto clickRegistration = CreateSafeEventRegistration(Button, Click);
            auto clickEvent = std::make_shared<Event>();

            xaml_controls::Page^ page2 = nullptr;
            auto newPageStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"AutoFocusPage2.xaml"));
            VERIFY_IS_NOT_NULL(newPageStackPanel);
            auto newpageLoadedRegistration = CreateSafeEventRegistration(StackPanel, Loaded);
            auto newpageLoadedEvent = std::make_shared<Event>();

            xaml::RoutedEventHandler^ gotFocusHandler = nullptr;

            RunOnUIThread([&]()
            {
                auto btn1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Btn1"));
                VERIFY_IS_NOT_NULL(btn1);

                auto btn2 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Btn2"));
                VERIFY_IS_NOT_NULL(btn2);

                gotFocusHandler = ref new xaml::RoutedEventHandler([gotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"[btn2]: Getting Focus Event Fired");
                    gotFocusEvent->Set();
                });

                gotFocusRegistration.Attach(btn2, gotFocusHandler);

                LOG_OUTPUT(L"Setting focus on Button1");
                btn1->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);

                clickRegistration.Attach(btn2, ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    LOG_OUTPUT(L"[btn2]: Click Event Fired");
                    LOG_OUTPUT(L"[Page1]: Navigating to Page 2");

                    page2 = TestServices::WindowHelper->SetupSimulatedAppPage();
                    page2->Content = newPageStackPanel;
                    page->Frame->Navigate(::Windows::UI::Xaml::Interop::TypeName(page2->GetType()));

                    clickEvent->Set();
                }));

                newpageLoadedRegistration.Attach(newPageStackPanel, ref new xaml::RoutedEventHandler(
                    [&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
                {
                    newpageLoadedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->Enter();

            gotFocusEvent->WaitForDefault();
            clickEvent->WaitForDefault();
            newpageLoadedEvent->WaitForDefault();
            VERIFY_IS_TRUE(newpageLoadedEvent->HasFired());

            auto newpagegotFocusRegistration = CreateSafeEventRegistration(UIElement, GotFocus);
            auto newpagegotFocusEvent = std::make_shared<Event>();

            xaml::RoutedEventHandler^ gotFocusHandler2 = nullptr;

            RunOnUIThread([&]()
            {
                auto btn1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(newPageStackPanel, L"newPageBtn1"));
                VERIFY_IS_NOT_NULL(btn1);

                auto btn2 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(newPageStackPanel, L"newPageBtn2"));
                VERIFY_IS_NOT_NULL(btn2);

                gotFocusHandler2 = ref new xaml::RoutedEventHandler([newpagegotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"[NewPagebtn2]: Getting Focus Event Fired");
                    newpagegotFocusEvent->Set();
                });

                newpagegotFocusRegistration.Attach(btn2, gotFocusHandler2);

                LOG_OUTPUT(L"[NewPage] Setting focus on Button1");
                btn1->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();
            VERIFY_IS_TRUE(newpagegotFocusEvent->HasFired());
        }

        //--------------------------------------------------------------------------------
        // Test case: Renders AutoFocusScrollViewer.xaml
        //            Sets focus on the first focusable element,
        //            Verifies that ScrollViewer is able to move focus within its items
        //            when interacted with using a Gamepad
        //--------------------------------------------------------------------------------
        void XYFocusTests::ScrollViewerScrollTest()
        {
            TestCleanupWrapper cleanup;

            auto rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"AutoFocusScrollViewer.xaml"));

            auto btn0_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn1_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn2_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn3_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn4_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn5_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto bottombtn_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);

            auto bottombtn_gotFocusEvent = std::make_shared<Event>();
            auto btn0_gotFocusEvent = std::make_shared<Event>();
            auto btn1_gotFocusEvent = std::make_shared<Event>();
            auto btn2_gotFocusEvent = std::make_shared<Event>();
            auto btn3_gotFocusEvent = std::make_shared<Event>();
            auto btn4_gotFocusEvent = std::make_shared<Event>();
            auto btn5_gotFocusEvent = std::make_shared<Event>();


            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto topButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"TopButton"));
                VERIFY_IS_NOT_NULL(topButton);

                auto button0 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button0"));
                VERIFY_IS_NOT_NULL(button0);

                btn0_gotFocusRegistration.Attach(button0, [btn0_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button0]: Got Focus Event Fired");
                    btn0_gotFocusEvent->Set();
                });

                LOG_OUTPUT(L"Setting focus on Button");
                topButton->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(btn0_gotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                auto btn1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button1"));
                btn1_gotFocusRegistration.Attach(btn1, [btn1_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button1]: Got Focus Event Fired");
                    btn1_gotFocusEvent->Set();
                });

                auto btn2 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button2"));
                btn2_gotFocusRegistration.Attach(btn2, [btn2_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button2]: Got Focus Event Fired");
                    btn2_gotFocusEvent->Set();
                });

                auto btn3 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button3"));
                btn3_gotFocusRegistration.Attach(btn3, [btn3_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button3]: Got Focus Event Fired");
                    btn3_gotFocusEvent->Set();
                });

                auto btn4 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button4"));
                btn4_gotFocusRegistration.Attach(btn4, [btn4_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button4]: Got Focus Event Fired");
                    btn4_gotFocusEvent->Set();
                });

                auto btn5 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button5"));
                btn5_gotFocusRegistration.Attach(btn5, [btn5_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button5]: Got Focus Event Fired");
                    btn5_gotFocusEvent->Set();
                });

                auto bottombtn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"BottomButton"));
                bottombtn_gotFocusRegistration.Attach(bottombtn, [bottombtn_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[BottomButton]: Got Focus Event Fired");
                    bottombtn_gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();

            const int PRESSES = 6;

            for (int count = 0; count < PRESSES; count++)
            {
                TestServices::KeyboardHelper->GamepadDpadDown();
                TestServices::WindowHelper->WaitForIdle();
            }
            VERIFY_IS_TRUE(btn1_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn2_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn3_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn4_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn5_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(bottombtn_gotFocusEvent->HasFired());
        }

        void XYFocusTests::NarrowScrollViewerScrollTest()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootStackPanel = nullptr;

            auto btn0_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn1_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn2_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn3_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn4_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto btn5_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);
            auto bottombtn_gotFocusRegistration = CreateSafeEventRegistration(Button, GotFocus);

            auto bottombtn_gotFocusEvent = std::make_shared<Event>();
            auto btn0_gotFocusEvent = std::make_shared<Event>();
            auto btn1_gotFocusEvent = std::make_shared<Event>();
            auto btn2_gotFocusEvent = std::make_shared<Event>();
            auto btn3_gotFocusEvent = std::make_shared<Event>();
            auto btn4_gotFocusEvent = std::make_shared<Event>();
            auto btn5_gotFocusEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                rootStackPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" x:Name="rootStackPanel">
                          <StackPanel Orientation="Horizontal" HorizontalAlignment="Center">
                            <Button x:Name="TopButton" Width="500" Content="Top Button" />
                          </StackPanel>
                          <ScrollViewer Height="300" Width="50" x:Name="ScrollViewer">
                              <StackPanel>
                                  <Button x:Name="Button0" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                                  <Button x:Name="Button1" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                                  <Button x:Name="Button2" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                                  <Button x:Name="Button3" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                                  <Button x:Name="Button4" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                                  <Button x:Name="Button5" Content="Button" Height="100" HorizontalAlignment="Stretch" />
                              </StackPanel>
                          </ScrollViewer>
                          <StackPanel Orientation="Horizontal" HorizontalAlignment="Center">
                            <Button x:Name="BottomButton" Content="Bottom Button" />
                            <Button Content="Bottom Button" />
                          </StackPanel>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto topButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"TopButton"));
                VERIFY_IS_NOT_NULL(topButton);

                auto button0 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button0"));
                VERIFY_IS_NOT_NULL(button0);

                btn0_gotFocusRegistration.Attach(button0, [btn0_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button0]: Got Focus Event Fired");
                    btn0_gotFocusEvent->Set();
                });

                LOG_OUTPUT(L"Setting focus on Button");
                topButton->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(btn0_gotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                auto btn1 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button1"));
                btn1_gotFocusRegistration.Attach(btn1, [btn1_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button1]: Got Focus Event Fired");
                    btn1_gotFocusEvent->Set();
                });

                auto btn2 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button2"));
                btn2_gotFocusRegistration.Attach(btn2, [btn2_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button2]: Got Focus Event Fired");
                    btn2_gotFocusEvent->Set();
                });

                auto btn3 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button3"));
                btn3_gotFocusRegistration.Attach(btn3, [btn3_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button3]: Got Focus Event Fired");
                    btn3_gotFocusEvent->Set();
                });

                auto btn4 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button4"));
                btn4_gotFocusRegistration.Attach(btn4, [btn4_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button4]: Got Focus Event Fired");
                    btn4_gotFocusEvent->Set();
                });

                auto btn5 = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Button5"));
                btn5_gotFocusRegistration.Attach(btn5, [btn5_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[Button5]: Got Focus Event Fired");
                    btn5_gotFocusEvent->Set();
                });

                auto bottombtn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"BottomButton"));
                bottombtn_gotFocusRegistration.Attach(bottombtn, [bottombtn_gotFocusEvent]
                {
                    LOG_OUTPUT(L"[BottomButton]: Got Focus Event Fired");
                    bottombtn_gotFocusEvent->Set();
                });
            });

            TestServices::WindowHelper->WaitForIdle();

            const int PRESSES = 6;

            for (int count = 0; count < PRESSES; count++)
            {
                TestServices::KeyboardHelper->GamepadDpadDown();
                TestServices::WindowHelper->WaitForIdle();
            }
            VERIFY_IS_TRUE(btn1_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn2_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn3_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn4_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(btn5_gotFocusEvent->HasFired());
            VERIFY_IS_TRUE(bottombtn_gotFocusEvent->HasFired());
        }

        //--------------------------------------------------------------------------------
        // Test case: Renders AutoFocusHyperlink.xaml
        //            Sets focus on the first focusable element,
        //            Verifies that Hyperlinks are focusable using Automatic Focus
        //--------------------------------------------------------------------------------
        void XYFocusTests::HyperlinkFocusableTest()
        {
            TestCleanupWrapper cleanup;

            auto rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"AutoFocusHyperlink.xaml"));
            xaml::Controls::Button^ btn = nullptr;
            xaml::Documents::Hyperlink^ hyperlink = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<Button^>(TreeHelper::GetVisualChildByName(rootStackPanel, L"Btn"));
                VERIFY_IS_NOT_NULL(btn);


                LOG_OUTPUT(L"Setting focus on Button");
                btn->Focus(Microsoft::UI::Xaml::FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootStackPanel->FindName(L"hyperlink"));
                VERIFY_IS_NOT_NULL(hyperlink);

                //Focus should now be on the hyperlink, which is hosted in a TextBlock
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                hyperlink = safe_cast<xaml_docs::Hyperlink^>(rootStackPanel->FindName(L"hyperlinkRTB"));
                VERIFY_IS_NOT_NULL(hyperlink);

                //Focus should now be on the hyperlink, which is hosted in a RichTextBlock
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //Focus should be back on the button
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void XYFocusTests::XYFocusUsesDirectionOverride()
        {
            XYFocusUsesDirectionOverrideInternal(FocusElementType::Button);
        }

        void XYFocusTests::XYFocusUsesDirectionOverrideForStackPanel()
        {
            XYFocusUsesDirectionOverrideInternal(FocusElementType::StackPanel);
        }

        void XYFocusTests::XYFocusUsesDirectionOverrideForTextBlock()
        {
            XYFocusUsesDirectionOverrideInternal(FocusElementType::TextBlock);
        }

        void XYFocusTests::XYFocusUsesDirectionOverrideForRichTextBlock()
        {
            XYFocusUsesDirectionOverrideInternal(FocusElementType::RichTextBlock);
        }

        void XYFocusTests::XYFocusUsesDirectionOverrideInternal(FocusElementType elementType)
        {            
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            FrameworkElement^ element1;
            FrameworkElement^ element2;

            auto elementGotFocusEvent = std::make_shared<Event>();
            auto elementGotFocusRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, GotFocus);

            RunOnUIThread([&]()
            {
                switch (elementType)
                {
                    case FocusElementType::Button:
                        rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                            L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                            L"    <Button x:Name='element1' Height='20' Width='200' Content='Button' />"
                            L"    <Button x:Name='element2' Height='80' Width='200' Content='Button2'/>"
                            L"    <Button x:Name='element3' Height='80' Width='200' Content='Button3'/>"
                            L"</StackPanel>"));
                        break;
                        
                    case FocusElementType::StackPanel:
                        rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                            L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                            L"    <StackPanel x:Name='element1' HorizontalAlignment='Left' Width='50' Height='20' Background='Red' IsTabStop='True' UseSystemFocusVisuals='True'/>"
                            L"    <StackPanel x:Name='element2' HorizontalAlignment='Left' Width='50' Height='20' Background='Green' IsTabStop='True' UseSystemFocusVisuals='True'/>"
                            L"    <StackPanel x:Name='element3' HorizontalAlignment='Left' Width='50' Height='20' Background='Blue' IsTabStop='True' UseSystemFocusVisuals='True'/>"
                            L"</StackPanel>"));
                        break;
                        
                    case FocusElementType::TextBlock:
                        rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                            L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                            L"    <TextBlock x:Name='element1' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True' Text='TextBlock1'/>"
                            L"    <TextBlock x:Name='element2' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True' Text='TextBlock2'/>"
                            L"    <TextBlock x:Name='element3' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True' Text='TextBlock3'/>"
                            L"</StackPanel>"));
                        break;
                        
                    case FocusElementType::RichTextBlock:
                        rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                            L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                            L"    <RichTextBlock x:Name='element1' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True'>"
                            L"      <Paragraph>RichTextBlock1</Paragraph>"
                            L"    </RichTextBlock>"
                            L"    <RichTextBlock x:Name='element2' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True'>"
                            L"      <Paragraph>RichTextBlock2</Paragraph>"
                            L"    </RichTextBlock>"
                            L"    <RichTextBlock x:Name='element3' HorizontalAlignment='Left' IsTabStop='True' UseSystemFocusVisuals='True'>"
                            L"      <Paragraph>RichTextBlock3</Paragraph>"
                            L"    </RichTextBlock>"
                            L"</StackPanel>"));
                        break;
                        
                }

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                element1 = safe_cast<FrameworkElement^>(rootPanel->FindName(L"element1"));
                element2 = safe_cast<FrameworkElement^>(rootPanel->FindName(L"element1"));

                element1->XYFocusLeft = element2;

                elementGotFocusRegistration.Attach(
                    element2,
                    [elementGotFocusEvent]()
                {
                    LOG_OUTPUT(L"Element 2 GotFocus.");
                    elementGotFocusEvent->Set();
                });

                FocusManager::TryFocusAsync(element1, FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->GamepadDpadLeft();
            elementGotFocusEvent->WaitForDefault();

            VERIFY_IS_TRUE(elementGotFocusEvent->HasFired());
        }

        void XYFocusTests::AutoFocusRunsWhenDirectionOverrideSetToNull()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            Button^ btn1;
            Button^ btn2;

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"    <Button x:Name='button3' Height='80' Width='200' Content='Button2'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
                btn1->XYFocusDown = nullptr;

                btn1->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                //Focus should be back on the button
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn2));
            });
        }

        void XYFocusTests::DirectionOverrideCanSetFocusToSelf()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            Button^ btn1;

            auto buttonLostFocusEvent = std::make_shared<Event>();
            auto buttonLostFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, LostFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"    <Button x:Name='button3' Height='80' Width='200' Content='Button2'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                auto btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
                btn1->XYFocusDown = btn1;

                buttonLostFocusRegistration.Attach(
                    btn1,
                    [buttonLostFocusEvent]()
                {
                    LOG_OUTPUT(L"Button Lost Focus.");
                    buttonLostFocusEvent->Set();
                });

                btn1->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_FALSE(buttonLostFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                //Focus should be back on the button
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn1));
            });
        }

        void XYFocusTests::AutoFocusConeEliminatesCandidates()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::RelativePanel^ rootPanel = nullptr;
            xaml_controls::Button^ btn = nullptr;

            auto gotFocusButtonEvent = std::make_shared<Event>();
            auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                    LR"(<RelativePanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Button Content="Button"/>
                        <Button Margin="50,300,0,0" Content="Button"/>
                        <Button x:Name="button" Margin="0,100,0,0" RelativePanel.AlignRightWithPanel="True" Content="Button"/>
                    </RelativePanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

                gotFocusButtonRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"button got focus");
                    gotFocusButtonEvent->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadRight();
            TestServices::WindowHelper->WaitForIdle();

            gotFocusButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButtonEvent->HasFired());
        }

        void XYFocusTests::ValidateNestedScopeImplviaFindNextFocus()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::ScrollViewer^ sv = nullptr;
            xaml_controls::Button^ svBtn = nullptr;

            auto svBtnGotFocusEvent = std::make_shared<Event>();
            auto svBtnGotFocusEventRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto svGotFocusEvent = std::make_shared<Event>();
            auto svGotFocusEventRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <StackPanel Orientation="Horizontal">
                                <Button Width="100" Height="40" Content="Button 0" HorizontalAlignment="Center" x:Name="btn0"/>
                                <ScrollViewer x:Name="sv" Width="100" Height="200" IsTabStop="True">
                                    <Button Width="75" Height="40" Content="SVButton" x:Name="svBtn"/>
                                </ScrollViewer>
                            </StackPanel>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                sv = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"sv"));
                svGotFocusEventRegistration.Attach(sv, [&]()
                {
                    LOG_OUTPUT(L"ScrollViewer got focus");
                    svGotFocusEvent->Set();
                });

                svBtn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"svBtn"));
                svBtnGotFocusEventRegistration.Attach(svBtn, [&]()
                {
                    LOG_OUTPUT(L"Button inside given scope (ScrollViewer) got focus");
                    svBtnGotFocusEvent->Set();
                });

                auto btn0 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn0"));
                btn0->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadRight(); //btn0->ScrollViewer

            TestServices::WindowHelper->WaitForIdle();
            svGotFocusEvent->WaitForDefault();

            //Push focus into the sub-tree by calling FindNextFocusWithHintRect
            Platform::Object^ focusManager;
            xaml_input::IFocusManagerStaticsPrivate^ focusManagerPrivate = nullptr;

            VERIFY_SUCCEEDED(wf::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Input.FocusManager").Get(),
                reinterpret_cast<IInspectable**>(&focusManager)));
            VERIFY_IS_NOT_NULL(focusManager);

            focusManagerPrivate = safe_cast<xaml_input::IFocusManagerStaticsPrivate^>(focusManager);
            VERIFY_IS_NOT_NULL(focusManagerPrivate);

            RunOnUIThread([&]()
            {
                wf::Rect rect = {};
                wf::Rect emptyRect = {};
                auto point1 = sv->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
                //Bounds of an imaginary rect above the ScrollViewer that will be used as a HintRect
                //to find next focus
                rect.X = point1.X;
                rect.Y = point1.Y-50;
                rect.Width = 75;
                rect.Height = 40;

                xaml_controls::Control^ candidate = safe_cast<xaml_controls::Control^>(focusManagerPrivate->FindNextFocusWithSearchRootIgnoreEngagementWithHintRect(xaml_input::FocusNavigationDirection::Down, sv, rect, emptyRect));
                candidate->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();
            svBtnGotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(svBtnGotFocusEvent->HasFired());
        }

        void XYFocusTests::XYFocusBubbles()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::Button^ btn = nullptr;
            xaml_controls::ListViewItem^ lvi4 = nullptr;
            xaml_controls::GridView^ gv = nullptr;

            auto gotFocusButtonEvent = std::make_shared<Event>();
            auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <GridView x:Name="gv">
                                <ListViewItem Content="Item 1"/>
                                <ListViewItem Content="Item 2"/>
                                <ListViewItem Content="Item 3"/>
                                <ListViewItem x:Name="lvi4" Content="Item 4"/>
                            </GridView>
                            <Button x:Name="button" Content="Button 1"/>
                            <GridView>
                                <ListViewItem Content="Item 1"/>
                                <ListViewItem Content="Item 2"/>
                                <ListViewItem Content="Item 3"/>
                                <ListViewItem Content="Item 4"/>
                            </GridView>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                gv = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gv"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                lvi4 = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"lvi4"));

                gv->XYFocusDown = btn;

                gotFocusButtonRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"button got focus");
                    gotFocusButtonEvent->Set();
                });

                gotFocusRegistration.Attach(lvi4, [&]()
                {
                    LOG_OUTPUT(L"listviewitem got focus");
                    gotFocusEvent->Set();
                });

                lvi4->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(gotFocusButtonEvent->HasFired());
        }

        void XYFocusTests::XYFocusSetLocallyTakesPriorityOverParent()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::Button^ btn = nullptr;
            xaml_controls::GridViewItem^ gvi5 = nullptr;
            xaml_controls::GridViewItem^ gvi4 = nullptr;
            xaml_controls::GridView^ gv = nullptr;

            auto gotFocusButtonEvent = std::make_shared<Event>();
            auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::GridViewItem, GotFocus);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::GridViewItem, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <GridView x:Name="gv">
                                <GridViewItem Content="Item 1"/>
                                <GridViewItem Content="Item 2"/>
                                <GridViewItem Content="Item 3"/>
                                <GridViewItem x:Name="gvi4" Content="Item 4"/>
                            </GridView>
                            <Button x:Name="button" Content="Button 1"/>
                            <GridView>
                                <GridViewItem x:Name="gvi5" Content="Item 1"/>
                                <GridViewItem Content="Item 2"/>
                                <GridViewItem Content="Item 3"/>
                                <GridViewItem Content="Item 4"/>
                            </GridView>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                gv = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gv"));
                gvi5 = safe_cast<xaml_controls::GridViewItem^>(rootPanel->FindName(L"gvi5"));
                gvi4 = safe_cast<xaml_controls::GridViewItem^>(rootPanel->FindName(L"gvi4"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

                gv->XYFocusDown = btn;
                gvi4->XYFocusDown = gvi5;

                gotFocusButtonRegistration.Attach(gvi5, [&]()
                {
                    LOG_OUTPUT(L"gridview item 5 got focus");
                    gotFocusButtonEvent->Set();
                });

                gotFocusRegistration.Attach(gvi4, [&]()
                {
                    LOG_OUTPUT(L"gridview item 4 got focus");
                    gotFocusEvent->Set();
                });

                gvi4->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(gotFocusButtonEvent->HasFired());
        }

        void XYFocusTests::XYFocusBubblingDoesntHappenWhenStillInsideScope()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::StackPanel^ rootPanel = nullptr;
            xaml_controls::GridViewItem^ gvi8 = nullptr;
            xaml_controls::ListViewItem^ lvi4 = nullptr;
            xaml_controls::ContentControl^ contentControl = nullptr;
            xaml_controls::Button^ btn = nullptr;

            auto gotFocusButtonEvent = std::make_shared<Event>();
            auto gotFocusButtonRegistration = CreateSafeEventRegistration(xaml_controls::GridViewItem, GotFocus);

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <ContentControl x:Name="contentControl">
                                <StackPanel>
                                    <GridView x:Name="gv">
                                        <ListViewItem Width="100" Content="Item 1"/>
                                        <ListViewItem Width="100" Content="Item 2"/>
                                        <ListViewItem Width="100" Content="Item 3"/>
                                        <ListViewItem Width="100" x:Name="lvi4" Content="Item 4"/>
                                    </GridView>
                                    <Button x:Name="button" Content="Button 1"/>
                                    <GridView>
                                        <GridViewItem Width="100" Content="Item 5"/>
                                        <GridViewItem Width="100" Content="Item 6"/>
                                        <GridViewItem Width="100" Content="Item 7"/>
                                        <GridViewItem Width="100" x:Name="gvi8" Content="Item 8"/>
                                    </GridView>
                                </StackPanel>
                            </ContentControl>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                contentControl = safe_cast<xaml_controls::ContentControl^>(rootPanel->FindName(L"contentControl"));
                lvi4 = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"lvi4"));
                gvi8 = safe_cast<xaml_controls::GridViewItem^>(rootPanel->FindName(L"gvi8"));
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));

                contentControl->XYFocusDown = btn;

                gotFocusRegistration.Attach(lvi4, [&]()
                {
                    LOG_OUTPUT(L"listview item 4 got focus");
                    gotFocusEvent->Set();
                });

                gotFocusButtonRegistration.Attach(gvi8, [&]()
                {
                    LOG_OUTPUT(L"listview item 8 got focus");
                    gotFocusButtonEvent->Set();
                });

                lvi4->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            gotFocusButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusButtonEvent->HasFired());
        }

        void XYFocusTests::DoNotSelectElementThatIsNotTabStoppable()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            Button^ btn1;
            Button^ btn2;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusBRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <Button x:Name='button2' Height='80' Width='200' Content='Button2'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));

                btn2->IsTabStop = false;

                gotFocusRegistration.Attach(
                    btn1, [&]()
                {
                    LOG_OUTPUT(L"Button 1 Got Focus.");
                    gotFocusEvent->Set();
                });

                gotFocusBRegistration.Attach(
                    btn2, [&]()
                {
                    VERIFY_FAIL(L"Focus should not move to Button 2 because it is not tab stoppable");
                });

                btn1->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn1));
            });
        }

        void XYFocusTests::TextBlockShouldNotBeACandidate()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            Button^ btn1;
            RichTextBlock^ rtb;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusBRegistration = CreateSafeEventRegistration(xaml_controls::RichTextBlock, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                    L"<StackPanel Width='400' Height='400' VerticalAlignment='Top' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                    L"    <Button x:Name='button1' Height='20' Width='200' Content='Button' />"
                    L"    <RichTextBlock x:Name='rtb' Height='80' Width='200'/>"
                    L"</StackPanel>"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                rtb = safe_cast<xaml_controls::RichTextBlock^>(rootPanel->FindName(L"rtb"));

                gotFocusRegistration.Attach(
                    btn1, [&]()
                {
                    LOG_OUTPUT(L"Button 1 Got Focus.");
                    gotFocusEvent->Set();
                });

                gotFocusBRegistration.Attach(
                    rtb, [&]()
                {
                    VERIFY_FAIL(L"Focus should not move to Button 2 because it is not tab stoppable");
                });

                btn1->Focus(FocusState::Keyboard);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn1));
            });
        }

        void XYFocusTests::ToggleSwitchWithLongHeaderStillGainsFocus()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel;
            Button^ btn1;
            ToggleSwitch^ toggle;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusToggleEvent = std::make_shared<Event>();
            auto gotFocusToggleRegistration = CreateSafeEventRegistration(xaml_controls::ToggleSwitch, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <Button x:Name="btn1" Content="Button 1"/>
                            <ToggleSwitch x:Name="toggle" Header="1234567890123456789"/>
                            <Button Content="Button 2"/>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                toggle = safe_cast<xaml_controls::ToggleSwitch^>(rootPanel->FindName(L"toggle"));

                gotFocusRegistration.Attach(
                    btn1, [&]()
                {
                    LOG_OUTPUT(L"Button 1 Got Focus.");
                    gotFocusEvent->Set();
                });

                gotFocusToggleRegistration.Attach(
                    toggle, [&]()
                {
                    LOG_OUTPUT(L"Toggle Button Got Focus.");
                    gotFocusToggleEvent->Set();
                });

                btn1->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            TestServices::WindowHelper->WaitForIdle();
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();

            gotFocusToggleEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusToggleEvent->HasFired());
        }

        void XYFocusTests::ValidateScopedSearch()
        {
            // Leak: TemplateContent peer not being unpegged
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            MenuFlyout^ menuFlyout = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <Button x:Name="btn1" Content="Click for MenuFlyout">
                                <Button.Flyout>
                                    <MenuFlyout x:Name="MyMenuFlyout">
                                        <MenuFlyoutItem Text="Reset"/>
                                        <MenuFlyoutSubItem x:Name="subitem1" Text="More actions">
                                            <MenuFlyoutItem Text="Extra action #1"/>
                                            <MenuFlyoutSubItem x:Name="subitem2" Text="Even More Actions">
                                                <MenuFlyoutItem x:Name="item1" Text="Even more #1"/>
                                                <MenuFlyoutItem Text="Even more #2" x:Name="item2"/>
                                                <MenuFlyoutItem Text="Even more #3"/>
                                            </MenuFlyoutSubItem>
                                        </MenuFlyoutSubItem>
                                        <MenuFlyoutSeparator />
                                        <ToggleMenuFlyoutItem Text="Shuffle"/>
                                        <ToggleMenuFlyoutItem Text="Repeat"/>
                                    </MenuFlyout>
                                </Button.Flyout>
                            </Button>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            auto openedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Opened);
            auto menuFlyoutOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                menuFlyout = safe_cast<xaml_controls::MenuFlyout^>(btn1->Flyout);
                VERIFY_IS_NOT_NULL(menuFlyout);

                openedRegistration.Attach(menuFlyout, [&]()
                {
                    LOG_OUTPUT(L"MenuFlyout opened");
                    menuFlyoutOpenedEvent->Set();
                });
                gotFocusRegistration.Attach(btn1, [&]()
                {
                    LOG_OUTPUT(L"Button 1 Got Focus.");
                    gotFocusEvent->Set();
                });
                btn1->Focus(FocusState::Keyboard);
            });
            gotFocusEvent->WaitForDefault();

            TestServices::KeyboardHelper->GamepadA(); //open flyout
            menuFlyoutOpenedEvent->WaitForDefault();

            MenuFlyoutSubItem^ subitem1 = nullptr;
            auto subitem1_gotFocusEvent = std::make_shared<Event>();
            auto subitem1_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutSubItem, GotFocus);

            RunOnUIThread([&]()
            {
                subitem1 = safe_cast<xaml_controls::MenuFlyoutSubItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"subitem1", rootPanel));

                subitem1_gotFocusRegistration.Attach(subitem1, [&]()
                {
                    LOG_OUTPUT(L"Subitem 1 Got Focus.");
                    subitem1_gotFocusEvent->Set();
                });
            });

            TestServices::KeyboardHelper->GamepadDpadDown(); //Focus SubItem 1
            subitem1_gotFocusEvent->WaitForDefault();

            TestServices::KeyboardHelper->GamepadA(); //Open SubItem 1
            TestServices::WindowHelper->WaitForIdle();

            MenuFlyoutSubItem^ subitem2 = nullptr;

            RunOnUIThread([&]()
            {
                subitem2 = safe_cast<xaml_controls::MenuFlyoutSubItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"subitem2", rootPanel));
            });

            FocusTestHelper::EnsureFocus(subitem2, FocusState::Keyboard);

            TestServices::KeyboardHelper->GamepadA(); //Open SubItem 2
            TestServices::WindowHelper->WaitForIdle();

            MenuFlyoutItem^ item1 = nullptr;
            MenuFlyoutItem^ item2 = nullptr;
            xaml_primitives::Popup^ parentPopup = nullptr;

            auto item2_gotFocusEvent = std::make_shared<Event>();
            auto item2_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);

            auto item2_lostFocusEvent = std::make_shared<Event>();
            auto item2_lostFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, LostFocus);

            RunOnUIThread([&]()
            {
                item1 = safe_cast<xaml_controls::MenuFlyoutItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"item1", rootPanel));
                item2 = safe_cast<xaml_controls::MenuFlyoutItem^>(TreeHelper::GetVisualChildByNameFromOpenPopups(L"item2", rootPanel));
                parentPopup = GetContainingPopup(item1);
                VERIFY_IS_NOT_NULL(parentPopup);

                item2_gotFocusRegistration.Attach(item2, [&]()
                {
                    LOG_OUTPUT(L"Item 2 Got Focus.");
                    item2_gotFocusEvent->Set();
                });
                item2_lostFocusRegistration.Attach(item2, [&]()
                {
                    LOG_OUTPUT(L"Item 2 Lost Focus, verifying the state of parent popup");
                    item2_lostFocusEvent->Set();
                });
            });

            FocusTestHelper::EnsureFocus(item1, FocusState::Keyboard);

            TestServices::KeyboardHelper->GamepadDpadDown(); //Focus Item 1
            item2_gotFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(parentPopup->IsOpen);
            });

            TestServices::KeyboardHelper->GamepadDpadLeft();
            item2_lostFocusEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_IS_FALSE(parentPopup->IsOpen);
            });
            //Cleanup
            TestServices::KeyboardHelper->GamepadB();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->GamepadB();
            TestServices::WindowHelper->WaitForIdle();
        }

        void XYFocusTests::EngagedElementCanStillNavigateThroughPopupsOpenedDuringEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;
            Button^ popupBtn = nullptr;
            xaml_primitives::Popup^ popup = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusPopupButtonEvent = std::make_shared<Event>();
            auto gotFocusPopupButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btnFocusEngaged = std::make_shared<Event>();
            auto btnFocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn" Content="Button 1"/>
                            <Popup x:Name="popup">
                                <StackPanel>
                                    <Button x:Name="popupButton" Content="Button 2"/>
                                </StackPanel>
                            </Popup>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                popupBtn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"popupButton"));
                popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));

                btn->IsFocusEngagementEnabled = true;
                btnFocusEngagedRegistration.Attach(btn, [&]() { btnFocusEngaged->Set(); });

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                gotFocusPopupButtonRegistration.Attach(popupBtn, [&]()
                {
                    LOG_OUTPUT(L"popup btn gained focus");
                    gotFocusPopupButtonEvent->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            LOG_OUTPUT(L"Engaging element");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btnFocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, true);
                LOG_OUTPUT(L"Opening Popup");
                popup->IsOpen = true;
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButtonEvent->HasFired());
        }

        void XYFocusTests::EngagedElementDoesNotNavigateThroughPopupsOpenedBeforeEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;
            xaml_primitives::Popup^ popup = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btnFocusEngaged = std::make_shared<Event>();
            auto btnFocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn" Content="Button 1"/>
                            <Popup x:Name="popup">
                                <StackPanel>
                                    <Button x:Name="popupButton" Content="Button 2"/>
                                </StackPanel>
                            </Popup>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));

                btn->IsFocusEngagementEnabled = true;
                popup->IsOpen = true;
                btnFocusEngagedRegistration.Attach(btn, [&]() { btnFocusEngaged->Set(); });

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            LOG_OUTPUT(L"Engaging element");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btnFocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, true);
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void XYFocusTests::EngagedElementCanDistinguishBetweenPopupsOpenedBeforeAndAfterEngagement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;
            Button^ popupBtn = nullptr;
            Button^ popupTopBtn = nullptr;
            xaml_primitives::Popup^ popup = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusPopupButtonEvent = std::make_shared<Event>();
            auto gotFocusPopupButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusPopupButton2Event = std::make_shared<Event>();
            auto gotFocusPopupButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btnFocusEngaged = std::make_shared<Event>();
            auto btnFocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn" Content="Button 1"/>
                            <Popup IsOpen="True">
                                <StackPanel>
                                    <Button x:Name="popupButton" Content="Button 2"/>
                                </StackPanel>
                            </Popup>
                            <Popup x:Name="popup">
                                <StackPanel>
                                    <Button x:Name="popupButton2" Margin="0, 50, 0, 0" Content="Button 3"/>
                                </StackPanel>
                            </Popup>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                popupBtn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"popupButton2"));
                popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));
                popupTopBtn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"popupButton"));

                btn->IsFocusEngagementEnabled = true;
                btnFocusEngagedRegistration.Attach(btn, [&]() { btnFocusEngaged->Set(); });

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                gotFocusPopupButtonRegistration.Attach(popupBtn, [&]()
                {
                    LOG_OUTPUT(L"popup btn gained focus");
                    gotFocusPopupButtonEvent->Set();
                });

                gotFocusPopupButton2Registration.Attach(popupTopBtn, [&]()
                {
                    LOG_OUTPUT(L"popup btn 2 gained focus");
                    gotFocusPopupButton2Event->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            LOG_OUTPUT(L"Engaging element");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btnFocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, true);
                LOG_OUTPUT(L"Opening Popup");
                popup->IsOpen = true;
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButtonEvent->HasFired());
            VERIFY_IS_FALSE(gotFocusPopupButton2Event->HasFired());

            gotFocusEvent->Reset();
            gotFocusPopupButtonEvent->Reset();
            gotFocusPopupButton2Event->Reset();
            btnFocusEngaged->Reset();

            LOG_OUTPUT(L"Disengage");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Cancel(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, false);
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButton2Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButton2Event->HasFired());

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButtonEvent->HasFired());

            RunOnUIThread([&]()
            {
                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            LOG_OUTPUT(L"Engaging element");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btnFocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, true);
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(btn));
            });
        }

        void XYFocusTests::EngagedElementCanNavigateToPopupOpenedByAnotherPopup()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn = nullptr;
            Button^ popupBtn = nullptr;
            Button^ popupBtn2 = nullptr;
            xaml_primitives::Popup^ popup = nullptr;
            xaml_primitives::Popup^ popup2 = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusPopupButtonEvent = std::make_shared<Event>();
            auto gotFocusPopupButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusPopupButton2Event = std::make_shared<Event>();
            auto gotFocusPopupButton2Registration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btnFocusEngaged = std::make_shared<Event>();
            auto btnFocusEngagedRegistration = CreateSafeEventRegistration(xaml_controls::Control, FocusEngaged);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn" Content="Button 1"/>
                            <Popup x:Name="popup">
                                <StackPanel>
                                    <Button x:Name="popupButton" Content="Button 2"/>
                                </StackPanel>
                            </Popup>
                            <Popup x:Name="popup2">
                                <StackPanel>
                                    <Button x:Name="popupButton2" Margin="0, 50, 0, 0" Content="Button 3"/>
                                </StackPanel>
                            </Popup>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                btn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn"));
                popupBtn = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"popupButton"));
                popupBtn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"popupButton2"));
                popup = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup"));
                popup2 = safe_cast<xaml_primitives::Popup^>(rootPanel->FindName(L"popup2"));

                btn->IsFocusEngagementEnabled = true;
                btnFocusEngagedRegistration.Attach(btn, [&]() { btnFocusEngaged->Set(); });

                gotFocusRegistration.Attach(btn, [&]()
                {
                    LOG_OUTPUT(L"btn gained focus");
                    gotFocusEvent->Set();
                });

                gotFocusPopupButtonRegistration.Attach(popupBtn, [&]()
                {
                    LOG_OUTPUT(L"popup btn gained focus");
                    gotFocusPopupButtonEvent->Set();
                });

                gotFocusPopupButton2Registration.Attach(popupBtn2, [&]()
                {
                    LOG_OUTPUT(L"popup btn 2 gained focus");
                    gotFocusPopupButton2Event->Set();
                });

                btn->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            LOG_OUTPUT(L"Engaging element");
            Microsoft::UI::Xaml::Tests::Common::CommonInputHelper::Accept(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            btnFocusEngaged->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(btn->IsFocusEngaged, true);
                LOG_OUTPUT(L"Opening Popup");
                popup->IsOpen = true;
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButtonEvent->HasFired());
            VERIFY_IS_FALSE(gotFocusPopupButton2Event->HasFired());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Opening Popup 2");
                popup2->IsOpen = true;
            });

            CommonInputHelper::Down(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusPopupButton2Event->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusPopupButton2Event->HasFired());
        }

        void XYFocusTests::EnsureClipBoundsBeingUsedWhenScoringElementsInsideSplitView()
        {
            TestCleanupWrapper cleanup;

            Grid^ rootPanel = nullptr;
            Button^ splitViewButton = nullptr;
            Button^ contentButton = nullptr;

            auto gotFocusEvent = std::make_shared<Event>();
            auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto gotFocusContentButtonEvent = std::make_shared<Event>();
            auto gotFocusContentButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                            <SplitView x:Name="MySplitView" DisplayMode="CompactOverlay" IsPaneOpen="False" CompactPaneLength="50" OpenPaneLength="150">
                                <SplitView.Pane>
                                    <StackPanel Background="Gray">
                                        <Button x:Name="button" Content="Button" Width="500" Height="50"/>
                                    </StackPanel>
                                </SplitView.Pane>
                                <SplitView.Content>
                                    <Canvas>
                                        <Button Canvas.Top="100" Canvas.Left="0" x:Name="contentButton" Content="button" />
                                    </Canvas>
                                </SplitView.Content>
                            </SplitView>
                        </Grid>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                splitViewButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
                contentButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"contentButton"));

                gotFocusRegistration.Attach(splitViewButton, [&]()
                {
                    LOG_OUTPUT(L"splitview button gained focus");
                    gotFocusEvent->Set();
                });

                gotFocusContentButtonRegistration.Attach(contentButton, [&]()
                {
                    LOG_OUTPUT(L"Content button gained focus");
                    gotFocusContentButtonEvent->Set();
                });

                splitViewButton->Focus(FocusState::Keyboard);
            });

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());

            CommonInputHelper::Right(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusContentButtonEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusContentButtonEvent->HasFired());

            gotFocusEvent->Reset();
            CommonInputHelper::Left(InputDevice::Gamepad);
            TestServices::WindowHelper->WaitForIdle();

            gotFocusEvent->WaitForDefault();
            VERIFY_IS_TRUE(gotFocusEvent->HasFired());
        }

        void XYFocusTests::ScrolledOutOccludedElementsTest()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ topRow_btn4 = nullptr;
            Button^ middleRow_btn4 = nullptr;
            Button^ bottomRow_btn4 = nullptr;

            auto topRow_btn4_gotFocusEvent = std::make_shared<Event>();
            auto topRow_btn4_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto middleRow_btn4_gotFocusEvent = std::make_shared<Event>();
            auto middleRow_btn4_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto bottomRow_btn4_gotFocusEvent = std::make_shared<Event>();
            auto bottomRow_btn4_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <StackPanel Orientation="Horizontal">
                                <Button Height="75" Width="75" Margin="10" Content="Button 1" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 2" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 3" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 4" x:Name="topRow_btn4"/>
                            </StackPanel>
                            <ScrollViewer HorizontalScrollBarVisibility="Auto" HorizontalScrollMode="Auto"
                                            VerticalScrollBarVisibility="Disabled" VerticalScrollMode="Disabled" Width="250" HorizontalAlignment="Left">
                                <StackPanel Orientation="Horizontal">
                                    <Button Height="75" Width="75" Margin="10" Content="Button 1" />
                                    <Button Height="75" Width="75" Margin="10" Content="Button 2" />
                                    <Button Height="75" Width="75" Margin="10" Content="Button 3" />
                                    <Button Height="75" Width="75" Margin="10" Content="Button 4" x:Name="middleRow_btn4"/>
                                </StackPanel>
                            </ScrollViewer>
                            <StackPanel Orientation="Horizontal">
                                <Button Height="75" Width="75" Margin="10" Content="Button 1" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 2" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 3" />
                                <Button Height="75" Width="75" Margin="10" Content="Button 4" x:Name="bottomRow_btn4"/>
                            </StackPanel>
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                topRow_btn4 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"topRow_btn4"));
                middleRow_btn4 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"middleRow_btn4"));
                bottomRow_btn4 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"bottomRow_btn4"));

                topRow_btn4_gotFocusRegistration.Attach(topRow_btn4, [&]()
                {
                    LOG_OUTPUT(L"topRow_btn4 gained focus");
                    topRow_btn4_gotFocusEvent->Set();
                });

                middleRow_btn4_gotFocusRegistration.Attach(middleRow_btn4, [&]()
                {
                    LOG_OUTPUT(L"middleRow_btn4 gained focus");
                    middleRow_btn4_gotFocusEvent->Set();
                });

                bottomRow_btn4_gotFocusRegistration.Attach(bottomRow_btn4, [&]()
                {
                    LOG_OUTPUT(L"bottomRow_btn4 gained focus");
                    bottomRow_btn4_gotFocusEvent->Set();
                });

                topRow_btn4->Focus(FocusState::Keyboard);
            });

            topRow_btn4_gotFocusEvent->WaitForDefault();
            CommonInputHelper::Down(InputDevice::Gamepad); //topRow_btn4->bottomRow_btn4

            bottomRow_btn4_gotFocusEvent->WaitForDefault();
            VERIFY_IS_FALSE(middleRow_btn4_gotFocusEvent->HasFired()); //middleRow_btn4 is occluded and clipped by ScrollViewer's viewport, it shouldn't get focus!
        }

        void XYFocusTests::HyperlinkScrollTest()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;
            Button^ btn3 = nullptr;
            xaml::Documents::Hyperlink^ hyperlink1 = nullptr;
            xaml::Documents::Hyperlink^ hyperlink2 = nullptr;

            auto btn1_gotFocusEvent = std::make_shared<Event>();
            auto btn1_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btn2_gotFocusEvent = std::make_shared<Event>();
            auto btn2_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            auto btn3_gotFocusEvent = std::make_shared<Event>();
            auto btn3_gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <ScrollViewer>
                                <StackPanel>
                                    <Button Height="100" Width="100" Margin="10" Content="Button 1" x:Name="btn1" />
                                    <TextBlock x:Name="HyperlinkText1">
                                        <Run xml:space="preserve">This is </Run><Hyperlink x:Name="hyperlink1" >Hyperlink Text</Hyperlink><Run>.</Run>
                                    </TextBlock>
                                    <Button Height="100" Width="100" Margin="10" Content="Button 2" x:Name="btn2" />
                                    <TextBlock x:Name="HyperlinkText2">
                                        <Run xml:space="preserve">This is </Run><Hyperlink x:Name="hyperlink2" > another Hyperlink Text</Hyperlink><Run>.</Run>
                                    </TextBlock>
                                </StackPanel>
                            </ScrollViewer>
                            <Button Height="100" Width="100" Margin="10" Content="Button 3" x:Name="btn3" />
                        </StackPanel>)"));

                TestServices::WindowHelper->WindowContent = rootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn2"));
                btn3 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn3"));
                hyperlink1 = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink1"));
                hyperlink2 = safe_cast<xaml_docs::Hyperlink^>(rootPanel->FindName(L"hyperlink2"));

                btn1_gotFocusRegistration.Attach(btn1, [&]()
                {
                    LOG_OUTPUT(L"btn1 gained focus");
                    btn1_gotFocusEvent->Set();
                });

                btn2_gotFocusRegistration.Attach(btn2, [&]()
                {
                    LOG_OUTPUT(L"btn2 gained focus");
                    btn2_gotFocusEvent->Set();
                });

                btn3_gotFocusRegistration.Attach(btn3, [&]()
                {
                    LOG_OUTPUT(L"btn3 gained focus");
                    btn3_gotFocusEvent->Set();
                });

                btn1->Focus(FocusState::Keyboard);
            });
            btn1_gotFocusEvent->WaitForDefault();

            CommonInputHelper::Down(InputDevice::Gamepad); //btn1->hyperlink1
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink1));
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Down(InputDevice::Gamepad); //hyperlink1->btn2
            btn2_gotFocusEvent->WaitForDefault();

            CommonInputHelper::Down(InputDevice::Gamepad); //btn2->hyperlink2
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(hyperlink2));
            });
            TestServices::WindowHelper->WaitForIdle();

            CommonInputHelper::Down(InputDevice::Gamepad); //hyperlink2->btn3
            btn3_gotFocusEvent->WaitForDefault();
        }

        void XYFocusTests::CandidatesThatAreFullyContainedWithinElementShouldBeIgnored()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            CheckBox^ checkBox = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <CheckBox x:Name="checkBox">
                                <StackPanel Orientation="Horizontal">
                                    <TextBlock Text="Checkbox"/>
                                    <HyperlinkButton Content="Test1" />
                                    <HyperlinkButton Content="Test2" />
                                </StackPanel>
                            </CheckBox>
                        </StackPanel>)"));

                checkBox = safe_cast<xaml_controls::CheckBox^>(rootPanel->FindName(L"checkBox"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(checkBox, FocusState::Keyboard);

            CommonInputHelper::Right(InputDevice::Gamepad);
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(checkBox));
            });
        }

        void XYFocusTests::UsingFocusHintRectShouldIncludeFocusedElement()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button x:Name="btn1" Width="50"/>
                            <Button x:Name="btn2" Width="50"/>
                        </StackPanel>)"));

                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"btn1"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn1, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                ::Windows::Foundation::Rect rect(-2, -2, 50, 1);
                Button^ element = safe_cast<Button^>(xaml_input::FocusManager::FindNextFocusableElement(xaml_input::FocusNavigationDirection::Down, rect));
                VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(element));
            });
        }

        void XYFocusTests::XYFocusIgnoresOcclusivity()
        {
            TestCleanupWrapper cleanup;

            StackPanel^ rootPanel = nullptr;
            Button^ btn1 = nullptr;
            Button^ btn2 = nullptr;

            RunOnUIThread([&]()
            {
                rootPanel = dynamic_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel Orientation='Horizontal'
                                    xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                    xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                            <Button Width='50' x:Name='button1' Canvas.ZIndex='1' Content='Button 1'/>
                            <Button Width='50' x:Name='button2' Canvas.ZIndex='0' Content='Occluding Button' Margin='-10,0,0,0'/>
                        </StackPanel>)"));

                btn1 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
                btn2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
                TestServices::WindowHelper->WindowContent = rootPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn1, FocusState::Keyboard);

            RunOnUIThread([&]()
            {
                xaml_input::FindNextElementOptions^ options = ref new xaml_input::FindNextElementOptions();
                xaml_input::IFindNextElementOptionsPrivate^ privateOptions = dynamic_cast<xaml_input::IFindNextElementOptionsPrivate^>(options);
                privateOptions->IgnoreOcclusivity = true;

                Button^ element = safe_cast<Button^>(xaml_input::FocusManager::FindNextElement(xaml_input::FocusNavigationDirection::Right, options));
                VERIFY_ARE_EQUAL(element, btn2);
            });
        }

        xaml_primitives::Popup^ XYFocusTests::GetContainingPopup(FrameworkElement^ element)
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(TestServices::WindowHelper->WindowContent->XamlRoot);

            for (auto popup : popups)
            {
                auto popupChild = safe_cast<xaml::FrameworkElement^>(popup->Child);
                if (popupChild->Name == element->Name)
                {
                    return popup;
                }
                else
                {
                    if (popupChild->FindName(element->Name))
                    {
                        return popup;
                    }
                }
            }
            return nullptr;
        }
    } }
} } } }
