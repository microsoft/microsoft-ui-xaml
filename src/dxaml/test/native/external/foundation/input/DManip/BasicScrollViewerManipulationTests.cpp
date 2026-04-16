// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BasicScrollViewerManipulationTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
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

        bool BasicScrollViewerManipulationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool BasicScrollViewerManipulationTests::ClassCleanup()
        {
            return true;
        }

        bool BasicScrollViewerManipulationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool BasicScrollViewerManipulationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        ScrollViewer^ BasicScrollViewerManipulationTests::SetupUI(
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration,
            bool withContentSmallerThanViewport,
            bool withInertia)
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

                StackPanel^ svChild = ref new StackPanel();
                svChild->Orientation = Orientation::Horizontal;
                sv->Content = svChild;

                // Fill the ScrollViewer with rectangles. Content size is 1200x600
                // in a 300x300 ScrollViewer when withContentSmallerThanViewport==False,
                // or 240x250 when withContentSmallerThanViewport==True.
                for (int i = 0; i < 12; i++)
                {
                    Microsoft::UI::Xaml::Shapes::Rectangle^ rect =
                        ref new Microsoft::UI::Xaml::Shapes::Rectangle();
                    SolidColorBrush^ blueBrush =
                        ref new SolidColorBrush(Microsoft::UI::Colors::Blue);
                    SolidColorBrush^ whiteBrush =
                        ref new SolidColorBrush(Microsoft::UI::Colors::White);
                    SolidColorBrush^ redBrush =
                        ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                    switch (i % 3)
                    {
                    case 0:
                        rect->Fill = redBrush;
                        break;
                    case 1:
                        rect->Fill = blueBrush;
                        break;
                    case 2:
                        rect->Fill = whiteBrush;
                        break;
                    }
                    if (withContentSmallerThanViewport)
                    {
                        rect->Width = 20;
                        rect->Height = 250;
                    }
                    else
                    {
                        rect->Width = 100;
                        rect->Height = 600;
                    }
                    svChild->Children->Append(rect);
                }

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
                    [withInertia](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
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
                    if (!scrollViewer->IsScrollInertiaEnabled && !scrollViewer->IsZoomInertiaEnabled && !withInertia)
                    {
                        VERIFY_IS_FALSE(args->IsInertial);
                    }
                }));

                Grid^ mainGrid = ref new Grid();
                mainGrid->Children->Append(sv);
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            spHasLoadedEvent->WaitForDefault();
            return sv;
        }

        //------------------------------------------------------------------------
        // Test case: Basic scenarios to verify the correct DComp visual tree is generated
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::BasicsExInternal(bool addRenderTransform, bool changeView)
        {
            ScrollViewer^ sv = nullptr;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();

            // Scenario 1:  Create a ScrollViewer with a Rectangle child
            LOG_OUTPUT(L"Setting up basic ScrollViewer");
            RunOnUIThread([&]()
            {
                sv = ref new ScrollViewer();
                sv->Width = 300;
                sv->Height = 300;

                // Hide the scrollbars as these kick off animations that we don't want to affect the dumps
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->HorizontalScrollMode = ScrollMode::Enabled;
                sv->VerticalScrollMode = ScrollMode::Enabled;

                Shapes::Rectangle^ rect = ref new Shapes::Rectangle();
                rect->Width = 600;
                rect->Height = 600;
                if (addRenderTransform)
                {
                    xaml_media::ScaleTransform^ scale = ref new xaml_media::ScaleTransform();
                    scale->ScaleX = 0.25;
                    scale->ScaleY = 0.25;
                    rect->RenderTransform = scale;
                }

                if (changeView)
                {
                    viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                        [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                    {
                        if (!args->IsIntermediate)
                        {
                            viewChangedEvent->Set();
                        }
                    }));
                }

                SolidColorBrush^ redBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->Fill = redBrush;
                sv->Content = rect;

                Grid^ mainGrid = ref new Grid();
                mainGrid->Children->Append(sv);
                TestServices::WindowHelper->WindowContent = mainGrid;

                loadedRegistration.Attach(sv, ref new xaml::RoutedEventHandler(
                    [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    spHasLoadedEvent->Set();
                }));
            });

            spHasLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            if (changeView)
            {
                RunOnUIThread([&]()
                {
                    sv->ChangeView(100.0, 50.0, 1.0f, true /*disableAnimation*/);
                });

                viewChangedEvent->WaitForDefault();
            }

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as after scenario 1
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Scenario 3:  Remove the Rectangle from the XAML tree.  This will tear down a portion of the DComp visual tree.
            LOG_OUTPUT(L"Removing Content from ScrollViewer");
            RunOnUIThread([&]()
            {
                sv->Content = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
        }

        //------------------------------------------------------------------------
        // Test case: Pan a ScrollViewer horizontally without inertia.
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::PanNoInertiaExInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

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

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        //------------------------------------------------------------------------
        // Test case: Pan a ScrollViewer vertically with inertia.
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::PanInertiaExInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            RunOnUIThread([&]()
            {
                sv->IsScrollInertiaEnabled = true;
            });

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -100 /*relY*/, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after vertical pan.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);
                // Allowing a range of offsets to account for random timing fluctuations.
                VERIFY_IS_TRUE(sv->HorizontalOffset == 0.0);
                VERIFY_IS_TRUE(sv->VerticalOffset >= 110.0);
                VERIFY_IS_TRUE(sv->VerticalOffset <= 300.0);
                VERIFY_IS_TRUE(sv->ZoomFactor == 1.0f);
            });
        }

        //------------------------------------------------------------------------
        // Test case: Zoom-in a ScrollViewer without inertia.
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::ZoomInNoInertiaExInternal()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching zoom-in operation.");
            TestServices::InputHelper->ZoomInToEdges(sv, 130 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Horizontal, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after zoom in.",
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

        //------------------------------------------------------------------------
        // Test case: Zoom-out a ScrollViewer with inertia.
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::ZoomOutInertiaExInternal()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                sv->IsScrollInertiaEnabled = true;
                sv->IsZoomInertiaEnabled = true;

                LOG_OUTPUT(L"Centering Content.");
                sv->ChangeView(450.0, 150.0, 1.0f, true /*disableAnimation*/);
            });

            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching zoom-out operation.");
            TestServices::InputHelper->ZoomOutFromEdges(sv, 80 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Horizontal, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after zoom out.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);
                VERIFY_IS_TRUE(sv->HorizontalOffset > 130.0);
                VERIFY_IS_TRUE(sv->HorizontalOffset < 210.0);
                VERIFY_IS_TRUE(sv->VerticalOffset >= 0.0);
                VERIFY_IS_TRUE(sv->VerticalOffset <= 30.0);
                VERIFY_IS_TRUE(sv->ZoomFactor <= 0.6f);
                VERIFY_IS_TRUE(sv->ZoomFactor >= 0.5f);
            });
        }

        //------------------------------------------------------------------------
        // Test case: Validates layout of small ScrollViewer content with various alignments, with DManip-on-DComp enabled.
        //------------------------------------------------------------------------
        void BasicScrollViewerManipulationTests::AlignContentInternal()
        {
            ScrollViewer^ sv = nullptr;
            Shapes::Rectangle^ rect = nullptr;

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

            // Create a ScrollViewer with a larger Rectangle child
            LOG_OUTPUT(L"Setting up basic ScrollViewer with Rectangle content.");
            RunOnUIThread([&]()
            {
                sv = ref new ScrollViewer();
                sv->Width = 300;
                sv->Height = 300;
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;

                rect = ref new Shapes::Rectangle();
                rect->Width = 800;
                rect->Height = 800;
                rect->HorizontalAlignment = xaml::HorizontalAlignment::Left;
                rect->VerticalAlignment = xaml::VerticalAlignment::Top;
                SolidColorBrush^ redBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                rect->Fill = redBrush;
                sv->Content = rect;

                loadedRegistration.Attach(sv, ref new xaml::RoutedEventHandler(
                    [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    spHasLoadedEvent->Set();
                }));

                Grid^ mainGrid = ref new Grid();
                mainGrid->Children->Append(sv);
                TestServices::WindowHelper->WindowContent = mainGrid;
            });

            spHasLoadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1a").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Changing alignment to Center/Bottom.");
                rect->HorizontalAlignment = xaml::HorizontalAlignment::Center;
                rect->VerticalAlignment = xaml::VerticalAlignment::Bottom;
            });
            TestServices::WindowHelper->WaitForIdle();
            // The visual tree should be unchanged
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1a").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Resizing Rectangle smaller than ScrollViewer viewport.");
                rect->Width = 200;
                rect->Height = 200;
            });
            TestServices::WindowHelper->WaitForIdle();
            // Rectangle expected to move inside ScrollViewer
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2a").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Resizing Rectangle larger than ScrollViewer viewport.");
                rect->Width = 800;
                rect->Height = 800;
            });
            TestServices::WindowHelper->WaitForIdle();
            // Rectangle expected to move back inside ScrollViewer, at top/left corner
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1a").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Applying small zoom factor on Rectangle to make it smaller than ScrollViewer viewport.");
                sv->ZoomToFactor(0.25f);
            });
            TestServices::WindowHelper->WaitForIdle();
            // Rectangle expected to move inside ScrollViewer, at center/bottom location
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2b").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Applying small margins on Rectangle to keep it smaller than ScrollViewer viewport.");
                rect->Margin = xaml::ThicknessHelper::FromUniformLength(40);
            });
            TestServices::WindowHelper->WaitForIdle();
            // Rectangle still expected to move inside ScrollViewer, at center/bottom location
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2c").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Applying large margins on Rectangle to make it larger than ScrollViewer viewport.");
                rect->Margin = xaml::ThicknessHelper::FromUniformLength(300);
            });
            TestServices::WindowHelper->WaitForIdle();
            // Rectangle expected to move back inside ScrollViewer, at top/left corner
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1b").GetString());
        }

        void BasicScrollViewerManipulationTests::BasicsExWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            BasicsExInternal();
        }

        void BasicScrollViewerManipulationTests::BasicsEx2WUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            BasicsExInternal(true /*addRenderTransform*/, true/*changeView*/);
        }

        void BasicScrollViewerManipulationTests::PanNoInertiaExWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            PanNoInertiaExInternal();
        }

        void BasicScrollViewerManipulationTests::PanNoInertiaOptWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            ScrollViewer^ sv = SetupUI(viewChangedEvent, viewChangingRegistration, viewChangedRegistration, false, false);

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

            MockDComp::IMockDCompDevice^ mockDevice = TestServices::WindowHelper->MockDCompDevice;
            MockDComp::IMockDCompDevice2^ mockDevice2 = safe_cast<MockDComp::IMockDCompDevice2^>(mockDevice);

            RunOnUIThread([&]()
            {
                unsigned int startAnimationCount;
                mockDevice2->GetWUCStartAnimationCount(&startAnimationCount);
                LOG_OUTPUT(L"StartAnimation Count = %d", startAnimationCount);
                VERIFY_ARE_EQUAL(startAnimationCount, 6u);
            });
        }

        void BasicScrollViewerManipulationTests::PanInertiaExWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            PanInertiaExInternal();
        }

        void BasicScrollViewerManipulationTests::ZoomInNoInertiaExWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            ZoomInNoInertiaExInternal();
        }

        void BasicScrollViewerManipulationTests::ZoomOutInertiaExWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            ZoomOutInertiaExInternal();
        }

        void BasicScrollViewerManipulationTests::AlignContentWUC()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            AlignContentInternal();
        }

        void BasicScrollViewerManipulationTests::NoHitTestableContentInRSV()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            SkipRSVBackgroundScopeGuard skipRSVBackground;

            const auto& wh = TestServices::WindowHelper;
            const auto& u = TestServices::Utilities;

            Grid^ element;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Creating tree with no IsHitTestVisible=\"true\" element that renders content. Should not have a DManip hit test visual.");

                element = ref new Grid();
                element->Width = 100;
                element->Height = 100;
                element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                element->IsHitTestVisible = false;

                Grid^ elementParent = ref new Grid();
                elementParent->Children->Append(element);

                Grid^ elementGrandparent = ref new Grid();
                elementGrandparent->Children->Append(elementParent);

                Grid^ root = ref new Grid();
                root->Children->Append(elementGrandparent);

                wh->WindowContent = root;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoHit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Setting IsHitTestVisible=\"true\". Should have a DManip hit test visual.");
                element->IsHitTestVisible = true;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Hit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Setting IsHitTestVisible=\"false\". Should not have a DManip hit test visual.");
                element->IsHitTestVisible = false;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoHit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Resetting root to IsHitTestVisible=\"true\" element. Should have a DManip hit test visual.");

                Grid^ hitElement = ref new Grid();
                hitElement->Width = 100;
                hitElement->Height = 100;
                hitElement->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                wh->WindowContent = hitElement;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"Hit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Resetting root to IsHitTestVisible=\"false\" element. Should not have a DManip hit test visual.");

                Grid^ missElement = ref new Grid();
                missElement->Width = 100;
                missElement->Height = 100;
                missElement->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                missElement->IsHitTestVisible = false;
                wh->WindowContent = missElement;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoHit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Resetting root to no content. Should not have a DManip hit test visual.");

                Grid^ root4 = ref new Grid();
                wh->WindowContent = root4;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoC");
        }

        void BasicScrollViewerManipulationTests::NoHitTestableContentInSV()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            const auto& wh = TestServices::WindowHelper;
            const auto& u = TestServices::Utilities;

            Grid^ element;
            ScrollViewer^ scrollViewer;

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Creating ScrollViewer with no IsHitTestVisible=\"true\" content. Should have a DManip hit test visual, because of the backing Grid.");

                element = ref new Grid();
                element->Width = 100;
                element->Height = 100;
                element->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);
                element->IsHitTestVisible = false;

                Grid^ scrollViewerContent = ref new Grid();
                scrollViewerContent->Children->Append(element);

                scrollViewer = ref new ScrollViewer();
                scrollViewer->Content = scrollViewerContent;
                scrollViewer->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Green);

                Grid^ root = ref new Grid();
                root->Children->Append(scrollViewer);

                wh->WindowContent = root;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"BG-Hit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Removing the internal Background and BorderBrush. Should not have a DManip hit test visual anymore.");

                scrollViewer->Background = nullptr;
                scrollViewer->BorderBrush = nullptr;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoBG-NoHit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Setting IsHitTestVisible=\"true\". Should have a DManip hit test visual.");
                element->IsHitTestVisible = true;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoBG-Hit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Setting IsHitTestVisible=\"false\". Should not have a DManip hit test visual.");
                element->IsHitTestVisible = false;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoBG-NoHit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Setting IsHitTestVisible=\"true\". Should have a DManip hit test visual.");
                element->IsHitTestVisible = true;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoBG-Hit");

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"> Clearing ScrollView content. Should not have a DManip hit test visual anymore.");
                scrollViewer->Content = nullptr;
            });
            wh->WaitForIdle();
            u->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"NoBG-NoC-NoHit");
        }
    } } }
} } } }
