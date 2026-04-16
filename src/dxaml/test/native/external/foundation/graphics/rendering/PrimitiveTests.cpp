// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PrimitiveTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Documents;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Shapes;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ PrimitiveTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool PrimitiveTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool PrimitiveTests::ClassCleanup()
{
    return true;
}

bool PrimitiveTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool PrimitiveTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void PrimitiveTests::LoadAndVerify(Platform::String^ markupFile, bool bigWindow, bool waitForIdle, test_infra::LastInputDeviceType deviceType)
{
    auto wh = TestServices::WindowHelper;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    if (bigWindow)
    {
        wh->SetWindowSizeOverride(wf::Size(5000, 5000));
    }
    else
    {
        wh->SetWindowSizeOverride(wf::Size(400, 400));
    }

    bool imageFound = false;
    auto imageRegistration = CreateSafeEventRegistration(xaml_controls::Image, ImageOpened);
    auto imageBrushRegistration = CreateSafeEventRegistration(xaml_media::ImageBrush, ImageOpened);
    auto imageOpenedEvent = std::make_shared<Event>();

    auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(GetResourcesPath() + markupFile));
    RunOnUIThread([&]()
    {
        wh->WindowContent = root;
    });

    if (deviceType != test_infra::LastInputDeviceType::None)
    {
        wh->WaitForIdle();

        XamlRoot^ xamlRoot = nullptr;
        RunOnUIThread([&]()
        {
            xamlRoot = root->XamlRoot;
        });

        TestServices::WindowHelper->SetLastInputMethod(deviceType, xamlRoot);
    }

    RunOnUIThread([&]()
    {
        auto image = safe_cast<Image^>(root->FindName(L"myImage"));
        if (image != nullptr)
        {
            imageFound = true;
            imageRegistration.Attach(
                image,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Image Opened event raised for Image element.");
                imageOpenedEvent->Set();
            }));
        }

        auto imageBrush = safe_cast<ImageBrush^>(root->FindName(L"myImageBrush"));
        if (imageBrush != nullptr)
        {
            imageFound = true;
            imageBrushRegistration.Attach(
                imageBrush,
                ref new xaml::RoutedEventHandler([imageOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Image Opened event raised for ImageBrush element.");
                imageOpenedEvent->Set();
            }));
        }
    });

    if (imageFound)
    {
        imageOpenedEvent->WaitForDefault();
    }
    // WaitForIdle will cause us to wait until animations complete, but some test files have animations that run forever.
    if (waitForIdle)
    {
        wh->WaitForIdle();
    }
    else
    {
        wh->SynchronouslyTickUIThread(3);
    }

    RunOnUIThread([&]()
    {
        auto controlToFocus = safe_cast<Control^>(root->FindName(L"shouldReceiveFocus"));
        if (controlToFocus != nullptr)
        {
            controlToFocus->Focus(FocusState::Keyboard);
        }
    });

    // WaitForIdle will cause us to wait until animations complete, but some test files have animations that run forever.
    if (waitForIdle)
    {
        wh->WaitForIdle();
    }
    else
    {
        wh->SynchronouslyTickUIThread(3);
    }
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void PrimitiveTests::SolidColor1()
{
    LoadAndVerify(L"SolidColor1.xaml");
}

void PrimitiveTests::SolidColor2()
{
    LoadAndVerify(L"SolidColor2.xaml");
}

void PrimitiveTests::SolidColor3()
{
    LoadAndVerify(L"SolidColor3.xaml");
}

void PrimitiveTests::Texture1()
{
    LoadAndVerify(L"Texture1.xaml");
}

void PrimitiveTests::Texture2()
{
    LoadAndVerify(L"Texture2.xaml");
}

void PrimitiveTests::Texture3()
{
    LoadAndVerify(L"Texture3.xaml");
}

void PrimitiveTests::Texture4()
{
    LoadAndVerify(L"Texture4.xaml");
}

void PrimitiveTests::Texture5()
{
    LoadAndVerify(L"Texture5.xaml");
}

void PrimitiveTests::Texture6()
{
    LoadAndVerify(L"Texture6.xaml");
}

void PrimitiveTests::Texture7()
{
    LoadAndVerify(L"Texture7.xaml");
}

void PrimitiveTests::Texture8()
{
    LoadAndVerify(L"Texture8.xaml");
}

void PrimitiveTests::Texture9()
{
    LoadAndVerify(L"Texture9.xaml", true /*bigWindow*/);
}

void PrimitiveTests::Texture10()
{
    LoadAndVerify(L"Texture10.xaml");
}

void PrimitiveTests::Texture11()
{
    LoadAndVerify(L"Texture11.xaml");
}

void PrimitiveTests::LinearGradient1()
{
    LoadAndVerify(L"LinearGradient1.xaml");
}

void PrimitiveTests::LinearGradient2()
{
    LoadAndVerify(L"LinearGradient2.xaml");
}

void PrimitiveTests::LinearGradient3()
{
    LoadAndVerify(L"LinearGradient3.xaml");
}

void PrimitiveTests::LinearGradient4()
{
    LoadAndVerify(L"LinearGradient4.xaml");
}

