// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListBox.h"
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

        bool ListBoxTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ListBoxTest::ClassCleanup()
        {
            return true;
        }

        bool ListBoxTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ ListBoxTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ScrollViewer^ ListBoxTest::SetupUI(
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
                ListBox^ listBox = safe_cast<ListBox^>(root->FindName(L"myListBox"));
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(listBox, 0));
                sv = safe_cast<ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;

                viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                {
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

        void ListBoxTest::Basics()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListBox.xaml";
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

        void ListBoxTest::Pan()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListBox.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -200 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void ListBoxTest::Zoom()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ListBox.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            RunOnUIThread([&]()
            {
                sv->VerticalScrollMode = ScrollMode::Disabled;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Launching zoom operation.");

// TODO:  Using input injection is producing a tiny fluctuation at a random frequency, causing test noise.
// For now, stabilizing this test by using ChangeView().  Ideally we should be using input injection.
//                TestServices::InputHelper->ZoomInToEdges(sv, 130 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Vertical, 0.66 /*velocityFactor*/);
                sv->ChangeView(nullptr, nullptr, 2.0f, true);
            });

            viewChangedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

    } } }
} } } }
