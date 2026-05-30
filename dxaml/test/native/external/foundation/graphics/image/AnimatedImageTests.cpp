// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ImageEventWaitingContext.h"
#include <collection.h>
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <robuffer.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "AnimatedImageTests.h"
#include "ImageTestEngine.h"
#include "MUX-ETWEvents.h"
#include "ETWWaiterProxy.h"
#include <WUCRenderingScopeGuard.h>

using namespace MockDComp;
using namespace test_infra;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace Microsoft::UI::Xaml::Tests::Foundation::Graphics::Image;


static void VerifyStaticOrDefault(xaml_imaging::BitmapImage ^bitmapImage)
{
    VERIFY_IS_FALSE(bitmapImage->IsPlaying);
    VERIFY_IS_FALSE(bitmapImage->IsAnimatedBitmap);

    LOG_OUTPUT(L"Play() should not change playing state for empty or static image");
    bitmapImage->Play();
    VERIFY_IS_FALSE(bitmapImage->IsPlaying);
    VERIFY_IS_FALSE(bitmapImage->IsAnimatedBitmap);

    LOG_OUTPUT(L"Stop() should not change playing state for empty or static image");
    bitmapImage->Stop();
    VERIFY_IS_FALSE(bitmapImage->IsPlaying);
    VERIFY_IS_FALSE(bitmapImage->IsAnimatedBitmap);
}

static void VerifyPlayForStaticImage(Platform::String ^fileName)
{
    TestCleanupWrapper cleanup;
    xaml_imaging::BitmapImage ^bitmapImage;
    ImageEventWaitingContext waiter;
    RunOnUIThread([&]()
    {
        bitmapImage = ref new xaml_imaging::BitmapImage();
        waiter.Attach(bitmapImage);
        bitmapImage->UriSource = ref new wf::Uri(fileName);
        auto testImage = ref new xaml_controls::Image();
        testImage->Source = bitmapImage;
        TestServices::WindowHelper->WindowContent = testImage;
    });

    waiter.WaitOpened();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VerifyStaticOrDefault(bitmapImage);
    });
}

static void VerifyUIThreadTicking()
{
    TestServices::WindowHelper->WaitForIdle();

    ETWWaiterProxy tickWaiter;
    tickWaiter.Start(WINDOWS_UI_XAML_ETW_PROVIDER, TickInfo_value);

    // Ticking indicates that we most likely render something
    for (int i = 0; i < 3; i++)
    {
        tickWaiter.WaitForDefault();
    }
}

Platform::String^ AnimatedImageTests::GetImagePath(Platform::String ^imageFileName) const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\" + imageFileName;
}

bool AnimatedImageTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool AnimatedImageTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool AnimatedImageTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void AnimatedImageTests::BitmapImageInitialState()
{
    TestCleanupWrapper cleanup;
    RunOnUIThread([&]()
    {
        auto bitmapImage = ref new xaml_imaging::BitmapImage();
        VERIFY_IS_TRUE(bitmapImage->AutoPlay, L"AutoPlay should be true by default");
        VerifyStaticOrDefault(bitmapImage);
    });
}

void AnimatedImageTests::StaticBitmapImageStateBmp()
{
    VerifyPlayForStaticImage(GetImagePath(L"Smiley.bmp"));
}

void AnimatedImageTests::StaticBitmapImageStateGif()
{
    VerifyPlayForStaticImage(GetImagePath(L"animatedgif\\08bMStatic3.gif"));
}

void AnimatedImageTests::NoAutoPlay()
{
    TestCleanupWrapper cleanup;

    xaml_imaging::BitmapImage^ bitmapImage;
    ImageEventWaitingContext waiter;

    RunOnUIThread([&]()
    {
        auto testImage = ref new xaml_controls::Image();
        testImage->Stretch = xaml_media::Stretch::Fill;

        TestServices::WindowHelper->WindowContent = testImage;

        bitmapImage = ref new xaml_imaging::BitmapImage();
        waiter.Attach(bitmapImage);
        bitmapImage->AutoPlay = false;
        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

        testImage->Source = bitmapImage;
    });

    waiter.WaitOpened();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(bitmapImage->IsAnimatedBitmap);
        VERIFY_IS_FALSE(bitmapImage->IsPlaying, L"Should not start playing when AutoPlay=false");

        VERIFY_IS_FALSE(bitmapImage->IsPlaying, L"Manually restart animation");
        bitmapImage->Play();

        VERIFY_IS_TRUE(bitmapImage->IsPlaying, L"Still able to restart manually even if AutoPlay=false");
    });
}

