// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomBrushes.h"
#include "RenderTargetBitmapTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <WUCRenderingScopeGuard.h>
#include <XamlFileTestEngine.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Composition;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Xaml::Media::Animation;

using namespace test_infra;
using namespace Concurrency;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

Platform::String^ RenderTargetBitmapTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\rendering\\";
}

bool RenderTargetBitmapTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool RenderTargetBitmapTests::TestSetup()
{
    TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool RenderTargetBitmapTests::TestCleanup()
{
    TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

//------------------------------------------------------------------------
//
// Invokes RenderTargetBitmap.RenderAsync for a simple visual tree and
// consumes the result in an Image.
//
//------------------------------------------------------------------------
void RenderTargetBitmapTests::BasicRenderTargetBitmapInternal()
{
    StackPanelTestHelper(L"RTBTests-Basic.xaml", false, false);
}

//------------------------------------------------------------------------
//
// Invokes RenderTargetBitmap.RenderAsync for a Popup child visual tree and
// consumes the result in an Image.
//
//------------------------------------------------------------------------
void RenderTargetBitmapTests::PopupChildRTBInternal()
{
    StackPanelTestHelper(L"RTBTests-PopupChild.xaml", false, false);
}

//------------------------------------------------------------------------
//
// Invokes RenderTargetBitmap.RenderAsync for a Popup parent visual tree and
// consumes the result in an Image.
//
//------------------------------------------------------------------------
void RenderTargetBitmapTests::PopupRTBInternal()
{
    StackPanelTestHelper(L"RTBTests-Popup.xaml", false, false);
}


//------------------------------------------------------------------------
//
// Invokes RenderTargetBitmap.RenderAsync for a ScrollViewer and consumes
// the result in an Image.
//
//------------------------------------------------------------------------
void RenderTargetBitmapTests::RenderScrollViewerBitmapInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();
    auto viewChangedEvent = std::make_shared<Event>();
    auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;
    ScrollViewer^ scrollViewer = nullptr;
    StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RTBTests-ScrollViewer.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootStackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scrollViewer = safe_cast<ScrollViewer^>(rootStackPanel->FindName(L"sv"));
        VERIFY_IS_NOT_NULL(scrollViewer);

        viewChangedRegistration.Attach(
            scrollViewer,
            ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent, scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
        {
            LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            if (args->IsIntermediate == false)
            {
                LOG_OUTPUT(L"Final ViewChanged event raised.");
                viewChangedEvent->Set();
            }
        }));

        scrollViewer->ChangeView(60.0, 60.0, 1.1f, true /*disableAnimation*/);
    });

    LOG_OUTPUT(L"Waiting for the end of view change.");
    viewChangedEvent->WaitForDefault();

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 60.0);
        VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 60.0);
        VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.1f);

        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));
        VERIFY_IS_NOT_NULL(img);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(scrollViewer)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));
        VERIFY_IS_NOT_NULL(img);

        img->Source = rtb;
        img->Width = scrollViewer->ActualWidth;
        img->Height = scrollViewer->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"GetPixelsAsync operation completed.");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(getPixelsCallback);
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying bitmap content.");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, 78400u); // 140 x 140 x 4 == 78400

        LOG_OUTPUT(L"Pixel (64,42) expected to be white = %d, %d, %d, %d", generatedImage[(42 * 140 + 64) * 4], generatedImage[(42 * 140 + 64) * 4 + 1], generatedImage[(42 * 140 + 64) * 4 + 2], generatedImage[(42 * 140 + 64) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 64) * 4], 255);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 64) * 4 + 1], 255);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 64) * 4 + 2], 255);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 64) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixel (138,42) expected to be red = %d, %d, %d, %d", generatedImage[(42 * 140 + 138) * 4], generatedImage[(42 * 140 + 138) * 4 + 1], generatedImage[(42 * 140 + 138) * 4 + 2], generatedImage[(42 * 140 + 138) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 138) * 4], 0);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 138) * 4 + 1], 0);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 138) * 4 + 2], 255);
        VERIFY_ARE_EQUAL(generatedImage[(42 * 140 + 138) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixels (62,92) expected to be blue = %d, %d, %d, %d", generatedImage[(92 * 140 + 62) * 4], generatedImage[(92 * 140 + 62) * 4 + 1], generatedImage[(92 * 140 + 62) * 4 + 2], generatedImage[(92 * 140 + 62) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(92 * 140 + 62) * 4], 255);
        VERIFY_ARE_EQUAL(generatedImage[(92 * 140 + 62) * 4 + 1], 0);
        VERIFY_ARE_EQUAL(generatedImage[(92 * 140 + 62) * 4 + 2], 0);
        VERIFY_ARE_EQUAL(generatedImage[(92 * 140 + 62) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixels (114,125) expected to be dark gray = %d, %d, %d, %d", generatedImage[(125 * 140 + 114) * 4], generatedImage[(125 * 140 + 114) * 4 + 1], generatedImage[(125 * 140 + 114) * 4 + 2], generatedImage[(125 * 140 + 114) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 114) * 4], 68);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 114) * 4 + 1], 68);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 114) * 4 + 2], 68);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 114) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixels (117,125) expected to be gray = %d, %d, %d, %d", generatedImage[(125 * 140 + 117) * 4], generatedImage[(125 * 140 + 117) * 4 + 1], generatedImage[(125 * 140 + 117) * 4 + 2], generatedImage[(125 * 140 + 117) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 117) * 4], 85);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 117) * 4 + 1], 85);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 117) * 4 + 2], 85);
        VERIFY_ARE_EQUAL(generatedImage[(125 * 140 + 117) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixels (114,128) expected to be gray = %d, %d, %d, %d", generatedImage[(128 * 140 + 114) * 4], generatedImage[(128 * 140 + 114) * 4 + 1], generatedImage[(128 * 140 + 114) * 4 + 2], generatedImage[(128 * 140 + 114) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 114) * 4], 119);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 114) * 4 + 1], 119);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 114) * 4 + 2], 119);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 114) * 4 + 3], 255);

        LOG_OUTPUT(L"Pixels (117,128) expected to be light gray = %d, %d, %d, %d", generatedImage[(128 * 140 + 117) * 4], generatedImage[(128 * 140 + 117) * 4 + 1], generatedImage[(128 * 140 + 117) * 4 + 2], generatedImage[(128 * 140 + 117) * 4 + 3]);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 117) * 4], 136);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 117) * 4 + 1], 136);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 117) * 4 + 2], 136);
        VERIFY_ARE_EQUAL(generatedImage[(128 * 140 + 117) * 4 + 3], 255);
    });
}