void PrimitiveTests::LinearGradient5()
{
    LoadAndVerify(L"LinearGradient5.xaml");
}

void PrimitiveTests::LinearGradient6()
{
    LoadAndVerify(L"LinearGradient6.xaml");
}

void PrimitiveTests::LinearGradient7()
{
    LoadAndVerify(L"LinearGradient7.xaml");
}

void PrimitiveTests::LinearGradient8()
{
    LoadAndVerify(L"LinearGradient8.xaml");
}

void PrimitiveTests::Element1()
{
    LoadAndVerify(L"Element1.xaml");
}

void PrimitiveTests::Element2()
{
    LoadAndVerify(L"Element2.xaml");
}

void PrimitiveTests::Element3()
{
    LoadAndVerify(L"Element3.xaml");
}

void PrimitiveTests::Element5()
{
    LoadAndVerify(L"Element5.xaml");
}

void PrimitiveTests::Element6()
{
    LoadAndVerify(L"Element6.xaml");
}

void PrimitiveTests::Element7()
{
    LoadAndVerify(L"Element7.xaml");
}

void PrimitiveTests::Element8()
{
    LoadAndVerify(L"Element8.xaml");
}

void PrimitiveTests::Element9()
{
    LoadAndVerify(L"Element9.xaml", false /* bigWindow */, false /* waitForIdle */);
}

void PrimitiveTests::Element10()
{
    LoadAndVerify(L"Element10.xaml");
}

void PrimitiveTests::Element11()
{
    LoadAndVerify(L"Element11.xaml");
}

void PrimitiveTests::Element12()
{
    LoadAndVerify(L"Element12.xaml");
}

void PrimitiveTests::Element13()
{
    LoadAndVerify(L"Element13.xaml");
}

void PrimitiveTests::Element14()
{
    LoadAndVerify(L"Element14.xaml");
}

void PrimitiveTests::Element15()
{
    LoadAndVerify(L"Element15.xaml");
}

void PrimitiveTests::Element16()
{
    LoadAndVerify(L"Element16.xaml");
}

void PrimitiveTests::Element17()
{
    LoadAndVerify(L"Element17.xaml");
}

void PrimitiveTests::Element19()
{
    LoadAndVerify(L"Element19.xaml");
}

void PrimitiveTests::Element20()
{
    LoadAndVerify(L"Element20.xaml");
}

void PrimitiveTests::Element21()
{
    LoadAndVerify(L"Element21.xaml");
}

void PrimitiveTests::Element22()
{
    LoadAndVerify(L"Element22.xaml");
}

void PrimitiveTests::Element23()
{
    LoadAndVerify(L"Element23.xaml");
}

void PrimitiveTests::Element24()
{
    LoadAndVerify(L"Element24.xaml", true /* bigWindow */);
}

void PrimitiveTests::Element25()
{
    LoadAndVerify(L"Element25.xaml", true /* bigWindow */);
}

void PrimitiveTests::Element26()
{
    LoadAndVerify(L"Element26.xaml");
}

void PrimitiveTests::Element27()
{
    LoadAndVerify(L"Element27.xaml");
}

void PrimitiveTests::Element28()
{
    LoadAndVerify(L"Element28.xaml");
}

void PrimitiveTests::Element29()
{
    LoadAndVerify(L"Element29.xaml", false, true, test_infra::LastInputDeviceType::Keyboard);
}

void PrimitiveTests::Element4WUCFull()
{
    LoadAndVerify(L"Element4.xaml", false /* bigWindow */);
}

void PrimitiveTests::Element4BWUCFull()
{
    LoadAndVerify(L"Element4B.xaml", false /* bigWindow */);
}

void PrimitiveTests::AlphaMask1WUCFull()
{
    LoadAndVerify(L"AlphaMask1.xaml", false /* bigWindow */);
}

void PrimitiveTests::AlphaMask2WUCFull()
{
    LoadAndVerify(L"AlphaMask2.xaml", false /* bigWindow */);
}

void PrimitiveTests::AlphaMask3WUCFull()
{
    LoadAndVerify(L"AlphaMask3.xaml", false /* bigWindow */);
}

void PrimitiveTests::AlphaMask4WUCFull()
{
    LoadAndVerify(L"AlphaMask4.xaml", false /* bigWindow */);
}

void PrimitiveTests::AdvancedPathWUCFull()
{
    LoadAndVerify(L"Path2.xaml", false /* bigWindow */);
}

Border^ PrimitiveTests::MakeEmptyBorder()
{
    const auto& element = ref new Border();
    element->Width = 100;
    element->Height = 20;
    return element;
}

Border^ PrimitiveTests::MakeBorder(bool hasBackground)
{
    const auto& element = MakeEmptyBorder();
    element->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
    element->BorderThickness = ThicknessHelper::FromUniformLength(8.0);
    if (hasBackground)
    {
        element->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
    }
    return element;
}

Grid^ PrimitiveTests::MakeGrid(bool hasBackground)
{
    const auto& element = ref new Grid();
    element->Width = 100;
    element->Height = 20;
    element->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
    element->BorderThickness = ThicknessHelper::FromUniformLength(8.0);
    if (hasBackground)
    {
        element->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
    }
    return element;
}

