// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "BorderTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>
#include <SafeEventRegistration.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ BorderTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\";
}

bool BorderTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool BorderTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool BorderTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void BorderTests::RenderSolidColorBorders()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    Canvas^ rootCanvas = safe_cast<Canvas^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SolidColorBorders.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void BorderTests::BorderDiscardMaskOnResizeWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    BorderDiscardMaskOnResizeInternal();
}

void BorderTests::BorderDiscardMaskOnResizeInternal()
{
    LOG_OUTPUT(L"[Border renders with alpha mask.]");
    Canvas^ rootCanvas;
    Border^ elem;
    RunOnUIThread([&]()
    {
        elem = ref new Border();
        elem->Width = 30;
        elem->Height = 30;
        elem->BorderThickness = xaml::Thickness({ 2,2,2,2 });
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(elem);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "1");

    LOG_OUTPUT(L"[Border gets an opaque brush. No longer renders with alpha mask.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Border gets a new size. The old alpha mask is now incorrect.]");
    RunOnUIThread([&]()
    {
        elem->Width = 60;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Border gets a transparent brush, and renders with an alpha mask once again. The alpha mask should be updated with the new size.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
}

void BorderTests::GridDiscardMaskOnResizeWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    GridDiscardMaskOnResizeInternal();
}

void BorderTests::GridDiscardMaskOnResizeInternal()
{
    LOG_OUTPUT(L"[Border renders with alpha mask.]");
    Canvas^ rootCanvas;
    Grid^ elem;
    RunOnUIThread([&]()
    {
        elem = ref new Grid();
        elem->Width = 30;
        elem->Height = 30;
        elem->BorderThickness = xaml::Thickness({ 2,2,2,2 });
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(elem);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "1");

    LOG_OUTPUT(L"[Grid gets an opaque brush. No longer renders with alpha mask.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Grid gets a new size. The old alpha mask is now incorrect.]");
    RunOnUIThread([&]()
    {
        elem->Width = 60;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Grid gets a transparent brush, and renders with an alpha mask once again. The alpha mask should be updated with the new size.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
}

void BorderTests::ContentPresenterDiscardMaskOnResizeWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ContentPresenterDiscardMaskOnResizeInternal();
}

void BorderTests::ContentPresenterDiscardMaskOnResizeInternal()
{
    LOG_OUTPUT(L"[ContentPresenter renders with alpha mask.]");
    Canvas^ rootCanvas;
    ContentPresenter^ elem;
    RunOnUIThread([&]()
    {
        elem = ref new ContentPresenter();
        elem->Width = 30;
        elem->Height = 30;
        elem->BorderThickness = xaml::Thickness({ 2,2,2,2 });
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(elem);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "1");

    LOG_OUTPUT(L"[ContentPresenter gets an opaque brush. No longer renders with alpha mask.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[ContentPresenter gets a new size. The old alpha mask is now incorrect.]");
    RunOnUIThread([&]()
    {
        elem->Width = 60;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[ContentPresenter gets a transparent brush, and renders with an alpha mask once again. The alpha mask should be updated with the new size.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
}

void BorderTests::RectangleDiscardMaskOnResizeWUC()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RectangleDiscardMaskOnResizeInternal();
}

void BorderTests::RectangleDiscardMaskOnResizeInternal()
{
    LOG_OUTPUT(L"[Rectangle renders with alpha mask.]");
    Canvas^ rootCanvas;
    Microsoft::UI::Xaml::Shapes::Rectangle^ elem;
    RunOnUIThread([&]()
    {
        elem = ref new Microsoft::UI::Xaml::Shapes::Rectangle();
        elem->Width = 30;
        elem->Height = 30;
        elem->StrokeThickness = 2;
        elem->Stroke = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(elem);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "1");

    LOG_OUTPUT(L"[Rect loses its stroke. No longer renders with alpha mask.]");
    RunOnUIThread([&]()
    {
        elem->Stroke = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Rect gets a new size. The old alpha mask is now incorrect.]");
    RunOnUIThread([&]()
    {
        elem->Width = 60;
    });
    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"[Rect gets a stroke, and renders with an alpha mask once again. The alpha mask should be updated with the new size.]");
    RunOnUIThread([&]()
    {
        elem->Stroke = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
}

void BorderTests::RenderTransparentBordersAndBackgrounds()
{
    TestCleanupWrapper cleanup;
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    xaml_media::SolidColorBrush^ transparentBrush = nullptr;
    xaml_controls::Border^ border = nullptr;

    LOG_OUTPUT(L"A null Background and a null BorderBrush should result in zero Visuals.");
    RunOnUIThread([&]()
    {
        transparentBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(0, 0, 0, 0));
        border = ref new xaml_controls::Border();

        border->Width = 200;
        border->Height = 200;

        border->Background = nullptr;
        border->BorderBrush = nullptr;
        border->BorderThickness = xaml::Thickness({ 0,0,0,0 });

        TestServices::WindowHelper->WindowContent = border;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ZeroVisuals");

    LOG_OUTPUT(L"A transparent Background and a null BorderBrush should result in one Visual deflated based on the BorderThickness.");
    RunOnUIThread([&]()
    {
        border->Background = transparentBrush;
        border->BorderBrush = nullptr;
        border->BorderThickness = xaml::Thickness({ 2,2,2,2 });
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "DeflatedSingleVisual");

    LOG_OUTPUT(L"A null Background and a transparent BorderBrush with an all-zeroes BorderThickness should result in zero Visuals.");
    RunOnUIThread([&]()
    {
        border->Background = nullptr;
        border->BorderBrush = transparentBrush;
        border->BorderThickness = xaml::Thickness({ 0,0,0,0 });
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "ZeroVisuals");

    LOG_OUTPUT(L"A null Background and a transparent BorderBrush with a non-zero BorderThickness should result in one NineGrid.");
    RunOnUIThread([&]()
    {
        border->Background = nullptr;
        border->BorderBrush = transparentBrush;
        border->BorderThickness = xaml::Thickness({ 2,2,2,2 });
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "SingleNineGrid");

    LOG_OUTPUT(L"A transparent Background and a transparent BorderBrush should result in one Visual inflated.");
    RunOnUIThread([&]()
    {
        border->Background = transparentBrush;
        border->BorderBrush = transparentBrush;
        border->BorderThickness = xaml::Thickness({ 0,0,0,0 }); /* All-zeroes thickness*/
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "InflatedSingleVisual");

    LOG_OUTPUT(L"A transparent Background and a transparent BorderBrush should result in one Visual inflated.");
    RunOnUIThread([&]()
    {
        border->Background = transparentBrush;
        border->BorderBrush = transparentBrush;
        border->BorderThickness = xaml::Thickness({ 2,2,2,2 }); /* Non-zero thickness */
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "InflatedSingleVisual");
}

void BorderTests::NineGridWithRoundedCorners()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    LOG_OUTPUT(L"[Border renders with alpha mask and a hollow NineGrid.]");
    RunOnUIThread([&]()
    {
        xaml_controls::Border^ border = ref new xaml_controls::Border();
        border->Width = 100;
        border->Height = 100;
        border->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 20,20,20,20 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(border);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void BorderTests::RoundedCornersLayoutRounding()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 1.5f);

    LOG_OUTPUT(L"[1px thick border under 1.5x scale should be rounded up.]");
    RunOnUIThread([&]()
    {
        xaml_controls::Border^ square = ref new xaml_controls::Border();
        square->Width = 50;
        square->Height = 50;
        square->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        square->BorderThickness = xaml::Thickness({ 1,1,1,1 });

        xaml_controls::Border^ rounded = ref new xaml_controls::Border();
        rounded->Width = 50;
        rounded->Height = 50;
        rounded->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        rounded->BorderThickness = xaml::Thickness({ 1,1,1,1 });
        rounded->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(2);
        Canvas::SetTop(rounded, 60);

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(square);
        rootCanvas->Children->Append(rounded);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void BorderTests::RoundedCornersLayoutRounding2()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 1.25f);

    LOG_OUTPUT(L"[Square corners on the right edge under 1.25x scale should not cause AA pixels in the mask.]");
    RunOnUIThread([&]()
    {
        xaml_controls::Border^ rounded = ref new xaml_controls::Border();
        rounded->Width = 200;
        rounded->Height = 50;
        rounded->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        rounded->CornerRadius = xaml::CornerRadiusHelper::FromRadii(10, 0, 0, 10);  // TL, TR, BR, BL

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(rounded);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void BorderTests::RoundedCornersLayoutRounding3()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(wf::Size(400, 300), 1.25f);

    LOG_OUTPUT(L"[Square corners on the right edge under 1.25x scale with layout rounding turned off and fractional sizes should not cause AA pixels in the mask.]");
    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->UseLayoutRounding = false;

        Border^ fractionOffset = MakeBorder(rootCanvas, 100.0f, 60.0f, 0.3f, 0.3f);
        fractionOffset->UseLayoutRounding = false;
        fractionOffset->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        fractionOffset->CornerRadius = xaml::CornerRadiusHelper::FromRadii(10, 0, 0, 10);  // TL, TR, BR, BL

        Border^ fractionSize = MakeBorder(rootCanvas, 100.3f, 60.3f, 0.0f, 80.0f);
        fractionSize->UseLayoutRounding = false;
        fractionSize->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        fractionSize->CornerRadius = xaml::CornerRadiusHelper::FromRadii(10, 0, 0, 10);  // TL, TR, BR, BL

        Border^ fractionCorner = MakeBorder(rootCanvas, 100.0f, 60.0f, 0.0f, 160.0f);
        fractionCorner->UseLayoutRounding = false;
        fractionCorner->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        fractionCorner->CornerRadius = xaml::CornerRadiusHelper::FromRadii(10.3, 0, 0, 10.3);  // TL, TR, BR, BL

        Border^ fractionEverything = MakeBorder(rootCanvas, 100.3f, 60.3f, 0.3f, 240.3f);
        fractionEverything->UseLayoutRounding = false;
        fractionEverything->Background = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        fractionEverything->CornerRadius = xaml::CornerRadiusHelper::FromRadii(10.3, 0, 0, 10.3);  // TL, TR, BR, BL

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void BorderTests::RoundedCornersWithNoChildren()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_controls::Border^ border;
    xaml_controls::Border^ border2;
    xaml_shapes::Rectangle^ rectangle;

    LOG_OUTPUT(L"[Borders with rounded inside corners (which require a clip). One has a child and one does not.]");
    RunOnUIThread([&]()
    {
        border = ref new xaml_controls::Border();
        border->Width = 100;
        border->Height = 100;
        border->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 2,2,2,2 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));

        border2 = ref new xaml_controls::Border();
        border2->Width = 100;
        border2->Height = 100;
        border2->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border2->BorderThickness = xaml::Thickness({ 2,2,2,2 });
        border2->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);
        border2->Child = rectangle;
        Canvas::SetTop(border2, 100);

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(border);
        rootCanvas->Children->Append(border2);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    LOG_OUTPUT(L"[Moving the child to a different border]");
    RunOnUIThread([&]()
    {
        border2->Child = nullptr;
        border->Child = rectangle;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    LOG_OUTPUT(L"[Removing rounded corners]");
    RunOnUIThread([&]()
    {
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(0);
        border2->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(0);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");

    LOG_OUTPUT(L"[Moving the child back]");
    RunOnUIThread([&]()
    {
        border->Child = nullptr;
        border2->Child = rectangle;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void BorderTests::RasterizationScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        bool invalidArgThrown = false;

        xaml_controls::Border^ border = ref new xaml_controls::Border();
        try
        {
            border->RasterizationScale = 0;
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            invalidArgThrown = true;
        }

        VERIFY_IS_TRUE(invalidArgThrown);
    });

    RunOnUIThread([&]()
    {
        bool invalidArgThrown = false;

        xaml_controls::Border^ border = ref new xaml_controls::Border();
        try
        {
            border->RasterizationScale = -1;
        }
        catch (Platform::Exception^ e)
        {
            VERIFY_IS_TRUE(e->HResult == E_INVALIDARG);
            invalidArgThrown = true;
        }

        VERIFY_IS_TRUE(invalidArgThrown);
    });

    RunOnUIThread([&]()
    {
        xaml_controls::Border^ border = ref new xaml_controls::Border();
        border->Width = 100;
        border->Height = 100;
        border->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 20,20,20,20 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);
        border->RasterizationScale = 2;

        xaml_controls::Border^ border2 = ref new xaml_controls::Border();
        border2->Width = 100;
        border2->Height = 100;
        border2->BorderBrush = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border2->BorderThickness = xaml::Thickness({ 20,20,20,20 });
        border2->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(30);
        border2->RasterizationScale = 2;

        Canvas^ rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(border);
        rootCanvas->Children->Append(border2);

        TestServices::WindowHelper->WindowContent = rootCanvas;
    });
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

Border^ BorderTests::MakeBorder(Panel^ parent, float width, float height, float x, float y)
{
    Border^ border = ref new Border();
    border->Width = width;
    border->Height = height;
    parent->Children->Append(border);
    Canvas::SetLeft(border, x);
    Canvas::SetTop(border, y);
    return border;
}

void BorderTests::AddBorderVariation(Panel^ parent, int x, xaml::Thickness thickness, xaml::CornerRadius cornerRadius, ImageBrush^ imageBrush)
{
    // Solid color border - collapsed nine grid
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 0.0f);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }

    // Gradient border - uncollapsed nine grid
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 70.0f);
        border->BorderBrush = imageBrush;
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }

    // Solid color background - collapsed nine grid
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 140.0f);
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }

    // Gradient background - no nine grid
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 210.0f);
        border->Background = imageBrush;
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }

    // Solid color border + solid color background - collapsed nine grid for both
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 280.0f);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 128, 0, 0));
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 128, 0));
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }

    // Gradient border + solid color background - uncollapsed nine grid for border, no nine grid for background
    {
        Border^ border = MakeBorder(parent, 100.0f, 60.0f, static_cast<float>(x), 350.0f);
        border->BorderBrush = imageBrush;
        border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 255));
        border->BorderThickness = thickness;
        border->CornerRadius = cornerRadius;
    }
}

void BorderTests::NineGridOptimization()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    wh->SetWindowSizeOverrideWithWindowScale(wf::Size(975, 615), 1.5f);

    Canvas^ rootCanvas;
    Border^ lose1, ^lose2, ^lose3, ^lose4;
    Border^ gain1, ^gain2, ^gain3, ^gain4;
    ImageBrush^ imageBrush;

    auto bitmapImageOpenedEvent = std::make_shared<Event>();
    auto imageOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);

    auto uri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\80x80Checker.png");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Load an ImageBrush first.");

        BitmapImage^ bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = uri;
        imageOpenedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"  > BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        imageBrush = ref new ImageBrush();
        imageBrush->ImageSource = bitmapImage;
        imageBrush->Stretch = Stretch::Fill;

        rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        Border^ dummyBorder = MakeBorder(rootCanvas, 100.0f, 60.0f, 0.0f, 0.0f);
        dummyBorder->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(3);
        dummyBorder->BorderThickness = xaml::Thickness({ 3,3,3,3 });
        dummyBorder->BorderBrush = imageBrush;
    });
    bitmapImageOpenedEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Optimization variations.");
        rootCanvas->Children->Clear();

        // Nine grid without a mask - for avoiding overdraw in the center
        {
            Border^ border = MakeBorder(rootCanvas, 100.0f, 60.0f, 550.0f, 0.0f);
            border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
            border->BorderThickness = xaml::Thickness({ 1,1,1,1 });
        }

        // Nine grid with a mask just for the background - mask should be collapsed
        {
            Border^ border = MakeBorder(rootCanvas, 100.0f, 60.0f, 550.0f, 70.0f);
            border->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
            border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(3);
        }

        // Nine grid with a mask, square insides
        AddBorderVariation(rootCanvas, 0, xaml::Thickness({ 10,10,10,10 }), xaml::CornerRadiusHelper::FromUniformRadius(3), imageBrush);

        // Nine grid with a mask, round insides
        AddBorderVariation(rootCanvas, 110, xaml::Thickness({ 10,10,10,10 }), xaml::CornerRadiusHelper::FromUniformRadius(10), imageBrush);

        // Nine grid with a mask, round + square insides
        AddBorderVariation(rootCanvas, 220, xaml::Thickness({ 20,20,20,20 }), xaml::CornerRadiusHelper::FromRadii(4, 8, 12, 16), imageBrush);

        // Degenerate border that's all round
        AddBorderVariation(rootCanvas, 330, xaml::Thickness({ 0,0,0,0 }), xaml::CornerRadiusHelper::FromUniformRadius(50), imageBrush);

        // Degenerate border that's all round
        AddBorderVariation(rootCanvas, 440, xaml::Thickness({ 20,20,20,20 }), xaml::CornerRadiusHelper::FromUniformRadius(20), imageBrush);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

    RunOnUIThread([&]()
    {
        rootCanvas->Children->Clear();

        lose1 = MakeBorder(rootCanvas, 150, 80, 0, 0);
        lose1->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        lose1->CornerRadius =xaml::CornerRadiusHelper::FromUniformRadius(10);

        gain1 = MakeBorder(rootCanvas, 150, 80, 160, 0);
        gain1->Background = imageBrush;
        gain1->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        lose2 = MakeBorder(rootCanvas, 150, 80, 0, 90);
        lose2->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        lose2->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        lose2->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        gain2 = MakeBorder(rootCanvas, 150, 80, 160, 90);
        gain2->BorderBrush = imageBrush;
        gain2->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        gain2->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        lose3 = MakeBorder(rootCanvas, 150, 80, 0, 180);
        lose3->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        lose3->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        lose3->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        lose3->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        gain3 = MakeBorder(rootCanvas, 150, 80, 160, 180);
        gain3->BorderBrush = imageBrush;
        gain3->Background = imageBrush;
        gain3->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        gain3->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        lose4 = MakeBorder(rootCanvas, 150, 80, 0, 270);
        lose4->BorderBrush = imageBrush;
        lose4->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        lose4->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        lose4->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);

        gain4 = MakeBorder(rootCanvas, 150, 80, 160, 270);
        gain4->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        gain4->Background = imageBrush;
        gain4->BorderThickness = xaml::Thickness({ 10,10,10,10 });
        gain4->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10);
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

    RunOnUIThread([&]()
    {
        lose1->Background = imageBrush;
        gain1->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));

        lose2->BorderBrush = imageBrush;
        gain2->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));

        lose3->BorderBrush = imageBrush;
        lose3->Background = imageBrush;
        gain3->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        gain3->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));

        lose4->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        lose4->Background = imageBrush;
        gain4->BorderBrush = imageBrush;
        gain4->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");

    RunOnUIThread([&]()
    {
        lose1->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        gain1->Background = imageBrush;

        lose2->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        gain2->BorderBrush = imageBrush;

        lose3->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        lose3->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        gain3->BorderBrush = imageBrush;
        gain3->Background = imageBrush;

        lose4->BorderBrush = imageBrush;
        lose4->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
        gain4->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        gain4->Background = imageBrush;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"3");

    RunOnUIThread([&]()
    {
        rootCanvas->Children->Clear();

        Border^ roundBorder = MakeBorder(rootCanvas, 100, 60, 0, 0);
        roundBorder->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        roundBorder->BorderThickness = xaml::Thickness({ 1,1,1,1 });
        roundBorder->CornerRadius = xaml::CornerRadiusHelper::FromRadii(4, 0, 0, 4);

        Border^ squareBorder = MakeBorder(rootCanvas, 100, 60, 110, 0);
        squareBorder->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        squareBorder->BorderThickness = xaml::Thickness({ 1,1,1,1 });
    });
    wh->WaitForIdle();
    // No surface comparison. Helix shows inconsistent AA differences in the rounded corner border alpha mask. The
    // thing we're testing here is that the square border correctly shows 1.333 insets (1.333 * 1.5 zoom scale = 2px),
    // which doesn't have to do with the mask anyway.
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"4");
}

void BorderTests::NineGridOptimizationLayoutRounding()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        Border^ border = MakeBorder(rootCanvas, 100, 60, 0, 0);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 1,1,1,1 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromRadii(4, 0, 0, 4);

        border = MakeBorder(rootCanvas, 100, 60, 110, 0);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 1,1,1,1 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromRadii(4, 0, 0, 4);
        border->UseLayoutRounding = false;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void BorderTests::AllBorderBorder()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    RunOnUIThread([&]()
    {
        Canvas^ rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;
        Border^ border;

        // Degenerate border that's all border
        border = MakeBorder(rootCanvas, 100, 60, 0, 0);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 40,40,40,40 }); // Total border thickness is 80, which is greater than the height of 60.

        // Degenerate border that's all border and is rounded
        border = MakeBorder(rootCanvas, 100, 60, 110, 0);
        border->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(128, 255, 0, 0));
        border->BorderThickness = xaml::Thickness({ 40,40,40,40 });
        border->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(10); // Total border thickness is 80, which is greater than the height of 60.
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } }
