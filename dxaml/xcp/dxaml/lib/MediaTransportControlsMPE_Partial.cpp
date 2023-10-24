// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBlock.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "Button.g.h"
#include "ToggleButton.g.h"
#include "Slider.g.h"
#include "Image.g.h"
#include "ImageBrush.g.h"
#include "BitmapImage.g.h"
#include "MediaTransportControls.g.h"
#include "MediaTransportControlsHelper.g.h"
#include "MediaPlayerElement.g.h"
#include "MediaTransportControlsThumbnailRequestedEventArgs.g.h"
#include "TimedTextSource.h"
#include <Windows.Media.h>
#include <Windows.Media.Playback.h>
#include <FrameworkUdk/MediaPlaybackDataSourceExtension.h>
#include <MediaPlaybackListExtension.h>
#include <MediaPlaybackSessionExtension.h>
#include <FrameworkUdk/MediaPlayerExtension.h>
#include "AgileCallback.h"
#include "Grid.g.h"
#include "MediaPlayerExtensions.h"
#include "AutomationProperties.h"
#include "localizedResource.h"

#pragma warning(disable:4996) // use of apis marked as [[deprecated("PrivateAPI")]]

using namespace ctl;
using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL;

_Check_return_ HRESULT
MediaTransportControls::InitializeTransportControls(_In_ MediaPlayerElement* pMediaPlayerElement)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    BOOLEAN isFullWindow = FALSE;
#endif // DISABLE_FULL_WINDOW
    ctl::ComPtr<MediaPlayerElement> spOwnerMPE = pMediaPlayerElement;
    ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer;
    ctl::ComPtr<MediaPlayerElement> spOldOwnerMPE;

    IFCPTR(pMediaPlayerElement);

    IFC(m_wrOwnerParent.As(&spOldOwnerMPE));

    if (spOldOwnerMPE != spOwnerMPE)
    {
        // Initialize first Time Transport Controls.
#if false // DISABLE_FULL_WINDOW
        if (!m_wrOwnerParent)
        {
            IFC(spOwnerMPE->get_IsFullWindow(&isFullWindow));
            m_isFullWindow = isFullWindow;
        }
#endif // DISABLE_FULL_WINDOW
        // Set weak reference to parent MPE
        IFC(ctl::AsWeak(pMediaPlayerElement, &m_wrOwnerParent));

        // set reference to MediaPlayer
        IFC(spOwnerMPE->get_MediaPlayer(&spMediaPlayer));
        if (m_spMediaPlayer != spMediaPlayer)
        {
            if (m_isMediaPlayerSubscribed)
            {
                // Unregister MediaPlayer notification
                IFC(UnSubscribeMediaPlayerEvents());
                m_isMediaPlayerSubscribed = FALSE;
            }
            m_spMediaPlayer = spMediaPlayer;
            // delay subscription till template applied.
            if (m_isTemplateApplied)
            {
                IFC(SubscribeMediaPlayerEvents());
                m_isMediaPlayerSubscribed = TRUE;
            }
        }
        m_parentType = MTCParent_MediaPlayerElement;
    }