ContentPresenter^ PrimitiveTests::MakeContentPresenter(bool hasBackground)
{
    const auto& element = ref new ContentPresenter();
    element->Width = 100;
    element->Height = 20;
    element->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));
    element->BorderThickness = ThicknessHelper::FromUniformLength(8.0);
    if (hasBackground)
    {
        element->Background = ref new SolidColorBrush(ColorHelper::FromArgb(255, 0, 255, 0));
    }
    return element;
}

void PrimitiveTests::MaskNineGridWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    StackPanel^ root;
    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        wh->WindowContent = root;

        // Border only, constant thickness
        const auto& borderOnly1 = MakeBorder(false);
        const auto& borderOnly2 = MakeGrid(false);
        const auto& borderOnly3 = MakeContentPresenter(false);
        root->Children->Append(borderOnly1);
        root->Children->Append(borderOnly2);
        root->Children->Append(borderOnly3);

        // Border + background, variable thickness
        const auto& borderVariableThickness1 = MakeBorder(true);
        const auto& borderVariableThickness2 = MakeGrid(true);
        const auto& borderVariableThickness3 = MakeContentPresenter(true);
        borderVariableThickness1->BorderThickness = Thickness({2, 4, 6, 8});
        borderVariableThickness2->BorderThickness = Thickness({2, 4, 6, 8});
        borderVariableThickness3->BorderThickness = Thickness({2, 4, 6, 8});
        root->Children->Append(borderVariableThickness1);
        root->Children->Append(borderVariableThickness2);
        root->Children->Append(borderVariableThickness3);

        // Rounded border, the inside of the stroke is square
        const auto& roundedBorder1 = MakeBorder(false);
        const auto& roundedBorder2 = MakeGrid(false);
        const auto& roundedBorder3 = MakeContentPresenter(false);
        roundedBorder1->CornerRadius = CornerRadius({1, 2, 3, 4});
        roundedBorder2->CornerRadius = CornerRadius({1, 2, 3, 4});
        roundedBorder3->CornerRadius = CornerRadius({1, 2, 3, 4});
        root->Children->Append(roundedBorder1);
        root->Children->Append(roundedBorder2);
        root->Children->Append(roundedBorder3);

        // Rounded border + background, the inside of the stroke is rounded
        const auto& roundedBorderInnerRadius1 = MakeBorder(true);
        const auto& roundedBorderInnerRadius2 = MakeGrid(true);
        const auto& roundedBorderInnerRadius3 = MakeContentPresenter(true);
        roundedBorderInnerRadius1->BorderThickness = ThicknessHelper::FromUniformLength(2.0);
        roundedBorderInnerRadius2->BorderThickness = ThicknessHelper::FromUniformLength(2.0);
        roundedBorderInnerRadius3->BorderThickness = ThicknessHelper::FromUniformLength(2.0);
        roundedBorderInnerRadius1->CornerRadius = CornerRadius({4, 6, 8, 10});
        roundedBorderInnerRadius2->CornerRadius = CornerRadius({4, 6, 8, 10});
        roundedBorderInnerRadius3->CornerRadius = CornerRadius({4, 6, 8, 10});
        root->Children->Append(roundedBorderInnerRadius1);
        root->Children->Append(roundedBorderInnerRadius2);
        root->Children->Append(roundedBorderInnerRadius3);

        // Scaled mask
        ScaleTransform^ scale3x = ref new ScaleTransform();
        scale3x->ScaleX = 3;
        scale3x->ScaleY = 3;
        Border^ scaled = ref new Border();
        scaled->RenderTransform = scale3x;
        root->Children->Append(scaled);
        const auto& roundedBorderInnerRadius4 = MakeBorder(true);
        roundedBorderInnerRadius4->BorderThickness = ThicknessHelper::FromUniformLength(2.0);
        roundedBorderInnerRadius4->CornerRadius = CornerRadius({4, 6, 8, 10});
        scaled->Child = roundedBorderInnerRadius4;
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

    RunOnUIThread([&]()
    {
        ScaleTransform^ scale = ref new ScaleTransform();
        scale->ScaleX = 1.25;
        scale->ScaleY = 1.25;
        root->RenderTransform = scale;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"scaled");
}

Image^ PrimitiveTests::MakeImage(BitmapImage^ source, double width, double height, Stretch stretch)
{
    const auto& image = ref new Image();
    image->Margin = ThicknessHelper::FromUniformLength(10);
    image->Source = source;
    image->Width = width;
    image->Height = height;
    image->Stretch = stretch;
    image->NineGrid = Thickness({10, 10, 20, 20});
    return image;
}

