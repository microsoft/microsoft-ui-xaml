// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ComboBox.h"
#include <RuntimeEnabledFeaturesEnum.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <ComboBoxHelper.h>
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

        bool ComboBoxTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ComboBoxTest::ClassCleanup()
        {
            return true;
        }

        bool ComboBoxTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ ComboBoxTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ComboBox^ ComboBoxTest::SetupUI(_In_ Platform::String^ filename)
        {
            ComboBox^ comboBox = nullptr;
            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                comboBox = safe_cast<ComboBox^>(root->FindName(L"myComboBox"));

                TestServices::WindowHelper->WindowContent = root;
            });

            return comboBox;
        }

        xaml_controls::ScrollViewer^ ComboBoxTest::SetupScrollViewer(
            _In_ xaml_controls::ComboBox^ comboBox,
            _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(xaml_controls::ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            ScrollViewer^ sv = nullptr;

            RunOnUIThread([&]()
            {
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(comboBox, 0));
                sv = safe_cast<ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->HorizontalScrollMode = ScrollMode::Disabled;

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

        void ComboBoxTest::Basics()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            Platform::String^ filename = GetResourcesPath() + L"ComboBox.xaml";
            ComboBox^ comboBox = SetupUI(filename);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                comboBox->SelectedIndex = 0;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Opening ComboBox");
            ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            ComboBoxHelper::CloseComboBox(comboBox);
            TestServices::WindowHelper->WaitForIdle();
        }

        void ComboBoxTest::PanInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"ComboBox.xaml";
            ComboBox^ comboBox = SetupUI(filename);
            TestServices::WindowHelper->WaitForIdle();
            ScrollViewer^ scrollViewer = SetupScrollViewer(comboBox, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            LOG_OUTPUT(L"Opening ComboBox");
            ComboBoxHelper::OpenComboBox(comboBox, ComboBoxHelper::OpenMethod::Programmatic);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Launching vertical pan operation.");
            TestServices::InputHelper->PanFromCenter(scrollViewer, 0 /*relX*/, -125 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            ComboBoxHelper::CloseComboBox(comboBox);
            TestServices::WindowHelper->WaitForIdle();
        }

        void ComboBoxTest::PanWUC()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
            PanInternal();
        }

    } } }
} } } }
