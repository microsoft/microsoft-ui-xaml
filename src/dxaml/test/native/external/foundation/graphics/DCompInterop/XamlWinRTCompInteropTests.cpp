// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"

#include <SafeEventRegistration.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include "XamlWinRTCompInteropTests.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <DisableErrorReportingScopeGuard.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Hosting;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Composition;

using namespace test_infra;
using namespace MockDComp;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        Platform::String^ XamlWinRTCompInteropTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"Resources\\Native\\Foundation\\Graphics\\DCompInterop\\";
        }

        bool XamlWinRTCompInteropTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool XamlWinRTCompInteropTests::ClassCleanup()
        {
            return true;
        }

        bool XamlWinRTCompInteropTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool XamlWinRTCompInteropTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case:
        //  - Retrieves a WinRT Composition Windows.UI.Composition.Visual instance
        //    for a UIElement using the ElementCompositionPreview.GetElementVisual
        //    method.
        //  - Reads and writes the Opacity property of the Visual instance.
        //  - Discards the visual by discarding its owning UIElement.
        //------------------------------------------------------------------------
        void XamlWinRTCompInteropTests::BasicUIElementCompositionVisualInternal()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

            Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"XamlDCompInteropTests-BasicUIElementDCompVisual.xaml"));
            Border^ border = nullptr;
            Microsoft::UI::Composition::Visual^ compositionVisual = nullptr;

            RunOnUIThread([&]()
            {
                VERIFY_IS_NOT_NULL(rootCanvas);
                TestServices::WindowHelper->WindowContent = rootCanvas;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Retrieving DComp Windows.UI.Composition.Visual for Border element.");
                border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
                VERIFY_IS_NOT_NULL(border);

                compositionVisual = ElementCompositionPreview::GetElementVisual(border);
                VERIFY_IS_NOT_NULL(compositionVisual);

                LOG_OUTPUT(L"Reading WinRT Visual.Opacity property value.");
                float opacity = compositionVisual->Opacity;
                LOG_OUTPUT(L"Windows.UI.Composition.Visual's default Opacity=%f.", opacity);
                compositionVisual->Opacity = 0.25;
                opacity = compositionVisual->Opacity;
                LOG_OUTPUT(L"Windows.UI.Composition.Visual's new Opacity=%f.", opacity);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Discarding DComp visual for Border element.");
                compositionVisual = nullptr;
                rootCanvas->Children->Clear();
            });

            TestServices::WindowHelper->WaitForIdle();
        }

        void XamlWinRTCompInteropTests::BasicUIElementCompositionVisualWUC()
        {
            WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);
            BasicUIElementCompositionVisualInternal();
        }

        void XamlWinRTCompInteropTests::HandOffPropertyCache()
        {
            WUCRenderingScopeGuard wuc(DCompRendering::WUCCompleteSynchronousCompTree);

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

            Canvas^ rootCanvas = nullptr;
            Border^ border = nullptr;
            Microsoft::UI::Composition::Visual^ compositionVisual = nullptr;

            LOG_OUTPUT(L"Base case");
            RunOnUIThread([&]()
            {
                border = ref new Border();
                border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 255, 0));
                border->Width = 200;
                border->Height = 100;

                rootCanvas = ref new Canvas();
                rootCanvas->Children->Append(border);

                TestServices::WindowHelper->WindowContent = rootCanvas;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"1").GetString());

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Retrieving DComp Windows.UI.Composition.Visual for Border element.");
                border = safe_cast<Border^>(rootCanvas->Children->GetAt(0));
                VERIFY_IS_NOT_NULL(border);

                compositionVisual = ElementCompositionPreview::GetElementVisual(border);
                VERIFY_IS_NOT_NULL(compositionVisual);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

            LOG_OUTPUT(L"Collapse with HandOff visual, defaults go to cache");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

            LOG_OUTPUT(L"Uncollapse with HandOff visual, bring back defaults from cache");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

            LOG_OUTPUT(L"Offset the Border to (10,20)");
            RunOnUIThread([&]()
            {
                xaml_controls::Canvas::SetLeft(border, 10);
                xaml_controls::Canvas::SetTop(border, 20);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"4").GetString());

            LOG_OUTPUT(L"Collapse with offset Border, offsets go to cache");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

            LOG_OUTPUT(L"Uncollapse with offset Border, bring back (10,20) from cache");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"4").GetString());

            LOG_OUTPUT(L"Collapse with offset Border, offset back to (0,0) while collapsed, then uncollapse, (10, 20) from cache gets overridden");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

            RunOnUIThread([&]()
            {
                xaml_controls::Canvas::SetLeft(border, 0);
                xaml_controls::Canvas::SetTop(border, 0);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"2").GetString());

            LOG_OUTPUT(L"App offsets HandOff visual to (50,60)");
            RunOnUIThread([&]()
            {
                ::Windows::Foundation::Numerics::float3 newOffset = { 50, 60, 0 };
                compositionVisual->Offset = newOffset;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"5").GetString());

            LOG_OUTPUT(L"Collapse with offset Handoff visual, (0,0) goes into cache");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

            LOG_OUTPUT(L"Uncollapse, cache is same as default so (50,60) stays");
            RunOnUIThread([&]()
            {
                border->Visibility = xaml::Visibility::Visible;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"5").GetString());

            LOG_OUTPUT(L"Remove Border");
            RunOnUIThread([&]()
            {
                border = nullptr;
                compositionVisual = nullptr;
                rootCanvas->Children->Clear();
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, Platform::StringReference(L"3").GetString());

        }

    } }
} } } }
