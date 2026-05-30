// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "ImageTests.h"
#include "ImageTestEngine.h"
#include "ETWWaiterProxy.h"
#include "ImageEventWaitingContext.h"
#include "HdrOutputOverrideHelper.h"
#include <WUCRenderingScopeGuard.h>
#include <XamlFileTestEngine.h>
#include <collection.h>
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <robuffer.h>
#include <MUX-ETWEvents.h>

#undef GetClassName // Conflicts with automation peer method of the same name

using namespace Concurrency;
using namespace ::Windows::Foundation;
using namespace ::Windows::Graphics::Imaging;
using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace ::Windows::Media::Casting;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics::Image;

Platform::String^ ImageTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool ImageTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool ImageTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool ImageTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

// Test case: Renders a simple image element.
void ImageTests::SimpleImageElement()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Fill;

        auto bitmapImage = ref new BitmapImage();
        VERIFY_IS_NOT_NULL(bitmapImage);

        testImage->Source = bitmapImage;

        auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
        VERIFY_IS_NOT_NULL(testUri);

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        bitmapImage->UriSource = testUri;
    });
    bitmapImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::MultiImagesSameUriFirstFrame()
{
    MultipleImagesTestHelper(
        GetResourcesPath() + L"TwoImages.xaml",
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
        MockDComp::SurfaceIdMode::XmlOrder
        );
}

void ImageTests::MultiImageSameUriDiffSizeFirstFrame()
{
    MultipleImagesTestHelper(
        GetResourcesPath() + L"TwoImagesDifferentSizes.xaml",
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
        MockDComp::SurfaceIdMode::XmlOrder
        );
}

// Renders a simple image element using a relative path to reference the image.
void ImageTests::SimpleImageElementRelativePath()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithUri.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmpImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Fill;

        auto bitmapImage = safe_cast<BitmapImage^>(testImage->Source);

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmpImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmpImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    bitmpImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::ExplicitXaml()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImageWithUriExplicitSyntax.xaml"));

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmpImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Fill;

        auto bitmapImage = safe_cast<BitmapImage^>(testImage->Source);

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmpImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmpImageOpenedEvent->Set();
        }));

        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    bitmpImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::PlateauScaleChange()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    ::Windows::Foundation::Size size(400, 300);
    TestServices::WindowHelper->SetWindowSizeOverride(size);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));

        testImage->Width = 100;
        testImage->Height = 100;

        testImage->Stretch = Stretch::Fill;

        auto bitmapImage = ref new BitmapImage();

        // Must set logical pixel size for it to register with the reload manager on plateau scale changes.
        bitmapImage->DecodePixelWidth = 100;
        bitmapImage->DecodePixelHeight = 100;
        bitmapImage->DecodePixelType = DecodePixelType::Logical;

        testImage->Source = bitmapImage;

        auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
        VERIFY_IS_NOT_NULL(testUri);

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            VERIFY_IS_FALSE(bitmapImageOpenedEvent->HasFired());
            bitmapImageOpenedEvent->Set();
        }));

        bitmapImage->UriSource = testUri;
    });
    bitmapImageOpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    // Update the plateau scale and tick the UI thread.
    LOG_OUTPUT(L"Set the new plateau scale");
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.4f);
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::WriteableBitmapInvalidate()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Uniform;

        auto writeableBitmap = CreateSolidColorWriteableBitmap(100, 100, 0xFFFF0000);

        writeableBitmap->Invalidate();

        testImage->Source = writeableBitmap;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::WriteableBitmapNoInvalidate()
{
    // Applications should call WriteableBitmap::Invalidate to indicate contents are updated
    // However, XAML has been forgiving of this, so check that it works without Invalidate
    // for the initial setup scenario for app compat cases.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        testImage->Stretch = Stretch::Uniform;

        auto writeableBitmap = CreateSolidColorWriteableBitmap(100, 100, 0xFFFF0000);

        testImage->Source = writeableBitmap;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::BaseImageTestEngineInternal(DCompRendering rendering)
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = rendering;
    pEngine->Execute();
}

void ImageTests::BaseImageTestEngineWUCFull()
{
    BaseImageTestEngineInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::SimpleCRCCheck()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    pEngine->MockDCompSurfaceIdMode = MockDComp::SurfaceIdMode::CRC;

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::BmpImage()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"Smiley.bmp";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::PngImage()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"Windows.png";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::JxrImage()
{
    HdrOutputOverrideHelper hdrOutputOverrideHelper;

    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    pEngine->MockDCompVerification = MockDComp::SurfaceComparison::ReferencedOnlyCRC;

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"journey.jxr";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::JxrImageSDR()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"journey.jxr";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::ChangeHDRMode()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ImageEventWaitingContext eventWaitingContext;

    BitmapImage ^bitmapImage;

    RunOnUIThread([&]
    {
        bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"journey.jxr");

        auto image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;
        image->Source = bitmapImage;
    });

    eventWaitingContext.WaitOpened();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnlyCRC, "sdr");

    // Default test environment runs in SDR mode. Switch to HDR and expect the image to redecode.

    ETWWaiterProxy imageEtwWaiter;

    imageEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        ImageUpdateHardwareResourcesEnd_value);

    HdrOutputOverrideHelper hdrOutputOverrideHelper;

    imageEtwWaiter.WaitForDefault();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnlyCRC, "hdr");
}