StackPanel^ PrimitiveTests::MakeImagePanel(BitmapImage^ bitmapImage, Stretch stretch)
{
    const auto& stackPanel = ref new StackPanel();

    stackPanel->Children->Append(MakeImage(bitmapImage, 80, 80, stretch));      // natural size
    stackPanel->Children->Append(MakeImage(bitmapImage, 160, 160, stretch));    // 2x wide, 2x tall
    stackPanel->Children->Append(MakeImage(bitmapImage, 160, 40, stretch));     // 2x wide, 1/2 tall
    stackPanel->Children->Append(MakeImage(bitmapImage, 40, 160, stretch));     // 1/2 wide, 2x tall
    stackPanel->Children->Append(MakeImage(bitmapImage, 40, 40, stretch));      // 1/2 wide, 1/2 tall
    stackPanel->Children->Append(MakeImage(bitmapImage, 20, 20, stretch));      // smaller than tne ninegrid

    return stackPanel;
}

void PrimitiveTests::BrushNineGrid(float windowScaleOverride)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    wh->SetWindowSizeOverrideWithWindowScale(wf::Size(180 * 4 * windowScaleOverride, 620 * windowScaleOverride), windowScaleOverride);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    StackPanel^ root;
    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        root->Orientation = Orientation::Horizontal;
        wh->WindowContent = root;

        const auto& uri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\80x80Checker.png");

        const auto& bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = uri;

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        root->Children->Append(MakeImagePanel(bitmapImage, Stretch::None));
        root->Children->Append(MakeImagePanel(bitmapImage, Stretch::Fill));
        root->Children->Append(MakeImagePanel(bitmapImage, Stretch::Uniform));
        root->Children->Append(MakeImagePanel(bitmapImage, Stretch::UniformToFill));
    });

    bitmapImageOpenedEvent->WaitForDefault();

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void PrimitiveTests::BrushNineGridWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /* resizeWindow */);
    BrushNineGrid();
}

void PrimitiveTests::BrushNineGridHighDPIWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /* resizeWindow */);

    // Make sure the NineGridBrush's Source Brush gets Stretch mode applied
    BrushNineGrid(1.123456789f);
}

xaml_shapes::Rectangle^ PrimitiveTests::MakeRectangle(BitmapImage^ source, double width, double height, Stretch stretch)
{
    const auto& imageBrush = ref new ImageBrush();
    imageBrush->ImageSource = source;
    imageBrush->Stretch = stretch;

    const auto& rectangle = ref new xaml_shapes::Rectangle();
    rectangle->Margin = ThicknessHelper::FromUniformLength(10);
    rectangle->Fill = imageBrush;
    rectangle->Width = width;
    rectangle->Height = height;
    return rectangle;
}

StackPanel^ PrimitiveTests::MakeRectanglePanel(BitmapImage^ bitmapImage, Stretch stretch)
{
    const auto& stackPanel = ref new StackPanel();

    stackPanel->Children->Append(MakeRectangle(bitmapImage, 80, 80, stretch));      // natural size
    stackPanel->Children->Append(MakeRectangle(bitmapImage, 160, 160, stretch));    // 2x wide, 2x tall
    stackPanel->Children->Append(MakeRectangle(bitmapImage, 160, 40, stretch));     // 2x wide, 1/2 tall
    stackPanel->Children->Append(MakeRectangle(bitmapImage, 40, 160, stretch));     // 1/2 wide, 2x tall
    stackPanel->Children->Append(MakeRectangle(bitmapImage, 40, 40, stretch));      // 1/2 wide, 1/2 tall

    return stackPanel;
}

void PrimitiveTests::ImageBrushStretch()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    wh->SetWindowSizeOverride(wf::Size(180 * 4, 580));

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    StackPanel^ root;
    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        root->Orientation = Orientation::Horizontal;
        wh->WindowContent = root;

        const auto& uri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\80x80Checker.png");

        const auto& bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = uri;

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        root->Children->Append(MakeRectanglePanel(bitmapImage, Stretch::None));
        root->Children->Append(MakeRectanglePanel(bitmapImage, Stretch::Fill));
        root->Children->Append(MakeRectanglePanel(bitmapImage, Stretch::Uniform));
        root->Children->Append(MakeRectanglePanel(bitmapImage, Stretch::UniformToFill));
    });

    bitmapImageOpenedEvent->WaitForDefault();

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void PrimitiveTests::ImageBrushStretchWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /* resizeWindow */);
    ImageBrushStretch();
}

