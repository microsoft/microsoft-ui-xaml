// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicPointerTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>

using namespace Windows::UI::Core;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Pointer {

        bool BasicPointerTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicPointerTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicPointerTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Drag a Rectangle in a Canvas with the left mouse button and
        // raw pointer events.
        //------------------------------------------------------------------------
        void BasicPointerTests::CanDragARectangle()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> rectLoadedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> rectPointerCaptureLostEvent = std::make_shared<Event>();

            Canvas^ canvas = nullptr;
            Microsoft::UI::Xaml::Shapes::Rectangle^ rect = nullptr;
            wf::EventRegistrationToken rectLoadedToken = {};
            wf::EventRegistrationToken rectPointerEnteredToken = {};
            wf::EventRegistrationToken rectPointerPressedToken = {};
            wf::EventRegistrationToken rectPointerMovedToken = {};
            wf::EventRegistrationToken rectPointerReleasedToken = {};
            wf::EventRegistrationToken rectPointerExitedToken = {};
            wf::EventRegistrationToken rectPointerCaptureLostToken = {};

            UINT rectPointerEnteredCount = 0;
            UINT rectPointerPressedCount = 0;
            UINT rectPointerMovedCount = 0;
            UINT rectPointerReleasedCount = 0;
            UINT rectPointerExitedCount = 0;
            UINT rectPointerCaptureLostCount = 0;

            RunOnUIThread([&]()
            {
                Grid^ mainGrid = ref new Grid();

                canvas = ref new Canvas();
                canvas->Width = 300;
                canvas->Height = 400;
                canvas->HorizontalAlignment = HorizontalAlignment::Center;
                canvas->VerticalAlignment = VerticalAlignment::Center;
                canvas->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(canvas);

                rect = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                rect->Width = 100;
                rect->Height = 120;
                rect->Fill = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                canvas->Children->Append(rect);

                rectLoadedToken = rect->Loaded +=
                    ref new xaml::RoutedEventHandler([rectLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rectangle Loaded event");
                    rectLoadedEvent->Set();
                });

                rectPointerEnteredToken = rect->PointerEntered +=
                    ref new PointerEventHandler([&rectPointerEnteredCount, canvas](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    VERIFY_ARE_EQUAL(args->Pointer->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Mouse);
                    rectPointerEnteredCount++;

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);
                    LOG_OUTPUT(L"Rectangle PointerEntered event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                rectPointerPressedToken = rect->PointerPressed +=
                    ref new PointerEventHandler([&rectPointerPressedCount](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    rectPointerPressedCount++;

                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(rect);
                    VERIFY_IS_TRUE(ptrPt->Properties->IsLeftButtonPressed);
                    VERIFY_IS_FALSE(ptrPt->Properties->IsRightButtonPressed);

                    VERIFY_IS_TRUE(rect->CapturePointer(args->Pointer));
                    LOG_OUTPUT(L"Rectangle PointerPressed event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                rectPointerMovedToken = rect->PointerMoved +=
                    ref new PointerEventHandler([&rectPointerMovedCount, canvas](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    rectPointerMovedCount++;

                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);

                    Canvas::SetLeft(rect, ptrPt->Position.X - rect->Width / 2.0);
                    Canvas::SetTop(rect, ptrPt->Position.Y - rect->Height / 2.0);
                    LOG_OUTPUT(L"Rectangle PointerMoved event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                rectPointerReleasedToken = rect->PointerReleased +=
                    ref new PointerEventHandler([&rectPointerReleasedCount, canvas](Platform::Object^ sender, PointerRoutedEventArgs^ args)
                {
                    rectPointerReleasedCount++;

                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect = dynamic_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(sender);
                    VERIFY_IS_NOT_NULL(rect);

                    rect->ReleasePointerCapture(args->Pointer);

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);
                    LOG_OUTPUT(L"Rectangle PointerReleased event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                rectPointerExitedToken = rect->PointerExited +=
                    ref new PointerEventHandler([&rectPointerExitedCount, canvas](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    rectPointerExitedCount++;

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);
                    LOG_OUTPUT(L"Rectangle PointerExited event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                rectPointerCaptureLostToken = rect->PointerCaptureLost +=
                    ref new PointerEventHandler([&rectPointerCaptureLostCount, rectPointerCaptureLostEvent, canvas](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    rectPointerCaptureLostCount++;

                    rectPointerCaptureLostEvent->Set();

                    Microsoft::UI::Input::PointerPoint^ ptrPt = args->GetCurrentPoint(canvas);
                    LOG_OUTPUT(L"Rectangle PointerCaptureLost event. Position=(%.2f,%.2f).", ptrPt->Position.X, ptrPt->Position.Y);
                });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();
            rectLoadedEvent->WaitForDefault();
            LOG_OUTPUT(L"Dragging Rectangle.");
            TestServices::InputHelper->DragFromCenter(rect, 40 /*relX*/, 60 /*relY*/, 0.1 /*velocityFactor*/);
            rectPointerCaptureLostEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                double left = Canvas::GetLeft(rect);
                double top = Canvas::GetTop(rect);
                LOG_OUTPUT(L"Final Rectangle location: Left=%f Top=%f", left, top);

                rect->Loaded -= rectLoadedToken;
                rect->PointerEntered -= rectPointerEnteredToken;
                rect->PointerPressed -= rectPointerPressedToken;
                rect->PointerMoved -= rectPointerMovedToken;
                rect->PointerReleased -= rectPointerReleasedToken;
                rect->PointerExited -= rectPointerExitedToken;
                rect->PointerCaptureLost -= rectPointerCaptureLostToken;

                VERIFY_ARE_EQUAL(rectPointerEnteredCount, 1u);
                VERIFY_ARE_EQUAL(rectPointerPressedCount, 1u);
                VERIFY_IS_TRUE(rectPointerMovedCount > 0);
                VERIFY_ARE_EQUAL(rectPointerReleasedCount, 1u);
                VERIFY_ARE_EQUAL(rectPointerExitedCount, 0u);
                VERIFY_ARE_EQUAL(rectPointerCaptureLostCount, 1u);

                VERIFY_IS_TRUE(left > 38);
                VERIFY_IS_TRUE(left < 40);
                VERIFY_IS_TRUE(top > 58);
                VERIFY_IS_TRUE(top < 60);
            });
        }

        // Call VisualTreeHelper.FindElementsInHostCoordinates(Point) on the given target
        // and return true if it is found.
        bool HitTestAtPoint(UIElement^ target)
        {
            GeneralTransform^ transform = target->TransformToVisual(nullptr);
            wf::Point pt(5,5); // 5,5 is an arbitrary small offset into the target
            pt = transform->TransformPoint(pt);
            auto elems = VisualTreeHelper::FindElementsInHostCoordinates(pt, nullptr);
            for (auto iter = elems->First(); iter->HasCurrent; iter->MoveNext())
            {
                auto e = iter->Current;
                if (e == target)
                    return true;
            }
            return false;
        }

        // Call VisualTreeHelper.FindElementsInHostCoordinates(Rect) on the given target
        // and return true if it is found.
        bool HitTestAtRect(UIElement^ target)
        {
            GeneralTransform^ transform = target->TransformToVisual(nullptr);
            wf::Rect rect(0,0,5,5); // 0,0 to 5,5 is an arbitrary small offset into the target
            rect = transform->TransformBounds(rect);
            auto elems = VisualTreeHelper::FindElementsInHostCoordinates(rect, nullptr);
            for (auto iter = elems->First(); iter->HasCurrent; iter->MoveNext())
            {
                auto e = iter->Current;
                if (e == target)
                    return true;
            }
            return false;
        }

        bool HitTest(UIElement^ target)
        {
            return HitTestAtPoint(target) && HitTestAtRect(target);
        }

        //------------------------------------------------------------------------
        // Test case: Test VisualTreeHelper.FindElementsInHostCoordinates in LTR/RTL
        // at different scale levels.
        //------------------------------------------------------------------------
        void BasicPointerTests::VisualTreeHelperHitTest()
        {
            TestCleanupWrapper cleanup;

            Grid^ mainGrid = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ button1 = nullptr;
            Button^ button2 = nullptr;
            Button^ button3 = nullptr;
            Button^ button4 = nullptr;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            RunOnUIThread([&]()
            {
                mainGrid = ref new Grid();

                stackPanel = ref new StackPanel();
                stackPanel->Width = 400;
                stackPanel->Height = 400;
                stackPanel->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(stackPanel);

                button1 = ref new Button();
                button1->Content = "First";
                stackPanel->Children->Append(button1);

                button2 = ref new Button();
                button2->Content = "Second";
                stackPanel->Children->Append(button2);

                button3 = ref new Button();
                button3->Content = "Third";
                stackPanel->Children->Append(button3);

                button4 = ref new Button();
                button4->Content = "Fourth";
                stackPanel->Children->Append(button4);

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Testing LTR at 1.0 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::LeftToRight;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            LOG_OUTPUT(L"Testing RTL at 1.0 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::RightToLeft;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            // --- Switch to a different plateau scale
            // 1.4 scale tests
            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.4f);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Testing LTR at 1.4 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::LeftToRight;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            LOG_OUTPUT(L"Testing RTL at 1.4 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::RightToLeft;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            // Centered at 1.0 scale tests
            TestServices::WindowHelper->SetWindowSizeOverride(size);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Testing LTR centered at 1.0 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::LeftToRight;
                // Use a width less than size.Width which will default to centering
                mainGrid->Width = 200;
                stackPanel->Width = 200;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            LOG_OUTPUT(L"Testing RTL centered at 1.0 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::RightToLeft;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            // --- Switch to a different plateau scale
            // 1.4 scale tests
            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.4f);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Testing LTR centered at 1.4 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::LeftToRight;
                // Use a width less than size.Width which will default to centering
                mainGrid->Width = 200;
                stackPanel->Width = 200;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

            LOG_OUTPUT(L"Testing RTL centered at 1.4 scale");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::RightToLeft;
                VERIFY_IS_TRUE(HitTest(button1));
                VERIFY_IS_TRUE(HitTest(button2));
                VERIFY_IS_TRUE(HitTest(button3));
                VERIFY_IS_TRUE(HitTest(button4));
            });

        }

        //------------------------------------------------------------------------
        // Test case: Test fuzzy hit-testing for touch in LTR/RTL.
        //------------------------------------------------------------------------
        void BasicPointerTests::TouchFuzzyHitTest()
        {
            TestCleanupWrapper cleanup;

            Grid^ mainGrid = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ button1 = nullptr;
            Button^ button2 = nullptr;
            Button^ button3 = nullptr;
            Button^ button4 = nullptr;

            std::shared_ptr<Event> button1ClickEvent = std::make_shared<Event>();
            std::shared_ptr<Event> button2ClickEvent = std::make_shared<Event>();
            std::shared_ptr<Event> button3ClickEvent = std::make_shared<Event>();
            std::shared_ptr<Event> button4ClickEvent = std::make_shared<Event>();

            auto button1ClickRegistration = CreateSafeEventRegistration(Button, Click);
            auto button2ClickRegistration = CreateSafeEventRegistration(Button, Click);
            auto button3ClickRegistration = CreateSafeEventRegistration(Button, Click);
            auto button4ClickRegistration = CreateSafeEventRegistration(Button, Click);

            int button1ClickCount = 0;
            int button2ClickCount = 0;
            int button3ClickCount = 0;
            int button4ClickCount = 0;

            RunOnUIThread([&]()
            {
                mainGrid = ref new Grid();

                stackPanel = ref new StackPanel();
                stackPanel->HorizontalAlignment = HorizontalAlignment::Stretch;
                stackPanel->VerticalAlignment = VerticalAlignment::Stretch;
                stackPanel->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(stackPanel);

                button1 = ref new Button();
                button1->Margin = xaml::Thickness({0, 40, 0, 0});
                button1->Content = "First Button";
                stackPanel->Children->Append(button1);

                button2 = ref new Button();
                button2->Content = "Second Button";
                stackPanel->Children->Append(button2);

                button3 = ref new Button();
                button3->Content = "Third Button";
                stackPanel->Children->Append(button3);

                button4 = ref new Button();
                button4->Content = "Fourth Button";
                stackPanel->Children->Append(button4);

                button1ClickRegistration.Attach(button1,
                    ref new RoutedEventHandler([button1ClickEvent, &button1ClickCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button1 Click event");
                    button1ClickCount++;
                    button1ClickEvent->Set();
                }));

                button2ClickRegistration.Attach(button2,
                    ref new RoutedEventHandler([button2ClickEvent, &button2ClickCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button2 Click event");
                    button2ClickCount++;
                    button2ClickEvent->Set();
                }));

                button3ClickRegistration.Attach(button3,
                    ref new RoutedEventHandler([button3ClickEvent, &button3ClickCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button3 Click event");
                    button3ClickCount++;
                    button3ClickEvent->Set();
                }));

                button4ClickRegistration.Attach(button4,
                    ref new RoutedEventHandler([button4ClickEvent, &button4ClickCount](Platform::Object^, RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Button4 Click event");
                    button4ClickCount++;
                    button4ClickEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            LOG_OUTPUT(L"Testing LTR");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::LeftToRight;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(button1);
            button1ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button2);
            button2ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button3);
            button3ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button4);
            button4ClickEvent->WaitForDefault();

            LOG_OUTPUT(L"Testing RTL");

            RunOnUIThread([&]()
            {
                mainGrid->FlowDirection = FlowDirection::RightToLeft;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(button1);
            button1ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button2);
            button2ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button3);
            button3ClickEvent->WaitForDefault();
            TestServices::InputHelper->Tap(button4);
            button4ClickEvent->WaitForDefault();

            // Each button got 2 clicks.
            VERIFY_ARE_EQUAL(button1ClickCount, 2);
            VERIFY_ARE_EQUAL(button2ClickCount, 2);
            VERIFY_ARE_EQUAL(button3ClickCount, 2);
            VERIFY_ARE_EQUAL(button4ClickCount, 2);
        }

        void BasicPointerTests::PointerRoutedAway()
        {
            Grid^ mainGrid = nullptr;
            StackPanel^ stackPanel = nullptr;
            Button^ button = nullptr;
            UINT pointerId = 0;
            std::shared_ptr<Event> buttonPointerCaptureLostEvent = std::make_shared<Event>();
            std::shared_ptr<Event> buttonPointerEnteredEvent = std::make_shared<Event>();
            wf::EventRegistrationToken buttonPointerCaptureLostToken = {};
            wf::EventRegistrationToken buttonPointerEnteredToken = {};
            XamlRoot^ xamlRoot = nullptr;

            RunOnUIThread([&]()
            {
                mainGrid = ref new Grid();

                stackPanel = ref new StackPanel();
                stackPanel->HorizontalAlignment = HorizontalAlignment::Stretch;
                stackPanel->VerticalAlignment = VerticalAlignment::Stretch;
                stackPanel->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                mainGrid->Children->Append(stackPanel);

                button = ref new Button();
                button->Margin = xaml::Thickness({0, 40, 0, 0});
                button->Content = "First Button";
                stackPanel->Children->Append(button);

                buttonPointerCaptureLostToken = button->PointerCaptureLost +=
                    ref new PointerEventHandler([buttonPointerCaptureLostEvent](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    buttonPointerCaptureLostEvent->Set();
                });

                buttonPointerEnteredToken = button->PointerEntered +=
                    ref new PointerEventHandler([&pointerId, buttonPointerEnteredEvent](Platform::Object^, PointerRoutedEventArgs^ args)
                {
                    // Get the pointer ID.
                    pointerId = args->Pointer->PointerId;
                    buttonPointerEnteredEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                xamlRoot = mainGrid->XamlRoot;
            });

            TestServices::InputHelper->DynamicPressCenter(button, 0, 0, PointerFinger::Finger1);
            buttonPointerEnteredEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->InjectWindowMessage(WM_POINTERROUTEDAWAY, pointerId, 0, xamlRoot);
            // Expect button fire PointCaptureLost event.
            buttonPointerCaptureLostEvent->WaitForDefault();
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
            TestServices::WindowHelper->WaitForIdle();
        }

        void BasicPointerTests::CanLeftMouseClick()
        {
            TestCleanupWrapper cleanup;

            xaml_controls::Button^ button = nullptr;

            auto clickEvent = std::make_shared<Event>(EventOptions::CaptureScreenOnTimeout, L"ClickEvent");
            auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

            RunOnUIThread([&]()
            {
                button = ref new xaml_controls::Button();
                button->Content = "ButtonContent";
                button->HorizontalAlignment = HorizontalAlignment::Left;
                button->VerticalAlignment = VerticalAlignment::Top;
                button->Margin = ThicknessHelper::FromLengths(0,25,0,0);
                clickRegistration.Attach(button, [clickEvent]() {
                    LOG_OUTPUT(L"Click event received");
                    clickEvent->Set();
                });
                TestServices::WindowHelper->WindowContent = button;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(button);

            clickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                button->HorizontalAlignment = HorizontalAlignment::Right;
                button->VerticalAlignment = VerticalAlignment::Top;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(button);
            clickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                button->HorizontalAlignment = HorizontalAlignment::Left;
                button->VerticalAlignment = VerticalAlignment::Bottom;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(button);
            clickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                button->HorizontalAlignment = HorizontalAlignment::Right;
                button->VerticalAlignment = VerticalAlignment::Bottom;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(button);
            clickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                button->HorizontalAlignment = HorizontalAlignment::Center;
                button->VerticalAlignment = VerticalAlignment::Center;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->LeftMouseClick(button);
            clickEvent->WaitForDefault();
        }

        ref class CustomStackPanel sealed : public StackPanel
        {
        public:
            void SetProtectedCursor()
            {
                ProtectedCursor = Microsoft::UI::Input::InputSystemCursor::Create(Microsoft::UI::Input::InputSystemCursorShape::Hand);
            }
        };

        void BasicPointerTests::ProtectedCursorOnNonLiveElement()
        {
            Grid^ mainGrid = nullptr;
            CustomStackPanel^ stackPanel = nullptr;

            RunOnUIThread([&]()
            {
                mainGrid = ref new Grid();

                // Create a stack panel that won't be part of the live elements
                stackPanel = ref new CustomStackPanel();
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Set ProtectedCursor on non-live element
                stackPanel->SetProtectedCursor();
            });
            // The test passes if it doesn't crash
        }

    } } }
} } } }