//------------------------------------------------------------------------
//
// Invokes RenderTargetBitmap.RenderAsync for a ScrollViewer and then pans it.
//
//------------------------------------------------------------------------
void RenderTargetBitmapTests::PanAfterRenderScrollViewerBitmapInternal()
{
    // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    auto renderedEvent = std::make_shared<Event>();
    auto viewChangedEvent = std::make_shared<Event>();
    auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

    RenderTargetBitmap^ rtb = nullptr;
    ScrollViewer^ scrollViewer = nullptr;
    StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RTBTests-ScrollViewer.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootStackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        scrollViewer = safe_cast<ScrollViewer^>(rootStackPanel->FindName(L"sv"));
        VERIFY_IS_NOT_NULL(scrollViewer);

        viewChangedRegistration.Attach(
            scrollViewer,
            ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent, scrollViewer](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
        {
            LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
            if (args->IsIntermediate == false)
            {
                LOG_OUTPUT(L"Final ViewChanged event raised.");
                viewChangedEvent->Set();
            }
        }));

        scrollViewer->ChangeView(60.0, 60.0, 1.1f, true /*disableAnimation*/);
    });

    LOG_OUTPUT(L"Waiting for the end of view change.");
    viewChangedEvent->WaitForDefault();
    LOG_OUTPUT(L"View change done.");

    TestServices::WindowHelper->WaitForIdle();
    viewChangedEvent->Reset();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 60.0);
        VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 60.0);
        VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.1f);

        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));
        VERIFY_IS_NOT_NULL(img);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(scrollViewer)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));

        img->Source = rtb;
        img->Width = scrollViewer->ActualWidth;
        img->Height = scrollViewer->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();

    LOG_OUTPUT(L"Launching horizontal pan operation.");
    TestServices::InputHelper->PanFromCenter(scrollViewer, 100 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);

    LOG_OUTPUT(L"Waiting for the end of manipulation.");
    viewChangedEvent->WaitForDefault();

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
        VERIFY_ARE_EQUAL(scrollViewer->VerticalOffset, 60.0);
        VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.1f);
    });

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void RenderTargetBitmapTests::RenderTransformRTBInternal()
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(600, 600));

    auto renderedEvent = std::make_shared<Event>();

    Canvas^ root = nullptr;

    RenderTargetBitmap^ rtb = nullptr;
    LOG_OUTPUT(L"Get root stack panel");
    StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"RTBTests-RenderTransform.xaml"));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootStackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Get root canvas for: TranslateRotate");
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateRotate"));
        VERIFY_IS_NOT_NULL(root);

        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateRotate"));
        VERIFY_IS_NOT_NULL(img);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(root)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateRotate"));
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateRotate"));

        img->Source = rtb;
        img->Width = root->ActualWidth;
        img->Height = root->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Get root canvas for: TranslateSkew");
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateSkew"));
        VERIFY_IS_NOT_NULL(root);

        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateSkew"));
        VERIFY_IS_NOT_NULL(img);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(root)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateSkew"));
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateSkew"));

        img->Source = rtb;
        img->Width = root->ActualWidth;
        img->Height = root->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Get root canvas for: TranslateComposite");
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateComposite"));
        VERIFY_IS_NOT_NULL(root);

        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateComposite"));
        VERIFY_IS_NOT_NULL(img);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(root)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        root = safe_cast<Canvas^>(rootStackPanel->FindName(L"RootTranslateComposite"));
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"imgTranslateComposite"));

        img->Source = rtb;
        img->Width = root->ActualWidth;
        img->Height = root->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"RenderTransformRTBFinal");
}

// Invokes RenderTargetBitmap.RenderAsync for an RTL element
void RenderTargetBitmapTests::RTLElementInternal()
{
    StackPanelTestHelper(L"RTBTests-RTLinLTR.xaml", true, false);
}

// Invokes RenderTargetBitmap.RenderAsync for an RTL element located within an RTL parent
void RenderTargetBitmapTests::RTLwithinRTLInternal()
{
    StackPanelTestHelper(L"RTBTests-RTLinRTL.xaml", true, false);
}

// Invokes RenderTargetBitmap.RenderAsync for an LTR element located within an RTL parent
void RenderTargetBitmapTests::LTRwithinRTLInternal()
{
    StackPanelTestHelper(L"RTBTests-LTRinRTL.xaml", false, false);
}

void RenderTargetBitmapTests::XamlCompositionBrushRTBInternal(bool expectCaptureAsync)
{
    StackPanelTestHelper(L"RTBTests-XamlCompositionBrush.xaml", false, true, expectCaptureAsync);
}