Cleanup:
    return hr;

}
_Check_return_ HRESULT
MediaTransportControls::DeinitializeTransportControlsFromMPE()
{
    if (m_isMediaPlayerSubscribed)
    {
        // Unregister MediaPlayer notification
        IFC_RETURN(UnSubscribeMediaPlayerEvents());
        m_isMediaPlayerSubscribed = FALSE;
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::InitializeVisualStateFromMPE()
{
    HRESULT hr = S_OK;
    wmp::MediaPlaybackState currentState = wmp::MediaPlaybackState_None;

    if (m_spMediaPlayer)
    {
        wf::TimeSpan value;
        ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;

        // Since we delay subscription till template applied.Now we need to subscribe after template applied.
        if (m_isTemplateApplied && !m_isMediaPlayerSubscribed)
        {
            IFC(SubscribeMediaPlayerEvents());
            m_isMediaPlayerSubscribed = TRUE;
        }

        IFC(GetCurrentPlaybackSession(&spPlaybackSession));
        // Refresh cached sourceReady, isPlaying and isBuffering states
        IFC(spPlaybackSession->get_PlaybackState(&currentState));

        m_sourceLoaded = currentState != wmp::MediaPlaybackState_None &&
            currentState != wmp::MediaPlaybackState_Opening;
        m_isPlaying = currentState == wmp::MediaPlaybackState_Buffering ||
            currentState == wmp::MediaPlaybackState_Playing;
        m_isBuffering = currentState == wmp::MediaPlaybackState_Buffering;

        if (m_sourceLoaded || m_isPlaying || m_isBuffering || m_isBreakPlaying)
        {
            // Refresh cached NaturalDuration value
            IFC(spPlaybackSession->get_NaturalDuration(&value));
            m_naturalDuration.TimeSpan = value;
        }

        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            ctl::ComPtr<wfc::IVectorView<wmc::VideoTrack*>> spVideoTracks;
            ctl::ComPtr<wfc::IVectorView<wmc::AudioTrack*>> spAudioTracks;
            unsigned int audioCount = 0, videoCount = 0;
            spPlaybackItem->get_VideoTracks(&spVideoTracks);
            spPlaybackItem->get_AudioTracks(&spAudioTracks);
            IFC(spVideoTracks->get_Size(&videoCount));
            IFC(spAudioTracks->get_Size(&audioCount));
            m_isAudioOnly = (videoCount == 0 && audioCount > 0);
            IFC(UpdateCCSelectionUI());
        }

        IFC(UpdateTracksUI());
        IFC(UpdateTrickModeFallbackUI());
        IFC(UpdateSeekPositionsUI());
        IFC(UpdatePlaybackRateUI());
        IFC(UpdateBreakUI());
    }
    else
    {
        // Reset default values.
        m_naturalDuration.TimeSpan.Duration = 0;
        m_sourceLoaded = FALSE;
        m_isPlaying = FALSE;
        m_isBuffering = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
MediaTransportControls::UpdateVisualStateFromMPE(_In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN wentToState = FALSE;
    wmp::MediaPlaybackState currentState = wmp::MediaPlaybackState_None;
    ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
    BOOLEAN isMuted = FALSE;

    if (m_transportControlsEnabled)
    {
        // Set Control Panel visibility
        if (m_controlPanelVisibilityChanged)
        {
            MTCTelemetryData data;
            data.isCPVisible = m_controlPanelIsVisible;
            m_AggTelemetry.AddData(MTCTelemetryType::ControlPanelVisibility, data);

            if (m_controlPanelIsVisible)
            {
                IFC(GoToState(bUseTransitions, L"ControlPanelFadeIn", &wentToState));
            }
            else
            {
                IFC(GoToState(bUseTransitions, L"ControlPanelFadeOut", &wentToState));
            }

            m_controlPanelVisibilityChanged = FALSE;
        }

        // Set Media State visual state
        if (m_hasError)
        {
            IFC(GoToState(bUseTransitions, L"Error", &wentToState));
        }
        else
        {
            if (m_spMediaPlayer)
            {
               IFC(GetCurrentPlaybackSession(&spPlaybackSession));
               IFC(spPlaybackSession->get_PlaybackState(&currentState));
            }

            switch (currentState)
            {

                case wmp::MediaPlaybackState_None:
                {
                    IFC(GoToState(bUseTransitions, L"Disabled", &wentToState));
                    break;
                }
                case wmp::MediaPlaybackState_Opening:
                {
                    IFC(GoToState(bUseTransitions, L"Loading", &wentToState));
                    break;
                }
                case wmp::MediaPlaybackState_Buffering:
                {
                    IFC(GoToState(bUseTransitions, L"Buffering", &wentToState));
                    break;
                }
                case wmp::MediaPlaybackState_Playing:
                case wmp::MediaPlaybackState_Paused:
                {
                    IFC(GoToState(bUseTransitions, L"Normal", &wentToState));
                    break;
                }
            }
        }

        //  Specific 2 row vs 1 row
        IFC(GoToState(bUseTransitions, (m_isCompact ? L"CompactMode" : L"NormalMode"), &wentToState));
        IFC(GoToState(bUseTransitions, (m_isPlaying ? L"PauseState" : L"PlayState"), &wentToState));
        if (m_spMediaPlayer)
        {
            IFC(m_spMediaPlayer->get_IsMuted(&isMuted));
        }
        IFC(GoToState(bUseTransitions, isMuted ? L"MuteState" : L"VolumeState", &wentToState));
#if false // DISABLE_FULL_WINDOW
        IFC(GoToState(bUseTransitions, m_isFullWindow && !m_isMiniView ? L"FullWindowState" : L"NonFullWindowState", &wentToState));
#endif // DISABLE_FULL_WINDOW
        // Set Audio Selection Availability
        IFC(GoToState(bUseTransitions, m_hasMultipleAudioStreams ? L"AudioSelectionAvailable" : L"AudioSelectionUnavailable", &wentToState));
        IFC(GoToState(bUseTransitions, m_hasCCTracks ? L"CCSelectionAvailable" : L"CCSelectionUnavailable", &wentToState));

        MediaPlaybackDataSourceExtension_RepeatMode repeatMode;

        IFC(GetRepeatMode(&repeatMode));
        IFC(GoToState(
                bUseTransitions, 
                repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::None ?
                    L"RepeatNoneState" : 
                    (repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::One) ? L"RepeatOneState" : L"RepeatAllState",
                &wentToState));
    }

Cleanup:
    return hr;

}

_Check_return_ HRESULT
MediaTransportControls::ShowControlPanelFromMPE()
{
    ctl::ComPtr<MediaPlayerElement> spOwnerMPE;
    IFC_RETURN(m_wrOwnerParent.As(&spOwnerMPE));
    if (spOwnerMPE.Get() && m_tpControlPanelGrid)
    {
        // The RootGrid of the MTC is expanded to fill the entire MediaPlayerElement, we only
        // want the height of the MTC bar at the bottom. The ControlPanelGrid is the height
        // we want for that.
        DOUBLE height = 0.0;
        IFC_RETURN(m_tpControlPanelGrid.Cast<Grid>()->get_ActualHeight(&height));
        IFC_RETURN(spOwnerMPE->OnMTCVisible(height));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::HideControlPanelFromMPE()
{
    ctl::ComPtr<MediaPlayerElement> spOwnerMPE;
    IFC_RETURN(m_wrOwnerParent.As(&spOwnerMPE));
    if (spOwnerMPE.Get())
    {
        IFC_RETURN(spOwnerMPE->OnMTCVisible(0));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnPlayPauseFromMPE()
{
     HRESULT hr = S_OK;

    if (m_spMediaPlayer)
    {
        wmp::MediaPlayerState currentState = wmp::MediaPlayerState_Closed;
        IFC(m_spMediaPlayer->get_CurrentState(&currentState));
        if (currentState == wmp::MediaPlayerState_Closed)
        {
            if (m_hasError)
            {
                ASSERT(!m_isPlaying);
                IFC(Play());
            }
        }
        else
        {
            if (m_isPlaying)
            {
                // if we are in trick mode, then resume playback
                if (m_isTrickBackwardMode || m_isTrickForwardMode)
                {
                    IFC(Play());
                    IFC(ResetTrickMode());
                }
                else
                {
                    // User has hit Pause. Media is currently playing or buffering
                    IFC(Pause());
                }
            }
            else
            {
                // User has hit Play
                IFC(Play());
            }
        }

            // Update UIA Name and tooltip for PlayPause button
            IFC(UpdatePlayPauseUI());
        }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::IsMediaStateClosedFromMPE(_Out_ BOOLEAN* value)
{
    HRESULT hr = S_OK;
    *value = FALSE;

    ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
    IFC(GetCurrentPlaybackSession(&spPlaybackSession));
    if (spPlaybackSession)
    {
        wmp::MediaPlaybackState currentState = wmp::MediaPlaybackState_None;
        IFC(spPlaybackSession->get_PlaybackState(&currentState));
        if (currentState == wmp::MediaPlaybackState_None)
        {
            *value = TRUE;
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SetAudioTrackFromMPE(_In_ UINT selectedIndex)
{
    HRESULT hr = S_OK;

    if (m_spMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
        UINT audioStreamCount = 0;

        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            ctl::ComPtr<wfc::IVectorView<wmc::AudioTrack*>> spAudioTracks;
            ctl::ComPtr<wmc::ISingleSelectMediaTrackList> spAudioTrackList;

            IFC(spPlaybackItem->get_AudioTracks(&spAudioTracks));
            IFC(spAudioTracks->get_Size(&audioStreamCount));
            if (audioStreamCount > 0)
            {
                // Apply user's track selection
                IFC(spAudioTracks.As(&spAudioTrackList));
                IFC(spAudioTrackList->put_SelectedIndex(selectedIndex));
            }
        }
    }

Cleanup:
    return hr;
}

// Called when a user selects a track from the CC menu, the old value
// needs to be deselected and the new value selected
_Check_return_ HRESULT
MediaTransportControls::SetCCTrackFromMPE(_In_ UINT selectedIndex)
{
    HRESULT hr = S_OK;

    if (m_spMediaPlayer)
    {
        INT newTrackId = -1;
        newTrackId = m_trackIdMappings[selectedIndex];

        if (m_currentTrack != newTrackId)
        {
            ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;

            if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
            {
                ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
                ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;

                IFC(spPlaybackItem->get_TimedMetadataTracks(&spTextTracks));
                IFC(spTextTracks.As(&spTrackList));

                // Turn off current track, if it exists
                if (m_currentTrack != -1)
                {
                    IFC(spTrackList->SetPresentationMode((unsigned int)m_currentTrack, wmp::TimedMetadataTrackPresentationMode_Hidden));
                }

                // Turn on new track, if it exists
                if (newTrackId != -1)
                {
                    IFC(spTrackList->SetPresentationMode((unsigned int)newTrackId, wmp::TimedMetadataTrackPresentationMode_PlatformPresented));
                }

                m_currentTrack = newTrackId;
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateErrorUIFromMPE()
{
    HRESULT hr = S_OK;
    UINT32 errorResourceID = 0;
    wrl_wrappers::HString strError;

    m_hasError = m_errcodeFromMPE != MF_MEDIA_ENGINE_ERR_NOERROR;

    if (m_hasError)
    {
        IFC(GetErrorResourceID(m_errcodeFromMPE, &errorResourceID));
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(errorResourceID, strError.ReleaseAndGetAddressOf()));
        if (m_tpErrorTextBlock)
        {
            IFC(m_tpErrorTextBlock.Cast<TextBlock>()->put_Text(strError));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpErrorTextBlock.Cast<TextBlock>(), strError));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateAudioSelectionFlyoutFromMPE()
{
    HRESULT hr = S_OK;
    UINT audioStreamCount = 0;
    ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
    ctl::ComPtr<wf::IPropertyValue> spIndexAsPV;
    ctl::ComPtr<wf::IReference<INT>> spIndex;
    int activeAudioStreamIndex = -1;
    ctl::ComPtr<wfc::IVectorView<wmc::AudioTrack*>> spAudioTracks;
    ctl::ComPtr<wmc::ISingleSelectMediaTrackList> spAudioTrackList;
    std::vector<wrl_wrappers::HString> audioTags;

    if (m_spMediaPlayer)
    {
        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            IFC(spPlaybackItem->get_AudioTracks(&spAudioTracks));
            IFC(spAudioTracks->get_Size(&audioStreamCount));
            IFC(spAudioTracks.As(&spAudioTrackList));
            // Get active audio stream so that its language entry  can be marked with "on" suffix
            IFC(spAudioTrackList->get_SelectedIndex(&activeAudioStreamIndex));
        }

        // UpdateAudioSelectionFlyout() is only called when user clicks on Audio Selection
        // button, which should only show up if source has multiple audio streams
        ASSERT(m_hasMultipleAudioStreams = TRUE && audioStreamCount > 1);

        IFC(m_tpAvailableAudioTracksMenuFlyout->get_Items(&spMenuFlyoutItems));

        // Clear all tracks and associated click event handlers
        IFC(spMenuFlyoutItems->Clear());
        ReleaseMenuFlyoutItemClickHandlers();
        (void)CreateAudioTags(spPlaybackItem.Get(), std::move(audioTags));

        for (UINT i=0; i < audioStreamCount; i++)
        {
            wrl_wrappers::HString strLocalizedLanguageName;
            wrl_wrappers::HString strProcessedLanguageName;

            // Wrap i as IReference<INT>
            IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));
            IFC(spPropertyValueFactory->CreateInt32(i, &spIndexAsPV));
            IFC(spIndexAsPV.As<wf::IReference<INT>>(&spIndex));

            strLocalizedLanguageName.Set(audioTags.at(i).Get());

            // Mark active stream with "on" suffix
            if (activeAudioStreamIndex == i)
            {
                IFC(MarkLanguageSelection(strLocalizedLanguageName.Get(), strProcessedLanguageName.ReleaseAndGetAddressOf()));
            }
            else
            {
                *strProcessedLanguageName.ReleaseAndGetAddressOf() = strLocalizedLanguageName.Detach();
            }

            ctl::ComPtr<MenuFlyoutItem> spNewMenuFlyoutItem;
            IFC(ctl::make<MenuFlyoutItem>(&spNewMenuFlyoutItem));

            IFC(spNewMenuFlyoutItem->put_Text(strProcessedLanguageName));
            IFC(spMenuFlyoutItems->Append(spNewMenuFlyoutItem.Get()));

            // Create event handler via EventPtr for this MenuFlyoutItem and add it to m_audioTrackClickHandlers
            EventPtr<MenuFlyoutItemClickEventCallback>* pMenuFlyoutItemClickHandler = new EventPtr<MenuFlyoutItemClickEventCallback>();

            IFC(pMenuFlyoutItemClickHandler->AttachEventHandler(spNewMenuFlyoutItem.Get(),
                [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
                {
                    RRETURN(OnAudioTrackClicked(pSender, pArgs));
                }));

            m_audioTrackClickHandlers.push_back(pMenuFlyoutItemClickHandler);
        }
    }

Cleanup:
    return hr;

}

_Check_return_ HRESULT
MediaTransportControls::UpdateCCSelectionFlyoutFromMPE()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
    unsigned int trackCount = 0;
    wrl_wrappers::HString strOffLabel;
    ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;
    int captionTrackIndexOffset = 0;

    if (m_spMediaPlayer)
    {
        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            IFC(spPlaybackItem->get_TimedMetadataTracks(&spTextTracks));
            IFC(spTextTracks->get_Size(&trackCount));

            ASSERT(m_hasCCTracks == TRUE && trackCount > 0);

            IFC(m_tpAvailableCCTracksMenuFlyout->get_Items(&spMenuFlyoutItems));

            // Clear all tracks and associated click event handlers
            IFC(spMenuFlyoutItems->Clear());
            ReleaseCCSelectionMenuFlyoutItemClickHandlers();
            m_trackIdMappings.clear();
            m_trackIdMappings.reserve(trackCount + 1); // +1 for the "Off" selection

            // Since the visible track can be set programmatically without MTC's involvement
            // we must find the visible text track before we create the flyout items. This is
            // needed to correctly show the (on) flag for that item
            IFC(spTextTracks.As(&spTrackList));
            m_currentTrack = -1;
            for (unsigned int i = 0; i < trackCount; i++)
            {
                ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
                IFC(spTextTracks->GetAt(i, spTrack.GetAddressOf()));

                if (CTimedTextSource::IsSupportedTrack(spTrack.Get(), spPlaybackItem.Get()))
                {
                    wmp::TimedMetadataTrackPresentationMode presentationMode;

                    IFC(spTrackList->GetPresentationMode(i, &presentationMode));
                    if (presentationMode == wmp::TimedMetadataTrackPresentationMode_PlatformPresented)
                    {
                        m_currentTrack = i;
                    }
                }
            }

            if (m_currentTrack >= 0)
            {
                // Only add the "Off" option if a track is selected
                IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_MEDIA_CC_OFF, strOffLabel.ReleaseAndGetAddressOf()));
                IFC(CreateCCFlyoutTrack(strOffLabel, -1, 0));
                captionTrackIndexOffset++;
            }

            for (unsigned int i = 0; i < trackCount; i++)
            {
                ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
                IFC(spTextTracks->GetAt(i, spTrack.GetAddressOf()));

                if (CTimedTextSource::IsSupportedTrack(spTrack.Get(), spPlaybackItem.Get()))
                {
                    ctl::ComPtr<wmc::IMediaTrack> spMediaTrack;
                    wrl_wrappers::HString trackLabel;
                    // TODO identify label based on the existing tracks.
                    IFC(spTrack.As(&spMediaTrack));
                    IFC(spMediaTrack->get_Label(trackLabel.GetAddressOf()));
                    if (!trackLabel)
                    {
                        // a label was not set, use the language string
                        IFC(spMediaTrack->get_Language(trackLabel.GetAddressOf()));
                        if (!trackLabel)
                        {
                            // if no label or language, mark as "untitled"
                            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_MEDIA_AUDIO_TRACK_UNTITLED, trackLabel.ReleaseAndGetAddressOf()));
                        }
                    }

                    IFC(CreateCCFlyoutTrack(trackLabel.Get(), i, captionTrackIndexOffset++));
                }
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SubscribeMediaPlayerEvents() noexcept
{
    if (m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
        ctl::ComPtr<wmp::IMediaBreakManager> spBreakManager;
        ctl::ComPtr<wmp::IMediaPlaybackSession> spBreakPlaybackSession;
        ctl::WeakRefPtr wrThis;

        // Using weak reference in event handler callback to make
        // sure this pointer still exist. If weak reference as unreachable we won?t call into it
        // as resolving weak reference returns NULL.
        IFC_RETURN(ctl::AsWeak(this, &wrThis));
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(spMediaPlayerExt->get_BreakManager(&spBreakManager));
        IFC_RETURN(spBreakManager->get_PlaybackSession(&spBreakPlaybackSession));

        IFC_RETURN(m_spMediaPlayer->add_MediaOpened(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_MediaOpened)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerMediaOpenedToken));

        IFC_RETURN(spMediaPlayerExt->add_SourceChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_Source)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerSourceChangeToken));

        IFC_RETURN(spMediaPlayerExt->add_IsMutedChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_Mute)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerMuteChangeToken));

        IFC_RETURN(m_spMediaPlayer->add_VolumeChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_Volume)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerVolumeChangeToken));

        IFC_RETURN(spPlaybackSession->add_DownloadProgressChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_DownloadProgress)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerDownloadProgressChangeToken));

        IFC_RETURN(spPlaybackSession->add_PlaybackStateChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_CurrentState)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerCurrentStateChangeToken));

        IFC_RETURN(spPlaybackSession->add_NaturalDurationChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_NaturalDuration)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerNaturalDurationChangeToken));

        IFC_RETURN(spPlaybackSession->add_PlaybackRateChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_PlaybackRate)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerPlaybackRateChangeToken));

        IFC_RETURN(m_spMediaPlayer->add_MediaFailed(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, wmp::MediaPlayerFailedEventArgs*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, wmp::IMediaPlayerFailedEventArgs *pArgs) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::HandleMediaFailed, ctl::ComPtr<wmp::IMediaPlayerFailedEventArgs>(pArgs))));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerMediaFailedToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPlayCommandBehaviour;
        IFC_RETURN(spCommandManager->get_PlayBehavior(&spPlayCommandBehaviour));

        IFC_RETURN(spPlayCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdatePlayPauseUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerPlayBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPauseCommandBehaviour;
        IFC_RETURN(spCommandManager->get_PauseBehavior(&spPauseCommandBehaviour));
        IFC_RETURN(spPauseCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdatePlayPauseUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerPauseBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spNextCommandBehaviour;
        IFC_RETURN(spCommandManager->get_NextBehavior(&spNextCommandBehaviour));
        IFC_RETURN(spNextCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateTracksUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerNextBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPrevCommandBehaviour;
        IFC_RETURN(spCommandManager->get_PreviousBehavior(&spPrevCommandBehaviour));
        IFC_RETURN(spPrevCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateTracksUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerPreviousBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spFastforwardCommandBehaviour;
        IFC_RETURN(spCommandManager->get_FastForwardBehavior(&spFastforwardCommandBehaviour));
        IFC_RETURN(spFastforwardCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateTrickModeFallbackUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerFastForwardBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRewindCommandBehaviour;
        IFC_RETURN(spCommandManager->get_RewindBehavior(&spRewindCommandBehaviour));
        IFC_RETURN(spRewindCommandBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateTrickModeFallbackUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerRewindBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPositionBehaviour;
        IFC_RETURN(spCommandManager->get_PositionBehavior(&spPositionBehaviour));
        IFC_RETURN(spPositionBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateSeekPositionsUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerPositionBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRateBehaviour;
        IFC_RETURN(spCommandManager->get_RateBehavior(&spRateBehaviour));
        IFC_RETURN(spRateBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdatePlaybackRateUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerRateBehaviorChangeToken));

        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spAutoRepeatBehaviour;
        IFC_RETURN(spCommandManager->get_AutoRepeatModeBehavior(&spAutoRepeatBehaviour));
        IFC_RETURN(spAutoRepeatBehaviour->add_IsEnabledChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackCommandManagerCommandBehavior*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackCommandManagerCommandBehavior*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateRepeatButtonUI)));
            }
            return S_OK;
        }).Get(), &m_cmdManagerAutoRepeatBehaviorChangeToken));

        // Subscribe Break Events
        IFC_RETURN(spBreakManager->add_BreakStarted(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaBreakManager*, wmp::MediaBreakStartedEventArgs*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaBreakManager*, wmp::IMediaBreakStartedEventArgs*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateBreakStatus, true)));
            }
            return S_OK;
        }).Get(), &m_brkManagerBreakStartToken));

        IFC_RETURN(spBreakManager->add_BreakEnded(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaBreakManager*, wmp::MediaBreakEndedEventArgs*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaBreakManager*, wmp::IMediaBreakEndedEventArgs*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateBreakStatus, false)));
            }
            return S_OK;
        }).Get(), &m_brkManagerBreakEndToken));

        // Media Break Events
        IFC_RETURN(spBreakManager->add_BreakSkipped(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaBreakManager*, wmp::MediaBreakSkippedEventArgs*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaBreakManager*, wmp::IMediaBreakSkippedEventArgs*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get() && spThis->m_transportControlsEnabled)
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::UpdateBreakStatus, false)));
            }
            return S_OK;
        }).Get(), &m_brkManagerBreakSkippedToken));

        IFC_RETURN(spBreakPlaybackSession->add_PlaybackStateChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_MediaBreak_CurrentState)));
            }
            return S_OK;
        }).Get(), &m_mediaBreakCurrentStateChangeToken));

        IFC_RETURN(spBreakPlaybackSession->add_PositionChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_MediaBreak_Position)));
            }
            return S_OK;
        }).Get(), &m_mediaBreakPositionChangeToken));

        IFC_RETURN(spBreakPlaybackSession->add_DownloadProgressChanged(
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlaybackSession*, IInspectable*) mutable -> HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_MediaBreak_DownloadProgress)));
            }
            return S_OK;
        }).Get(), &m_mediaBreakDownloadProgressChangeToken));

        IFC_RETURN(SubscribeTrackEvents());

        IFC_RETURN(MediaPlayerExtension_AddIsLoopingEnabledChanged(
            m_spMediaPlayer.Get(),
            wrl::Callback< Microsoft::WRL::Implements<
            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            Microsoft::WRL::FtmBase >> (
            [wrThis](wmp::IMediaPlayer*, IInspectable*) mutable->HRESULT
        {
            ctl::ComPtr<MediaTransportControls> spThis;
            IFC_RETURN(wrThis.As(&spThis));
            if (spThis.Get())
            {
                IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                    spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_Repeat)));
            }
            return S_OK;
        }).Get(), &m_mediaPlayerIsLoopingEnabledToken));

        m_isMediaPlayerSubscribed = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UnSubscribeMediaPlayerEvents() noexcept
{
    if (m_spMediaPlayer.Get())
    {
        if (m_mediaPlayerMediaOpenedToken.value > 0)
        {
            IFC_RETURN(m_spMediaPlayer->remove_MediaOpened(m_mediaPlayerMediaOpenedToken));
            m_mediaPlayerMediaOpenedToken.value = 0;
        }

        if (m_mediaPlayerMediaFailedToken.value > 0)
        {
            IFC_RETURN(m_spMediaPlayer->remove_MediaFailed(m_mediaPlayerMediaFailedToken));
            m_mediaPlayerMediaFailedToken.value = 0;
        }

        if (m_mediaPlayerVolumeChangeToken.value > 0)
        {
            IFC_RETURN(m_spMediaPlayer->remove_VolumeChanged(m_mediaPlayerVolumeChangeToken));
            m_mediaPlayerVolumeChangeToken.value = 0;
        }

        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));

        if (m_mediaPlayerMuteChangeToken.value > 0)
        {
            IFC_RETURN(spMediaPlayerExt->remove_IsMutedChanged(m_mediaPlayerMuteChangeToken));
            m_mediaPlayerMuteChangeToken.value = 0;
        }

        if (m_mediaPlayerSourceChangeToken.value > 0)
        {
            IFC_RETURN(spMediaPlayerExt->remove_SourceChanged(m_mediaPlayerSourceChangeToken));
            m_mediaPlayerSourceChangeToken.value = 0;
        }

        if (m_mediaPlayerDownloadProgressChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));

            IFC_RETURN(spPlaybackSession->remove_DownloadProgressChanged(m_mediaPlayerDownloadProgressChangeToken));
            m_mediaPlayerDownloadProgressChangeToken.value = 0;
        }

        if (m_mediaPlayerCurrentStateChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));

            IFC_RETURN(spPlaybackSession->remove_PlaybackStateChanged(m_mediaPlayerCurrentStateChangeToken));
            m_mediaPlayerCurrentStateChangeToken.value = 0;
        }

        if (m_mediaPlayerNaturalDurationChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));

            IFC_RETURN(spPlaybackSession->remove_NaturalDurationChanged(m_mediaPlayerNaturalDurationChangeToken));
            m_mediaPlayerNaturalDurationChangeToken.value = 0;
        }

        if (m_mediaPlayerPlaybackRateChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));

            IFC_RETURN(spPlaybackSession->remove_PlaybackRateChanged(m_mediaPlayerPlaybackRateChangeToken));
            m_mediaPlayerPlaybackRateChangeToken.value = 0;
        }

        ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));

        if (m_cmdManagerPlayBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPlayCommandBehaviour;
            IFC_RETURN(spCommandManager->get_PlayBehavior(&spPlayCommandBehaviour));

            IFC_RETURN(spPlayCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerPlayBehaviorChangeToken));
            m_cmdManagerPlayBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerPauseBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPauseCommandBehaviour;
            IFC_RETURN(spCommandManager->get_PauseBehavior(&spPauseCommandBehaviour));

            IFC_RETURN(spPauseCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerPauseBehaviorChangeToken));
            m_cmdManagerPauseBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerNextBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spNextCommandBehaviour;
            IFC_RETURN(spCommandManager->get_NextBehavior(&spNextCommandBehaviour));

            IFC_RETURN(spNextCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerNextBehaviorChangeToken));
            m_cmdManagerNextBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerPreviousBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPrevCommandBehaviour;
            IFC_RETURN(spCommandManager->get_PreviousBehavior(&spPrevCommandBehaviour));

            IFC_RETURN(spPrevCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerPreviousBehaviorChangeToken));
            m_cmdManagerPreviousBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerFastForwardBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spFastforwardCommandBehaviour;
            IFC_RETURN(spCommandManager->get_FastForwardBehavior(&spFastforwardCommandBehaviour));

            IFC_RETURN(spFastforwardCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerFastForwardBehaviorChangeToken));
            m_cmdManagerFastForwardBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerRewindBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRewindCommandBehaviour;
            IFC_RETURN(spCommandManager->get_RewindBehavior(&spRewindCommandBehaviour));

            IFC_RETURN(spRewindCommandBehaviour->remove_IsEnabledChanged(m_cmdManagerRewindBehaviorChangeToken));
            m_cmdManagerRewindBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerPositionBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPositionBehaviour;
            IFC_RETURN(spCommandManager->get_PositionBehavior(&spPositionBehaviour));

            IFC_RETURN(spPositionBehaviour->remove_IsEnabledChanged(m_cmdManagerPositionBehaviorChangeToken));
            m_cmdManagerPositionBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerRateBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRateBehaviour;
            IFC_RETURN(spCommandManager->get_RateBehavior(&spRateBehaviour));

            IFC_RETURN(spRateBehaviour->remove_IsEnabledChanged(m_cmdManagerRateBehaviorChangeToken));
            m_cmdManagerRateBehaviorChangeToken.value = 0;
        }

        if (m_cmdManagerAutoRepeatBehaviorChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spAutoRepeatBehaviour;
            IFC_RETURN(spCommandManager->get_AutoRepeatModeBehavior(&spAutoRepeatBehaviour));

            IFC_RETURN(spAutoRepeatBehaviour->remove_IsEnabledChanged(m_cmdManagerAutoRepeatBehaviorChangeToken));
            m_cmdManagerAutoRepeatBehaviorChangeToken.value = 0;
        }

        ctl::ComPtr<wmp::IMediaBreakManager> spBreakManager;
        IFC_RETURN(spMediaPlayerExt->get_BreakManager(&spBreakManager));

        if (m_brkManagerBreakStartToken.value > 0)
        {
            IFC_RETURN(spBreakManager->remove_BreakStarted(m_brkManagerBreakStartToken));
            m_brkManagerBreakStartToken.value = 0;
        }

        if (m_brkManagerBreakEndToken.value > 0)
        {
            IFC_RETURN(spBreakManager->remove_BreakEnded(m_brkManagerBreakEndToken));
            m_brkManagerBreakEndToken.value = 0;
        }

        if (m_brkManagerBreakSkippedToken.value > 0)
        {
            IFC_RETURN(spBreakManager->remove_BreakSkipped(m_brkManagerBreakSkippedToken));
            m_brkManagerBreakSkippedToken.value = 0;
        }

        if (m_mediaBreakCurrentStateChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spBreakPlaybackSession;
            IFC_RETURN(spBreakManager->get_PlaybackSession(&spBreakPlaybackSession));

            IFC_RETURN(spBreakPlaybackSession->remove_PlaybackStateChanged(m_mediaBreakCurrentStateChangeToken));
            m_mediaBreakCurrentStateChangeToken.value = 0;
        }

        if (m_mediaBreakPositionChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spBreakPlaybackSession;
            IFC_RETURN(spBreakManager->get_PlaybackSession(&spBreakPlaybackSession));

            IFC_RETURN(spBreakPlaybackSession->remove_PositionChanged(m_mediaBreakPositionChangeToken));
            m_mediaBreakPositionChangeToken.value = 0;
        }

        if (m_mediaBreakDownloadProgressChangeToken.value > 0)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spBreakPlaybackSession;
            IFC_RETURN(spBreakManager->get_PlaybackSession(&spBreakPlaybackSession));

            IFC_RETURN(spBreakPlaybackSession->remove_DownloadProgressChanged(m_mediaBreakDownloadProgressChangeToken));
            m_mediaBreakDownloadProgressChangeToken.value = 0;
        }

        IFC_RETURN(UnSubscribeTrackEvents());
        IFC_RETURN(UnSubscribeBreakListEvents());

        if (m_mediaPlayerIsLoopingEnabledToken.value > 0)
        {
            IFC_RETURN(MediaPlayerExtension_RemoveIsLoopingEnabledChanged(m_spMediaPlayer.Get(), m_mediaPlayerIsLoopingEnabledToken));
            m_mediaPlayerIsLoopingEnabledToken.value = 0;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SubscribeTrackEvents()
{
    if (m_spMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlaybackList> spMediaPlaybackList;
        ctl::ComPtr<wmp::IMediaPlaybackSource> spMediaPlaybackSource;
        ctl::ComPtr<wmp::IMediaPlayerSource2> spMediaPlayerSource;
        ctl::WeakRefPtr wrThis;

        // Using weak reference in event handler callback to make
        // sure "this" pointer still exist. If weak reference as unreachable we skip it
        // as resolving weak reference returns NULL.
        IFC_RETURN(ctl::AsWeak(this, &wrThis));
        // Take reference to PlayList for Subscribing and Unsubscribing Track Events.
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerSource));
        IFC_RETURN(spMediaPlayerSource->get_Source(&spMediaPlaybackSource));
        if (spMediaPlaybackSource)
        {
            if SUCCEEDED(spMediaPlaybackSource.As(&spMediaPlaybackList))
            {
                m_spMediaPlaybackList = spMediaPlaybackList;
            }
        }

        if (m_spMediaPlaybackList.Get())
        {
            IFC_RETURN(m_spMediaPlaybackList->add_CurrentItemChanged(
                wrl::Callback< Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::ITypedEventHandler<wmp::MediaPlaybackList*, wmp::CurrentMediaPlaybackItemChangedEventArgs*>,
                Microsoft::WRL::FtmBase >> (
                [wrThis](wmp::IMediaPlaybackList*, wmp::ICurrentMediaPlaybackItemChangedEventArgs*) mutable -> HRESULT
            {
                ctl::ComPtr<MediaTransportControls> spThis;
                IFC_RETURN(wrThis.As(&spThis));
                if (spThis.Get())
                {
                    IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                        spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_ItemChanged)));
                }
                return S_OK;
            }).Get(), &m_mediaPlayerItemChangedToken));

            IFC_RETURN(m_spMediaPlaybackList->add_ItemFailed(
                wrl::Callback< Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::ITypedEventHandler<wmp::MediaPlaybackList*, wmp::MediaPlaybackItemFailedEventArgs*>,
                Microsoft::WRL::FtmBase >> (
                [wrThis](wmp::IMediaPlaybackList*, wmp::IMediaPlaybackItemFailedEventArgs* pArgs) mutable -> HRESULT
            {
                ctl::ComPtr<MediaTransportControls> spThis;
                IFC_RETURN(wrThis.As(&spThis));
                if (spThis.Get())
                {
                    IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                        spThis, &MediaTransportControls::HandleItemMediaFailed, ctl::ComPtr<wmp::IMediaPlaybackItemFailedEventArgs>(pArgs))));
                }
                return S_OK;
            }).Get(), &m_mediaPlayerItemFailedToken));

            IFC_RETURN(MediaPlaybackListExtension_AddAutoRepeatChanged(
                m_spMediaPlaybackList.Get(),
                wrl::Callback< Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::ITypedEventHandler<wmp::MediaPlaybackList*, IInspectable*>,
                Microsoft::WRL::FtmBase >> (
                [wrThis](wmp::IMediaPlaybackList*, IInspectable*) mutable -> HRESULT
            {
                ctl::ComPtr<MediaTransportControls> spThis;
                IFC_RETURN(wrThis.As(&spThis));
                if (spThis.Get())
                {
                    IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                        spThis, &MediaTransportControls::OnMediaPropertyChanged, MediaPlayer_Repeat)));
                }
                return S_OK;
            }).Get(), &m_mediaPlayerAutoRepeatChangedToken));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UnSubscribeTrackEvents()
{
    if (m_spMediaPlaybackList.Get())
    {
        if (m_mediaPlayerItemChangedToken.value > 0)
        {
            IFC_RETURN(m_spMediaPlaybackList->remove_CurrentItemChanged(m_mediaPlayerItemChangedToken));
            m_mediaPlayerItemChangedToken.value = 0;
        }

        if (m_mediaPlayerItemFailedToken.value > 0)
        {
            IFC_RETURN(m_spMediaPlaybackList->remove_ItemFailed(m_mediaPlayerItemFailedToken));
            m_mediaPlayerItemFailedToken.value = 0;
        }

        if (m_mediaPlayerAutoRepeatChangedToken.value > 0)
        {
            IFC_RETURN(MediaPlaybackListExtension_RemoveAutoRepeatChanged(m_spMediaPlaybackList.Get(), m_mediaPlayerAutoRepeatChangedToken));
            m_mediaPlayerAutoRepeatChangedToken.value = 0;
        }

        // release the reference
        m_spMediaPlaybackList = nullptr;
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SubscribeBreakListEvents()
{

    IFC_RETURN(UnSubscribeBreakListEvents());
    if (m_spMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
        ctl::ComPtr<wmp::IMediaBreakManager> spBreakManager;
        ctl::ComPtr<wmp::IMediaBreak> spCurrentBreak;
        ctl::ComPtr<wmp::IMediaPlaybackList> spMediaBreakPlaybackList;

        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_BreakManager(&spBreakManager));
        IFC_RETURN(spBreakManager->get_CurrentBreak(&spCurrentBreak));

        if (spCurrentBreak)
        {
            IFC_RETURN(spCurrentBreak->get_PlaybackList(&spMediaBreakPlaybackList));
        }

        if (spMediaBreakPlaybackList)
        {
            ctl::WeakRefPtr wrThis;

            // Using weak reference in event handler callback to make
            // sure this pointer still exist. If weak reference as unreachable we won?t call into it
            // as resolving weak reference returns NULL.
            IFC_RETURN(ctl::AsWeak(this, &wrThis));
            m_spMediaBreakPlaybackList = spMediaBreakPlaybackList;

            IFC_RETURN(m_spMediaBreakPlaybackList->add_CurrentItemChanged(
                wrl::Callback< Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::ITypedEventHandler<wmp::MediaPlaybackList*, wmp::CurrentMediaPlaybackItemChangedEventArgs*>,
                Microsoft::WRL::FtmBase >> (
                [wrThis](wmp::IMediaPlaybackList*, wmp::ICurrentMediaPlaybackItemChangedEventArgs*) mutable -> HRESULT
            {
                ctl::ComPtr<MediaTransportControls> spThis;
                IFC_RETURN(wrThis.As(&spThis));
                if (spThis.Get() && spThis->m_transportControlsEnabled)
                {
                    IFC_RETURN(spThis->GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                        spThis, &MediaTransportControls::InitializeVisualState)));
                }
                return S_OK;
            }).Get(), &m_mediaPlayerBreakItemChangedToken));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UnSubscribeBreakListEvents()
{
    if (m_spMediaBreakPlaybackList.Get())
    {
        if (m_mediaPlayerBreakItemChangedToken.value > 0)
        {
            IFC_RETURN(m_spMediaBreakPlaybackList->remove_CurrentItemChanged(m_mediaPlayerBreakItemChangedToken));
            m_mediaPlayerBreakItemChangedToken.value = 0;
        }

        // release the reference
        m_spMediaBreakPlaybackList = nullptr;
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnMediaPropertyChanged(_In_ MediaPlayer_Property propertyIndex)
{
    HRESULT hr = S_OK;
    BOOLEAN previousIsPlaying = m_isPlaying;
    BOOLEAN previousIsBuffering = m_isBuffering;

    if (m_transportControlsEnabled)
    {
        switch (propertyIndex)
        {
            case MediaPlayer_Property::MediaPlayer_MediaOpened:
            {
                m_errcodeFromMPE = MF_MEDIA_ENGINE_ERR_NOERROR;
                InitializeVisualState();
                IFC(ResetTrickMode());
                IFC(GetPlaybackRate(&m_orginalPlaybackRate));
                break;
            }

            case MediaPlayer_Property::MediaPlayer_Position:
            case MediaPlayer_Property::MediaPlayer_MediaBreak_Position:
            {
                // This events received continuously while video playbacks, but we need update UI only when Control Panel Visible.
                if (m_controlPanelIsVisible)
                {
                    IFC(UpdatePositionUI());
                }
                break;
            }

            case MediaPlayer_Property::MediaPlayer_Mute:
            {
                IFC(UpdateIsMutedUI());
                IFC(UpdateVisualState());
                break;
            }

            case MediaPlayer_Property::MediaPlayer_Repeat:
            {
                IFC(UpdateRepeatButtonUI());
                IFC(UpdateVisualState());
                break;
            }

            case MediaPlayer_Property::MediaPlayer_Volume:
            {
                IFC(UpdateVolumeUI());
                break;
            }

            case MediaPlayer_Property::MediaPlayer_DownloadProgress:
            case MediaPlayer_Property::MediaPlayer_MediaBreak_DownloadProgress:
            {
                // This events received continuously while video playbacks, but we need update UI only when Control Panel Visible.
                if (m_controlPanelIsVisible)
                {
                    // Update indicator since the % downloaded has changed
                    IFC(UpdateDownloadProgressUI());
                }
                break;
            }

            case MediaPlayer_Property::MediaPlayer_CurrentState:
            case MediaPlayer_Property::MediaPlayer_MediaBreak_CurrentState:
            {
                ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
                IFC(GetCurrentPlaybackSession(&spPlaybackSession));
                if (spPlaybackSession)
                {
                    wmp::MediaPlaybackState currentState = wmp::MediaPlaybackState_None;
                    IFC(spPlaybackSession->get_PlaybackState(&currentState));
                    m_sourceLoaded = currentState != wmp::MediaPlaybackState_None &&
                        currentState != wmp::MediaPlaybackState_Opening;
                    if (currentState != wmp::MediaPlaybackState_Buffering)
                    {
                        m_isPlaying = currentState == wmp::MediaPlaybackState_Playing;
                    }

                    m_isBuffering = currentState == wmp::MediaPlaybackState_Buffering;

                    // If playing state changed, toggle position timer
                    // to avoid ticking if position is not updating
                    if (previousIsPlaying != m_isPlaying)
                    {
                        if (m_isPlaying)
                        {
                            IFC(StartPositionUpdateTimer());
                        }
                        else
                        {
                            IFC(StopPositionUpdateTimer());
                        }
                    }

                    IFC(UpdatePlayPauseUI());

                    if ((previousIsPlaying != m_isPlaying || previousIsBuffering != m_isBuffering) && !m_isInScrubMode)
                    {
                        if ((m_isPlaying && !m_isBuffering || m_shouldDismissControlPanel) && !m_isthruScrubber)
                        {
                            m_shouldDismissControlPanel = TRUE;
                            IFC(HideControlPanel());
                        }
                        else
                        {
                            IFC(ShowControlPanel());
                        }
                    }
                    if (!m_isPlaying)
                    {
                        IFC(ResetTrickMode());
                    }
                    // Timing issues still Natural duration values zero even after source loaded.
                    if (m_sourceLoaded && m_naturalDuration.TimeSpan.Duration == 0)
                    {
                        wf::TimeSpan value;
                        IFC(spPlaybackSession->get_NaturalDuration(&value));
                        m_naturalDuration.TimeSpan = value;
                    }
                    IFC(UpdateVisualState());
                }
                break;
            }

            case MediaPlayer_Property::MediaPlayer_NaturalDuration:
            {
                ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
                IFC(GetCurrentPlaybackSession(&spPlaybackSession));
                if (spPlaybackSession)
                {
                    wf::TimeSpan value;
                    IFC(spPlaybackSession->get_NaturalDuration(&value));
                    m_naturalDuration.TimeSpan = value;
                }
                IFC(UpdatePositionUI());
                IFC(UpdateCCSelectionUI());
                break;
            }
            case MediaPlayer_Property::MediaPlayer_Source:
            {
                // Release the existing Playlist subscribers if exist.
                m_isBreakPlaying = FALSE;
                IFC(ResetTrickMode());
                IFC(UnSubscribeTrackEvents());
                IFC(UnSubscribeBreakListEvents());
                IFC(SubscribeTrackEvents());
                IFC(InitializeVisualState());
                break;
            }
            case MediaPlayer_Property::MediaPlayer_ItemChanged:
            {
                m_errcodeFromMPE = MF_MEDIA_ENGINE_ERR_NOERROR;
                m_isBreakPlaying = FALSE;
                IFC(InitializeVisualState());
                IFC(ResetTrickMode());
                IFC(GetPlaybackRate(&m_orginalPlaybackRate));
                break;
            }

            case MediaPlayer_Property::MediaPlayer_PlaybackRate:
            {
                if (m_isTrickBackwardMode || m_isTrickForwardMode)
                {
                    double playbackRate;
                    IFC(GetPlaybackRate(&playbackRate));
                    // Back to Original speed, so reset Trick mode.
                    if (playbackRate == m_orginalPlaybackRate)
                    {
                        IFC(ResetTrickMode());
                    }
                    else
                    {
                        IFC(UpdateTrickModeButton(playbackRate));
                    }
                }
                else
                {
                    IFC(GetPlaybackRate(&m_orginalPlaybackRate));
                }
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::HandleMediaFailed(_In_ wmp::IMediaPlayerFailedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wmp::IMediaPlayerFailedEventArgs> spArgs = pArgs;
    if (m_transportControlsEnabled && spArgs)
    {
        wmp::MediaPlayerError errorMediaPlayer;
        IFC(spArgs->get_Error(&errorMediaPlayer));
        m_errcodeFromMPE = GetMediaEngineErrorCode(errorMediaPlayer);
        IFC(UpdateErrorUI());
        IFC(UpdateVisualState());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::HandleItemMediaFailed(_In_ wmp::IMediaPlaybackItemFailedEventArgs *pArgs)
{
    ctl::ComPtr<wmp::IMediaPlaybackItemFailedEventArgs> spArgs = pArgs;
    ctl::ComPtr<wmp::IMediaPlaybackItemError> spMediaItemError;
    if (m_transportControlsEnabled && spArgs)
    {
        wmp::MediaPlaybackItemErrorCode errorMediaPlaybackItem;
        IFC_RETURN(spArgs->get_Error(&spMediaItemError));
        IFC_RETURN(spMediaItemError->get_ErrorCode(&errorMediaPlaybackItem));
        m_errcodeFromMPE = GetMediaEngineErrorCode(errorMediaPlaybackItem);
        IFC_RETURN(UpdateErrorUI());
        IFC_RETURN(UpdateVisualState());
    }
    return S_OK;
}

UINT32
MediaTransportControls::GetMediaEngineErrorCode(_In_ wmp::MediaPlayerError errorMediaPlayer)
{
    switch (errorMediaPlayer)
    {
        case wmp::MediaPlayerError_Aborted:
            return MF_MEDIA_ENGINE_ERR_ABORTED;
        case wmp::MediaPlayerError_NetworkError:
            return MF_MEDIA_ENGINE_ERR_NETWORK;
        case wmp::MediaPlayerError_DecodingError:
            return MF_MEDIA_ENGINE_ERR_DECODE;
        case wmp::MediaPlayerError_SourceNotSupported:
            return MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED;
        default:
            return MF_MEDIA_ENGINE_ERR_ENCRYPTED;
    }
}

UINT32
MediaTransportControls::GetMediaEngineErrorCode(_In_ wmp::MediaPlaybackItemErrorCode errorMediaPlaybackItem)
{
    switch (errorMediaPlaybackItem)
    {
    case wmp::MediaPlaybackItemErrorCode_Aborted:
        return MF_MEDIA_ENGINE_ERR_ABORTED;
    case wmp::MediaPlaybackItemErrorCode_NetworkError:
        return MF_MEDIA_ENGINE_ERR_NETWORK;
    case wmp::MediaPlaybackItemErrorCode_DecodeError:
        return MF_MEDIA_ENGINE_ERR_DECODE;
    case wmp::MediaPlaybackItemErrorCode_SourceNotSupportedError:
        return MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED;
    default:
        return MF_MEDIA_ENGINE_ERR_ENCRYPTED;
    }
}

//  Synopsis:
//  Update the UI in response to property change of owner MediaPlayerElement.
_Check_return_ HRESULT
MediaTransportControls::OnOwnerMPEPropertyChanged(_In_ KnownPropertyIndex propertyIndex)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_controls::IMediaPlayerElement> spOwnerMPE;
    ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer;

    switch (propertyIndex)
    {
#if false // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaPlayerElement_IsFullWindow:
        {
            if (m_isMiniViewClicked)
            {
                // Called by clicking MiniView Button
                IFC(UpdateMiniViewUI());
                m_isMiniViewClicked = FALSE;
            }
            else
            {
                IFC(UpdateFullWindowUI());
                // Update Safe Margin for xbox scenario in useVisible desired mode
                IFC(UpdateSafeMarginsinFullWindow(m_isFullWindow ? true : false));
            }
            IFC(UpdateVisualState());
            break;
        }
#endif // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaPlayerElement_MediaPlayer:
        {
            IFC(m_wrOwnerParent.As(&spOwnerMPE))
            if (spOwnerMPE.Get())
            {
                IFC(spOwnerMPE->get_MediaPlayer(&spMediaPlayer));
                if (m_spMediaPlayer != spMediaPlayer)
                {
                    if (m_isMediaPlayerSubscribed)
                    {
                        // Unregister MediaPlayer notification
                        IFC(UnSubscribeMediaPlayerEvents());
                        m_isMediaPlayerSubscribed = FALSE;
                    }
                    // set reference to MediaPlayer
                    m_spMediaPlayer = spMediaPlayer;
                    // delay subscription till template applied.
                    if (m_isTemplateApplied)
                    {
                        IFC(SubscribeMediaPlayerEvents());
                        m_isMediaPlayerSubscribed = TRUE;
                    }
                }
                IFC(InitializeVisualState());
            }
            break;
        }
    }
Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::TrickModeForward()
{
    if (m_spMediaPlayer && m_isPlaying)
    {
        BOOLEAN isSupport = TRUE;
        double playbackRate;

        if (!m_isTrickForwardMode)
        {
            IFC_RETURN(GetPlaybackRate(&playbackRate));
            // We need to check only when original playrate, in other case like already in rewind mode, FF makes decrement the rate.
            if (playbackRate == m_orginalPlaybackRate)
            {
                // Check Minimum playback rate supports
                IFC_RETURN(IsPlaybackRateSupported(MinSupportedPlayRate, &isSupport));
            }
        }
        // We always send the FastForward Command to Command Manager, if trick mode doesn't support, it will do nothing but CommandManager invokes app handlers
        // for FastForward
        ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
        IFC_RETURN(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
        if (spMediaPlayer2)
        {
            IFC_RETURN(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(
                spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_FastForward));
            m_isTrickForwardMode = TRUE;
        }

        // if trick mode doesn't support we fallback to Skip
        if (!isSupport)
        {
            // Still we are not trick mode so we fallback to Skip
            IFC_RETURN(SkipForward());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::TrickModeBackward()
{
    if (m_spMediaPlayer && m_isPlaying)
    {
        BOOLEAN isSupport = TRUE;
        double playbackRate;

        if (!m_isTrickBackwardMode)  //Already in Rewind mode, no need to check
        {
            IFC_RETURN(GetPlaybackRate(&playbackRate));
            // We need to check only when original playrate, in other cases like already in FF mode, rewind makes decrement the rate.
            if (playbackRate == m_orginalPlaybackRate)
            {
                // Check Minimum playback rate supports
                IFC_RETURN(IsPlaybackRateSupported((MinSupportedPlayRate*(-1)), &isSupport));
            }
        }

        // We always send the Rewind Command to Command Manager, if trick mode doesn't support, it will do nothing but CommandManager invokes app handlers
        // for rewind
        ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
        IFC_RETURN(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
        if (spMediaPlayer2)
        {
            IFC_RETURN(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(
                spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_Rewind));
            m_isTrickBackwardMode = TRUE;
        }

        // if trick mode doesn't support we fallback to Skip
        if (!isSupport)
        {
            // Still we are not trick mode so we fallback to Skip
            IFC_RETURN(SkipBackward());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::ResetTrickMode()
{
    if (m_isTrickBackwardMode || m_isTrickForwardMode)
    {
        IFC_RETURN(UpdateTrickModeButton(m_orginalPlaybackRate));
        m_isTrickBackwardMode = FALSE;
        m_isTrickForwardMode = FALSE;
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateTrickModeFallbackUI()
{
    xaml_media::FastPlayFallbackBehaviour value;
    BOOLEAN ffVisible = FALSE;
    BOOLEAN frVisible = FALSE;
    BOOLEAN ffenable = FALSE;
    BOOLEAN rfenable = FALSE;
    BOOLEAN isSupport = FALSE;
    ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spFastforwardCommandBehaviour;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRewindCommandBehaviour;

    if (m_spMediaPlayer)
    {
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(get_IsFastForwardButtonVisible(&ffVisible));
        IFC_RETURN(get_IsFastRewindButtonVisible(&frVisible));
        IFC_RETURN(get_IsFastForwardEnabled(&ffenable));
        IFC_RETURN(get_IsFastRewindEnabled(&rfenable));
        IFC_RETURN(get_FastPlayFallbackBehaviour(&value));

        if (ffVisible)
        {
            BOOLEAN isFFEnabled = FALSE;
            IFC_RETURN(spCommandManager->get_FastForwardBehavior(&spFastforwardCommandBehaviour));
            IFC_RETURN(spFastforwardCommandBehaviour->get_IsEnabled(&isFFEnabled));
            // Check Minimum playback rate supports
            IFC_RETURN(IsPlaybackRateSupported(MinSupportedPlayRate, &isSupport));
            // Only if playback not supports then fallback based on the API
            if (!isSupport)
            {
                if (value == xaml_media::FastPlayFallbackBehaviour::FastPlayFallbackBehaviour_Disable)
                {
                    if (m_tpFastForwardButton)
                    {
                        IFC_RETURN(m_tpFastForwardButton.Cast<Button>()->put_IsEnabled(FALSE));
                    }
                }
                if (value == xaml_media::FastPlayFallbackBehaviour::FastPlayFallbackBehaviour_Hide)
                {
                    if (m_tpFastForwardButton)
                    {
                        IFC_RETURN(m_tpFastForwardButton.Cast<Button>()->put_Visibility(xaml::Visibility_Collapsed));
                    }
                }
                else
                {
                    if (m_tpFastForwardButton)
                    {
                        // 1. FF API enable 2. MediaBreak is not playing.
                        IFC_RETURN(m_tpFastForwardButton.Cast<Button>()->put_IsEnabled(ffenable && !m_isBreakPlaying));
                    }
                }
            }
            else
            {
                if (m_tpFastForwardButton)
                {
                    IFC_RETURN(m_tpFastForwardButton.Cast<Button>()->put_Visibility(
                        (ffVisible) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
                    // Command Manager FastForward Behaviour always override the MTC developer API option.
                    // It should enable only all condition are true:
                    // 1. FF enable on Command Manager 2. FF API enable 3. MediaBreak is not playing.
                    IFC_RETURN(m_tpFastForwardButton.Cast<Button>()->put_IsEnabled(isFFEnabled && ffenable && !m_isBreakPlaying));
                }
            }
        }

        if (frVisible)
        {
            BOOLEAN isRewindEnabled = FALSE;
            IFC_RETURN(spCommandManager->get_RewindBehavior(&spRewindCommandBehaviour));
            IFC_RETURN(spRewindCommandBehaviour->get_IsEnabled(&isRewindEnabled));
            // Check Minimum negative playback rate supports
            IFC_RETURN(IsPlaybackRateSupported(MinSupportedPlayRate*(-1), &isSupport));
            // Only if playback not supports then fallback based on the API
            if (!isSupport)
            {
                if (value == xaml_media::FastPlayFallbackBehaviour::FastPlayFallbackBehaviour_Disable)
                {
                    if  (m_tpFastRewindButton)
                    {
                        IFC_RETURN(m_tpFastRewindButton.Cast<Button>()->put_IsEnabled(FALSE));
                    }

                }
                else
                if (value == xaml_media::FastPlayFallbackBehaviour::FastPlayFallbackBehaviour_Hide)
                {
                    if (m_tpFastRewindButton)
                    {
                        IFC_RETURN(m_tpFastRewindButton.Cast<Button>()->put_Visibility(xaml::Visibility_Collapsed));
                    }
                }
                else
                {
                    if (m_tpFastRewindButton)
                    {
                        // 1. Rewind API enable 2. MediaBreak is not playing.
                        IFC_RETURN(m_tpFastRewindButton.Cast<Button>()->put_IsEnabled(rfenable && !m_isBreakPlaying));
                    }
                }
            }
            else
            {
                if (m_tpFastRewindButton)
                {
                    IFC_RETURN(m_tpFastRewindButton.Cast<Button>()->put_Visibility(
                        (frVisible) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
                    // Command Manager Rewind Behaviour always override the MTC developer API option.
                    // 1. Rewind enable on Command Manager 2. Rewind API enable 3. MediaBreak is not playing.
                    IFC_RETURN(m_tpFastRewindButton.Cast<Button>()->put_IsEnabled(isRewindEnabled && rfenable  && !m_isBreakPlaying));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::NextTrack()
{
    ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
    IFC_RETURN(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
    if (spMediaPlayer2)
    {
        IFC_RETURN(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(
            spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_Next))
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::PreviousTrack()
{
    ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
    IFC_RETURN(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
    if (spMediaPlayer2)
    {
        IFC_RETURN(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(
            spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_Previous));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateTracksUI()
{
    BOOLEAN nextTrk = FALSE;
    BOOLEAN previousTrk = FALSE;
    BOOLEAN isNextEnabled = FALSE;
    BOOLEAN isPrevEnabled = FALSE;

    IFC_RETURN(get_IsNextTrackButtonVisible(&nextTrk));
    IFC_RETURN(get_IsPreviousTrackButtonVisible(&previousTrk));
    if (nextTrk || previousTrk)
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
        ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spNextCommandBehaviour;
        ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPrevCommandBehaviour;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(spCommandManager->get_NextBehavior(&spNextCommandBehaviour));
        IFC_RETURN(spNextCommandBehaviour->get_IsEnabled(&isNextEnabled));
        IFC_RETURN(spCommandManager->get_PreviousBehavior(&spPrevCommandBehaviour));
        IFC_RETURN(spPrevCommandBehaviour->get_IsEnabled(&isPrevEnabled));

        if (m_tpNextTrackButton && nextTrk)
        {
            IFC_RETURN(m_tpNextTrackButton.Cast<Button>()->put_IsEnabled(nextTrk))
        }
        if (m_tpPreviousTrackButton && previousTrk)
        {
            IFC_RETURN(m_tpPreviousTrackButton.Cast<Button>()->put_IsEnabled(previousTrk));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateSeekPositionsUI()
{
    BOOLEAN skipBck = FALSE;
    BOOLEAN skipForward = FALSE;
    BOOLEAN isPositionEnabled = TRUE;

    ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPositionBehaviour;

    if (m_spMediaPlayer)
    {
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(spCommandManager->get_PositionBehavior(&spPositionBehaviour));
        IFC_RETURN(spPositionBehaviour->get_IsEnabled(&isPositionEnabled));
    }

    IFC_RETURN(get_IsSkipBackwardButtonVisible(&skipBck));
    IFC_RETURN(get_IsSkipForwardButtonVisible(&skipForward));
    if (skipBck)
    {
        BOOLEAN isSkipBackEnable = FALSE;
        IFC_RETURN(get_IsSkipBackwardEnabled(&isSkipBackEnable));
        if (m_tpSkipBackwardButton)
        {
            // Command Manager Position Behaviour always override the MTC developer API option.
            // It should enable only all condition are true:
            // 1. Position enable on Command Manager 2. SkipBackward API enable 3. MediaBreak is not playing.
            IFC_RETURN(m_tpSkipBackwardButton.Cast<Button>()->put_IsEnabled(isPositionEnabled && isSkipBackEnable && !m_isBreakPlaying));
        }
    }

    if (skipForward)
    {
        BOOLEAN isSkipForwardEnable = FALSE;
        IFC_RETURN(get_IsSkipForwardEnabled(&isSkipForwardEnable));
        if (m_tpSkipForwardButton)
        {
            // Command Manager Position Behaviour always override the MTC developer API option.
            // It should enable only all condition are true:
            // 1. Position enable on Command Manager 2. SkipForward API enable 3. MediaBreak is not playing.
            IFC_RETURN(m_tpSkipForwardButton.Cast<Button>()->put_IsEnabled(isPositionEnabled && isSkipForwardEnable && !m_isBreakPlaying));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdatePlaybackRateUI()
{
    BOOLEAN playbackRateVisible = FALSE;
    BOOLEAN isRateEnabled = TRUE;
    ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spRateBehaviour;

    if (m_spMediaPlayer)
    {
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(spCommandManager->get_RateBehavior(&spRateBehaviour));
        IFC_RETURN(spRateBehaviour->get_IsEnabled(&isRateEnabled));
    }

    IFC_RETURN(get_IsPlaybackRateButtonVisible(&playbackRateVisible));
    if (playbackRateVisible)
    {
        BOOLEAN isPlaybackRateEnabled = FALSE;
        IFC_RETURN(get_IsPlaybackRateEnabled(&isPlaybackRateEnabled));
        if (m_tpPlaybackRateButton)
        {
            // Command Manager PlaybackRate Behaviour always override the MTC developer API option.
            // It should enable only all condition are true:
            // 1. rate enable on Command Manager 2. Playback Rate API enable and 3. MediaBreak is not playing.
            IFC_RETURN(m_tpPlaybackRateButton.Cast<Button>()->put_IsEnabled(isRateEnabled && isPlaybackRateEnabled & !m_isBreakPlaying));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateBreakStatus(_In_ bool isBreakStart)
{
    if (isBreakStart)
    {
        m_isBreakPlaying = TRUE;
        IFC_RETURN(SubscribeBreakListEvents());
    }
    else
    {
        m_isBreakPlaying=FALSE;
        IFC_RETURN(UnSubscribeBreakListEvents());
    }
    IFC_RETURN(InitializeVisualState());
    return S_OK;

}
_Check_return_ HRESULT
MediaTransportControls::UpdateBreakUI()
{
    ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;

    if (m_isBreakPlaying)
    {
        if (m_tpMediaPositionSlider)
        {
            // disable the SeekBar while Media Break Playing
            IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->put_IsEnabled(!m_isBreakPlaying));
        }
    }
    else
    {
        BOOLEAN value = FALSE;
        if (m_tpMediaPositionSlider)
        {
            IFC_RETURN(get_IsSeekEnabled(&value));
            IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->put_IsEnabled(value));
        }
    }
    if (m_tpCastButton)
    {
        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            ctl::ComPtr<wmp::IMediaPlaybackItem2> spMediaPlaybackItemExt;
            ctl::ComPtr<wmp::IMediaBreakSchedule> spMediaBreakSchedule;
            ctl::ComPtr<wmp::IMediaBreak> spPreRollBreak;
            ctl::ComPtr<wfc::IVectorView<wmp::MediaBreak*>> spMidRollBreaks;
            ctl::ComPtr<wmp::IMediaBreak> spPostRollBreak;
            unsigned int size = 0;
            IFC_RETURN(spPlaybackItem.As(&spMediaPlaybackItemExt));
            IFC_RETURN(spMediaPlaybackItemExt->get_BreakSchedule(&spMediaBreakSchedule));
            IFC_RETURN(spMediaBreakSchedule->get_PrerollBreak(&spPreRollBreak));
            IFC_RETURN(spMediaBreakSchedule->get_MidrollBreaks(&spMidRollBreaks));
            IFC_RETURN(spMidRollBreaks->get_Size(&size));
            IFC_RETURN(spMediaBreakSchedule->get_PostrollBreak(&spPostRollBreak));
            // Enable Cast button when there is no Break Exist in the Playback Item
            IFC_RETURN(m_tpCastButton.Cast<Button>()->put_IsEnabled(spPreRollBreak == nullptr && size == 0  && spPostRollBreak == nullptr));
        }
    }
    return S_OK;
}


_Check_return_ HRESULT
MediaTransportControls::FireThumbnailEvent()
{
    if (MTCParent_MediaPlayerElement == m_parentType && m_isThumbnailEnabled)
    {
        // We need to reference to Thumbnail and TimeElapsedPreview which is a part of MediaSlider template
        // We cannot get the from OnApplyTemplate, Since We should be able to do this once in the Loaded event of the MediaSlider.
        // So we extract those when it really required.
        if (m_tpMediaPositionSlider && !m_tpThumbnailImage)
        {
            ctl::ComPtr<IImage> spImage;
            IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->GetTemplatePart<IImage>(STR_LEN_PAIR(L"ThumbnailImage"),
                spImage.ReleaseAndGetAddressOf()));
            SetPtrValueWithQIOrNull(m_tpThumbnailImage, spImage.Get());
        }

        if (m_tpMediaPositionSlider && !m_tpTimeElapsedPreview)
        {
            ctl::ComPtr<ITextBlock> spText;
            IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->GetTemplatePart<ITextBlock>(STR_LEN_PAIR(L"TimeElapsedPreview"),
                spText.ReleaseAndGetAddressOf()));
            SetPtrValueWithQIOrNull(m_tpTimeElapsedPreview, spText.Get());
        }

        ctl::ComPtr<MediaTransportControlsThumbnailRequestedEventArgs> spArgs;
        ThumbnailRequestedEventSourceType* pEventSource = nullptr;

        ctl::ComPtr<DirectUI::MediaTransportControls> spThis(this);
        ctl::ComPtr<xaml_controls::IMediaTransportControls> spThisIMTC;
        IFC_RETURN(spThis.As(&spThisIMTC));

        IFC_RETURN(ctl::make<MediaTransportControlsThumbnailRequestedEventArgs>(&spArgs));
        IFC_RETURN(spArgs->SetOwner(spThisIMTC.Get()));
        // Raise event
        MediaTransportControlsGenerated::GetThumbnailRequestedEventSourceNoRef(&pEventSource);
        IFC_RETURN(pEventSource->Raise(spThisIMTC.Get(), spArgs.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::ShowHideThumbnail(BOOLEAN value)
{
    if (MTCParent_MediaPlayerElement == m_parentType && m_isThumbnailEnabled)
    {
        if (m_tpMediaPositionSlider)
        {
            IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->put_IsThumbToolTipEnabled(value));
        }
    }
    return S_OK;
}


_Check_return_ HRESULT
MediaTransportControls::SetThumbnailImage(_In_ wsts::IInputStream* pstream)
{
    ctl::ComPtr<BitmapImage> spBitmapImage;
    ctl::ComPtr<IBitmapSource> spBitmapSource;
    ctl::ComPtr<wsts::IRandomAccessStream> spRandomStream;
    ctl::ComPtr<wsts::IInputStream> spInputStream = pstream;
    ctl::ComPtr<IImageSource> spImageSource;

    if (m_tpThumbnailImage && spInputStream)
    {
        IFC_RETURN(ctl::make(&spBitmapImage));
        IFC_RETURN(spInputStream.As(&spRandomStream));
        IFC_RETURN(spBitmapImage.As(&spBitmapSource));
        IFC_RETURN(spBitmapSource->SetSource(spRandomStream.Get()));
        IFC_RETURN(spBitmapImage.As(&spImageSource));
        IFC_RETURN(m_tpThumbnailImage.Cast<Image>()->put_Source(spImageSource.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateTrickModeButton(_In_ double playrate)
{
    unsigned int resourceID;
    wrl_wrappers::HString strAutomationName;

    if (m_isTrickForwardMode && m_tpFastForwardButton)
    {
        const double FastForwardTrickPlaybackRateList[] = { 2.0, 4.0, 8.0, 16.0 };
        if (playrate == FastForwardTrickPlaybackRateList[0])
        {
            resourceID = UIA_MEDIA_FASTFORWARD_2X;
        }
        else
        if (playrate == FastForwardTrickPlaybackRateList[1])
        {
            resourceID = UIA_MEDIA_FASTFORWARD_4X;
        }
        else
        if (playrate == FastForwardTrickPlaybackRateList[2])
        {
            resourceID = UIA_MEDIA_FASTFORWARD_8X;
        }
        else
        if (playrate == FastForwardTrickPlaybackRateList[3])
        {
            resourceID = UIA_MEDIA_FASTFORWARD_16X;
        }
        else
        {
            resourceID = UIA_MEDIA_FASTFORWARD;
        }
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(resourceID, strAutomationName.ReleaseAndGetAddressOf()));
        IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpFastForwardButton.Cast<Button>(), strAutomationName));
        IFC_RETURN(AddTooltip(m_tpFastForwardButton.Cast<Button>(), strAutomationName));
    }
    if (m_isTrickBackwardMode && m_tpFastRewindButton)
    {
        const double RewindTrickPlaybackRateList[] = { -2.0, -4.0, -8.0, -16.0 };
        if (playrate == RewindTrickPlaybackRateList[0])
        {
            resourceID = UIA_MEDIA_REWIND_2X;
        }
        else
        if (playrate == RewindTrickPlaybackRateList[1])
        {
            resourceID = UIA_MEDIA_REWIND_4X;
        }
        else
        if (playrate == RewindTrickPlaybackRateList[2])
        {
            resourceID = UIA_MEDIA_REWIND_8X;
        }
        else
        if (playrate == RewindTrickPlaybackRateList[3])
        {
            resourceID = UIA_MEDIA_REWIND_16X;
        }
        else
        {
            resourceID = UIA_MEDIA_REWIND;
        }

        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(resourceID, strAutomationName.ReleaseAndGetAddressOf()));
        IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpFastRewindButton.Cast<Button>(), strAutomationName));
        IFC_RETURN(AddTooltip(m_tpFastRewindButton.Cast<Button>(), strAutomationName));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::IsPlaybackRateSupported(_In_ double playrate, _Out_ BOOLEAN* value)
{
    if (m_spMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;

        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_PlaybackSession(&spPlaybackSession));
        // Check playback rate supports
        IFC_RETURN(MediaPlaybackSessionExtension_IsPlaybackRateSupported(spPlaybackSession.Get(), playrate, value));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::GetCurrentPlaybackSession(_Outptr_result_maybenull_ wmp::IMediaPlaybackSession** ppValue)
{
    IFC_RETURN(MediaPlayer_GetCurrentPlaybackSession(m_spMediaPlayer.Get(), ppValue));

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::GetMediaPlayer2ForPlaybackDataSource(_Outptr_result_maybenull_ wmp::IMediaPlayer2** value)
{
    *value = nullptr;
    if (m_spMediaPlayer)
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
        ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
        BOOLEAN isEnable = FALSE;

        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
        IFC_RETURN(spMediaPlayer3->get_CommandManager(&spCommandManager));
        IFC_RETURN(spCommandManager->get_IsEnabled(&isEnable));
        // only consider when Command Manager is enable.
        if (isEnable)
        {
            ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;

            IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer2));
            *value = spMediaPlayer2.Detach();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::ConcatFields(_Inout_ wrl_wrappers::HString* base, _In_ wrl_wrappers::HString& input, _Inout_ int* fieldCount, _In_ int numberOfFields)
{
    wrl_wrappers::HString separator;
    IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_MEDIA_AUDIO_TRACK_SEPARATOR, separator.ReleaseAndGetAddressOf()));

    if (!input.IsEmpty() && *fieldCount < numberOfFields)
    {
        if (base->IsEmpty())
        {
            IFC_RETURN(base->Set(input.Get()));
        }
        else
        {
            IFC_RETURN(base->Concat(separator, *base));
            IFC_RETURN(base->Concat(input, *base));
        }
        *fieldCount++;
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::CreateUniqueTags(_In_ std::vector<TrackFields> trackData, _Out_ std::vector<wrl_wrappers::HString>&& pTrackTags)
{
    for (unsigned int i = 0; i < trackData.size(); i++)
    {
        BOOLEAN bFoundUnique = FALSE;
        int fieldCount = 1;

        while (!bFoundUnique)
        {
            wrl_wrappers::HString current;
            BOOLEAN bUsedPreviously = FALSE;
            IFC_RETURN(current.Set(L""));

            IFC_RETURN(AudioTag(trackData.at(i), fieldCount, current));
            IFC_RETURN(UsedPreviously(std::move(pTrackTags), current, &bUsedPreviously));
            if (bUsedPreviously && fieldCount <= 6)
            {
                fieldCount++;
                bFoundUnique = FALSE;
            }
            else
            {
                bFoundUnique = TRUE;
                pTrackTags.push_back(std::move(current));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UsedPreviously(_In_ std::vector<wrl_wrappers::HString>&& trackTags, _In_ wrl_wrappers::HString& current, _Out_ BOOLEAN* value)
{
    *value = FALSE;

    for (unsigned int i = 0; i < trackTags.size() && *value == FALSE; i++)
    {
        if (current == trackTags.at(i))
        {
            *value = TRUE;
        }
    }

    return S_OK;
}

HRESULT MediaTransportControls::CreateLanguage(_In_z_ PCWSTR languageTag, _COM_Outptr_ wg::ILanguage ** ppLanguage)
{
    wrl_wrappers::HStringReference tag(languageTag);

    ctl::ComPtr<wg::ILanguageFactory> spLanguageFactory;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Language).Get(),
        spLanguageFactory.GetAddressOf()));

    ctl::ComPtr<wg::ILanguage> spLanguage;
    IFC_RETURN(spLanguageFactory->CreateLanguage(
        tag.Get(),
        spLanguage.GetAddressOf()));

    return spLanguage.CopyTo(ppLanguage);
}

_Check_return_ HRESULT
MediaTransportControls::AudioTag(_In_ TrackFields trackFields, _In_ int numberOfFields, _Out_ wrl_wrappers::HString& audioTag)
{
    wrl_wrappers::HString base;
    int fieldsCount = 0;

    // If there is a label available for the audio track then it will be used as the audio track name.
    // If one is not available then a unique name is constructed from the available metadata of the audio track.
    if (!trackFields.label.IsEmpty())
    {
        IFC_RETURN(trackFields.label.CopyTo(base.GetAddressOf()));
    }
    else if (!trackFields.name.IsEmpty())
    {
        IFC_RETURN(trackFields.name.CopyTo(base.GetAddressOf()));
    }
    else
    {
        // Concat Track index
        WCHAR wszVal[5];
        IFCEXPECT_RETURN(swprintf_s(wszVal, 5, L"%d", trackFields.trackIndex) >= 0);
        wrl_wrappers::HString strIndex;
        strIndex.Set(wszVal);
        IFC_RETURN(ConcatFields(&base, strIndex, &fieldsCount, numberOfFields));

        // Concat Language
        if (!trackFields.language.IsEmpty())
        {
            ctl::ComPtr<wg::ILanguage> spLanguage;
            IFC_RETURN(CreateLanguage(trackFields.language.GetRawBuffer(nullptr), spLanguage.GetAddressOf()));
            wrl_wrappers::HString strDisplayName;
            IFC_RETURN(spLanguage->get_DisplayName(strDisplayName.GetAddressOf()));
            IFC_RETURN(ConcatFields(&base, strDisplayName, &fieldsCount, numberOfFields));
        }

        IFC_RETURN(ConcatFields(&base, trackFields.subtype, &fieldsCount, numberOfFields));
        IFC_RETURN(ConcatFields(&base, trackFields.channelCount, &fieldsCount, numberOfFields));
    }

    IFC_RETURN(base.CopyTo(audioTag.GetAddressOf()));

    return S_OK;
}

// Creates a unique name for each audio track in a video to avoid user confusion about track selection.
_Check_return_ HRESULT
MediaTransportControls::CreateAudioTags(_In_ wmp::IMediaPlaybackItem* pPlaybackItem, _Out_ std::vector<wrl_wrappers::HString>&& pTagList)
{
    ctl::ComPtr<wfc::IVectorView<wmc::AudioTrack*>> spAudioTracks;
    ctl::ComPtr<wmp::IMediaPlaybackItem> spMediaPlaybackItem = pPlaybackItem;
    std::vector<TrackFields> trackData;
    wrl_wrappers::HString strData;
    unsigned int audioTracksSize;

    IFC_RETURN(spMediaPlaybackItem->get_AudioTracks(&spAudioTracks));
    IFC_RETURN(spAudioTracks->get_Size(&audioTracksSize));

    for (unsigned int i = 0; i < audioTracksSize; i++)
    {
        TrackFields trackMetadata;
        ctl::ComPtr<wmc::IMediaTrack> spMediaTrack;
        ctl::ComPtr<wmc::IAudioTrack> spAudioTrack;
        ctl::ComPtr<wm::MediaProperties::IAudioEncodingProperties> spAudioEncodingProperties;
        ctl::ComPtr<wm::MediaProperties::IMediaEncodingProperties> spMediaEncodingProperties;

        IFC_RETURN(spAudioTracks->GetAt(i, spMediaTrack.GetAddressOf()));
        IFC_RETURN(spMediaTrack.As(&spAudioTrack));
        IFC_RETURN(spAudioTrack->GetEncodingProperties(spAudioEncodingProperties.GetAddressOf()));
        IFC_RETURN(spAudioEncodingProperties.As(&spMediaEncodingProperties));

        // Label
        IFC_RETURN(spMediaTrack->get_Label(strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(trackMetadata.label.Set(strData.Get()));

        // Name
        IFC_RETURN(spAudioTrack->get_Name(strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(trackMetadata.name.Set(strData.Get()));

        // Language
        IFC_RETURN(spMediaTrack->get_Language(strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(trackMetadata.language.Set(strData.Get()));

        // Id
        IFC_RETURN(spMediaTrack->get_Id(strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(trackMetadata.id.Set(strData.Get()));

        // Subtype
        IFC_RETURN(spMediaEncodingProperties->get_Subtype(strData.ReleaseAndGetAddressOf()));
        IFC_RETURN(trackMetadata.subtype.Set(strData.Get()));

        // Channel Count
        unsigned int channelCount;
        IFC_RETURN(spAudioEncodingProperties->get_ChannelCount(&channelCount));
        std::wstring strChannel = std::to_wstring(channelCount);
        IFC_RETURN(trackMetadata.channelCount.Set(strChannel.c_str()));

        // Track index
        trackMetadata.trackIndex = i;

        trackData.push_back(trackMetadata);
    }

    IFC_RETURN(CreateUniqueTags(trackData, std::move(pTagList)));

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateRepeatButtonUIFromMPE()
{
    BOOLEAN repeatButtonVisible = FALSE;
    BOOLEAN isAutoRepeatEnabled = TRUE;
    ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
    ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spAutoRepeatBehaviour;

    if (m_spMediaPlayer)
    {
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerExt));
        IFC_RETURN(spMediaPlayerExt->get_CommandManager(&spCommandManager));
        IFC_RETURN(spCommandManager->get_AutoRepeatModeBehavior(&spAutoRepeatBehaviour));
        IFC_RETURN(spAutoRepeatBehaviour->get_IsEnabled(&isAutoRepeatEnabled));
    }

    IFC_RETURN(get_IsRepeatButtonVisible(&repeatButtonVisible));
    if (repeatButtonVisible)
    {
        BOOLEAN isRepeatEnabled = FALSE;
        IFC_RETURN(get_IsRepeatEnabled(&isRepeatEnabled));
        if (m_tpRepeatButton)
        {
            // Command Manager AutoRepeat Behaviour always override the MTC developer API option.
            // It should enable only all condition are true:
            // 1. AutoRepeat enable on Command Manager 2. Repeat API enable and 3. MediaBreak is not playing.
            IFC_RETURN(m_tpRepeatButton.Cast<ToggleButton>()->put_IsEnabled(isAutoRepeatEnabled && isRepeatEnabled & !m_isBreakPlaying));
        }
    }
    return S_OK;
}
