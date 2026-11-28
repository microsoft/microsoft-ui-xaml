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
#include <TreeHelper.h>
#include <WindowsNumerics.h>
#include <algorithm>
#include <atomic>

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

        namespace
        {
            void RestoreCaptionSettingsLeftByKilledRun();
        }

        Platform::String^ MediaCCTestsMPE::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\media\\";
        }

        bool MediaCCTestsMPE::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            // The caption tests (and their master files) depend on the Windows caption settings: undo a change left by a
            // CaptionSettingsScope whose test host was killed.
            RestoreCaptionSettingsLeftByKilledRun();
            return true;
        }

        bool MediaCCTestsMPE::TestSetup()
        {
            Private::Infrastructure::TestServices::WindowHelper->InitializeXaml();

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

        // Regression: any MediaPlaybackItem makes CTimedTextSource register a TimedMetadataTracksChanged handler; teardown must remove it, not orphan it (no CC content required to reproduce the leak).
        void MediaCCTestsMPE::TimedTextSourceTeardownDoesNotLeak()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            {
                MediaHelperMPE mediaHelper;

                // Sets mpe->Source to a MediaPlaybackItem, so CTimedTextSource registers its track handler.
                MediaHelperMPE::SetupBasicMediaPlayerElement(mediaHelper, GetResourcesPath(), L"blueframe_video.mp4", false /*useMTC*/, true /*waitForOpened*/, true /*useNewPlayback*/);

                TestServices::WindowHelper->WaitForIdle();
            }

            // Detach the MediaPlayerElement, driving MediaPlayerElement::LeaveImpl -> CTimedTextSource::SetMediaPlayer(nullptr).
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            TestServices::WindowHelper->WaitForIdle();

            // No IgnoreLeaksForTest: TestCleanup -> VerifyTestCleanup asserts the track registration was released.
        }

        //------------------------------------------------------------------------
        // Helpers for the CueStyler tests below. Captions are built in code (TimedTextCue/TimedTextStyle/TimedTextRegion)
        // so every style input is explicit, and the rendered XAML elements are inspected instead of master files.
        //------------------------------------------------------------------------
        namespace
        {
            namespace wmcc = ::Windows::Media::ClosedCaptioning;

            const INT64 c_hnsPerMs = 10000;
            const double c_blueframeNaturalHeight = 480.0; // blueframe_video.mp4 is 854x480.
            const double c_tolerance = 0.01;
            const wchar_t c_captionSettingsKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\ClosedCaptioning";

            // Windows caption settings (Settings > Accessibility > Captions), backing ClosedCaptionProperties.
            struct CaptionSettings
            {
                wmcc::ClosedCaptionColor fontColor = wmcc::ClosedCaptionColor::Default;
                wmcc::ClosedCaptionOpacity fontOpacity = wmcc::ClosedCaptionOpacity::Default;
                wmcc::ClosedCaptionSize fontSize = wmcc::ClosedCaptionSize::Default;
                wmcc::ClosedCaptionStyle fontStyle = wmcc::ClosedCaptionStyle::Default;
                wmcc::ClosedCaptionEdgeEffect edgeEffect = wmcc::ClosedCaptionEdgeEffect::None;
                wmcc::ClosedCaptionColor backgroundColor = wmcc::ClosedCaptionColor::Default;
                wmcc::ClosedCaptionOpacity backgroundOpacity = wmcc::ClosedCaptionOpacity::Default;
                wmcc::ClosedCaptionColor regionColor = wmcc::ClosedCaptionColor::Default;
                wmcc::ClosedCaptionOpacity regionOpacity = wmcc::ClosedCaptionOpacity::Default;
            };

            const wchar_t* const c_captionSettingNames[] =
            {
                L"CaptionColor", L"CaptionOpacity", L"CaptionSize", L"CaptionFontStyle", L"CaptionEdgeEffect",
                L"BackgroundColor", L"BackgroundOpacity", L"RegionColor", L"RegionOpacity",
            };

            // Test-owned copy of the original caption settings, kept while a CaptionSettingsScope may have changed them, so
            // that a run whose host is killed before the scope's destructor runs is repaired by the next run.
            const wchar_t c_captionSettingsBackupKey[] = L"Software\\Microsoft\\Microsoft.UI.Xaml.Tests\\ClosedCaptioningBackup";
            // Bit i is set when c_captionSettingNames[i] existed. Written after the backed up values, so it marks a complete backup.
            const wchar_t c_captionSettingsBackupExistedMask[] = L"ExistedMask";

            // Restores the settings to 'original' (deleting the values that did not exist); returns false if any restore failed.
            bool RestoreCaptionSettings(const DWORD (&original)[ARRAYSIZE(c_captionSettingNames)], DWORD existedMask)
            {
                bool restored = true;
                for (size_t i = 0; i < ARRAYSIZE(c_captionSettingNames); i++)
                {
                    const LSTATUS status = (existedMask & (1u << i)) ?
                        RegSetKeyValueW(HKEY_CURRENT_USER, c_captionSettingsKey, c_captionSettingNames[i], REG_DWORD, &original[i], sizeof(DWORD)) :
                        RegDeleteKeyValueW(HKEY_CURRENT_USER, c_captionSettingsKey, c_captionSettingNames[i]);
                    if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
                    {
                        WEX::Logging::Log::Error(WEX::Common::String().Format(L"Failed to restore caption setting %s (error %d)", c_captionSettingNames[i], status));
                        restored = false;
                    }
                }
                return restored;
            }

            // Restores the caption settings from a backup left by a run that was killed while it had changed them.
            void RestoreCaptionSettingsLeftByKilledRun()
            {
                DWORD existedMask = 0;
                DWORD size = sizeof(existedMask);
                const LSTATUS maskStatus = RegGetValueW(HKEY_CURRENT_USER, c_captionSettingsBackupKey, c_captionSettingsBackupExistedMask, RRF_RT_REG_DWORD, nullptr, &existedMask, &size);
                if (maskStatus == ERROR_FILE_NOT_FOUND)
                {
                    // No backup, or an incomplete one written before any setting was changed.
                    RegDeleteTreeW(HKEY_CURRENT_USER, c_captionSettingsBackupKey);
                    return;
                }
                VERIFY_ARE_EQUAL(ERROR_SUCCESS, maskStatus, L"Read the caption settings backup");

                WEX::Logging::Log::Warning(L"Restoring the Windows caption settings changed by a previous test run that did not finish");
                DWORD original[ARRAYSIZE(c_captionSettingNames)] = {};
                for (size_t i = 0; i < ARRAYSIZE(c_captionSettingNames); i++)
                {
                    if (existedMask & (1u << i))
                    {
                        size = sizeof(DWORD);
                        VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegGetValueW(HKEY_CURRENT_USER, c_captionSettingsBackupKey, c_captionSettingNames[i], RRF_RT_REG_DWORD, nullptr, &original[i], &size),
                            L"Read a backed up caption setting");
                    }
                }
                VERIFY_IS_TRUE(RestoreCaptionSettings(original, existedMask), L"Restore the caption settings from the backup");
                VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegDeleteTreeW(HKEY_CURRENT_USER, c_captionSettingsBackupKey), L"Delete the caption settings backup");
            }

            // Overrides the per-user caption settings and restores the original registry values (deleting values that did
            // not exist) when it goes out of scope, including when the test fails. The originals are also backed up to
            // c_captionSettingsBackupKey before anything is changed, and the backup is only removed after a successful restore.
            class CaptionSettingsScope
            {
            public:
                CaptionSettingsScope()
                {
                    RestoreCaptionSettingsLeftByKilledRun();

                    for (size_t i = 0; i < ARRAYSIZE(c_captionSettingNames); i++)
                    {
                        DWORD value = 0;
                        DWORD size = sizeof(value);
                        const LSTATUS status = RegGetValueW(HKEY_CURRENT_USER, c_captionSettingsKey, c_captionSettingNames[i], RRF_RT_REG_DWORD, nullptr, &value, &size);
                        // Only a missing key or value means "not set". Any other error (for example a value that is not a
                        // DWORD, or access denied) stops the test here, before anything is written, so nothing is lost.
                        VERIFY_IS_TRUE(status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND,
                            WEX::Common::String().Format(L"Snapshot caption setting %s (error %d)", c_captionSettingNames[i], status));
                        if (status == ERROR_SUCCESS)
                        {
                            m_existedMask |= (1u << i);
                            m_original[i] = value;
                        }
                    }

                    HKEY backupKey = nullptr;
                    VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegCreateKeyExW(HKEY_CURRENT_USER, c_captionSettingsBackupKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &backupKey, nullptr));
                    auto closeBackupKey = wil::scope_exit([backupKey]() { RegCloseKey(backupKey); });
                    for (size_t i = 0; i < ARRAYSIZE(c_captionSettingNames); i++)
                    {
                        if (m_existedMask & (1u << i))
                        {
                            VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegSetValueExW(backupKey, c_captionSettingNames[i], 0, REG_DWORD, reinterpret_cast<const BYTE*>(&m_original[i]), sizeof(DWORD)));
                        }
                    }
                    VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegSetValueExW(backupKey, c_captionSettingsBackupExistedMask, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&m_existedMask), sizeof(DWORD)));
                }

                ~CaptionSettingsScope()
                {
                    if (RestoreCaptionSettings(m_original, m_existedMask))
                    {
                        RegDeleteTreeW(HKEY_CURRENT_USER, c_captionSettingsBackupKey);
                    }
                    else
                    {
                        // Keep the backup: the next CaptionSettingsScope or MediaCCTestsMPE::ClassSetup restores from it.
                        WEX::Logging::Log::Error(L"The caption settings were not fully restored; the backup is kept for the next run");
                    }
                }

                CaptionSettingsScope(const CaptionSettingsScope&) = delete;
                CaptionSettingsScope& operator=(const CaptionSettingsScope&) = delete;

                // Writes the settings and verifies, as a precondition, that the public ClosedCaptionProperties API reports them.
                void Apply(const CaptionSettings& settings)
                {
                    const DWORD values[] =
                    {
                        static_cast<DWORD>(settings.fontColor),
                        static_cast<DWORD>(settings.fontOpacity),
                        static_cast<DWORD>(settings.fontSize),
                        static_cast<DWORD>(settings.fontStyle),
                        static_cast<DWORD>(settings.edgeEffect),
                        static_cast<DWORD>(settings.backgroundColor),
                        static_cast<DWORD>(settings.backgroundOpacity),
                        static_cast<DWORD>(settings.regionColor),
                        static_cast<DWORD>(settings.regionOpacity),
                    };
                    static_assert(ARRAYSIZE(values) == ARRAYSIZE(c_captionSettingNames), "One value per caption setting");

                    HKEY key = nullptr;
                    VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegCreateKeyExW(HKEY_CURRENT_USER, c_captionSettingsKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr));
                    auto closeKey = wil::scope_exit([key]() { RegCloseKey(key); });
                    for (size_t i = 0; i < ARRAYSIZE(values); i++)
                    {
                        VERIFY_ARE_EQUAL(ERROR_SUCCESS, RegSetValueExW(key, c_captionSettingNames[i], 0, REG_DWORD, reinterpret_cast<const BYTE*>(&values[i]), sizeof(DWORD)));
                    }

                    const wchar_t* precondition = L"Precondition: ClosedCaptionProperties reports the caption setting written to HKCU";
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.fontColor), static_cast<int>(wmcc::ClosedCaptionProperties::FontColor), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.fontOpacity), static_cast<int>(wmcc::ClosedCaptionProperties::FontOpacity), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.fontSize), static_cast<int>(wmcc::ClosedCaptionProperties::FontSize), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.fontStyle), static_cast<int>(wmcc::ClosedCaptionProperties::FontStyle), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.edgeEffect), static_cast<int>(wmcc::ClosedCaptionProperties::FontEffect), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.backgroundColor), static_cast<int>(wmcc::ClosedCaptionProperties::BackgroundColor), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.backgroundOpacity), static_cast<int>(wmcc::ClosedCaptionProperties::BackgroundOpacity), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.regionColor), static_cast<int>(wmcc::ClosedCaptionProperties::RegionColor), precondition);
                    VERIFY_ARE_EQUAL(static_cast<int>(settings.regionOpacity), static_cast<int>(wmcc::ClosedCaptionProperties::RegionOpacity), precondition);
                }

            private:
                DWORD m_existedMask = 0;
                DWORD m_original[ARRAYSIZE(c_captionSettingNames)] = {};
            };

            wf::TimeSpan Milliseconds(INT64 ms)
            {
                wf::TimeSpan value;
                value.Duration = ms * c_hnsPerMs;
                return value;
            }

            TimedTextDouble Pixels(double value)
            {
                TimedTextDouble result;
                result.Value = value;
                result.Unit = TimedTextUnit::Pixels;
                return result;
            }

            ::Windows::UI::Color Argb(unsigned char a, unsigned char r, unsigned char g, unsigned char b)
            {
                ::Windows::UI::Color color;
                color.A = a;
                color.R = r;
                color.G = g;
                color.B = b;
                return color;
            }

            TimedTextStyle^ CreateStyle(Platform::String^ fontFamily, double fontSizeInPixels)
            {
                auto style = ref new TimedTextStyle();
                style->FontFamily = fontFamily;
                style->FontSize = Pixels(fontSizeInPixels);
                return style;
            }

            TimedTextCue^ CreateTextCue(INT64 startMs, INT64 durationMs, Platform::String^ text, TimedTextStyle^ style, TimedTextRegion^ region)
            {
                auto cue = ref new TimedTextCue();
                cue->StartTime = Milliseconds(startMs);
                cue->Duration = Milliseconds(durationMs);
                auto line = ref new TimedTextLine();
                line->Text = text;
                cue->Lines->Append(line);
                if (style != nullptr)
                {
                    cue->CueStyle = style;
                }
                if (region != nullptr)
                {
                    cue->CueRegion = region;
                }
                return cue;
            }

            TimedMetadataTrack^ CreateCaptionTrack(std::initializer_list<IMediaCue^> cues)
            {
                auto track = ref new TimedMetadataTrack(L"CueStylerCaptions", L"en", TimedMetadataKind::Caption);
                for (auto cue : cues)
                {
                    track->AddCue(cue);
                }
                return track;
            }

            // Opens blueframe_video.mp4 (paused, no transport controls), adds the tracks as external timed metadata tracks
            // and switches each one to PlatformPresented as soon as the playback item reports it.
            void SetupCaptionTracks(MediaHelperMPE& mediaHelper, Platform::String^ resourcePath, const std::vector<TimedMetadataTrack^>& tracks)
            {
                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 500));
                MediaHelperMPE::SetupBasicMediaPlayerElement(mediaHelper, resourcePath, L"blueframe_video.mp4", false /*useMTC*/, true /*waitForOpened*/, true /*useNewPlayback*/);

                const unsigned int expectedCount = static_cast<unsigned int>(tracks.size());
                auto presentedCount = std::make_shared<std::atomic<unsigned int>>(0);
                auto allPresented = std::make_shared<Event>();
                auto tracksChangedRegistration = CreateSafeEventRegistration(MediaPlaybackItem, TimedMetadataTracksChanged);

                RunOnUIThread([&]()
                {
                    auto item = MediaHelperMPE::s_CurrentPlaybackItem;
                    VERIFY_IS_NOT_NULL(item);
                    tracksChangedRegistration.Attach(
                        item,
                        ref new wf::TypedEventHandler<MediaPlaybackItem^, wfc::IVectorChangedEventArgs^>(
                            [presentedCount, allPresented, expectedCount](MediaPlaybackItem^ sender, wfc::IVectorChangedEventArgs^ args)
                    {
                        if (args->CollectionChange == wfc::CollectionChange::ItemInserted)
                        {
                            sender->TimedMetadataTracks->SetPresentationMode(args->Index, TimedMetadataTrackPresentationMode::PlatformPresented);
                            if (++(*presentedCount) == expectedCount)
                            {
                                allPresented->Set();
                            }
                        }
                    }));

                    for (auto track : tracks)
                    {
                        item->Source->ExternalTimedMetadataTracks->Append(track);
                    }
                });

                allPresented->WaitFor(std::chrono::milliseconds(10000));
                TestServices::WindowHelper->WaitForIdle();
            }

            void SeekAndWait(xaml_controls::MediaPlayerElement^ mpe, INT64 positionMs)
            {
                auto seekCompleted = std::make_shared<Event>();
                auto seekRegistration = CreateSafeEventRegistration(MediaPlaybackSession, SeekCompleted);

                RunOnUIThread([&]()
                {
                    auto session = mpe->MediaPlayer->PlaybackSession;
                    seekRegistration.Attach(
                        session,
                        ref new wf::TypedEventHandler<MediaPlaybackSession^, Platform::Object^>([seekCompleted](MediaPlaybackSession^, Platform::Object^)
                    {
                        seekCompleted->Set();
                    }));
                    session->Position = Milliseconds(positionMs);
                });

                seekCompleted->WaitFor(std::chrono::milliseconds(10000));
            }

            // Seeks into an active cue and waits until CCueRenderer::AddCue has finished building it.
            void SeekAndWaitForCueAdded(xaml_controls::MediaPlayerElement^ mpe, INT64 positionMs)
            {
                ETWWaiterProxy cueAddedWaiter;
                cueAddedWaiter.Start(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, TimedTextCueEnd_value);
                SeekAndWait(mpe, positionMs);
                cueAddedWaiter.WaitFor(10000);
                TestServices::WindowHelper->WaitForIdle();
            }

            // Seeks out of the active cue and waits until CCueRenderer::RemoveCue has run.
            void SeekAndWaitForCueRemoved(xaml_controls::MediaPlayerElement^ mpe, INT64 positionMs)
            {
                ETWWaiterProxy cueRemovedWaiter;
                cueRemovedWaiter.Start(WINDOWS_UI_XAML_DIAG_ETW_PROVIDER, CueRemoved_value);
                SeekAndWait(mpe, positionMs);
                cueRemovedWaiter.WaitFor(10000);
                TestServices::WindowHelper->WaitForIdle();
            }

            Platform::String^ GetInlineText(xaml_controls::TextBlock^ textBlock)
            {
                std::wstring text;
                for (unsigned int i = 0; i < textBlock->Inlines->Size; i++)
                {
                    auto run = dynamic_cast<xaml_docs::Run^>(textBlock->Inlines->GetAt(i));
                    if (run != nullptr)
                    {
                        text += run->Text->Data();
                    }
                }
                return ref new Platform::String(text.c_str());
            }

            // Elements built by CCueRenderer for one caption line:
            // cue container Grid -> region Border -> region StackPanel -> CueElement Grid -> TopWrapper -> MiddleWrapper
            // -> BottomWrapper -> TextContainer Grid -> { outline TextBlocks..., UnderTextBlock Canvas, caption TextBlock }.
            struct RenderedCaption
            {
                xaml_controls::TextBlock^ captionText = nullptr;
                xaml_controls::Grid^ textContainer = nullptr;
                xaml_controls::Canvas^ underTextBlock = nullptr;
                xaml_controls::Grid^ topWrapper = nullptr;
                xaml_controls::Grid^ cueElement = nullptr;
                xaml_controls::StackPanel^ regionPanel = nullptr;
                xaml_controls::Border^ regionBorder = nullptr;
                xaml_controls::Grid^ cueContainer = nullptr;
                std::vector<xaml_controls::TextBlock^> outlines;
            };

            // Must be called on the UI thread.
            RenderedCaption FindRenderedCaption(xaml_controls::MediaPlayerElement^ mpe, Platform::String^ lineText)
            {
                RenderedCaption result;

                auto textBlocks = ref new Platform::Collections::Vector<xaml_controls::TextBlock^>();
                TreeHelper::GetVisualChildrenByType<xaml_controls::TextBlock>(mpe, textBlocks);
                for (unsigned int i = 0; i < textBlocks->Size; i++)
                {
                    xaml_controls::TextBlock^ textBlock = textBlocks->GetAt(i);
                    if (dynamic_cast<xaml_media::TranslateTransform^>(textBlock->RenderTransform) == nullptr &&
                        Platform::String::CompareOrdinal(GetInlineText(textBlock), lineText) == 0)
                    {
                        VERIFY_IS_NULL(result.captionText, L"Only one caption TextBlock renders the line");
                        result.captionText = textBlock;
                    }
                }
                VERIFY_IS_NOT_NULL(result.captionText, WEX::Common::String().Format(L"Caption TextBlock for '%s' is in the tree", lineText->Data()));

                result.textContainer = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(result.captionText));
                for (unsigned int i = 0; i < result.textContainer->Children->Size; i++)
                {
                    auto child = result.textContainer->Children->GetAt(i);
                    if (auto canvas = dynamic_cast<xaml_controls::Canvas^>(child))
                    {
                        result.underTextBlock = canvas;
                    }
                    else if (auto outline = dynamic_cast<xaml_controls::TextBlock^>(child))
                    {
                        if (dynamic_cast<xaml_media::TranslateTransform^>(outline->RenderTransform) != nullptr)
                        {
                            result.outlines.push_back(outline);
                        }
                    }
                }
                VERIFY_IS_NOT_NULL(result.underTextBlock);

                auto bottomWrapper = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(result.textContainer));
                auto middleWrapper = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(bottomWrapper));
                result.topWrapper = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(middleWrapper));
                result.cueElement = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(result.topWrapper));
                result.regionPanel = safe_cast<xaml_controls::StackPanel^>(xaml_media::VisualTreeHelper::GetParent(result.cueElement));
                result.regionBorder = safe_cast<xaml_controls::Border^>(xaml_media::VisualTreeHelper::GetParent(result.regionPanel));
                result.cueContainer = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(result.regionBorder));
                return result;
            }

            xaml_docs::Run^ GetRun(xaml_controls::TextBlock^ textBlock, unsigned int index)
            {
                return safe_cast<xaml_docs::Run^>(textBlock->Inlines->GetAt(index));
            }

            // CueStyler scales style font sizes by (displayed video height / natural video height); the cue container is
            // sized to the displayed video area.
            double GetFontScale(const RenderedCaption& caption)
            {
                return caption.cueContainer->Height / c_blueframeNaturalHeight;
            }

            void VerifyClose(double expected, double actual, const wchar_t* what)
            {
                VERIFY_IS_TRUE(std::abs(expected - actual) <= c_tolerance,
                    WEX::Common::String().Format(L"%s: expected %f, actual %f", what, expected, actual));
            }

            void VerifyThickness(xaml::Thickness expected, xaml::Thickness actual, const wchar_t* what)
            {
                VERIFY_IS_TRUE(
                    std::abs(expected.Left - actual.Left) <= c_tolerance &&
                    std::abs(expected.Top - actual.Top) <= c_tolerance &&
                    std::abs(expected.Right - actual.Right) <= c_tolerance &&
                    std::abs(expected.Bottom - actual.Bottom) <= c_tolerance,
                    WEX::Common::String().Format(L"%s: expected {%f,%f,%f,%f}, actual {%f,%f,%f,%f}", what,
                        expected.Left, expected.Top, expected.Right, expected.Bottom,
                        actual.Left, actual.Top, actual.Right, actual.Bottom));
            }

            xaml::Thickness MakeThickness(double left, double top, double right, double bottom)
            {
                xaml::Thickness thickness;
                thickness.Left = left;
                thickness.Top = top;
                thickness.Right = right;
                thickness.Bottom = bottom;
                return thickness;
            }

            void VerifyBrushColor(::Windows::UI::Color expected, xaml_media::Brush^ brush, const wchar_t* what)
            {
                auto solidBrush = dynamic_cast<xaml_media::SolidColorBrush^>(brush);
                VERIFY_IS_NOT_NULL(solidBrush, what);
                const auto actual = solidBrush->Color;
                VERIFY_IS_TRUE(actual.A == expected.A && actual.R == expected.R && actual.G == expected.G && actual.B == expected.B,
                    WEX::Common::String().Format(L"%s: expected #%02X%02X%02X%02X, actual #%02X%02X%02X%02X", what,
                        expected.A, expected.R, expected.G, expected.B, actual.A, actual.R, actual.G, actual.B));
            }

            void VerifyText(const wchar_t* expected, Platform::String^ actual, const wchar_t* what)
            {
                VERIFY_IS_TRUE(actual != nullptr && wcscmp(expected, actual->Data()) == 0,
                    WEX::Common::String().Format(L"%s: expected '%s', actual '%s'", what, expected, actual == nullptr ? L"<null>" : actual->Data()));
            }

            void VerifyFontFamily(const wchar_t* expected, xaml_docs::Run^ run)
            {
                VerifyText(expected, run->FontFamily->Source, WEX::Common::String().Format(L"FontFamily of run '%s'", run->Text->Data()));
            }

            // Offsets of the SpriteVisual copies that the edge effect hands in under the caption TextBlock, sorted.
            std::vector<std::pair<float, float>> GetEdgeEffectCopyOffsets(const RenderedCaption& caption)
            {
                std::vector<std::pair<float, float>> offsets;
                auto handIn = xaml_hosting::ElementCompositionPreview::GetElementChildVisual(caption.underTextBlock);
                if (handIn != nullptr)
                {
                    auto container = dynamic_cast<Microsoft::UI::Composition::ContainerVisual^>(handIn);
                    VERIFY_IS_NOT_NULL(container, L"Edge effect copies are hosted in a ContainerVisual");
                    for (auto copy : container->Children)
                    {
                        VerifyClose(caption.captionText->ActualSize.x, copy->Size.x, L"Edge effect copy width matches the caption TextBlock");
                        VerifyClose(caption.captionText->ActualSize.y, copy->Size.y, L"Edge effect copy height matches the caption TextBlock");
                        VerifyClose(0.0, copy->Offset.z, L"Edge effect copy z offset");
                        offsets.emplace_back(copy->Offset.x, copy->Offset.y);
                    }
                }
                std::sort(offsets.begin(), offsets.end());
                return offsets;
            }

            void VerifyEdgeEffectCopies(std::vector<std::pair<float, float>> expected, const std::vector<std::pair<float, float>>& actual, const wchar_t* what)
            {
                std::sort(expected.begin(), expected.end());
                VERIFY_ARE_EQUAL(expected.size(), actual.size(), WEX::Common::String().Format(L"%s: number of offset copies", what));
                for (size_t i = 0; i < expected.size(); i++)
                {
                    VERIFY_IS_TRUE(expected[i] == actual[i],
                        WEX::Common::String().Format(L"%s: copy %zu expected offset (%f,%f), actual (%f,%f)", what, i,
                            expected[i].first, expected[i].second, actual[i].first, actual[i].second));
                }
            }

            // Each edge effect copy is a SpriteVisual whose brush is a CompositionMaskBrush (text alpha mask) over a
            // CompositionColorBrush; verifies that color (0xAARRGGBB) for every copy, chosen by the copy's offset.
            void VerifyEdgeEffectCopyColors(const RenderedCaption& caption, std::function<unsigned int(float, float)> expectedColor, const wchar_t* what)
            {
                auto container = dynamic_cast<Microsoft::UI::Composition::ContainerVisual^>(
                    xaml_hosting::ElementCompositionPreview::GetElementChildVisual(caption.underTextBlock));
                VERIFY_IS_NOT_NULL(container, L"Edge effect copies are hosted in a ContainerVisual");
                unsigned int count = 0;
                for (auto copy : container->Children)
                {
                    auto sprite = dynamic_cast<Microsoft::UI::Composition::SpriteVisual^>(copy);
                    VERIFY_IS_NOT_NULL(sprite, L"Edge effect copies are SpriteVisuals");
                    auto mask = dynamic_cast<Microsoft::UI::Composition::CompositionMaskBrush^>(sprite->Brush);
                    VERIFY_IS_NOT_NULL(mask, L"Edge effect copies are painted with a mask brush");
                    VERIFY_IS_NOT_NULL(mask->Mask, L"The mask brush uses the caption text as its mask");
                    auto colorBrush = dynamic_cast<Microsoft::UI::Composition::CompositionColorBrush^>(mask->Source);
                    VERIFY_IS_NOT_NULL(colorBrush, L"The mask brush source is a color brush");
                    const auto color = colorBrush->Color;
                    const unsigned int actual = (static_cast<unsigned int>(color.A) << 24) | (static_cast<unsigned int>(color.R) << 16) | (static_cast<unsigned int>(color.G) << 8) | color.B;
                    const unsigned int expected = expectedColor(copy->Offset.x, copy->Offset.y);
                    VERIFY_ARE_EQUAL(expected, actual, WEX::Common::String().Format(L"%s: color of the copy at (%f,%f): expected 0x%08X, actual 0x%08X",
                        what, copy->Offset.x, copy->Offset.y, expected, actual));
                    count++;
                }
                VERIFY_IS_TRUE(count > 0, WEX::Common::String().Format(L"%s: the edge effect has copies", what));
            }

            double ExpectedTextMargin(wmcc::ClosedCaptionSize size)
            {
                switch (size)
                {
                case wmcc::ClosedCaptionSize::FiftyPercent: return 4;
                case wmcc::ClosedCaptionSize::OneHundredFiftyPercent: return 8;
                case wmcc::ClosedCaptionSize::TwoHundredPercent: return 12;
                default: return 6;
                }
            }

            const wmcc::ClosedCaptionSize c_allCaptionSizes[] =
            {
                wmcc::ClosedCaptionSize::Default,
                wmcc::ClosedCaptionSize::FiftyPercent,
                wmcc::ClosedCaptionSize::OneHundredPercent,
                wmcc::ClosedCaptionSize::OneHundredFiftyPercent,
                wmcc::ClosedCaptionSize::TwoHundredPercent,
            };
        }

        //------------------------------------------------------------------------
        // Test case: a caption cue with neither CueStyle nor CueRegion uses the default presentation
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::CaptionCueWithoutStyleOrRegionUsesDefaultPresentation()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            auto cue = CreateTextCue(1000, 3000, L"Unstyled caption", nullptr, nullptr);
            VERIFY_IS_NULL(cue->CueStyle, L"Precondition: a new TimedTextCue has no style");
            VERIFY_IS_NULL(cue->CueRegion, L"Precondition: a new TimedTextCue has no region");

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ cue }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, L"Unstyled caption");
                const double containerHeight = caption.cueContainer->Height;
                const double defaultPadding = containerHeight * 0.025; // Half a line of text: 2.5% of the caption area height.

                VERIFY_ARE_EQUAL(1u, caption.captionText->Inlines->Size);
                auto run = GetRun(caption.captionText, 0);
                VerifyText(L"Unstyled caption", run->Text, L"Run text");
                VerifyFontFamily(L"Segoe UI", run);
                VerifyClose(containerHeight * 0.05, run->FontSize, L"Default caption font size is 5% of the caption area height");
                VERIFY_ARE_EQUAL(static_cast<unsigned short>(400), run->FontWeight.Weight);
                VerifyBrushColor(Argb(0xFF, 0xFF, 0xFF, 0xFF), run->Foreground, L"Default caption foreground");
                VERIFY_ARE_EQUAL(xaml::FlowDirection::LeftToRight, run->FlowDirection);
                VERIFY_ARE_EQUAL(xaml::TextWrapping::Wrap, caption.captionText->TextWrapping);

                VerifyBrushColor(Argb(0, 0, 0, 0), caption.cueElement->Background, L"Default line background");
                VERIFY_ARE_EQUAL(xaml::HorizontalAlignment::Left, caption.cueElement->HorizontalAlignment);

                VERIFY_ARE_EQUAL(xaml::VerticalAlignment::Top, caption.regionPanel->VerticalAlignment, L"Default display alignment is Before (top)");
                VerifyBrushColor(Argb(0, 0, 0, 0), caption.regionBorder->Background, L"Default region background");
                VERIFY_IS_TRUE(std::isnan(caption.regionBorder->Width), L"No region extent: width is not set");
                VERIFY_IS_TRUE(std::isnan(caption.regionBorder->Height), L"No region extent: height is not set");
                VerifyThickness(MakeThickness(defaultPadding, defaultPadding, defaultPadding, defaultPadding), caption.regionBorder->Padding, L"Default region padding");
                VerifyThickness(MakeThickness(-defaultPadding, -defaultPadding, -defaultPadding, -defaultPadding), caption.regionBorder->Margin, L"Default region margin");

                VerifyThickness(MakeThickness(6, 6, 6, 6), caption.textContainer->Margin, L"Text margin for the default caption size");
                VERIFY_IS_NULL(xaml_hosting::ElementCompositionPreview::GetElementChildVisual(caption.underTextBlock), L"No edge effect copies with edge effect None");
            });
        }

        //------------------------------------------------------------------------
        // Test case: subformat styles map TTML generic font families, weights, font styles, decorations and flow direction
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::CaptionSubformatStylesMapGenericFontFamiliesWeightsAndDecorations()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            struct Segment
            {
                const wchar_t* text;
                const wchar_t* timedTextFontFamily;
                const wchar_t* expectedFontFamily;
            };
            const Segment segments[] =
            {
                { L"mono ", L"monospaceSerif", L"Courier New" },
                { L"georgia ", L"proportionalSerif", L"Georgia" },
                { L"lucida ", L"monospaceSansSerif", L"Lucida Console" },
                { L"segoe ", L"proportionalSansSerif", L"Segoe UI" },
                { L"comic ", L"casual", L"Comic Sans MS" },
                { L"ink ", L"cursive", L"Ink Free" },
                { L"caps ", L"smallCaps", L"Segoe UI" },
                { L"arial", L"Arial", L"Arial" },
            };

            std::wstring lineText;
            for (const auto& segment : segments)
            {
                lineText += segment.text;
            }

            auto cue = CreateTextCue(1000, 3000, ref new Platform::String(lineText.c_str()), CreateStyle(L"default", 20), nullptr);
            auto line = cue->Lines->GetAt(0);
            int start = 0;
            for (size_t i = 0; i < ARRAYSIZE(segments); i++)
            {
                auto style = CreateStyle(ref new Platform::String(segments[i].timedTextFontFamily), 20);
                switch (i)
                {
                case 0: style->FontWeight = TimedTextWeight::Bold; break;
                case 1: style->FontStyle = TimedTextFontStyle::Italic; break;
                case 2: style->FontStyle = TimedTextFontStyle::Oblique; break;
                case 3: style->IsUnderlineEnabled = true; break;
                case 4: style->IsLineThroughEnabled = true; break;
                case 5: style->IsUnderlineEnabled = true; style->IsLineThroughEnabled = true; break;
                case 7: style->FlowDirection = TimedTextFlowDirection::RightToLeft; break;
                }

                auto subformat = ref new TimedTextSubformat();
                subformat->StartIndex = start;
                subformat->Length = static_cast<int>(wcslen(segments[i].text));
                subformat->SubformatStyle = style;
                line->Subformats->Append(subformat);
                start += subformat->Length;
            }

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ cue }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, ref new Platform::String(lineText.c_str()));
                const double expectedFontSize = 20 * GetFontScale(caption);

                VERIFY_ARE_EQUAL(static_cast<unsigned int>(ARRAYSIZE(segments)), caption.captionText->Inlines->Size, L"One run per subformat");
                for (unsigned int i = 0; i < ARRAYSIZE(segments); i++)
                {
                    auto run = GetRun(caption.captionText, i);
                    VerifyText(segments[i].text, run->Text, L"Run text");
                    VerifyFontFamily(segments[i].expectedFontFamily, run);
                    VerifyClose(expectedFontSize, run->FontSize, L"Subformat font size");

                    const unsigned short expectedWeight = (i == 0) ? 700 : 400;
                    VERIFY_ARE_EQUAL(expectedWeight, run->FontWeight.Weight, L"Font weight");

                    const auto expectedFontStyle =
                        (i == 1) ? ::Windows::UI::Text::FontStyle::Italic :
                        (i == 2) ? ::Windows::UI::Text::FontStyle::Oblique :
                                   ::Windows::UI::Text::FontStyle::Normal;
                    VERIFY_ARE_EQUAL(static_cast<int>(expectedFontStyle), static_cast<int>(run->FontStyle), L"Font style");

                    unsigned int expectedDecorations = static_cast<unsigned int>(::Windows::UI::Text::TextDecorations::None);
                    if (i == 3 || i == 5)
                    {
                        expectedDecorations |= static_cast<unsigned int>(::Windows::UI::Text::TextDecorations::Underline);
                    }
                    if (i == 4 || i == 5)
                    {
                        expectedDecorations |= static_cast<unsigned int>(::Windows::UI::Text::TextDecorations::Strikethrough);
                    }
                    VERIFY_ARE_EQUAL(expectedDecorations, static_cast<unsigned int>(run->TextDecorations), L"Text decorations");

                    VERIFY_ARE_EQUAL((i == 7) ? xaml::FlowDirection::RightToLeft : xaml::FlowDirection::LeftToRight, run->FlowDirection, L"Flow direction");
                    VERIFY_ARE_EQUAL((i == 6) ? xaml::FontCapitals::SmallCaps : xaml::FontCapitals::Normal, xaml_docs::Typography::GetCapitals(run), L"Typography capitals");
                }
            });
        }

        //------------------------------------------------------------------------
        // Test case: region alignment, extent, position, padding units, z-index, line alignment and wrapping
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::CaptionRegionAlignmentExtentPositionAndPaddingUnits()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            // Region A: centered, pixel extent and position, percentage padding, wrapping.
            auto regionA = ref new TimedTextRegion();
            regionA->Name = L"regionA";
            regionA->DisplayAlignment = TimedTextDisplayAlignment::Center;
            TimedTextSize extentA = {};
            extentA.Width = 200;
            extentA.Height = 50;
            extentA.Unit = TimedTextUnit::Pixels;
            regionA->Extent = extentA;
            TimedTextPoint positionA = {};
            positionA.X = 10;
            positionA.Y = 20;
            positionA.Unit = TimedTextUnit::Pixels;
            regionA->Position = positionA;
            // The same padding on every edge: CueStyler maps Before/After to Left/Right and Start/End to Top/Bottom, while TTML
            // defines before/after as the block (top/bottom) edges and start/end as the inline (left/right) edges (PC-6). Equal
            // edges keep the expected thickness valid for both mappings; percentages still scale by width horizontally and by
            // height vertically.
            TimedTextPadding paddingA = {};
            paddingA.Before = 10;
            paddingA.After = 10;
            paddingA.Start = 10;
            paddingA.End = 10;
            paddingA.Unit = TimedTextUnit::Percentage;
            regionA->Padding = paddingA;
            regionA->TextWrapping = TimedTextWrapping::Wrap;
            regionA->Background = Argb(0x80, 0x00, 0x00, 0xFF);
            auto styleA = CreateStyle(L"default", 16);
            styleA->LineAlignment = TimedTextLineAlignment::Center;

            // Region B: bottom aligned, percentage extent and position, pixel padding, no wrapping, z-index.
            auto regionB = ref new TimedTextRegion();
            regionB->Name = L"regionB";
            regionB->DisplayAlignment = TimedTextDisplayAlignment::After;
            TimedTextSize extentB = {};
            extentB.Width = 50;
            extentB.Height = 20;
            extentB.Unit = TimedTextUnit::Percentage;
            regionB->Extent = extentB;
            TimedTextPoint positionB = {};
            positionB.X = 10;
            positionB.Y = 60;
            positionB.Unit = TimedTextUnit::Percentage;
            regionB->Position = positionB;
            TimedTextPadding paddingB = {};
            paddingB.Before = 6;
            paddingB.After = 6;
            paddingB.Start = 6;
            paddingB.End = 6;
            paddingB.Unit = TimedTextUnit::Pixels;
            regionB->Padding = paddingB;
            regionB->TextWrapping = TimedTextWrapping::NoWrap;
            regionB->ZIndex = 3;
            auto styleB = CreateStyle(L"default", 16);
            styleB->LineAlignment = TimedTextLineAlignment::End;

            // An image cue without an extent.
            auto imageCue = ref new ImageCue();
            imageCue->StartTime = Milliseconds(5500);
            imageCue->Duration = Milliseconds(1000);
            TimedTextPoint imagePosition = {};
            imagePosition.X = 30;
            imagePosition.Y = 40;
            imagePosition.Unit = TimedTextUnit::Pixels;
            imageCue->Position = imagePosition;
            imageCue->SoftwareBitmap = ref new ::Windows::Graphics::Imaging::SoftwareBitmap(
                ::Windows::Graphics::Imaging::BitmapPixelFormat::Bgra8, 16, 16, ::Windows::Graphics::Imaging::BitmapAlphaMode::Premultiplied);
            auto imageTrack = ref new TimedMetadataTrack(L"CueStylerImages", L"", TimedMetadataKind::ImageSubtitle);
            imageTrack->AddCue(imageCue);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), {
                CreateCaptionTrack({ CreateTextCue(500, 2000, L"Region A", styleA, regionA), CreateTextCue(3000, 2000, L"Region B", styleB, regionB) }),
                imageTrack });

            LOG_OUTPUT(L"Region A: Center alignment, pixel extent/position, percentage padding.");
            SeekAndWaitForCueAdded(mpe, 1500);
            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, L"Region A");
                const double width = caption.cueContainer->Width;
                const double height = caption.cueContainer->Height;

                VERIFY_ARE_EQUAL(xaml::VerticalAlignment::Center, caption.regionPanel->VerticalAlignment);
                VerifyClose(200, caption.regionBorder->Width, L"Pixel extent width");
                VerifyClose(50, caption.regionBorder->Height, L"Pixel extent height (center aligned keeps the extent height)");
                VerifyThickness(MakeThickness(10, 20, 0, 0), caption.regionBorder->Margin, L"Pixel position with an extent");
                VerifyThickness(MakeThickness(width * 0.10, height * 0.10, width * 0.10, height * 0.10), caption.regionBorder->Padding, L"Percentage padding");
                VerifyBrushColor(Argb(0x80, 0x00, 0x00, 0xFF), caption.regionBorder->Background, L"Region background from the cue region");
                VERIFY_ARE_EQUAL(0, xaml_controls::Canvas::GetZIndex(caption.regionBorder));
                VERIFY_ARE_EQUAL(xaml::HorizontalAlignment::Center, caption.cueElement->HorizontalAlignment, L"Line alignment Center");
                VERIFY_ARE_EQUAL(xaml::TextWrapping::Wrap, caption.captionText->TextWrapping);
            });

            LOG_OUTPUT(L"Region B: After alignment, percentage extent/position, pixel padding.");
            SeekAndWaitForCueAdded(mpe, 4000);
            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, L"Region B");
                const double width = caption.cueContainer->Width;
                const double height = caption.cueContainer->Height;

                VERIFY_ARE_EQUAL(xaml::VerticalAlignment::Bottom, caption.regionPanel->VerticalAlignment);
                VerifyClose(width * 0.5, caption.regionBorder->Width, L"Percentage extent width");
                // Bottom aligned captions span from the top of the area to the bottom of the extent: 60% position + (20% + 5% line padding) extent.
                VerifyClose(height * 0.85, caption.regionBorder->Height, L"Bottom aligned region height");
                VerifyThickness(MakeThickness(width * 0.10, 0, 0, 0), caption.regionBorder->Margin, L"Percentage position, top margin folded into the height");
                VerifyThickness(MakeThickness(6, 6, 6, 6), caption.regionBorder->Padding, L"Pixel padding");
                VERIFY_ARE_EQUAL(3, xaml_controls::Canvas::GetZIndex(caption.regionBorder));
                VERIFY_ARE_EQUAL(xaml::HorizontalAlignment::Right, caption.cueElement->HorizontalAlignment, L"Line alignment End");
                VERIFY_ARE_EQUAL(xaml::TextWrapping::NoWrap, caption.captionText->TextWrapping);
            });

            LOG_OUTPUT(L"Image cue without extent.");
            SeekAndWaitForCueAdded(mpe, 6000);
            RunOnUIThread([&]()
            {
                auto images = ref new Platform::Collections::Vector<xaml_controls::Image^>();
                TreeHelper::GetVisualChildrenByType<xaml_controls::Image>(mpe, images);
                xaml_controls::Image^ cueImage = nullptr;
                for (unsigned int i = 0; i < images->Size; i++)
                {
                    xaml_controls::Image^ image = images->GetAt(i);
                    if (dynamic_cast<xaml_imaging::SoftwareBitmapSource^>(image->Source) != nullptr)
                    {
                        cueImage = image;
                    }
                }
                VERIFY_IS_NOT_NULL(cueImage, L"Image cue is rendered");

                auto regionBorder = TreeHelper::FindAncestor<xaml_controls::Border^>(cueImage);
                VERIFY_IS_NOT_NULL(regionBorder);
                auto cueContainer = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetParent(regionBorder));
                const double padding = cueContainer->Height * 0.025;
                VERIFY_IS_TRUE(std::isnan(regionBorder->Width), L"No image extent: width is not set");
                VerifyThickness(MakeThickness(30 - padding, 40 - padding, -padding, -padding), regionBorder->Margin, L"Image region margin includes the default padding");
            });
        }

        //------------------------------------------------------------------------
        // Test case: text outside subformats uses the cue style and a percentage outline is scaled by the font size
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::CaptionSubformatGapsAndPercentOutlineThickness()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            auto cueStyle = CreateStyle(L"default", 20);
            cueStyle->Foreground = Argb(0xFF, 0xFF, 0xFF, 0x00);
            cueStyle->OutlineColor = Argb(0xFF, 0xFF, 0x00, 0x00);
            TimedTextDouble outlineThickness = {};
            outlineThickness.Value = 10;
            outlineThickness.Unit = TimedTextUnit::Percentage;
            cueStyle->OutlineThickness = outlineThickness;

            auto subformatStyle = CreateStyle(L"casual", 20);
            subformatStyle->Foreground = Argb(0xFF, 0x00, 0xFF, 0xFF);
            subformatStyle->OutlineColor = Argb(0xFF, 0x00, 0x00, 0xFF);

            auto adjacentStyle = CreateStyle(L"default", 30);
            adjacentStyle->Foreground = Argb(0xFF, 0x00, 0xFF, 0x00);
            adjacentStyle->OutlineColor = Argb(0xFF, 0x00, 0x80, 0x00);

            // "lead-" (no subformat) "mid-" (styled subformat after a gap) "ta" (adjacent styled subformat) "il" (after the last subformat).
            auto cue = CreateTextCue(1000, 3000, L"lead-mid-tail", cueStyle, nullptr);
            auto line = cue->Lines->GetAt(0);
            auto styledSubformat = ref new TimedTextSubformat();
            styledSubformat->StartIndex = 5;
            styledSubformat->Length = 4;
            styledSubformat->SubformatStyle = subformatStyle;
            line->Subformats->Append(styledSubformat);
            auto adjacentSubformat = ref new TimedTextSubformat();
            adjacentSubformat->StartIndex = 9;
            adjacentSubformat->Length = 2;
            adjacentSubformat->SubformatStyle = adjacentStyle;
            line->Subformats->Append(adjacentSubformat);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ cue }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, L"lead-mid-tail");
                const double scale = GetFontScale(caption);

                VERIFY_ARE_EQUAL(4u, caption.captionText->Inlines->Size, L"Gap run, styled run, adjacent styled run and trailing run");
                const wchar_t* expectedTexts[] = { L"lead-", L"mid-", L"ta", L"il" };
                for (unsigned int i = 0; i < ARRAYSIZE(expectedTexts); i++)
                {
                    VerifyText(expectedTexts[i], GetRun(caption.captionText, i)->Text, L"Run text");
                }

                auto gapRun = GetRun(caption.captionText, 0);
                VerifyFontFamily(L"Segoe UI", gapRun);
                VerifyClose(20 * scale, gapRun->FontSize, L"Text before the first subformat uses the cue style font size");
                VerifyBrushColor(Argb(0xFF, 0xFF, 0xFF, 0x00), gapRun->Foreground, L"Text before the first subformat uses the cue style foreground");

                auto styledRun = GetRun(caption.captionText, 1);
                VerifyFontFamily(L"Comic Sans MS", styledRun);
                VerifyBrushColor(Argb(0xFF, 0x00, 0xFF, 0xFF), styledRun->Foreground, L"Styled subformat foreground");

                auto adjacentRun = GetRun(caption.captionText, 2);
                VerifyFontFamily(L"Segoe UI", adjacentRun);
                VerifyClose(30 * scale, adjacentRun->FontSize, L"A subformat that starts where the previous one ends uses its own font size");
                VerifyBrushColor(Argb(0xFF, 0x00, 0xFF, 0x00), adjacentRun->Foreground, L"A subformat that starts where the previous one ends uses its own foreground");

                // Eight outline copies offset by 10% of the cue style font size in every direction.
                const double offset = 0.10 * 20 * scale;
                VERIFY_ARE_EQUAL(static_cast<size_t>(8), caption.outlines.size(), L"Eight outline TextBlocks");
                std::vector<std::pair<int, int>> directions;
                for (auto outline : caption.outlines)
                {
                    auto transform = safe_cast<xaml_media::TranslateTransform^>(outline->RenderTransform);
                    VERIFY_IS_TRUE(std::abs(std::abs(transform->X) - offset) <= c_tolerance || std::abs(transform->X) <= c_tolerance,
                        WEX::Common::String().Format(L"Outline X offset %f is 0 or +/-%f", transform->X, offset));
                    VERIFY_IS_TRUE(std::abs(std::abs(transform->Y) - offset) <= c_tolerance || std::abs(transform->Y) <= c_tolerance,
                        WEX::Common::String().Format(L"Outline Y offset %f is 0 or +/-%f", transform->Y, offset));
                    directions.emplace_back(
                        std::abs(transform->X) <= c_tolerance ? 0 : (transform->X > 0 ? 1 : -1),
                        std::abs(transform->Y) <= c_tolerance ? 0 : (transform->Y > 0 ? 1 : -1));

                    VerifyBrushColor(Argb(0xFF, 0xFF, 0x00, 0x00), GetRun(outline, 0)->Foreground, L"Outline of the cue styled text uses the cue outline color");
                    VerifyBrushColor(Argb(0xFF, 0x00, 0x00, 0xFF), GetRun(outline, 1)->Foreground, L"Outline of the styled subformat uses its outline color");
                    VerifyBrushColor(Argb(0xFF, 0x00, 0x80, 0x00), GetRun(outline, 2)->Foreground, L"Outline of the adjacent subformat uses its outline color");
                }
                std::sort(directions.begin(), directions.end());
                const std::vector<std::pair<int, int>> expectedDirections = { {-1,-1}, {-1,0}, {-1,1}, {0,-1}, {0,1}, {1,-1}, {1,0}, {1,1} };
                VERIFY_IS_TRUE(directions == expectedDirections, L"One outline copy in each of the eight directions");
            });
        }

        //------------------------------------------------------------------------
        // Shared body for the user caption color tests: renders a styled cue under the given Windows caption settings.
        //------------------------------------------------------------------------
        namespace
        {
            TimedTextCue^ CreateColorStyledCue(Platform::String^ text)
            {
                auto style = CreateStyle(L"default", 20);
                style->Foreground = Argb(0xFF, 0x10, 0x20, 0x30);
                style->Background = Argb(0xFF, 0x40, 0x50, 0x60);
                auto region = ref new TimedTextRegion();
                region->Name = L"colorRegion";
                region->Background = Argb(0xFF, 0x70, 0x80, 0x90);
                return CreateTextCue(1000, 3000, text, style, region);
            }

            // Rendered caption colors packed as 0xAARRGGBB, and the opacity of the caption text layer.
            struct RenderedCaptionColors
            {
                unsigned int foreground = 0;
                unsigned int lineBackground = 0;
                unsigned int regionBackground = 0;
                float textOpacity = -1;
            };

            unsigned int PackBrushColor(xaml_media::Brush^ brush)
            {
                auto solidBrush = dynamic_cast<xaml_media::SolidColorBrush^>(brush);
                VERIFY_IS_NOT_NULL(solidBrush, L"Caption colors are SolidColorBrushes");
                const auto color = solidBrush->Color;
                return (static_cast<unsigned int>(color.A) << 24) | (static_cast<unsigned int>(color.R) << 16) | (static_cast<unsigned int>(color.G) << 8) | color.B;
            }

            RenderedCaptionColors ReadRenderedCaptionColors(xaml_controls::MediaPlayerElement^ mpe, Platform::String^ text)
            {
                RenderedCaptionColors colors;
                RunOnUIThread([&]()
                {
                    auto caption = FindRenderedCaption(mpe, text);
                    colors.foreground = PackBrushColor(GetRun(caption.captionText, 0)->Foreground);
                    colors.lineBackground = PackBrushColor(caption.cueElement->Background);
                    colors.regionBackground = PackBrushColor(caption.regionBorder->Background);
                    auto textLayer = xaml_hosting::ElementCompositionPreview::GetElementChildVisual(caption.topWrapper);
                    VERIFY_IS_NOT_NULL(textLayer, L"Caption text is rendered through a hand-in visual");
                    colors.textOpacity = textLayer->Opacity;
                });
                return colors;
            }
        }

        //------------------------------------------------------------------------
        // Test case: Windows caption color and opacity settings override the cue style (red text, blue line, yellow window)
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionColorsAndOpacityOverrideCueStyle()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            CaptionSettings settings;
            settings.fontColor = wmcc::ClosedCaptionColor::Red;
            settings.fontOpacity = wmcc::ClosedCaptionOpacity::SeventyFivePercent;
            settings.backgroundColor = wmcc::ClosedCaptionColor::Blue;
            settings.backgroundOpacity = wmcc::ClosedCaptionOpacity::TwentyFivePercent;
            settings.regionColor = wmcc::ClosedCaptionColor::Yellow;
            settings.regionOpacity = wmcc::ClosedCaptionOpacity::ZeroPercent;
            captionSettings.Apply(settings);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateColorStyledCue(L"User colors") }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            // The cue style asks for #FF102030 text, a #FF405060 line and a #FF708090 window; the user settings win.
            const auto colors = ReadRenderedCaptionColors(mpe, L"User colors");
            VERIFY_ARE_EQUAL(0xFFFF0000u, colors.foreground, L"Red caption text (opacity is applied to the text layer, not the brush)");
            VERIFY_ARE_EQUAL(0x400000FFu, colors.lineBackground, L"Blue line background at 25%");
            VERIFY_ARE_EQUAL(0x00FFFF00u, colors.regionBackground, L"Yellow window at 0%");
            VERIFY_ARE_EQUAL(0.75f, colors.textOpacity, L"Caption text layer at 75%");
        }

        //------------------------------------------------------------------------
        // Test case: the remaining caption colors and opacities (green/magenta/cyan, then white/black)
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionColorsAndOpacityOverrideCueStyleAlternatePalette()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            CaptionSettings settings;
            settings.fontColor = wmcc::ClosedCaptionColor::Green;
            settings.fontOpacity = wmcc::ClosedCaptionOpacity::TwentyFivePercent;
            settings.backgroundColor = wmcc::ClosedCaptionColor::Magenta;
            settings.backgroundOpacity = wmcc::ClosedCaptionOpacity::SeventyFivePercent;
            settings.regionColor = wmcc::ClosedCaptionColor::Cyan;
            settings.regionOpacity = wmcc::ClosedCaptionOpacity::OneHundredPercent;
            captionSettings.Apply(settings);

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateColorStyledCue(L"Alternate colors") }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            // Green maps to exactly #00FF00 (FCC requirement).
            auto colors = ReadRenderedCaptionColors(mpe, L"Alternate colors");
            VERIFY_ARE_EQUAL(0xFF00FF00u, colors.foreground, L"Green caption text");
            VERIFY_ARE_EQUAL(0xC0FF00FFu, colors.lineBackground, L"Magenta line background at 75%");
            VERIFY_ARE_EQUAL(0xFF00FFFFu, colors.regionBackground, L"Cyan window at 100%");
            VERIFY_ARE_EQUAL(0.25f, colors.textOpacity, L"Caption text layer at 25%");

            LOG_OUTPUT(L"White text at 100%%, black line with the default opacity, white window at 0%%.");
            SeekAndWaitForCueRemoved(mpe, 5500);
            settings.fontColor = wmcc::ClosedCaptionColor::White;
            settings.fontOpacity = wmcc::ClosedCaptionOpacity::OneHundredPercent;
            settings.backgroundColor = wmcc::ClosedCaptionColor::Black;
            settings.backgroundOpacity = wmcc::ClosedCaptionOpacity::Default;
            settings.regionColor = wmcc::ClosedCaptionColor::White;
            settings.regionOpacity = wmcc::ClosedCaptionOpacity::ZeroPercent;
            captionSettings.Apply(settings);
            SeekAndWaitForCueAdded(mpe, 2000);

            colors = ReadRenderedCaptionColors(mpe, L"Alternate colors");
            VERIFY_ARE_EQUAL(0xFFFFFFFFu, colors.foreground, L"White caption text");
            VERIFY_ARE_EQUAL(0xFF000000u, colors.lineBackground, L"Black line background with the default (opaque) opacity");
            VERIFY_ARE_EQUAL(0x00FFFFFFu, colors.regionBackground, L"White window at 0%");
            VERIFY_ARE_EQUAL(1.0f, colors.textOpacity, L"A 100% caption opacity leaves the text layer opaque");

            // Only one of the two window settings differs from Default: the user window setting still wins over the cue region color.
            LOG_OUTPUT(L"Window color White with the default window opacity; everything else default.");
            SeekAndWaitForCueRemoved(mpe, 5500);
            settings = CaptionSettings();
            settings.regionColor = wmcc::ClosedCaptionColor::White;
            captionSettings.Apply(settings);
            SeekAndWaitForCueAdded(mpe, 2000);
            colors = ReadRenderedCaptionColors(mpe, L"Alternate colors");
            VERIFY_ARE_EQUAL(0xFF102030u, colors.foreground, L"Default user text settings keep the cue style foreground");
            VERIFY_ARE_EQUAL(0xFF405060u, colors.lineBackground, L"Default user background settings keep the cue style background");
            VERIFY_ARE_EQUAL(0xFFFFFFFFu, colors.regionBackground, L"A user window color alone overrides the cue region color (opaque white)");

            LOG_OUTPUT(L"Window opacity 0 with the default window color; everything else default.");
            SeekAndWaitForCueRemoved(mpe, 5500);
            settings = CaptionSettings();
            settings.regionOpacity = wmcc::ClosedCaptionOpacity::ZeroPercent;
            captionSettings.Apply(settings);
            SeekAndWaitForCueAdded(mpe, 2000);
            colors = ReadRenderedCaptionColors(mpe, L"Alternate colors");
            VERIFY_ARE_EQUAL(0x00FFFFFFu, colors.regionBackground, L"A user window opacity alone overrides the cue region color (default white at 0%)");

            LOG_OUTPUT(L"Caption text opacity 0%%; everything else default.");
            SeekAndWaitForCueRemoved(mpe, 5500);
            settings = CaptionSettings();
            settings.fontOpacity = wmcc::ClosedCaptionOpacity::ZeroPercent;
            captionSettings.Apply(settings);
            SeekAndWaitForCueAdded(mpe, 2000);
            colors = ReadRenderedCaptionColors(mpe, L"Alternate colors");
            VERIFY_ARE_EQUAL(0.0f, colors.textOpacity, L"Caption text layer at 0%");
            VERIFY_ARE_EQUAL(0xFFFFFFFFu, colors.foreground, L"A user text opacity alone overrides the cue text color (default white; the opacity is applied to the text layer)");
        }

        //------------------------------------------------------------------------
        // Test case: Windows caption font style and size settings override the cue font
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionFontStyleAndSizeScaleCaptionText()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            // The cue asks for a serif font; the user setting must win.
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateTextCue(1000, 3000, L"User font", CreateStyle(L"proportionalSerif", 20), nullptr) }) });

            struct Case
            {
                wmcc::ClosedCaptionStyle style;
                wmcc::ClosedCaptionSize size;
                const wchar_t* expectedFontFamily;
                double expectedScale;
                bool expectSmallCaps;
            };
            const Case cases[] =
            {
                { wmcc::ClosedCaptionStyle::Casual, wmcc::ClosedCaptionSize::Default, L"Comic Sans MS", 1.0, false },
                { wmcc::ClosedCaptionStyle::SmallCapitals, wmcc::ClosedCaptionSize::FiftyPercent, L"Segoe UI", 0.5, true },
                { wmcc::ClosedCaptionStyle::MonospacedWithSerifs, wmcc::ClosedCaptionSize::OneHundredFiftyPercent, L"Courier New", 1.5, false },
                { wmcc::ClosedCaptionStyle::Cursive, wmcc::ClosedCaptionSize::TwoHundredPercent, L"Ink Free", 2.0, false },
                { wmcc::ClosedCaptionStyle::MonospacedWithoutSerifs, wmcc::ClosedCaptionSize::OneHundredPercent, L"Lucida Console", 1.0, false },
            };

            bool cueShown = false;
            for (const auto& testCase : cases)
            {
                if (cueShown)
                {
                    SeekAndWaitForCueRemoved(mpe, 5500);
                }
                CaptionSettings settings;
                settings.fontStyle = testCase.style;
                settings.fontSize = testCase.size;
                captionSettings.Apply(settings);
                SeekAndWaitForCueAdded(mpe, 2000);
                cueShown = true;

                RunOnUIThread([&]()
                {
                    auto caption = FindRenderedCaption(mpe, L"User font");
                    auto run = GetRun(caption.captionText, 0);
                    VerifyFontFamily(testCase.expectedFontFamily, run);
                    VERIFY_ARE_EQUAL(testCase.expectSmallCaps ? xaml::FontCapitals::SmallCaps : xaml::FontCapitals::Normal, xaml_docs::Typography::GetCapitals(run));
                    VerifyClose(20 * GetFontScale(caption) * testCase.expectedScale, run->FontSize, L"User caption size scales the style font size");
                    const double margin = ExpectedTextMargin(testCase.size);
                    VerifyThickness(MakeThickness(margin, margin, margin, margin), caption.textContainer->Margin, L"Text margin follows the caption size");
                });
            }
        }

        //------------------------------------------------------------------------
        // Test case: raised and depressed edge effects for every caption size
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionRaisedAndDepressedEdgeEffects()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateTextCue(1000, 3000, L"Edge effect", CreateStyle(L"default", 20), nullptr) }) });

            auto expectedOffsets = [](wmcc::ClosedCaptionEdgeEffect effect, wmcc::ClosedCaptionSize size) -> std::vector<std::pair<float, float>>
            {
                const bool raised = (effect == wmcc::ClosedCaptionEdgeEffect::Raised);
                switch (size)
                {
                case wmcc::ClosedCaptionSize::FiftyPercent:
                    return raised ? std::vector<std::pair<float, float>>{ {-0.5f, -0.5f}, {1.0f, 1.0f} } : std::vector<std::pair<float, float>>{ {-1.0f, -1.0f}, {1.0f, 1.0f} };
                case wmcc::ClosedCaptionSize::OneHundredFiftyPercent:
                    return raised ? std::vector<std::pair<float, float>>{ {-2.0f, -2.0f}, {1.0f, 1.0f}, {2.0f, 2.0f} } : std::vector<std::pair<float, float>>{ {-2.5f, -2.5f}, {2.5f, 2.5f} };
                case wmcc::ClosedCaptionSize::TwoHundredPercent:
                    return raised ? std::vector<std::pair<float, float>>{ {-2.0f, -2.0f}, {2.0f, 2.0f}, {4.0f, 4.0f} } : std::vector<std::pair<float, float>>{ {-3.0f, -3.0f}, {3.0f, 3.0f} };
                default:
                    return raised ? std::vector<std::pair<float, float>>{ {-1.0f, -1.0f}, {1.0f, 1.0f} } : std::vector<std::pair<float, float>>{ {-2.0f, -2.0f}, {2.0f, 2.0f} };
                }
            };

            bool cueShown = false;
            for (auto effect : { wmcc::ClosedCaptionEdgeEffect::Raised, wmcc::ClosedCaptionEdgeEffect::Depressed })
            {
                for (auto size : c_allCaptionSizes)
                {
                    if (cueShown)
                    {
                        SeekAndWaitForCueRemoved(mpe, 5500);
                    }
                    CaptionSettings settings;
                    settings.edgeEffect = effect;
                    settings.fontSize = size;
                    captionSettings.Apply(settings);
                    SeekAndWaitForCueAdded(mpe, 2000);
                    cueShown = true;

                    LOG_OUTPUT(L"Edge effect %d, caption size %d", static_cast<int>(effect), static_cast<int>(size));
                    RunOnUIThread([&]()
                    {
                        auto caption = FindRenderedCaption(mpe, L"Edge effect");
                        VerifyEdgeEffectCopies(expectedOffsets(effect, size), GetEdgeEffectCopyOffsets(caption), L"Raised/depressed copies");
                        // Raised: light copy (up-left) 60% white, dark copies (down-right) black.
                        // Depressed: dark copy (up-left) black, light copy (down-right) 20% black.
                        const bool raised = (effect == wmcc::ClosedCaptionEdgeEffect::Raised);
                        VerifyEdgeEffectCopyColors(caption, [raised](float x, float) -> unsigned int
                        {
                            const bool upLeft = x < 0;
                            return raised ? (upLeft ? 0x99FFFFFFu : 0xFF000000u) : (upLeft ? 0xFF000000u : 0x33000000u);
                        }, raised ? L"Raised copies" : L"Depressed copies");
                        const double margin = ExpectedTextMargin(size);
                        VerifyThickness(MakeThickness(margin, margin, margin, margin), caption.textContainer->Margin, L"Text margin follows the caption size");
                    });
                }
            }
        }

        //------------------------------------------------------------------------
        // Test case: depressed edge effect as the first edge effect of a new caption renderer
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionDepressedEdgeEffectAsFirstEffect()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            CaptionSettings settings;
            settings.edgeEffect = wmcc::ClosedCaptionEdgeEffect::Depressed;
            captionSettings.Apply(settings);

            // A new MediaPlayerElement, so no other edge effect has created the composition brushes yet.
            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateTextCue(1000, 3000, L"Depressed first", CreateStyle(L"default", 20), nullptr) }) });
            SeekAndWaitForCueAdded(mpe, 2000);

            RunOnUIThread([&]()
            {
                auto caption = FindRenderedCaption(mpe, L"Depressed first");
                VerifyEdgeEffectCopies({ {-2.0f, -2.0f}, {2.0f, 2.0f} }, GetEdgeEffectCopyOffsets(caption), L"Depressed copies at the default caption size");
                // Dark copy (up-left) black, light copy (down-right) 20% black.
                VerifyEdgeEffectCopyColors(caption, [](float x, float) -> unsigned int { return x < 0 ? 0xFF000000u : 0x33000000u; }, L"Depressed copies");
            });
        }

        //------------------------------------------------------------------------
        // Test case: uniform edge effect for every caption size
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionUniformEdgeEffect()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateTextCue(1000, 3000, L"Uniform", CreateStyle(L"default", 20), nullptr) }) });

            const std::vector<std::pair<float, float>> halfPixelRing = { {-0.5f, -0.5f}, {0.5f, -0.5f}, {-0.5f, 0.5f}, {0.5f, 0.5f} };
            const std::vector<std::pair<float, float>> onePixelRing = { {-1.0f, -1.0f}, {0.0f, -1.0f}, {1.0f, -1.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}, {-1.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f} };
            const std::vector<std::pair<float, float>> wideRing =
            {
                {-1.5f, -1.5f}, {-0.5f, -1.5f}, {0.5f, -1.5f}, {1.5f, -1.5f},
                {-1.5f, -0.5f}, {1.5f, -0.5f}, {-1.5f, 0.5f}, {1.5f, 0.5f},
                {-1.5f, 1.5f}, {-0.5f, 1.5f}, {0.5f, 1.5f}, {1.5f, 1.5f},
            };

            bool cueShown = false;
            for (auto size : c_allCaptionSizes)
            {
                if (cueShown)
                {
                    SeekAndWaitForCueRemoved(mpe, 5500);
                }
                CaptionSettings settings;
                settings.edgeEffect = wmcc::ClosedCaptionEdgeEffect::Uniform;
                settings.fontSize = size;
                captionSettings.Apply(settings);
                SeekAndWaitForCueAdded(mpe, 2000);
                cueShown = true;

                LOG_OUTPUT(L"Caption size %d", static_cast<int>(size));
                RunOnUIThread([&]()
                {
                    auto caption = FindRenderedCaption(mpe, L"Uniform");
                    const auto& expected =
                        (size == wmcc::ClosedCaptionSize::FiftyPercent) ? halfPixelRing :
                        (size == wmcc::ClosedCaptionSize::TwoHundredPercent) ? wideRing : onePixelRing;
                    VerifyEdgeEffectCopies(expected, GetEdgeEffectCopyOffsets(caption), L"Uniform copies");
                    VerifyEdgeEffectCopyColors(caption, [](float, float) { return 0xFF000000u; }, L"Uniform copies are black");
                });
            }
        }

        //------------------------------------------------------------------------
        // Test case: drop shadow edge effect for every caption size
        //------------------------------------------------------------------------
        void MediaCCTestsMPE::UserCaptionDropShadowEdgeEffect()
        {
            TestCleanupWrapper cleanup;
            CaptionSettingsScope captionSettings;
            captionSettings.Apply(CaptionSettings());

            MediaHelperMPE mediaHelper;
            MediaPlayerElement^ mpe = mediaHelper.GetMediaPlayerElement();
            SetupCaptionTracks(mediaHelper, GetResourcesPath(), { CreateCaptionTrack({ CreateTextCue(1000, 3000, L"Drop shadow", CreateStyle(L"default", 20), nullptr) }) });

            struct Case
            {
                wmcc::ClosedCaptionSize size;
                float blurRadius;
                float offsetY;
            };
            const Case cases[] =
            {
                { wmcc::ClosedCaptionSize::Default, 6.0f, 7.0f },
                { wmcc::ClosedCaptionSize::FiftyPercent, 5.0f, 4.0f },
                { wmcc::ClosedCaptionSize::OneHundredPercent, 6.0f, 7.0f },
                { wmcc::ClosedCaptionSize::OneHundredFiftyPercent, 7.0f, 8.0f },
                { wmcc::ClosedCaptionSize::TwoHundredPercent, 11.0f, 8.0f },
            };

            bool cueShown = false;
            for (const auto& testCase : cases)
            {
                if (cueShown)
                {
                    SeekAndWaitForCueRemoved(mpe, 5500);
                }
                CaptionSettings settings;
                settings.edgeEffect = wmcc::ClosedCaptionEdgeEffect::DropShadow;
                settings.fontSize = testCase.size;
                captionSettings.Apply(settings);
                SeekAndWaitForCueAdded(mpe, 2000);
                cueShown = true;

                LOG_OUTPUT(L"Caption size %d", static_cast<int>(testCase.size));
                RunOnUIThread([&]()
                {
                    auto caption = FindRenderedCaption(mpe, L"Drop shadow");
                    VERIFY_IS_TRUE(GetEdgeEffectCopyOffsets(caption).empty(), L"A drop shadow does not create offset copies of the text");

                    auto layerVisual = dynamic_cast<Microsoft::UI::Composition::LayerVisual^>(xaml_hosting::ElementCompositionPreview::GetElementVisual(caption.captionText));
                    VERIFY_IS_NOT_NULL(layerVisual, L"The caption TextBlock is rendered through a LayerVisual");
                    auto shadow = dynamic_cast<Microsoft::UI::Composition::DropShadow^>(layerVisual->Shadow);
                    VERIFY_IS_NOT_NULL(shadow, L"The caption LayerVisual has a drop shadow");
                    VerifyClose(testCase.blurRadius, shadow->BlurRadius, L"Drop shadow blur radius");
                    VerifyClose(0.0, shadow->Offset.x, L"Drop shadow x offset");
                    VerifyClose(testCase.offsetY, shadow->Offset.y, L"Drop shadow y offset");
                    VERIFY_ARE_EQUAL(0xFF, shadow->Color.A);
                    VERIFY_IS_TRUE(shadow->Color.R == 0 && shadow->Color.G == 0 && shadow->Color.B == 0, L"Drop shadow is black");
                });
            }
        }

    } } }
} } } }
