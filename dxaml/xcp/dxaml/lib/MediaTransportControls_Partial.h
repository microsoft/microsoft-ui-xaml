// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents the Media Transport Controls that can
//      optionally be enabled on a MediaPlayerElement.

#pragma once

#include "MediaTransportControls.g.h"
#include "MediaPlayerElement.g.h"
#include <windows.graphics.display.h>
#include <MediaTransportControlAggregationTelemetry.h>
#include <FrameworkUdk/MediaPlaybackDataSourceExtension.h>

namespace DirectUI
{
    // Indicates which parent MTC using
    enum MTCParent
    {
        MTCParent_None,
        MTCParent_MediaPlayerElement
    };

    // Indicates MediaPlayer Property
    enum MediaPlayer_Property
    {
        MediaPlayer_MediaOpened,
        MediaPlayer_MediaFailed,
        MediaPlayer_Position,
        MediaPlayer_Volume,
        MediaPlayer_Mute,
        MediaPlayer_DownloadProgress,
        MediaPlayer_CurrentState,
        MediaPlayer_NaturalDuration,
        MediaPlayer_Source,
        MediaPlayer_ItemChanged,
        MediaPlayer_PlaybackRate,
        MediaPlayer_MediaBreak_CurrentState,
        MediaPlayer_MediaBreak_Position,
        MediaPlayer_MediaBreak_DownloadProgress,
        MediaPlayer_Repeat
    };

    struct TrackFields
    {
        wrl_wrappers::HString label;
        wrl_wrappers::HString name;
        wrl_wrappers::HString language;
        wrl_wrappers::HString id;
        wrl_wrappers::HString subtype;
        wrl_wrappers::HString channelCount;
        int trackIndex;

        TrackFields() { }
        TrackFields(const TrackFields& fields)
        {
            label.Set(fields.label.Get());
            name.Set(fields.name.Get());
            language.Set(fields.language.Get());
            id.Set(fields.id.Get());
            subtype.Set(fields.subtype.Get());
            channelCount.Set(fields.channelCount.Get());
            trackIndex = fields.trackIndex;
        }
    };

    class IMediaPlayerElement;

    PARTIAL_CLASS(MediaTransportControls)
    {
    public:

        static _Check_return_ HRESULT Create(_Outptr_ MediaTransportControls** ppMediaTransportControls);

        _Check_return_ HRESULT InitializeTransportControls(_In_ MediaPlayerElement* pMediaPlayerElement);

        _Check_return_ HRESULT DeinitializeTransportControls();
        _Check_return_ HRESULT DeinitializeTransportControlsFromMPE();

        _Check_return_ HRESULT Enable();

        _Check_return_ HRESULT Disable();


        _Check_return_ HRESULT InitializeVisualState();
        _Check_return_ HRESULT InitializeVisualStateFromMPE();

        _Check_return_ HRESULT UpdateVisualState(_In_ bool bUseTransitions = true);
        _Check_return_ HRESULT UpdateVisualStateFromMPE(_In_ bool bUseTransitions = true);

        _Check_return_ HRESULT UpdateAudioSelectionFlyout();
        _Check_return_ HRESULT UpdateAudioSelectionFlyoutFromMPE();

        _Check_return_ HRESULT UpdateCCSelectionFlyout();
        _Check_return_ HRESULT UpdateCCSelectionFlyoutFromMPE();

        _Check_return_ HRESULT UpdatePlaybackRateFlyout();

        void ReleaseMenuFlyoutItemClickHandlers();

        void ReleaseCCSelectionMenuFlyoutItemClickHandlers();

        void ReleasePlaybackRateMenuFlyoutItemClickHandlers();

        BOOLEAN GetIsEnabled() { return m_transportControlsEnabled; }

        _Check_return_ HRESULT OnOwnerPropertyChanged(_In_ KnownPropertyIndex propertyIndex);
        _Check_return_ HRESULT OnOwnerMPEPropertyChanged(_In_ KnownPropertyIndex propertyIndex);

        _Check_return_ HRESULT AddToFullWindowMediaRoot();

        static _Check_return_ HRESULT SetMediaPlayerElementFullWindow(ctl::ComPtr<xaml_controls::IMediaPlayerElement> spMediaPlayer, BOOLEAN value);

        _Check_return_ HRESULT UpdateAfterEnteringFullWindow();
        _Check_return_ HRESULT SetFocusAfterEnteringFullWindowMode();
        _Check_return_ HRESULT HandleExitFullWindowMode();

        _Check_return_ HRESULT RemoveFromFullWindowMediaRoot();

        _Check_return_ HRESULT HandleMediaFailed();
        _Check_return_ HRESULT HandleMediaFailed(_In_ wmp::IMediaPlayerFailedEventArgs *pArgs);
        _Check_return_ HRESULT HandleItemMediaFailed(_In_ wmp::IMediaPlaybackItemFailedEventArgs *pArgs);

        BOOLEAN GetControlPanelIsVisible() { return m_controlPanelIsVisible; }

        // Used to handle back button presses on phone
        _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* returnValue);

        _Check_return_ HRESULT ReleasePlaybackItemReference();

        _Check_return_ HRESULT SetThumbnailImage(_In_ wsts::IInputStream* pstream);
        IFACEMETHOD(add_ThumbnailRequested)(_In_ wf::ITypedEventHandler<xaml_controls::MediaTransportControls*, xaml_media::MediaTransportControlsThumbnailRequestedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
        IFACEMETHOD(remove_ThumbnailRequested)(_In_ EventRegistrationToken tToken) override;

        _Check_return_ HRESULT EnterImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bUseLayoutRounding) final;