void PrimitiveTests::RecyclePrimitivesWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    StackPanel^ root;
    Border^ wucColor;
    Border^ wucGradient;
    Border^ wucSurface;
    Border^ wucNineGridColorSource;
    Border^ wucNineGridGradientSource;
    Image^ wucNineGridSurfaceSource;
    xaml_shapes::Ellipse^ wucMaskColorSource;
    xaml_shapes::Ellipse^ wucMaskGradientSource;
    xaml_shapes::Ellipse^ wucMaskSurfaceSource;
    Border^ wucNineGridMaskColorSource;

    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        wh->WindowContent = root;

        const auto& solidColorBrush = ref new SolidColorBrush(ColorHelper::FromArgb(255, 255, 0, 0));

        const auto& linearGradientBrush = MakeLinearGradientBrush(0.0f, 1.0f);

        const auto& uri = ref new Uri(GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\80x80Checker.png");

        const auto& bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = uri;

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        const auto& imageBrush = ref new ImageBrush();
        imageBrush->ImageSource = bitmapImage;

        // WUC color brush
        wucColor = MakeEmptyBorder();
        wucColor->Background = solidColorBrush;
        root->Children->Append(wucColor);

        // WUC linear gradient brush
        wucGradient = MakeEmptyBorder();
        wucGradient->Background = linearGradientBrush;
        root->Children->Append(wucGradient);

        // WUC surface brush
        wucSurface = MakeEmptyBorder();
        wucSurface->Background = imageBrush;
        root->Children->Append(wucSurface);

        // WUC nine grid brush, solid color source
        wucNineGridColorSource = MakeEmptyBorder();
        wucNineGridColorSource->BorderBrush = solidColorBrush;
        wucNineGridColorSource->BorderThickness = ThicknessHelper::FromUniformLength(5.0);
        root->Children->Append(wucNineGridColorSource);

        // Only SolidColorBrush borders can use nine grids. Linear gradient borders will go down the mask code path.

        // WUC nine grid brush, surface source
        wucNineGridSurfaceSource = MakeImage(bitmapImage, 100, 40, Stretch::Fill);
        root->Children->Append(wucNineGridSurfaceSource);

        // WUC mask brush, solid color source
        wucMaskColorSource = ref new xaml_shapes::Ellipse();
        wucMaskColorSource->Width = 100;
        wucMaskColorSource->Height = 20;
        wucMaskColorSource->Fill = solidColorBrush;
        root->Children->Append(wucMaskColorSource);

        // WUC mask brush, linear gradient source
        wucMaskGradientSource = ref new xaml_shapes::Ellipse();
        wucMaskGradientSource->Width = 100;
        wucMaskGradientSource->Height = 20;
        wucMaskGradientSource->Fill = linearGradientBrush;
        root->Children->Append(wucMaskGradientSource);

        // WUC mask brush, surface source
        wucMaskSurfaceSource = ref new xaml_shapes::Ellipse();
        wucMaskSurfaceSource->Width = 100;
        wucMaskSurfaceSource->Height = 20;
        wucMaskSurfaceSource->Fill = imageBrush;
        root->Children->Append(wucMaskSurfaceSource);

        // WUC nine grid mask brush, solid color source
        wucNineGridMaskColorSource = MakeEmptyBorder();
        wucNineGridMaskColorSource->BorderBrush = solidColorBrush;
        wucNineGridMaskColorSource->CornerRadius = CornerRadiusHelper::FromUniformRadius(2);
        wucNineGridMaskColorSource->BorderThickness = ThicknessHelper::FromUniformLength(5.0);
        root->Children->Append(wucNineGridMaskColorSource);

        // WUC nine grid mask brush requires a solid color border, so there's no nine grid mask brush with a surface source

        Canvas::SetZIndex(wucColor, 1);
        Canvas::SetZIndex(wucGradient, 2);
        Canvas::SetZIndex(wucSurface, 3);
        Canvas::SetZIndex(wucNineGridColorSource, 11);
        Canvas::SetZIndex(wucNineGridSurfaceSource, 12);
        Canvas::SetZIndex(wucMaskColorSource, 21);
        Canvas::SetZIndex(wucMaskGradientSource, 22);
        Canvas::SetZIndex(wucMaskSurfaceSource, 23);
        Canvas::SetZIndex(wucNineGridMaskColorSource, 31);

        // Expect Color, Gradient, Surface, NGColor, NGSurface, MColor, MGradient, MSurface, NGMColor
    });

    bitmapImageOpenedEvent->WaitForDefault();

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "1");

    RunOnUIThread([&]()
    {
        // Flip solid color with surface
        Canvas::SetZIndex(wucColor, 2);
        Canvas::SetZIndex(wucGradient, 3);
        Canvas::SetZIndex(wucSurface, 1);
        // Flip the nine grids
        Canvas::SetZIndex(wucNineGridColorSource, 12);
        Canvas::SetZIndex(wucNineGridSurfaceSource, 11);
        // Flip the mask brushes
        Canvas::SetZIndex(wucMaskColorSource, 22);
        Canvas::SetZIndex(wucMaskGradientSource, 23);
        Canvas::SetZIndex(wucMaskSurfaceSource, 31);
        Canvas::SetZIndex(wucNineGridMaskColorSource, 21);

        // Expect Surface, Color, Gradient, NGSurface, NGColor, NGMColor, MColor, MGradient, MSurface
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "2");

    RunOnUIThread([&]()
    {
        // Shuffle everything
        Canvas::SetZIndex(wucColor, 11);
        Canvas::SetZIndex(wucGradient, 21);
        Canvas::SetZIndex(wucSurface, 31);
        Canvas::SetZIndex(wucNineGridColorSource, 1);
        Canvas::SetZIndex(wucNineGridSurfaceSource, 22);
        Canvas::SetZIndex(wucMaskColorSource, 2);
        Canvas::SetZIndex(wucMaskGradientSource, 23);
        Canvas::SetZIndex(wucMaskSurfaceSource, 12);
        Canvas::SetZIndex(wucNineGridMaskColorSource, 3);

        // Expect NGColor, MColor, NGMColor, Color, MSurface, Gradient, NGSurface, MGradient, Surface
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "3");

}

LinearGradientBrush^ PrimitiveTests::MakeLinearGradientBrush(float startX, float endX)
{
    GradientStop^ stop1 = ref new GradientStop();
    stop1->Color = Microsoft::UI::Colors::Green;
    stop1->Offset = 0.2;

    GradientStop^ stop2 = ref new GradientStop();
    stop2->Color = Microsoft::UI::Colors::Red;
    stop2->Offset = 0.8;

    GradientStopCollection^ stopCollection = ref new GradientStopCollection();
    stopCollection->Append(stop1);
    stopCollection->Append(stop2);

    LinearGradientBrush^ brush = ref new LinearGradientBrush();
    brush->StartPoint = ::Windows::Foundation::Point(startX, 0);
    brush->EndPoint = ::Windows::Foundation::Point(endX, 0);
    brush->GradientStops = stopCollection;
    return brush;
}

void PrimitiveTests::LinearGradientWUCFull()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    StackPanel^ root;
    LinearGradientBrush^ linearGradientBrush;
    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        wh->WindowContent = root;

        linearGradientBrush = MakeLinearGradientBrush(0.0f, 1.0f);

        const auto& border1 = MakeEmptyBorder();
        border1->Background = linearGradientBrush;
        root->Children->Append(border1);

        const auto& ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 300;
        ellipse->Height = 20;
        ellipse->Fill = linearGradientBrush;
        root->Children->Append(ellipse);

        const auto& border2 = MakeEmptyBorder();
        border2->Width = 150;
        border2->BorderBrush = linearGradientBrush;
        border2->BorderThickness = ThicknessHelper::FromUniformLength(5.0);
        root->Children->Append(border2);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Updating a gradient stop and replacing another one");
        linearGradientBrush->GradientStops->GetAt(0)->Color = Microsoft::UI::Colors::Yellow;

        linearGradientBrush->GradientStops->RemoveAt(1);

        GradientStop^ stop = ref new GradientStop();
        stop->Color = Microsoft::UI::Colors::Blue;
        stop->Offset = 0.65;
        linearGradientBrush->GradientStops->Append(stop);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& extend = MakeLinearGradientBrush(0.4f, 0.6f);
        const auto& border1 = MakeEmptyBorder();
        border1->Background = extend;
        root->Children->Append(border1);

        const auto& flip = MakeLinearGradientBrush(0.4f, 0.6f);
        flip->SpreadMethod = GradientSpreadMethod::Reflect;
        const auto& border2 = MakeEmptyBorder();
        border2->Background = flip;
        root->Children->Append(border2);

        const auto& tile = MakeLinearGradientBrush(0.4f, 0.6f);
        tile->SpreadMethod = GradientSpreadMethod::Repeat;
        const auto& border3 = MakeEmptyBorder();
        border3->Background = tile;
        root->Children->Append(border3);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Spread");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& translate = ref new TranslateTransform();
        translate->X = 40;
        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->Transform = translate;

        const auto& border1 = MakeEmptyBorder();
        border1->Height = 100;
        border1->Background = gradient1;
        root->Children->Append(border1);

        const auto& scale = ref new ScaleTransform();
        scale->ScaleX = 0.5;
        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->Transform = scale;

        const auto& border2 = MakeEmptyBorder();
        border2->Height = 100;
        border2->Background = gradient2;
        root->Children->Append(border2);

        const auto& rotate = ref new RotateTransform();
        rotate->Angle = 90;
        const auto& gradient3 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient3->Transform = rotate;

        const auto& border3 = MakeEmptyBorder();
        border3->Height = 100;
        border3->Background = gradient3;
        root->Children->Append(border3);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Transform");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& translate = ref new TranslateTransform();
        translate->X = -0.4;
        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->RelativeTransform = translate;

        const auto& border1 = MakeEmptyBorder();
        border1->Width = 300;
        border1->Background = gradient1;
        root->Children->Append(border1);

        const auto& scale = ref new ScaleTransform();
        scale->ScaleX = -0.5;
        scale->CenterX = 0.5;
        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->RelativeTransform = scale;

        const auto& border2 = MakeEmptyBorder();
        border2->Height = 50;
        border2->Background = gradient2;
        root->Children->Append(border2);

        const auto& rotate = ref new RotateTransform();
        rotate->Angle = -90;
        rotate->CenterY = 1;
        const auto& thenTranslate = ref new TranslateTransform();
        thenTranslate->Y = 25;
        const auto& gradient3 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient3->RelativeTransform = rotate;
        gradient3->Transform = thenTranslate;

        const auto& border3 = MakeEmptyBorder();
        border3->Height = 100;
        border3->Background = gradient3;
        root->Children->Append(border3);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"RelativeTransform");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& gradient = MakeLinearGradientBrush(20.0f, 80.0f);
        gradient->MappingMode = BrushMappingMode::Absolute;

        const auto& border1 = MakeEmptyBorder();
        border1->Width = 100;
        border1->Background = gradient;
        root->Children->Append(border1);

        const auto& border2 = MakeEmptyBorder();
        border2->Width = 300;
        border2->Background = gradient;
        root->Children->Append(border2);

        const auto& border3 = MakeEmptyBorder();
        border3->Width = 60;
        border3->Background = gradient;
        root->Children->Append(border3);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"MappingMode");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->ColorInterpolationMode = ColorInterpolationMode::SRgbLinearInterpolation;

        const auto& border1 = MakeEmptyBorder();
        border1->Width = 100;
        border1->Background = gradient1;
        root->Children->Append(border1);

        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->ColorInterpolationMode = ColorInterpolationMode::ScRgbLinearInterpolation;

        const auto& border2 = MakeEmptyBorder();
        border2->Width = 100;
        border2->Background = gradient2;
        root->Children->Append(border2);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ColorInterpolationMode");
}