void RenderTargetBitmapTests::StackPanelTestHelper(
    Platform::String^ fileName,     // Name of Xaml file with content to capture as RTB
    bool expectRTL,                 // Whether RightToLeft property is set
    bool useXCB,                    // Whether solid color brushes on the Red/White/Blue rectangles are replaced with XamlCompositionBrushes
    bool expectCaptureAsync,        // Whether it uses the Composition API CaptureAsync to render to RTB which captures composition objects as well.
    int renderCallCount,            // How many times to issue the RenderAsync operation
    bool verifyIsTransparent        // If true, we expect the RTB surface to be completely transparent
)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;
    StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + fileName));
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootStackPanel;

        // A XamlCompositionBrushBase- derived brush currently cannot be rendered in RTB mode, instead a solid FallbackColor should be used.
        // For this test, define XCB-based brushes wiht fallback colors that match the original red/white/blue rendering.
        if (useXCB)
        {
            XcbPurpleBrush^ xcbBlueFallback = ref new XcbPurpleBrush();
            xcbBlueFallback->FallbackColor = Microsoft::UI::Colors::Blue;

            XcbPurpleBrush^ xcbWhiteFallback = ref new XcbPurpleBrush();
            xcbWhiteFallback->FallbackColor = Microsoft::UI::Colors::White;

            XcbPurpleBrush^ xcbRedFallback = ref new XcbPurpleBrush();
            xcbRedFallback->FallbackColor = Microsoft::UI::Colors::Red;

            Microsoft::UI::Xaml::Shapes::Rectangle^ blueRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootStackPanel->FindName(L"BlueRectangle"));
            Microsoft::UI::Xaml::Shapes::Rectangle^ whiteRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootStackPanel->FindName(L"WhiteRectangle"));
            Microsoft::UI::Xaml::Shapes::Rectangle^ redRectangle = safe_cast<Microsoft::UI::Xaml::Shapes::Rectangle^>(rootStackPanel->FindName(L"RedRectangle"));

            blueRectangle->Fill = xcbBlueFallback;
            whiteRectangle->Fill = xcbWhiteFallback;
            redRectangle->Fill = xcbRedFallback;
        }
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        auto colors = safe_cast<FrameworkElement^>(rootStackPanel->FindName(L"colors"));
        VERIFY_IS_NOT_NULL(colors);

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        // Issue multiple renders on the same element for stress purposes if requested.
        // Only the last call will be used for comparison.  This is generally used to ensure there isn't
        // a crash or an assertion hit internally when multiple renders occur on the same element.
        while (renderCallCount > 1)
        {
            rtb->RenderAsync(colors);
            renderCallCount--;
        }

        create_task(rtb->RenderAsync(colors)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        auto colors = safe_cast<FrameworkElement^>(rootStackPanel->FindName(L"colors"));
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));
        VERIFY_IS_NOT_NULL(img);

        img->Source = rtb;
        img->Width = colors->ActualWidth;
        img->Height = colors->ActualHeight;
        LOG_OUTPUT(L"colors->ActualWidth = %d, colors->ActualHeight = %d", colors->ActualWidth, colors->ActualHeight);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"GetPixelsAsync operation completed.");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(getPixelsCallback);
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying bitmap content.");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, 48000u); // 100 x 120 x 4 == 48000

        if (verifyIsTransparent)
        {
            VERIFY_ARE_EQUAL(generatedImage[39 * 4], 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 2], 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 3], 0);

            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4], 0);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 2], 0);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 3], 0);

            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4], 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 2], 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 3], 0);
        }
        else if (useXCB && expectCaptureAsync)
        {
            LOG_OUTPUT(L"Pixel (0,39) expected to be purple = %d, %d, %d, %d",
                       generatedImage[39 * 4], generatedImage[39 * 4 + 1], generatedImage[39 * 4 + 2], generatedImage[39 * 4 + 3]);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4], 128);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 2], 128);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 3], 255);

            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4], 128);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 2], 128);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 3], 255);

            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4], 128);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 2], 128);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 3], 255);
        }
        else
        {
            LOG_OUTPUT(L"Pixel (0,39) expected to be %s = %d, %d, %d, %d", expectRTL ? L"red" : L"blue",
                       generatedImage[39 * 4], generatedImage[39 * 4 + 1], generatedImage[39 * 4 + 2], generatedImage[39 * 4 + 3]);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4], expectRTL ? 0 : 255);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 2], expectRTL ? 255 : 0);
            VERIFY_ARE_EQUAL(generatedImage[39 * 4 + 3], 255);

            LOG_OUTPUT(L"Pixel (60,50) expected to be white = %d, %d, %d, %d", generatedImage[(50 * 120 + 60) * 4], generatedImage[(50 * 120 + 60) * 4 + 1], generatedImage[(50 * 120 + 60) * 4 + 2], generatedImage[(50 * 120 + 60) * 4 + 3]);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4], 255);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 1], 255);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 2], 255);
            VERIFY_ARE_EQUAL(generatedImage[(50 * 120 + 60) * 4 + 3], 255);

            LOG_OUTPUT(L"Pixels (119,99) expected to be %s = %d, %d, %d, %d", expectRTL ? L"blue" : L"red",
                       generatedImage[(99 * 120 + 119) * 4], generatedImage[(99 * 120 + 119) * 4 + 1], generatedImage[(99 * 120 + 119) * 4 + 2], generatedImage[(99 * 120 + 119) * 4 + 3]);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4], expectRTL ? 255 : 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 1], 0);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 2], expectRTL ? 0 : 255);
            VERIFY_ARE_EQUAL(generatedImage[(99 * 120 + 119) * 4 + 3], 255);
        }
    });
}

