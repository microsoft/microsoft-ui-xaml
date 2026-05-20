// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListView.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
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

        bool ListViewTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ListViewTest::ClassCleanup()
        {
            return true;
        }

        bool ListViewTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ ListViewTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ScrollViewer^ ListViewTest::SetupUI(
            _In_ Platform::String^ filename,
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            ScrollViewer^ sv = nullptr;

            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                ListView^ listView = safe_cast<ListView^>(root->FindName(L"myListView"));
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(listView, 0));
                sv = safe_cast<ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;

                viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent, sv](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                        sv->HorizontalOffset, sv->VerticalOffset, sv->ZoomFactor, args->IsIntermediate);
                    if (!args->IsIntermediate)
                    {
                        viewChangedEvent->Set();
                    }
                }));

                viewChangingRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
                    [](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging, NextView: %f, %f, %f, FinalView: %f, %f, %f, IsInertial: %d",
                        args->NextView->HorizontalOffset,
                        args->NextView->VerticalOffset,
                        args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset,
                        args->FinalView->VerticalOffset,
                        args->FinalView->ZoomFactor,
                        args->IsInertial);
                }));
            });

            return sv;
        }

        void ListViewTest::Basics()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListView.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void ListViewTest::Pan()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListView.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -200 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void ListViewTest::Zoom()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListView.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching zoom operation.");

            TestServices::InputHelper->ZoomInToEdges(sv, 130 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 0.66 /*velocityFactor*/);

            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        // Zooms out a ListView with an inner VirtualizingStackPanel using the ctrl key and mouse wheel
        void ListViewTest::ZoomOutWithMouseWheel()
        {
            ZoomWithMouseWheel(false /*zoomIn*/);
        }

        // Zooms in a ListView with an inner VirtualizingStackPanel using the ctrl key and mouse wheel
        void ListViewTest::ZoomInWithMouseWheel()
        {
            ZoomWithMouseWheel(true /*zoomIn*/);
        }

        void ListViewTest::ZoomWithMouseWheel(bool zoomIn)
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListViewWithVSP.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                FrameworkElement^ root = safe_cast<FrameworkElement^>(TestServices::WindowHelper->WindowContent);
                ListView^ listView = safe_cast<ListView^>(root->FindName(L"myListView"));
                listView->Focus(FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Pressing Ctrl key.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl");
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching zoom operation with the mouse wheel.");
            for (int wheelClicks = 0; wheelClicks < 2; wheelClicks++)
            {
                TestServices::InputHelper->ScrollMouseWheel(sv, zoomIn ? 1 : -1 /* numberOfWheelClicks */);
                TestServices::WindowHelper->WaitForIdle();
            }

            LOG_OUTPUT(L"Waiting for ScrollViewer's zoom to complete.");
            viewChangedEvent->WaitForDefault();
            LOG_OUTPUT(L"Done.");

            LOG_OUTPUT(L"Releasing Ctrl key.");
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl");

            RunOnUIThread([&]()
            {
                if (zoomIn)
                {
                    VERIFY_IS_TRUE(sv->ZoomFactor > 1.0f);
                }
                else
                {
                    VERIFY_IS_TRUE(sv->ZoomFactor < 1.0f);
                }
            });
        }

        // Zooms and pans a ListView of variable-sized items and validates the ViewChanging's FinalView property after inertia.
        void ListViewTest::ValidateEndOfInertiaView()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>(EventOptions::CaptureScreenOnTimeout, L"ViewChanged");
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChanging2Registration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"HorizontalListView.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);
            double finalHorizontalOffset1 = 0.0;
            double finalHorizontalOffset2 = 0.0;

            RunOnUIThread([&]()
            {
                // Populate the ListView with variable sized items
                auto itemList = ref new Platform::Collections::Vector<Platform::String^>();
                for (int i = 0; i < 200; i++)
                {
                    itemList->Append(L"Item Identity " + i*i*i);
                }

                FrameworkElement^ root = safe_cast<FrameworkElement^>(TestServices::WindowHelper->WindowContent);
                ListView^ listView = safe_cast<ListView^>(root->FindName(L"myListView"));
                listView->ItemsSource = itemList;
            });

            LOG_OUTPUT(L"Launching zoom operation.");
            TestServices::InputHelper->ZoomInToEdges(sv, 20 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after zoom-in.",
                    sv->HorizontalOffset,
                    sv->VerticalOffset,
                    sv->ZoomFactor);

                VERIFY_IS_TRUE(sv->HorizontalOffset > 0.0);
                VERIFY_IS_TRUE(sv->VerticalOffset == 0.0);
                VERIFY_IS_TRUE(sv->ZoomFactor > 1.0f);

                // Record the two latest FinalView.HorizontalOffset values.
                viewChanging2Registration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
                    [&finalHorizontalOffset1, &finalHorizontalOffset2](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
                {
                    finalHorizontalOffset1 = finalHorizontalOffset2;
                    finalHorizontalOffset2 = args->FinalView->HorizontalOffset;
                }));

                // Support inertia in the coming pan manipulations.
                sv->IsScrollInertiaEnabled = true;

                // Turn off horizontal optional scroll snap points.
                sv->HorizontalSnapPointsType = SnapPointsType::None;
            });

            for (int i = 0; i < 3; i++)
            {
                LOG_OUTPUT(L"Launching horizontal pan operation to the left.");
                // Pan to re-estimate the scrolled off size.
                TestServices::InputHelper->PanFromCenter(sv, -200 /*relX*/, 0 /*relY*/, 2 /*velocityFactor*/);

                viewChangedEvent->WaitForDefault();
                viewChangedEvent->Reset();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after vertical pan.",
                        sv->HorizontalOffset,
                        sv->VerticalOffset,
                        sv->ZoomFactor);

                    // The last two ViewChanging FinalView values are expected to be equal to the resulting ScrollViewer.HorizontalOffset.
                    VERIFY_ARE_EQUAL(finalHorizontalOffset1, finalHorizontalOffset2);
                    VERIFY_ARE_EQUAL(sv->HorizontalOffset, finalHorizontalOffset2);
                });
            }

            for (int i = 0; i < 2; i++)
            {
                LOG_OUTPUT(L"Launching horizontal pan operation to the right.");
                // Pan to re-estimate the scrolled off size.
                TestServices::InputHelper->PanFromCenter(sv, +200 /*relX*/, 0 /*relY*/, 2 /*velocityFactor*/);

                viewChangedEvent->WaitForDefault();
                viewChangedEvent->Reset();
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    LOG_OUTPUT(L"Final HorizontalOffset: %f, VerticalOffset: %f, ZoomFactor: %f, after vertical pan.",
                        sv->HorizontalOffset,
                        sv->VerticalOffset,
                        sv->ZoomFactor);

                    // The last two ViewChanging FinalView values are expected to be equal to the resulting ScrollViewer.HorizontalOffset.
                    VERIFY_ARE_EQUAL(finalHorizontalOffset1, finalHorizontalOffset2);
                    VERIFY_ARE_EQUAL(sv->HorizontalOffset, finalHorizontalOffset2);
                });
            }
        }
    } } }
} } } }
