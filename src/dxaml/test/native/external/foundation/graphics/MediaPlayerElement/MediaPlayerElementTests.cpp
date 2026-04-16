// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include "TestCleanupWrapper.h"
#include "MediaPlayerElementTests.h"
#include "FileLoader.h"
#include "TreeHelper.h"
#include "EventHelpers.h"
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Media::Playback;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

Platform::String^ MediaPlayerElementTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\media\\";
}

bool MediaPlayerElementTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool MediaPlayerElementTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool MediaPlayerElementTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void MediaPlayerElementTests::SmokeTestApi()
{
    TestCleanupWrapper cleanup;
    xaml_controls::MediaPlayerElement^ mpe;
    xaml_controls::Grid^ grid;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating in code...");
        mpe = ref new xaml_controls::MediaPlayerElement();
        VERIFY_IS_NULL(mpe->Source);

        LOG_OUTPUT(L"Creating in XAML...");
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <MediaPlayerElement x:Name='theMedia' />"
            L"</Grid>"));

        mpe = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
        VERIFY_IS_NOT_NULL(mpe);

        TestServices::WindowHelper->WindowContent = grid;
    });
}

void MediaPlayerElementTests::SmokeTestPeer()
{
    TestCleanupWrapper cleanup;
    xaml_controls::MediaPlayerElement^ mpe;
    xaml_controls::Grid^ grid;

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating in XAML...");
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <MediaPlayerElement x:Name='theMedia' />"
            L"</Grid>"));

        mpe = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
        auto peer = ref new Microsoft::UI::Xaml::Automation::Peers::MediaPlayerElementAutomationPeer(mpe);

        TestServices::WindowHelper->WindowContent = grid;
    });
}

xaml_controls::MediaPlayerElement^ CreateMediaPlayerElement(const wchar_t* mediaPlayerElementXaml, const wchar_t* name=L"theMedia")
{
    xaml_controls::MediaPlayerElement^ testMediaPlayerElement = nullptr;
    xaml_controls::Grid^ grid = nullptr;

    RunOnUIThread([&]()
    {
        std::wstring xamlStr;
        xamlStr += L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>";
        xamlStr += L"\r\n";
        xamlStr += mediaPlayerElementXaml;
        xamlStr += L"\r\n";
        xamlStr += L"</Grid>";
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ref new Platform::String(xamlStr.c_str())));

        TestServices::WindowHelper->WindowContent = grid;
        testMediaPlayerElement = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(ref new Platform::String(name)));
        VERIFY_IS_NOT_NULL(testMediaPlayerElement);
    });

    TestServices::WindowHelper->WaitForIdle();

    return testMediaPlayerElement;
}

void MediaPlayerElementTests::CanEnableDisableMTC()
{
    TestCleanupWrapper cleanup;

    xaml_controls::MediaPlayerElement^ testMediaPlayerElement = CreateMediaPlayerElement(L"<MediaPlayerElement x:Name='theMedia'/>");

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Enable MTC Controls");
        testMediaPlayerElement->AreTransportControlsEnabled = true;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_TRUE(testMediaPlayerElement->AreTransportControlsEnabled);
        VERIFY_IS_NOT_NULL(testMediaPlayerElement->TransportControls);
        VERIFY_ARE_EQUAL(testMediaPlayerElement->TransportControls->Visibility, Visibility::Visible);
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Disable MTC Controls");
        testMediaPlayerElement->AreTransportControlsEnabled = false;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_FALSE(testMediaPlayerElement->AreTransportControlsEnabled);
        VERIFY_ARE_EQUAL(testMediaPlayerElement->TransportControls->Visibility, Visibility::Collapsed);
    });

    xaml_controls::MediaTransportControls ^otherMTC;
    RunOnUIThread([&]()
    {
        otherMTC = ref new xaml_controls::MediaTransportControls;
        testMediaPlayerElement->TransportControls = otherMTC;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(testMediaPlayerElement->TransportControls->Visibility, Visibility::Collapsed);
        testMediaPlayerElement->AreTransportControlsEnabled = true;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(testMediaPlayerElement->TransportControls->Visibility, Visibility::Visible);
    });

    RunOnUIThread([&]()
    {
        testMediaPlayerElement->TransportControls = nullptr;
        testMediaPlayerElement->AreTransportControlsEnabled = false;
        testMediaPlayerElement->AreTransportControlsEnabled = true;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_NULL(testMediaPlayerElement->TransportControls);
    });
}

void SetMediaSource(xaml_controls::MediaPlayerElement^ mpe, Platform::String^ urlStr)
{
    RunOnUIThread([&]()
    {
        auto source = ::Windows::Media::Core::MediaSource::CreateFromUri(ref new ::Windows::Foundation::Uri(urlStr));
        auto item = ref new ::Windows::Media::Playback::MediaPlaybackItem(source);
        mpe->Source = item;
        LOG_OUTPUT(L"MediaPlayerElement got new source");
    });

    TestServices::WindowHelper->WaitForIdle();
}