bool RenderTargetBitmapTests::CheckPixelValue(
    Platform::Array<byte>^ imageData,
    int x,
    int y,
    int pixelStride,
    uint32_t expectedPixelValue)
{
    uint32_t actualPixelValue = *(uint32_t*)&imageData[(y * pixelStride + x) * 4];

    LOG_OUTPUT(L"Pixel (%d,%d) expected = %08lx, actual = %08lx", x, y, expectedPixelValue, actualPixelValue);

    return expectedPixelValue == actualPixelValue;
}

void RenderTargetBitmapTests::RenderToSizeInternal()
{

    const int Width = 100;
    const int Height = 48;
    int scales [] = { 100, 125, 150 };

    auto renderedEvent = std::make_shared<Event>();

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;
    Border^ rootBorder = nullptr;
    RunOnUIThread([&]()
    {
        // Initialize the content to be rendered
        rootBorder = ref new Border();
        rootBorder->Width = Width;
        rootBorder->Height = Height;
        rootBorder->Background = ref new SolidColorBrush(Colors::Red);

        TestServices::WindowHelper->WindowContent = rootBorder;
    });

    TestServices::WindowHelper->WaitForIdle();

    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));
    for (int i = 0; i < ARRAYSIZE(scales); ++i)
    {
        int expectedWidth = (Width * scales[i]) / 100;
        int expectedHeight = (Height * scales[i]) / 100;

        RunOnUIThread([&]()
        {
            rtb = ref new RenderTargetBitmap();
            LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync: scale=100 width=%d height=%d", expectedWidth, expectedHeight);

            create_task(rtb->RenderAsync(rootBorder, expectedWidth, expectedHeight)).then([&renderedEvent] ()
            {
                LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
                renderedEvent->Set();
            });
        });

        renderedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedWidth, rtb->PixelWidth);
            VERIFY_ARE_EQUAL(expectedHeight, rtb->PixelHeight);
        });

        TestServices::WindowHelper->WaitForIdle();
    }
}

void RenderTargetBitmapTests::ScaledRenderInternal()
{
    const float initialScale = TestServices::WindowHelper->GetCurrentWindowScale();
    const int initialScalePercentage = static_cast<int>(.5f + initialScale * 100.0f);

    LOG_OUTPUT(L"InitialScale=%f, initialScalePercentage=%d\n", initialScale, initialScalePercentage);

    const int Width = 100;
    const int Height = 48;
    const int scale = 125;

    auto renderedEvent = std::make_shared<Event>();

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;
    Border^ rootBorder = nullptr;
    RunOnUIThread([&]()
    {
        // This is a static method which changes the current core zoom factor and therefore can be
        // tested even if we don't have a background task
        Platform::Object^ factory = nullptr;
        VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Media.Imaging.XamlRenderingBackgroundTask").Get(),
                reinterpret_cast<IInspectable**>(&factory)));
            IXamlRenderingBackgroundTaskStaticsPrivate^ factoryPrivate = dynamic_cast<IXamlRenderingBackgroundTaskStaticsPrivate^>(factory);
            VERIFY_IS_NOT_NULL(factoryPrivate);

            factoryPrivate->SetScalePercentage(scale);

        // Initialize the content to be rendered
        rootBorder = ref new Border();
        rootBorder->Width = Width;
        rootBorder->Height = Height;
        rootBorder->Background = ref new SolidColorBrush(Colors::Red);

        TestServices::WindowHelper->WindowContent = rootBorder;
    });

    TestServices::WindowHelper->WaitForIdle();

    int expectedWidth = (Width * scale) / 100;
    int expectedHeight = (Height * scale) / 100;

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync: scale=%d width=%d height=%d", scale, Width, Height);

        create_task(rtb->RenderAsync(rootBorder)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(expectedWidth, rtb->PixelWidth);
        VERIFY_ARE_EQUAL(expectedHeight, rtb->PixelHeight);
    });

    TestServices::WindowHelper->WaitForIdle();

    expectedWidth = static_cast<int>(.5 + Width * initialScale);
    expectedHeight = static_cast<int>(.5 + Height * initialScale);

    RunOnUIThread([&]()
    {
        // This is a static method which changes the current core zoom factor and therefore can be
        // tested even if we don't have a background task
        Platform::Object^ factory = nullptr;
        VERIFY_SUCCEEDED(::Windows::Foundation::GetActivationFactory(wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Media.Imaging.XamlRenderingBackgroundTask").Get(),
                reinterpret_cast<IInspectable**>(&factory)));
        IXamlRenderingBackgroundTaskStaticsPrivate^ factoryPrivate = dynamic_cast<IXamlRenderingBackgroundTaskStaticsPrivate^>(factory);
        VERIFY_IS_NOT_NULL(factoryPrivate);

        factoryPrivate->SetScalePercentage(initialScalePercentage);

        rtb = ref new RenderTargetBitmap();

        create_task(rtb->RenderAsync(rootBorder, Width, Height)).then([&renderedEvent]()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(expectedWidth, rtb->PixelWidth);
        VERIFY_ARE_EQUAL(expectedHeight, rtb->PixelHeight);
    });
}

void RenderTargetBitmapTests::AncestorOpacity0Internal()
{
    StackPanelTestHelper(L"RTBTests-AncestorOpacity0.xaml");
}

void RenderTargetBitmapTests::AncestorClippedInternal()
{
    StackPanelTestHelper(L"RTBTests-AncestorClipped.xaml");
}