TextBlock^ PrimitiveTests::MakeTextBlock(Brush^ foreground)
{
    const auto& textBlock = ref new TextBlock();
    textBlock->FontSize = 18;
    textBlock->Text = "TextBlock";
    textBlock->Foreground = foreground;
    return textBlock;
}

TextBlock^ PrimitiveTests::MakeTextBlock2Runs(Brush^ foreground)
{
    const auto& run1 = ref new Run();
    run1->Text = " Text ";
    run1->Foreground = foreground;

    const auto& run2 = ref new Run();
    run2->Text = " Block ";
    run2->Foreground = foreground;

    const auto& textBlock = ref new TextBlock();
    textBlock->FontSize = 18;
    textBlock->Inlines->Append(run1);
    textBlock->Inlines->Append(run2);
    return textBlock;
}

RichTextBlock^ PrimitiveTests::MakeRichTextBlock(Brush^ foreground)
{
    const auto& run = ref new Run();
    run->Text = "RichTextBlock";
    run->Foreground = foreground;

    const auto& paragraph = ref new Paragraph();
    paragraph->Inlines->Append(run);

    const auto& richTextBlock = ref new RichTextBlock();
    richTextBlock->FontSize = 18;
    richTextBlock->Blocks->Append(paragraph);
    return richTextBlock;
}

