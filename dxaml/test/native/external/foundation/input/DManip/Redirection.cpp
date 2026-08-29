// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Redirection.h"
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

        bool Redirection::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool Redirection::ClassCleanup()
        {
            return true;
        }

        bool Redirection::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ Redirection::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ScrollViewer^ Redirection::SetupUI(
            _In_ Platform::String^ filename,
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            ScrollViewer^ sv = nullptr;
            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                sv = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));

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

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                Primitives::Popup^ popup = safe_cast<Primitives::Popup^>(root->FindName(L"myPopup"));
                popup->IsOpen = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            return sv;
        }

        void Redirection::RemovePopup()
        {
            LOG_OUTPUT(L"Removing Popup");
            RunOnUIThread([&]()
            {
                FrameworkElement^ root = safe_cast<FrameworkElement^>(TestServices::WindowHelper->WindowContent);
                Primitives::Popup^ popup = safe_cast<Primitives::Popup^>(root->FindName(L"myPopup"));
                Panel^ parent = safe_cast<Panel^>(popup->Parent);
                unsigned int index;
                VERIFY_IS_TRUE(parent->Children->IndexOf(popup, &index));
                parent->Children->RemoveAt(index);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void Redirection::Popup()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"Redirection1.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as after scenario 1
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Scenario 3:  Remove the Popup from the XAML tree.  This will tear down a portion of the DComp visual tree.
            RemovePopup();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
        }

        void Redirection::PopupPan()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"Redirection1.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, -125 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            RemovePopup();
        }

        void Redirection::PopupZoom()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"Redirection1.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching zoom operation.");
            TestServices::InputHelper->ZoomInToEdges(sv, 130 /*equidistanceFromEdges*/, Microsoft::UI::Xaml::Controls::Orientation::Horizontal, 0.66 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            RemovePopup();
        }

        void Redirection::NestedPopup()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"Redirection2.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"0").GetString());

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as after scenario 1, with one exception.  Popup will add temporary visual elements
            // into the tree.  The elements don't actually render anything and are removed on the next render walk.  Those visuals willget
            // removed during the render walk after the device lost so we need to account for that in the baseline comparison.
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            // Scenario 3:  Remove the Popup and its nested child Popup from the XAML tree.  This will tear down a portion of the DComp visual tree.
            RemovePopup();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());
        }

        void Redirection::NestedPopupPan()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"Redirection2.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, -125 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            RemovePopup();
        }

        void Redirection::RegressionTest_3271313()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"RegressionTest_3271313.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                sv->ChangeView(nullptr, 10.5, nullptr, true /*disableAnimation*/);
            });

            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            RemovePopup();
        }

    } } }
} } } }