void RenderTargetBitmapTests::AncestorScaledTo0Internal(bool verifyIsTransparent)
{
    StackPanelTestHelper(L"RTBTests-AncestorScaledTo0.xaml", false, false, false, 1, verifyIsTransparent);
}

void RenderTargetBitmapTests::LTEOpacity0Internal()
{
    auto wh = TestServices::WindowHelper;
    auto u = TestServices::Utilities;

    u->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    auto renderedEvent = std::make_shared<Event>();

    StackPanel^ rootStackPanel;
    xaml_shapes::Rectangle^ rectangle;
    Border^ border1;
    Border^ border2;
    RenderTargetBitmap^ rtb;

    RunOnUIThread([&]()
    {
        rootStackPanel = ref new StackPanel();

        rectangle = ref new xaml_shapes::Rectangle();
        rectangle->Width = 100;
        rectangle->Height = 100;
        rectangle->Fill = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));

        border1 = ref new Border();

        border2 = ref new Border();

        border1->Child = rectangle;
        rootStackPanel->Children->Append(border1);
        rootStackPanel->Children->Append(border2);

        wh->WindowContent = rootStackPanel;
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        create_task(rtb->RenderAsync(rectangle)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });

        auto lte = wh->AddTestLTE(border1, rootStackPanel, LTEParentMode::NormalTree, false /* isAbsolutelyPositioned */);
        lte->Opacity = 0.0f;
    });
    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        ImageBrush^ brush = ref new ImageBrush();
        brush->ImageSource = rtb;
        border2->Background = brush;
        border2->Width = rtb->PixelWidth;
        border2->Height = rtb->PixelHeight;
    });
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void RenderTargetBitmapTests::RenderSameElementInternal()
{
    StackPanelTestHelper(L"RTBTests-Basic.xaml", false, false, false, 2);
}

void RenderTargetBitmapTests::TiledTextInternal(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup;
    XamlFileTestEngine engine;
    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();
    xaml_controls::Canvas^ canvas;
    RenderTargetBitmap^ rtb;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation;
    IBuffer^ buffer;

    // Test workflow:
    // Load markup
    // RTB target Canvas element with large TextBlock
    // Put generated RTB back into the live XAML tree
    // Generate MockDComp dump including surfaces and validate
    engine.SetDCompRenderingMode(dcompRendering);
    engine.SetXamlFilePath(GetResourcesPath() + L"RTBTests-LongTextBlock.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        canvas = safe_cast<xaml_controls::Canvas^>(rootElement->FindName(L"largeCanvas"));

        rtb = ref new RenderTargetBitmap();

        create_task(rtb->RenderAsync(canvas)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        renderedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    });
    engine.SetValidationCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        // Hide the primitives generated by the tiled TextBlock, we don't want or need to verify the
        // (multiple) surfaces it will produce.
        canvas->Opacity = 0;

        auto rtbCanvas = safe_cast<xaml_controls::Canvas^>(rootElement->FindName(L"rtbCanvas"));
        ImageBrush^ brush = ref new ImageBrush();
        brush->ImageSource = rtb;
        rtbCanvas->Background = brush;
        rtbCanvas->Width = rtb->PixelWidth;
        rtbCanvas->Height = rtb->PixelHeight;
    });
    engine.Execute();
}

void RenderTargetBitmapTests::BorderDiscardMaskOnResizeInternal(CacheMode^ cacheMode)
{
    const auto& wh = TestServices::WindowHelper;

    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();
    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;

    unsigned int width = 30;
    unsigned int height = 30;

    // Pick a place near the right edge. It starts half transparent, then turns fully opaque after getting a new brush,
    // then turns transparent black after the border is made wider (and that pixel is now in the center of the border).
    int x = 27;
    int y = 15;

    LOG_OUTPUT(L"[Border renders with alpha mask.]");
    Canvas^ rootCanvas;
    Border^ elem;
    RunOnUIThread([&]()
    {
        elem = ref new Border();
        elem->Width = width;
        elem->Height = height;
        elem->BorderThickness = ThicknessHelper::FromUniformLength(5);
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));

        rootCanvas = ref new Canvas();
        rootCanvas->CacheMode = cacheMode;
        rootCanvas->Children->Append(elem);
        wh->WindowContent = rootCanvas;

        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"  [Invoking RenderTargetBitmap::RenderAsync.]");
        create_task(rtb->RenderAsync(elem)).then([renderedEvent]() { renderedEvent->Set(); });
    });
    renderedEvent->WaitForDefault();
    renderedEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"  [Invoking RenderTargetBitmap::RenderAsync.]");
        create_task(rtb->RenderAsync(elem)).then([renderedEvent]() { renderedEvent->Set(); });
    });

    renderedEvent->WaitForDefault();
    renderedEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"  [GetPixelsAsync operation completed.]");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();
    getPixelsEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  [Verifying bitmap content.]");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, width * height * 4u);

        byte toVerify = 0x80;
        LOG_OUTPUT(L"toVerify = 0x%X", toVerify);

        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4] = 0x%X", generatedImage[(y * width + x) * 4]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 1] = 0x%X", generatedImage[(y * width + x) * 4 + 1]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 2] = 0x%X", generatedImage[(y * width + x) * 4 + 2]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 3] = 0x%X", generatedImage[(y * width + x) * 4 + 3]);

        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4], 0x00);    // bgra order
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 1], 0x00);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 2], toVerify);    // Premultiplied
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 3], toVerify);
    });

    LOG_OUTPUT(L"[Border gets an opaque brush. No longer renders with alpha mask.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0xff, 0xff, 0, 0));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"  [Invoking RenderTargetBitmap::RenderAsync.]");
        create_task(rtb->RenderAsync(elem)).then([renderedEvent]() { renderedEvent->Set(); });
    });

    renderedEvent->WaitForDefault();
    renderedEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"  [GetPixelsAsync operation completed.]");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();
    getPixelsEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  [Verifying bitmap content.]");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, width * height * 4u);

        byte toVerify = 0xff;
        LOG_OUTPUT(L"toVerify = 0x%X", toVerify);

        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4] = 0x%X", generatedImage[(y * width + x) * 4]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 1] = 0x%X", generatedImage[(y * width + x) * 4 + 1]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 2] = 0x%X", generatedImage[(y * width + x) * 4 + 2]);
        LOG_OUTPUT(L"generatedImage[(y * width + x) * 4 + 3] = 0x%X", generatedImage[(y * width + x) * 4 + 3]);

        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4], 0x00);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 1], 0x00);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 2], toVerify);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 3], toVerify);
    });

    LOG_OUTPUT(L"[Border gets a new size. The old alpha mask is now incorrect.]");
    RunOnUIThread([&]()
    {
        width = 60;
        elem->Width = width;
    });
    wh->WaitForIdle();

    LOG_OUTPUT(L"[Border gets a transparent brush, and renders with an alpha mask once again. The alpha mask should be updated with the new size.]");
    RunOnUIThread([&]()
    {
        elem->BorderBrush = ref new SolidColorBrush(ColorHelper::FromArgb(0x80, 0xff, 0, 0));
    });
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"  [Invoking RenderTargetBitmap::RenderAsync.]");
        create_task(rtb->RenderAsync(elem)).then([renderedEvent]() { renderedEvent->Set(); });
    });

    renderedEvent->WaitForDefault();
    renderedEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"  [GetPixelsAsync operation completed.]");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();
    getPixelsEvent->Reset();
    wh->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"  [Verifying bitmap content.]");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, width * height * 4u);

        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4], 0);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 1], 0);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 2], 0);
        VERIFY_ARE_EQUAL(generatedImage[(y * width + x) * 4 + 3], 0);
    });
}