void ImageTests::StretchMode()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    xaml_media::Stretch modes[] = {
        xaml_media::Stretch::None,
        xaml_media::Stretch::Fill,
        xaml_media::Stretch::Uniform,
        xaml_media::Stretch::UniformToFill,
    };

    for (xaml_media::Stretch& mode : modes)
    {
        auto pTestImage = ref new TestImage();
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
        pTestImage->Stretch = mode;

        pEngine->AddTestImage(pTestImage);
    }

    pEngine->Execute();
}

void ImageTests::StretchModeNoneLarger()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"barcelona.jpg";
    pTestImage->Stretch = xaml_media::Stretch::None;
    pTestImage->ElementSize = pEngine->WindowSize;

    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::DecodeToRenderSizeOff()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->DecodeToRenderSize = false;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::MultipleImages()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    Platform::String^ imagePaths[] = {
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
        GetResourcesPath() + L"Smiley.bmp",
        GetResourcesPath() + L"Windows.png",
    };

    for (Platform::String^& pImagePath : imagePaths)
    {
        auto pTestImage = ref new TestImage();
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->ImagePath = pImagePath;

        pEngine->AddTestImage(pTestImage);
    }

    pEngine->Execute();
}

void ImageTests::MultipleImagesSameUri()
{
    // Note: This test function does not load the image in the same frame since the
    //       test engine loads each image serially.
    //       Loading all images in the first frame is done in MultiImagesSameUriFirstFrame
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    Platform::String^ imagePaths[] = {
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
    };

    for (Platform::String^& pImagePath : imagePaths)
    {
        auto pTestImage = ref new TestImage();
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->ImagePath = pImagePath;

        pEngine->AddTestImage(pTestImage);
    }

    pEngine->Execute();
}

void ImageTests::MultipleImagesSameUriIgnoreCache()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    // Use allocation ordering so that caching behavior is observed
    pEngine->MockDCompSurfaceIdMode = MockDComp::SurfaceIdMode::AllocationOrder;

    Platform::String^ imagePaths[] = {
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
        GetResourcesPath() + L"Rainier_444_2048x1536.jpg",
    };

    for (Platform::String^& pImagePath : imagePaths)
    {
        auto pTestImage = ref new TestImage();
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->BitmapCreateOptions = xaml_imaging::BitmapCreateOptions::IgnoreImageCache;
        pTestImage->ImagePath = pImagePath;

        pEngine->AddTestImage(pTestImage);
    }

    pEngine->Execute();
}

void ImageTests::BitmapCacheWUCFull()
{
    BitmapCache(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::BitmapCache(DCompRendering dcompRendering)
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->BitmapCache = true;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = dcompRendering;

    pEngine->Execute();
}

void ImageTests::LoadStreamSync()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->LoadApi = TestImageEnums::LoadApi::SetSource;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";

    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::LoadStreamAsync()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->LoadApi = TestImageEnums::LoadApi::SetSourceAsync;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::LoadStreamAsync2()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->LoadApi = TestImageEnums::LoadApi::SetSourceAsyncTwice;
    pTestImage->ImagePath = GetResourcesPath() + L"animatedgif\\08bMStatic3.gif";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::StreamCleanupAndRestore()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // The image should be decoded twice to the same size (the rendered size)
    const int imageElementWidth = 100;
    const int imageElementHeight = 100;

    Platform::String^ etwValidationString =
        L"@DecodeWidth=" + imageElementWidth + L" AND " +
        L"@DecodeHeight=" + imageElementHeight;

    ETWWaiterProxy imageEtwWaiter;

    imageEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        DecodeToRenderSizeBegin_value,
        etwValidationString);

    pTestImage->LoadApi = TestImageEnums::LoadApi::SetSourceAsync;
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->ElementSize = wf::Size(
        static_cast<float>(imageElementWidth),
        static_cast<float>(imageElementHeight));
    pTestImage->ImagePath = GetResourcesPath() + L"Smiley.bmp";
    pTestImage->TrimAndRestoreHardwareResources = true;
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();

    imageEtwWaiter.WaitForDefault();
}

