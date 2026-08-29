// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RelativePanelIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RelativePanel {

    bool RelativePanelIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RelativePanelIntegrationTests::ClassCleanup()
    {
        return true;
    }

    bool RelativePanelIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RelativePanelIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void  RelativePanelIntegrationTests::CanPerformLayout()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue' RelativePanel.RightOf='b0'/>"
                L"    <Rectangle x:Name='b2' Width='100' Height='100' Fill='Green' RelativePanel.RightOf='b1'/>"
                L"</RelativePanel>"
                ));

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Before");

        LOG_OUTPUT(L"Changing layout properties.");
        RunOnUIThread([&]()
        {
            xaml_controls::RelativePanel::SetRightOf(relativePanel->Children->GetAt(1), nullptr);
            xaml_controls::RelativePanel::SetRightOf(relativePanel->Children->GetAt(2), nullptr);
            xaml_controls::RelativePanel::SetBelow(relativePanel->Children->GetAt(1), relativePanel->Children->GetAt(0));
            xaml_controls::RelativePanel::SetBelow(relativePanel->Children->GetAt(2), relativePanel->Children->GetAt(1));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "After");
    }

    void  RelativePanelIntegrationTests::CanParseDeferredElements()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, true);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"    Background='Magenta' HorizontalAlignment='Left' VerticalAlignment='Top'>"
                L"    <Rectangle x:Name='b0' x:DeferLoadStrategy='Lazy' Width='100' Height='100' Fill='Red' RelativePanel.AlignRightWithPanel='True'/>"
                L"    <Rectangle x:Name='b1' Width='200' Height='200' Fill='Blue' RelativePanel.LeftOf='b0'/>"
                L"    <Rectangle x:Name='b2' x:DeferLoadStrategy='Lazy' Width='100' Height='100' Fill='Green' RelativePanel.Below='b1' RelativePanel.AlignRightWith='b0'/>"
                L"    <Rectangle x:Name='b3' x:DeferLoadStrategy='Lazy' Width='100' Height='100' Fill='Purple' RelativePanel.Below='b2' RelativePanel.AlignRightWith='b0'/>"
                L"    <Rectangle x:Name='b4' x:DeferLoadStrategy='Lazy' Width='100' Height='100' Fill='Yellow' RelativePanel.Below='b3' RelativePanel.AlignRightWith='b0'/>"
                L"    <Rectangle x:Name='b5' Width='100' Height='100' Fill='Orange' RelativePanel.Below='b4' RelativePanel.AlignRightWith='b4'/>"
                L"    <Rectangle x:Name='b6' x:DeferLoadStrategy='Lazy' Width='50' Height='50' Fill='Yellow' RelativePanel.Below='b5' RelativePanel.AlignRightWith='b5'/>"
                L"</RelativePanel>"
                ));

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void RelativePanelIntegrationTests::CanBeEmpty()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = ref new xaml_controls::RelativePanel;
            relativePanel->HorizontalAlignment = Microsoft::UI::Xaml::HorizontalAlignment::Left;
            relativePanel->VerticalAlignment = Microsoft::UI::Xaml::VerticalAlignment::Top;
            relativePanel->Width = 300.0;
            relativePanel->Height = 300.0;
            relativePanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void RelativePanelIntegrationTests::CanArrangePhysicallyImpossibleDefinitions()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;
        xaml_shapes::Rectangle^ r0 = nullptr;
        xaml_shapes::Rectangle^ r1 = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = ref new xaml_controls::RelativePanel;
            relativePanel->HorizontalAlignment = Microsoft::UI::Xaml::HorizontalAlignment::Left;
            relativePanel->VerticalAlignment = Microsoft::UI::Xaml::VerticalAlignment::Top;
            relativePanel->Width = 300.0;
            relativePanel->Height = 300.0;
            relativePanel->Background = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Green);

            r0 = ref new xaml_shapes::Rectangle;
            r0->Width = 100.0;
            r0->Height = 100.0;
            r0->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Red);

            r1 = ref new xaml_shapes::Rectangle;
            r1->Width = 100.0;
            r1->Height = 100.0;
            r1->Fill = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::Blue);

            xaml_controls::RelativePanel::SetLeftOf(r1, r0);
            xaml_controls::RelativePanel::SetRightOf(r1, r0);
            xaml_controls::RelativePanel::SetAbove(r1, r0);
            xaml_controls::RelativePanel::SetBelow(r1, r0);

            relativePanel->Children->Append(r0);
            relativePanel->Children->Append(r1);

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(r0->DesiredSize.Width, 100.0);
            VERIFY_ARE_EQUAL(r0->DesiredSize.Height, 100.0);
            VERIFY_ARE_EQUAL(r1->DesiredSize.Width, 0.0);
            VERIFY_ARE_EQUAL(r1->DesiredSize.Height, 0.0);
        });
    }

    void RelativePanelIntegrationTests::ThrowsExceptionForCircularDependencies()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red' RelativePanel.LeftOf='b1'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue' RelativePanel.RightOf='b0'/>"
                L"</RelativePanel>"
                ));

            VERIFY_THROWS_WINRT(
                relativePanel->Measure(wf::Size(400, 400)),
                Platform::Exception^);
        });

        // Clear layout exception element.
        TestServices::WindowHelper->SetLastLayoutExceptionElement(nullptr);
    }

    void RelativePanelIntegrationTests::ThrowsExceptionForNameNotFound()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue' RelativePanel.RightOf='b8'/>"
                L"</RelativePanel>"
                ));

            VERIFY_THROWS_WINRT(
                relativePanel->Measure(wf::Size(400, 400)),
                Platform::Exception^);
        });

        // Clear layout exception element.
        TestServices::WindowHelper->SetLastLayoutExceptionElement(nullptr);
    }

    void RelativePanelIntegrationTests::ThrowsExceptionForReferenceNotFound()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue'/>"
                L"</RelativePanel>"
                ));

            auto rectangle = ref new xaml_shapes::Rectangle();
            xaml_controls::RelativePanel::SetRightOf(safe_cast<xaml::FrameworkElement^>(relativePanel->Children->GetAt(1)), rectangle);

            VERIFY_THROWS_WINRT(
                relativePanel->Measure(wf::Size(400, 400)),
                Platform::Exception^);
        });

        // Clear layout exception element.
        TestServices::WindowHelper->SetLastLayoutExceptionElement(nullptr);
    }

    void RelativePanelIntegrationTests::ThrowsExceptionForInvalidType()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue'/>"
                L"</RelativePanel>"
                ));

            VERIFY_THROWS_WINRT(
                xaml_controls::RelativePanel::SetRightOf(safe_cast<xaml::FrameworkElement^>(relativePanel->Children->GetAt(1)), true),
                Platform::Exception^);
        });
    }

    void RelativePanelIntegrationTests::ThrowsExceptionForInvalidParsingOfDeferredElements()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;

        RunOnUIThread([&]()
        {
            VERIFY_THROWS_WINRT(
                auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' x:DeferLoadStrategy='Lazy' Width='100' Height='100' Fill='Blue' RelativePanel.RightOf='{Binding ElementName=b0}'/>"
                L"</RelativePanel>"
                )),
                Platform::Exception^);
        });
    }

    void  RelativePanelIntegrationTests::BorderChrome()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel BorderBrush='Red' BorderThickness='10' Padding='10' CornerRadius='10' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <Rectangle x:Name='b1' Width='100' Height='100' Fill='Blue' RelativePanel.RightOf='b0'/>"
                L"    <Rectangle x:Name='b2' Width='100' Height='100' Fill='Green' RelativePanel.RightOf='b1'/>"
                L"</RelativePanel>"
                ));

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void  RelativePanelIntegrationTests::VerifyPropertyChangesInvalidateMeasure()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b0' Width='100' Height='100' Fill='Red'/>"
                L"    <TextBlock x:Name='b1' Width='100' Height='100' Text='TextBlock' RelativePanel.RightOf='b0'/>"
                L"</RelativePanel>"
                ));

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(relativePanel->DesiredSize.Width, 200.0);
            VERIFY_ARE_EQUAL(relativePanel->DesiredSize.Height, 100.0);
        });

        LOG_OUTPUT(L"Changing layout properties.");
        RunOnUIThread([&]()
        {
            xaml_controls::RelativePanel::SetRightOf(relativePanel->Children->GetAt(1), nullptr);
            xaml_controls::RelativePanel::SetBelow(relativePanel->Children->GetAt(1), relativePanel->Children->GetAt(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(relativePanel->DesiredSize.Width, 100.0);
            VERIFY_ARE_EQUAL(relativePanel->DesiredSize.Height, 200.0);
        });
    }

    void  RelativePanelIntegrationTests::VerifyLayoutViaXamlReader()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        xaml_controls::RelativePanel^ relativePanel = nullptr;

        RunOnUIThread([&]()
        {
            relativePanel = safe_cast<xaml_controls::RelativePanel^>(xaml_markup::XamlReader::Load(
                L"<RelativePanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Rectangle x:Name='b1' Width='200' Height='200' Fill='Red'/>"
                L"    <Rectangle x:Name='b2' Width='200' Height='200' Fill='Blue' RelativePanel.RightOf='b1'/>"
                L"</RelativePanel>"
                ));

            TestServices::WindowHelper->WindowContent = relativePanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RelativePanel