void RenderTargetBitmapTests::RTB0SizeInternal()
{
    TestCleanupWrapper cleanup;

    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;

    XamlFileTestEngine engine;
    engine.SetMockDCompVerificationEnabled(false);
    engine.SetXamlFilePath(GetResourcesPath() + L"RTBTests-Element0Size.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        auto uiElement = safe_cast<xaml::UIElement^>(rootElement->FindName(L"grid"));

        rtb = ref new RenderTargetBitmap();

        create_task(rtb->RenderAsync(uiElement)).then([&renderedEvent] ()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        renderedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    });
    engine.Execute();

    // Get the pixels back and make sure it has no pixel data.
    RunOnUIThread([&] ()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent] (wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"GetPixelsAsync operation completed.");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(getPixelsCallback);
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });
    getPixelsEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&] ()
    {
        LOG_OUTPUT(L"Verifying empty bitmap content.");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);
        VERIFY_ARE_EQUAL(buffer->Length, 0u);
    });
    TestServices::WindowHelper->WaitForIdle();
}

void RenderTargetBitmapTests::BasicRenderTargetBitmapWithDebugDevice()
{
    RuntimeEnabledFeatureOverride featureUseDebugD3DDevice(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableDebugD3DDevice, true);
    BasicRenderTargetBitmapWUCFull();
}

void RenderTargetBitmapTests::BasicRenderTargetBitmapWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    BasicRenderTargetBitmapInternal();
}

void RenderTargetBitmapTests::PopupChildRTBWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    PopupChildRTBInternal();
}

void RenderTargetBitmapTests::PopupRTBWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    PopupRTBInternal();
}

void RenderTargetBitmapTests::RenderScrollViewerBitmapWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderScrollViewerBitmapInternal();
}

void RenderTargetBitmapTests::PanAfterRenderScrollViewerBitmapWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    PanAfterRenderScrollViewerBitmapInternal();
}

void RenderTargetBitmapTests::RenderTransformRTBWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderTransformRTBInternal();
}

void RenderTargetBitmapTests::RTLElementWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RTLElementInternal();
}

void RenderTargetBitmapTests::RTLwithinRTLWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RTLwithinRTLInternal();
}

void RenderTargetBitmapTests::LTRwithinRTLWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    LTRwithinRTLInternal();
}

void RenderTargetBitmapTests::RenderToSizeWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderToSizeInternal();
}

void RenderTargetBitmapTests::ScaledRenderWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
    ScaledRenderInternal();
}

void RenderTargetBitmapTests::BorderDiscardMaskOnResizeWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    BorderDiscardMaskOnResizeInternal(nullptr);
}

void RenderTargetBitmapTests::BorderDiscardMaskOnResizeBitmapCacheWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    CacheMode^ cacheMode;
    RunOnUIThread([&]() { cacheMode = ref new BitmapCache(); });

    // RTB'ing a child of BitmapCache should be the same as without BitmapCache now, since BitmapCache is a no-op.
    BorderDiscardMaskOnResizeInternal(cacheMode);
}

void RenderTargetBitmapTests::XamlCompositionBrushRTBWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    XamlCompositionBrushRTBInternal(true /* expectCaptureAsync */);
}

void RenderTargetBitmapTests::AncestorOpacity0WUCFull()
{
    // Leak: PVLStaggerFunction is leaked, resulting in 75KB of leaked memory
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    AncestorOpacity0Internal();
}

