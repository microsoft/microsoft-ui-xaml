// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "GestureTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "FocusTestHelper.h"

using namespace Microsoft::UI::Xaml::Input;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Gestures {

        bool GestureTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool GestureTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool GestureTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ GestureTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\foundation\\input\\Gestures\\");
        }

        //------------------------------------------------------------------------
        // Test case: Validates leaving the visual tree while handling OnTapped 
        //            event and having a manipulatable parent
        //------------------------------------------------------------------------
        void GestureTests::LeaveTreeInTappedHandler()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
            auto btnTappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Tapped);

            xaml_controls::Grid^ grd1 = nullptr;
            xaml_controls::Button^ btn1 = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"GestureTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                grd1 = dynamic_cast<xaml_controls::Grid^>(rootStackPanel->FindName(L"grd1"));
                VERIFY_IS_NOT_NULL(grd1);

                btn1 = dynamic_cast<xaml_controls::Button^>(rootStackPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);
                VERIFY_IS_TRUE(btn1->IsTapEnabled);

                btnTappedRegistration.Attach(btn1, ref new xaml_input::TappedEventHandler(
                    [gestureCompletedEvent, grd1](Platform::Object^, TappedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Tapped event raised.");
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);
                    gestureCompletedEvent->Set();

                    // Remove the Canvas child
                    grd1->Children->RemoveAt(0);
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tapping the Button.");
            TestServices::InputHelper->Tap(btn1);
            gestureCompletedEvent->WaitForDefault();
        }

        //------------------------------------------------------------------------
        // Test case: Validates leaving and re-entering the visual tree while handling
        //            OnTapped event and having a manipulatable parent
        //------------------------------------------------------------------------
        void GestureTests::LeaveAndReenterTreeInTappedHandler()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> gestureCompletedEvent = std::make_shared<Event>();
            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            auto btnTappedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Tapped);
            auto cnvManipulationCompletedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, ManipulationCompleted);

            xaml_controls::Grid^ grd1 = nullptr;
            xaml_controls::Grid^ grd2 = nullptr;
            xaml_controls::Canvas^ cnv1 = nullptr;
            xaml_controls::Button^ btn1 = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"GestureTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                grd1 = dynamic_cast<xaml_controls::Grid^>(rootStackPanel->FindName(L"grd1"));
                VERIFY_IS_NOT_NULL(grd1);

                grd2 = dynamic_cast<xaml_controls::Grid^>(rootStackPanel->FindName(L"grd2"));
                VERIFY_IS_NOT_NULL(grd2);

                cnv1 = dynamic_cast<xaml_controls::Canvas^>(rootStackPanel->FindName(L"cnv1"));
                VERIFY_IS_NOT_NULL(cnv1);

                btn1 = dynamic_cast<xaml_controls::Button^>(rootStackPanel->FindName(L"btn1"));
                VERIFY_IS_NOT_NULL(btn1);
                VERIFY_IS_TRUE(btn1->IsTapEnabled);

                btnTappedRegistration.Attach(btn1, ref new xaml_input::TappedEventHandler(
                    [gestureCompletedEvent, grd1, grd2, cnv1](Platform::Object^, TappedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Tapped event raised.");
                    VERIFY_ARE_EQUAL(args->PointerDeviceType, Microsoft::UI::Input::PointerDeviceType::Touch);

                    if (grd1->Children->Size > 0)
                    {
                        // Remove the Canvas child
                        grd1->Children->RemoveAt(0);

                        // Re-enter the tree as a child of grd2
                        grd2->Children->Append(cnv1);
                    }

                    gestureCompletedEvent->Set();
                }));

                cnvManipulationCompletedRegistration.Attach(cnv1, ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    manipulationCompletedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            FocusTestHelper::EnsureFocus(btn1, FocusState::Keyboard);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tapping the Button.");
            TestServices::InputHelper->Tap(btn1);
            TestServices::WindowHelper->WaitForIdle();
            gestureCompletedEvent->WaitForDefault();
            gestureCompletedEvent->Reset();

            // If the two taps are too close together, they get coalesced into a double tap, and the second tap doesn't
            // register. Pump a few frames to space them apart.
            TestServices::WindowHelper->SynchronouslyTickUIThread(20);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tapping the Button again.");
            TestServices::InputHelper->Tap(btn1);
            TestServices::WindowHelper->WaitForIdle();
            gestureCompletedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Injecting horizontal pan for the Canvas.");
            TestServices::InputHelper->PanFromCenter(cnv1, 100 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
            TestServices::WindowHelper->WaitForIdle();
            manipulationCompletedEvent->WaitForDefault();
        }

        //------------------------------------------------------------------------
        // Test case: Validates leaving and re-entering the visual tree while
        //            handling a ManipulationDelta event
        //------------------------------------------------------------------------
        void GestureTests::LeaveAndReenterTreeInManipulationDeltaHandler()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> manipulationCompletedEvent = std::make_shared<Event>();
            auto cnvManipulationDeltaRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, ManipulationDelta);
            auto cnvManipulationCompletedRegistration = CreateSafeEventRegistration(xaml_controls::Canvas, ManipulationCompleted);

            xaml_controls::Grid^ grd1 = nullptr;
            xaml_controls::Grid^ grd2 = nullptr;
            xaml_controls::Canvas^ cnv1 = nullptr;
            xaml_controls::StackPanel^ rootStackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"GestureTests.xaml"));
            VERIFY_IS_NOT_NULL(rootStackPanel);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootStackPanel;

                grd1 = dynamic_cast<xaml_controls::Grid^>(rootStackPanel->FindName(L"grd1"));
                VERIFY_IS_NOT_NULL(grd1);

                grd2 = dynamic_cast<xaml_controls::Grid^>(rootStackPanel->FindName(L"grd2"));
                VERIFY_IS_NOT_NULL(grd2);

                cnv1 = dynamic_cast<xaml_controls::Canvas^>(rootStackPanel->FindName(L"cnv1"));
                VERIFY_IS_NOT_NULL(cnv1);

                cnvManipulationDeltaRegistration.Attach(cnv1, ref new xaml_input::ManipulationDeltaEventHandler(
                    [grd1, grd2, cnv1](Platform::Object^, ManipulationDeltaRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationDelta, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);
                    if (args->Cumulative.Translation.X > 20 && grd1->Children->Size > 0)
                    {
                        LOG_OUTPUT(L"Reparenting the Canvas.");
                        grd1->Children->RemoveAt(0);
                        grd2->Children->Append(cnv1);
                    }
                }));

                cnvManipulationCompletedRegistration.Attach(cnv1, ref new xaml_input::ManipulationCompletedEventHandler(
                    [manipulationCompletedEvent](Platform::Object^, ManipulationCompletedRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ManipulationCompleted, Cumulative.Translation.X: %f, Position: (%f, %f), IsInertial: %d",
                        args->Cumulative.Translation.X, args->Position.X, args->Position.Y, args->IsInertial);
                    VERIFY_IS_TRUE(args->Cumulative.Translation.X > 0);
                    manipulationCompletedEvent->Set();
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Injecting horizontal pan for the Canvas.");
            TestServices::InputHelper->PanFromCenter(cnv1, 50 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);

            // Tapping the reparented canvas to trigger its ManipulationCompleted event.
            // This can be removed if we decide to fix this issue.
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Tapping the Canvas.");
            TestServices::InputHelper->Tap(cnv1);

            manipulationCompletedEvent->WaitForDefault();
        }

    } } }
} } } }
