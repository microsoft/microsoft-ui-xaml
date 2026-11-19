// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AutoScrollBehaviorTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <ControlHelper.h>
#include <ppltasks.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool AutoScrollBehaviorTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool AutoScrollBehaviorTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool AutoScrollBehaviorTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ AutoScrollBehaviorTests::GetPathToFiles() const
        {
            // Get the deployment directory, and then append our test's directory to the end
            auto deploymentDir = GetTestDeploymentDir();
            return ref new Platform::String(deploymentDir + L"resources\\native\\external\\foundation\\input\\dmanip\\");
        }

        // Holds and drags the first item in a horizontal GridView and drops
        // it at the end of the grid via auto-scroll. Simulates a touch-based reordering.
        void AutoScrollBehaviorTests::AutoScrollHorizontallyInGridView()
        {
            AutoScrollHorizontallyInGridView(false /*alsoScrollWithMouseWheel*/);
        }

        // Holds and drags the first item in a horizontal GridView and while auto-scrolling, triggers
        // a mouse-wheel-based scroll. Then drops item at the end of the grid via auto-scroll.
        void AutoScrollBehaviorTests::MouseWheelScrollDuringTouchAutoScroll()
        {
            AutoScrollHorizontallyInGridView(true /*alsoScrollWithMouseWheel*/);
        }

        void AutoScrollBehaviorTests::AutoScrollHorizontallyInGridView(bool alsoScrollWithMouseWheel)
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            bool hasScrolledWithMouseWheel = false;
            bool hasAutoScrolled = false;
            int dragStartingCount = 0;
            int dragOverCount = 0;
            bool hasHeldItem = false;
            auto dropResult = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::None;
            auto dropCompletedEvent = std::make_shared<Event>();

            auto dragStartingRegistration = CreateSafeEventRegistration(xaml::UIElement, DragStarting);
            auto dragOverRegistration = CreateSafeEventRegistration(xaml::UIElement, DragOver);
            auto dropCompletedRegistration = CreateSafeEventRegistration(xaml::UIElement, DropCompleted);
            auto holdingRegistration = CreateSafeEventRegistration(xaml::UIElement, Holding);

            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            xaml_controls::GridViewItem^ item = nullptr;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml_controls::GridView^ gridView;
            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"AutoScrollBehaviorInGridView.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootGrid,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                gridView = safe_cast<xaml_controls::GridView^>(rootGrid->FindName(L"gridView"));
                VERIFY_IS_NOT_NULL(gridView);

            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                item = safe_cast<xaml_controls::GridViewItem^>(gridView->ContainerFromIndex(0));
                VERIFY_IS_NOT_NULL(item);
                // A setup tap can interfere with recognizing the following touch-and-hold.
                gridView->SelectedIndex = 0;
                holdingRegistration.Attach(item, ref new xaml_input::HoldingEventHandler(
                    [&hasHeldItem](Platform::Object^, xaml_input::HoldingRoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"Item Holding: state=%d.", args->HoldingState);
                    if (args->HoldingState == Microsoft::UI::Input::HoldingState::Started)
                    {
                        hasHeldItem = true;
                    }
                }));
                dragStartingRegistration.Attach(item, ref new wf::TypedEventHandler<xaml::UIElement^, xaml::DragStartingEventArgs^>(
                    [&dragStartingCount](xaml::UIElement^, xaml::DragStartingEventArgs^ args)
                {
                    ++dragStartingCount;
                    LOG_OUTPUT(L"Item DragStarting: cancel=%d.", args->Cancel);
                }));
                dropCompletedRegistration.Attach(item, ref new wf::TypedEventHandler<xaml::UIElement^, xaml::DropCompletedEventArgs^>(
                    [&dropResult, dropCompletedEvent](xaml::UIElement^, xaml::DropCompletedEventArgs^ args)
                {
                    dropResult = args->DropResult;
                    LOG_OUTPUT(L"Item DropCompleted: result=%d.", args->DropResult);
                    dropCompletedEvent->Set();
                }));
                dragOverRegistration.Attach(gridView, ref new xaml::DragEventHandler(
                    [&dragOverCount](Platform::Object^, xaml::DragEventArgs^)
                {
                    ++dragOverCount;
                }));
            });

            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            RunOnUIThread([&]()
            {
                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(gridView, L"ScrollViewer"));
                VERIFY_IS_NOT_NULL(scrollViewer);

                viewChangedRegistration.Attach(
                    scrollViewer,
                    ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [&hasScrolledWithMouseWheel, &hasAutoScrolled, &dragStartingCount, alsoScrollWithMouseWheel, scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged event raised with ScrollViewer.HorizontalOffset = %f.", scrollViewer->HorizontalOffset);
                    if (args->IsIntermediate == false)
                    {
                        if (scrollViewer->HorizontalOffset >= 14.0)
                        {
                            LOG_OUTPUT(L"Final ViewChanged event raised.");
                        }
                    }
                    else if (dragStartingCount > 0 && scrollViewer->HorizontalOffset >= 4.0)
                    {
                        hasAutoScrolled = true;
                        if (!hasScrolledWithMouseWheel && alsoScrollWithMouseWheel)
                        {
                            hasScrolledWithMouseWheel = true;
                            LOG_OUTPUT(L"Launching horizontal scroll operation with the mouse wheel during the auto-scroll.");
                            TestServices::InputHelper->ScrollMouseWheel(scrollViewer, -1 /* numberOfWheelClicks */);
                        }
                    }
                }));
            });

            LOG_OUTPUT(L"Hold and drag first GridView item to opposite edge.");
            // Swipe selection is disabled in the .xaml so press-and-hold initiates reordering.
            TestServices::InputHelper->PressHoldAndPanFromCenter(item,
                280, 150, 0.02, 2000);

            dropCompletedEvent->WaitForDefault();

            LOG_OUTPUT(L"Waiting for the end of the auto-scroll.");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(dragStartingCount, 1);
                VERIFY_IS_TRUE(hasHeldItem);
                VERIFY_IS_GREATER_THAN(dragOverCount, 0);
                VERIFY_IS_TRUE(hasAutoScrolled);
                VERIFY_ARE_EQUAL(alsoScrollWithMouseWheel, hasScrolledWithMouseWheel);
                VERIFY_ARE_EQUAL(dropResult, ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Move);
                LOG_OUTPUT(L"Final view of ScrollViewer is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
                VERIFY_IS_TRUE(scrollViewer->HorizontalOffset >= 14.0);
                VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 0.0);
                VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            });
        }

        // Tears out the first item in a vertical ListView and drops
        // it at the end of the list via auto-scroll. Simulates a mouse-based reordering.
        void AutoScrollBehaviorTests::AutoScrollVerticallyInListView()
        {
            AutoScrollVerticallyInListView(false /*alsoScrollWithMouseWheel*/);
        }

        // Tears out the first item in a vertical ListView and while auto-scrolling triggers a
        // mouse-wheel-based scroll, then drops item at the location where the mouse-wheel scroll ends.
        void AutoScrollBehaviorTests::MouseWheelScrollDuringMouseAutoScroll()
        {
            AutoScrollVerticallyInListView(true /*alsoScrollWithMouseWheel*/);
        }

        void AutoScrollBehaviorTests::AutoScrollVerticallyInListView(bool alsoScrollWithMouseWheel)
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(300, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto viewChangedEvent = std::make_shared<Event>();
            auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

            auto rootLoadedEvent = std::make_shared<Event>();
            auto rootLoadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            bool hasScrolledWithMouseWheel = false;

            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml_controls::ListViewItem^ item = nullptr;
            xaml_controls::ListView^ listView = nullptr;
            xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetPathToFiles() + L"AutoScrollBehaviorInListView.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;
                LOG_OUTPUT(L"Listening to root FrameworkElement.Loaded.");
                rootLoadedRegistration.Attach(
                    rootGrid,
                    ref new xaml::RoutedEventHandler(
                    [rootLoadedEvent](Platform::Object^, xaml::IRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Root FE Loaded handler.");
                    rootLoadedEvent->Set();
                }));

                listView = safe_cast<xaml_controls::ListView^>(rootGrid->FindName(L"listView"));
                VERIFY_IS_NOT_NULL(listView);
            });

            LOG_OUTPUT(L"Waiting for root FE Loaded event.");
            rootLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                item = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
                VERIFY_IS_NOT_NULL(item);

                scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listView, L"ScrollViewer"));
                VERIFY_IS_NOT_NULL(scrollViewer);

                viewChangedRegistration.Attach(
                    scrollViewer,
                    ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [&hasScrolledWithMouseWheel, alsoScrollWithMouseWheel, viewChangedEvent, scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged event raised with ScrollViewer.VerticalOffset = %f.", scrollViewer->VerticalOffset);
                    if (args->IsIntermediate == false)
                    {
                        LOG_OUTPUT(L"Final ViewChanged event raised.");
                        viewChangedEvent->Set();
                    }
                    else if (!hasScrolledWithMouseWheel && alsoScrollWithMouseWheel && scrollViewer->VerticalOffset >= 100.0)
                    {
                        hasScrolledWithMouseWheel = true;
                        LOG_OUTPUT(L"Launching vertical scroll operation with the mouse wheel during the auto-scroll.");
                        TestServices::InputHelper->ScrollMouseWheel(scrollViewer, -1 /* numberOfWheelClicks */);
                    }
                }));
            });

            LOG_OUTPUT(L"Mouse moved over first ListView item.");
            TestServices::InputHelper->MoveMouse(item);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            LOG_OUTPUT(L"Mouse moved a bit to the left.");
            TestServices::InputHelper->MoveMouse(wf::Point(50.0f, 47.0f));
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            LOG_OUTPUT(L"Left mouse button down over first ListView item.");
            TestServices::InputHelper->MouseButtonDown(wf::Point(50.0f, 47.0f), MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            LOG_OUTPUT(L"Tear out first ListView item by pulling it toward the right.");
            TestServices::InputHelper->MouseDrag(wf::Point(50.0f, 47.0f), wf::Point(110.0f, 47.0f), MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MouseDrag(wf::Point(110.0f, 47.0f), wf::Point(170.0f, 47.0f), MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::InputHelper->MouseDrag(wf::Point(170.0f, 47.0f), wf::Point(230.0f, 47.0f), MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->SynchronouslyTickUIThread(2);

            LOG_OUTPUT(L"Drag first ListView item to the bottom edge.");
            viewChangedEvent->Reset();
            TestServices::InputHelper->MouseDrag(wf::Point(230.0f, 47.0f), wf::Point(230.0f, 370.0f), MouseButton::Left);

            LOG_OUTPUT(L"Waiting for the end of the auto-scroll.");
            viewChangedEvent->WaitForDefault();

            LOG_OUTPUT(L"Left mouse button up from first ListView item.");
            TestServices::InputHelper->MouseButtonUp(wf::Point(230.0f, 370.0f), MouseButton::Left);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final view of ScrollViewer is (x, y, z) = (%f, %f, %f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
                VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
                // When alsoScrollWithMouseWheel is True, the dragged item is inserted at the location where the mouse wheel input stops. Otherwise it's inserted at the end of the list.
                VERIFY_IS_GREATER_THAN(alsoScrollWithMouseWheel ? 200.0 : 1400.0, scrollViewer->VerticalOffset);
                VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, alsoScrollWithMouseWheel ? 100.0 : 1300.0);
                VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            });
        }
    } } }
} } } }