void RenderTargetBitmapTests::AncestorClippedWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    AncestorClippedInternal();
}

void RenderTargetBitmapTests::AncestorScaledTo0WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    // RTB'ing the child of an element scaled down to 0 in SpriteVisuals mode is expected to produce a completely transparent bitmap.
    AncestorScaledTo0Internal(true /* verifyIsTransparent */);
}

void RenderTargetBitmapTests::LTEOpacity0WUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    LTEOpacity0Internal();
}

void RenderTargetBitmapTests::RenderSameElementWUCFull()
{
    // - This bug is tracking the investigation of all StackPanelTestHelper related tests reporting leaks.
    //                It is highly suspected it is a test issue and the StackPanelTestHelper has been around for a while.
    //                The new test leak detection mechanism may have discovered a test leak that has gone undetected.
    TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RenderSameElementInternal();
}

void RenderTargetBitmapTests::TiledTextWUCFull()
{
    TiledTextInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void RenderTargetBitmapTests::RTBFirstFailWUCFull()
{
    // This test just checks for crash based on the conditions for a known issue
    TestCleanupWrapper cleanup;
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto renderedEvent = std::make_shared<Event>();
    RenderTargetBitmap^ rtb = nullptr;

    XamlFileTestEngine engine;
    engine.SetMockDCompVerificationEnabled(false);
    engine.SetXamlFilePath(GetResourcesPath() + L"RTBTests-AncestorCollapsed.xaml");
    engine.SetPostInitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        auto uiElement = safe_cast<xaml::UIElement^>(rootElement->FindName(L"rect"));

        rtb = ref new RenderTargetBitmap();

        create_task(rtb->RenderAsync(uiElement)).then([&renderedEvent] (Concurrency::task<void> renderAsyncTask)
        {
            // It is expected in this scenario for there to be a FailRender and thus an exception due to an ancestor element
            // of the RTB target being collapsed.  This test is meant to ensure this scenario doesn't crash.
            VERIFY_THROWS_WINRT(renderAsyncTask.get(), Platform::Exception^, L"RenderAsync threw an exception as expected");

            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");
            renderedEvent->Set();
        });
    });
    engine.SetPostInitWaitCallback([&] (xaml::FrameworkElement^ rootElement)
    {
        renderedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    });
    engine.Execute();
}

void RenderTargetBitmapTests::RTB0SizeWUCFull()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);
    RTB0SizeInternal();
}

void RenderTargetBitmapTests::EllipseTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    uint32_t red = RgbaToUint32(255, 0, 0, 255);
    uint32_t blue = RgbaToUint32(0, 0, 255, 255);
    uint32_t yellow = RgbaToUint32(0, 255, 255, 255);

    TestShapeInternal(L"ShapeTests-Ellipse.xaml", [&](Platform::Array<uint32_t>^ buffer, int w, int h)
    {
        VERIFY_ARE_EQUAL(w, 100);
        VERIFY_ARE_EQUAL(h, 100);

        // Verify if the top left pixel is red
        VERIFY_ARE_EQUAL(buffer[PixelIndex(0, 0, w)], red);

        // Verify if the top left (NW) pixel outside of the ellipse stroke is red
        VERIFY_ARE_EQUAL(buffer[PixelIndex(13, 13, w)], red);

        // Verify if the top left (NW) pixel inside of the ellipse stroke (outer edge) is blue
        VERIFY_ARE_EQUAL(buffer[PixelIndex(15, 15, w)], blue);

        // Verify if the top left (NW) pixel inside of the ellipse stroke (inner edge) is blue
        VERIFY_ARE_EQUAL(buffer[PixelIndex(20, 20, w)], blue);

        // Verify if the top left (NW) pixel outside of the ellipse stroke (inner edge) is yellow (ellipse fill color)
        VERIFY_ARE_EQUAL(buffer[PixelIndex(23, 23, w)], yellow);

        // Verify if the center pixel is yellow
        VERIFY_ARE_EQUAL(buffer[PixelIndex(50, 50, w)], yellow);
    });
}

void RenderTargetBitmapTests::LineTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    uint32_t red = RgbaToUint32(255, 0, 0, 255);
    uint32_t blue = RgbaToUint32(0, 0, 255, 255);

    TestShapeInternal(L"ShapeTests-Line.xaml", [&](Platform::Array<uint32_t>^ buffer, int w, int h)
    {
        VERIFY_ARE_EQUAL(w, 100);
        VERIFY_ARE_EQUAL(h, 100);

        //Point just outside of the stroke, first row
        VERIFY_ARE_EQUAL(buffer[PixelIndex(9, 0, w)], red);

        //Point on the stroke which should be fully opaque (when using composition / re-rasterized)
        VERIFY_ARE_EQUAL(buffer[PixelIndex(7, 0, w)], blue);

        //Point in the middle of the screen, should be covered by the path and be fully opaque
        VERIFY_ARE_EQUAL(buffer[PixelIndex(50, 50, w)], blue);
    });
}

void RenderTargetBitmapTests::PathTest()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    uint32_t red = RgbaToUint32(255, 0, 0, 255);
    uint32_t blue = RgbaToUint32(0, 0, 255, 255);

    TestShapeInternal(L"ShapeTests-Path.xaml", [&](Platform::Array<uint32_t>^ buffer, int w, int h)
    {
        VERIFY_ARE_EQUAL(w, 100);
        VERIFY_ARE_EQUAL(h, 100);

        //Point just outside of the stroke, first row
        VERIFY_ARE_EQUAL(buffer[PixelIndex(9, 0, w)], red);

        //Point on the stroke which should be fully opaque (when using composition / re-rasterized)
        VERIFY_ARE_EQUAL(buffer[PixelIndex(7, 0, w)], blue);

        //Point in the middle of the screen, should be covered by the path and be fully opaque
        VERIFY_ARE_EQUAL(buffer[PixelIndex(50, 50, w)], blue);

    });
}