void ImageTests::Opacity50()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->Opacity = 0.5;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::Opacity0()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->Opacity = 0.0;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::DecodePixelWidth()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // Image should appear blurry
    pTestImage->DecodePixelWidth = 25;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::DecodePixelHeight()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // Image should appear blurry
    pTestImage->DecodePixelHeight = 25;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::DecodePixelWidthAndHeightInternal(DCompRendering rendering)
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // Image should appear blurry
    pTestImage->DecodePixelWidth = 25;
    pTestImage->DecodePixelHeight = 25;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = rendering;

    // Specifying DecodePixelWidth/DecodePixelHeight currently disables BackgroundThreadImageLoading
    // optimization.  This ETW check makes sure we release the intermediate software surface that
    // is handed back from the cache to the UI thread.
    ETWWaiterProxy imageEtwWaiter;

    imageEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        OfferableSoftwareBitmapFreeInfo_value);

    pEngine->Execute();

    imageEtwWaiter.WaitForDefault();
}

void ImageTests::DecodePixelWidthAndHeightWUCFull()
{
    DecodePixelWidthAndHeightInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::DecodePixelTypeLogical()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // Image should appear blurry
    pTestImage->DecodePixelWidth = 25;
    pTestImage->DecodePixelType = xaml_imaging::DecodePixelType::Logical;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::BorderElement()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ParentElement = TestImageEnums::ParentElement::Border;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";
    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::EllipseShapeInternal(DCompRendering rendering)
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ParentElement = TestImageEnums::ParentElement::Ellipse;
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";

    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = rendering;

    pEngine->Execute();
}

void ImageTests::EllipseShapeWUCFull()
{
    EllipseShapeInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::NineGrid()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->NineGrid = xaml::Thickness({ 10.0, 30.0, 20.0, 40.0 });
    pTestImage->ImagePath = GetResourcesPath() + L"Rainier_444_2048x1536.jpg";

    pEngine->AddTestImage(pTestImage);

    pEngine->Execute();
}

void ImageTests::NineGridNoSource()
{
    auto wh = TestServices::WindowHelper;

    RunOnUIThread([&]()
    {
        auto bitmapImage = ref new BitmapImage();
        // Don't set the source.

        auto testImage = ref new xaml_controls::Image();
        testImage->Width = 50;
        testImage->Height = 50;
        testImage->Source = bitmapImage;
        // Set a NineGrid on the Image. This disqualifies the BitmapImage from decoding to render size, which makes us
        // log an ETW event. Make sure that doesn't crash the process.
        testImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });

        auto rootCanvas = ref new Canvas();
        rootCanvas->Children->Append(testImage);
        wh->WindowContent = rootCanvas;
    });

    wh->WaitForIdle();
}

void ImageTests::VeryLargeImageSize()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"CN-EOS5DMarkII-ORI-BottomUp.jpg";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::VeryLargeImageSizeNineGridInternal(DCompRendering rendering)
{
    // The purpose of the ninegrid is to disable decode to render size.
    // TODO: Consider changing this to the DecodeToRenderSize setting.
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ImagePath = GetResourcesPath() + L"CN-EOS5DMarkII-ORI-BottomUp.jpg";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });
    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = rendering;
    pEngine->Execute();
}

void ImageTests::VeryLargeImageSizeNineGridWUCFull()
{
    VeryLargeImageSizeNineGridInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::VeryLarge_4640x168_NineGrid()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // This image is intended to invoke tiling (width > 2048) but have height smaller than a tile (height < 504)
    pTestImage->ImagePath = GetResourcesPath() + L"LargeHorizontalStrip_4640x168.jpg";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::VeryLarge_4640x504_NineGridInternal(DCompRendering rendering)
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // This image is intended to invoke tiling (width > 2048) but have height smaller exactly the tile size (height == 504)
    pTestImage->ImagePath = GetResourcesPath() + L"LargeHorizontalStrip_4640x504.jpg";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });
    pEngine->AddTestImage(pTestImage);
    pEngine->DCompRenderingMode = rendering;
    pEngine->Execute();
}