        _Check_return_ HRESULT ShowImpl() { return ShowControlPanel(); }
        _Check_return_ HRESULT HideImpl() { return HideControlPanel(true /*hideImmediately*/); }

    protected:
        MediaTransportControls();

        ~MediaTransportControls() override;

        IFACEMETHOD(OnApplyTemplate)() override;

        _Check_return_ HRESULT Initialize() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        _Check_return_ HRESULT LeaveImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset) final;

    private:
        _Check_return_ HRESULT GetComponentSizeConstants() noexcept;
        _Check_return_ HRESULT HookupPartsAndHandlers();
        _Check_return_ HRESULT HookupVolumeAndProgressPartsAndHandlers();
        _Check_return_ HRESULT MoreControls();

        _Check_return_ HRESULT InitializeAudio();
        _Check_return_ HRESULT InitializeVideo();
        _Check_return_ HRESULT InitializeVolume();

        _Check_return_ HRESULT ShowControlPanel();
        _Check_return_ HRESULT ShowControlPanelFromMPE();

        _Check_return_ HRESULT HideControlPanel(_In_ bool hideImmediately = false);
        _Check_return_ HRESULT HideControlPanelFromMPE();

        _Check_return_ HRESULT ShowVerticalVolume();

        _Check_return_ HRESULT HideVerticalVolume(BOOLEAN forceHide = FALSE);

        _Check_return_ HRESULT OnRootUserControlLoaded();

        _Check_return_ HRESULT OnProgressSliderSizeChanged();

        _Check_return_ HRESULT OnProgressSliderFocusDisengaged();

        _Check_return_ HRESULT OnPositionUpdateTimerTick();

        _Check_return_ HRESULT OnHideControlPanelTimerTick();

        _Check_return_ HRESULT OnHideVerticalVolumeTimerTick();

        _Check_return_ HRESULT OnAudioSelectionButtonClick();

        _Check_return_ HRESULT OnTHAudioTrackSelectionButtonClick();

        _Check_return_ HRESULT OnCCSelectionButtonClick();

        _Check_return_ HRESULT OnPlaybackRateButtonClick();

        _Check_return_ HRESULT OnMuteClick();

        _Check_return_ HRESULT OnVolumeClick();

        _Check_return_ HRESULT OnPlayPauseClick();
        _Check_return_ HRESULT OnPlayPauseFromMPE();

        _Check_return_ HRESULT OnFullWindowClick();

        _Check_return_ HRESULT OnZoomClick();

        _Check_return_ HRESULT OnVolumeSliderValueChanged();

        _Check_return_ HRESULT OnVolumeButtonGotFocus(
            _In_ xaml::IRoutedEventArgs *pArgs);

        _Check_return_ HRESULT OnControlPanelEntered();

        _Check_return_ HRESULT OnControlPanelExited();

        _Check_return_ HRESULT OnControlPanelTapped(
            _In_ xaml_input::ITappedRoutedEventArgs *pArgs);

        _Check_return_ HRESULT OnControlPanelGotFocus(
            _In_ xaml::IRoutedEventArgs *pArgs);

        _Check_return_ HRESULT OnControlPanelLostFocus(
            _In_ xaml::IRoutedEventArgs *pArgs);

        _Check_return_ HRESULT OnControlPanelCaptureLost(
            _In_ xaml_input::IPointerRoutedEventArgs *pArgs);

        _Check_return_ HRESULT OnRootExited();

        _Check_return_ HRESULT OnRootPressed();

        _Check_return_ HRESULT OnRootReleased();

        _Check_return_ HRESULT OnRootCaptureLost();

        _Check_return_ HRESULT OnRootMoved();

        _Check_return_ HRESULT OnPositionSliderValueChanged(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IRangeBaseValueChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnPositionSliderPressed(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPositionSliderReleased(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPositionSliderKeyDown(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPositionSliderKeyUp(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnAudioTrackClicked(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnCCTrackClicked(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPlaybackRateMenuClicked(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnFastForwardButtonClicked();

        _Check_return_ HRESULT OnFastRewindButtonClicked();

        _Check_return_ HRESULT SkipBackward();

        _Check_return_ HRESULT SkipForward();

        _Check_return_ HRESULT OnStopButtonClicked();

        _Check_return_ HRESULT OnCastButtonClicked();

        _Check_return_ HRESULT OnRepeatButtonClicked();

        _Check_return_ HRESULT OnCompactOverlayButtonClicked();

        _Check_return_ HRESULT OnSizeChanged();

        _Check_return_ HRESULT OnBorderSizeChanged();

        _Check_return_ HRESULT OnCoreWindowKeyDown(
            _In_ wuc::IKeyEventArgs* pArgs);

        _Check_return_ HRESULT OnVisibilityVisualStateChanged(
            _In_ xaml::IVisualStateChangedEventArgs* pEventArgs);

        _Check_return_ HRESULT UpdatePlayPauseUI();

        _Check_return_ HRESULT UpdateAudioSelectionUI();

        _Check_return_ HRESULT UpdateCCSelectionUI();

        _Check_return_ HRESULT UpdateFullWindowUI();

        _Check_return_ HRESULT UpdateMiniViewUI();

        _Check_return_ HRESULT SetMiniView(_In_ bool bIsEnable);

        _Check_return_ HRESULT UpdateIsMutedUI();

        _Check_return_ HRESULT UpdateVolumeUI();

        _Check_return_ HRESULT UpdatePositionUI();

        _Check_return_ HRESULT UpdateDownloadProgressUI();

        _Check_return_ HRESULT UpdateErrorUI();
        _Check_return_ HRESULT UpdateErrorUIFromMPE();

        _Check_return_ HRESULT UpdateRepeatButtonUI();
        _Check_return_ HRESULT UpdateRepeatButtonUIFromMPE();

        _Check_return_ HRESULT StartPositionUpdateTimer();

        _Check_return_ HRESULT StopPositionUpdateTimer();

        _Check_return_ HRESULT StartVerticalVolumeHideTimer();

        _Check_return_ HRESULT StopVerticalVolumeHideTimer();

        _Check_return_ HRESULT StartControlPanelHideTimer();

        _Check_return_ HRESULT StopControlPanelHideTimer();

        _Check_return_ HRESULT SetCheckedProperty(
            _In_opt_ xaml_primitives::IToggleButton *pToggleButton,
            _In_ BOOLEAN value);

        _Check_return_ HRESULT AddTooltip(
            _In_ xaml::IDependencyObject* pTooltipTarget,
            _In_ HSTRING hstrTooltipText);

        _Check_return_ HRESULT UpdateTooltipText(
            _In_ xaml::IDependencyObject* pTooltipTarget,
            _In_ HSTRING hstrTooltipText);

        _Check_return_ HRESULT ConvertSecondsToHString(
            _In_ INT64 totalSeconds,
            _Outptr_ HSTRING* pDisplayTime);

        _Check_return_ HRESULT GetLocalizedLanguageName(
            _In_ HSTRING languageTag,
            _Outptr_ HSTRING* pProcessedLanguageName);

        _Check_return_ HRESULT MarkLanguageSelection(
            _In_opt_ HSTRING localizedLanguage,
            _Outptr_ HSTRING* pMarkedLocalizedLanguage);

        _Check_return_ HRESULT GetErrorResourceID(
            _In_ UINT32 errorCode,
            _Out_ UINT32* pResourceID);

        _Check_return_ HRESULT HasKeyOrProgFocus(
            _In_ xaml::IRoutedEventArgs *pArgs,
            _Out_ BOOLEAN *pHasKeyOrProgFocus);

        _Check_return_ HRESULT CompareWithOriginalSource(
            _In_ xaml::IRoutedEventArgs *pArgs,
            _In_ IInspectable *pObjectToCompare,
            _Out_ BOOLEAN *pIsEqual);

        _Check_return_ HRESULT HitTestHelper(
            _In_ wf::Point point,
            _In_ xaml::IUIElement* pElement,
            _Out_ BOOLEAN* pHasHit);

        BOOLEAN ShouldHideVerticalVolume();

        BOOLEAN ShouldHideControlPanel();
        BOOLEAN ShouldHideControlPanelWhilePlaying();

        _Check_return_ HRESULT EnterScrubbingMode();

        _Check_return_ HRESULT ExitScrubbingMode();

        inline BOOLEAN IsLiveContent()
        {
            // Content that reports a 0 or Infinite duration is considered live for UI purposes
            return m_sourceLoaded &&
                  (m_naturalDuration.TimeSpan.Duration == INT64_MAX ||
                   m_naturalDuration.TimeSpan.Duration == 0);
        }

        _Check_return_ HRESULT LoadMediaTransportControlsFromXBF(
            _Outptr_ xaml_controls::IUserControl** ppTransportControlsRoot);

        _Check_return_ HRESULT
            UpdateMediaTransportBounds();

        _Check_return_ HRESULT
            SetupDefaultProperties();

        _Check_return_ HRESULT
            UpdateMediaControlState(_In_ KnownPropertyIndex propertyIndex) noexcept;

        _Check_return_ HRESULT
            UpdateMediaControlAllStates();

        _Check_return_ HRESULT CreateCCFlyoutTrack(_In_ HSTRING strLabel, _In_ int id, _In_ int idx);

        _Check_return_ HRESULT MeasureCommandBar();
        _Check_return_ HRESULT SetMeasureCommandBar();

        _Check_return_ HRESULT Dropout(_In_ double availableSize,
                                      _In_ wf::Size expectSize);
        _Check_return_ HRESULT Expand(_In_ double availableSize,
                                     _In_ wf::Size expectSize);
        _Check_return_ HRESULT AddMarginsBetweenGroups();
        _Check_return_ HRESULT ResetMargins();
        BOOLEAN IsButtonCollapsedbySystem(_In_ xaml::IUIElement* element);

        _Check_return_ HRESULT GetCastingDevicePicker(
                                    _Out_ ctl::ComPtr<wm::Casting::ICastingDevicePicker> &spCastingDevicePicker);
        _Check_return_ HRESULT ResetPlayBackAfterCasting();
        _Check_return_ HRESULT HideCastButtonIfNecessary();

        _Check_return_ HRESULT OnCommandBarLoaded();
        _Check_return_ HRESULT HideMoreButtonIfNecessary();

        _Check_return_ HRESULT OnVolumeSliderPointerWheelChanged(
                                    _In_ IInspectable* pSender,
                                    _In_ xaml_input::IPointerRoutedEventArgs* pArgs);
        _Check_return_ HRESULT UpdateFullScreenMode(
                                    _In_ BOOLEAN isFullWindow);
        _Check_return_ HRESULT GetFullScreenView(
                                    _Outptr_opt_ wuv::IApplicationView3** ppAppView);
        _Check_return_ HRESULT GetMiniView(
                                    _Outptr_opt_ wuv::IApplicationView4** ppAppView);

        _Check_return_ HRESULT UpdatePlaybackItemReference();

        _Check_return_ HRESULT IsMediaStateClosedFromMPE(_Out_ BOOLEAN* value);
        _Check_return_ HRESULT SetAudioTrackFromMPE(_In_ UINT selectedIndex);
        _Check_return_ HRESULT SetCCTrackFromMPE(_In_ UINT selectedIndex);

        _Check_return_ HRESULT UpdateSafeMargins(_In_ bool applySafeMargin);
        _Check_return_ HRESULT UpdateSafeMarginsinFullWindow(_In_ bool applySafeMargin);
        _Check_return_ HRESULT SetTabIndex();

    private:
         // HNS Hundreds of Nano Seconds used for conversion in timer duration
         static const unsigned int HNSPerSecond;
         // Control Panel Timeout in secs, after timeout Control Panel will be hide.
         static const double ControlPanelDisplayTimeoutInSecs;
         // Vertical Volume bar Timeout in secs, after Timeout Vertical Volume bar will be hide.
         static const double VerticalVolumeDisplayTimeoutInSecs;
         // Timer frequency in second to update Seek bar.
         static const double SeekbarPositionUpdateFreqInSecs;
         // Elapsed-Remaining Button used seeking interval defined in HNS
         static const long TimeButtonUsedSeekIntervalInHNS;
         // Maximum Time String length used in the Time Buttons.
         static const unsigned int MaxTimeButtonTextLength;
         // Maximum Processed Language length
         static const unsigned int MaxProcessedLanguageNameLength;
         // Maximum Dropout levels used in WinBlue
         static const unsigned int MaxDropuOutLevels;
         // Maximum PlayRate Counts
         static const unsigned int AvailablePlaybackRateCount;
         static const double AvailablePlaybackRateList[];
         // Skip forward/Skip Backward time interval defined in Seconds
         static const unsigned int SkipForwardInSecs;
         static const unsigned int SkipBackwardInSecs;
         static const int VolumeSliderWheelScrollStep;
         static const int MinSupportedPlayRate;

         ctl::WeakRefPtr m_wrOwnerParent;                    // Week reference to Parent(ME/MPE) associated with this MediaTransportControls.

         // Component size constants, defined as StaticResources in Xaml
         double m_resControlPanelHeight;                     // Height of Control Panel
         double m_resVerticalVolumeSliderMinHeight;          // Minimum height of Vertical Volume slider track
         double m_resVerticalVolumeSliderMaxHeight;          // Maximum height of slider track
         double m_resVerticalVolumeSliderTopPadding;         // Padding at top of slider track, but inside the panel
         double m_resVerticalVolumeSliderTopGap;             // Required minimum gap between top of slider panel and top of content area
         double m_resSideMargins;                            // Sum of left and right margins around each subcontrol
         double m_resMediaButtonWidth;                       // Width of each of the round buttons (PlayPause, Volume, Mute, etc)
         double m_resTimeButtonWidth;                        // Width of each time button (ElapsedTime, RemainingTime)
         double m_resPositionSliderMinimumWidth;             // Minimum width of Position Slider
         double m_resHorizontalVolumeSliderWidth;            // Width of Horizontal Volume Slider

        // State flags
         int  m_dropOutLevel;
         BOOLEAN m_transportControlsEnabled         : 1;
#if false // DISABLE_FULL_WINDOW
         BOOLEAN m_stretchOnFullWindowChanged       : 1;
#endif // DISABLE_FULL_WINDOW
         BOOLEAN m_verticalVolumeVisibilityChanged  : 1;
         BOOLEAN m_verticalVolumeIsVisible          : 1;
         BOOLEAN m_verticalVolumeHasKeyOrProgFocus  : 1;
         BOOLEAN m_controlPanelVisibilityChanged    : 1;
         BOOLEAN m_controlPanelIsVisible            : 1;
         BOOLEAN m_controlPanelHasPointerOver       : 1;
         BOOLEAN m_shouldDismissControlPanel        : 1;     // Specifies whether Control Panel should be dismissed. It gets set when
                                                             //  - we hit FullScreen button while playing media,
                                                             //  - we tap on the screen
                                                             //  - play state is changed (to Buffering or to Pause).
                                                             // This flag overrides m_controlPanelHasPointerOver flag in ShouldHideControlPanel logic.
         BOOLEAN m_rootHasPointerPressed            : 1;
         BOOLEAN m_controlsHaveKeyOrProgFocus       : 1;     // Specifies whether one of the transport controls has keyboard or programmatic focus
         BOOLEAN m_positionUpdateUIOnly             : 1;     // If true, update the Position slider value only - do not set underlying ME.Position DP
         BOOLEAN m_volumeUpdateUIOnly               : 1;     // If true, update the Volume slider value only - do not set underlying ME.Volume DP
         BOOLEAN m_sourceLoaded                     : 1;
         BOOLEAN m_isPlaying                        : 1;     // Specifies whether we are currently playing (for setting PlayButtons state). This includes buffering also
         BOOLEAN m_isBuffering                      : 1;     // Specifies whether we are currently buffering
         BOOLEAN m_hasError                         : 1;
         BOOLEAN m_hasMultipleAudioStreams          : 1;
         BOOLEAN m_hasCCTracks                      : 1;
         BOOLEAN m_isInScrubMode                    : 1;

         // Cache values which are accessed frequently
         BOOLEAN  m_isAudioOnly                     : 1;
#if false // DISABLE_FULL_WINDOW
         BOOLEAN  m_isFullWindow                    : 1;
         BOOLEAN  m_isFullScreen                    : 1;
#endif // DISABLE_FULL_WINDOW
         BOOLEAN  m_isCompact                       : 1;
         BOOLEAN  m_isFlyoutOpen                    : 1;
         BOOLEAN  m_isPausedForCastingSelection     : 1;
#if false // DISABLE_FULL_WINDOW
         BOOLEAN  m_isFullScreenClicked             : 1;
         BOOLEAN  m_isFullScreenPending             : 1;
         BOOLEAN  m_isLaunchedAsFullScreen          : 1;
#endif // DISABLE_FULL_WINDOW
         BOOLEAN  m_isCastSupports                  : 1;
         BOOLEAN  m_isthruScrubber                  : 1;
         BOOLEAN  m_isPointerMove                   : 1;
#if false // DISABLE_COMPACT_OVERLAY
         BOOLEAN  m_isMiniView                      : 1;
         BOOLEAN  m_isMiniViewClicked               : 1;
         BOOLEAN  m_isSpanningCompactEnabled        : 1;
#endif // DISABLE_COMPACT_OVERLAY
         double   m_positionSliderMinimum;
         double   m_positionSliderMaximum;
         double   m_volumeSliderMinimum;
         double   m_volumeSliderMaximum;
         xaml::Duration m_naturalDuration;
         xaml_media::Stretch m_stretchToRestore;
#if false // DISABLE_COMPACT_OVERLAY
         double   m_lastKnownMiniViewWidth = 0;
         double   m_lastKnownMiniViewHeight = 0;
#endif // DISABLE_COMPACT_OVERLAY
         MTCParent m_parentType;

         //Telemetry Helper Class
         CAggMediaControlEvents m_AggTelemetry;
        //
        // References to control parts we need to manipulate
        //
        // Reference to the control panel grid
        TrackerPtr<xaml_controls::IGrid> m_tpControlPanelGrid;

        // Reference to the media position slider.
        TrackerPtr<xaml_controls::ISlider> m_tpMediaPositionSlider;

        // Reference to the horizontal volume slider (audio-only mode audio slider).
        TrackerPtr<xaml_controls::ISlider> m_tpHorizontalVolumeSlider;

        // Reference to the vertical volume slider (video-mode audio slider).
        TrackerPtr<xaml_controls::ISlider> m_tpVerticalVolumeSlider;

        // Reference to the Threshold Volume slider (video-mode & audio-mode slider).
        TrackerPtr<xaml_controls::ISlider> m_tpTHVolumeSlider;

        // Reference to currently active volume slider
        TrackerPtr<xaml_controls::ISlider> m_tpActiveVolumeSlider;

        // Reference to download progress indicator, which is a part in the MediaSlider template
        TrackerPtr<xaml_controls::ISlider> m_tpDownloadProgressIndicator;

        // Reference to the buffering indeterminate progress bar
        TrackerPtr<xaml_controls::ISlider> m_tpBufferingProgressBar;

        // Reference to the PlayPause button used in Blue and Threshold
        TrackerPtr<xaml_primitives::IButtonBase> m_tpPlayPauseButton;

        // Reference to the PlayPause button used only in Threshold
        TrackerPtr<xaml_primitives::IButtonBase> m_tpTHLeftSidePlayPauseButton;

        // Reference to the Audio Selection button
        TrackerPtr<xaml_controls::IButton> m_tpAudioSelectionButton;

        // Reference to the Audio Selection button for Threshold
        TrackerPtr<xaml_controls::IButton> m_tpTHAudioTrackSelectionButton;

        // Reference to the Available Audiotracks flyout
        TrackerPtr<xaml_controls::IMenuFlyout> m_tpAvailableAudioTracksMenuFlyout;

        // Reference to the Available Audiotracks flyout target
        TrackerPtr<xaml::IFrameworkElement> m_tpAvailableAudioTracksMenuFlyoutTarget;

        // Reference to the Close Captioning Selection button
        TrackerPtr<xaml_controls::IButton> m_tpCCSelectionButton;

        // Reference to the Available Close Captioning tracks flyout
        TrackerPtr<xaml_controls::IMenuFlyout> m_tpAvailableCCTracksMenuFlyout;

        // Reference to the Play Rate Selection button
        TrackerPtr<xaml_controls::IButton> m_tpPlaybackRateButton;

        // Reference to the Available Play Rate List flyout
        TrackerPtr<xaml_controls::IMenuFlyout> m_tpAvailablePlaybackRateMenuFlyout;

        // Reference to the Video volume button
        TrackerPtr<xaml_primitives::IToggleButton> m_tpVideoVolumeButton;

        // Reference to the Audio-mute button for Blue and Mute button for Video/Audio in Threshold
        TrackerPtr<xaml_primitives::IButtonBase> m_tpMuteButton;

        // Reference to the Threshold volume button
        TrackerPtr<xaml_primitives::IButtonBase> m_tpTHVolumeButton;

        // Reference to the Full Window button
#if false // DISABLE_FULL_WINDOW
        TrackerPtr<xaml_primitives::IButtonBase> m_tpFullWindowButton;
#endif // DISABLE_FULL_WINDOW
        // Reference to the Zoom button
        TrackerPtr<xaml_primitives::IButtonBase> m_tpZoomButton;

        // Reference to currently active volume button
        TrackerPtr<xaml_primitives::IToggleButton> m_tpActiveVolumeButton;

        // Reference to Time Elapsed / -30 sec seek button or Time Elapsed TextBlock
        TrackerPtr<xaml::IFrameworkElement> m_tpTimeElapsedElement;

        // Reference to Time Remaining / +30 sec seek button or Time Remaining TextBlock
        TrackerPtr<xaml::IFrameworkElement> m_tpTimeRemainingElement;

        // Reference to the fast forward button
        TrackerPtr<xaml_controls::IButton> m_tpFastForwardButton;

        // Reference to the rewind button
        TrackerPtr<xaml_controls::IButton> m_tpFastRewindButton;

        // Reference to the stop button
        TrackerPtr<xaml_controls::IButton> m_tpStopButton;

        // Reference to the cast button
        TrackerPtr<xaml_controls::IButton> m_tpCastButton;

        // Reference to the Skip Forward button
        TrackerPtr<xaml_controls::IButton> m_tpSkipForwardButton;
        // Reference to the Skip Backward button
        TrackerPtr<xaml_controls::IButton> m_tpSkipBackwardButton;
        // Reference to the Next Track button
        TrackerPtr<xaml_controls::IButton> m_tpNextTrackButton;
        // Reference to the Previous Track button
        TrackerPtr<xaml_controls::IButton> m_tpPreviousTrackButton;
        // Reference to currently Repeat button
        TrackerPtr<xaml_primitives::IToggleButton> m_tpRepeatButton;
        // Reference to the Mini View button
#if false // DISABLE_COMPACT_OVERLAY
        TrackerPtr<xaml_controls::IButton> m_tpCompactOverlayButton;
#endif // DISABLE_COMPACT_OVERLAY
        // Reference to the Left AppBarSeparator
        TrackerPtr<xaml_controls::IAppBarSeparator> m_tpLeftAppBarSeparator;
        // Reference to the Right AppBarSeparator
        TrackerPtr<xaml_controls::IAppBarSeparator> m_tpRightAppBarSeparator;
        // Reference to the Image thumbnail preview
        TrackerPtr<xaml_controls::IImage> m_tpThumbnailImage;
        // Reference to the Time Elapsed  preview
        TrackerPtr<xaml_controls::ITextBlock> m_tpTimeElapsedPreview;

        // Reference to Error TextBlock
        TrackerPtr<xaml_controls::ITextBlock> m_tpErrorTextBlock;

        // Dispatcher timer responsible for updating clock and position slider
        TrackerPtr<xaml::IDispatcherTimer> m_tpPositionUpdateTimer;

        // Dispatcher timer responsible for hiding vertical volume host border
        TrackerPtr<xaml::IDispatcherTimer> m_tpHideVerticalVolumeTimer;

        // Dispatcher timer responsible for hiding UI control panel
        TrackerPtr<xaml::IDispatcherTimer> m_tpHideControlPanelTimer;

        // Dispatcher timer to detect the pointer move ends.
        TrackerPtr<xaml::IDispatcherTimer> m_tpPointerMoveEndTimer;

        // Reference to the Visibility Border element.
        TrackerPtr<xaml_controls::IBorder> m_tpControlPanelVisibilityBorder;

        // Reference to the CommandBar Element.
        TrackerPtr<xaml_controls::ICommandBar> m_tpCommandBar;

        // Reference to the CommandBar Element.
        TrackerPtr<xaml_primitives::IFlyoutBase> m_tpVolumeFlyout;

        // Reference to the VisualStateGroup
        TrackerPtr<xaml::IVisualStateGroup> m_tpVisibilityStatesGroup;

        //
        // Event handlers
        //

        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epRootUserControlLoadedHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPlayPauseButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epLeftsidePlayPauseButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epAudioSelectionButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epAudioTrackSelectionButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epCCSelectionButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPlaybackRateButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epVolumeButtonClickHandler;
        ctl::EventPtr<UIElementGotFocusEventCallback> m_epVolumeButtonGotFocusHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epAudioMuteButtonClickHandler;
#if false // DISABLE_FULL_WINDOW
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epFullWindowButtonClickHandler;
#endif // DISABLE_FULL_WINDOW
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epZoomButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epFastForwardButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epFastRewindButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epStopButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epCastButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epSkipForwardButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epSkipBackwardButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epNextTrackButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPreviousTrackButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epRepeatButtonClickHandler;
#if false // DISABLE_COMPACT_OVERLAY
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epCompactOverlayButtonClickHandler;
#endif // DISABLE_COMPACT_OVERLAY
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epProgressSliderSizeChangedHandler;
        ctl::EventPtr<ControlFocusDisengagedEventCallback> m_epProgressSliderFocusDisengagedHandler;
        ctl::EventPtr<RangeBaseValueChangedEventCallback> m_epHorizontalVolumeChangedHandler;
        ctl::EventPtr<RangeBaseValueChangedEventCallback> m_epVerticalVolumeChangedHandler;
        ctl::EventPtr<RangeBaseValueChangedEventCallback> m_epVolumeChangedHandler;
        ctl::EventPtr<RangeBaseValueChangedEventCallback> m_epPositionChangedHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epControlPanelEnteredHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epControlPanelExitedHandler;
        ctl::EventPtr<UIElementTappedEventCallback> m_epControlPanelTappedHandler;
        ctl::EventPtr<UIElementPointerCaptureLostEventCallback> m_epControlPanelCaptureLostHandler;
        ctl::EventPtr<UIElementGotFocusEventCallback> m_epControlPanelGotFocusHandler;
        ctl::EventPtr<UIElementLostFocusEventCallback> m_epControlPanelLostFocusHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epRootExitedHandler;
        ctl::EventPtr<UIElementPointerPressedEventCallback> m_epRootPressedHandler;
        ctl::EventPtr<UIElementPointerReleasedEventCallback> m_epRootReleasedHandler;
        ctl::EventPtr<UIElementPointerCaptureLostEventCallback> m_epRootCaptureLostHandler;
        ctl::EventPtr<UIElementPointerMovedEventCallback> m_epRootMovedHandler;
        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epPositionUpdateTimerTickHandler;
        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epHideControlPanelTimerTickHandler;
        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epHideVerticalVolumeTimerTickHandler;
        ctl::EventPtr<VisualStateGroupCurrentStateChangedEventCallback> m_visibilityStateChangedEventHandler;
        ctl::EventPtr<DispatcherTimerTickEventCallback> m_epPointerMoveEndTimerTickHandler;

        // Following handlers need to set handledEventsToo flag, so that they can get events
        // marked handled by source subcontrols. EventPtr template does not support this.
        wrl::ComPtr<xaml_input::IPointerEventHandler> m_positionSliderPressedHandler {};
        wrl::ComPtr<xaml_input::IPointerEventHandler> m_positionSliderReleasedHandler {};
        wrl::ComPtr<xaml_input::IKeyEventHandler> m_positionSliderKeyDownHandler {};
        wrl::ComPtr<xaml_input::IKeyEventHandler> m_positionSliderKeyUpHandler {};

        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epSizeChangedHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epBorderSizeChangedHandler;
        EventRegistrationToken m_tokLayoutBoundsChanged{0};
        EventRegistrationToken m_tokCoreWindowKeyDown;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epCommandBarLoadedHandler;
        ctl::EventPtr<FlyoutBaseOpenedEventCallback> m_epFlyoutOpenedHandler;
        ctl::EventPtr<FlyoutBaseClosedEventCallback> m_epFlyoutClosedHandler;
        ctl::EventPtr<UIElementPointerWheelChangedEventCallback> m_volumeSliderPointerWheelChangedHandler;

        // Vector of Click event handlers for Audio Selection MenuFlyoutItems
        std::vector<ctl::EventPtr<MenuFlyoutItemClickEventCallback>*> m_audioTrackClickHandlers;

        // Vector of Click event handlers for Close Caption Selection MenuFlyoutItems
        std::vector<ctl::EventPtr<MenuFlyoutItemClickEventCallback>*> m_CCTrackClickHandlers;

        // Vector maps UI index(key) to MF track IDs(value)
        std::vector<INT> m_trackIdMappings;

        // MF ID of the current selected text track
        INT m_currentTrack;

        // CurrentPlayback Rate
        double m_currentPlaybackRate;
        // keep track original playrate in FF/Rewind mode
        double m_orginalPlaybackRate;

        // Vector of Click event handlers for Play Rate Selection MenuFlyoutItems
        std::vector<ctl::EventPtr<MenuFlyoutItemClickEventCallback>*> m_playbackRateClickHandlers;

        // Vector of currently available playback rates
        std::vector<double> m_currentPlaybackRates;

        ctl::ComPtr<wmp::IMediaPlaybackItem> m_spCurrentItem;
        EventRegistrationToken m_trackAddedEventToken;
        // Reference to the Casting Device Picker
        ctl::ComPtr<wm::Casting::ICastingDevicePicker> m_spCastingDevicePicker;
        ctl::ComPtr<wm::Casting::ICastingConnection> m_spCastingConnection;
        EventRegistrationToken m_castingDeviceSelectedToken;
        EventRegistrationToken m_castingPickerDismissedToken;
        EventRegistrationToken m_castingConnectStateChangeToken;
        BOOLEAN m_isMediaPlayerSubscribed :1;
        BOOLEAN m_isTrickBackwardMode     :1;
        BOOLEAN m_isTrickForwardMode      :1;
        BOOLEAN m_isThumbnailEnabled      :1;
        BOOLEAN m_isTemplateApplied       :1;
        BOOLEAN m_isBreakPlaying          :1;
        BOOLEAN m_isVSStateChangeExternal :1;

        _Check_return_ HRESULT SubscribeMediaPlayerEvents() noexcept;
        _Check_return_ HRESULT UnSubscribeMediaPlayerEvents() noexcept;
        _Check_return_ HRESULT SubscribeTrackEvents();
        _Check_return_ HRESULT UnSubscribeTrackEvents();
        _Check_return_ HRESULT SubscribeBreakListEvents();
        _Check_return_ HRESULT UnSubscribeBreakListEvents();
        _Check_return_ HRESULT GetMediaPlayer2ForPlaybackDataSource(_Outptr_result_maybenull_ wmp::IMediaPlayer2** value);
        _Check_return_ HRESULT GetCurrentPlaybackSession(_Outptr_result_maybenull_ wmp::IMediaPlaybackSession** ppValue);

        //MediaAPI Properties wrappers
        _Check_return_ HRESULT GetMuted(_Out_ BOOLEAN *value);
        _Check_return_ HRESULT SetMuted(_In_ BOOLEAN value);
        _Check_return_ HRESULT GetFullWindow(_Out_ BOOLEAN *value);
        _Check_return_ HRESULT SetFullWindow(_In_ BOOLEAN value);
        _Check_return_ HRESULT GetStretch(_Out_ xaml_media::Stretch *value);
        _Check_return_ HRESULT SetStretch(_In_ xaml_media::Stretch value);
        _Check_return_ HRESULT GetVolume(_Out_ DOUBLE *value);
        _Check_return_ HRESULT SetVolume(_In_ DOUBLE value);
        _Check_return_ HRESULT GetPosition(_Out_ wf::TimeSpan *value);
        _Check_return_ HRESULT SetPosition(_In_ wf::TimeSpan value, _In_ bool isScrubber=true);
        _Check_return_ HRESULT GetAudioTrackCount(_Out_ INT *value);
        _Check_return_ HRESULT GetCCTrackCount(_Out_ UINT *pValue);
        _Check_return_ HRESULT GetSupportedTrackCount(_In_ wfc::IVectorView<wmc::TimedMetadataTrack*> *pList,
            _In_ wmp::IMediaPlaybackItem* pCurrentItem, _Out_ UINT *pValue);
        _Check_return_ HRESULT GetDownloadProgress(_Out_ DOUBLE *value);
        _Check_return_ HRESULT GetPlaybackRate(_Out_ DOUBLE *value);
        _Check_return_ HRESULT SetPlaybackRate(_In_ DOUBLE value, _In_ bool isScrubber = true);
        _Check_return_ HRESULT Stop();
        _Check_return_ HRESULT Play(_In_ bool isCast = false);
        _Check_return_ HRESULT Pause(_In_ bool isCast = false);
        _Check_return_ HRESULT GetRepeatMode(_Out_ MediaPlaybackDataSourceExtension_RepeatMode* value);
        _Check_return_ HRESULT SetRepeatMode(_In_ MediaPlaybackDataSourceExtension_RepeatMode value);
        _Check_return_ HRESULT GetAsCastingSource(_Outptr_opt_ wm::Casting::ICastingSource** returnValue);
        _Check_return_ HRESULT OnMediaPropertyChanged(_In_ MediaPlayer_Property propertyIndex);
        _Check_return_ HRESULT TrickModeForward();
        _Check_return_ HRESULT TrickModeBackward();
        _Check_return_ HRESULT ResetTrickMode();
        _Check_return_ HRESULT UpdateTrickModeFallbackUI();
        _Check_return_ HRESULT NextTrack();
        _Check_return_ HRESULT PreviousTrack();
        _Check_return_ HRESULT UpdateTracksUI();
        _Check_return_ HRESULT FireThumbnailEvent();
        _Check_return_ HRESULT ShowHideThumbnail(BOOLEAN value);
        _Check_return_ HRESULT UpdateTrickModeButton(_In_ double playrate);
        _Check_return_ HRESULT IsPlaybackRateSupported(_In_ double playrate, _Out_ BOOLEAN* value);
        _Check_return_ HRESULT UpdateSeekPositionsUI();
        _Check_return_ HRESULT UpdatePlaybackRateUI();
        _Check_return_ HRESULT UpdateBreakStatus(_In_ bool isBreakStart);
        _Check_return_ HRESULT UpdateBreakUI();

        EventRegistrationToken m_mediaPlayerMuteChangeToken;
        EventRegistrationToken m_mediaPlayerVolumeChangeToken;
        EventRegistrationToken m_mediaPlayerDownloadProgressChangeToken;
        EventRegistrationToken m_mediaPlayerCurrentStateChangeToken;
        EventRegistrationToken m_mediaPlayerNaturalDurationChangeToken;
        EventRegistrationToken m_mediaPlayerMediaOpenedToken;
        EventRegistrationToken m_mediaPlayerMediaFailedToken;
        EventRegistrationToken m_mediaPlayerSourceChangeToken;
        EventRegistrationToken m_mediaPlayerItemFailedToken;
        EventRegistrationToken m_mediaPlayerItemChangedToken;
        EventRegistrationToken m_mediaPlayerBreakItemChangedToken;
        EventRegistrationToken m_mediaPlayerPlaybackRateChangeToken;
        EventRegistrationToken m_mediaPlayerAutoRepeatChangedToken;
        EventRegistrationToken m_cmdManagerPlayBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerPauseBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerNextBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerPreviousBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerFastForwardBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerRewindBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerPositionBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerRateBehaviorChangeToken;
        EventRegistrationToken m_cmdManagerAutoRepeatBehaviorChangeToken;
        EventRegistrationToken m_brkManagerBreakStartToken;
        EventRegistrationToken m_brkManagerBreakEndToken;
        EventRegistrationToken m_brkManagerBreakSkippedToken;
        EventRegistrationToken m_mediaBreakCurrentStateChangeToken;
        EventRegistrationToken m_mediaBreakPositionChangeToken;
        EventRegistrationToken m_mediaBreakDownloadProgressChangeToken;
        EventRegistrationToken m_mediaPlayerIsLoopingEnabledToken;
        ctl::ComPtr<wmp::IMediaPlayer> m_spMediaPlayer;
        ctl::ComPtr<wmp::IMediaPlaybackList> m_spMediaPlaybackList;
        ctl::ComPtr<wmp::IMediaPlaybackList> m_spMediaBreakPlaybackList;

        _Check_return_ HRESULT AddRegistrationCoreWindowKeyDown();
        _Check_return_ HRESULT RemoveRegistrationCoreWindowKeyDown();
        _Check_return_ HRESULT EnableValueChangedEventThrottlingOnSliderAutomation(bool value);
        _Check_return_ HRESULT UpdateTimeAutomationProperties();

        UINT32 m_errcodeFromMPE;
        UINT32 GetMediaEngineErrorCode(_In_ wmp::MediaPlayerError errorMediaPlayer);
        UINT32 GetMediaEngineErrorCode(_In_ wmp::MediaPlaybackItemErrorCode errorMediaPlaybackItem);

        _Check_return_ HRESULT ConcatFields(_Inout_ wrl_wrappers::HString* base,
                                            _In_ wrl_wrappers::HString& input,
                                            _Inout_ int* fieldCount,
                                            _In_ int numberOfFields);
        _Check_return_ HRESULT CreateAudioTags(_In_ wmp::IMediaPlaybackItem* pPlaybackItem,
                                               _Out_ std::vector<wrl_wrappers::HString>&& pTagList);
        _Check_return_ HRESULT CreateUniqueTags(_In_ std::vector<TrackFields> trackData,
                                                _Out_ std::vector<wrl_wrappers::HString>&& pTrackTags);
        _Check_return_ HRESULT AudioTag(_In_ TrackFields trackFields,
                                        _In_ int numberOfFields,
                                        _Out_ wrl_wrappers::HString& pAudioTag);
        _Check_return_ HRESULT UsedPreviously(_In_ std::vector<wrl_wrappers::HString>&& trackTags,
                                              _In_ wrl_wrappers::HString& current,
                                              _Out_ BOOLEAN* value);
        _Check_return_ HRESULT CreateLanguage(_In_z_ PCWSTR languageTag, _COM_Outptr_ wg::ILanguage ** ppLanguage);

    };
}