RichTextBlock^ PrimitiveTests::MakeRichTextBlock3Runs(Brush^ foreground)
{
    const auto& run1 = ref new Run();
    run1->Text = " Rich ";
    run1->Foreground = foreground;

    const auto& run2 = ref new Run();
    run2->Text = " Text ";
    run2->Foreground = foreground;

    const auto& run3 = ref new Run();
    run3->Text = " Block ";
    run3->Foreground = foreground;

    const auto& paragraph = ref new Paragraph();
    paragraph->Inlines->Append(run1);
    paragraph->Inlines->Append(run2);
    paragraph->Inlines->Append(run3);

    const auto& richTextBlock = ref new RichTextBlock();
    richTextBlock->FontSize = 18;
    richTextBlock->Blocks->Append(paragraph);
    return richTextBlock;
}

void PrimitiveTests::LinearGradientText()
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    StackPanel^ root;
    LinearGradientBrush^ linearGradientBrush;
    RunOnUIThread([&]()
    {
        root = ref new StackPanel();
        root->Width = 200;
        wh->WindowContent = root;

        linearGradientBrush = MakeLinearGradientBrush(0.0f, 1.0f);

        const auto& rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 200;
        rectangle->Height = 50;
        rectangle->Fill = linearGradientBrush;
        root->Children->Append(rectangle);

        root->Children->Append(MakeTextBlock(linearGradientBrush));
        root->Children->Append(MakeTextBlock2Runs(linearGradientBrush));
        root->Children->Append(MakeRichTextBlock(linearGradientBrush));
        root->Children->Append(MakeRichTextBlock3Runs(linearGradientBrush));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Updating a gradient stop and replacing another one");
        linearGradientBrush->GradientStops->GetAt(0)->Color = Microsoft::UI::Colors::Yellow;

        linearGradientBrush->GradientStops->RemoveAt(1);

        GradientStop^ stop = ref new GradientStop();
        stop->Color = Microsoft::UI::Colors::Blue;
        stop->Offset = 0.65;
        linearGradientBrush->GradientStops->Append(stop);
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& extend = MakeLinearGradientBrush(0.4f, 0.6f);
        root->Children->Append(MakeTextBlock(extend));
        root->Children->Append(MakeTextBlock2Runs(extend));
        root->Children->Append(MakeRichTextBlock(extend));
        root->Children->Append(MakeRichTextBlock3Runs(extend));

        const auto& flip = MakeLinearGradientBrush(0.4f, 0.6f);
        flip->SpreadMethod = GradientSpreadMethod::Reflect;
        root->Children->Append(MakeTextBlock(flip));
        root->Children->Append(MakeTextBlock2Runs(flip));
        root->Children->Append(MakeRichTextBlock(flip));
        root->Children->Append(MakeRichTextBlock3Runs(flip));

        const auto& tile = MakeLinearGradientBrush(0.4f, 0.6f);
        tile->SpreadMethod = GradientSpreadMethod::Repeat;
        root->Children->Append(MakeTextBlock(tile));
        root->Children->Append(MakeTextBlock2Runs(tile));
        root->Children->Append(MakeRichTextBlock(tile));
        root->Children->Append(MakeRichTextBlock3Runs(tile));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Spread");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& translate = ref new TranslateTransform();
        translate->X = 40;
        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->Transform = translate;

        root->Children->Append(MakeTextBlock(gradient1));
        root->Children->Append(MakeTextBlock2Runs(gradient1));
        root->Children->Append(MakeRichTextBlock(gradient1));
        root->Children->Append(MakeRichTextBlock3Runs(gradient1));

        const auto& scale = ref new ScaleTransform();
        scale->ScaleX = 0.5;
        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->Transform = scale;

        root->Children->Append(MakeTextBlock(gradient2));
        root->Children->Append(MakeTextBlock2Runs(gradient2));
        root->Children->Append(MakeRichTextBlock(gradient2));
        root->Children->Append(MakeRichTextBlock3Runs(gradient2));

        const auto& rotate = ref new RotateTransform();
        rotate->Angle = 90;
        const auto& gradient3 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient3->Transform = rotate;

        root->Children->Append(MakeTextBlock(gradient3));
        root->Children->Append(MakeTextBlock2Runs(gradient3));
        root->Children->Append(MakeRichTextBlock(gradient3));
        root->Children->Append(MakeRichTextBlock3Runs(gradient3));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Transform");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& translate = ref new TranslateTransform();
        translate->X = -0.4;
        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->RelativeTransform = translate;

        root->Children->Append(MakeTextBlock(gradient1));
        root->Children->Append(MakeTextBlock2Runs(gradient1));
        root->Children->Append(MakeRichTextBlock(gradient1));
        root->Children->Append(MakeRichTextBlock3Runs(gradient1));

        const auto& scale = ref new ScaleTransform();
        scale->ScaleX = -0.5;
        scale->CenterX = 0.5;
        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->RelativeTransform = scale;

        root->Children->Append(MakeTextBlock(gradient2));
        root->Children->Append(MakeTextBlock2Runs(gradient2));
        root->Children->Append(MakeRichTextBlock(gradient2));
        root->Children->Append(MakeRichTextBlock3Runs(gradient2));

        const auto& rotate = ref new RotateTransform();
        rotate->Angle = -90;
        rotate->CenterY = 1;
        const auto& thenTranslate = ref new TranslateTransform();
        thenTranslate->Y = 25;
        const auto& gradient3 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient3->RelativeTransform = rotate;
        gradient3->Transform = thenTranslate;

        root->Children->Append(MakeTextBlock(gradient3));
        root->Children->Append(MakeTextBlock2Runs(gradient3));
        root->Children->Append(MakeRichTextBlock(gradient3));
        root->Children->Append(MakeRichTextBlock3Runs(gradient3));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"RelativeTransform");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& gradient = MakeLinearGradientBrush(20.0f, 80.0f);
        gradient->MappingMode = BrushMappingMode::Absolute;

        root->Children->Append(MakeTextBlock(gradient));
        root->Children->Append(MakeTextBlock2Runs(gradient));
        root->Children->Append(MakeRichTextBlock(gradient));
        root->Children->Append(MakeRichTextBlock3Runs(gradient));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"MappingMode");

    RunOnUIThread([&]()
    {
        root->Children->Clear();

        const auto& gradient1 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient1->ColorInterpolationMode = ColorInterpolationMode::SRgbLinearInterpolation;

        root->Children->Append(MakeTextBlock(gradient1));
        root->Children->Append(MakeTextBlock2Runs(gradient1));
        root->Children->Append(MakeRichTextBlock(gradient1));
        root->Children->Append(MakeRichTextBlock3Runs(gradient1));

        const auto& gradient2 = MakeLinearGradientBrush(0.1f, 0.9f);
        gradient2->ColorInterpolationMode = ColorInterpolationMode::ScRgbLinearInterpolation;

        root->Children->Append(MakeTextBlock(gradient2));
        root->Children->Append(MakeTextBlock2Runs(gradient2));
        root->Children->Append(MakeRichTextBlock(gradient2));
        root->Children->Append(MakeRichTextBlock3Runs(gradient2));
    });

    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"ColorInterpolationMode");
}

void PrimitiveTests::LinearGradientTextWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    LinearGradientText();
}

void PrimitiveTests::RegressionTest_11908060()
{
    const auto& wh = TestServices::WindowHelper;

    Canvas^ root;
    Border^ border;
    RunOnUIThread([&]()
    {
        root = ref new Canvas();
        border = ref new Border();
        border->Width = 200;
        border->Height = 200;
        border->CornerRadius = CornerRadiusHelper::FromUniformRadius(5);
        border->BorderThickness = ThicknessHelper::FromUniformLength(5);
        border->BorderBrush = ref new SolidColorBrush(Microsoft::UI::Colors::Black);
        root->Children->Append(border);
        wh->WindowContent = root;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        // Now change the border to a smaller size, this will test calling BaseContentRenderer::MaskCombinedRenderHelper()
        // during the pre-children phase of rendering with an already-present HW texture for the stroke.
        // In this case we're verifying we don't use this texture in the call to MaskRasterizeSoftware() and crash.
        border->Width = 10;
        border->Height = 10;
    });
    wh->WaitForIdle();
}

} } } } } }