void ImageTests::VeryLarge_4640x504_NineGridWUCFull()
{
    VeryLarge_4640x504_NineGridInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::VeryLarge_168x3744_NineGrid()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // This image is intended to invoke tiling (height > 2048) but have width smaller than a tile (width < 504)
    pTestImage->ImagePath = GetResourcesPath() + L"LargeVerticalStrip_168x3744.jpg";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::VeryLarge_504x3725_NineGrid()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    // This image is intended to invoke tiling (height > 2048) but have width exactly the tile size (width == 504)
    pTestImage->ImagePath = GetResourcesPath() + L"LargeVerticalStrip_504x3725.JPG";
    pTestImage->Stretch = xaml_media::Stretch::Fill;
    pTestImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::VeryLarge_4640x168_Hardware_And_BitmapCacheWUCFull()
{
    VeryLarge_4640x168_Hardware_And_BitmapCache(DCompRendering::WUCCompleteSynchronousCompTree);
}

void ImageTests::VeryLarge_4640x168_Hardware_And_BitmapCache(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(dcompRendering);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto rootCanvas = ref new Canvas();
        wh->WindowContent = rootCanvas;

        auto bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"LargeHorizontalStrip_4640x168.jpg");

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        auto testImage = ref new xaml_controls::Image();
        VERIFY_IS_NOT_NULL(testImage);
        testImage->Source = bitmapImage;
        rootCanvas->Children->Append(testImage);

        auto testImageBitmapCache = ref new xaml_controls::Image();
        VERIFY_IS_NOT_NULL(testImageBitmapCache);
        testImageBitmapCache->Source = bitmapImage;
        testImageBitmapCache->CacheMode = ref new xaml_media::BitmapCache();
        rootCanvas->Children->Append(testImageBitmapCache);
    });

    bitmapImageOpenedEvent->WaitForDefault();
    wh->WaitForIdle();
    VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::InvalidImageInImageControl()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = GetResourcesPath() + L"InvalidImage.png";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::NotExistingImageInImageControl()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = GetResourcesPath() + L"DoesNotExist.png";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::NotExistingImageInImageControlMsAppx()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = L"ms-appx:///resources/native/external/foundation/graphics/DoesNotExist.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::InvalidImageInImageBrush()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ParentElement = TestImageEnums::ParentElement::Border;
    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = GetResourcesPath() + L"InvalidImage.png";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::NotExistingImageInImageBrush()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ParentElement = TestImageEnums::ParentElement::Border;
    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = GetResourcesPath() + L"DoesNotExist.png";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::NotExistingImageInImageBrushMsAppx()
{
    auto pEngine = ref new ImageTestEngine();
    VERIFY_IS_NOT_NULL(pEngine);

    auto pTestImage = ref new TestImage();
    VERIFY_IS_NOT_NULL(pTestImage);

    pTestImage->ParentElement = TestImageEnums::ParentElement::Border;
    pTestImage->FailureExpected = true;
    pTestImage->ImagePath = L"ms-appx:///resources/native/external/foundation/graphics/DoesNotExist.jpg";
    pEngine->AddTestImage(pTestImage);
    pEngine->Execute();
}

void ImageTests::InvalidImageClearsPreviousOne()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ImageEventWaitingContext eventWaitingContext;

    BitmapImage ^bitmapImage;

    RunOnUIThread([&]
    {
        bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"barcelona.jpg");

        auto image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;
        image->Source = bitmapImage;
    });

    eventWaitingContext.WaitOpened();
    eventWaitingContext.ResetState();

    RunOnUIThread([&]
    {
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"InvalidImage.png");
    });

    eventWaitingContext.WaitFailed();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::NotExistingImageClearsPreviousOne()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ImageEventWaitingContext eventWaitingContext;

    BitmapImage ^bitmapImage;

    RunOnUIThread([&]
    {
        bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"barcelona.jpg");

        auto image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;
        image->Source = bitmapImage;
    });

    eventWaitingContext.WaitOpened();
    eventWaitingContext.ResetState();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]
    {
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"DoesNotExist.png");
    });

    eventWaitingContext.WaitFailed();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
}

void ImageTests::NullUriClearsPreviousImage()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ImageEventWaitingContext eventWaitingContext;

    BitmapImage ^bitmapImage;

    RunOnUIThread([&]
    {
        bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"barcelona.jpg");

        auto image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;
        image->Source = bitmapImage;
    });

    eventWaitingContext.WaitOpened();
    TestServices::WindowHelper->WaitForIdle();

    eventWaitingContext.ResetState();

    RunOnUIThread([&]
    {
        bitmapImage->UriSource = nullptr;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::AutomationPeerDefault()
{
    TestCleanupWrapper cleanup;
    ImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;
    engine.SetMockDCompVerificationEnabled(false);

    engine.SetXamlFilePath(GetResourcesPath() + L"SimpleImage.xaml");
    engine.SetPostInitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));

        auto bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
        testImage->Source = bitmapImage;
    });
    engine.SetPostInitWaitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        eventWaitingContext.WaitOpened();
    });
    engine.SetValidationCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootElement->FindName(L"imageElement"));
        auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(testImage);
        VERIFY_IS_TRUE(automationPeer->GetClassName() == L"Image");
        VERIFY_IS_TRUE(automationPeer->GetName() == L"");
        VERIFY_IS_TRUE(automationPeer->GetFullDescription() == L"");
        VERIFY_IS_TRUE(automationPeer->GetAutomationControlType() == xaml_automation_peers::AutomationControlType::Image);
    });
    engine.Execute();
}