void AnimatedImageTests::SimpleImageElement()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    xaml_imaging::BitmapImage^ bitmapImage;
    ImageEventWaitingContext waiter;

    RunOnUIThread([&]()
    {
        auto testImage = ref new xaml_controls::Image();
        testImage->Stretch = xaml_media::Stretch::Fill;

        TestServices::WindowHelper->WindowContent = testImage;

        bitmapImage = ref new xaml_imaging::BitmapImage();
        waiter.Attach(bitmapImage);

        testImage->Source = bitmapImage;
        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

        VERIFY_IS_FALSE(bitmapImage->IsAnimatedBitmap, L"IsAnimatedBitmap is assumed false until image is loaded");
        VERIFY_IS_FALSE(bitmapImage->IsPlaying, L"IsPlaying is assumed false until image is loaded");
    });

    waiter.WaitOpened();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(bitmapImage->IsAnimatedBitmap);
        VERIFY_IS_TRUE(bitmapImage->IsPlaying, L"Should automatically start playing when AutoPlay=true");

        LOG_OUTPUT(L"Manually stop animation");
        bitmapImage->Stop();

        VERIFY_IS_TRUE(bitmapImage->IsAnimatedBitmap);
        VERIFY_IS_FALSE(bitmapImage->IsPlaying);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void AnimatedImageTests::AnimatedSetUri()
{
    TestCleanupWrapper cleanup;

    xaml_imaging::BitmapImage ^bitmapImage;
    ImageEventWaitingContext imageWaiter;

    RunOnUIThread([&]()
    {
        auto testImage = ref new xaml_controls::Image();
        testImage->Stretch = xaml_media::Stretch::Fill;

        TestServices::WindowHelper->WindowContent = testImage;

        bitmapImage = ref new xaml_imaging::BitmapImage();
        testImage->Source = bitmapImage;

        imageWaiter.Attach(bitmapImage);
        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\08bSAnim3.gif"));
    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::AnimatedSetSource()
{
    TestCleanupWrapper cleanup;

    ImageEventWaitingContext imageWaiter;

    wsts::IRandomAccessStream^ fileStream = LoadBinaryFile(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

    RunOnUIThread([&]()
    {
        auto testImage = ref new xaml_controls::Image();
        testImage->Stretch = xaml_media::Stretch::Fill;

        TestServices::WindowHelper->WindowContent = testImage;

        auto bitmapImage = ref new xaml_imaging::BitmapImage();
        imageWaiter.Attach(bitmapImage);

        bitmapImage->SetSource(fileStream);
        testImage->Source = bitmapImage;

    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::AnimatedSetSourceNotInTree()
{
    TestCleanupWrapper cleanup;

    ImageEventWaitingContext imageWaiter;

    wsts::IRandomAccessStream^ fileStream = LoadBinaryFile(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

    RunOnUIThread([&]()
    {
        auto testImage = ref new xaml_controls::Image();
        testImage->Stretch = xaml_media::Stretch::Fill;

        TestServices::WindowHelper->WindowContent = testImage;

        auto bitmapImage = ref new xaml_imaging::BitmapImage();
        imageWaiter.Attach(bitmapImage);

        testImage->Source = bitmapImage;
        bitmapImage->SetSource(fileStream);
    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::AnimatedSetSourceAsyncPreLiveTree()
{
    TestCleanupWrapper cleanup;

    ImageEventWaitingContext imageWaiter;

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetImagePath(L"SimpleImage.xaml")));

    wsts::IRandomAccessStream^ fileStream = LoadBinaryFile(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

    xaml_controls::Image^ testImage = nullptr;
    xaml_imaging::BitmapImage^ bitmapImage = nullptr;

    // Make sure to load the grid first otherwise ticks won't be processed since the main tree won't
    // be loaded.
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    // Load the bitmap image before attaching it to the live tree to test the case
    // where DecodeToRenderSize and BackgroundThreadImageLoading are off.
    RunOnUIThread([&]()
    {
        bitmapImage = ref new xaml_imaging::BitmapImage();
        imageWaiter.Attach(bitmapImage);
        bitmapImage->SetSourceAsync(fileStream);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        testImage->Stretch = xaml_media::Stretch::Fill;
        testImage->Source = bitmapImage;
    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::AnimatedSetSourceAsyncPostLiveTree()
{
    TestCleanupWrapper cleanup;

    ImageEventWaitingContext imageWaiter;

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetImagePath(L"SimpleImage.xaml")));

    wsts::IRandomAccessStream ^fileStream = LoadBinaryFile(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

    xaml_controls::Image^ testImage = nullptr;
    xaml_imaging::BitmapImage^ bitmapImage = nullptr;

    // Load the bitmap image after attaching it to the live tree to test the case
    // where DecodeToRenderSize and BackgroundThreadImageLoading are on.
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        testImage->Stretch = xaml_media::Stretch::Fill;

        bitmapImage = ref new xaml_imaging::BitmapImage();
        testImage->Source = bitmapImage;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        imageWaiter.Attach(bitmapImage);
        bitmapImage->SetSourceAsync(fileStream);
    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::AnimatedSWBrush()
{
    TestCleanupWrapper cleanup;

    ImageEventWaitingContext imageWaiter;

    RunOnUIThread([&]()
    {
        auto bitmapImage = ref new xaml_imaging::BitmapImage();
        imageWaiter.Attach(bitmapImage);
        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\08bSAnim3.gif"));

        auto imageBrush = ref new xaml_media::ImageBrush();
        imageBrush->ImageSource = bitmapImage;

        auto border = ref new xaml_controls::Border();
        border->CornerRadius = CornerRadiusHelper::FromUniformRadius(30);
        border->Background = imageBrush;

        TestServices::WindowHelper->WindowContent = border;
    });

    imageWaiter.WaitOpened();

    VerifyUIThreadTicking();
}

void AnimatedImageTests::PlateauScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::CRC);

    auto windowSize = wf::Size(400, 300);
    TestServices::WindowHelper->SetWindowSizeOverride(windowSize);

    ImageEventWaitingContext imageWaiter;
    ETWWaiterProxy animationEndWaiter;
    animationEndWaiter.Start(WINDOWS_UI_XAML_ETW_PROVIDER, ImageAnimationEndInfo_value);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetImagePath(L"SimpleImage.xaml")));
    xaml_imaging::BitmapImage ^bitmapImage;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;
        auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
        testImage->Stretch = xaml_media::Stretch::Uniform;
        testImage->Width = 100;
        testImage->Height = 100;

        bitmapImage = ref new xaml_imaging::BitmapImage();
        // Set logical pixel decode size so the reload manager re-decodes at the
        // new resolution when the plateau scale changes (same pattern as ImageTests::PlateauScaleChange).
        bitmapImage->DecodePixelWidth = 100;
        bitmapImage->DecodePixelHeight = 100;
        bitmapImage->DecodePixelType = xaml_imaging::DecodePixelType::Logical;
        imageWaiter.Attach(bitmapImage);

        testImage->Source = bitmapImage;

        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\2_frames_once.gif"));
    });

    imageWaiter.WaitOpened();
    animationEndWaiter.WaitForDefault();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"scale_100");

    LOG_OUTPUT(L"Change plateau scale");
    TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(windowSize, 3.0f);

    TestServices::WindowHelper->SynchronouslyTickUIThread(3); // Ensure Xaml has time to re-render after scale change.
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(bitmapImage->IsPlaying);
    });
    animationEndWaiter.WaitForDefault();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"scale_300");
}

void AnimatedImageTests::DeviceLost()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(100, 100));

    ImageEventWaitingContext imageWaiter;
    ETWWaiterProxy animationEndWaiter;
    animationEndWaiter.Start(WINDOWS_UI_XAML_ETW_PROVIDER, ImageAnimationEndInfo_value);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetImagePath(L"SimpleImage.xaml")));
    xaml_imaging::BitmapImage^ bitmapImage;
    xaml_controls::Image^ testImage;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;
        testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));

        bitmapImage = ref new xaml_imaging::BitmapImage();
        imageWaiter.Attach(bitmapImage);

        testImage->Source = bitmapImage;
    });
    TestServices::WindowHelper->WaitForIdle();

    // Important to attach the image to the tree first before setting the source.
    // Otherwise the output could be variable whether it uses DecodeToRenderSize or not.
    RunOnUIThread([&]()
    {
        bitmapImage->UriSource = ref new wf::Uri(GetImagePath(L"animatedgif\\2_frames_once.gif"));
    });

    imageWaiter.WaitOpened();
    animationEndWaiter.WaitForDefault();

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"before");

    LOG_OUTPUT(L"Simulating device lost");
    animationEndWaiter.Start(WINDOWS_UI_XAML_ETW_PROVIDER, ImageAnimationEndInfo_value);
    TestServices::WindowHelper->SimulateDeviceLost();

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(bitmapImage->IsPlaying);
    });
    animationEndWaiter.WaitForDefault();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"after");
}
