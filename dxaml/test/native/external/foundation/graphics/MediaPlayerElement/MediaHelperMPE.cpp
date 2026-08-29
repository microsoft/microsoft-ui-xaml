// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include <MediaHelperMPE.h>
#include <Collection.h>
#include <mferror.h>
#include <Windows.Graphics.Imaging.Interop.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::Media::Core;
using namespace ::Windows::Media::Playback;
using namespace Microsoft::WRL;
using namespace test_infra;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Graphics::Imaging;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        INT64 MediaHelperMPE::s_HNSPerMillisecond = 10000;
        ::Windows::Media::Playback::MediaPlaybackItem^ MediaHelperMPE::s_CurrentPlaybackItem = nullptr;

        MediaHelperMPE::MediaHelperMPE(bool enableMediaFailed, bool enableMediaOpened, bool enableMediaEnded)
        {
            m_enableMediaFailed = enableMediaFailed;
            m_enableMediaOpened = enableMediaOpened;
            m_enableMediaEnded = enableMediaEnded;

            RunOnUIThread([&]()
            {
                m_mpe = ref new xaml_controls::MediaPlayerElement();
                auto player =  ref new MediaPlayer();
                m_mpe->SetMediaPlayer(player);      // Explicitly create MediaPlayer so we can attach MediaOpened/Ended/Failed
                AddDefaultEventHandlers();
            });
        }        

        Platform::String^ MediaHelperMPE::GetDefaultResourcePath()
        {
            Platform::String^ defaultResourcePath = GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\media\\";
            return defaultResourcePath;
        }

        void MediaHelperMPE::AddDefaultEventHandlers(_In_ xaml_controls::MediaPlayerElement^ mpe)
        {
            if (mpe == nullptr)
            {
                mpe = m_mpe;
            }

            RunOnUIThread([&]()
            {
                if (m_enableMediaFailed && !m_mediaFailedRegistration.IsAttached())
                {
                    m_mediaFailedRegistration.Attach(
                        mpe->MediaPlayer,
                        ref new wf::TypedEventHandler<MediaPlayer ^, MediaPlayerFailedEventArgs ^>([&](MediaPlayer^, MediaPlayerFailedEventArgs^ e)
                        {
                            VERIFY_FAIL((L"MediaFailed Event has fired unintentionally. Exception Message: " + e->ErrorMessage)->Data());
                        }));
                }

                if (m_enableMediaOpened && !m_mediaOpenedRegistration.IsAttached())
                {
                    m_mediaOpenedRegistration.Attach(
                        mpe->MediaPlayer,
                        ref new wf::TypedEventHandler<MediaPlayer ^, Platform::Object ^>([&](MediaPlayer^, Platform::Object^)
                    {
                        LOG_OUTPUT(L"Media Opened has occurred");
                    }));
                }

                if (m_enableMediaEnded && !m_mediaEndedRegistration.IsAttached())
                {
                    m_mediaEndedRegistration.Attach(
                        mpe->MediaPlayer,
                        ref new wf::TypedEventHandler<MediaPlayer ^, Platform::Object ^>([&](MediaPlayer^, Platform::Object^)
                    {
                        LOG_OUTPUT(L"Media Ended has occurred");
                    }));
                }
            });
        }

        void MediaHelperMPE::RemoveDefaultMediaFailed()
        {
            if (m_mediaFailedRegistration.IsAttached())
            {
                m_mediaFailedRegistration.Detach();
            }
        }

        void MediaHelperMPE::RemoveDefaultMediaOpened()
        {
            if (m_mediaOpenedRegistration.IsAttached())
            {
                m_mediaOpenedRegistration.Detach();
            }
        }

        void MediaHelperMPE::RemoveDefaultMediaEnded()
        {
            if (m_mediaEndedRegistration.IsAttached())
            {
                m_mediaEndedRegistration.Detach();
            }
        }

        void MediaHelperMPE::SetupBasicMediaPlayerElement(_In_ MediaHelperMPE& MediaHelperMPE, _In_ Platform::String^ resourcePath, _In_ Platform::String^ fileName, _In_ bool useMTC, _In_ bool waitForOpened, _In_ bool useNewPlayback)
        {
            StackPanel^ rootStackPanel = nullptr;

            auto loadedRegistration = CreateSafeEventRegistration(StackPanel, Loaded);
            auto openedRegistration = CreateSafeEventRegistration(MediaPlayer, MediaOpened);
            auto loadedEvent = std::make_shared<Event>();
            auto mediaOpenedEvent = std::make_shared<Event>();

            MediaHelperMPE.RemoveDefaultMediaOpened();

            xaml_controls::MediaPlayerElement^ mpe = nullptr;

            LOG_OUTPUT(L"Create the MediaPlayerElement and stack panel");
            RunOnUIThread([&]()
            {
                rootStackPanel = ref new xaml_controls::StackPanel();
                loadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded event fired on StackPanel");
                    loadedEvent->Set();
                }));
                mpe = MediaHelperMPE.GetMediaPlayerElement();
                mpe->AutoPlay = false;

                openedRegistration.Attach(
                    mpe->MediaPlayer,
                    ref new wf::TypedEventHandler<MediaPlayer ^, Platform::Object ^>([&](MediaPlayer^, Platform::Object^)
                {
                    LOG_OUTPUT(L"Media Opened Event Fired");
                    mediaOpenedEvent->Set();
                }));

                rootStackPanel->Children->Append(mpe);
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Set the MTC & Source");
            RunOnUIThread([&]()
            {
                mpe->AreTransportControlsEnabled = useMTC;

                if (resourcePath != nullptr && fileName != nullptr)
                {
                    auto testUri = ref new Uri(resourcePath + fileName);
                    VERIFY_IS_NOT_NULL(testUri);

                    if (useNewPlayback)
                    {
                        LOG_OUTPUT(L"Set the source with SetPlaybackSource");

                        auto source = ::Windows::Media::Core::MediaSource::CreateFromUri(testUri);
                        VERIFY_IS_NOT_NULL(source);

                        auto playbackItem = ref new ::Windows::Media::Playback::MediaPlaybackItem(source);
                        VERIFY_IS_NOT_NULL(playbackItem);

                        mpe->Source = playbackItem;

                        s_CurrentPlaybackItem = playbackItem;
                    }
                    else
                    {
                        LOG_OUTPUT(L"Set the source with Source");
                        auto source = ::Windows::Media::Core::MediaSource::CreateFromUri(testUri);
                        mpe->Source = source;
                        s_CurrentPlaybackItem = nullptr;
                    }
                }
            });

            if (waitForOpened)
            {
                mediaOpenedEvent->WaitForDefault();
            }
            TestServices::WindowHelper->WaitForIdle();

            MediaHelperMPE.AddDefaultEventHandlers();
            LOG_OUTPUT(L"Setup complete");
        }

        void MediaHelperMPE::SetupMediaPlayerElementWithCC(_In_ MediaHelperMPE& MediaHelperMPE, _In_ Platform::String^ resourcePath, _In_ Platform::String^ ccFile, _In_ bool useMTC, _In_ Platform::String^ lang)
        {
            MediaPlayerElement^ mpe = MediaHelperMPE.GetMediaPlayerElement();
            MediaHelperMPE::SetupBasicMediaPlayerElement(MediaHelperMPE, resourcePath, L"blueframe_video.mp4", useMTC, true, true);

            auto trackAddedEvent = std::make_shared<Event>();
            auto trackAdded = CreateSafeEventRegistration(MediaPlaybackItem, TimedMetadataTracksChanged);

            LOG_OUTPUT(L"Adding CC tracks");
            RunOnUIThread([&]()
            {
                auto ccUri = ref new Uri(resourcePath + ccFile);
                VERIFY_IS_NOT_NULL(ccUri);

                auto ccSource = ::Windows::Media::Core::TimedTextSource::CreateFromUri(ccUri, nullptr);
                VERIFY_IS_NOT_NULL(ccSource);

                VERIFY_IS_NOT_NULL(s_CurrentPlaybackItem);

                auto mediaSource = s_CurrentPlaybackItem->Source;
                VERIFY_IS_NOT_NULL(mediaSource);

                trackAdded.Attach(
                    s_CurrentPlaybackItem,
                    ref new ::Windows::Foundation::TypedEventHandler<MediaPlaybackItem^, IVectorChangedEventArgs^>(
                        [trackAddedEvent](MediaPlaybackItem^ sender, IVectorChangedEventArgs^ arg)
                {
                    switch (arg->CollectionChange)
                    {
                    case CollectionChange::ItemInserted:
                        LOG_OUTPUT(L"Track inserted");
                        trackAddedEvent->Set();
                        break;
                    }

                    sender->TimedMetadataTracks->SetPresentationMode(0, TimedMetadataTrackPresentationMode::PlatformPresented);
                    LOG_OUTPUT(L"Track 0 visible");
                }));

                mediaSource->ExternalTimedTextSources->Append(ccSource);
            });

            TestServices::WindowHelper->WaitForIdle();
            trackAddedEvent->WaitForDefault();

            LOG_OUTPUT(L"Tracks added");
        }

        void MediaHelperMPE::AddImageCue(
            TimedMetadataTrack^ track,
            long startTimeInMs,
            long durationInMs,
            double positionX,
            double positionY,
            double height,
            double width,
            SoftwareBitmap ^softwareBitmap,
            bool usePercentage)
        {
            TimeSpan startTime;
            TimeSpan duration;
            ::Windows::Media::Core::TimedTextPoint position;
            ::Windows::Media::Core::TimedTextSize extent = {};

            auto imageCue = ref new ImageCue();
            VERIFY_IS_NOT_NULL(imageCue);
            startTime.Duration = startTimeInMs * 10000;
            duration.Duration = durationInMs * 10000;
            imageCue->StartTime = startTime;
            imageCue->Duration = duration;
            imageCue->SoftwareBitmap = softwareBitmap;
            position.Unit = usePercentage ? ::Windows::Media::Core::TimedTextUnit::Percentage : ::Windows::Media::Core::TimedTextUnit::Pixels;
            position.X = positionX;
            position.Y = positionY;
            imageCue->Position = position;
            extent.Height = height;
            extent.Width = width;
            extent.Unit = usePercentage ? ::Windows::Media::Core::TimedTextUnit::Percentage : ::Windows::Media::Core::TimedTextUnit::Pixels;
            imageCue->Extent = extent;
            track->AddCue(imageCue);
        }

        void MediaHelperMPE::SetupMediaPlayerElementWithImageCue(
            _In_ MediaHelperMPE& MediaHelperMPE,
            _In_ Platform::String^ resourcePath,
            _In_ bool useMTC,
            _In_ int numberOfCues,
            _In_ bool usePercentage)
        {
            wgri::SoftwareBitmap^ softwareBitmap = nullptr;
            auto imageDecodedEvent = std::make_shared<Event>();
            TimedMetadataTrack^ timedMetadataTrack = nullptr;

            RunOnUIThread([&]()
            {
                auto path = GetPackageFolder() + L"resources\\native\\foundation\\graphics\\rendering\\asdf.png";
                concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(path))
                    .then([](::Windows::Storage::StorageFile^ picture)
                {
                    LOG_OUTPUT(L"Opening file");
                    return picture->OpenAsync(::Windows::Storage::FileAccessMode::Read);
                }).then([](IRandomAccessStream^ stream)
                {
                    LOG_OUTPUT(L"Creating decoder");
                    return BitmapDecoder::CreateAsync(stream);
                }).then([](BitmapDecoder^ decoder)
                {
                    LOG_OUTPUT(L"Decoding");
                    return decoder->GetSoftwareBitmapAsync(wgri::BitmapPixelFormat::Bgra8, wgri::BitmapAlphaMode::Premultiplied);
                }).then([&softwareBitmap, imageDecodedEvent](SoftwareBitmap^ softbitmap)
                {
                    LOG_OUTPUT(L"Setting software bitmap");
                    softwareBitmap = softbitmap;
                    imageDecodedEvent->Set();
                });
            });
            imageDecodedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating TimedMedia ImageSubtitle track");
                timedMetadataTrack = ref new TimedMetadataTrack(L"TrackID", L"", TimedMetadataKind::ImageSubtitle);
                VERIFY_IS_NOT_NULL(timedMetadataTrack);

                double positionY = 60;
                long startTimeInMs = 1000;
                for (int i = 0; i < numberOfCues; i++)
                {
                    if (usePercentage)
                    {
                        AddImageCue(timedMetadataTrack, startTimeInMs /*startTimeInMs*/,  2000/*durationInMs*/, 50 /*positionX*/, positionY, 10 /*height*/, 10 /*width*/, softwareBitmap, usePercentage);
                        positionY += 10;
                        startTimeInMs += 1; //unnecessary but need to increment the starting time for multiple cues, otherwise it will crash
                    }
                    else
                    {
                        AddImageCue(timedMetadataTrack, startTimeInMs /*startTimeInMs*/, 2000 /*durationInMs*/, 50 /*positionX*/, positionY, 80 /*height*/, 200 /*width*/, softwareBitmap, usePercentage);
                        positionY += 100;
                        startTimeInMs += 1; //unnecessary but need to increment the starting time for multiple cues, otherwise it will crash
                    }
                }
            });

            MediaPlayerElement^ mpe = MediaHelperMPE.GetMediaPlayerElement();
            MediaHelperMPE::SetupBasicMediaPlayerElement(MediaHelperMPE, resourcePath, L"blueframe_video.mp4", useMTC, true, true);

            auto trackAddedEvent = std::make_shared<Event>();
            auto trackAdded = CreateSafeEventRegistration(MediaPlaybackItem, TimedMetadataTracksChanged);

            RunOnUIThread([&]()
            {
                auto mediaSource = s_CurrentPlaybackItem->Source;
                VERIFY_IS_NOT_NULL(mediaSource);

                trackAdded.Attach(
                    s_CurrentPlaybackItem,
                    ref new ::Windows::Foundation::TypedEventHandler<MediaPlaybackItem^, IVectorChangedEventArgs^>(
                        [trackAddedEvent](MediaPlaybackItem^ sender, IVectorChangedEventArgs^ arg)
                {
                    switch (arg->CollectionChange)
                    {
                    case CollectionChange::ItemInserted:
                        LOG_OUTPUT(L"Track inserted");
                        trackAddedEvent->Set();
                        break;
                    }
                    sender->TimedMetadataTracks->SetPresentationMode(0, TimedMetadataTrackPresentationMode::PlatformPresented);
                }));

                mediaSource->ExternalTimedMetadataTracks->Append(timedMetadataTrack);
            });

            trackAddedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        void MediaHelperMPE::SetupMediaPlayerElementUI(
            _In_ MediaHelperMPE& MediaHelperMPE,
            _In_ std::shared_ptr<Event>& stateChangeEvent,
            _In_ std::shared_ptr<Event>& tappedEvent,
            _In_ SafeEventRegistrationType(wmp::MediaPlaybackSession, PlaybackStateChanged)& stateChangeRegistration,
            _In_ SafeEventRegistrationType(MediaPlayerElement, Tapped)& tappedRegistration,
            _In_ Platform::String^ fileName,
            _In_ bool waitForEvents)
        {

            TestServices::WindowHelper->SetWindowSizeOverride(Size(400, 300));

            MediaPlayerElement^ mpe = nullptr;
            StackPanel^ rootStackPanel = nullptr;
            auto loadedRegistration = CreateSafeEventRegistration(StackPanel, Loaded);
            auto openedRegistration = CreateSafeEventRegistration(MediaPlayer, MediaOpened);
            auto loadedEvent = std::make_shared<Event>();
            auto openedEvent = std::make_shared<Event>();

            MediaHelperMPE.RemoveDefaultMediaOpened();

            LOG_OUTPUT(L"Create the MediaPlayerElement and stack panel");
            RunOnUIThread([&]()
            {
                rootStackPanel = ref new xaml_controls::StackPanel();
                loadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Loaded event fired on StackPanel");
                    loadedEvent->Set();
                }));
                mpe = MediaHelperMPE.GetMediaPlayerElement();

                openedRegistration.Attach(
                    mpe->MediaPlayer,
                    ref new wf::TypedEventHandler<MediaPlayer ^, Platform::Object ^>([&](MediaPlayer^, Platform::Object^)
                {
                    LOG_OUTPUT(L"Media Opened Event Fired");
                    openedEvent->Set();
                    stateChangeRegistration.Attach(
                        mpe->MediaPlayer->PlaybackSession,
                        ref new wf::TypedEventHandler<MediaPlaybackSession ^,Platform::Object ^>([&](MediaPlaybackSession^ session, Platform::Object^)
                        {
                            LOG_OUTPUT(L"State Change Event Fired %s", session->PlaybackState.ToString()->Data());
                            stateChangeEvent->Set();
                        }));
                }));
                rootStackPanel->Children->Append(mpe);
                TestServices::WindowHelper->WindowContent = rootStackPanel;
            });
            TestServices::WindowHelper->WaitForIdle();
            loadedEvent->WaitForDefault();

            LOG_OUTPUT(L"Set the MTC & Source");
            RunOnUIThread([&]()
            {
                mpe->AreTransportControlsEnabled = true;
                auto source = ::Windows::Media::Core::MediaSource::CreateFromUri(ref new ::Windows::Foundation::Uri(GetDefaultResourcePath() + fileName));
                mpe->Source = source;
                tappedRegistration.Attach(mpe, ref new xaml_input::TappedEventHandler([tappedEvent](Platform::Object^, xaml_input::TappedRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"MediaPlayerElement Tapped for Show MTC");
                    tappedEvent->Set();
                }));

            });
            if (waitForEvents)
            {
                openedEvent->WaitForDefault();
                stateChangeEvent->WaitForDefault();
            }
            TestServices::WindowHelper->WaitForIdle();

            MediaHelperMPE.AddDefaultEventHandlers();
        }
    } } }
} } } }