void ImageTests::AutomationPeerProperties()
{
    TestCleanupWrapper cleanup;
    ImageEventWaitingContext eventWaitingContext;
    XamlFileTestEngine engine;
    engine.SetMockDCompVerificationEnabled(false);

    engine.SetXamlFilePath(GetResourcesPath() + L"SimpleImageAutomation.xaml");
    engine.SetPostInitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootElement->FindName(L"imageElement"));

        auto bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
        testImage->Source = bitmapImage;
    });
    engine.SetPostInitWaitCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        eventWaitingContext.WaitOpened();
    });
    engine.SetValidationCallback([&] (Microsoft::UI::Xaml::FrameworkElement^ rootElement)
    {
        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootElement->FindName(L"imageElement"));
        auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::FromElement(testImage);
        VERIFY_IS_TRUE(automationPeer->GetClassName() == L"Image");
        VERIFY_IS_TRUE(automationPeer->GetName() == L"Simple Image Name");
        VERIFY_IS_TRUE(automationPeer->GetFullDescription() == L"Simple Image Description");
        VERIFY_IS_TRUE(automationPeer->GetAutomationControlType() == xaml_automation_peers::AutomationControlType::Image);
    });
    engine.Execute();
}

xaml_imaging::WriteableBitmap^ ImageTests::CreateSolidColorWriteableBitmap(int width, int height, uint32 colorValue)
{
    // TODO: Extend this in the future to create a WriteableBitmap
    //       from a set of bits or from a URI source through BitmapDecoder
    byte* pDstPixels = nullptr;
    auto dataWritten = std::make_shared<Event>();

    // Create the WriteableBitmap
    auto bitmap = ref new WriteableBitmap(width, height);

    // Get access to the pixels
    IBuffer^ buffer = bitmap->PixelBuffer;

    // Obtain IBufferByteAccess
    wrl::ComPtr<IBufferByteAccess> pBufferByteAccess;
    wrl::ComPtr<IUnknown> pBuffer((IUnknown*)buffer);
    pBuffer.As(&pBufferByteAccess);

    // Get pointer to pixel bytes
    pBufferByteAccess->Buffer(&pDstPixels);

    uint32* pCurrentDstPixel = reinterpret_cast<uint32*>(pDstPixels);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++, pCurrentDstPixel++)
        {
            *pCurrentDstPixel = colorValue;
        }
    }

    return bitmap;
}

// Creates an Image element and verifies that we can get the casting source from it.
void ImageTests::ImageElementCastingSource()
{
    TestCleanupWrapper cleanup;

    ::Windows::Foundation::Size size(400, 300);
    TestServices::WindowHelper->SetWindowSizeOverride(size);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmpImageOpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);

        // Verify that we can get a casting source from the image before any media is loaded
        CastingSource^ castingSource = testImage->GetAsCastingSource();
        VERIFY_IS_NOT_NULL(castingSource);

        auto bitmapImage = ref new BitmapImage();
        VERIFY_IS_NOT_NULL(bitmapImage);

        testImage->Source = bitmapImage;

        auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
        VERIFY_IS_NOT_NULL(testUri);

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmpImageOpenedEvent, castingSource, testImage](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");

            // Verify that we can get a casting source after the media is loaded,
            // and that it's the same casting source we get before the media was loaded
            CastingSource^ castingSource2 = testImage->GetAsCastingSource();
            VERIFY_IS_NOT_NULL(castingSource2);
            VERIFY_IS_TRUE(castingSource2 == castingSource);

            bitmpImageOpenedEvent->Set();
        }));

        bitmapImage->UriSource = testUri;
    });

    TestServices::WindowHelper->WaitForIdle();

    bitmpImageOpenedEvent->WaitForDefault();
    VERIFY_IS_TRUE(bitmpImageOpenedEvent->HasFired());
}

