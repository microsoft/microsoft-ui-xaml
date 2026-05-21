// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

using namespace ::Windows::Media::Playback;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        class MediaHelperMPE
        {
        public:
            MediaHelperMPE(bool enableMediaFailed = true, bool enableMediaOpened = true, bool enableMediaEnded = true);

            static void SetupBasicMediaPlayerElement(_In_ MediaHelperMPE& MediaHelperMPE, _In_ Platform::String^ resourcePath, _In_ Platform::String^ fileName, _In_ bool useMTC, _In_ bool waitForOpened, _In_ bool useNewPlayback);
            static void SetupMediaPlayerElementWithCC(_In_ MediaHelperMPE& MediaHelperMPE, _In_ Platform::String^ resourcePath, _In_ Platform::String^ ccFile, _In_ bool useMTC, _In_ Platform::String^ lang);
            static void SetupMediaPlayerElementWithImageCue(_In_ MediaHelperMPE& MediaHelperMPE, _In_ Platform::String^ resourcePath, _In_ bool useMTC, _In_ int numberOfCues, _In_ bool usePercentage);

            static Platform::String^ GetDefaultResourcePath();

            static void SetupMediaPlayerElementUI(
                _In_ MediaHelperMPE& MediaHelperMPE,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& stateChangeEvent,
                _In_ std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& tappedEvent,
                _In_ SafeEventRegistrationType(MediaPlaybackSession, PlaybackStateChanged)& stateChangeRegistration,
                _In_ SafeEventRegistrationType(xaml_controls::MediaPlayerElement, Tapped)& tappedRegistration,
                _In_ Platform::String^ fileName,
                _In_ bool waitForEvents);
            static INT64 s_HNSPerMillisecond;
            static ::MediaPlaybackItem^ s_CurrentPlaybackItem;

            void AddDefaultEventHandlers(_In_ xaml_controls::MediaPlayerElement^ mediaElement = nullptr);
            xaml_controls::MediaPlayerElement^ GetMediaPlayerElement() const { return m_mpe; };

            void RemoveDefaultMediaFailed();
            void RemoveDefaultMediaOpened();
            void RemoveDefaultMediaEnded();

        private:
            SafeEventRegistrationType(MediaPlayer, MediaFailed) m_mediaFailedRegistration =
                CreateSafeEventRegistration(MediaPlayer, MediaFailed);

            SafeEventRegistrationType(MediaPlayer, MediaOpened) m_mediaOpenedRegistration =
                CreateSafeEventRegistration(MediaPlayer, MediaOpened);

            SafeEventRegistrationType(MediaPlayer, MediaEnded) m_mediaEndedRegistration =
                CreateSafeEventRegistration(MediaPlayer, MediaEnded);

            static void AddImageCue(
                ::Windows::Media::Core::TimedMetadataTrack^ track,
                long startTimeInMs,
                long durationInMs,
                double positionX,
                double positionY,
                double height,
                double width,
                ::Windows::Graphics::Imaging::SoftwareBitmap^ softwareBitmap,
                bool usePercentage);

            bool m_enableMediaFailed;
            bool m_enableMediaOpened;
            bool m_enableMediaEnded;

            xaml_controls::MediaPlayerElement^ m_mpe;
        };

    } } }
} } } }