unsigned int RenderTargetBitmapTests::PixelIndex(unsigned int x, unsigned int y, unsigned int w)
{
    return (y * w) + x;
}

uint32_t RenderTargetBitmapTests::RgbaToUint32(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return r << 24 | g << 16 | b << 8 | a;
}

void RenderTargetBitmapTests::TestShapeInternal(Platform::String^ fileName, std::function<void(Platform::Array<uint32_t>^, int, int)> verifyFunc)
{
    TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(100, 100));

    auto renderedEvent = std::make_shared<Event>();
    auto getPixelsEvent = std::make_shared<Event>();

    RenderTargetBitmap^ rtb = nullptr;
    wf::IAsyncOperation<IBuffer^>^ getPixelsAsyncOperation = nullptr;
    IBuffer^ buffer = nullptr;

    StackPanel^ rootStackPanel = safe_cast<StackPanel^>(LoadXamlFileOnUIThread(GetResourcesPath() + fileName));

    TestServices::WindowHelper->WaitForIdle();

    FrameworkElement^ gridElement = nullptr;
    Storyboard^ animation = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootStackPanel;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        gridElement = safe_cast<FrameworkElement^>(rootStackPanel->FindName(L"gridParent"));
        VERIFY_IS_NOT_NULL(gridElement);

        animation = safe_cast<Storyboard^>(gridElement->FindName(L"shapeAnimation"));
        VERIFY_IS_NOT_NULL(animation);

        animation->Begin();
    });

    TestServices::WindowHelper->SynchronouslyTickUIThread(3);

    auto h = 0;
    auto w = 0;

    RunOnUIThread([&]()
    {
        rtb = ref new RenderTargetBitmap();
        LOG_OUTPUT(L"Invoking RenderTargetBitmap::RenderAsync.");

        create_task(rtb->RenderAsync(gridElement)).then([&renderedEvent]()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");

            renderedEvent->Set();
        });
    });

    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        getPixelsAsyncOperation = rtb->GetPixelsAsync();

        h = rtb->PixelHeight;
        w = rtb->PixelWidth;

        auto getPixelsCallback = ref new wf::AsyncOperationCompletedHandler<IBuffer^>(
            [&buffer, getPixelsEvent](wf::IAsyncOperation<IBuffer^>^ operation, wf::AsyncStatus)
        {
            LOG_OUTPUT(L"GetPixelsAsync operation completed.");
            buffer = operation->GetResults();
            getPixelsEvent->Set();
        });
        VERIFY_IS_NOT_NULL(getPixelsCallback);
        getPixelsAsyncOperation->Completed = getPixelsCallback;
    });

    getPixelsEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        animation->Stop();
    });

    RunOnUIThread([&]()
    {
        Image^ img = safe_cast<Image^>(rootStackPanel->FindName(L"img"));
        VERIFY_IS_NOT_NULL(img);

        img->Source = rtb;
        img->Width = gridElement->ActualWidth;
        img->Height = gridElement->ActualHeight;
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Verifying bitmap content.");
        DataReader^ dataReader = DataReader::FromBuffer(buffer);
        Platform::Array<byte>^ generatedImage = ref new Platform::Array<byte>(buffer->Length);
        dataReader->ReadBytes(generatedImage);

        Platform::Array<uint32_t>^ rgbPixels = ref new Platform::Array<uint32_t>(buffer->Length / 4);
        for (unsigned int i = 0; i < buffer->Length / 4; i++)
        {
            rgbPixels[i / 4] = RgbaToUint32(generatedImage[0 + i], generatedImage[1 + i], generatedImage[2 + i], generatedImage[3 + i]);
        }

        verifyFunc(rgbPixels, w, h);
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
}

void RenderTargetBitmapTests::RasterizationScale()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    Canvas^ root;
    RenderTargetBitmap^ rtb;
    auto renderedEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Setting up tree.");

        xaml_shapes::Ellipse^ ellipse = ref new xaml_shapes::Ellipse();
        ellipse->Width = 50;
        ellipse->Height = 50;
        ellipse->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 0));
        ellipse->RasterizationScale = 0.1f;

        xaml_shapes::Ellipse^ ellipse2 = ref new xaml_shapes::Ellipse();
        ellipse2->Width = 50;
        ellipse2->Height = 50;
        ellipse2->Fill = ref new xaml_media::SolidColorBrush(ColorHelper::FromArgb(255, 0, 0, 0));

        Canvas^ target = ref new Canvas();
        target->Children->Append(ellipse);
        target->Children->Append(ellipse2);
        Canvas::SetLeft(ellipse2, 50);

        root = ref new Canvas();
        root->Children->Append(target);
        wh->WindowContent = root;

        LOG_OUTPUT(L"> Invoking RenderTargetBitmap::RenderAsync.");

        rtb = ref new RenderTargetBitmap();
        create_task(rtb->RenderAsync(target)).then([&renderedEvent]()
        {
            LOG_OUTPUT(L"RenderTargetBitmap::RenderAsync completed.");

            renderedEvent->Set();
        });
    });
    renderedEvent->WaitForDefault();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"> Putting RTB back in the tree.");

        xaml_controls::Image^ image = ref new xaml_controls::Image();
        image->Source = rtb;

        root->Children->Clear();
        root->Children->Append(image);
    });
    wh->WaitForIdle();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } }
