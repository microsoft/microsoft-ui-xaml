// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ClosedCaptionTestsMPE.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include "TestCleanupWrapper.h"
#include <SafeEventRegistration.h>

#include "Utils.h"
#include "MediaHelperMPE.h"
#include "FileLoader.h"
#include <Collection.h>
#include <ETWWaiterProxy.h>
#include <MUX-ETWEvents.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace ::Windows::Media::Playback;
using namespace ::Windows::Media::Core;

using namespace Private::Infrastructure;

#define HUNDRED_NS_PER_MS 10000

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        Platform::String^ MediaCCTestsMPE::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\media\\";
        }

        bool MediaCCTestsMPE::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool MediaCCTestsMPE::TestSetup()
        {
            Private::Infrastructure::TestServices::WindowHelper->InitializeXaml();

            // TODO: [MediaPlayerElement] Native CC tests leak 36 bytes via DirectUI::CTimedTextSource::AddMediaPlayerEventRegistration
            TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

            return true;
        }


        bool MediaCCTestsMPE::TestCleanup()
        {
            Private::Infrastructure::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void MediaCCTestsMPE::MediaPlayerElementWithImageCueHelper(bool userMTC, int numberOfCues, bool usePercentage)
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            ETWWaiterProxy CueEnteredEtwWaiter;
            CueEnteredEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                TimedTextCueEnd_value);

            ETWWaiterProxy CueRemovedEtwWaiter;
            CueRemovedEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                CueRemoved_value);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            MediaHelperMPE::SetupMediaPlayerElementWithImageCue(mediaHelper, GetResourcesPath(), userMTC, numberOfCues, usePercentage);

            auto seekCompleteRegistration = CreateSafeEventRegistration(MediaPlaybackSession, SeekCompleted);
            auto seekCompletedEvent = std::make_shared<Event>();

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                seekCompleteRegistration.Attach(
                    mpe->MediaPlayer->PlaybackSession,
                    ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^, Platform::Object^)
                    {
                        seekCompletedEvent->Set();
                        LOG_OUTPUT(L"Seek Completed");
                    }));

            });
            TestServices::WindowHelper->WaitForIdle();

            ::Windows::Foundation::TimeSpan ts;
            LOG_OUTPUT(L"Seeking to 2 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 2000 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            CueEnteredEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree("ImageCueOn");

            LOG_OUTPUT(L"Seeking to  4 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 4000 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            CueRemovedEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree("ImageCueOff");
        }

        void MediaCCTestsMPE::MediaPlayerElementWithTextCueHelper(bool userMTC, Platform::String^ ccFile, Platform::String^ lang)
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            ETWWaiterProxy CueEnteredEtwWaiter;
            CueEnteredEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                TimedTextCueEnd_value);

            ETWWaiterProxy CueRemovedEtwWaiter;
            CueRemovedEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                CueRemoved_value);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            MediaHelperMPE::SetupMediaPlayerElementWithCC(mediaHelper, GetResourcesPath(), ccFile, userMTC, lang);

            auto seekCompleteRegistration = CreateSafeEventRegistration(MediaPlaybackSession, SeekCompleted);
            auto seekCompletedEvent = std::make_shared<Event>();

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                seekCompleteRegistration.Attach(
                    mpe->MediaPlayer->PlaybackSession,
                    ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^, Platform::Object^)
                    {
                        seekCompletedEvent->Set();
                        LOG_OUTPUT(L"Seek Completed");
                    }));

            });
            TestServices::WindowHelper->WaitForIdle();

            ::Windows::Foundation::TimeSpan ts;
            LOG_OUTPUT(L"Seeking to .2 sec..");
            RunOnUIThread([&]()
            {
                ts.Duration = 200 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            CueEnteredEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree("CueOn");

            LOG_OUTPUT(L"Seeking to  6 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 6000 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            CueRemovedEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree("CueOff");
            TestServices::WindowHelper->WaitForIdle();
        }

        void MediaCCTestsMPE::MediaPlayerElementWithImageCueNoMTCUsePercent()
        {
            MediaPlayerElementWithImageCueHelper(false /*userMTC*/, 1 /*numberOfCues*/, true /*usePercentage*/);
        }

        void MediaCCTestsMPE::MediaPlayerElementWithImageCueNoMTCUsePixels()
        {
            MediaPlayerElementWithImageCueHelper(false /*userMTC*/, 1 /*numberOfCues*/, false /*usePercentage*/);
        }

        void MediaCCTestsMPE::MediaPlayerElementWithMultipleImageCues()
        {
            MediaPlayerElementWithImageCueHelper(false /*userMTC*/, 2 /*numberOfCues*/, true /*usePercentage*/);
        }
        //------------------------------------------------------------------------
        // Test case: Closed captioning with a custom style
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithCustomStyle()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"cc_style.ttm" /*ccFile*/,  L"eng" /*lang*/);
        }

        //------------------------------------------------------------------------
        // Test case: Closed captioning with a custom region
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithCustomRegion()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"cc_region.ttm" /*ccFile*/, L"eng" /*lang*/);
        }

        //------------------------------------------------------------------------
        // Test case: Closed captioning with multiple lines per cue
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithLines()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));

            ETWWaiterProxy CueEnteredEtwWaiter;
            CueEnteredEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                TimedTextCueEnd_value);

            ETWWaiterProxy CueRemovedEtwWaiter;
            CueRemovedEtwWaiter.Start(
                WINDOWS_UI_XAML_DIAG_ETW_PROVIDER,
                CueRemoved_value);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            MediaHelperMPE::SetupMediaPlayerElementWithCC(mediaHelper, GetResourcesPath(), L"cc_lines.ttm", false, L"eng");

            auto seekCompleteRegistration = CreateSafeEventRegistration(MediaPlaybackSession, SeekCompleted);
            auto seekCompletedEvent = std::make_shared<Event>();

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                seekCompleteRegistration.Attach(
                    mpe->MediaPlayer->PlaybackSession,
                    ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^, Platform::Object^)
                    {
                        seekCompletedEvent->Set();
                        LOG_OUTPUT(L"Seek Completed");
                    }));

            });
            TestServices::WindowHelper->WaitForIdle();

            ::Windows::Foundation::TimeSpan ts;
            LOG_OUTPUT(L"Seeking to 1.5 sec..");
            RunOnUIThread([&]()
            {
                ts.Duration = 1500 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            CueEnteredEtwWaiter.WaitForDefault();
            TestServices::Utilities->VerifyUIElementTree("CueOn-1");

            LOG_OUTPUT(L"Seeking to 2.5 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 2500 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            CueEnteredEtwWaiter.WaitForDefault();
            TestServices::Utilities->VerifyUIElementTree("CueOn-2");

            LOG_OUTPUT(L"Seeking to 3.5 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 3500 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            CueEnteredEtwWaiter.WaitForDefault();
            TestServices::Utilities->VerifyUIElementTree("CueOn-3");

            LOG_OUTPUT(L"Seeking to 4.1 secs...");
            RunOnUIThread([&]()
            {
                ts.Duration = 4100 * HUNDRED_NS_PER_MS;
                mpe->MediaPlayer->Position = ts;
            });
            seekCompletedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            CueRemovedEtwWaiter.WaitForDefault();
            TestServices::Utilities->VerifyUIElementTree("CueOff");
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Closed captioning with a bad ttm
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithBadFile()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            auto errorRegistration = CreateSafeEventRegistration(TimedTextSource, Resolved);
            auto errorEvent = std::make_shared<Event>();

            TestServices::WindowHelper->SetWindowSizeOverride(Size(400, 300));

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            MediaHelperMPE::SetupBasicMediaPlayerElement(mediaHelper, GetResourcesPath(), L"blueframe_video.mp4", false, true, true);

            RunOnUIThread([&]()
            {
                auto ccUri = ref new Uri(GetResourcesPath() + L"cc_bad.ttm");
                VERIFY_IS_NOT_NULL(ccUri);

                auto textSource = TimedTextSource::CreateFromUri(ccUri, nullptr);

                errorRegistration.Attach(
                    textSource,
                    ref new ::Windows::Foundation::TypedEventHandler<::Windows::Media::Core::TimedTextSource^, TimedTextSourceResolveResultEventArgs^>(
                    [errorEvent, mpe](::Windows::Media::Core::TimedTextSource^ sender, TimedTextSourceResolveResultEventArgs^ arg)
                {
                    LOG_OUTPUT(L"Error Event Fired");
                    errorEvent->Set();
                }));

                MediaHelperMPE::s_CurrentPlaybackItem->Source->ExternalTimedTextSources->Append(textSource);
            });
            LOG_OUTPUT(L"Starting video");
            RunOnUIThread([&]()
            {
                mpe->MediaPlayer->Play();
            });

            LOG_OUTPUT(L"Waiting for error event on bad file");
            TestServices::WindowHelper->WaitForIdle();
            errorEvent->WaitForDefault();
        }

        //------------------------------------------------------------------------
        // Test case: Closed captioning with a custom style
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithSubformatting()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"cc_subformat.ttm" /*ccFile*/, L"eng" /*lang*/);
        }

        //------------------------------------------------------------------------
        // Test case: A single line with text in multiple colors
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithMultipleColors()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"Color008.xml" /*ccFile*/, L"en" /*lang*/);
        }

        //------------------------------------------------------------------------
        // Test case: A single line with 2 pixel red outline
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::MediaPlayerElementWithOutline()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"TextOutline004.xml" /*ccFile*/, L"en" /*lang*/);
        }

        void MediaCCTestsMPE::RemovedWhenDisabled()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            MediaHelperMPE::SetupMediaPlayerElementWithCC(mediaHelper, GetResourcesPath(), L"cc_longduration.ttm", false, L"eng");

            auto cueRegistration = CreateSafeEventRegistration(TimedMetadataTrack, CueEntered);
            auto cueEvent = std::make_shared<Event>();

            auto cueExitRegistration = CreateSafeEventRegistration(TimedMetadataTrack, CueExited);
            auto cueExitedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                VERIFY_IS_NOT_NULL(MediaHelperMPE::s_CurrentPlaybackItem);

                auto track = MediaHelperMPE::s_CurrentPlaybackItem->TimedMetadataTracks->GetAt(0);
                VERIFY_IS_NOT_NULL(track);

                cueRegistration.Attach(
                    track,
                    ref new ::Windows::Foundation::TypedEventHandler<::Windows::Media::Core::TimedMetadataTrack^, ::Windows::Media::Core::MediaCueEventArgs^>(
                        [&](::Windows::Media::Core::TimedMetadataTrack^ sender, ::Windows::Media::Core::MediaCueEventArgs^ arg)
                {
                    LOG_OUTPUT(L"Processing cue");

                    TimedTextCue^ cue = dynamic_cast<TimedTextCue^>(arg->Cue);
                    VERIFY_IS_NOT_NULL(cue);

                    LOG_OUTPUT(L"processing line 1");
                    VERIFY_IS_TRUE(cue->Lines->Size == 1);
                    VERIFY_IS_TRUE(Platform::String::CompareOrdinal(cue->Lines->GetAt(0)->Text, L"Caption 1") == 0);

                    VERIFY_IS_TRUE(CheckTimeWithError(cue->StartTime.Duration, 1000000)); // .1s in HNS

                    VERIFY_IS_TRUE(CheckTimeWithError(cue->Duration.Duration, 600000000, 50i64));  // 60s in HNS, error margin is less than 1e-7, default tolerance a little narrow for such a large number

                    TimedTextStyle^ style = cue->CueStyle;
                    VERIFY_IS_NOT_NULL(style);

                    VERIFY_IS_TRUE(Platform::String::CompareOrdinal(style->FontFamily, L"default") == 0);
                    cueEvent->Set();
                }));

                cueExitRegistration.Attach(track, [&]() { cueExitedEvent->Set(); });

                LOG_OUTPUT(L"Starting video");
                mpe->MediaPlayer->Play();
            });
            TestServices::WindowHelper->WaitForIdle();
            cueEvent->WaitForDefault();

            // This cue will stay on screen for 60 seconds unless the track is disabled programmatically.  By disabling the track and waiting for the CueExited event
            // we verify that the framework removes the cue due to the PresentationModeChanged event and not the track expiring due to it's duration
            LOG_OUTPUT(L"Disabling the cues, they should be removed.");
            RunOnUIThread([&]()
            {
                MediaHelperMPE::s_CurrentPlaybackItem->TimedMetadataTracks->SetPresentationMode(0, TimedMetadataTrackPresentationMode::Disabled);
            });
            TestServices::WindowHelper->WaitForIdle();
            cueExitedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();
        }

        void MediaCCTestsMPE::MediaPlayerElementWithCentering()
        {
            MediaPlayerElementWithTextCueHelper(false /*userMTC*/, L"Style001.xml" /*ccFile*/, L"en" /*lang*/);
        }

    } } }
} } } }