void MediaPlayerElementTests::ParseSourceUri()
{
    TestCleanupWrapper cleanup;

    xaml_controls::MediaPlayerElement^ testMediaPlayerElement;
    xaml_controls::Grid^ grid;

    RunOnUIThread([&]()
    {
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <MediaPlayerElement x:Name='theMedia' Source='ms-appx:///resources/native/foundation/graphics/Media/CastingVideo.mp4' />"
            L"</Grid>"));

        TestServices::WindowHelper->WindowContent = grid;
        testMediaPlayerElement = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
        VERIFY_IS_NOT_NULL(testMediaPlayerElement);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        VERIFY_IS_NOT_NULL(testMediaPlayerElement->Source);
    });
    TestServices::WindowHelper->WaitForIdle();
}

void MediaPlayerElementTests::PeerHasLocalizedControlType()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Grid^ grid;
    xaml_controls::MediaPlayerElement^ mediaPlayerElement;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating in XAML...");
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <MediaPlayerElement x:Name='theMedia' />"
            L"</Grid>"));

        TestServices::WindowHelper->WindowContent = grid;

        mediaPlayerElement = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
    });

    TestServices::WindowHelper->WaitForIdle();

    Microsoft::UI::Xaml::Automation::Peers::MediaPlayerElementAutomationPeer^ peer = nullptr;
    RunOnUIThread([&]()
    {
        peer = ref new Microsoft::UI::Xaml::Automation::Peers::MediaPlayerElementAutomationPeer(mediaPlayerElement);
        VERIFY_IS_NOT_NULL(peer);

        auto controlType = peer->GetAutomationControlType();
        VERIFY_ARE_EQUAL(Microsoft::UI::Xaml::Automation::Peers::AutomationControlType::Custom, controlType);

        Platform::String^ localizedControlType = peer->GetLocalizedControlType();
        VERIFY_ARE_EQUAL(ref new Platform::String(L"media player"), localizedControlType);
    });

    TestServices::WindowHelper->WaitForIdle();
}

void MediaPlayerElementTests::PosterSource()
{
    TestCleanupWrapper cleanup;

    xaml_controls::Grid^ grid;
    xaml_controls::MediaPlayerElement^ mediaPlayerElement;
    xaml_controls::Image^ posterImage;
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Creating in XAML...");
        grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
            L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
            L"  <MediaPlayerElement x:Name='theMedia' PosterSource='ms-appx:///resources/native/external/foundation/graphics/image/barcelona.jpg' />"
            L"</Grid>"));

        TestServices::WindowHelper->WindowContent = grid;

        mediaPlayerElement = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        posterImage = safe_cast<xaml_controls::Image^>(TreeHelper::GetVisualChildByName(mediaPlayerElement, "PosterImage"));
        VERIFY_IS_NOT_NULL(posterImage);
        VERIFY_ARE_EQUAL(posterImage->Visibility, xaml::Visibility::Visible);
    });

    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set PosterSource property to null");
        mediaPlayerElement->PosterSource = nullptr;
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(posterImage->Visibility, xaml::Visibility::Collapsed);
    });

    auto posterSourceUri = ref new Uri("ms-appx:///resources/native/external/foundation/graphics/media/barcelona.jpg");
    RunOnUIThread([&]()
    {
        LOG_OUTPUT(L"Set PosterSource property");
        mediaPlayerElement->PosterSource = ref new Microsoft::UI::Xaml::Media::Imaging::BitmapImage(posterSourceUri);
    });
    TestServices::WindowHelper->WaitForIdle();
    RunOnUIThread([&]()
    {
        VERIFY_ARE_EQUAL(posterImage->Visibility, xaml::Visibility::Visible);
    });
    TestServices::WindowHelper->WaitForIdle();
}

void MediaPlayerElementTests::RasterizationScale(double mediaPlayerRasterizationScale, double rootRasterizationScale)
{
    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, true /*resizeWindow*/);

    auto eventRegistration = CreateSafeEventRegistration(MediaPlaybackSession, PlaybackStateChanged);
    auto playbackStateChangedEvent = std::make_shared<Event>();
    xaml_controls::MediaPlayerElement^ mediaPlayerElement;

    RunOnUIThread([&]()
    {
        mediaPlayerElement = ref new xaml_controls::MediaPlayerElement();
        mediaPlayerElement->Width = 100;
        mediaPlayerElement->Height = 100;
        mediaPlayerElement->AutoPlay = true;
        mediaPlayerElement->RasterizationScale = mediaPlayerRasterizationScale;

        eventRegistration.Attach(
            mediaPlayerElement->MediaPlayer->PlaybackSession,
            ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^ session, Platform::Object^)
            {
                if (session->PlaybackState == MediaPlaybackState::Playing)
                {
                    LOG_OUTPUT(L"> MediaPlaybackSession.PlaybackStateChanged fired, current state is Playing");
                    playbackStateChangedEvent->Set();
                }
            }));

        xaml_controls::Canvas^ root = ref new xaml_controls::Canvas();
        root->RasterizationScale = rootRasterizationScale;
        root->Children->Append(mediaPlayerElement);
        wh->WindowContent = root;

        mediaPlayerElement->Source = ::Windows::Media::Core::MediaSource::CreateFromUri(ref new Uri(GetResourcesPath() + L"testfile.wmv"));
    });

    playbackStateChangedEvent->WaitForDefault();
    wh->WaitForIdle();
    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Inline");
}

void MediaPlayerElementTests::MediaRasterizationScale()
{
    RasterizationScale(2.0, 1.0);
}

void MediaPlayerElementTests::RootRasterizationScale()
{
    RasterizationScale(1.0, 2.0);
}

    } } }
} } } }