void ImageTests::MultipleImagesTestHelper(
    Platform::String^ pXamlPath,
    Platform::String^ pImagePath,
    MockDComp::SurfaceIdMode surfaceIdMode
    )
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(pXamlPath));
    VERIFY_IS_NOT_NULL(rootGrid);

    auto openedRegistration1 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImage1OpenedEvent = std::make_shared<Event>();
    auto bitmapImage2OpenedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto testImage1 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement1"));
        VERIFY_IS_NOT_NULL(testImage1);

        auto testImage2 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement2"));
        VERIFY_IS_NOT_NULL(testImage2);

        testImage1->Stretch = Stretch::Fill;
        testImage2->Stretch = Stretch::Fill;

        auto bitmapImage1 = ref new BitmapImage();
        VERIFY_IS_NOT_NULL(bitmapImage1);

        auto bitmapImage2 = ref new BitmapImage();
        VERIFY_IS_NOT_NULL(bitmapImage2);

        testImage1->Source = bitmapImage1;
        testImage2->Source = bitmapImage2;

        auto testUri = ref new Uri(pImagePath);
        VERIFY_IS_NOT_NULL(testUri);

        openedRegistration1.Attach(
            bitmapImage1,
            ref new xaml::RoutedEventHandler([bitmapImage1OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage1 Opened Event Fired");
            bitmapImage1OpenedEvent->Set();
        }));

        openedRegistration2.Attach(
            bitmapImage2,
            ref new xaml::RoutedEventHandler([bitmapImage2OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage2 Opened Event Fired");
            bitmapImage2OpenedEvent->Set();
        }));

        bitmapImage1->UriSource = testUri;
        bitmapImage2->UriSource = testUri;
    });
    bitmapImage1OpenedEvent->WaitForDefault();
    bitmapImage2OpenedEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->SetMockDCompSurfaceIdMode(surfaceIdMode);

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::DownloadProgressUriSource()
{
    DownloadProgressTestHelper(TestImageEnums::LoadApi::Uri);
}

void ImageTests::DownloadProgressStreamSource()
{
    DownloadProgressTestHelper(TestImageEnums::LoadApi::SetSource);
}

void ImageTests::DownloadProgressStreamSourceAsync()
{
    DownloadProgressTestHelper(TestImageEnums::LoadApi::SetSourceAsync);
}

void ImageTests::DownloadProgressTestHelper(TestImageEnums::LoadApi loadApi)
{
    TestCleanupWrapper cleanup;

    int lastProgress = -1;
    int downloadProgressCount = 0;
    auto downloadProgressReg = CreateSafeEventRegistration(BitmapImage, DownloadProgress);

    xaml_imaging::BitmapImage ^bitmapImage;
    RunOnUIThread([&]()
    {
        bitmapImage = ref new xaml_imaging::BitmapImage();
    });

    ImageEventWaitingContext waiter;
    waiter.Attach(bitmapImage);
    downloadProgressReg.Attach(
        bitmapImage,
        ref new xaml_imaging::DownloadProgressEventHandler(
            [&](Platform::Object^, xaml_imaging::DownloadProgressEventArgs ^args)
    {
        LOG_OUTPUT(L"DownloadProgress: %d", args->Progress);
        VERIFY_IS_FALSE(waiter.IsOpened());
        VERIFY_IS_TRUE(args->Progress > lastProgress);
        lastProgress = args->Progress;
        downloadProgressCount++;
    }));

    LoadHelper(bitmapImage, loadApi);

    RunOnUIThread([&]()
    {
        auto image = ref new xaml_controls::Image();
        image->Source = bitmapImage;
        TestServices::WindowHelper->WindowContent = image;
    });

    waiter.WaitOpened();
    VERIFY_IS_TRUE(downloadProgressCount > 0);

    LOG_OUTPUT(L"Unset the source");
    lastProgress = -1;
    downloadProgressCount = 0;
    waiter.ResetState();
    RunOnUIThread([&]()
    {
        // We can only do this via UriSource because SetSource[Async] do not support null arguments
        bitmapImage->UriSource = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();
    VERIFY_ARE_EQUAL(0, downloadProgressCount, L"Setting the source to null should not produce download events");

    LoadHelper(bitmapImage, loadApi);
    waiter.WaitOpened();
    VERIFY_IS_TRUE(downloadProgressCount > 0, L"DownloadProgress should fire again on reload");

    TestServices::WindowHelper->WaitForIdle();
}

void ImageTests::LoadHelper(xaml_imaging::BitmapImage ^bitmapImage, TestImageEnums::LoadApi loadApi)
{
    wsts::IRandomAccessStream^ stream;
    if (loadApi == TestImageEnums::LoadApi::SetSourceAsync ||
        loadApi == TestImageEnums::LoadApi::SetSource)
    {
        stream = LoadBinaryFile(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
    }

    RunOnUIThread([&]()
    {
        switch (loadApi)
        {
        case TestImageEnums::LoadApi::SetSourceAsync:
            bitmapImage->SetSourceAsync(stream);
            break;
        case TestImageEnums::LoadApi::SetSource:
            bitmapImage->SetSource(stream);
            break;
        case TestImageEnums::LoadApi::Uri:
            bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
            break;
        }
    });
}

void ImageTests::ToggleUriSource()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    ImageEventWaitingContext eventWaitingContext;

    BitmapImage ^bitmapImage;

    RunOnUIThread([&]
    {
        bitmapImage = ref new BitmapImage();
        eventWaitingContext.Attach(bitmapImage);
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg"); // A

        auto image = ref new xaml_controls::Image();
        TestServices::WindowHelper->WindowContent = image;
        image->Source = bitmapImage;
    });

    eventWaitingContext.WaitOpened();
    eventWaitingContext.ResetState();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "A");

    RunOnUIThread([&]
    {
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"barcelona.jpg"); // B
    });

    eventWaitingContext.WaitOpened();
    eventWaitingContext.ResetState();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "B");

    RunOnUIThread([&]
    {
        bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg"); // A again
    });

    eventWaitingContext.WaitOpened();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, "Back-To-A");
}

void ImageTests::RasterizationScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    xaml_controls::StackPanel^ stackPanel;
    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();
    auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent2 = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating visual tree in two parts so image decode doesn't race over surface IDs.");

        auto uri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");

        BitmapImage^ bitmapImage = ref new BitmapImage();
        bitmapImage->CreateOptions = BitmapCreateOptions::IgnoreImageCache;
        bitmapImage->UriSource = uri;
        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));

        xaml_controls::Image^ image = ref new xaml_controls::Image();
        image->Width = 100;
        image->Height = 100;
        image->RasterizationScale = 2;
        image->Source = bitmapImage;

        stackPanel = ref new xaml_controls::StackPanel();
        stackPanel->Children->Append(image);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });
    bitmapImageOpenedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Adding second image.");

        auto uri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");

        BitmapImage^ bitmapImage2 = ref new BitmapImage();
        bitmapImage2->CreateOptions = BitmapCreateOptions::IgnoreImageCache;
        bitmapImage2->UriSource = uri;
        openedRegistration2.Attach(
            bitmapImage2,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent2](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent2->Set();
        }));

        ImageBrush^ imageBrush = ref new ImageBrush();
        imageBrush->ImageSource = bitmapImage2;

        xaml_controls::Grid^ grid = ref new xaml_controls::Grid();
        grid->Width = 100;
        grid->Height = 100;
        grid->RasterizationScale = 2;
        grid->Background = imageBrush;

        stackPanel->Children->Append(grid);
    });
    bitmapImageOpenedEvent2->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void ImageTests::DecodeToRenderSizeUnderCollapsedSubtree()
{
    const bool applyGalleryWorkaround = false;

    TestCleanupWrapper cleanup;
    auto wh = TestServices::WindowHelper;

    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();

    auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent2 = std::make_shared<Event>();

    Grid^ rootGrid;
    Border^ problematicBorder;
    Border^ workaroundBorder;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Creating collapsed subtree.");

        Uri^ uri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");

        BitmapImage^ bitmapImage = ref new BitmapImage();
        bitmapImage->UriSource = uri;
        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"  > BitmapImage.Opened fired.");
            bitmapImageOpenedEvent->Set();
        }));

        ImageBrush^ imageBrush = ref new ImageBrush();
        imageBrush->ImageSource = bitmapImage;

        problematicBorder = ref new Border();
        problematicBorder->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(8);
        problematicBorder->Background = imageBrush;

        rootGrid = ref new Grid();
        rootGrid->Visibility = xaml::Visibility::Collapsed;
        rootGrid->Children->Append(problematicBorder);

        // workaround for Gallery
        {
            Uri^ uri2 = ref new Uri(GetResourcesPath() + L"barcelona.jpg");

            BitmapImage^ bitmapImage2 = ref new BitmapImage();
            bitmapImage2->UriSource = uri2;
            openedRegistration2.Attach(
                bitmapImage,
                ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent2](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"  > BitmapImage2.Opened fired.");
                bitmapImageOpenedEvent2->Set();
            }));

            ImageBrush^ imageBrush2 = ref new ImageBrush();
            imageBrush2->ImageSource = bitmapImage2;

            workaroundBorder = ref new Border();
            workaroundBorder->Background = imageBrush2;

            Border^ roundedCornerBorder = ref new Border();
            roundedCornerBorder->CornerRadius = xaml::CornerRadiusHelper::FromUniformRadius(8);
            roundedCornerBorder->Child = workaroundBorder;
            rootGrid->Children->Append(roundedCornerBorder);
        }

        wh->WindowContent = rootGrid;

        // Because the subtree is collapsed, the Border/ImageBrush won't render, which then won't go through
        // CImageSource::ReloadReleasedSoftwareImage -> ImageSurfaceWrapper::SetKeepSystemMemory. We'll decode only a
        // hardware surface.
    });

    LOG_OUTPUT(L"> Waiting for BitmapImage.Opened.");
    bitmapImageOpenedEvent->WaitForDefault();
    bitmapImageOpenedEvent2->WaitForDefault();

    // No visuals should be produced while elements are in a collapsed subtree.
    auto visuals = ref new Platform::Collections::Vector<Object^>();
    wh->GetElementRenderedVisuals(problematicBorder, visuals);
    VERIFY_IS_TRUE(visuals->Size == 0);

    auto visuals2 = ref new Platform::Collections::Vector<Object^>();
    wh->GetElementRenderedVisuals(workaroundBorder, visuals2);
    VERIFY_IS_TRUE(visuals2->Size == 0);

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Making subtree visible.");
        rootGrid->Visibility = xaml::Visibility::Visible;

        //
        // The BitmapImage now needs a software surface due to BaseContentRenderer::MaskPartEnsureRealizationTexture's
        // partHasContent check. We won't find one since we never set a software surface. We also won't try to redecode
        // the encoded image we already have because CImageSource::RedecodeEncodedImage doesn't actually do a decode if
        // it's decoding to render size. We're stuck with an ImageSource with no software surface that can't render.
        //
        // This is a bug that Gallery is hitting. They have a BitmapImage in a collapsed
        // subtree that decoded without setting a software surface, and when the subtree is made visible again that
        // ImageBrush/BitmapImage don't render again. For now Gallery can work around it.
        //
        // A few options for fixing:
        //  - Why is a software surface needed in the first place? We don't use it in the Visual tree. Is it to compute
        //    the stretch matrix in case the ImageBrush doesn't fill the surface? We can use the hardware surface size
        //    for that.
        //  - Why not trigger a software decode from RedecodeEncodedImage if we're decoding to render size? Does that
        //    break something? (rather, is there test coverage for the things that it breaks?)
        //
    });
    wh->WaitForIdle();

    // This should have a visual, but doesn't due to the bug.
    wh->GetElementRenderedVisuals(problematicBorder, visuals);
    VERIFY_IS_TRUE(visuals->Size == 0);

    // This has a visual despite the bug
    wh->GetElementRenderedVisuals(workaroundBorder, visuals2);
    VERIFY_IS_TRUE(visuals2->Size == 1);
}

void ImageTests::DontReloadImageFromStream()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    const auto& wh = TestServices::WindowHelper;

    BitmapImage ^bitmapImage;
    auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
    auto bitmapImageOpenedEvent = std::make_shared<Event>();
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Creating ImageSource from stream.");
        bitmapImage = ref new BitmapImage();

        LOG_OUTPUT(L"  > Set BitmapCreateOptions before loading from stream.");
        bitmapImage->CreateOptions = BitmapCreateOptions::IgnoreImageCache;

        openedRegistration.Attach(
            bitmapImage,
            ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
        {
            LOG_OUTPUT(L"  > BitmapImage Opened Event Fired");
            bitmapImageOpenedEvent->Set();
        }));
    });
    // Note: Must use async. The synchronous version goes through BitmapImage::SetSource, which explicitly sets the
    // URI to null.
    LoadHelper(bitmapImage, TestImageEnums::LoadApi::SetSourceAsync);

    xaml_controls::Image^ image;
    Canvas^ canvas;
    RunOnUIThread([&]()
    {
        image = ref new xaml_controls::Image();
        image->Width = 100;
        image->Height = 100;
        image->Source = bitmapImage;

        canvas = ref new Canvas();
        canvas->Width = 10;
        canvas->Height = 10;
        canvas->Background = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        xaml_controls::StackPanel^ stackPanel = ref new xaml_controls::StackPanel();
        stackPanel->Children->Append(image);
        stackPanel->Children->Append(canvas);
        TestServices::WindowHelper->WindowContent = stackPanel;
    });
    bitmapImageOpenedEvent->WaitForDefault();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Update the canvas. This should be the only element rendered.");
        canvas->Width = 11;

        //
        // The <Image> needs clean bounds too. There are other problems that cover up the redraw problem...
        //
        // The Rectangle inside the Image has m_fFillBrushDirty and m_fNWContentDirty always marked and never cleaned,
        // which prevent render dirty flag propagation. Dirty flags are actually going up through the bounds flags,
        // which require the bounds to be clean. Do a programmatic hit test now to make sure we calculate bounds
        // on the Image and clean the bounds dirty flags.
        //
        wf::Point point(50, 50);
        VisualTreeHelper::FindElementsInHostCoordinates(point, image);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        int elementsRendered = wh->GetElementsRenderedCount();
        VERIFY_ARE_EQUAL(1, elementsRendered);
    });

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  > Update the canvas. This should still be the only element rendered.");
        canvas->Width = 12;

        wf::Point point(50, 50);
        xaml_media::VisualTreeHelper::FindElementsInHostCoordinates(point, image);
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        int elementsRendered = wh->GetElementsRenderedCount();
        VERIFY_ARE_EQUAL(1, elementsRendered);
    });
}
