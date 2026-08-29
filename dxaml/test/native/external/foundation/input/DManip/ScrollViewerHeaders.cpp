// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollViewerHeaders.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TraceConsumerSession.h>
#include <MUX-ETWEvents.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool ScrollViewerHeaders::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ScrollViewerHeaders::ClassCleanup()
        {
            return true;
        }

        bool ScrollViewerHeaders::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        ScrollViewer^ ScrollViewerHeaders::SetupUI(
            _In_ std::shared_ptr<Event>& viewChangingEvent,
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            ScrollViewer^ sv = nullptr;
            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

            RunOnUIThread([&]()
            {
                sv = ref new ScrollViewer();
                sv->Width = 300;
                sv->Height = 300;
                sv->HorizontalAlignment = HorizontalAlignment::Center;
                sv->VerticalAlignment = VerticalAlignment::Center;
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->HorizontalScrollMode = ScrollMode::Enabled;
                sv->VerticalScrollMode = ScrollMode::Enabled;
                sv->ZoomMode = ZoomMode::Enabled;
                sv->MinZoomFactor = 0.5f;
                sv->MaxZoomFactor = 2.0f;
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;
                sv->HorizontalSnapPointsType = SnapPointsType::None;
                sv->VerticalSnapPointsType = SnapPointsType::None;
                sv->IsVerticalRailEnabled = false;
                sv->IsHorizontalRailEnabled = false;

                Shapes::Rectangle^ rect = ref new Shapes::Rectangle();
                rect->HorizontalAlignment = HorizontalAlignment::Left;
                rect->VerticalAlignment = VerticalAlignment::Top;
                rect->Width = 600;
                rect->Height = 600;
                SolidColorBrush^ redBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->Fill = redBrush;
                sv->Content = rect;

                Shapes::Rectangle^ leftHeader = ref new Shapes::Rectangle();
                leftHeader->Width = 40;
                leftHeader->Height = 300;
                SolidColorBrush^ yellowBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Yellow);
                leftHeader->Fill = yellowBrush;
                sv->LeftHeader = leftHeader;

                Shapes::Rectangle^ topHeader = ref new Shapes::Rectangle();
                topHeader->Width = 300;
                topHeader->Height = 40;
                SolidColorBrush^ greenBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Green);
                topHeader->Fill = greenBrush;
                sv->TopHeader = topHeader;

                Shapes::Rectangle^ topLeftHeader = ref new Shapes::Rectangle();
                topLeftHeader->Width = 40;
                topLeftHeader->Height = 40;
                SolidColorBrush^ purpleBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Purple);
                topLeftHeader->Fill = purpleBrush;
                sv->TopLeftHeader = topLeftHeader;

                VERIFY_IS_TRUE(sv->HorizontalOffset == 0.0);
                VERIFY_IS_TRUE(sv->VerticalOffset == 0.0);
                VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);

                loadedRegistration.Attach(sv, ref new xaml::RoutedEventHandler(
                    [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    spHasLoadedEvent->Set();
                }));

                viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                {
                    if (!args->IsIntermediate)
                    {
                        viewChangedEvent->Set();
                    }
                }));

                viewChangingRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
                    [viewChangingEvent](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging, NextView: %f, %f, %f, FinalView: %f, %f, %f, IsInertial: %d",
                        args->NextView->HorizontalOffset,
                        args->NextView->VerticalOffset,
                        args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset,
                        args->FinalView->VerticalOffset,
                        args->FinalView->ZoomFactor,
                        args->IsInertial);
                    ScrollViewer^ scrollViewer = dynamic_cast<ScrollViewer^>(sender);
                    if (!scrollViewer->IsScrollInertiaEnabled && !scrollViewer->IsZoomInertiaEnabled)
                    {
                        VERIFY_IS_FALSE(args->IsInertial);
                    }
                    viewChangingEvent->Set();
                }));

                Grid^ mainGrid = ref new Grid();
                mainGrid->Children->Append(sv);
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            spHasLoadedEvent->WaitForDefault();
            return sv;
        }

        void ScrollViewerHeaders::HeadersInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as after scenario 1
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            // Scenario 3:  Remove the content from the XAML tree.  This will tear down a portion of the DComp visual tree.
            LOG_OUTPUT(L"Removing Content from ScrollViewer");
            RunOnUIThread([&]()
            {
                sv->Content = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");
        }

        void ScrollViewerHeaders::HeadersPanInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, -125 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after horizontal pan.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);

                VERIFY_IS_TRUE(sv->HorizontalOffset == 125.0);
                VERIFY_IS_TRUE(sv->VerticalOffset == 0.0);
                VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);
            });

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            LOG_OUTPUT(L"Launching vertical pan operation.");

            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -125 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after horizontal pan.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);

                VERIFY_IS_TRUE(sv->HorizontalOffset == 125.0);
                VERIFY_IS_TRUE(sv->VerticalOffset == 125.0);
                VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);
            });

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
        }

        void ScrollViewerHeaders::HeadersOverpanInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            LOG_OUTPUT(L"Launching vertical overpan operation.");
            TestServices::InputHelper->DynamicPressCenter(sv, 0, 0, PointerFinger::Finger1);
            TestServices::InputHelper->DynamicPressCenter(sv, 0, 100, PointerFinger::Finger1);
            TestServices::InputHelper->DynamicPressCenter(sv, -50, 100, PointerFinger::Finger1);

            viewChangingEvent->WaitForDefault();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
        }

        void ScrollViewerHeaders::HeadersZoomInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching zoom operation.");
            TestServices::InputHelper->ZoomInToEdges(sv, 130 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Horizontal, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after horizontal pan.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);

                VERIFY_IS_TRUE(sv->HorizontalOffset == 150.0);
                VERIFY_IS_TRUE(sv->VerticalOffset == 150.0);
                VERIFY_IS_TRUE(sv->ZoomFactor == 2.0f);
            });

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void ScrollViewerHeaders::HeadersFractionalZoomInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Launching offset changes with ChangeView operation.");
                sv->ChangeView(10.25, 10.75, nullptr, true);
            });

            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Launching zoom factor change with ChangeView operation.");
                sv->ChangeView(nullptr, nullptr, 1.92101908f, true);
            });

            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);
            });

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void ScrollViewerHeaders::ChangeViewInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            {
                TraceConsumerSession session(WINDOWS_UI_XAML_ETW_PROVIDER);
                TraceConsumer::EnableTracingByEventId(CreateDManipSharedTransformInfo_value);
                TraceConsumer::EnableTracingByEventId(ReleaseDManipSharedTransformInfo_value);

                LOG_OUTPUT(L"Launching ChangeView operation.");
                RunOnUIThread([&]()
                {
                    sv->ChangeView(150.0, 150.0, nullptr, true);
                });
                viewChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after horizontal pan.",
                        sv->HorizontalOffset,
                        sv->VerticalOffset,
                        sv->ZoomFactor);

                    VERIFY_IS_TRUE(sv->HorizontalOffset == 150.0);
                    VERIFY_IS_TRUE(sv->VerticalOffset == 150.0);
                    VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);
                });

                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

                session.Stop();
                VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(CreateDManipSharedTransformInfo_value, 4));
                VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ReleaseDManipSharedTransformInfo_value, 4));
            }
        }

        void ScrollViewerHeaders::ChangeViewWithAnimationInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangingEvent = std::make_shared<Event>();
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangingEvent, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            {
                TraceConsumerSession session(WINDOWS_UI_XAML_ETW_PROVIDER);
                TraceConsumer::EnableTracingByEventId(CreateDManipSharedTransformInfo_value);
                TraceConsumer::EnableTracingByEventId(ReleaseDManipSharedTransformInfo_value);

                LOG_OUTPUT(L"Launching ChangeView operation.");
                RunOnUIThread([&]()
                {
                    sv->IsZoomInertiaEnabled = true;
                    sv->ChangeView(150.0, 150.0, nullptr, false);
                });
                viewChangedEvent->WaitForDefault();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after horizontal pan.",
                        sv->HorizontalOffset,
                        sv->VerticalOffset,
                        sv->ZoomFactor);

                    VERIFY_IS_TRUE(sv->HorizontalOffset == 150.0);
                    VERIFY_IS_TRUE(sv->VerticalOffset == 150.0);
                    VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);
                });

                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

                session.Stop();
                VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(CreateDManipSharedTransformInfo_value, 0));
                VERIFY_NO_THROW(TraceConsumer::VerifyEventTraced(ReleaseDManipSharedTransformInfo_value, 0));
            }
        }

        void ScrollViewerHeaders::HeadersPan()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            HeadersPanInternal();
        }

        void ScrollViewerHeaders::HeadersOverpanWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            HeadersOverpanInternal();
        }

        void ScrollViewerHeaders::HeadersZoom()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            HeadersZoomInternal();
        }

        void ScrollViewerHeaders::ChangeViewWithAnimation()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            ChangeViewWithAnimationInternal();
        }

        void ScrollViewerHeaders::HeadersWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            HeadersInternal();
        }

        void ScrollViewerHeaders::HeadersFractionalZoomWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            HeadersFractionalZoomInternal();
        }

        void ScrollViewerHeaders::ChangeViewWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            ChangeViewInternal();
        }

    } } }
} } } }
