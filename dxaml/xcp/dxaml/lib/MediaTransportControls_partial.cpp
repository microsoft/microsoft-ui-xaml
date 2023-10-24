// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaTransportControls.g.h"
#include "MediaTransportControlsHelper.g.h"
#include "Slider.g.h"
#include "Button.g.h"
#include "ToggleButton.g.h"
#include "TextBlock.g.h"
#include "Border.g.h"
#include "Grid.g.h"
#include "DispatcherTimer.g.h"
#include "MenuFlyout.g.h"
#include "MenuFlyoutItem.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "ToolTip.g.h"
#include "ToolTipService.g.h"
#include "TranslateTransform.g.h"
#include "TappedRoutedEventArgs.g.h"
#include "CommandBar.g.h"
#include "Window.g.h"
#include <windows.graphics.display.h>
#include <windows.ui.viewmanagement.h>
#include "XamlTraceLogging.h"
#include "RangeBaseAutomationPeer.g.h"
#include "TimedTextSource.h"
#include "MediaPlayerElement.g.h"
#include <FrameworkUdk/MediaPlayerExtension.h>
#include "RectangleGeometry.g.h"
#include <Windows.Media.Casting.h>
#include <FocusMgr.h>
#include <XboxUtility.h>
#include "KeyRoutedEventArgs.g.h"
#include <FrameworkUdk/BackButtonIntegration.h>
#include "VisualTreeHelper.h"
#include "ResourceDictionary_partial.h"
#include "Callback.h"
#include <corewindow.h>
#include "AutomationProperties.h"
#include "AgileCallback.h"
#include "localizedResource.h"
#include <windows.foundation.metadata.h>
#include "XamlRoot.g.h"

using namespace ctl;
using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace Microsoft::WRL;
using namespace xaml_markup;
using namespace xaml_media;

const unsigned int MediaTransportControls::HNSPerSecond                     = 10000000;
const double MediaTransportControls::ControlPanelDisplayTimeoutInSecs       = 3.0;
const double MediaTransportControls::VerticalVolumeDisplayTimeoutInSecs     = 3.0;
const double MediaTransportControls::SeekbarPositionUpdateFreqInSecs        = 0.250;
const long MediaTransportControls::TimeButtonUsedSeekIntervalInHNS          = 300000000;
const unsigned int MediaTransportControls::MaxTimeButtonTextLength          = 9;  // [H]H:mm:ss time string has up to 8 chars; also include terminating '\0'
const unsigned int MediaTransportControls::MaxProcessedLanguageNameLength   = 50; // include terminating '\0'
const unsigned int MediaTransportControls::MaxDropuOutLevels                = 10;
const unsigned int MediaTransportControls::AvailablePlaybackRateCount       = 5;
const double MediaTransportControls::AvailablePlaybackRateList[]            = { 0.25, 0.5, 1.0, 1.5, 2.0 };
const unsigned int MediaTransportControls::SkipForwardInSecs                = 30;
const unsigned int MediaTransportControls::SkipBackwardInSecs               = 10;
const int MediaTransportControls::VolumeSliderWheelScrollStep               = 2;
const int MediaTransportControls::MinSupportedPlayRate                      = 2;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the MediaTransportControls class.
//
//------------------------------------------------------------------------
MediaTransportControls::MediaTransportControls() :
    m_resControlPanelHeight(0)
    , m_resVerticalVolumeSliderMinHeight(0)
    , m_resVerticalVolumeSliderMaxHeight(0)
    , m_resVerticalVolumeSliderTopPadding(0)
    , m_resVerticalVolumeSliderTopGap(0)
    , m_resSideMargins(0)
    , m_resMediaButtonWidth(0)
    , m_resTimeButtonWidth(0)
    , m_resPositionSliderMinimumWidth(0)
    , m_resHorizontalVolumeSliderWidth(0)
    , m_transportControlsEnabled(FALSE)
    , m_sourceLoaded(FALSE)
    , m_isPlaying(FALSE)
    , m_isBuffering(FALSE)
#if false // DISABLE_FULL_WINDOW
    , m_stretchOnFullWindowChanged(FALSE)
#endif // DISABLE_FULL_WINDOW
    , m_verticalVolumeVisibilityChanged(FALSE)
    , m_verticalVolumeIsVisible(FALSE)
    , m_verticalVolumeHasKeyOrProgFocus(FALSE)
    , m_controlPanelVisibilityChanged(FALSE)
    , m_controlPanelIsVisible(FALSE)
    , m_rootHasPointerPressed(FALSE)
    , m_controlPanelHasPointerOver(FALSE)
    , m_shouldDismissControlPanel(FALSE)
    , m_controlsHaveKeyOrProgFocus(FALSE)
    , m_isAudioOnly(false)
#if false // DISABLE_FULL_WINDOW
    , m_isFullWindow(FALSE)
    , m_isFullScreen(FALSE)
    , m_isFullScreenClicked(FALSE)
    , m_isLaunchedAsFullScreen(FALSE)
    , m_isFullScreenPending(FALSE)
#endif // DISABLE_FULL_WINDOW
    , m_hasError(FALSE)
    , m_positionUpdateUIOnly(FALSE)
    , m_volumeUpdateUIOnly(FALSE)
    , m_stretchToRestore(Stretch_None)
    , m_hasMultipleAudioStreams(FALSE)
    , m_hasCCTracks(FALSE)
    , m_isInScrubMode(FALSE)
    , m_dropOutLevel(-1)
    , m_AggTelemetry()
    , m_currentTrack(-1)
    , m_isCompact(FALSE)
    , m_isFlyoutOpen(FALSE)
    , m_isPausedForCastingSelection(FALSE)
    , m_isCastSupports(TRUE)
    , m_parentType(MTCParent_None)
    , m_currentPlaybackRate(0.0)
    , m_orginalPlaybackRate(0.0)
    , m_errcodeFromMPE(MF_MEDIA_ENGINE_ERR_NOERROR)
    , m_isMediaPlayerSubscribed(FALSE)
    , m_isTrickBackwardMode(FALSE)
    , m_isTrickForwardMode(FALSE)
    , m_isThumbnailEnabled(FALSE)
    , m_isTemplateApplied(FALSE)
    , m_isBreakPlaying(FALSE)
    , m_isthruScrubber(FALSE)
    , m_isVSStateChangeExternal(FALSE)
    , m_isPointerMove(FALSE)
#if false // DISABLE_COMPACT_OVERLAY
    , m_isMiniView(FALSE)
    , m_isMiniViewClicked(FALSE)
    , m_isSpanningCompactEnabled(FALSE)
#endif // DISABLE_COMPACT_OVERLAY
{
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Destroys an instance of the MediaTransportControls class.
//
//------------------------------------------------------------------------
MediaTransportControls::~MediaTransportControls()
{
    DeinitializeTransportControls();

    ReleaseMenuFlyoutItemClickHandlers();
    ReleaseCCSelectionMenuFlyoutItemClickHandlers();
    ReleasePlaybackRateMenuFlyoutItemClickHandlers();
}

_Check_return_ HRESULT
MediaTransportControls::Create(_Outptr_ MediaTransportControls** ppMediaTransportControls)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MediaTransportControls> spMediaTransportControls;

    // Instantiate the MediaTransportControls object
    IFC(ctl::make<MediaTransportControls>(&spMediaTransportControls));
    IFC(spMediaTransportControls.CopyTo(ppMediaTransportControls));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize initial MTC state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::Initialize()
{
    HRESULT hr = S_OK;

    IFC(MediaTransportControlsGenerated::Initialize());

    m_naturalDuration.TimeSpan.Duration = 0;
    m_naturalDuration.Type = xaml::DurationType_TimeSpan;
    m_tokLayoutBoundsChanged.value = 0;
    m_tokCoreWindowKeyDown.value = 0;
    m_trackAddedEventToken.value = 0;
    m_castingDeviceSelectedToken.value = 0;
    m_castingPickerDismissedToken.value = 0;
    m_castingConnectStateChangeToken.value = 0;
    m_mediaPlayerMuteChangeToken.value = 0;
    m_mediaPlayerVolumeChangeToken.value = 0;
    m_mediaPlayerDownloadProgressChangeToken.value = 0;
    m_mediaPlayerCurrentStateChangeToken.value = 0;
    m_mediaPlayerNaturalDurationChangeToken.value = 0;
    m_mediaPlayerMediaOpenedToken.value = 0;
    m_mediaPlayerMediaFailedToken.value = 0;
    m_mediaPlayerSourceChangeToken.value = 0;
    m_mediaPlayerItemFailedToken.value = 0;
    m_mediaPlayerItemChangedToken.value = 0;
    IFC(SetupDefaultProperties());
Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Enable the controls by initializing size and visual state,
//       then appending them as child of the owner ME.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::Enable()
{
    HRESULT hr = S_OK;
    if (!m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // Set enabled flag here as it is checked by below calls
        m_transportControlsEnabled = TRUE;

        // Set initial visual state
        IFC(InitializeVisualState());
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Disable the transport controls by removing them from ME's children.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::Disable()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(StopPositionUpdateTimer());
        IFC(StopVerticalVolumeHideTimer());
        IFC(StopControlPanelHideTimer());
#if false // DISABLE_COMPACT_OVERLAY
        if (m_isMiniView)
        {
            IFC(SetMiniView(false));
        }
#endif // DISABLE_COMPACT_OVERLAY
        m_transportControlsEnabled = FALSE;
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::AddRegistrationCoreWindowKeyDown()
{
    // Add the core window key down event if doesn't exist
    if (m_tokCoreWindowKeyDown.value == 0)
    {
        // TASK: MediaTransportControls should not rely on dummy window for keyboard events
        // Media is not currently supported in lifted XAML, and no tests are being ran to validate potential changes.
        // https://microsoft.visualstudio.com/OS/_workitems/edit/37095527
        Window *pWindow = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
        IFCPTR_RETURN(pWindow);
        {
            ctl::ComPtr<wuc::ICoreWindow> spCoreWindow;
            ctl::WeakRefPtr wrThis;
            // Using weak reference in event handler callback to make
            // sure this pointer still exists. If weak reference as unreachable we won't call into it
            // as resolving weak reference returns NULL.
            IFC_RETURN(ctl::AsWeak(this, &wrThis));
            IFC_RETURN(pWindow->get_CoreWindow(&spCoreWindow));
            // TODO: WinUI Desktop support! Part of: https://microsoft.visualstudio.com/OS/_workitems/edit/37095527
            if (spCoreWindow)
            {
                IFC_RETURN(spCoreWindow->add_KeyDown(
                    wrl::Callback< Microsoft::WRL::Implements<
                    Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                    wf::ITypedEventHandler<wuc::CoreWindow*, wuc::KeyEventArgs*>,
                    Microsoft::WRL::FtmBase >> (
                    [wrThis](wuc::ICoreWindow*, wuc::IKeyEventArgs* pArgs) mutable -> HRESULT
                {
                    ctl::ComPtr<MediaTransportControls> spThis;
                    IFC_RETURN(wrThis.As(&spThis));
                    if (spThis.Get())
                    {
                        IFC_RETURN(spThis->OnCoreWindowKeyDown(pArgs));
                    }
                    return S_OK;
                }).Get(), &m_tokCoreWindowKeyDown));
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::RemoveRegistrationCoreWindowKeyDown()
{
    // Remove the core window key down event.
    if (m_tokCoreWindowKeyDown.value)
    {
        ctl::ComPtr<wuc::ICoreWindow> spCoreWindow;

        // TASK: MediaTransportControls should not rely on dummy window for keyboard events
        // Media is not currently supported in lifted XAML, and no tests are being ran to validate potential changes.
        // https://microsoft.visualstudio.com/OS/_workitems/edit/37095527
        Window *pWindow = DXamlCore::GetCurrent()->GetDummyWindowNoRef();
        IFCPTR_RETURN(pWindow);
        IFC_RETURN(pWindow->get_CoreWindow(&spCoreWindow));
        IFC_RETURN(spCoreWindow->remove_KeyDown(m_tokCoreWindowKeyDown));
    }
    m_tokCoreWindowKeyDown.value = 0;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       De-initialize the Transport Controls.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::DeinitializeTransportControls()
{
    HRESULT hr = S_OK;

    if (auto peg = m_tpPositionUpdateTimer.TryMakeAutoPeg())
    {
        IFC(m_tpPositionUpdateTimer->Stop());
    }
    if (auto peg = m_tpHideVerticalVolumeTimer.TryMakeAutoPeg())
    {
        IFC(m_tpHideVerticalVolumeTimer->Stop());
    }
    if (auto peg = m_tpHideControlPanelTimer.TryMakeAutoPeg())
    {
        IFC(m_tpHideControlPanelTimer->Stop());
    }
    if (auto peg = m_tpPointerMoveEndTimer.TryMakeAutoPeg())
    {
        IFC(m_tpPointerMoveEndTimer->Stop());
    }

#if false // DISABLE_COMPACT_OVERLAY
    if (m_isMiniView)
    {
        IFC(SetMiniView(false));
    }
#endif // DISABLE_COMPACT_OVERLAY

    // Note - DetachHandler() may no-op if already  being called
    //        This is ok since associated
    //        EventPtr's will also be destroyed in this code path, and clean up
    //        the handlers.

    // Release Play button's event handlers
    IFC(DetachHandler(m_epPlayPauseButtonClickHandler, m_tpPlayPauseButton));
    IFC(DetachHandler(m_epLeftsidePlayPauseButtonClickHandler, m_tpTHLeftSidePlayPauseButton));

    // Release Audio Selection button's event handlers
    IFC(DetachHandler(m_epAudioSelectionButtonClickHandler, m_tpAudioSelectionButton));

    // Release Audio Selection button's event handlers for Threshold
    IFC(DetachHandler(m_epAudioTrackSelectionButtonClickHandler, m_tpTHAudioTrackSelectionButton));

    // Release Closed Caption Selection button's event handlers
    IFC(DetachHandler(m_epCCSelectionButtonClickHandler, m_tpCCSelectionButton));

    // Release Play Rate Selection button's event handlers
    IFC(DetachHandler(m_epPlaybackRateButtonClickHandler, m_tpPlaybackRateButton));

    // Release Video Mute/Volume button's event handlers
    IFC(DetachHandler(m_epVolumeButtonClickHandler, m_tpVideoVolumeButton));

    // Release Audio Mute button's event handlers
    IFC(DetachHandler(m_epAudioMuteButtonClickHandler, m_tpMuteButton));
#if false // DISABLE_FULL_WINDOW
    // Release Full Window button's event handlers
    IFC(DetachHandler(m_epFullWindowButtonClickHandler, m_tpFullWindowButton));
#endif // DISABLE_FULL_WINDOW
    // Release Zoom button's event handlers
    IFC(DetachHandler(m_epZoomButtonClickHandler, m_tpZoomButton));

    // Release other Media button's event handlers
    IFC(DetachHandler(m_epFastForwardButtonClickHandler, m_tpFastForwardButton));
    IFC(DetachHandler(m_epFastRewindButtonClickHandler, m_tpFastRewindButton));
    IFC(DetachHandler(m_epStopButtonClickHandler, m_tpStopButton));
    IFC(DetachHandler(m_epCastButtonClickHandler, m_tpCastButton));
    IFC(DetachHandler(m_epSkipForwardButtonClickHandler, m_tpSkipForwardButton));
    IFC(DetachHandler(m_epSkipBackwardButtonClickHandler, m_tpSkipBackwardButton));
    IFC(DetachHandler(m_epNextTrackButtonClickHandler, m_tpNextTrackButton));
    IFC(DetachHandler(m_epPreviousTrackButtonClickHandler, m_tpPreviousTrackButton));
    IFC(DetachHandler(m_epRepeatButtonClickHandler, m_tpRepeatButton));
#if false // DISABLE_COMPACT_OVERLAY
    IFC(DetachHandler(m_epCompactOverlayButtonClickHandler, m_tpCompactOverlayButton));
#endif // DISABLE_COMPACT_OVERLAY
    // Release Position Slider's event handlers
    IFC(DetachHandler(m_epProgressSliderSizeChangedHandler, m_tpMediaPositionSlider));
    IFC(DetachHandler(m_epProgressSliderFocusDisengagedHandler, m_tpMediaPositionSlider));
    IFC(DetachHandler(m_epPositionChangedHandler, m_tpMediaPositionSlider));
    {
        auto spMediaPositionSlider = m_tpMediaPositionSlider.GetSafeReference();

        if (spMediaPositionSlider)
        {
            if (m_positionSliderPressedHandler)
            {
                PointerPressedEventSourceType* pointerPressedEventSource = nullptr;
                IFC(spMediaPositionSlider.Cast<Slider>()->GetPointerPressedEventSourceNoRef(&pointerPressedEventSource));
                IFC(pointerPressedEventSource->RemoveHandler(m_positionSliderPressedHandler.Get()));
                m_positionSliderPressedHandler = nullptr;
            }

            if (m_positionSliderReleasedHandler)
            {
                PointerReleasedEventSourceType* pointerReleasedEventSource = nullptr;
                IFC(spMediaPositionSlider.Cast<Slider>()->GetPointerReleasedEventSourceNoRef(&pointerReleasedEventSource));
                IFC(pointerReleasedEventSource->RemoveHandler(m_positionSliderReleasedHandler.Get()));
                m_positionSliderReleasedHandler = nullptr;
            }

            if (m_positionSliderKeyDownHandler)
            {
                KeyDownEventSourceType* keyDownReleasedEventSource = nullptr;
                IFC(spMediaPositionSlider.Cast<Slider>()->GetKeyDownEventSourceNoRef(&keyDownReleasedEventSource));
                IFC(keyDownReleasedEventSource->RemoveHandler(m_positionSliderKeyDownHandler.Get()));
                m_positionSliderKeyDownHandler = nullptr;
            }

            if (m_positionSliderKeyUpHandler)
            {
                KeyUpEventSourceType* keyUpReleasedEventSource = nullptr;
                IFC(spMediaPositionSlider.Cast<Slider>()->GetKeyUpEventSourceNoRef(&keyUpReleasedEventSource));
                IFC(keyUpReleasedEventSource->RemoveHandler(m_positionSliderKeyUpHandler.Get()));
                m_positionSliderKeyUpHandler = nullptr;
            }
        }
    }
    if (m_spCastingDevicePicker)
    {
        if (m_castingDeviceSelectedToken.value != 0)
        {
            IFC(m_spCastingDevicePicker->remove_CastingDeviceSelected(m_castingDeviceSelectedToken));
        }
        if (m_castingPickerDismissedToken.value != 0)
        {
            IFC(m_spCastingDevicePicker->remove_CastingDevicePickerDismissed(m_castingPickerDismissedToken));
            m_castingPickerDismissedToken.value = 0;
        }
        if (m_spCastingConnection && m_castingConnectStateChangeToken.value != 0)
        {
            IFC(m_spCastingConnection->remove_StateChanged(m_castingConnectStateChangeToken));
            m_castingConnectStateChangeToken.value = 0;
        }
        m_spCastingDevicePicker = nullptr;
    }

    // Release Horizontal volume slider's event handlers
    IFC(DetachHandler(m_epHorizontalVolumeChangedHandler, m_tpHorizontalVolumeSlider));

    // Release Threshold volume slider's event handlers
    IFC(DetachHandler(m_epVolumeChangedHandler, m_tpTHVolumeSlider));
    IFC(DetachHandler(m_volumeSliderPointerWheelChangedHandler, m_tpTHVolumeSlider));

    // Release Video volume button's event handlers
    IFC(DetachHandler(m_epVolumeButtonGotFocusHandler, m_tpVideoVolumeButton));

    // Release Vertical volume slider's event handlers
    IFC(DetachHandler(m_epVerticalVolumeChangedHandler, m_tpVerticalVolumeSlider));

    // Release control panel grid's event handlers
    IFC(DetachHandler(m_epControlPanelEnteredHandler, m_tpControlPanelGrid));
    IFC(DetachHandler(m_epControlPanelExitedHandler, m_tpControlPanelGrid));
    IFC(DetachHandler(m_epControlPanelTappedHandler, m_tpControlPanelGrid));
    IFC(DetachHandler(m_epControlPanelCaptureLostHandler, m_tpControlPanelGrid));
    IFC(DetachHandler(m_epControlPanelGotFocusHandler, m_tpControlPanelGrid));
    IFC(DetachHandler(m_epControlPanelLostFocusHandler, m_tpControlPanelGrid));

    IFC(DetachHandler(m_epBorderSizeChangedHandler, m_tpControlPanelVisibilityBorder));
    IFC(DetachHandler(m_visibilityStateChangedEventHandler, m_tpVisibilityStatesGroup));

    // Release Position Timer's event handlers
    IFC(DetachHandler(m_epPositionUpdateTimerTickHandler, m_tpPositionUpdateTimer));

    // Release Control Panel Hide Timer's event handlers
    IFC(DetachHandler(m_epHideControlPanelTimerTickHandler, m_tpHideControlPanelTimer));

    // Release Pointer Move Timer's event handlers
    IFC(DetachHandler(m_epPointerMoveEndTimerTickHandler, m_tpPointerMoveEndTimer));

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
         IFC(DeinitializeTransportControlsFromMPE());
    }

    IFC(DetachHandler(m_epFlyoutOpenedHandler, m_tpVolumeFlyout));
    IFC(DetachHandler(m_epFlyoutClosedHandler, m_tpVolumeFlyout));

    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
    {
        xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
    }

    IFC(RemoveRegistrationCoreWindowKeyDown());

    // Clean up any existing template parts
    m_tpControlPanelGrid.Clear();
    m_tpMediaPositionSlider.Clear();
    m_tpHorizontalVolumeSlider.Clear();
    m_tpVerticalVolumeSlider.Clear();
    m_tpActiveVolumeSlider.Clear();
    m_tpTHVolumeSlider.Clear();
    m_tpDownloadProgressIndicator.Clear();
    m_tpBufferingProgressBar.Clear();
    m_tpPlayPauseButton.Clear();
    m_tpTHLeftSidePlayPauseButton.Clear();
    m_tpAudioSelectionButton.Clear();
    m_tpTHAudioTrackSelectionButton.Clear();
    m_tpCCSelectionButton.Clear();
    m_tpPlaybackRateButton.Clear();
    m_tpAvailableAudioTracksMenuFlyout.Clear();
    m_tpAvailableAudioTracksMenuFlyoutTarget.Clear();
    m_tpAvailableCCTracksMenuFlyout.Clear();
    m_tpAvailablePlaybackRateMenuFlyout.Clear();
    m_tpVideoVolumeButton.Clear();
    m_tpMuteButton.Clear();
    m_tpTHVolumeButton.Clear();
#if false // DISABLE_FULL_WINDOW
    m_tpFullWindowButton.Clear();
#endif // DISABLE_FULL_WINDOW
    m_tpZoomButton.Clear();
    m_tpActiveVolumeButton.Clear();
    m_tpTimeElapsedElement.Clear();
    m_tpTimeRemainingElement.Clear();
    m_tpErrorTextBlock.Clear();
    m_tpPositionUpdateTimer.Clear();
    m_tpPointerMoveEndTimer.Clear();
    m_tpHideVerticalVolumeTimer.Clear();
    m_tpHideControlPanelTimer.Clear();
    m_tpFastForwardButton.Clear();
    m_tpFastRewindButton.Clear();
    m_tpStopButton.Clear();
    m_tpCommandBar.Clear();
    m_tpCastButton.Clear();
    m_tpVolumeFlyout.Clear();
    m_tpSkipForwardButton.Clear();
    m_tpSkipBackwardButton.Clear();
    m_tpNextTrackButton.Clear();
    m_tpPreviousTrackButton.Clear();
    m_tpRepeatButton.Clear();
#if false // DISABLE_COMPACT_OVERLAY
    m_tpCompactOverlayButton.Clear();
#endif // DISABLE_COMPACT_OVERLAY
    m_tpThumbnailImage.Clear();
    m_tpTimeElapsedPreview.Clear();
    m_tpControlPanelVisibilityBorder.Clear();
    m_tpVisibilityStatesGroup.Clear();

    if (m_spCurrentItem && m_trackAddedEventToken.value != 0)
    {
        IFC(m_spCurrentItem->remove_TimedMetadataTracksChanged(m_trackAddedEventToken));
        m_trackAddedEventToken.value = 0;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Helper to get sizes of transport controls components
//       defined as StaticResource's in Xaml.
//
//       Used for vertical volume height and dropout level calculations.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::GetComponentSizeConstants() noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IResourceDictionary> spResourceDictionary;
    ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
    ctl::ComPtr<wf::IPropertyValue> spKeyAsPV;
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<wf::IReference<DOUBLE>> spValueAsDouble;

    IFC(get_Resources(&spResourceDictionary));

    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCControlPanelHeight")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resControlPanelHeight));

    //
    // Sizes used for vertical volume height calculation
    //
    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCVerticalVolumeSliderMinHeight")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resVerticalVolumeSliderMinHeight));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCVerticalVolumeSliderMaxHeight")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resVerticalVolumeSliderMaxHeight));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCVerticalVolumeSliderTopPadding")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resVerticalVolumeSliderTopPadding));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCVerticalVolumeSliderTopGap")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resVerticalVolumeSliderTopGap));

    // Sizes used for dropout level calculation

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCSideMargins")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resSideMargins));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCMediaButtonWidth")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resMediaButtonWidth));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCTimeButtonWidth")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resTimeButtonWidth));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCPositionSliderMinimumWidth")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resPositionSliderMinimumWidth));

    IFC(spPropertyValueFactory->CreateString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MTCHorizontalVolumeSliderWidth")).Get(), &spKeyAsPV));
    IFC(spResourceDictionary.Cast<ResourceDictionary>()->Lookup(spKeyAsPV.Get(), &spValue));
    IFC(spValue.As(&spValueAsDouble));
    IFC(spValueAsDouble->get_Value(&m_resHorizontalVolumeSliderWidth));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Apply a template to the MediaTransportControls
//
//------------------------------------------------------------------------
IFACEMETHODIMP MediaTransportControls::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    if (m_parentType == MTCParent_None || m_wrOwnerParent.Get())
    {
        m_isTemplateApplied = TRUE;
        // Detach any existing handlers
        IFC(DeinitializeTransportControls());

        IFC(MediaTransportControlsGenerated::OnApplyTemplate());

        IFC(HookupPartsAndHandlers());

        // Initialize the visual state
        IFC(InitializeVisualState());

        // Update MediaControl States
        UpdateMediaControlAllStates();

        // Register Keydown Events on the onApplyTemplate.
        IFC(AddRegistrationCoreWindowKeyDown());
    }

Cleanup:
    return hr;
}


_Check_return_ HRESULT MediaTransportControls::UpdateSafeMargins(_In_ bool applySafeMargin)
{
    xaml::Thickness margin = {0};

    if (applySafeMargin)
    {
        ctl::ComPtr<xaml::IResourceDictionary> resources;
        ctl::ComPtr<IInspectable> boxedResourceKey;
        ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
        BOOLEAN doesResourceExist = FALSE;
        IFC_RETURN(get_Resources(&resources));
        IFC_RETURN(resources.As(&resourcesMap));
        IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MediaTransportControlsTitleSafeBounds")).Get(), &boxedResourceKey));
        IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &doesResourceExist));
        // Developer have options to change DesiredBounds or override the MediaTransportControlsTitleSafeBounds resource key to override this.
        if (doesResourceExist)
        {
            ctl::ComPtr<IInspectable> boxedResource;
            IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));
            IFCPTR_RETURN(boxedResource);
            auto thicknessReference = ctl::query_interface_cast<wf::IReference<xaml::Thickness>>(boxedResource.Get());
            IFC_RETURN(thicknessReference->get_Value(&margin));
        }
    }

    if (m_tpControlPanelGrid)
    {
        IFC_RETURN(m_tpControlPanelGrid.Cast<Grid>()->put_Margin(margin));
    }
    return S_OK;
}

_Check_return_ HRESULT MediaTransportControls::UpdateSafeMarginsinFullWindow(_In_ bool applySafeMargin)
{
#if false // DISABLE_FULL_WINDOW
    if (XboxUtility::IsOnXbox())
    {
        wuv::ApplicationViewBoundsMode boundsMode;
        IFC_RETURN(LayoutBoundsChangedHelper::GetDesiredBoundsMode(&boundsMode));
        // Update SafeMargin in useVisible mode in the FullWindow
        if (boundsMode == wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseVisible)
        {
            IFC_RETURN(UpdateSafeMargins(applySafeMargin));
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Obtain references for control components to be
//       manipulated and hook up handlers for events to be handled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::HookupPartsAndHandlers()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spComponentAsDO;
    wrl_wrappers::HString strAutomationName;
    wrl_wrappers::HString strSeparator;
    wrl_wrappers::HString strSkipString;
    wf::TimeSpan positionUpdateTickFrequency;
    wf::TimeSpan controlPanelDisplayTimeoutFrequency;
    wf::TimeSpan pointerMoveEndTimeout;
    ctl::ComPtr<DispatcherTimer> spPositionUpdateTimer;
    ctl::ComPtr<DispatcherTimer> spHideControlPanelTimer;
    ctl::ComPtr<DispatcherTimer> spPointerMoveEndTimer;

    // Get the parts for obtain reference
    // No-op if part doesn't available
    // Also set localized UIA Name and add Tooltip where applicable. Note that
    // both use the same localized text strings.

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"ControlPanelGrid").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpControlPanelGrid, spComponentAsDO.Get());

    // Need to set Title Safe zone for Xbox
    if (m_tpControlPanelGrid && XboxUtility::IsOnXbox())
    {
        wuv::ApplicationViewBoundsMode boundsMode;
        IFC(LayoutBoundsChangedHelper::GetDesiredBoundsMode(&boundsMode));

        // Apply Title safe margins only if desired bounds as UseCoreWindow
        // If we're in UseVisible mode, then the framework is already laying things out to the TV safe bounds so the MTC doesn't need to do anything.
        // In UseCoreWindow, the app has decided they're smart enough to lay out to the screen and will do TV safe bounds themselves - but for MTC
        // they can't do that specifically so thy need the framework to do this anyway.Thus, the only time MTC's code needs to do TV safe correction is in UseCoreWindow.
        if (wuv::ApplicationViewBoundsMode::ApplicationViewBoundsMode_UseCoreWindow == boundsMode)
        {
            IFC(UpdateSafeMargins(true /*apply safe region margin*/));
        }
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"TimeElapsedElement").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTimeElapsedElement, spComponentAsDO.Get());
    if (m_tpTimeElapsedElement)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_TIME_ELAPSED, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTimeElapsedElement.Cast<FrameworkElement>(), strAutomationName));
        IFC(AddTooltip(m_tpTimeElapsedElement.Cast<FrameworkElement>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"TimeRemainingElement").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTimeRemainingElement, spComponentAsDO.Get());
    if (m_tpTimeRemainingElement)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_TIME_REMAINING, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTimeRemainingElement.Cast<FrameworkElement>(), strAutomationName));
        IFC(AddTooltip(m_tpTimeRemainingElement.Cast<FrameworkElement>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"ProgressSlider").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpMediaPositionSlider, spComponentAsDO.Get());
    if (m_tpMediaPositionSlider)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_SEEK, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpMediaPositionSlider.Cast<Slider>(), strAutomationName));
        IFC(AddTooltip(m_tpMediaPositionSlider.Cast<Slider>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"PlayPauseButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpPlayPauseButton, spComponentAsDO.Get());
    if (m_tpPlayPauseButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_PLAY, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPlayPauseButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpPlayPauseButton.Cast<ButtonBase>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"PlayPauseButtonOnLeft").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTHLeftSidePlayPauseButton, spComponentAsDO.Get());
    if (m_tpTHLeftSidePlayPauseButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_PLAY, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>(), strAutomationName));
    }
#if false // DISABLE_FULL_WINDOW
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"FullWindowButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpFullWindowButton, spComponentAsDO.Get());
    if (m_tpFullWindowButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_FULLSCREEN, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpFullWindowButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpFullWindowButton.Cast<ButtonBase>(), strAutomationName));
    }
#endif // DISABLE_FULL_WINDOW
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"ZoomButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpZoomButton, spComponentAsDO.Get());
    if (m_tpZoomButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_ASPECTRATIO, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpZoomButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpZoomButton.Cast<ButtonBase>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"ErrorTextBlock").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpErrorTextBlock, spComponentAsDO.Get());
    if (m_tpErrorTextBlock)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_ERROR, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpErrorTextBlock.Cast<TextBlock>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"MediaControlsCommandBar").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpCommandBar, spComponentAsDO.Get());

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"VolumeFlyout").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpVolumeFlyout, spComponentAsDO.Get());

    // Attach handlers for events we need to track
    IFC(m_epRootUserControlLoadedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml::IRoutedEventArgs* pArgs)
        {
            RRETURN(OnRootUserControlLoaded());
        }));
    if (m_tpPlayPauseButton)
    {
        IFC(m_epPlayPauseButtonClickHandler.AttachEventHandler(m_tpPlayPauseButton.Cast<ButtonBase>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                RRETURN(OnPlayPauseClick());
            }));
    }
    if (m_tpTHLeftSidePlayPauseButton)
    {
        IFC(m_epLeftsidePlayPauseButtonClickHandler.AttachEventHandler(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnPlayPauseClick());
        }));
    }
#if false // DISABLE_FULL_WINDOW
    if (m_tpFullWindowButton)
    {
        IFC(m_epFullWindowButtonClickHandler.AttachEventHandler(m_tpFullWindowButton.Cast<ButtonBase>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                RRETURN(OnFullWindowClick());
            }));
    }
#endif // DISABLE_FULL_WINDOW
    if (m_tpZoomButton)
    {
        IFC(m_epZoomButtonClickHandler.AttachEventHandler(m_tpZoomButton.Cast<ButtonBase>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                RRETURN(OnZoomClick());
            }));
    }
    if (m_tpMediaPositionSlider)
    {
        IFC(m_epProgressSliderSizeChangedHandler.AttachEventHandler(m_tpMediaPositionSlider.Cast<Slider>(),
            [this](IInspectable *pSender, ISizeChangedEventArgs *pArgs)
            {
                RRETURN(OnProgressSliderSizeChanged());
            }));

        IFC(m_epProgressSliderFocusDisengagedHandler.AttachEventHandler(m_tpMediaPositionSlider.AsOrNull<xaml_controls::IControl>().Get(),
            [this](xaml_controls::IControl *pSender, IFocusDisengagedEventArgs *pArgs)
            {
                RRETURN(OnProgressSliderFocusDisengaged());
            }));


        IFC(m_epPositionChangedHandler.AttachEventHandler(m_tpMediaPositionSlider.Cast<Slider>(),
            [this](IInspectable *pSender, xaml_primitives::IRangeBaseValueChangedEventArgs *pArgs)
            {
                RRETURN(OnPositionSliderValueChanged(pSender, pArgs));
            }));

        if (!m_positionSliderPressedHandler)
        {
            m_positionSliderPressedHandler = wrl::Callback<xaml_input::IPointerEventHandler>(
                [this](IInspectable* sender,  xaml_input::IPointerRoutedEventArgs* args) mutable -> HRESULT
                {
                    RRETURN(OnPositionSliderPressed(sender, args));
                });

            PointerPressedEventSourceType* pointerPressedEventSource = nullptr;
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->GetPointerPressedEventSourceNoRef(&pointerPressedEventSource));
            IFC(pointerPressedEventSource->AddHandler(m_positionSliderPressedHandler.Get(), TRUE /* handledEventsToo */));
        }

        if (!m_positionSliderReleasedHandler)
        {
            m_positionSliderReleasedHandler = wrl::Callback<xaml_input::IPointerEventHandler>(
                [this](IInspectable* sender,  xaml_input::IPointerRoutedEventArgs* args) mutable -> HRESULT
                {
                    RRETURN(OnPositionSliderReleased(sender, args));
                });

            PointerReleasedEventSourceType* pointerReleasedEventSource = nullptr;
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->GetPointerReleasedEventSourceNoRef(&pointerReleasedEventSource));
            IFC(pointerReleasedEventSource->AddHandler(m_positionSliderReleasedHandler.Get(), TRUE /* handledEventsToo */));
        }

        if (!m_positionSliderKeyDownHandler)
        {
            m_positionSliderKeyDownHandler = wrl::Callback<xaml_input::IKeyEventHandler>(
                [this](IInspectable* sender,  xaml_input::IKeyRoutedEventArgs* args) mutable -> HRESULT
                {
                    RRETURN(OnPositionSliderKeyDown(sender, args));
                });

            KeyDownEventSourceType* keyDownReleasedEventSource = nullptr;
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->GetKeyDownEventSourceNoRef(&keyDownReleasedEventSource));
            IFC(keyDownReleasedEventSource->AddHandler(m_positionSliderKeyDownHandler.Get(), TRUE /* handledEventsToo */));
        }

        if (!m_positionSliderKeyUpHandler)
        {
            m_positionSliderKeyUpHandler = wrl::Callback<xaml_input::IKeyEventHandler>(
                [this](IInspectable* sender,  xaml_input::IKeyRoutedEventArgs* args) mutable -> HRESULT
                {
                    RRETURN(OnPositionSliderKeyUp(sender, args));
                });

            KeyUpEventSourceType* keyUpReleasedEventSource = nullptr;
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->GetKeyUpEventSourceNoRef(&keyUpReleasedEventSource));
            IFC(keyUpReleasedEventSource->AddHandler(m_positionSliderKeyUpHandler.Get(), TRUE /* handledEventsToo */));
        }

        IFC(ctl::make<DispatcherTimer>(&spPositionUpdateTimer));
        SetPtrValueWithQIOrNull(m_tpPositionUpdateTimer, spPositionUpdateTimer.Get());

        positionUpdateTickFrequency.Duration = static_cast<INT64>(SeekbarPositionUpdateFreqInSecs * static_cast<DOUBLE> (HNSPerSecond));
        IFC(m_tpPositionUpdateTimer.Cast<DispatcherTimer>()->put_Interval(positionUpdateTickFrequency));

        IFC(m_epPositionUpdateTimerTickHandler.AttachEventHandler(m_tpPositionUpdateTimer.Cast<DispatcherTimer>(),
            [this](IInspectable *pSender, IInspectable *pArgs)
        {
            RRETURN(OnPositionUpdateTimerTick());
        }));

        IFC(EnableValueChangedEventThrottlingOnSliderAutomation(true));
    }

    if (m_tpControlPanelGrid)
    {
        IFC(m_epControlPanelEnteredHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
            {
                RRETURN(OnControlPanelEntered());
            }));

        IFC(m_epControlPanelExitedHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
            {
                RRETURN(OnControlPanelExited());
            }));

        IFC(m_epControlPanelCaptureLostHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
            {
                RRETURN(OnControlPanelCaptureLost(pArgs));
            }));

        IFC(m_epControlPanelGotFocusHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable *pSender, xaml::IRoutedEventArgs *pArgs)
            {
                RRETURN(OnControlPanelGotFocus(pArgs));
            }));

        IFC(m_epControlPanelLostFocusHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable *pSender, xaml::IRoutedEventArgs *pArgs)
            {
                RRETURN(OnControlPanelLostFocus(pArgs));
            }));

        IFC(ctl::make<DispatcherTimer>(&spHideControlPanelTimer));
        SetPtrValueWithQIOrNull(m_tpHideControlPanelTimer, spHideControlPanelTimer.Get());

        controlPanelDisplayTimeoutFrequency.Duration = static_cast<INT64>(ControlPanelDisplayTimeoutInSecs * static_cast<DOUBLE> (HNSPerSecond));
        IFC(m_tpHideControlPanelTimer.Cast<DispatcherTimer>()->put_Interval(controlPanelDisplayTimeoutFrequency));

        IFC(m_epHideControlPanelTimerTickHandler.AttachEventHandler(m_tpHideControlPanelTimer.Cast<DispatcherTimer>(),
            [this](IInspectable *pSender, IInspectable *pArgs)
        {
            RRETURN(OnHideControlPanelTimerTick());
        }));

        // Look for TranslateVertical Transform, if exist then need to clip the border on the size change.
        ctl::ComPtr<IInspectable> spTranformAsII;
        ctl::ComPtr<IFrameworkElement> spControlPanelGridAsIFE;
        IFC(m_tpControlPanelGrid.As(&spControlPanelGridAsIFE));
        IFC(spControlPanelGridAsIFE->FindName(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"TranslateVertical")).Get(),
                            &spTranformAsII));
        if (spTranformAsII)
        {
            IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"ControlPanel_ControlPanelVisibilityStates_Border").Get(), &spComponentAsDO));
            SetPtrValueWithQIOrNull(m_tpControlPanelVisibilityBorder, spComponentAsDO.Get());
            if (m_tpControlPanelVisibilityBorder)
            {
                IFC(m_epBorderSizeChangedHandler.AttachEventHandler(m_tpControlPanelVisibilityBorder.Cast<Border>(),
                    [this](IInspectable *pSender, IInspectable *pArgs)
                {
                    RRETURN(OnBorderSizeChanged());
                }));
            }
        }
    }

    IFC(m_epRootExitedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnRootExited());
        }));

    IFC(m_epRootPressedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnRootPressed());
        }));

    IFC(m_epRootReleasedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnRootReleased());
        }));

    IFC(m_epRootCaptureLostHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnRootCaptureLost());
        }));

    IFC(m_epRootMovedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnRootMoved());
        }));


    IFC(m_epSizeChangedHandler.AttachEventHandler(this,
        [this](IInspectable *pSender, IInspectable *pArgs)
        {
            RRETURN(OnSizeChanged());
        }));

    if (m_tpCommandBar)
    {
        // Attach handlers for CommandBar Loaded
        IFC(m_epCommandBarLoadedHandler.AttachEventHandler(m_tpCommandBar.Cast<CommandBar>(),
            [this](IInspectable *pSender, xaml::IRoutedEventArgs* pArgs)
        {
            RRETURN(OnCommandBarLoaded());
        }));
    }

    if (m_tpVolumeFlyout)
    {
        // Attach handler for Flyout Opened
        IFC(m_epFlyoutOpenedHandler.AttachEventHandler(m_tpVolumeFlyout.Cast<FlyoutBase>(),
            [this](IInspectable* pSender, IInspectable* pArgs)
        {
            // if volume flyout open, we should not hide MTC panel
            m_isFlyoutOpen = TRUE;
            return S_OK;
        }));
        // Attach handler for Flyout Closed
        IFC(m_epFlyoutClosedHandler.AttachEventHandler(m_tpVolumeFlyout.Cast<FlyoutBase>(),
            [this](IInspectable* pSender, IInspectable* pArgs)
        {
            m_isFlyoutOpen = FALSE;
            HideControlPanel();
            return S_OK;
        }));

        IFC(m_tpVolumeFlyout.Cast<FlyoutBase>()->put_ShouldConstrainToRootBounds(TRUE));
    }

    IFC(ctl::make<DispatcherTimer>(&spPointerMoveEndTimer));
    SetPtrValueWithQIOrNull(m_tpPointerMoveEndTimer, spPointerMoveEndTimer.Get());

    pointerMoveEndTimeout.Duration = 0;
    IFC(m_tpPointerMoveEndTimer.Cast<DispatcherTimer>()->put_Interval(pointerMoveEndTimeout));

    IFC(m_epPointerMoveEndTimerTickHandler.AttachEventHandler(m_tpPointerMoveEndTimer.Cast<DispatcherTimer>(),
        [this](IInspectable *pSender, IInspectable *pArgs)
    {
        m_isPointerMove = FALSE;
        if (m_tpPointerMoveEndTimer)
        {
            m_tpPointerMoveEndTimer->Stop();
        }
        RRETURN(StartControlPanelHideTimer());
    }));

    IFC(HookupVolumeAndProgressPartsAndHandlers());

    if (m_tpControlPanelGrid)
    {
        IFC(m_epControlPanelTappedHandler.AttachEventHandler(m_tpControlPanelGrid.Cast<Grid>(),
            [this](IInspectable* pSender, xaml_input::ITappedRoutedEventArgs* pArgs)
            {
                RRETURN(OnControlPanelTapped(pArgs));
            }));
    }

    IFC(GetComponentSizeConstants());
    IFC(MoreControls());
    IFC(SetTabIndex());

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::HookupVolumeAndProgressPartsAndHandlers()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spComponentAsDO;
    wrl_wrappers::HString strAutomationName;
    ctl::ComPtr<DispatcherTimer> spHideVerticalVolumeTimer;
    ctl::ComPtr<xaml_controls::IButton> spSeekButton;

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"HorizontalVolumeSlider").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpHorizontalVolumeSlider, spComponentAsDO.Get());
    if (m_tpHorizontalVolumeSlider)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_VOLUME, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpHorizontalVolumeSlider.Cast<Slider>(), strAutomationName));
        IFC(AddTooltip(m_tpHorizontalVolumeSlider.Cast<Slider>(), strAutomationName));
    }


    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"VerticalVolumeSlider").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpVerticalVolumeSlider, spComponentAsDO.Get());
    if (m_tpVerticalVolumeSlider)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_VOLUME, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpVerticalVolumeSlider.Cast<Slider>(), strAutomationName));
        IFC(AddTooltip(m_tpVerticalVolumeSlider.Cast<Slider>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"VolumeSlider").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTHVolumeSlider, spComponentAsDO.Get());
    if (m_tpTHVolumeSlider)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_VOLUME, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTHVolumeSlider.Cast<Slider>(), strAutomationName));
        IFC(AddTooltip(m_tpTHVolumeSlider.Cast<Slider>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"AudioSelectionButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpAudioSelectionButton, spComponentAsDO.Get());
    if (m_tpAudioSelectionButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_AUDIO_SELECTION, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpAudioSelectionButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpAudioSelectionButton.Cast<Button>(), strAutomationName));
    }

    // Audio Track Selection for Threshold and Flyout attached to it.
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"AudioTracksSelectionButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTHAudioTrackSelectionButton, spComponentAsDO.Get());
    if (m_tpTHAudioTrackSelectionButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_AUDIO_SELECTION, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTHAudioTrackSelectionButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpTHAudioTrackSelectionButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"AvailableAudioTracksMenuFlyout").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpAvailableAudioTracksMenuFlyout, spComponentAsDO.Get());

    if (m_tpAvailableAudioTracksMenuFlyout)
    {
        IFC(m_tpAvailableAudioTracksMenuFlyout.Cast<MenuFlyout>()->put_ShouldConstrainToRootBounds(TRUE));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"AvailableAudioTracksMenuFlyoutTarget").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpAvailableAudioTracksMenuFlyoutTarget, spComponentAsDO.Get());

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"CCSelectionButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpCCSelectionButton, spComponentAsDO.Get());
    if (m_tpCCSelectionButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_CC_SELECTION, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpCCSelectionButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpCCSelectionButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"PlaybackRateButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpPlaybackRateButton, spComponentAsDO.Get());
    if (m_tpPlaybackRateButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_PLAYBACKRATE, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPlaybackRateButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpPlaybackRateButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"VolumeButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpVideoVolumeButton, spComponentAsDO.Get());
    if (m_tpVideoVolumeButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_VOLUME, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpVideoVolumeButton.Cast<ToggleButton>(), strAutomationName));
        IFC(AddTooltip(m_tpVideoVolumeButton.Cast<ToggleButton>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"AudioMuteButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpMuteButton, spComponentAsDO.Get());
    if (m_tpMuteButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_MUTE, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpMuteButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpMuteButton.Cast<ButtonBase>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"VolumeMuteButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpTHVolumeButton, spComponentAsDO.Get());
    if (m_tpTHVolumeButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_MUTE, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTHVolumeButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpTHVolumeButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"BufferingProgressBar").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpBufferingProgressBar, spComponentAsDO.Get());
    if (m_tpBufferingProgressBar)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_BUFFERING_PROGRESS, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpBufferingProgressBar.Cast<Slider>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"FastForwardButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpFastForwardButton, spComponentAsDO.Get());
    if (m_tpFastForwardButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_FASTFORWARD, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpFastForwardButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpFastForwardButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"RewindButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpFastRewindButton, spComponentAsDO.Get());
    if (m_tpFastRewindButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_REWIND, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpFastRewindButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpFastRewindButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"StopButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpStopButton, spComponentAsDO.Get());
    if (m_tpStopButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_STOP, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpStopButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpStopButton.Cast<Button>(), strAutomationName));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"CastButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpCastButton, spComponentAsDO.Get());
    if (m_tpCastButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_CAST, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpCastButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpCastButton.Cast<Button>(), strAutomationName));
    }

    if (m_tpAudioSelectionButton)
    {
        IFC(m_epAudioSelectionButtonClickHandler.AttachEventHandler(m_tpAudioSelectionButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnAudioSelectionButtonClick());
        }));
    }

    if (m_tpTHAudioTrackSelectionButton)
    {
        IFC(m_epAudioTrackSelectionButtonClickHandler.AttachEventHandler(m_tpTHAudioTrackSelectionButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnTHAudioTrackSelectionButtonClick());
        }));
    }

    if (m_tpCCSelectionButton)
    {
        IFC(m_epCCSelectionButtonClickHandler.AttachEventHandler(m_tpCCSelectionButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnCCSelectionButtonClick());
        }));
    }

    if (m_tpPlaybackRateButton)
    {
        IFC(m_epPlaybackRateButtonClickHandler.AttachEventHandler(m_tpPlaybackRateButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnPlaybackRateButtonClick());
        }));
    }

    if (m_tpVideoVolumeButton)
    {

        IFC(m_epVolumeButtonClickHandler.AttachEventHandler(m_tpVideoVolumeButton.Cast<ToggleButton>(),
        [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnVolumeClick());
        }));

        IFC(m_epVolumeButtonGotFocusHandler.AttachEventHandler(m_tpVideoVolumeButton.Cast<ToggleButton>(),
            [this](IInspectable *pSender, xaml::IRoutedEventArgs *pArgs)
        {
            RRETURN(OnVolumeButtonGotFocus(pArgs));
        }));
    }

    if (m_tpMuteButton)
    {
        IFC(m_epAudioMuteButtonClickHandler.AttachEventHandler(m_tpMuteButton.Cast<ButtonBase>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnMuteClick());
        }));
    }

    if (m_tpHorizontalVolumeSlider)
    {

        IFC(m_epHorizontalVolumeChangedHandler.AttachEventHandler(m_tpHorizontalVolumeSlider.Cast<Slider>(),
            [this](IInspectable *pSender, xaml_primitives::IRangeBaseValueChangedEventArgs *pArgs)
        {
            RRETURN(OnVolumeSliderValueChanged());
        }));
    }

    if (m_tpVerticalVolumeSlider)
    {
        IFC(m_epVerticalVolumeChangedHandler.AttachEventHandler(m_tpVerticalVolumeSlider.Cast<Slider>(),
            [this](IInspectable *pSender, xaml_primitives::IRangeBaseValueChangedEventArgs *pArgs)
        {
            RRETURN(OnVolumeSliderValueChanged());
        }));
    }

    if (m_tpTHVolumeSlider)
    {

        IFC(m_epVolumeChangedHandler.AttachEventHandler(m_tpTHVolumeSlider.Cast<Slider>(),
            [this](IInspectable *pSender, xaml_primitives::IRangeBaseValueChangedEventArgs *pArgs)
        {
            RRETURN(OnVolumeSliderValueChanged());
        }));

        IFC(m_volumeSliderPointerWheelChangedHandler.AttachEventHandler(m_tpTHVolumeSlider.Cast<Slider>(),
            [this](IInspectable *pSender, xaml_input::IPointerRoutedEventArgs *pArgs)
        {
            RRETURN(OnVolumeSliderPointerWheelChanged(pSender, pArgs));
        }));
    }

    if (m_tpFastForwardButton)
    {
        IFC(m_epFastForwardButtonClickHandler.AttachEventHandler(m_tpFastForwardButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnFastForwardButtonClicked());
        }));
    }

    if (m_tpFastRewindButton)
    {
        IFC(m_epFastRewindButtonClickHandler.AttachEventHandler(m_tpFastRewindButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnFastRewindButtonClicked());
        }));
    }

    if (m_tpStopButton)
    {
        IFC(m_epStopButtonClickHandler.AttachEventHandler(m_tpStopButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnStopButtonClicked());
        }));
    }

    if (m_tpCastButton)
    {
        IFC(m_epCastButtonClickHandler.AttachEventHandler(m_tpCastButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnCastButtonClicked());
        }));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Hookup new Controls for Redstone
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
MediaTransportControls::MoreControls()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spComponentAsDO;
    wrl_wrappers::HString strAutomationName;
    ctl::ComPtr<IVisualStateGroup> spVisibilityStatesGroup;

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"SkipForwardButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpSkipForwardButton, spComponentAsDO.Get());
    if (m_tpSkipForwardButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_SKIPFORWARD, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpSkipForwardButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpSkipForwardButton.Cast<Button>(), strAutomationName));

        IFC(m_epSkipForwardButtonClickHandler.AttachEventHandler(m_tpSkipForwardButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(SkipForward());
        }));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"SkipBackwardButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpSkipBackwardButton, spComponentAsDO.Get());
    if (m_tpSkipBackwardButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_SKIPBACKWARD, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpSkipBackwardButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpSkipBackwardButton.Cast<Button>(), strAutomationName));

        IFC(m_epSkipBackwardButtonClickHandler.AttachEventHandler(m_tpSkipBackwardButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(SkipBackward());
        }));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"NextTrackButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpNextTrackButton, spComponentAsDO.Get());
    if (m_tpNextTrackButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_NEXTRACK, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpNextTrackButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpNextTrackButton.Cast<Button>(), strAutomationName));

        IFC(m_epNextTrackButtonClickHandler.AttachEventHandler(m_tpNextTrackButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(NextTrack());
        }));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"PreviousTrackButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpPreviousTrackButton, spComponentAsDO.Get());
    if (m_tpPreviousTrackButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_PREVIOUSTRACK, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPreviousTrackButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpPreviousTrackButton.Cast<Button>(), strAutomationName));

        IFC(m_epPreviousTrackButtonClickHandler.AttachEventHandler(m_tpPreviousTrackButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(PreviousTrack());
        }));
    }

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"RepeatButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpRepeatButton, spComponentAsDO.Get());
    if (m_tpRepeatButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_REPEAT_NONE, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpRepeatButton.Cast<ToggleButton>(), strAutomationName));
        IFC(AddTooltip(m_tpRepeatButton.Cast<ToggleButton>(), strAutomationName));
        IFC(m_epRepeatButtonClickHandler.AttachEventHandler(m_tpRepeatButton.Cast<ToggleButton>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnRepeatButtonClicked());
        }));
    }
#if false // DISABLE_COMPACT_OVERLAY
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"CompactOverlayButton").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpCompactOverlayButton, spComponentAsDO.Get());
    if (m_tpCompactOverlayButton)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_MINIVIEW, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpCompactOverlayButton.Cast<Button>(), strAutomationName));
        IFC(AddTooltip(m_tpCompactOverlayButton.Cast<Button>(), strAutomationName));

        IFC(m_epCompactOverlayButtonClickHandler.AttachEventHandler(m_tpCompactOverlayButton.Cast<Button>(),
            [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
        {
            RRETURN(OnCompactOverlayButtonClicked());
        }));
    }
#endif // DISABLE_COMPACT_OVERLAY
    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"LeftSeparator").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpLeftAppBarSeparator, spComponentAsDO.Get());

    IFC(GetTemplateChild(wrl_wrappers::HStringReference(L"RightSeparator").Get(), &spComponentAsDO));
    SetPtrValueWithQIOrNull(m_tpRightAppBarSeparator, spComponentAsDO.Get());

    // This is specific fix to Movies&TV app(re-template without commandbar)
    // Listen Visibility visual state changes which can trigger through app by re-template. so that we should sync MTC internal state with VisualStates.
    if (!m_tpCommandBar.Get())
    {
        IFC(GetTemplatePart<IVisualStateGroup>(STR_LEN_PAIR(L"ControlPanelVisibilityStates"), spVisibilityStatesGroup.ReleaseAndGetAddressOf()));
        SetPtrValue(m_tpVisibilityStatesGroup, spVisibilityStatesGroup.Get());

        if (m_tpVisibilityStatesGroup)
        {
            IFC(m_visibilityStateChangedEventHandler.AttachEventHandler(m_tpVisibilityStatesGroup.Get(),
                [this](IInspectable* pSender, IVisualStateChangedEventArgs* pArgs)
            {
                RRETURN(OnVisibilityVisualStateChanged(pArgs));
            }));
        }
    }

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Initialize the UI / visual state of the Transport Controls.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::InitializeVisualState()
{
    HRESULT hr = S_OK;

    if (m_wrOwnerParent.Get())
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(InitializeVisualStateFromMPE());
        }

        IFC(InitializeVolume());

        IFC(UpdateRepeatButtonUI());

        // Update UI
        IFC(UpdatePlayPauseUI());
#if false // DISABLE_FULL_WINDOW
        IFC(UpdateFullWindowUI());
#endif // DISABLE_FULL_WINDOW
        IFC(UpdatePositionUI());
        IFC(UpdateDownloadProgressUI());
        IFC(UpdateErrorUI());

        if (m_tpMediaPositionSlider)
        {
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->get_Minimum(&m_positionSliderMinimum));
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->get_Maximum(&m_positionSliderMaximum));
        }

        // We could have switched into or out of audio mode, which changes the controls that are displayed.
        IFC(UpdateAudioSelectionUI());
        IFC(UpdateIsMutedUI());
        IFC(UpdateVolumeUI());

        // ShowControlPanel() calls UpdateVisualState()
        IFC(ShowControlPanel());
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize state specific to video mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::InitializeVideo()
{
    HRESULT hr = S_OK;

    // Set references to video-mode volume parts
    if (m_tpVerticalVolumeSlider)
    {
        SetPtrValue(m_tpActiveVolumeSlider, m_tpVerticalVolumeSlider.Get());
        // Cache slider min/max
        IFC(m_tpVerticalVolumeSlider.Cast<Slider>()->get_Minimum(&m_volumeSliderMinimum));
        IFC(m_tpVerticalVolumeSlider.Cast<Slider>()->get_Maximum(&m_volumeSliderMaximum));
    }

    if (m_tpVideoVolumeButton)
    {
        SetPtrValue(m_tpActiveVolumeButton, m_tpVideoVolumeButton.Get());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize state specific to audio mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::InitializeAudio()
{
    HRESULT hr = S_OK;

    // Set references to audio-mode volume parts
    if (m_tpHorizontalVolumeSlider)
    {
        SetPtrValue(m_tpActiveVolumeSlider, m_tpHorizontalVolumeSlider.Get());
        // Cache slider min/max
        IFC(m_tpHorizontalVolumeSlider.Cast<Slider>()->get_Minimum(&m_volumeSliderMinimum));
        IFC(m_tpHorizontalVolumeSlider.Cast<Slider>()->get_Maximum(&m_volumeSliderMaximum));
    }
    if (m_tpMuteButton)
    {
        ctl::ComPtr<xaml_primitives::IToggleButton> spToggleButton;
        IFC(m_tpMuteButton.As(&spToggleButton));
        SetPtrValue(m_tpActiveVolumeButton, spToggleButton);
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize state specific to Volume in the Threshold.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::InitializeVolume()
{
    HRESULT hr = S_OK;

    // Set references to volume parts
    if (m_tpTHVolumeSlider)
    {
        SetPtrValue(m_tpActiveVolumeSlider, m_tpTHVolumeSlider.Get());
        // Cache slider min/max
        IFC(m_tpTHVolumeSlider.Cast<Slider>()->get_Minimum(&m_volumeSliderMinimum));
        IFC(m_tpTHVolumeSlider.Cast<Slider>()->get_Maximum(&m_volumeSliderMaximum));
    }

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Update all aspects of visual state based on state variables
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateVisualState(_In_ bool bUseTransitions)
{
    HRESULT hr = S_OK;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        IFC(UpdateVisualStateFromMPE(bUseTransitions));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Show (fade in) the vertical volume host border
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::ShowVerticalVolume()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        if (!m_verticalVolumeIsVisible)
        {
            m_verticalVolumeIsVisible = TRUE;
            m_verticalVolumeVisibilityChanged = TRUE;
            IFC(UpdateVisualState());

            // Immediately start the timer to hide vertical volume
            IFC(StartVerticalVolumeHideTimer());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Hide (fade out) the vertical volume host border,
//       provided that pre-requisite conditions for hiding it are met.
//
//       If forceHide is TRUE, hide even if input state conditions are not met.
//       This allows consistent experience in corner cases (for example,
//       vertical volume is up and has keyboard focus, then volume button is pressed).
//       In addition, if the input conditions ever get in a bad state, this
//       forces them to get reset.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::HideVerticalVolume(BOOLEAN forceHide)
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {

        if (ShouldHideVerticalVolume() || forceHide)
        {
            // Reset input state if vertical volume hide is forced
            if (forceHide)
            {
                m_verticalVolumeHasKeyOrProgFocus = FALSE;
            }

            // Vertical volume will now be hidden, so stop its hide timer.
            IFC(StopVerticalVolumeHideTimer());

            m_verticalVolumeIsVisible = FALSE;
            m_verticalVolumeVisibilityChanged = TRUE;

            IFC(UpdateVisualState());
        }
    }

Cleanup:
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Show (fade in) the control panel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::ShowControlPanel()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (!m_controlPanelIsVisible)
        {
            m_controlPanelIsVisible = TRUE;
            if (!m_isVSStateChangeExternal) // Skip if Visual State already happen through external
            {
                m_controlPanelVisibilityChanged = TRUE;
            }
        }

        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(ShowControlPanelFromMPE());
        }

        // Resume position updates now that CP is visible
        IFC(StartPositionUpdateTimer());

        // Immediately start the timer to hide control panel
        IFC(StartControlPanelHideTimer());

        IFC(UpdateVisualState());

        m_isVSStateChangeExternal = FALSE;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Hide (fade out) the control panel, provided that pre-requisite
//       conditions for hiding it are met.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::HideControlPanel(_In_ bool hideImmediately)
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (m_tpHideControlPanelTimer)
        {
            if (hideImmediately || ShouldHideControlPanel() || m_isVSStateChangeExternal)
            {
                // Both CP and Vertical Volume will be hidden, so stop their hide timers.
                IFC(StopControlPanelHideTimer());
                IFC(StopVerticalVolumeHideTimer());

                // Stop position updates now that CP is not visible
                IFC(StopPositionUpdateTimer());

                // Flag vertical volume to hide so that it won't get displayed
                // next time the ControlPanel becomes visible
                if (m_verticalVolumeIsVisible)
                {
                    m_verticalVolumeIsVisible = FALSE;
                    m_verticalVolumeVisibilityChanged = TRUE;
                }

                // Flag control panel itself to hide
                m_controlPanelIsVisible = FALSE;
                if (!m_isVSStateChangeExternal) // Skip if Visual State already happen through external
                {
                    m_controlPanelVisibilityChanged = TRUE;
                }

                if (MTCParent_MediaPlayerElement == m_parentType)
                {
                    IFC(HideControlPanelFromMPE());
                }

                IFC(UpdateVisualState());
            }
        }
        m_shouldDismissControlPanel = FALSE;
        m_isthruScrubber = FALSE;
        m_isVSStateChangeExternal = FALSE;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Loaded event fires on Root UserControl.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootUserControlLoaded()
{
    RRETURN(UpdateDownloadProgressUI());
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for the SizeChanged event on ProgressSlider
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnProgressSliderSizeChanged()
{
    RRETURN(UpdateDownloadProgressUI());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for the SizeChanged event on ProgressSlider
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnProgressSliderFocusDisengaged()
{
    IFC_RETURN(ShowHideThumbnail(FALSE));
    IFC_RETURN(ExitScrubbingMode());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when m_tpHideControlPanelTimer fires.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnHideControlPanelTimerTick()
{
    if (m_transportControlsEnabled)
    {
        if (IsInLiveTree())
        {
            IFC_RETURN(HideControlPanel());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when video-mode volume button gets the focus related to blue
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnVolumeButtonGotFocus(
_In_ xaml::IRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN volumeButtonHasKeyOrProgFocus = FALSE;

    if (m_transportControlsEnabled)
    {
        IFC(HasKeyOrProgFocus(pArgs, &volumeButtonHasKeyOrProgFocus));

        // When volume button is focused, vertical volume host should show up
        if (volumeButtonHasKeyOrProgFocus)
        {
            IFC(StopVerticalVolumeHideTimer());
            IFC(ShowVerticalVolume());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when pointer enters control panel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelEntered()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        m_controlPanelHasPointerOver = TRUE;
        IFC(StopControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when pointer exits control panel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelExited()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        m_controlPanelHasPointerOver = FALSE;
        IFC(StartControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when tap ControlPanel
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelTapped(
    _In_ xaml_input::ITappedRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isControlPanelTapped = FALSE;

    if (m_transportControlsEnabled)
    {
        IFC(CompareWithOriginalSource(static_cast<TappedRoutedEventArgs*>(pArgs), ctl::as_iinspectable(m_tpControlPanelGrid.Cast<Grid>()), &isControlPanelTapped));

        m_shouldDismissControlPanel |= isControlPanelTapped;
        IFC(HideControlPanel());
    }

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when control panel grid (which includes the volume slider host)
//      lost pointer capture. This ensures correct PointerPressed and PointerOver
//      state as those events may not fire while an element has capture.
//      (Mostly this occurs when user pressed down on a control in the panel and
//      drags the cursor off the panel while holding it).
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelCaptureLost(
    _In_ xaml_input::IPointerRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPointWhenCaptureLost;
    ctl::ComPtr<xaml::IUIElement> spControlPanelGridAsUIE;
    wf::Point pointWhenCaptureLost = {};
    BOOLEAN controlPanelGridHasHit = FALSE;

    if (m_transportControlsEnabled )
    {

        //
        // 1. Update PointerPressed state.
        //
        // If capture was lost on control panel, it is safe to say
        // pointer is not pressed over ME, since only the controls
        // making up the panel could possibly take capture.
        m_rootHasPointerPressed = FALSE;

        //
        // 2. Update PointerOver state.
        //
        // If volume slider is dragged with pointer outside the vertical volume host,
        // PointerExited event is not fired, however when pointer is released we will
        // get a CaptureLost event.
        //
        IFC(pArgs->GetCurrentPoint(NULL, &spPointerPointWhenCaptureLost));
        IFC(spPointerPointWhenCaptureLost->get_Position(&pointWhenCaptureLost));

        // Check if control panel grid is still hit
        IFC(m_tpControlPanelGrid.As(&spControlPanelGridAsUIE));
        IFC(HitTestHelper(pointWhenCaptureLost, spControlPanelGridAsUIE.Get(), &controlPanelGridHasHit));

        m_controlPanelHasPointerOver = controlPanelGridHasHit;

        // Kick off timers as needed based on updated PointerOver state
        if (!m_controlPanelHasPointerOver)
        {
            IFC(StartControlPanelHideTimer());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when pointer exited Root UserControl.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootExited()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {

        // If user presses pointer over root then drags it off while holding down
        // we get neither Released nor CaptureLost on the root. Thus, unset
        // m_rootHasPointerPressed whenever pointer leaves root.
        // For consistency, also enforce Pressed is FALSE for vertical volume host.
        m_rootHasPointerPressed = FALSE;

        // If pointer exited the root area, it is no longer over the
        // vertical volume or the control panel, enforce this here.
        m_controlPanelHasPointerOver = FALSE;

        IFC(StartControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user pressed the pointer over Root UserControl.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootPressed()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        m_rootHasPointerPressed = TRUE;
        IFC(StopControlPanelHideTimer());

        // Any click over media area should bring up control panel.
        IFC(ShowControlPanel());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user released pointer that was initially pressed
//      over Root UserControl.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootReleased()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        m_rootHasPointerPressed = FALSE;
        IFC(StartControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Root UserControl lost pointer capture.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootCaptureLost()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        m_rootHasPointerPressed = FALSE;
        IFC(StartControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when pointer moves over the ME's render area.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnRootMoved()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        BOOLEAN isAutoShowHide = false;
        IFC(get_ShowAndHideAutomatically(&isAutoShowHide));
        // Check flags to minimize work in this frequently called handler
        if (!m_isAudioOnly &&
            isAutoShowHide && // ignore if  when auto hide/show is disabled
            !m_controlPanelIsVisible &&
            !m_hasError)
        {
            IFC(ShowControlPanel());
        }
        m_isPointerMove = TRUE;
        // timer to detect when pointer move ends.
        m_tpPointerMoveEndTimer->Stop();
        m_tpPointerMoveEndTimer->Start();
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when m_tpPositionUpdateTimer fires.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionUpdateTimerTick()
{
    if (m_transportControlsEnabled)
    {
        if (IsInLiveTree())
        {
            IFC_RETURN(UpdatePositionUI());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Audio Selection button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnAudioSelectionButtonClick()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        IFC(UpdateAudioSelectionFlyout());
        IFC(m_tpAvailableAudioTracksMenuFlyout.Cast<MenuFlyout>()->ShowAt(m_tpAvailableAudioTracksMenuFlyoutTarget.Get()));
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Audio Selection button gets clicked in Threshold
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnTHAudioTrackSelectionButtonClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MenuFlyout> spNewMenuFlyout;

    if (m_transportControlsEnabled)
    {
        if (!m_tpAvailableAudioTracksMenuFlyout)
        {
            IFC(ctl::make<MenuFlyout>(&spNewMenuFlyout));
            IFC(spNewMenuFlyout->put_ShouldConstrainToRootBounds(TRUE));
            SetPtrValue(m_tpAvailableAudioTracksMenuFlyout, spNewMenuFlyout.Get());
            IFC(m_tpTHAudioTrackSelectionButton.Cast<Button>()->put_Flyout(m_tpAvailableAudioTracksMenuFlyout.Cast<MenuFlyout>()));
        }
        IFC(UpdateAudioSelectionFlyout());
        IFC(m_tpAvailableAudioTracksMenuFlyout.Cast<MenuFlyout>()->ShowAt(m_tpTHAudioTrackSelectionButton.Cast<Button>()));
    }
Cleanup:
    return hr;
}

// Called when Close Captioning Selection button gets clicked
_Check_return_ HRESULT
MediaTransportControls::OnCCSelectionButtonClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MenuFlyout> spNewMenuFlyout;
    MTCTelemetryData data;

    if (m_transportControlsEnabled)
    {
        if (!m_tpAvailableCCTracksMenuFlyout)
        {
            IFC(ctl::make<MenuFlyout>(&spNewMenuFlyout));
            IFC(spNewMenuFlyout->put_ShouldConstrainToRootBounds(TRUE));
            SetPtrValue(m_tpAvailableCCTracksMenuFlyout, spNewMenuFlyout.Get());
            IFC(m_tpCCSelectionButton.Cast<Button>()->put_Flyout(m_tpAvailableCCTracksMenuFlyout.Cast<MenuFlyout>()));
        }
        IFC(UpdateCCSelectionFlyout());
        IFC(m_tpAvailableCCTracksMenuFlyout.Cast<MenuFlyout>()->ShowAt(m_tpCCSelectionButton.Cast<Button>()));
    }

Cleanup:
    data.errCode = hr;
    data.trackId = m_currentTrack; // UpdateCCSelectionFlyout() updates current track as it can be set from outside MTC
    m_AggTelemetry.AddData(MTCTelemetryType::CCButtonClick, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when play rate button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPlaybackRateButtonClick()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MenuFlyout> spNewMenuFlyout;

    if (m_transportControlsEnabled)
    {
        if (!m_tpAvailablePlaybackRateMenuFlyout)
        {
            IFC(ctl::make<MenuFlyout>(&spNewMenuFlyout));
            IFC(spNewMenuFlyout->put_ShouldConstrainToRootBounds(TRUE));
            SetPtrValue(m_tpAvailablePlaybackRateMenuFlyout, spNewMenuFlyout.Get());
            IFC(m_tpPlaybackRateButton.Cast<Button>()->put_Flyout(m_tpAvailablePlaybackRateMenuFlyout.Cast<MenuFlyout>()));
        }
        IFC(UpdatePlaybackRateFlyout());
        IFC(m_tpAvailablePlaybackRateMenuFlyout.Cast<MenuFlyout>()->ShowAt(m_tpPlaybackRateButton.Cast<Button>()));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Audio-mode mute button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnMuteClick()
{
    HRESULT hr = S_OK;
    BOOLEAN isMuted = FALSE;
    MTCTelemetryData data;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(GetMuted(&isMuted));
        // For audio-mode, simply flip the mute value
        isMuted = !isMuted;
        IFC(SetMuted(isMuted));
    }

Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::MuteClick, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Video-mode volume button gets clicked related to blue
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnVolumeClick()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;
    if (m_transportControlsEnabled)
    {
        // For video-mode, volume slider expansion and mute function are
        // controlled by same button, need different logic from audio-mode.

        IFC(SetMuted(m_verticalVolumeIsVisible));

        // Force IsChecked to correct state
        IFC(SetCheckedProperty(m_tpActiveVolumeButton.Get(), m_verticalVolumeIsVisible));

        // Collapsing the volume bar, muting the sound
        if (m_verticalVolumeIsVisible)
        {
            // Vertical volume should always hide if it is up and
            // the button is pressed, regardless of input state.
            IFC(HideVerticalVolume(TRUE /* forceHide */));
        }
        // Expanding the volume bar, unmuting the sound
        else
        {
            IFC(ShowVerticalVolume());
        }
    }

Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::VolumeClick, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Play/Pause button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPlayPauseClick()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(OnPlayPauseFromMPE());
        }
    }

Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::PlayPauseClick, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Full Window toggle button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnFullWindowClick()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;
#if false // DISABLE_FULL_WINDOW

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        m_isFullScreenClicked = TRUE;
        // this state is not valid, as user change the state on tapping fullwindow button.
        if (m_isLaunchedAsFullScreen)
        {
            m_isLaunchedAsFullScreen = FALSE;
        }

        if (!m_isMiniView)
        {
            IFC(SetFullWindow(!m_isFullWindow));
        }
        else
        {
            // When you are in miniView, then first exits the MiniView State.
            // Then update the fullwindow UI.
            m_isMiniView = FALSE;
            IFC(SetMiniView(false));
            IFC(UpdateFullWindowUI());
        }

        m_shouldDismissControlPanel |= (m_isPlaying && !m_isBuffering);
        IFC(HideControlPanel());
    }

Cleanup:
#endif // DISABLE_FULL_WINDOW
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::FullWindowClick, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when hardware Back button gets pressed
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnBackButtonPressedImpl(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    ASSERT(m_isFullWindow);
#endif // DISABLE_FULL_WINDOW
    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
#if false // DISABLE_FULL_WINDOW
        IFC(SetFullWindow(FALSE));
        // only if user changed stretch then restore it.
        if (m_stretchOnFullWindowChanged)
        {
            IFC(SetStretch(m_stretchToRestore));
            m_stretchOnFullWindowChanged = FALSE;
        }
#endif // DISABLE_FULL_WINDOW

        //Handled
        *returnValue = TRUE;
    }
//Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when Zoom toggle button gets clicked
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnZoomClick()
{
    HRESULT hr = S_OK;
    xaml_media::Stretch stretch;
    MTCTelemetryData data;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // Uniform goes to UniformToFill. Everything else goes to Uniform.
        IFC(GetStretch(&stretch));
#if false // DISABLE_FULL_WINDOW
        if (m_isFullWindow && !m_stretchOnFullWindowChanged)
        {
            m_stretchOnFullWindowChanged = TRUE;
            m_stretchToRestore = stretch;
        }
#endif // DISABLE_FULL_WINDOW
        IFC(SetStretch(
            stretch == Stretch_Uniform
            ? Stretch_UniformToFill
            : Stretch_Uniform
            ));
    }
Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::ZoomClick, data);
    return hr;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//       Called when the size of the transport controls changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnSizeChanged()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;

    if (m_transportControlsEnabled)
    {
        IFC(SetMeasureCommandBar());
        // If error is showing, may need to switch between long / short / shortest form
        IFC(UpdateErrorUI());
        IFC(UpdateVisualState());
    }

    // This is arise when clicks the exit fullscreen screen on the title bar, then reset back from the full window
#if false // DISABLE_FULL_WINDOW
    if (m_isFullWindow && m_isFullScreen)
    {
        ctl::ComPtr<wuv::IApplicationView3> spAppView3;
        BOOLEAN fullscreenmode = FALSE;

        IFC(GetFullScreenView(&spAppView3));
        if (spAppView3)
        {
            IFC(spAppView3->get_IsFullScreenMode(&fullscreenmode));
        }
        if (!fullscreenmode)
        {
            if (!m_isFullScreenPending) // if true means still we are not under fullscreen, exit through titlebar doesn't occur still
            {
                if (!m_isMiniView)
                {
                    IFC(OnFullWindowClick());
                }
                else
                {
                    // While switching from Fullscreen to MiniView, just update the fullscreen states.
                    IFC(UpdateFullWindowUI());
                    m_isFullScreen = FALSE;
                }
            }
        }
        else
        {
            // m_isFullScreenPending Complete.
            m_isFullScreenPending = FALSE;

            // Find out if the API is available (currently behind a velocity key)
            ctl::ComPtr<wf::Metadata::IApiInformationStatics> apiInformationStatics;
            IFC(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Metadata_ApiInformation).Get(),
                &apiInformationStatics));

            // we are in full screen, so check for spanning mode
            uint32_t regionCount = 0;

            boolean isPresent = false;
            IFC(apiInformationStatics->IsMethodPresent(
                wrl_wrappers::HStringReference(L"Windows.UI.ViewManagement.ApplicationView").Get(),
                wrl_wrappers::HStringReference(L"GetDisplayRegions").Get(),
                &isPresent));

            if (isPresent)
            {
                // Get regions for current view
                ctl::ComPtr<wuv::IApplicationViewStatics2> applicationViewStatics;
                IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(
                                                        RuntimeClass_Windows_UI_ViewManagement_ApplicationView)
                                                        .Get(),
                                                        &applicationViewStatics));

                ctl::ComPtr<wuv::IApplicationView> applicationView;

                // Get Display Regions doesn't work on Win32 Apps, because there is no
                // application view. For the time being, just don't return an empty vector
                // when running in an unsupported mode.
                if (SUCCEEDED(applicationViewStatics->GetForCurrentView(&applicationView)))
                {
                    ctl::ComPtr<wuv::IApplicationView9> applicationView9;
                    IFC(applicationView.As(&applicationView9));

                    HRESULT hrGetForCurrentView;
                    ctl::ComPtr<wfc::IVectorView<wuwm::DisplayRegion*>> regions;
                    hrGetForCurrentView = applicationView9->GetDisplayRegions(&regions);
                    if (FAILED(hrGetForCurrentView))
                    {
                        // bug 14084372: APIs currently return a failure when there is only one display region.
                        return S_OK;
                    }

                    IFC(regions->get_Size(&regionCount));
                }
            }

            if (regionCount > 1  &&
                !m_isCompact &&
                !m_isSpanningCompactEnabled)
            {
                put_IsCompact(true);
                m_isSpanningCompactEnabled = TRUE;
            }
        }
    }
    else
    {
        // not fullscreen, in spanning compact mode is enabled, reset it
        if(m_isSpanningCompactEnabled)
        {
            put_IsCompact(false);
            m_isSpanningCompactEnabled = FALSE;
        }
    }
#endif // DISABLE_FULL_WINDOW
Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::SizeChanged, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when volume slider is manipulated
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnVolumeSliderValueChanged()
{
    HRESULT hr = S_OK;
    DOUBLE sliderValue = 0.0;
    DOUBLE newVolume = 0.0;


    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // Do not update the DP UpdateUIOnly flag for Volume is set
        if (m_volumeUpdateUIOnly)
        {
            goto Cleanup;
        }

        if (m_isAudioOnly && m_tpHorizontalVolumeSlider)
        {
            IFC(m_tpHorizontalVolumeSlider.Cast<Slider>()->get_Value(&sliderValue));
        }
        else if (!m_isAudioOnly && m_tpVerticalVolumeSlider)
        {
            IFC(m_tpVerticalVolumeSlider.Cast<Slider>()->get_Value(&sliderValue));
        }
        else if (m_tpTHVolumeSlider)
        {
            IFC(m_tpTHVolumeSlider.Cast<Slider>()->get_Value(&sliderValue));
        }

        // Calculate and update target volume
        newVolume = (sliderValue - m_volumeSliderMinimum) / DoubleUtil::Max(1, m_volumeSliderMaximum - m_volumeSliderMinimum);
        IFC(SetVolume(newVolume));

        // Set the Mute State when Volume is zero.
        if (m_tpTHVolumeSlider)
        {
            IFC(SetMuted(newVolume == 0 ? TRUE:FALSE));
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when control panel grid (which includes the volume
//      slider host) got focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelGotFocus(
    _In_ xaml::IRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN controlsHaveKeyOrProgFocus = FALSE;

    if (m_transportControlsEnabled)
    {

        IFC(HasKeyOrProgFocus(pArgs, &controlsHaveKeyOrProgFocus));
        m_controlsHaveKeyOrProgFocus = controlsHaveKeyOrProgFocus;

        if (controlsHaveKeyOrProgFocus)
        {
            BOOLEAN isVerticalVolumeSliderOriginalSource = FALSE;

            IFC(StopControlPanelHideTimer());
            IFC(ShowControlPanel());

            if (m_tpVerticalVolumeSlider)
            {
                // Check if vertical volume slider in particular has the focus
                IFC(CompareWithOriginalSource(pArgs, ctl::as_iinspectable(m_tpVerticalVolumeSlider.Cast<Slider>()), &isVerticalVolumeSliderOriginalSource));

                if (isVerticalVolumeSliderOriginalSource)
                {
                    m_verticalVolumeHasKeyOrProgFocus = TRUE;
                    IFC(StopVerticalVolumeHideTimer());
                }
            }
        }
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when control panel grid (which includes the volume
//      slider host) lost focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnControlPanelLostFocus(
    _In_ xaml::IRoutedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isVerticalVolumeSliderOriginalSource = FALSE;

    if (m_transportControlsEnabled)
    {
        if (m_tpVerticalVolumeSlider)
        {
            // Check if vertical volume slider in particular lost the focus
            IFC(CompareWithOriginalSource(pArgs, ctl::as_iinspectable(m_tpVerticalVolumeSlider.Cast<Slider>()), &isVerticalVolumeSliderOriginalSource));
            if (isVerticalVolumeSliderOriginalSource)
            {
                m_verticalVolumeHasKeyOrProgFocus = FALSE;
                IFC(StartVerticalVolumeHideTimer());
            }
        }

        // We do not know if control panel as a whole lost focus until
        // the element receiving focus fires its GotFocus event. For now,
        // we set m_controlsHaveKeyOrProgFocus to FALSE and kick off
        // the ControlPanel hide timer. If focus moves to another control
        // in the panel, both the flag and the timer will be corrected via
        // OnControlPanelGotFocus().
        m_controlsHaveKeyOrProgFocus = FALSE;
        IFC(StartControlPanelHideTimer());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      When Position slider value changes, we will apply change to UI and
//      update the MediaElement DP.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionSliderValueChanged(
    _In_ IInspectable* pSender,
    _In_ xaml_primitives::IRangeBaseValueChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    DOUBLE newSliderValue = 0.0;
    wf::TimeSpan newMediaPosition;
    BOOLEAN isMediaClosed;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // If slider was updated internally in response to Position DP change,
        // do not update the DP again.
        if (m_positionUpdateUIOnly)
        {
            goto Cleanup;
        }

        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(IsMediaStateClosedFromMPE(&isMediaClosed));
        }

        // If user tried to set the slider while in Closed state or for live content,
        // do not update the DP, but refresh Position UI (snap slider back to 0 position, etc).
        if (isMediaClosed || IsLiveContent())
        {
            IFC(UpdatePositionUI());
            goto Cleanup;
        }

        IFC(pArgs->get_NewValue(&newSliderValue));

        // If NaturalDuration is not known (i.e. is 0), new position also evaluates to 0.
        newMediaPosition.Duration =
            static_cast<INT64>((newSliderValue - m_positionSliderMinimum) / (m_positionSliderMaximum - m_positionSliderMinimum) *
                               static_cast<DOUBLE>(m_naturalDuration.TimeSpan.Duration));

        // Without below call to EnterScrubbingMode(), the order of events for a single Seek using PositionSlider during playback is:
        //
        // ... Content is playing ...
        // 1. Set Position (user seeks, Xaml  MediaTransportControls layer responds to ValueChanged event on position slider, call SetCurrentPlaybackTime())
        // 2. Set PlaybackRate = 0   ( Xaml MTC responds to PointerPressed on position slider, enter scrubbing mode)
        //      (i) Get MF_MEDIA_ENGINE_EVENT_SEEKING
        //     (ii) Get MF_MEDIA_ENGINE_EVENT_SEEKED
        // 3. Set PlaybackRate = 1 ( Xaml MTC responds to PointerReleased on position slider, exits scrubbing mode)
        //
        // Seek is issued before we set the PlaybackRate, This is due to bubble up pattern for raising routed events.
        // Both OnPositionSliderValueChanged() and OnPositionSliderPressed() handlers are called in response to a
        // PointerPressed event on a rectangle inside the slider. Slider implementation listens to event directly on
        // the rectangle, and gets the event first, calling OnPositionSliderValueChanged() as part of handling.
        // Later the event bubble's to MTC OnPositionSliderPressed() handler on the slider.
        //
        // The order above is not intended, but is acceptable for a single seek since in that case we don't really benefit from entering scrubbing mode.
        // However, issuing a seek just before toggling PlaybackRate very frequently reproes the following MediaEngine bug:
        // 289704: When changing rate immediately after seeking in MediaEngine during playback we sometimes end up Paused state unexpectedly
        //
        // To avoid hitting scenario above and work around 289704, we make sure to enter scrub mode before doing the seek.
        // The next call to EnterScrubMode()(via OnPositionSlidePressed()) will just no-op. We will leave scrub mode via
        // OnPositionSliderReleased(), OnPositionSliderKeyUp() or OnPositionSliderFocusDisengaged() as usual.
        IFC(EnterScrubbingMode());
        IFC(SetPosition(newMediaPosition));
        IFC(UpdatePositionUI());
        IFC(FireThumbnailEvent());
        m_isthruScrubber = TRUE;
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user pressed the pointer over position slider.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionSliderPressed(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    MTCTelemetryData data;
    data.errCode = S_OK;
    m_AggTelemetry.AddData(MTCTelemetryType::PositionSliderPressed, data);
    IFC_RETURN(ShowHideThumbnail(TRUE));
    IFC_RETURN(EnterScrubbingMode());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user released pointer that was initially pressed
//      over position slider (Slider takes pointer capture when pressed
//      so it gets this event even if pointer is released outside
//      its hit test area).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionSliderReleased(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    IFC_RETURN(ShowHideThumbnail(FALSE));
    IFC_RETURN(ExitScrubbingMode());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user pressed down a key while slider has focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionSliderKeyDown(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKey originalKey = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    // ignore up & down xbox gamepad keys, those doesn't supports scrubbing on the xbox.
    if (key == wsy::VirtualKey_Left  ||
        (key == wsy::VirtualKey_Down && !XboxUtility::IsGamepadNavigationDown(originalKey)) ||
        key == wsy::VirtualKey_Right ||
        (key == wsy::VirtualKey_Up && !XboxUtility::IsGamepadNavigationUp(originalKey)) ||
        key == wsy::VirtualKey_Home  ||
        key == wsy::VirtualKey_End)
    {
        IFC_RETURN(ShowHideThumbnail(TRUE));
        IFC_RETURN(EnterScrubbingMode());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when user released a key while slider has focus.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPositionSliderKeyUp(
    _In_ IInspectable* pSender,
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    wsy::VirtualKey key = wsy::VirtualKey_None;
    wsy::VirtualKey originalKey = wsy::VirtualKey_None;

    IFC_RETURN(pArgs->get_Key(&key));
    IFC_RETURN(static_cast<KeyRoutedEventArgs*>(pArgs)->get_OriginalKey(&originalKey));

    // ignore up & down xbox gamepad keys, those doesn't supports scrubbing on the xbox.
    if (key == wsy::VirtualKey_Left  ||
        (key == wsy::VirtualKey_Down && !XboxUtility::IsGamepadNavigationDown(originalKey)) ||
        key == wsy::VirtualKey_Right ||
        (key == wsy::VirtualKey_Up && !XboxUtility::IsGamepadNavigationUp(originalKey)) ||
        key == wsy::VirtualKey_Home  ||
        key == wsy::VirtualKey_End)
    {
        IFC_RETURN(ShowHideThumbnail(FALSE));
        IFC_RETURN(ExitScrubbingMode());
    }

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when an audio track is selected from the Audio Track MenuFlyout
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnAudioTrackClicked(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase> spItem;
    UINT selectedTrackIndex = 0;
    BOOLEAN isFound = FALSE;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(m_tpAvailableAudioTracksMenuFlyout->get_Items(&spMenuFlyoutItems));

        // Determine index of currently selected audio track
        IFC(ctl::do_query_interface(spItem, pSender));
        IFC(spMenuFlyoutItems->IndexOf(spItem.Get(), &selectedTrackIndex, &isFound));
        if (isFound)
        {
            // Apply user's track selection
            if (MTCParent_MediaPlayerElement == m_parentType)
            {
                IFC(SetAudioTrackFromMPE(selectedTrackIndex));
            }
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Play/Pause state - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdatePlayPauseUI()
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strAutomationName;
    ctl::ComPtr<xaml_primitives::IToggleButton> spToggleButton;

    if (m_transportControlsEnabled)
    {
        if (m_tpPlayPauseButton)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(m_isPlaying ? UIA_MEDIA_PAUSE : UIA_MEDIA_PLAY, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpPlayPauseButton.Cast<ButtonBase>(), strAutomationName));
            IFC(UpdateTooltipText(m_tpPlayPauseButton.Cast<ButtonBase>(), strAutomationName));
        }

        if (m_tpTHLeftSidePlayPauseButton)
        {
            IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(m_isPlaying ? UIA_MEDIA_PAUSE : UIA_MEDIA_PLAY, strAutomationName.ReleaseAndGetAddressOf()));
            IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>(), strAutomationName));
            IFC(UpdateTooltipText(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>(), strAutomationName));
        }

        // Show/Hide PlayPause button based on the CommandManager Behaviour
        if (m_parentType == MTCParent_MediaPlayerElement && m_spMediaPlayer)
        {
            ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayerExt;
            ctl::ComPtr<wmp::IMediaPlaybackCommandManager> spCommandManager;
            BOOLEAN isEnable = FALSE;

            IFC(m_spMediaPlayer.As(&spMediaPlayerExt));
            IFC(spMediaPlayerExt->get_CommandManager(&spCommandManager));
            if (m_isPlaying)
            {
                ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPauseCommandBehaviour;
                IFC(spCommandManager->get_PauseBehavior(&spPauseCommandBehaviour));
                IFC(spPauseCommandBehaviour->get_IsEnabled(&isEnable));
            }
            else
            {
                ctl::ComPtr<wmp::IMediaPlaybackCommandManagerCommandBehavior> spPlayCommandBehaviour;
                IFC(spCommandManager->get_PlayBehavior(&spPlayCommandBehaviour));
                IFC(spPlayCommandBehaviour->get_IsEnabled(&isEnable));
            }
            if (m_tpPlayPauseButton)
            {
                IFC(m_tpPlayPauseButton.Cast<ButtonBase>()->put_IsEnabled(isEnable));
            }
            if (m_tpTHLeftSidePlayPauseButton)
            {
                IFC(m_tpTHLeftSidePlayPauseButton.Cast<Button>()->put_IsEnabled(isEnable));
            }
        }
        // Update Time Elapsed/Remaining Automation value in pause/play state
        IFC(UpdateTimeAutomationProperties());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Audio Selection UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateAudioSelectionUI()
{
    HRESULT hr = S_OK;
    INT audioStreamCount = 0;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if ((m_tpAudioSelectionButton  && m_tpAvailableAudioTracksMenuFlyout) ||
            m_tpTHAudioTrackSelectionButton)
        {
            BOOLEAN hasMultipleAudioStreams = FALSE;
            // audioStreamCount is 0 if there is no valid source
            IFC(GetAudioTrackCount(&audioStreamCount));
            hasMultipleAudioStreams = audioStreamCount > 1;
            if (hasMultipleAudioStreams != m_hasMultipleAudioStreams)
            {
                m_hasMultipleAudioStreams = hasMultipleAudioStreams;
                IFC(UpdateVisualState());
                if (m_tpLeftAppBarSeparator || m_tpRightAppBarSeparator)
                {
                    IFC(SetMeasureCommandBar());
                }
            }
        }
    }

Cleanup:
    return hr;
}

// Update closed caption Selection UI
_Check_return_ HRESULT
MediaTransportControls::UpdateCCSelectionUI()
{
    HRESULT hr = S_OK;
    UINT ccTrackCount = 0;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (m_tpCCSelectionButton)
        {
            BOOLEAN hasCCTracks = FALSE;
            IFC(GetCCTrackCount(&ccTrackCount));
            hasCCTracks = ccTrackCount > 0;
            if (hasCCTracks != m_hasCCTracks)
            {
                m_hasCCTracks = hasCCTracks;
                IFC(UpdateVisualState());
                if (m_tpLeftAppBarSeparator || m_tpRightAppBarSeparator)
                {
                    IFC(SetMeasureCommandBar());
                }
            }
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Full Window state - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateFullWindowUI()
{
#if false // DISABLE_FULL_WINDOW
    wrl_wrappers::HString strAutomationName;
    BOOLEAN isFullWindow = FALSE;
    ctl::ComPtr<xaml_primitives::IToggleButton> spToggleButton;

    if (m_transportControlsEnabled  && m_wrOwnerParent.Get())
    {
        IFC_RETURN(GetFullWindow(&isFullWindow));
        m_isFullWindow = isFullWindow;

        if (m_isFullWindow)
        {
            if (!m_isFullScreenClicked) // This condition arise MediaElement launch as fullwindow, so we can skip register backbutton listener.
            {
                m_isLaunchedAsFullScreen = TRUE;
            }
            else
            {
                // Register Backbutton Listener on FullWindow
                IFC_RETURN(BackButtonIntegration_RegisterListener(this));
            }
        }
        else
        {
            // Deregister Backbutton Listener on non-FullWindow
            IFC_RETURN(BackButtonIntegration_UnregisterListener(this));
        }
        if (m_tpFullWindowButton)
        {
            IFC_RETURN(UpdateFullScreenMode(m_isFullWindow && !m_isMiniView));
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(isFullWindow ? UIA_MEDIA_EXIT_FULLSCREEN : UIA_MEDIA_FULLSCREEN, strAutomationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpFullWindowButton.Cast<ButtonBase>(), strAutomationName));
            IFC_RETURN(UpdateTooltipText(m_tpFullWindowButton.Cast<ButtonBase>(), strAutomationName));
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update IsMuted - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateIsMutedUI()
{
    HRESULT hr = S_OK;
    BOOLEAN isMuted = FALSE;

    if (m_tpMuteButton)
    {
        wrl_wrappers::HString strAutomationName;
        IFC(GetMuted(&isMuted));
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(isMuted ? UIA_MEDIA_UNMUTE : UIA_MEDIA_MUTE, strAutomationName.ReleaseAndGetAddressOf()));
        IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpMuteButton.Cast<ButtonBase>(), strAutomationName));
        IFC(AddTooltip(m_tpMuteButton.Cast<ButtonBase>(), strAutomationName));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Volume level - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateVolumeUI()
{
    HRESULT hr = S_OK;

    if (m_tpActiveVolumeSlider)
    {
        DOUBLE volume = 0;
        DOUBLE targetSliderValue = 0;

        if (m_transportControlsEnabled && m_wrOwnerParent.Get())
        {
            IFC(GetVolume(&volume));
            targetSliderValue = volume * (m_volumeSliderMaximum - m_volumeSliderMinimum) + m_volumeSliderMinimum;

            // Set UpdateUIOnly flag for Volume so that OnVolumeSliderValueChanged() does not try to update the Volume DP
            m_volumeUpdateUIOnly = TRUE;
            IFC(m_tpActiveVolumeSlider.Cast<Slider>()->put_Value(targetSliderValue));
            m_volumeUpdateUIOnly = FALSE;
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Position - related UI (including Remaining Time)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdatePositionUI()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
    wf::TimeSpan currentMediaPosition;
    wrl_wrappers::HString strTimeElapsedText;
    wrl_wrappers::HString strTimeRemainingText;
    INT64 timeElapsedSeconds = 0;
    INT64 timeRemainingSeconds = 0;
    INT64 naturalDurationSeconds = 0;
    DOUBLE targetSliderValue = 0;
    DOUBLE mediaPosRatio = 0;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // TODO junk values for when no source and Changing PlayList enable later when fix it in MediaPlayer
        // ASSERT(m_naturalDuration.TimeSpan.Duration >= 0);

        if (!m_sourceLoaded)
        {
            // If source has not loaded yet, report 0 for all Position UI.
            currentMediaPosition.Duration = 0;
            targetSliderValue = 0.0;
        }
        else
        {
            IFC(GetPosition(&currentMediaPosition));

            if (IsLiveContent())
            {
               targetSliderValue = 0.0;
            }
            else
            {
                mediaPosRatio = static_cast<DOUBLE>(currentMediaPosition.Duration) / static_cast<DOUBLE> (m_naturalDuration.TimeSpan.Duration);
                targetSliderValue = mediaPosRatio * (m_positionSliderMaximum - m_positionSliderMinimum) + m_positionSliderMinimum;
                if (DoubleUtil::IsNaN(targetSliderValue) || DoubleUtil::IsInfinity(targetSliderValue))
                {
                    targetSliderValue = 0;
                }
            }
        }

        if (m_tpMediaPositionSlider)
        {
            // Set UpdateUIOnly flag for Position so that OnPositionSliderValueChanged() does not try to update the Position DP
            m_positionUpdateUIOnly = TRUE;
            IFC(m_tpMediaPositionSlider.Cast<Slider>()->put_Value(targetSliderValue));
            m_positionUpdateUIOnly = FALSE;
        }

        //
        // Get natural duration, elapsed and remaining time in seconds
        //
        // Note: Since these times are displayed in the UI in whole seconds only,
        // truncate the NaturalDuration and TimeElapsed to integer, and obtain
        // TimeRemaining from their difference. This guarantees that the displayed
        // Elapsed and Remaining Times are updated at the same time.
        //
        timeElapsedSeconds = static_cast<INT64> (currentMediaPosition.Duration / HNSPerSecond);

        if (IsLiveContent())
        {
            // Duration and RemainingTime don't apply to live content case
            naturalDurationSeconds = INT64_MAX;
            timeRemainingSeconds = INT64_MAX;
        }
        else
        {
            naturalDurationSeconds = static_cast<INT64>(m_naturalDuration.TimeSpan.Duration / HNSPerSecond);
            timeRemainingSeconds = naturalDurationSeconds - timeElapsedSeconds;
        }

        // Get factory for PropertyValue to wrap string for consumption as Button.Content
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));

        // Update Time Elapsed
        if (m_tpTimeElapsedElement)
        {
            IFC(ConvertSecondsToHString(timeElapsedSeconds, strTimeElapsedText.GetAddressOf()));

            if (ctl::is<xaml_controls::IContentControl>(m_tpTimeElapsedElement.Get()))
            {
                ctl::ComPtr<wf::IPropertyValue> spContentAsPV;
                ctl::ComPtr<xaml_controls::IContentControl> spTimeElapsedButton;
                IFC(m_tpTimeElapsedElement.As(&spTimeElapsedButton));
                IFC(spPropertyValueFactory->CreateString(strTimeElapsedText.Get(), &spContentAsPV));
                IFC(spTimeElapsedButton->put_Content(ctl::as_iinspectable(spContentAsPV.Get())));
            }
            else
            {
                ctl::ComPtr<xaml_controls::ITextBlock> spTimeElapsedTextBlock;
                IFC(m_tpTimeElapsedElement.As(&spTimeElapsedTextBlock));
                IFC(spTimeElapsedTextBlock->put_Text(strTimeElapsedText));
            }
            if (m_isThumbnailEnabled && m_tpTimeElapsedPreview)
            {
                IFC(m_tpTimeElapsedPreview->put_Text(strTimeElapsedText));
            }
        }

        // Update Time Remaining
        if (m_tpTimeRemainingElement)
        {
            if (!m_sourceLoaded || IsLiveContent())
            {
                // NaturalDuration not known yet or is infinite (live content), show blank for Remaining Time
                strTimeRemainingText.Set(L"");
            }
            else
            {
                IFC(ConvertSecondsToHString(timeRemainingSeconds, strTimeRemainingText.GetAddressOf()));
            }

            if (ctl::is<xaml_controls::IContentControl>(m_tpTimeRemainingElement.Get()))
            {
                ctl::ComPtr<wf::IPropertyValue> spContentAsPV;
                ctl::ComPtr<xaml_controls::IContentControl> spTimeRemainingButton;
                IFC(m_tpTimeRemainingElement.As(&spTimeRemainingButton));
                IFC(spPropertyValueFactory->CreateString(strTimeRemainingText.Get(), &spContentAsPV));
                IFC(spTimeRemainingButton->put_Content(ctl::as_iinspectable(spContentAsPV.Get())));
            }
            else
            {
                ctl::ComPtr<xaml_controls::ITextBlock> spTimeRemainingTextBlock;
                IFC(m_tpTimeRemainingElement.As(&spTimeRemainingTextBlock));
                IFC(spTimeRemainingTextBlock->put_Text(strTimeRemainingText));
            }
        }

        if (m_sourceLoaded && !m_isPlaying)
        {
            IFC(UpdateTimeAutomationProperties());
        }
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update DownloadProgress - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateDownloadProgressUI()
{
    HRESULT hr = S_OK;
    DOUBLE downloadProgress = 0.0;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (m_tpMediaPositionSlider)
        {
            // If needed, get reference to DownloadProgressIndicator, which is a part of MediaSlider template
            // TODO: We should be able to do this once in the Loaded event of the MediaSlider or an
            //                         ancestor instead of here. However, the Loaded event in practice fires before
            //                         the template is expanded, which is not expected in Jupiter, according to MSDN doc:
            //                         http://msdn.microsoft.com/en-us/library/windows/apps/windows.ui.xaml.frameworkelement.loaded
            if (!m_tpDownloadProgressIndicator)
            {
                ctl::ComPtr<xaml::IDependencyObject> spTemplatePartAsDO;

                IFC(m_tpMediaPositionSlider.Cast<Slider>()->GetTemplateChild(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"DownloadProgressIndicator")).Get(), &spTemplatePartAsDO));
                SetPtrValueWithQIOrNull(m_tpDownloadProgressIndicator, spTemplatePartAsDO.Get());
                if (!m_tpDownloadProgressIndicator)
                {
                    goto Cleanup;
                }

                wrl_wrappers::HString strAutomationName;
                IFC(DirectUI::AutomationProperties::GetNameStatic(m_tpDownloadProgressIndicator.Cast<Slider>(), strAutomationName.ReleaseAndGetAddressOf()));
                if (strAutomationName.Get() == nullptr)
                {
                    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_DOWNLOAD_PROGRESS, strAutomationName.ReleaseAndGetAddressOf()));
                    IFC(DirectUI::AutomationProperties::SetNameStatic(m_tpDownloadProgressIndicator.Cast<Slider>(), strAutomationName));
                }
            }

            IFC(GetDownloadProgress(&downloadProgress));
            // DownloadProgress allow non-finite values (INF or NaN) as these can arise in live sources and custom MediaSources,
            // which do not care about download progress.
            if (!DoubleUtil::IsNaN(downloadProgress) && !DoubleUtil::IsInfinity(downloadProgress))
            {
                IFC(m_tpDownloadProgressIndicator.Cast<Slider>()->put_Value(downloadProgress * 100.0));
            }
            else
            {
                IFC(m_tpDownloadProgressIndicator.Cast<Slider>()->put_Value(0));
            }
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update Error - related UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateErrorUI()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(UpdateErrorUIFromMPE());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get Resource ID for the appropriate error string to
//      show in the ErrorTextBlock.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::GetErrorResourceID(
    _In_ UINT32 errorCode,
    _Out_ UINT32* pResourceID)
{
    ASSERT(m_hasError && errorCode != MF_MEDIA_ENGINE_ERR_NOERROR);

    switch (errorCode)
    {
        case MF_MEDIA_ENGINE_ERR_ABORTED :
        {
            *pResourceID = m_isAudioOnly ? AG_E_MEDIA_CONTROLS_LONG_ERR_ABORTED_AUDIO : AG_E_MEDIA_CONTROLS_LONG_ERR_ABORTED_VIDEO;
            break;
        }

        case MF_MEDIA_ENGINE_ERR_DECODE :
        {
            *pResourceID = m_isAudioOnly ? AG_E_MEDIA_CONTROLS_LONG_ERR_DECODE_AUDIO : AG_E_MEDIA_CONTROLS_LONG_ERR_DECODE_VIDEO;
            break;
        }

        case MF_MEDIA_ENGINE_ERR_SRC_NOT_SUPPORTED :
        {
            *pResourceID = m_isAudioOnly ? AG_E_MEDIA_CONTROLS_LONG_ERR_SRC_NOT_SUPPORTED_AUDIO : AG_E_MEDIA_CONTROLS_LONG_ERR_SRC_NOT_SUPPORTED_VIDEO;
            break;
        }

        case MF_MEDIA_ENGINE_ERR_NETWORK :
        {
            *pResourceID = m_isAudioOnly ? AG_E_MEDIA_CONTROLS_LONG_ERR_NETWORK_AUDIO : AG_E_MEDIA_CONTROLS_LONG_ERR_NETWORK_VIDEO;
            break;
        }

        default :
        {
            *pResourceID = AG_E_MEDIA_CONTROLS_LONG_ERR_DEFAULT;
            break;
        }
    }

    return S_OK;//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Refresh the position UI immediately, then start the position
//      update timer, if needed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StartPositionUpdateTimer()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {

        // Refresh position immediately to avoid a lag in position reporting and ensure
        // latest value even if condition to kick off timer below is not met
        IFC(OnPositionUpdateTimerTick());

        // Timer updates are only needed if content is playing and the Control Panel is visible
        if (m_tpPositionUpdateTimer &&
            m_controlPanelIsVisible &&
            m_isPlaying)
        {
            IFC(m_tpPositionUpdateTimer->Start());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Refresh the position UI immediately, then stop the position
//      update timer.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StopPositionUpdateTimer()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        // Refresh position immediately to ensure final value is the latest
        IFC(OnPositionUpdateTimerTick());

        if (m_tpPositionUpdateTimer)
        {
            IFC(m_tpPositionUpdateTimer->Stop());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Start timer responsible for vertical volume host fadeout,
//      provided that pre-requisite conditions for hiding it are met.
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StartVerticalVolumeHideTimer()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        if (m_tpHideVerticalVolumeTimer &&
            ShouldHideVerticalVolume())
        {
            IFC(m_tpHideVerticalVolumeTimer->Start());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stop timer responsible for vertical volume host fadeout
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StopVerticalVolumeHideTimer()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled)
    {
        if (m_tpHideVerticalVolumeTimer)
        {
            IFC(m_tpHideVerticalVolumeTimer->Stop());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Start timer responsible for control panel fadeout,
//      provided that pre-requisite conditions for hiding it are met.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StartControlPanelHideTimer()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled)
    {
        if (m_tpHideControlPanelTimer &&
            ShouldHideControlPanel())
        {
            IFC(m_tpHideControlPanelTimer->Start());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stop timer responsible for control panel fadeout
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::StopControlPanelHideTimer()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled)
    {
        if (m_tpHideControlPanelTimer)
        {
            IFC(m_tpHideControlPanelTimer->Stop());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Helper to set IsChecked property of a ToggleButton or blue
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::SetCheckedProperty(
_In_opt_ xaml_primitives::IToggleButton *pToggleButton,
_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spNewIsCheckedValue;
    ctl::ComPtr<wf::IReference<bool>> spNewIsCheckedValueReference;

    if (pToggleButton)
    {
        IFC(PropertyValue::CreateFromBoolean(value, &spNewIsCheckedValue));
        IFC(spNewIsCheckedValue.As(&spNewIsCheckedValueReference));
        IFC(pToggleButton->put_IsChecked(spNewIsCheckedValueReference.Get()));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Helper to add Tooltip to a component control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::AddTooltip(
    _In_ xaml::IDependencyObject* pTooltipTarget,
    _In_ HSTRING hstrTooltipText)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ToolTip> spToolTip;
    ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
    ctl::ComPtr<wf::IPropertyValue> spContentAsPV;

    IFC(ctl::make<ToolTip>(&spToolTip));

    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));
    IFC(spPropertyValueFactory->CreateString(hstrTooltipText, &spContentAsPV));
    IFC(spToolTip->put_Content(ctl::as_iinspectable(spContentAsPV.Get())));

    IFC(ToolTipServiceFactory::SetToolTipStatic(pTooltipTarget, ctl::as_iinspectable(spToolTip.Get())));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Helper to update an existing Tooltip text of a component control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateTooltipText(
    _In_ xaml::IDependencyObject* pTooltipTarget,
    _In_ HSTRING hstrTooltipText)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IInspectable> spToolTipAsInsp;
    ctl::ComPtr<xaml_controls::IToolTip> spToolTip;
    ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
    ctl::ComPtr<wf::IPropertyValue> spContentAsPV;

    IFC(ToolTipServiceFactory::GetToolTipStatic(pTooltipTarget, &spToolTipAsInsp));
    if (spToolTipAsInsp)
    {
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(), spPropertyValueFactory.ReleaseAndGetAddressOf()));
        IFC(spPropertyValueFactory->CreateString(hstrTooltipText, &spContentAsPV));
        IFC(spToolTipAsInsp.As(&spToolTip));
        IFC(spToolTip.Cast<ToolTip>()->put_Content(ctl::as_iinspectable(spContentAsPV.Get())));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update the list of tracks in AvailableAudioTracksMenuFlyout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateAudioSelectionFlyout()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(UpdateAudioSelectionFlyoutFromMPE());
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release the Click Handlers associated with each MenuFlyoutItem
//      in AvailableAudioTracksMenuFlyout.
//
//------------------------------------------------------------------------
void
MediaTransportControls::ReleaseMenuFlyoutItemClickHandlers()
{
    std::vector<EventPtr<MenuFlyoutItemClickEventCallback>*>::const_iterator iter;

    if (!m_audioTrackClickHandlers.empty())
    {
        // Delete each event pointer.
        for(iter = m_audioTrackClickHandlers.begin(); iter != m_audioTrackClickHandlers.end(); iter++)
        {
            EventPtr<MenuFlyoutItemClickEventCallback>* pEventPtr = *iter;
            delete pEventPtr;
        }

        // Clear the std::vector
        m_audioTrackClickHandlers.clear();
    }
}

// Update the list of tracks in AvailableCCTracksMenuFlyout.
_Check_return_ HRESULT
MediaTransportControls::UpdateCCSelectionFlyout()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(UpdateCCSelectionFlyoutFromMPE());
        }
    }

Cleanup:
    return hr;
}

// Helper that create the Flyout item for the menu items
_Check_return_ HRESULT
MediaTransportControls::CreateCCFlyoutTrack(_In_ HSTRING strLabelTemp, _In_ int id, _In_ int idx)
{
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    wrl_wrappers::HString strProcessedLabel;
    ctl::ComPtr<MenuFlyoutItem> spNewMenuFlyoutItem;

    IFC_RETURN(m_tpAvailableCCTracksMenuFlyout->get_Items(&spMenuFlyoutItems));

    if (m_currentTrack == id)
    {
        IFC_RETURN(MarkLanguageSelection(strLabelTemp, strProcessedLabel.ReleaseAndGetAddressOf()));
    }
    else
    {
        strProcessedLabel.Set(strLabelTemp);
    }

    m_trackIdMappings[idx] = id;

    IFC_RETURN(ctl::make<MenuFlyoutItem>(&spNewMenuFlyoutItem));

    IFC_RETURN(spNewMenuFlyoutItem->put_Text(strProcessedLabel));
    IFC_RETURN(spMenuFlyoutItems->Append(spNewMenuFlyoutItem.Get()));

    // Create event handler via EventPtr for this MenuFlyoutItem and add it to m_audioTrackClickHandlers
    EventPtr<MenuFlyoutItemClickEventCallback>* pMenuFlyoutItemClickHandler = new EventPtr<MenuFlyoutItemClickEventCallback>();

    IFC_RETURN(pMenuFlyoutItemClickHandler->AttachEventHandler(spNewMenuFlyoutItem.Get(),
        [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
    {
        return OnCCTrackClicked(pSender, pArgs);
    }));

    m_CCTrackClickHandlers.push_back(pMenuFlyoutItemClickHandler);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release the Click Handlers associated with each MenuFlyoutItem
//      in AvailableCCTracksMenuFlyout.
//
//------------------------------------------------------------------------
void
MediaTransportControls::ReleaseCCSelectionMenuFlyoutItemClickHandlers()
{
    std::vector<EventPtr<MenuFlyoutItemClickEventCallback>*>::const_iterator iter;

    if (!m_CCTrackClickHandlers.empty())
    {
        // Delete each event pointer.
        for (iter = m_CCTrackClickHandlers.begin(); iter != m_CCTrackClickHandlers.end(); iter++)
        {
            EventPtr<MenuFlyoutItemClickEventCallback>* pEventPtr = *iter;
            delete pEventPtr;
        }

        // Clear the std::vector
        m_CCTrackClickHandlers.clear();
        m_trackIdMappings.clear();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update the play rate menu in AvailablePlaybackRateMenuFlyout.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdatePlaybackRateFlyout()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        double playbackRate;
        unsigned int i = 0;
        BOOLEAN selectItemCompleted = FALSE;
        IFC(m_tpAvailablePlaybackRateMenuFlyout->get_Items(&spMenuFlyoutItems));

        // Clear all tracks and associated click event handlers
        IFC(spMenuFlyoutItems->Clear());
        ReleasePlaybackRateMenuFlyoutItemClickHandlers();

        m_currentPlaybackRates.reserve(AvailablePlaybackRateCount + 1);
        IFC(GetPlaybackRate(&playbackRate));

        while(i < AvailablePlaybackRateCount)
        {
            wrl_wrappers::HString strLabel;
            WCHAR szLabelBuffer[5];
            wrl_wrappers::HString strProcessedLabel;

            if (!selectItemCompleted && playbackRate <= AvailablePlaybackRateList[i])
            {
                IFCEXPECT(swprintf_s(szLabelBuffer, ARRAYSIZE(szLabelBuffer), L"%.2f", playbackRate) >= 0);
                IFC(strLabel.Set(szLabelBuffer));
                IFC(MarkLanguageSelection(strLabel.Get(), strProcessedLabel.ReleaseAndGetAddressOf()));
                m_currentPlaybackRates.push_back(playbackRate);
                if (playbackRate == AvailablePlaybackRateList[i])
                {
                    i++;
                }
                selectItemCompleted = TRUE;
            }
            else
            {
                IFCEXPECT(swprintf_s(szLabelBuffer, ARRAYSIZE(szLabelBuffer), L"%.2f", AvailablePlaybackRateList[i]) >= 0);
                IFC(strProcessedLabel.Set(szLabelBuffer));
                m_currentPlaybackRates.push_back(AvailablePlaybackRateList[i]);
                i++;
            }

            ctl::ComPtr<MenuFlyoutItem> spNewMenuFlyoutItem;
            IFC(ctl::make<MenuFlyoutItem>(&spNewMenuFlyoutItem));

            IFC(spNewMenuFlyoutItem->put_Text(strProcessedLabel.Get()));
            IFC(spMenuFlyoutItems->Append(spNewMenuFlyoutItem.Get()));

            // Create event handler via EventPtr for this MenuFlyoutItem and add it to m_playbackRateClickHandlers
            EventPtr<MenuFlyoutItemClickEventCallback>* pMenuFlyoutItemClickHandler = new EventPtr<MenuFlyoutItemClickEventCallback>();

            IFC(pMenuFlyoutItemClickHandler->AttachEventHandler(spNewMenuFlyoutItem.Get(),
                [this](IInspectable* pSender, IRoutedEventArgs* pArgs)
            {
                return OnPlaybackRateMenuClicked(pSender, pArgs);
            }));

            m_playbackRateClickHandlers.push_back(pMenuFlyoutItemClickHandler);
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Release the Click Handlers associated with each MenuFlyoutItem
//      in AvailablePlaybackRateMenuFlyout.
//
//------------------------------------------------------------------------
void
MediaTransportControls::ReleasePlaybackRateMenuFlyoutItemClickHandlers()
{
    std::vector<EventPtr<MenuFlyoutItemClickEventCallback>*>::const_iterator iter;

    if (!m_playbackRateClickHandlers.empty())
    {
        // Delete each event pointer.
        for (iter = m_playbackRateClickHandlers.begin(); iter != m_playbackRateClickHandlers.end(); iter++)
        {
            EventPtr<MenuFlyoutItemClickEventCallback>* pEventPtr = *iter;
            delete pEventPtr;
        }

        // Clear the std::vector
        m_playbackRateClickHandlers.clear();
        m_currentPlaybackRates.clear();
    }
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to convert a duration in seconds to HString in [H]H:mm:ss format
//      for display in the ProgressTime and RemainingTime TextBlocks in the UI.
//
//      Duration is taken modulo 1 day = 86400 sec, as that is the largest
//      valid input for GetTimeFormat().
//
//      In some cases, RemainingTime could be negative due to nonzero
//      start times as with some IIS Smooth Streaming content. To avoid a crash
//      here, we always clamp negative incoming time to 0 in this helper.
//      See WinBlue Bug 440546 for details.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::ConvertSecondsToHString(
    _In_ INT64 totalSeconds,
    _Outptr_ HSTRING* pDisplayTime)
{
    HRESULT hr = S_OK;
    WCHAR szDisplayTime[MaxTimeButtonTextLength];
    wrl_wrappers::HString strDisplayTime;
    INT32 totalSecondsInRange = (totalSeconds > 0) ? (totalSeconds % 86400) : 0;
    INT32 numHours = static_cast<INT32> (totalSecondsInRange / 3600);
    INT32 remainingSeconds = static_cast<INT32> (totalSecondsInRange % 3600);
    INT32 numMinutes = remainingSeconds / 60;
    INT32 numSeconds = remainingSeconds % 60;
    INT32 getTimeFormatResult = 0;
    SYSTEMTIME time = {0};

    ASSERT (numHours < 24 && numMinutes < 60 && numSeconds < 60);

    time.wSecond = static_cast<WORD>(numSeconds);
    time.wMinute = static_cast<WORD>(numMinutes);
    time.wHour = static_cast<WORD>(numHours);

    ZeroMemory(szDisplayTime, sizeof(WCHAR)* MaxTimeButtonTextLength);

    getTimeFormatResult = GetTimeFormat(
        LOCALE_USER_DEFAULT,
        TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT,
        &time,
        NULL,
        szDisplayTime,
        MaxTimeButtonTextLength);

    // Fall back to unlocalized string in case GetTimeFormat() fails.
    if (getTimeFormatResult == 0)
    {
        IFCEXPECT(swprintf_s(szDisplayTime, MaxTimeButtonTextLength, L"%02d:%02d:%02d", numHours, numMinutes, numSeconds) >= 0);
    }

    IFC(strDisplayTime.Set(szDisplayTime));
    *pDisplayTime = strDisplayTime.Detach();

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to parent MTC to FullWindowMediaRoot
//
//      Precondition - no other ME's MTC is currently parented to FullWindowMediaRoot
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::AddToFullWindowMediaRoot()
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;

    IFC(m_wrOwnerParent.As(&spOwnerParent))
    if (m_transportControlsEnabled && spOwnerParent.Get())
    {
        IFC(VisualTreeHelper::GetFullWindowMediaRootStatic(spOwnerParent.Get(), &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (spFullWindowMediaRoot)
        {
            IFC(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));
            IFC(spChildren->Append(this));
        }

        IFC(GetXamlDispatcherNoRef()->RunAsync(
            MakeCallback(
                ctl::ComPtr<MediaTransportControls>(this),
                &MediaTransportControls::UpdateAfterEnteringFullWindow)));
    }

Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateAfterEnteringFullWindow()
{
#if false // DISABLE_FULL_WINDOW
    if(m_tokLayoutBoundsChanged.value == 0)
    {
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef();
    
            ctl::WeakRefPtr wrThis;
            IFC_RETURN(ctl::AsWeak(this, &wrThis));
            layoutBoundsHelper->AddLayoutBoundsChangedCallback(
                [wrThis]() mutable
                {
                    ctl::ComPtr<MediaTransportControls> spThis;
                    IFC_RETURN(wrThis.As(&spThis));
                    if(spThis.Get())
                    {
                        ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;

                        IFC_RETURN(spThis->m_wrOwnerParent.As(&spOwnerParent))
                        if (spOwnerParent.Get())
                        {
                            if (spThis->m_isFullWindow && spOwnerParent.Cast<FrameworkElement>()->IsInLiveTree())
                            {
                                // Update the media transport control's bounds with the available layout bounds.
                                // showing or hiding the soft buttons.
                                IFC_RETURN(spThis->UpdateMediaTransportBounds());
                            }
                        }
                    }
                    return S_OK;
                }, &m_tokLayoutBoundsChanged);
        }
    }

    // Update the media transport control's bounds with the current available layout bounds.
    IFC_RETURN(UpdateMediaTransportBounds());
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SetFocusAfterEnteringFullWindowMode()
{
    BOOLEAN focused = FALSE;
    BOOLEAN compact = FALSE;
    IFC_RETURN(get_IsCompact(&compact));
    // In Full Window mode, we give focus to PlayPause button and set Cycle
    // TabNavigation behavior on the root, so that user has a clean way
    // to navigate the transport controls. For Windows 8.1 (Blue) IsCompact always false
    if (!compact)
    {
        if (m_tpPlayPauseButton)
        {
            IFC_RETURN(m_tpPlayPauseButton.Cast<ButtonBase>()->Focus(xaml::FocusState_Pointer, &focused));
            ASSERT(focused);
            IFC_RETURN(put_TabNavigation(xaml_input::KeyboardNavigationMode_Cycle));
        }
        else
        {
            // In case Play/Pause remove button through re-template, then we are loosing focus in that case, we need to set on the next focusable Element.
            CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this->GetHandle());
            if (pFocusManager)
            {
                IFC_RETURN(pFocusManager->SetFocusOnNextFocusableElement(DirectUI::FocusState::Programmatic, true, InputActivationBehavior::NoActivate));
            }
        }
    }
    else
    {
        if (m_tpTHLeftSidePlayPauseButton)
        {
            IFC_RETURN(m_tpTHLeftSidePlayPauseButton.Cast<ButtonBase>()->Focus(xaml::FocusState_Pointer, &focused));
            ASSERT(focused);
            IFC_RETURN(put_TabNavigation(xaml_input::KeyboardNavigationMode_Cycle));
        }
        else
        {
            // In case Play/Pause remove button through re-template, then we are loosing focus in that case, we need to set on the next focusable Element.
            CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this->GetHandle());
            if (pFocusManager)
            {
                IFC_RETURN(pFocusManager->SetFocusOnNextFocusableElement(DirectUI::FocusState::Programmatic, true, InputActivationBehavior::NoActivate));
            }
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to remove MTC from FullWindowMediaRoot's children
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::RemoveFromFullWindowMediaRoot()
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;

    IFC(m_wrOwnerParent.As(&spOwnerParent))
    if (m_transportControlsEnabled && spOwnerParent.Get())
    {
        IFC(VisualTreeHelper::GetFullWindowMediaRootStatic(spOwnerParent.Get(), &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (spFullWindowMediaRoot)
        {
            IFC(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));

            unsigned int childrenSize = 0;
            IFC(spChildren->get_Size(&childrenSize));
            for (unsigned int i = 0; i < childrenSize; i++)
            {
                ctl::ComPtr<xaml::IUIElement> spUIE;
                IFC(spChildren->GetAt(i, &spUIE));
                if (spUIE.Get() == this)
                {
                    IFC(spChildren->RemoveAt(i));
                    break;
                }
            }
        }

        // Go back to default TabNavigation behavior now that we are leaving FW mode.
        if (m_tpPlayPauseButton || m_tpTHLeftSidePlayPauseButton)
        {
            IFC(put_TabNavigation(xaml_input::KeyboardNavigationMode_Local));
        }

        // Remove the xamlRoot changed and app view bounds changed events.
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
        {
            xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
        }
    }

Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

// Called by MediaPlayerElement when we're exiting full window mode
// TODO: TFS 7990888 -- Factor this better with RemoveFromFullWindowMediaRoot, which is called by MediaElement
_Check_return_ HRESULT
MediaTransportControls::HandleExitFullWindowMode()
{
#if false // DISABLE_FULL_WINDOW
    ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;

    IFC_RETURN(m_wrOwnerParent.As(&spOwnerParent))
    if (m_transportControlsEnabled && spOwnerParent.Get())
    {
        // Go back to default TabNavigation behavior now that we are leaving FW mode.
        if (m_tpPlayPauseButton || m_tpTHLeftSidePlayPauseButton)
        {
            IFC_RETURN(put_TabNavigation(xaml_input::KeyboardNavigationMode_Local));
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Obtain the processed language string to display in audio selection menu:
//         >> Localized language name based on RFC 1766 tag if possible
//         >> "untitled" if NULL tag is passed in, or we could not get
//            localized language if from the tag
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::GetLocalizedLanguageName(
    _In_opt_ HSTRING languageTag,
    _Outptr_ HSTRING* pProcessedLanguageName)
{
    HRESULT hr = S_OK;
    LPCWSTR pszLanguageTag = NULL;
    UINT32 cLanguageTag = 0;
    WCHAR* pLocalizedNameBuffer = NULL;
    BOOLEAN useUntitled = TRUE;
    wrl_wrappers::HString strLocalizedLanguage;

    *pProcessedLanguageName = NULL;

    // If no languageTag is passed, we'll use "untitled" string
    if (languageTag)
    {
        pLocalizedNameBuffer = new WCHAR[MaxProcessedLanguageNameLength];
        ZeroMemory(pLocalizedNameBuffer, MaxProcessedLanguageNameLength);

        pszLanguageTag = HStringUtil::GetRawBuffer(languageTag, &cLanguageTag);

        if (cLanguageTag > 0 && pszLanguageTag)
        {
            // Success - nonzero return value
            if (0 != ::GetLocaleInfoEx(pszLanguageTag, LOCALE_SLOCALIZEDLANGUAGENAME, pLocalizedNameBuffer, MaxProcessedLanguageNameLength))
            {
                // Extra check to ensure string is always NULL terminated
                pLocalizedNameBuffer[MaxProcessedLanguageNameLength - 1] = L'\0';

                // We are here if everything succeeded, so use the obtained localized string.
                // All other cases use default "untitled" string.
                useUntitled = FALSE;
            }
            else
            {
                TRACE(TraceAlways, L"GetLocaleInfoEx() failed to obtain localized language name for language tag %s", pszLanguageTag);
            }
        }
    }

    if (useUntitled)
    {
        IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_MEDIA_AUDIO_TRACK_UNTITLED, strLocalizedLanguage.ReleaseAndGetAddressOf()));
    }
    else
    {
        IFC(strLocalizedLanguage.Set(pLocalizedNameBuffer));
    }

    *pProcessedLanguageName = strLocalizedLanguage.Detach();

Cleanup:
    delete[] pLocalizedNameBuffer;
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//     Appends localized " (on)" suffix to localizedLanguage to signify
//     this is the language associated with currently active audio stream.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::MarkLanguageSelection(
    _In_opt_ HSTRING localizedLanguage,
    _Outptr_ HSTRING* pMarkedLocalizedLanguage)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString strSuffix;
    wrl_wrappers::HString strResultTemp;
    wrl_wrappers::HString strResultFull;

    *pMarkedLocalizedLanguage = NULL;

    // Get the "(on)" suffix
    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(TEXT_MEDIA_AUDIO_TRACK_SELECTED, strSuffix.ReleaseAndGetAddressOf()));

    IFC(::WindowsConcatString(localizedLanguage, wrl_wrappers::HStringReference(L" ").Get(), strResultTemp.GetAddressOf()));

    IFC(strResultTemp.Concat(strSuffix, strResultFull));

    *pMarkedLocalizedLanguage = strResultFull.Detach();

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to check if originalSource passed in RoutedEventArgs has
//      programmatic or keyboard focus (as opposed to pointer or no focus).
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::HasKeyOrProgFocus(
    _In_ xaml::IRoutedEventArgs *pArgs,
    _Out_ BOOLEAN *pHasKeyOrProgFocus)
{
    HRESULT hr = S_OK;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;
    ctl::ComPtr<IInspectable> spOriginalSource;
    ctl::ComPtr<xaml::IUIElement> spFocusedElement;

    *pHasKeyOrProgFocus = FALSE;

    IFC(pArgs->get_OriginalSource(&spOriginalSource));

    if (spOriginalSource)
    {
        // Make sure Original source should be derived from control
        spFocusedElement = spOriginalSource.AsOrNull<xaml::IUIElement>();
        if (spFocusedElement)
        {
            IFC(spFocusedElement->get_FocusState(&focusState));

            if (focusState == xaml::FocusState_Keyboard ||
                focusState == xaml::FocusState_Programmatic)
            {
                *pHasKeyOrProgFocus = TRUE;
            }
        }
    }

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to check if pControlToCompare is the originalSource
//      passed in RoutedEventArgs.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::CompareWithOriginalSource(
    _In_ xaml::IRoutedEventArgs *pArgs,
    _In_ IInspectable *pObjectToCompare,
    _Out_ BOOLEAN *pIsEqual)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spOriginalSource;

    *pIsEqual = FALSE;

    IFC(pArgs->get_OriginalSource(&spOriginalSource));

    *pIsEqual = spOriginalSource.Get() == pObjectToCompare;

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to hit test pElement against point.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::HitTestHelper(
    _In_ wf::Point point,
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* pHasHit)
{
    HRESULT hr =  S_OK;
    ctl::ComPtr<wfc::IIterable<xaml::UIElement*>> spElements;
    ctl::ComPtr<wfc::IIterator<xaml::UIElement*>> spIterator;
    BOOLEAN hasCurrent;

    *pHasHit = FALSE;

    IFC(VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(point, pElement, (m_tpCommandBar) ? FALSE :TRUE /* includeAllElements */, &spElements));
    IFC(spElements->First(&spIterator));
    IFC(spIterator->get_HasCurrent(&hasCurrent));

    while (hasCurrent)
    {
        ctl::ComPtr<xaml::IUIElement> spElement;
        IFC(spIterator->get_Current(&spElement));
        if (pElement == spElement.Get())
        {
            *pHasHit = TRUE;
            break;
        }
        IFC(spIterator->MoveNext(&hasCurrent));
    }

Cleanup:
    return hr;
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to check if conditions are met to hide vertical volume UI.
//
//---------------------------------------------------------------------------
BOOLEAN
MediaTransportControls::ShouldHideVerticalVolume()
{
    return  m_verticalVolumeIsVisible &&
           !m_verticalVolumeHasKeyOrProgFocus;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to check if conditions are met to hide control panel.
//
//---------------------------------------------------------------------------
BOOLEAN
MediaTransportControls::ShouldHideControlPanel()
{
    BOOLEAN isAutoShowHide = false;
    IGNOREHR(get_ShowAndHideAutomatically(&isAutoShowHide));

    return  m_controlPanelIsVisible &&
           !m_isAudioOnly &&
           !m_hasError &&
           (m_shouldDismissControlPanel || !m_controlPanelHasPointerOver) &&
           !m_rootHasPointerPressed &&
           // Do not need to check this on the Xbox only if commandbar should exist in the template.
           (!m_controlsHaveKeyOrProgFocus || (XboxUtility::IsOnXbox() && m_tpCommandBar.Get())) &&
           !m_verticalVolumeHasKeyOrProgFocus &&
           ShouldHideControlPanelWhilePlaying() &&
           !m_isFlyoutOpen &&
           !m_isPointerMove &&
           // Hide MTC only if auto hide/Show is enabled
           isAutoShowHide;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to check if conditions are met to hide control panel while playing.
//      It should stay if we aren't playing video.
//
//---------------------------------------------------------------------------
BOOLEAN
MediaTransportControls::ShouldHideControlPanelWhilePlaying()
{
    return  (m_isPlaying && !m_isBuffering)
            || (m_shouldDismissControlPanel);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Enters MediaEngine's scrubbing mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::EnterScrubbingMode()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        // Scrub mode is not useful for audio-only or live content
        if (!m_isAudioOnly &&
            !IsLiveContent() &&
            !m_isInScrubMode)
        {
            IFC(GetPlaybackRate(&m_currentPlaybackRate));
            IFC(SetPlaybackRate(0.0));
            IFC(EnableValueChangedEventThrottlingOnSliderAutomation(false));
            m_isInScrubMode = TRUE;
        }
    }

Cleanup:
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::ScrubbingMode, data);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Exits MediaEngine's scrubbing mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::ExitScrubbingMode()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (!m_isAudioOnly &&
            !IsLiveContent() &&
            m_isInScrubMode)
        {
            IFC(SetPlaybackRate(m_currentPlaybackRate));
            IFC(EnableValueChangedEventThrottlingOnSliderAutomation(true));
            m_isInScrubMode = FALSE;
        }
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called to update media layout bounds.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateMediaTransportBounds()
{
#if false // DISABLE_FULL_WINDOW
    ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;
    IFC_RETURN(m_wrOwnerParent.As(&spOwnerParent))
    if (spOwnerParent.Get())
    {
        auto* pFullWindowMediaRoot = GetHandle()->GetContext()->GetMainFullWindowMediaRoot();
        if (pFullWindowMediaRoot)
        {
            // Update the media transport control's bounds when the layout bounds is changed by
            // showing or hiding the soft buttons.
            // The SystemTray and AppBar will be suppressed on the full windowed media mode
            // so the layout bounds is the right bounds of the media transport control.
            pFullWindowMediaRoot->InvalidateArrange();
            pFullWindowMediaRoot->InvalidateMeasure();
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called for property changes via DXaml SetValue()
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(MediaTransportControlsGenerated::OnPropertyChanged2(args));
    IFC(UpdateMediaControlState(args.m_pDP->GetIndex()));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Update all MediaControl States
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateMediaControlAllStates()
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFullWindowButtonVisible));
#endif // DISABLE_FULL_WINDOW
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsZoomButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSeekBarVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsVolumeButtonVisible));
#if false // DISABLE_FULL_WINDOW
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFullWindowEnabled));
#endif // DISABLE_FULL_WINDOW
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsVolumeEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsZoomEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSeekEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsPlaybackRateButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsPlaybackRateEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFastForwardButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFastForwardEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFastRewindEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsFastRewindButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsStopEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsStopButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsCompact));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSkipForwardEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSkipBackwardEnabled));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSkipForwardButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsSkipBackwardButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsNextTrackButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsPreviousTrackButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsRepeatButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsRepeatEnabled));
#if false // DISABLE_COMPACT_OVERLAY
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsCompactOverlayButtonVisible));
    IFC(UpdateMediaControlState(KnownPropertyIndex::MediaTransportControls_IsCompactOverlayEnabled));
#endif // DISABLE_COMPACT_OVERLAY
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Handling common to DXAML level and core level property changes
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateMediaControlState(_In_ KnownPropertyIndex propertyIndex) noexcept
{
    HRESULT hr = S_OK;
    BOOLEAN value = FALSE;
    BOOLEAN compact = FALSE;
    ctl::ComPtr<xaml::IDependencyObject> spOwnerParent;
    ctl::ComPtr<xaml_controls::ITextBlock> spTimeElapsedTextBlock;
    ctl::ComPtr<xaml_controls::ITextBlock> spTimeRemainingTextBlock;

    switch (propertyIndex)
    {
#if false // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaTransportControls_IsFullWindowButtonVisible:
        {
            if (m_tpFullWindowButton)
            {

                // Remove this code to disable and hide only after Deliverable 19012797: Fullscreen media works in ApplicationWindow and Win32 XAML Islands is complete
                CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
                if( contentRoot->GetType() == CContentRoot::Type::XamlIsland )
                {
                    IFC(m_tpFullWindowButton.Cast<ButtonBase>()->put_Visibility(xaml::Visibility_Collapsed));
                }
                else
                {
                    IFC(get_IsFullWindowButtonVisible(&value));
                    IFC(m_tpFullWindowButton.Cast<ButtonBase>()->put_Visibility(
                        (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
                }
            }
            break;
        }
#endif // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaTransportControls_IsZoomButtonVisible:
        {
            if (m_tpZoomButton)
            {
                IFC(get_IsZoomButtonVisible(&value));
                IFC(m_tpZoomButton.Cast<ButtonBase>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSeekBarVisible:
        {
            IFC(get_IsSeekBarVisible(&value));
            IFC(get_IsCompact(&compact));
            if (m_tpTimeElapsedElement && !compact)
            {
                IFC(m_tpTimeElapsedElement.As(&spTimeElapsedTextBlock));
                IFC(spTimeElapsedTextBlock.Cast<TextBlock>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            if (m_tpTimeRemainingElement && !compact)
            {
                IFC(m_tpTimeRemainingElement.As(&spTimeRemainingTextBlock));
                IFC(spTimeRemainingTextBlock.Cast<TextBlock>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            if (m_tpMediaPositionSlider)
            {
                IFC(m_tpMediaPositionSlider.Cast<Slider>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsVolumeButtonVisible:
        {

            if (m_tpTHVolumeButton)
            {
                IFC(get_IsVolumeButtonVisible(&value));
                IFC(m_tpTHVolumeButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsPlaybackRateButtonVisible:
        {

            if (m_tpPlaybackRateButton)
            {
                IFC(get_IsPlaybackRateButtonVisible(&value));
                IFC(m_tpPlaybackRateButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsFastForwardButtonVisible:
        {

            if (m_tpFastForwardButton)
            {
                IFC(get_IsFastForwardButtonVisible(&value));
                IFC(m_tpFastForwardButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsFastRewindButtonVisible:
        {

            if (m_tpFastRewindButton)
            {
                IFC(get_IsFastRewindButtonVisible(&value));
                IFC(m_tpFastRewindButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsStopButtonVisible:
        {

            if (m_tpStopButton)
            {
                IFC(get_IsStopButtonVisible(&value));
                IFC(m_tpStopButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSkipForwardButtonVisible:
        {

            if (m_tpSkipForwardButton)
            {
                IFC(get_IsSkipForwardButtonVisible(&value));
                IFC(m_tpSkipForwardButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSkipBackwardButtonVisible:
        {

            if (m_tpSkipBackwardButton)
            {
                IFC(get_IsSkipBackwardButtonVisible(&value));
                IFC(m_tpSkipBackwardButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsNextTrackButtonVisible:
        {

            if (m_tpNextTrackButton)
            {
                IFC(get_IsNextTrackButtonVisible(&value));
                IFC(m_tpNextTrackButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsPreviousTrackButtonVisible:
        {

            if (m_tpPreviousTrackButton)
            {
                IFC(get_IsPreviousTrackButtonVisible(&value));
                IFC(m_tpPreviousTrackButton.Cast<Button>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsRepeatButtonVisible:
        {

            if (m_tpRepeatButton)
            {
                IFC(get_IsRepeatButtonVisible(&value));
                IFC(m_tpRepeatButton.Cast<ToggleButton>()->put_Visibility(
                    (value) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
#if false // DISABLE_COMPACT_OVERLAY
        case KnownPropertyIndex::MediaTransportControls_IsCompactOverlayButtonVisible:
        {
            if (m_tpCompactOverlayButton)
            {
                ctl::ComPtr<wuv::IApplicationView4> spAppView4;
                boolean isSupport = false;
                IFC(GetMiniView(&spAppView4));
                if (spAppView4)
                {
                    IFC(spAppView4->IsViewModeSupported(wuv::ApplicationViewMode::ApplicationViewMode_CompactOverlay, &isSupport));
                }
                IFC(get_IsCompactOverlayButtonVisible(&value));

                IFC(m_tpCompactOverlayButton.Cast<Button>()->put_Visibility(
                    (value && isSupport) ? xaml::Visibility_Visible : xaml::Visibility_Collapsed));
            }
            break;
        }
#endif // DISABLE_COMPACT_OVERLAY
#if false // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaTransportControls_IsFullWindowEnabled:
        {
            if (m_tpFullWindowButton)
            {
                // Remove this code to disable and hide only after Deliverable 19012797: Fullscreen media works in ApplicationWindow and Win32 XAML Islands is complete
                CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
                if( contentRoot->GetType() == CContentRoot::Type::XamlIsland )
                {
                    IFC(m_tpFullWindowButton.Cast<ButtonBase>()->put_IsEnabled(false));
                }
                else
                {
                    IFC(get_IsFullWindowEnabled(&value));
                    IFC(m_tpFullWindowButton.Cast<ButtonBase>()->put_IsEnabled(value));
                }

            }
            break;
        }
#endif // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaTransportControls_IsVolumeEnabled:
        {
            if (m_tpTHVolumeButton)
            {
                IFC(get_IsVolumeEnabled(&value));
                IFC(m_tpTHVolumeButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsZoomEnabled:
        {
            if (m_tpZoomButton)
            {
                IFC(get_IsZoomEnabled(&value));
                IFC(m_tpZoomButton.Cast<ButtonBase>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSeekEnabled:
        {
            if (m_tpMediaPositionSlider)
            {
                IFC(get_IsSeekEnabled(&value));
                IFC(m_tpMediaPositionSlider.Cast<Slider>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsPlaybackRateEnabled:
        {
            if (m_tpPlaybackRateButton)
            {
                IFC(get_IsPlaybackRateEnabled(&value));
                IFC(m_tpPlaybackRateButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsFastForwardEnabled:
        {
            if (m_tpFastForwardButton)
            {
                IFC(get_IsFastForwardEnabled(&value));
                IFC(m_tpFastForwardButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsFastRewindEnabled:
        {
            if (m_tpFastRewindButton)
            {
                IFC(get_IsFastRewindEnabled(&value));
                IFC(m_tpFastRewindButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSkipForwardEnabled:
        {
            if (m_tpSkipForwardButton)
            {
                IFC(get_IsSkipForwardEnabled(&value));
                IFC(m_tpSkipForwardButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsSkipBackwardEnabled:
        {
            if (m_tpSkipBackwardButton)
            {
                IFC(get_IsSkipBackwardEnabled(&value));
                IFC(m_tpSkipBackwardButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsStopEnabled:
        {
            if (m_tpStopButton)
            {
                IFC(get_IsStopEnabled(&value));
                IFC(m_tpStopButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_IsRepeatEnabled:
        {
            if (m_tpRepeatButton)
            {
                IFC(get_IsRepeatEnabled(&value));
                IFC(m_tpRepeatButton.Cast<ToggleButton>()->put_IsEnabled(value));
            }
            break;
        }
#if false // DISABLE_COMPACT_OVERLAY
        case KnownPropertyIndex::MediaTransportControls_IsCompactOverlayEnabled:
        {
            if (m_tpCompactOverlayButton)
            {
                IFC(get_IsCompactOverlayEnabled(&value));
                IFC(m_tpCompactOverlayButton.Cast<Button>()->put_IsEnabled(value));
            }
            break;
        }
#endif // DISABLE_COMPACT_OVERLAY
        case KnownPropertyIndex::MediaTransportControls_IsCompact:
        {
            IFC(m_wrOwnerParent.As(&spOwnerParent))
            if (m_transportControlsEnabled && spOwnerParent.Get())
            {
                IFC(get_IsCompact(&compact));
                if (m_isCompact != compact)
                {
                    m_isCompact = compact;
                    IFC(UpdateVisualState());
                    IFC(SetMeasureCommandBar());
                    IFC(SetTabIndex());
                }
            }
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_FastPlayFallbackBehaviour:
        {
            IFC(UpdateTrickModeFallbackUI());
            break;
        }
        case KnownPropertyIndex::MediaTransportControls_ShowAndHideAutomatically:
        {
            BOOLEAN isAutoShowHide = false;
            IFC(get_ShowAndHideAutomatically(&isAutoShowHide));
            // If MTC is hides and AutoHide is disabled then show immediately
            if (!m_controlPanelIsVisible && !isAutoShowHide)
            {
                IFC(ShowControlPanel());
            }
            break;
        }
    }
Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//       Setup the default values for the MediaControls States
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::SetupDefaultProperties()
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    IFC(put_IsFullWindowButtonVisible(true));
    IFC(put_IsFullWindowEnabled(true));
#endif // DISABLE_FULL_WINDOW
    IFC(put_IsZoomButtonVisible(true));
    IFC(put_IsZoomEnabled(true));
    IFC(put_IsFastForwardButtonVisible(false));
    IFC(put_IsFastForwardEnabled(false));
    IFC(put_IsFastRewindButtonVisible(false));
    IFC(put_IsFastRewindEnabled(false));
    IFC(put_IsStopButtonVisible(false));
    IFC(put_IsStopEnabled(false));
    IFC(put_IsVolumeButtonVisible(true));
    IFC(put_IsVolumeEnabled(true));
    IFC(put_IsSeekBarVisible(true));
    IFC(put_IsSeekEnabled(true));
    IFC(put_IsPlaybackRateButtonVisible(false));
    IFC(put_IsPlaybackRateEnabled(false));
    IFC(put_IsSkipBackwardButtonVisible(false));
    IFC(put_IsSkipBackwardEnabled(false));
    IFC(put_IsSkipForwardButtonVisible(false));
    IFC(put_IsSkipForwardEnabled(false));
    IFC(put_IsPreviousTrackButtonVisible(false));
    IFC(put_IsNextTrackButtonVisible(false));
    IFC(put_FastPlayFallbackBehaviour(xaml_media::FastPlayFallbackBehaviour::FastPlayFallbackBehaviour_Skip));
    IFC(put_ShowAndHideAutomatically(true));
    IFC(put_IsRepeatEnabled(false));
    IFC(put_IsRepeatButtonVisible(false));
#if false // DISABLE_COMPACT_OVERLAY
    IFC(put_IsCompactOverlayEnabled(false));
    IFC(put_IsCompactOverlayButtonVisible(false));
#endif // DISABLE_COMPACT_OVERLAY
Cleanup:
    return hr;
}

// Called when a user selects a track from the CC menu, the old value
// needs to be deselected and the new value selected
_Check_return_ HRESULT
MediaTransportControls::OnCCTrackClicked(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase> spItem;
    UINT selectedTrackIndex = 0;
    BOOLEAN isFound = FALSE;
    MTCTelemetryData data;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(m_tpAvailableCCTracksMenuFlyout->get_Items(&spMenuFlyoutItems));

        // Determine index of currently selected CC track
        IFC(ctl::do_query_interface(spItem, pSender));
        IFC(spMenuFlyoutItems->IndexOf(spItem.Get(), &selectedTrackIndex, &isFound));
        if (isFound)
        {
            // Apply user's track selection
            if (MTCParent_MediaPlayerElement == m_parentType)
            {
                IFC(SetCCTrackFromMPE(selectedTrackIndex));
            }
        }
    }

Cleanup:
    data.errCode = hr;
    data.trackId = m_currentTrack;
    m_AggTelemetry.AddData(MTCTelemetryType::CCTrackClick, data);
    return hr;
}

// Called when a user selects a play rate from menu, the old value
// needs to be deselected and the new value selected
_Check_return_ HRESULT
MediaTransportControls::OnPlaybackRateMenuClicked(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<xaml_controls::MenuFlyoutItemBase*>> spMenuFlyoutItems;
    ctl::ComPtr<xaml_controls::IMenuFlyoutItemBase> spItem;
    UINT selectedPlaybackRateIndex = 0;
    BOOLEAN isFound = FALSE;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(m_tpAvailablePlaybackRateMenuFlyout->get_Items(&spMenuFlyoutItems));

        // Determine index of currently selected CC track
        IFC(ctl::do_query_interface(spItem, pSender));
        IFC(spMenuFlyoutItems->IndexOf(spItem.Get(), &selectedPlaybackRateIndex, &isFound));
        if (isFound)
        {
            double playbackRate = -1;
            ASSERT(selectedPlaybackRateIndex < m_currentPlaybackRates.size());
            playbackRate = m_currentPlaybackRates[selectedPlaybackRateIndex];
            IFC(SetPlaybackRate(playbackRate, false));
        }
    }

Cleanup:
    return hr;
}

// Called when fast forward button is clicked
_Check_return_ HRESULT
MediaTransportControls::OnFastForwardButtonClicked()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled)
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(TrickModeForward());
        }
        else
        {
            IFC(SkipForward());
        }
    }

Cleanup:
    return hr;
}

// Called when rewind button is clicked
_Check_return_ HRESULT
MediaTransportControls::OnFastRewindButtonClicked()
{
    HRESULT hr = S_OK;
    if (m_transportControlsEnabled)
    {
        if (MTCParent_MediaPlayerElement == m_parentType)
        {
            IFC(TrickModeBackward());
        }
        else
        {
            IFC(SkipBackward());
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SkipForward()
{
    HRESULT hr = S_OK;
    wf::TimeSpan currentMediaPosition;
    wf::TimeSpan newMediaPosition;
    INT64 hnsToEnd = 0;
    INT64 FFTimeInHNS = static_cast<INT64>(SkipForwardInSecs)* static_cast<INT64>(HNSPerSecond);

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (!IsLiveContent())
        {
            IFC(GetPosition(&currentMediaPosition));
            hnsToEnd = m_naturalDuration.TimeSpan.Duration - currentMediaPosition.Duration;

            // Seek +FFandRWTimeInSecs if position > FFandRWTimeInSecs away from the end, to the end otherwise
            newMediaPosition.Duration = (hnsToEnd > FFTimeInHNS) ? currentMediaPosition.Duration + FFTimeInHNS : m_naturalDuration.TimeSpan.Duration;
            IFC(SetPosition(newMediaPosition, false));
            IFC(UpdatePositionUI());
        }
    }

Cleanup:
    return hr;

}

_Check_return_ HRESULT
MediaTransportControls::SkipBackward()
{
    HRESULT hr = S_OK;
    wf::TimeSpan currentMediaPosition;
    wf::TimeSpan newMediaPosition;
    INT64 RWTimeInHNS = static_cast<INT64>(SkipBackwardInSecs) * static_cast<INT64>(HNSPerSecond);

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (!IsLiveContent())
        {
            IFC(GetPosition(&currentMediaPosition));

            // Seek -FFandRWTimeInSecs if position > FFandRWTimeInSecs, to 0s otherwise
            newMediaPosition.Duration = (currentMediaPosition.Duration > RWTimeInHNS) ? currentMediaPosition.Duration - RWTimeInHNS : 0;
            IFC(SetPosition(newMediaPosition, false));
            IFC(UpdatePositionUI());
        }
    }

Cleanup:
    return hr;
}

// Called when stop button is clicked
_Check_return_ HRESULT
MediaTransportControls::OnStopButtonClicked()
{
    HRESULT hr = S_OK;

    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        IFC(Stop());
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SetMeasureCommandBar()
{
    // We need to complete the event call and then let the Measure happen after that.
    // This will ensure that no peers are referenced while Measuring.
    IFC_RETURN(GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
        ctl::ComPtr<MediaTransportControls>(this), &MediaTransportControls::MeasureCommandBar)));
    return S_OK;
}


// Measure CommandBar to fit the buttons in given width.
_Check_return_ HRESULT
MediaTransportControls::MeasureCommandBar()
{
    HRESULT hr = S_OK;

    if (m_tpCommandBar)
    {
        wf::Size desiredSize;
        double availableSize;
        const wf::Size infiniteBounds = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

        IFC(ResetMargins());
        IFC(get_ActualWidth(&availableSize));
        IFC(m_tpCommandBar.Cast<CommandBar>()->Measure(infiniteBounds));
        IFC(m_tpCommandBar.Cast<CommandBar>()->get_DesiredSize(&desiredSize));

        if (availableSize < desiredSize.Width)
        {
            IFC(Dropout(availableSize, desiredSize));
        }
        else
        {
            IFC(Expand(availableSize, desiredSize));
            IFC(AddMarginsBetweenGroups());
        }

        // Remove this code to disable and hide only after Deliverable 19012797: Fullscreen media works in ApplicationWindow and Win32 XAML Islands is complete
        // since Expand or Dropout can make the full window button visible again, this code is used to hide it again
#if false // DISABLE_FULL_WINDOW
        CContentRoot* contentRoot = VisualTree::GetContentRootForElement(GetHandle());
        if( contentRoot->GetType() == CContentRoot::Type::XamlIsland )
        {
            IFC(m_tpFullWindowButton.Cast<ButtonBase>()->put_Visibility(xaml::Visibility_Collapsed));
        }
#endif // DISABLE_FULL_WINDOW
    }

Cleanup:
    return hr;
}

// Implement the logic to drop out the command bar based on the Lowest Visible Order, so that lowest should go first
_Check_return_ HRESULT MediaTransportControls::Dropout(
                                _In_ double availableSize,
                                _In_ wf::Size expectSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MediaTransportControlsHelperFactory> spFactory;
    ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> spPrimaryCommandObsVec;
    ctl::ComPtr<wfc::IVector<ICommandBarElement*>> spPrimaryButtons;
    unsigned int count = 0;

    xaml::Visibility visibility = xaml::Visibility_Collapsed;
    wf::Size desiredSize = expectSize;
    const wf::Size infiniteBounds = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

    IFC(ctl::make<MediaTransportControlsHelperFactory>(&spFactory));
    IFC(m_tpCommandBar.Cast<CommandBar>()->get_PrimaryCommands(&spPrimaryCommandObsVec));
    IFCPTR(spPrimaryCommandObsVec);
    IFC(spPrimaryCommandObsVec.As(&spPrimaryButtons));
    IFC(spPrimaryButtons->get_Size(&count));

    while (availableSize < desiredSize.Width)
    {
        int lowestVisibleOrder = INT_MAX;
        int lowestElementIndex = 0;

        for (unsigned int i = 0; i < count; i++)
        {
            ctl::ComPtr<ICommandBarElement> spCommandElement;

            IFC(spPrimaryButtons->GetAt(i, &spCommandElement));
            if (spCommandElement)
            {
                ctl::ComPtr<IUIElement> spElement;
                IFC(spCommandElement.As(&spElement));
                IFC(spElement->get_Visibility(&visibility));
                if (visibility == xaml::Visibility::Visibility_Visible)
                {
                    ctl::ComPtr<wf::IReference<int>> spOrder;
                    int order = 0;
                    IFC(spFactory->GetDropoutOrder(spElement.Get(), &spOrder));
                    if (spOrder)
                    {
                        IFC(spOrder->get_Value(&order));
                    }
                    if (lowestVisibleOrder > order && order > 0)
                    {
                        lowestVisibleOrder = order;
                        lowestElementIndex = i;
                    }
                }
            }
        }
        if (lowestVisibleOrder == INT_MAX)
        {
            break;
        }
        else
        {
            ctl::ComPtr<ICommandBarElement> spCommandElement;
            ctl::ComPtr<IUIElement> spElement;

            IFC(spPrimaryButtons->GetAt(lowestElementIndex, &spCommandElement));
            IFC(spCommandElement.As(&spElement));
            IFC(spElement->put_Visibility(xaml::Visibility::Visibility_Collapsed));
            IFC(m_tpCommandBar.Cast<CommandBar>()->Measure(infiniteBounds));
            IFC(m_tpCommandBar.Cast<CommandBar>()->get_DesiredSize(&desiredSize));
        }
    }

Cleanup:
    return hr;
}

// Implement the logic to Expand/Show buttons out of the command bar based on the High Collapse Order ,so that highest should retain.
_Check_return_ HRESULT MediaTransportControls::Expand(
                _In_ double availableSize,
                _In_ wf::Size expectSize)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<MediaTransportControlsHelperFactory> spFactory;
    ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> spPrimaryCommandObsVec;
    ctl::ComPtr<wfc::IVector<ICommandBarElement*>> spPrimaryButtons;
    unsigned int count = 0;
    wf::Size desiredSize = expectSize;

    IFC(ctl::make<MediaTransportControlsHelperFactory>(&spFactory));
    IFC(m_tpCommandBar.Cast<CommandBar>()->get_PrimaryCommands(&spPrimaryCommandObsVec))
    IFCPTR(spPrimaryCommandObsVec);
    IFC(spPrimaryCommandObsVec.As(&spPrimaryButtons));
    IFC(spPrimaryButtons->get_Size(&count));

    while (availableSize > desiredSize.Width)
    {
        int highestCollapseOrder = -1;
        int highestElementIndex = 0;

        for (unsigned int i = 0; i < count; i++)
        {
            ctl::ComPtr<ICommandBarElement> spCommandElement;
            xaml::Visibility visibility = xaml::Visibility_Collapsed;

            IFC(spPrimaryButtons->GetAt(i, &spCommandElement));
            if (spCommandElement)
            {
                ctl::ComPtr<IUIElement> spElement;
                IFC(spCommandElement.As(&spElement));
                IFC(spElement->get_Visibility(&visibility));
                if (visibility == xaml::Visibility::Visibility_Collapsed && !IsButtonCollapsedbySystem(spElement.Get()))
                {
                    ctl::ComPtr<wf::IReference<int>> spOrder;
                    int order = 0;
                    IFC(spFactory->GetDropoutOrder(spElement.Get(), &spOrder));
                    if (spOrder)
                    {
                        IFC(spOrder->get_Value(&order));
                    }
                    if (order > highestCollapseOrder)
                    {
                        highestCollapseOrder = order;
                        highestElementIndex = i;
                    }
                }
            }
        }

        if (highestCollapseOrder == -1)
        {
            break;
        }
        else
        {
            ctl::ComPtr<ICommandBarElement> spCommandElement;
            ctl::ComPtr<IUIElement> spElement;
            double width;
            const wf::Size infiniteBounds = { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };

            IFC(spPrimaryButtons->GetAt(highestElementIndex, &spCommandElement));
            IFC(spCommandElement.As(&spElement));
            IFC(spElement.Cast<FrameworkElement>()->get_Width(&width));
            // Make sure it should be complete space but not partial space to fit the button
            if(availableSize >= (desiredSize.Width + width))
            {
                IFC(spElement->put_Visibility(xaml::Visibility::Visibility_Visible));
                IFC(m_tpCommandBar.Cast<CommandBar>()->Measure(infiniteBounds));
                IFC(m_tpCommandBar.Cast<CommandBar>()->get_DesiredSize(&desiredSize));
            }
            else
            {
                break;
            }
        }
    }

Cleanup:
    return hr;

}

_Check_return_ HRESULT
MediaTransportControls::AddMarginsBetweenGroups()
{
    BOOLEAN compact = FALSE;
    HRESULT hr = S_OK;

    get_IsCompact(&compact);
    if ((m_tpLeftAppBarSeparator || m_tpRightAppBarSeparator) && !compact)
    {
        ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> spPrimaryCommandObsVec;
        ctl::ComPtr<wfc::IVector<ICommandBarElement*>> spPrimaryButtons;
        unsigned int count = 0;
        double totalWidth = 0;
        double leftWidth = 0;
        double middleWidth = 0;
        double rightWidth = 0;
        BOOL leftComplete = FALSE;
        BOOL rightStart = FALSE;

        IFC(get_ActualWidth(&totalWidth))
        IFC(m_tpCommandBar.Cast<CommandBar>()->get_PrimaryCommands(&spPrimaryCommandObsVec))
        IFCPTR(spPrimaryCommandObsVec);
        IFC(spPrimaryCommandObsVec.As(&spPrimaryButtons));
        IFC(spPrimaryButtons->get_Size(&count));

        for (unsigned int i = 0; i < count; i++)
        {
            ctl::ComPtr<ICommandBarElement> spCommandElement;
            xaml::Visibility visibility = xaml::Visibility_Collapsed;

            IFC(spPrimaryButtons->GetAt(i, &spCommandElement));
            if (spCommandElement)
            {
                ctl::ComPtr<IUIElement> spElement;
                IFC(spCommandElement.As(&spElement));
                IFC(spElement->get_Visibility(&visibility));
                if (visibility == xaml::Visibility::Visibility_Visible)
                {
                    if (spElement.Get() == m_tpLeftAppBarSeparator.AsOrNull<IUIElement>().Get())
                    {
                        leftComplete = TRUE;
                        continue;
                    }
                    if (spElement.Get() == m_tpRightAppBarSeparator.AsOrNull<IUIElement>().Get())
                    {
                        rightStart = TRUE;
                        continue;
                    }

                    ctl::ComPtr<IFrameworkElement> spFrmElement;
                    double width;
                    IFC(spElement.As(&spFrmElement));
                    IFC(spFrmElement->get_Width(&width));

                    if (!leftComplete)
                    {
                        leftWidth = leftWidth + width;
                    }
                    else if (!rightStart)
                    {
                        middleWidth = middleWidth + width;
                    }
                    else
                    {
                        rightWidth = rightWidth + width;
                    }
                }
            }
        }

        xaml::Thickness cmdMargin = { 0, 0, 0, 0 };
        // Consider control panel margin for xbox case
        if (m_tpControlPanelGrid)
        {
            m_tpControlPanelGrid.Cast<Grid>()->get_Margin(&cmdMargin);
        }

        double leftGap = (totalWidth / 2) - (cmdMargin.Left + leftWidth + (middleWidth / 2));
        double rightGap = (totalWidth / 2) - (cmdMargin.Right + rightWidth + (middleWidth / 2));
        // If we get negative value, means they are not in equal balance
        if (leftGap < 0 || rightGap < 0)
        {
            leftGap = rightGap = (totalWidth - (leftWidth + middleWidth + rightWidth)) / 2;
        }

        if (m_tpLeftAppBarSeparator)
        {
            xaml::Thickness extraMargin = { leftGap / 2, 0, leftGap / 2, 0 };
            IFC(m_tpLeftAppBarSeparator.Cast<AppBarSeparator>()->put_Margin(extraMargin));
        }
        if (m_tpRightAppBarSeparator)
        {
            xaml::Thickness extraMargin = { rightGap / 2, 0, rightGap / 2, 0 };
            IFC(m_tpRightAppBarSeparator.Cast<AppBarSeparator>()->put_Margin(extraMargin));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::ResetMargins()
{
    HRESULT hr = S_OK;
    xaml::Thickness zeroMargin = { 0, 0, 0, 0 };

    if (m_tpLeftAppBarSeparator)
    {
       IFC(m_tpLeftAppBarSeparator.Cast<AppBarSeparator>()->put_Margin(zeroMargin));
    }
    if (m_tpRightAppBarSeparator)
    {
       IFC(m_tpRightAppBarSeparator.Cast<AppBarSeparator>()->put_Margin(zeroMargin));
    }

Cleanup:
    return hr;
}

// Determine whether button collapsed by system.
BOOLEAN MediaTransportControls::IsButtonCollapsedbySystem(_In_ IUIElement* element)
{
    BOOLEAN value;

    //In case of Compact mode this button should collapse
    if (element == m_tpPlayPauseButton.AsOrNull<IUIElement>().Get() && m_isCompact)
    {
        return TRUE;
    }
    else
    //In case of the Missing Audio tracks this button should collapse
    if (element == m_tpTHAudioTrackSelectionButton.AsOrNull<IUIElement>().Get() && !m_hasMultipleAudioStreams)
    {
        return TRUE;
    }
    else
    //In case of the Missing CC tracks this button should collapse
    if (element == m_tpCCSelectionButton.AsOrNull<IUIElement>().Get() && !m_hasCCTracks)
    {
        return TRUE;
    }
    else //Remaining check whether thru APIs Button is collapsed.
    if (element == m_tpPlaybackRateButton.AsOrNull<IUIElement>().Get())
    {
        get_IsPlaybackRateButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpTHVolumeButton.AsOrNull<IUIElement>().Get())
    {
        get_IsVolumeButtonVisible(&value);
        return !value;
    }
#if false // DISABLE_FULL_WINDOW
    else
    if (element == m_tpFullWindowButton.AsOrNull<IUIElement>().Get())
    {
        get_IsFullWindowButtonVisible(&value);
        return !value;
    }
#endif // DISABLE_FULL_WINDOW
    else
    if (element == m_tpZoomButton.AsOrNull<IUIElement>().Get())
    {
        get_IsZoomButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpFastForwardButton.AsOrNull<IUIElement>().Get())
    {
        get_IsFastForwardButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpFastRewindButton.AsOrNull<IUIElement>().Get())
    {
        get_IsFastRewindButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpStopButton.AsOrNull<IUIElement>().Get())
    {
        get_IsStopButtonVisible(&value);
        return !value;
    }
    else
    // In case of the Cast doesn't supports this button should always collapse
    if (element == m_tpCastButton.AsOrNull<IUIElement>().Get() && !m_isCastSupports)
    {
        return TRUE;
    }
    else
    if (element == m_tpSkipForwardButton.AsOrNull<IUIElement>().Get())
    {
        get_IsSkipForwardButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpSkipBackwardButton.AsOrNull<IUIElement>().Get())
    {
        get_IsSkipBackwardButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpNextTrackButton.AsOrNull<IUIElement>().Get())
    {
        get_IsNextTrackButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpPreviousTrackButton.AsOrNull<IUIElement>().Get())
    {
        get_IsPreviousTrackButtonVisible(&value);
        return !value;
    }
    else
    if (element == m_tpRepeatButton.AsOrNull<IUIElement>().Get())
    {
        get_IsRepeatButtonVisible(&value);
        return !value;
    }
#if false // DISABLE_COMPACT_OVERLAY
    else
    if (element == m_tpCompactOverlayButton.AsOrNull<IUIElement>().Get())
    {
        get_IsCompactOverlayButtonVisible(&value);
        return !value;
    }
#endif // DISABLE_COMPACT_OVERLAY
    return FALSE;
}

// Called when cast button is clicked
_Check_return_ HRESULT
MediaTransportControls::OnCastButtonClicked()
{
    HRESULT hr = S_OK;
    MTCTelemetryData data;

    if (m_wrOwnerParent.Get() && m_tpCastButton)
    {
        ctl::ComPtr<wm::Casting::ICastingDevicePicker> spCastingDevicePicker;
        wf::Point targetPoint{};
        wf::Rect rectSelection;
        ctl::ComPtr<xaml_media::IGeneralTransform> spTransformToRoot;
        double buttonWidth = 0;
        double buttonHeight = 0;

        // Convert the target point from the target element to the root
        IFC(m_tpCastButton.Cast<Button>()->TransformToVisual(nullptr, &spTransformToRoot));
        IFC(spTransformToRoot->TransformPoint(targetPoint, &targetPoint));
        IFC(m_tpCastButton.Cast<Button>()->get_ActualHeight(&buttonHeight));
        IFC(m_tpCastButton.Cast<Button>()->get_ActualWidth(&buttonWidth));
        rectSelection.X = targetPoint.X;
        rectSelection.Y = targetPoint.Y;
        rectSelection.Width = static_cast<float>(buttonWidth);
        rectSelection.Height = static_cast<float>(buttonHeight);
        // get the cached Device Picker
        IFC(GetCastingDevicePicker(spCastingDevicePicker));
        if (spCastingDevicePicker)
        {
            // Before Showing the Device Picker, make sure pause video if playing
            if (m_isPlaying)
            {
                IFC(Pause(true));
                m_isPausedForCastingSelection = TRUE;
            }
            // Show the Device Picker above the Cast Button
            IFC(spCastingDevicePicker->ShowWithPlacement(rectSelection, wup::Placement::Placement_Above));
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        ResetPlayBackAfterCasting();
    }
    data.errCode = hr;
    m_AggTelemetry.AddData(MTCTelemetryType::CastButtonClick, data);
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetCastingDevicePicker(_Out_ ctl::ComPtr<wm::Casting::ICastingDevicePicker>& spCastingDevicePicker)
{
    if (!m_spCastingDevicePicker)
    {
        ctl::ComPtr<wm::Casting::ICastingDevicePickerFilter> spCastingDevicePickerFilter;
        ctl::ComPtr<wm::Casting::ICastingSource> spCastingSource;
        ctl::ComPtr<wfc::IVector<wm::Casting::CastingSource*>> supportedCastingSources;
        if (m_wrOwnerParent.Get())
        {
            IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Casting_CastingDevicePicker).Get(),
                    m_spCastingDevicePicker.ReleaseAndGetAddressOf()));

            // Get the HWND of the window if available
            ctl::ComPtr<DependencyObject> spOwnerElement;
            IFC_RETURN(m_wrOwnerParent.As(&spOwnerElement))
            ctl::ComPtr<XamlRoot> xamlRoot = XamlRoot::GetImplementationForElementStatic(spOwnerElement.Get());
            IFCEXPECTRC_RETURN(xamlRoot, S_FALSE);

            HWND xamlHwnd;
            IFC_RETURN(xamlRoot->get_HostWindow(&xamlHwnd));
            if (xamlHwnd)
            {
                // We need to set the HWND on the picker by QI'ing it for IInitializeWithWindow.
                // The reason is that the picker will eventually be modal to that window
                ctl::ComPtr<IInitializeWithWindow> spInitialize;
                IFC_RETURN(m_spCastingDevicePicker.As(&spInitialize));
                IFC_RETURN(spInitialize->Initialize(xamlHwnd));
            }

            // Set the MediaElement Casting Source in the Supported List.
            IFC_RETURN(GetAsCastingSource(&spCastingSource));
            IFC_RETURN(m_spCastingDevicePicker->get_Filter(&spCastingDevicePickerFilter));
            if (spCastingDevicePickerFilter)
            {
                IFC_RETURN(spCastingDevicePickerFilter->get_SupportedCastingSources(&supportedCastingSources));
                IFC_RETURN(supportedCastingSources->Append(spCastingSource.Get()));
            }

            IFC_RETURN(m_spCastingDevicePicker->add_CastingDeviceSelected(
                wrl::Callback<wf::ITypedEventHandler<wm::Casting::CastingDevicePicker*, wm::Casting::CastingDeviceSelectedEventArgs*>>(
                [this](wm::Casting::ICastingDevicePicker *pCastingDevicePicker, wm::Casting::ICastingDeviceSelectedEventArgs *pArgs) -> HRESULT
            {
                ctl::ComPtr<wm::Casting::ICastingDevice> spCastingDevice;
                ctl::ComPtr<wm::Casting::ICastingConnection> spCastingConnection;
                ctl::ComPtr<wm::Casting::ICastingSource> spCastingSource;
                ctl::ComPtr<wf::IAsyncOperation<wm::Casting::CastingConnectionErrorStatus>> spAsyncOperation;
                IFCPTR_RETURN(pCastingDevicePicker);
                IFCPTR_RETURN(pArgs);
                if (m_wrOwnerParent.Get())
                {
                    IFC_RETURN(GetAsCastingSource(&spCastingSource));
                    IFC_RETURN(pArgs->get_SelectedCastingDevice(&spCastingDevice));
                    if (spCastingDevice)
                    {
                        IFC_RETURN(spCastingDevice->CreateCastingConnection(&spCastingConnection));
                        if (spCastingConnection)
                        {
                            IFC_RETURN(spCastingConnection->RequestStartCastingAsync(spCastingSource.Get(), &spAsyncOperation));
                            // Removing previous Connection State change event
                            if (m_spCastingConnection && m_castingConnectStateChangeToken.value != 0)
                            {
                                IFC_RETURN(m_spCastingConnection->remove_StateChanged(m_castingConnectStateChangeToken));
                                m_castingConnectStateChangeToken.value = 0;
                            }

                            ctl::ComPtr<MediaTransportControls> spThis(this);
                            IFC_RETURN(spCastingConnection->add_StateChanged(
                                wrl::Callback<wf::ITypedEventHandler<wm::Casting::CastingConnection*, IInspectable*>>(
                                [this, spThis](wm::Casting::ICastingConnection *pCastingConnection, IInspectable *pArgs) -> HRESULT
                            {
                                wm::Casting::CastingConnectionState state;
                                if (pCastingConnection)
                                {
                                    IFC_RETURN(pCastingConnection->get_State(&state));
                                    if (state == wm::Casting::CastingConnectionState_Connected || state == wm::Casting::CastingConnectionState_Rendering)
                                    {
                                        IFC_RETURN(GetXamlDispatcherNoRef()->RunAsync(MakeCallback(
                                            this, &MediaTransportControls::ResetPlayBackAfterCasting)));
                                        // Hide the picker after successfully connected.
                                        if (m_spCastingDevicePicker)
                                        {
                                            IFC_RETURN(m_spCastingDevicePicker->Hide());
                                        }

                                        // Now that we've hidden the picker, unregister for state changes.  Showing the picker triggers state changes to
                                        // be fired (so the picker can update all it's device states), so we don't want that to trigger the picker to be hidden
                                        // if it is shown again later.
                                        if (m_castingConnectStateChangeToken.value != 0)
                                        {
                                            IFC_RETURN(pCastingConnection->remove_StateChanged(m_castingConnectStateChangeToken));
                                            m_castingConnectStateChangeToken.value = 0;
                                        }
                                    }
                                }
                                return S_OK;
                            }).Get(), &m_castingConnectStateChangeToken));

                            // Attach will release previous reference(if any) and now refer new connection pointer
                            m_spCastingConnection.Attach(spCastingConnection.Detach());

                        }
                    }
                }
                return S_OK;
            }).Get(), &m_castingDeviceSelectedToken));

            IFC_RETURN(m_spCastingDevicePicker->add_CastingDevicePickerDismissed(
                wrl::Callback<wf::ITypedEventHandler<wm::Casting::CastingDevicePicker*, IInspectable*>>(
                [this](wm::Casting::ICastingDevicePicker *pCastingDevicePicker, IInspectable *pArgs) -> HRESULT
            {
                HRESULT hr = E_FAIL;
                if (m_castingPickerDismissedToken.value != 0)
                {
                    hr = GetXamlDispatcherNoRef()->RunAsync(MakeCallback(this, &MediaTransportControls::ResetPlayBackAfterCasting));
                }
                return hr;
            }).Get(), &m_castingPickerDismissedToken));
        }
    }
    spCastingDevicePicker = m_spCastingDevicePicker;
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::ResetPlayBackAfterCasting()
{
    HRESULT hr = S_OK;
    // If we pause video before trying to casting, now need to resume

    if (m_wrOwnerParent.Get() && m_isPausedForCastingSelection)
    {
        IFC(Play(true));
    }
Cleanup:
    m_isPausedForCastingSelection = FALSE;
    return hr;
}

// Checks if the Casting API is available on the machine, and if not
// sets the cast button visibility to collapsed
_Check_return_ HRESULT
MediaTransportControls::HideCastButtonIfNecessary()
{
    if (m_tpCastButton)
    {
        // We only want to return error if we need to hide the button and fail to do so. Any other
        // failures we should ignore so that we don't fail to load the entire MediaTransportControls.
        ctl::ComPtr<wm::Casting::ICastingDeviceStatics> spCastingDeviceStatics;
        if (SUCCEEDED(GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Casting_CastingDevice).Get(), &spCastingDeviceStatics)))
        {
            wrl_wrappers::HString deviceSelector;
            if(SUCCEEDED(spCastingDeviceStatics->GetDeviceSelector(
                (wm::Casting::CastingPlaybackTypes_Audio |
                wm::Casting::CastingPlaybackTypes_Video |
                wm::Casting::CastingPlaybackTypes_Picture),
                deviceSelector.GetAddressOf())))
                {
                    if (deviceSelector.IsEmpty())
                    {
                        IFC_RETURN(m_tpCastButton.Cast<Button>()->put_Visibility(xaml::Visibility_Collapsed));
                        m_isCastSupports = FALSE;
                    }
                }
        }
    }

    return S_OK;
}

// OnLoaded Handler for CommandBar
_Check_return_ HRESULT MediaTransportControls::OnCommandBarLoaded()
{
    HRESULT hr = S_OK;

    IFC(HideMoreButtonIfNecessary());
    IFC(HideCastButtonIfNecessary());

    // doesn't require this event now.
    IFC(DetachHandler(m_epCommandBarLoadedHandler, m_tpCommandBar));
    // ReMeasure
    IFC(MeasureCommandBar());

Cleanup:
    return hr;
}
// Check whether More Button require to visible or collapsed
// this will be removed when overflow feature enabled on the commandbar.
_Check_return_ HRESULT
MediaTransportControls::HideMoreButtonIfNecessary()
{
    HRESULT hr = S_OK;

    if (m_tpCommandBar)
    {
        ctl::ComPtr<wfc::IObservableVector<ICommandBarElement*>> spSecondaryCommandObsVec;
        ctl::ComPtr<wfc::IVector<ICommandBarElement*>> spSecondaryButtons;
        unsigned int count = 0;

        IFC(m_tpCommandBar.Cast<CommandBar>()->get_SecondaryCommands(&spSecondaryCommandObsVec));
        IFCPTR(spSecondaryCommandObsVec);
        IFC(spSecondaryCommandObsVec.As(&spSecondaryButtons));
        IFC(spSecondaryButtons->get_Size(&count));
        // if there is no secondary buttons exist in the commandbar, hide the expand button.
        // Default MTC doesn't have secondary commands unless some re-template MTC, this is case doesn't arise.
        if (count == 0)
        {
            ctl::ComPtr<IUIElement> spMoreButton;
            IFC(m_tpCommandBar.Cast<CommandBar>()->GetTemplatePart<IUIElement>(STR_LEN_PAIR(L"MoreButton"),
                                                                        spMoreButton.ReleaseAndGetAddressOf()));

            if (spMoreButton)
            {
                IFC(spMoreButton->put_Visibility(xaml::Visibility_Collapsed));
            }
        }
    }
Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::OnVolumeSliderPointerWheelChanged(
                        _In_ IInspectable* pSender,
                        _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{

    HRESULT hr = S_OK;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
    ctl::ComPtr<IUIElement> spIUIElement;
    int mouseWheelDelta = 0;
    DOUBLE sliderValue = 0.0;

    ASSERT(pArgs);
    ASSERT(pSender);

    IFC(ctl::do_query_interface(spIUIElement,pSender));
    IFC(pArgs->GetCurrentPoint(this, &spPointerPoint));
    IFCEXPECT(spPointerPoint);
    IFC(spPointerPoint->get_Properties(&spPointerProperties));
    IFCEXPECT(spPointerProperties);
    IFC(spPointerProperties->get_MouseWheelDelta(&mouseWheelDelta));

    IFC(m_tpTHVolumeSlider.Cast<Slider>()->get_Value(&sliderValue));

    sliderValue = sliderValue + VolumeSliderWheelScrollStep * (mouseWheelDelta / WHEEL_DELTA);
    IFC(m_tpTHVolumeSlider.Cast<Slider>()->put_Value(sliderValue));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Update to FullScreenMode in the FullWindow Case.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
MediaTransportControls::UpdateFullScreenMode(BOOLEAN isFullWindow)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    ctl::ComPtr<wuv::IApplicationView3> spAppView3;
    BOOLEAN isfullScreenmode = FALSE;
    BOOLEAN result = FALSE;

    IFC(GetFullScreenView(&spAppView3));
    if (!spAppView3)
    {
        goto Cleanup;
    }

    IFC(spAppView3->get_IsFullScreenMode(&isfullScreenmode));

    // If in full window, then we make sure switch it to full screen.
    if (isFullWindow && !isfullScreenmode)
    {
        IFC(spAppView3->TryEnterFullScreenMode(&result));
        if (result)
        {
            m_isFullScreen = TRUE;
            m_isFullScreenPending = TRUE;
        }
    }
    // If in non-full window, then we make sure reset back from fullscreen only at least once fullscreen scenario started by tapping fullwindows button or launched as fullscreen
    if (!isFullWindow && isfullScreenmode && (m_isFullScreenClicked || m_isLaunchedAsFullScreen))
    {
        IFC(spAppView3->ExitFullScreenMode());
        m_isFullScreen = FALSE;
        if (m_isFullScreenPending)
        {
            m_isFullScreenPending = FALSE;
        }

    }

Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetFullScreenView(_Outptr_opt_ wuv::IApplicationView3** ppAppView)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    *ppAppView = nullptr;

    ctl::ComPtr<wuv::IApplicationViewStatics2> spAppViewStatics;
    ctl::ComPtr<wuv::IApplicationView> spAppView;
    ctl::ComPtr<wuv::IApplicationView3> spAppView3;

    IFC(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &spAppViewStatics));
    if (SUCCEEDED(spAppViewStatics->GetForCurrentView(&spAppView)))
    {
        IFC(spAppView.As<wuv::IApplicationView3>(&spAppView3));

        *ppAppView = spAppView3.Detach();
    }

Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetMuted(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    *value = FALSE;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            IFC(m_spMediaPlayer->get_IsMuted(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SetMuted(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            IFC(m_spMediaPlayer->put_IsMuted(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetFullWindow(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<xaml_controls::IMediaPlayerElement> spOwnerMPE;
        IFC(m_wrOwnerParent.As(&spOwnerMPE))
        if (spOwnerMPE.Get())
        {
            IFC(spOwnerMPE.Cast<MediaPlayerElement>()->get_IsFullWindow(value));
        }
    }
Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

HRESULT MediaTransportControls::SetMediaPlayerElementFullWindow(ctl::ComPtr<xaml_controls::IMediaPlayerElement> spMediaPlayer, BOOLEAN value)
{
#if false // DISABLE_FULL_WINDOW
    IFC_RETURN(spMediaPlayer->put_IsFullWindow(value));
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SetFullWindow(_In_ BOOLEAN value)
{
    HRESULT hr = S_OK;
#if false // DISABLE_FULL_WINDOW
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<xaml_controls::IMediaPlayerElement> spOwnerMPE;
        IFC(m_wrOwnerParent.As(&spOwnerMPE))
        if (spOwnerMPE.Get())
        {
            // Changing IsFullWindow will update the parent of our parent (e.g. the LayoutRoot grid)
            // We need to complete the event call and then let the reparenting happen after that.
            // This will ensure that no peers are referenced while changing the parent.
            IFC(GetXamlDispatcherNoRef()->RunAsync(
                MakeCallback(
                SetMediaPlayerElementFullWindow, spOwnerMPE, value)));
        }
    }
Cleanup:
#endif // DISABLE_FULL_WINDOW
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetStretch(_Out_ xaml_media::Stretch *value)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<xaml_controls::IMediaPlayerElement> spIMPE;
        IFC(m_wrOwnerParent.As(&spIMPE))
        if (spIMPE.Get())
        {
            ctl::ComPtr<MediaPlayerElement> spMPE = 
                static_cast<MediaPlayerElement*>(ctl::impl_cast<MediaPlayerElementGenerated>(spIMPE.Get()));

            IFC(spMPE->get_Stretch(value));
        }
    }

Cleanup:
    return hr;
}
_Check_return_ HRESULT
MediaTransportControls::SetStretch(_In_ xaml_media::Stretch value)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<xaml_controls::IMediaPlayerElement> spIMPE;
        IFC(m_wrOwnerParent.As(&spIMPE))
        if (spIMPE.Get())
        {
            ctl::ComPtr<MediaPlayerElement> spMPE = 
                static_cast<MediaPlayerElement*>(ctl::impl_cast<MediaPlayerElementGenerated>(spIMPE.Get()));

            IFC(spMPE->put_Stretch(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetVolume(_Out_ DOUBLE *value)
{
    HRESULT hr = S_OK;

    *value = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            IFC(m_spMediaPlayer->get_Volume(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SetVolume(_In_ DOUBLE value)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            IFC(m_spMediaPlayer->put_Volume(value));
        }
    }

Cleanup:
    return hr;

}

_Check_return_ HRESULT
MediaTransportControls::GetPosition(_Out_ wf::TimeSpan *value)
{
    HRESULT hr = S_OK;

    (*value).Duration = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        IFC(GetCurrentPlaybackSession(&spPlaybackSession));
        if (spPlaybackSession)
        {
            IFC(spPlaybackSession->get_Position(value));
        }
    }

Cleanup:
    return hr;
}
_Check_return_ HRESULT
MediaTransportControls::SetPosition(_In_ wf::TimeSpan value, _In_ bool isScrubber)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (isScrubber)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC(GetCurrentPlaybackSession(&spPlaybackSession));
            if (spPlaybackSession)
            {
                IFC(spPlaybackSession->put_Position(value));
            }
        }
        else
        {
            ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
            IFC(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
            if (spMediaPlayer2)
            {
                IFC(MediaPlaybackDataSourceExtension_SendPlaybackPositionChangeRequest(spMediaPlayer2.Get(), value.Duration));
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetAudioTrackCount(_Out_ INT *value)
{
    HRESULT hr = S_OK;

    *value = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
            if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
            {
                UINT audioTracks = 0;
                ctl::ComPtr<wfc::IVectorView<wmc::AudioTrack*>> spAudioTracks;
                spPlaybackItem->get_AudioTracks(&spAudioTracks);
                IFC(spAudioTracks->get_Size(&audioTracks));
                *value = audioTracks;
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetCCTrackCount(_Out_ UINT *pValue)
{
    HRESULT hr = S_OK;

    *pValue = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
            if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
            {
                ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTracks;
                IFC(spPlaybackItem->get_TimedMetadataTracks(&spTracks));
                IFC(GetSupportedTrackCount(spTracks.Get(), spPlaybackItem.Get(), pValue));
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetSupportedTrackCount(_In_ wfc::IVectorView<wmc::TimedMetadataTrack*> *pList,
        _In_ wmp::IMediaPlaybackItem* pCurrentItem,
        _Out_ UINT *pValue)
{
    ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTracks = pList;
    ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem = pCurrentItem;
    unsigned int trackCount = 0;
    unsigned int supportedTrackCount = 0;

    *pValue = 0;
    IFC_RETURN(spTracks->get_Size(&trackCount));

    for (unsigned int i = 0; i < trackCount; i++)
    {
        ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
        IFC_RETURN(spTracks->GetAt(i, &spTrack));
        if (CTimedTextSource::IsSupportedTrack(spTrack.Get(), spPlaybackItem.Get()))
        {
            supportedTrackCount++;
        }
    }

    *pValue = supportedTrackCount;
    return S_OK;
}


_Check_return_ HRESULT
MediaTransportControls::GetDownloadProgress(_Out_ DOUBLE *value)
{
    HRESULT hr = S_OK;

    *value = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        IFC(GetCurrentPlaybackSession(&spPlaybackSession));
        if (spPlaybackSession)
        {
            IFC(spPlaybackSession->get_DownloadProgress(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetPlaybackRate(_Out_ DOUBLE *value)
{
    HRESULT hr = S_OK;

    *value = 0;
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        IFC(GetCurrentPlaybackSession(&spPlaybackSession));
        if (spPlaybackSession)
        {
            IFC(spPlaybackSession->get_PlaybackRate(value));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::SetPlaybackRate(_In_ DOUBLE value, _In_ bool isScrubber)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (isScrubber)
        {
            ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
            IFC(GetCurrentPlaybackSession(&spPlaybackSession));
            if (spPlaybackSession)
            {
                IFC(spPlaybackSession->put_PlaybackRate(value));
            }
        }
        else
        {
            ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
            IFC(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
            if (spMediaPlayer2)
            {
                IFC(MediaPlaybackDataSourceExtension_SendPlaybackRateChangeRequest(spMediaPlayer2.Get(), value));
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::Stop()
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        IFC(GetCurrentPlaybackSession(&spPlaybackSession));
        if (spPlaybackSession)
        {
            wf::TimeSpan zeroPosition = {};
            IFC(spPlaybackSession->put_Position(zeroPosition));
            IFC(m_spMediaPlayer->Pause());
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::Play(_In_ bool isCast)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        // While in casting, we directly calling MediaPlayer.
        // Also in buffer state we directly calling MediaPlayer, STMC might skip Play/Pause commands.
        if (isCast|| m_isBuffering)
        {
            if (m_spMediaPlayer)
            {
                IFC(m_spMediaPlayer->Play());
            }
        }
        else
        {
            ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
            IFC(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
            if (spMediaPlayer2)
            {
                IFC(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_Play));
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::Pause(_In_ bool isCast)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        // While in casting, we directly calling MediaPlayer.
        // Also in buffer state we directly calling MediaPlayer, STMC might skip Play/Pause commands.
        if (isCast || m_isBuffering)
        {
            if (m_spMediaPlayer)
            {
                IFC(m_spMediaPlayer->Pause());
            }
        }
        else
        {
            ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
            IFC(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
            if (spMediaPlayer2)
            {
                IFC(MediaPlaybackDataSourceExtension_SendMediaPlaybackCommand(spMediaPlayer2.Get(), MediaPlaybackDataSourceExtension_MediaPlaybackCommands::MediaPlaybackCommand_Pause));
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetAsCastingSource(_Outptr_opt_ wm::Casting::ICastingSource** returnValue)
{
    HRESULT hr = S_OK;

    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        if (m_spMediaPlayer)
        {
            ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
            IFC(m_spMediaPlayer.As(&spMediaPlayer3));
            IFC(spMediaPlayer3->GetAsCastingSource(returnValue));
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
MediaTransportControls::GetRepeatMode(_Out_ MediaPlaybackDataSourceExtension_RepeatMode* value)
{
    *value = MediaPlaybackDataSourceExtension_RepeatMode::None;

    if (MTCParent_MediaPlayerElement == m_parentType && m_spMediaPlayer)
    {
        BOOLEAN isLoopingEnabled;

        IFC_RETURN(m_spMediaPlayer->get_IsLoopingEnabled(&isLoopingEnabled));
        if (isLoopingEnabled)
        {
            *value = MediaPlaybackDataSourceExtension_RepeatMode::One;
        }
        else if (m_spMediaPlaybackList)
        {
            // When IsLoopingEnabled isn't set, check for AutoRepeatEnabled on the IMediaPlaybackList.
            BOOLEAN isAutoRepeatEnabled;

            IFC_RETURN(m_spMediaPlaybackList->get_AutoRepeatEnabled(&isAutoRepeatEnabled));
            if (isAutoRepeatEnabled)
            {
                *value = MediaPlaybackDataSourceExtension_RepeatMode::All;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SetRepeatMode(_In_ MediaPlaybackDataSourceExtension_RepeatMode value)
{
    if (MTCParent_MediaPlayerElement == m_parentType)
    {
        ctl::ComPtr<wmp::IMediaPlayer2> spMediaPlayer2;
        IFC_RETURN(GetMediaPlayer2ForPlaybackDataSource(&spMediaPlayer2));
        if (spMediaPlayer2)
        {
            IFC_RETURN(MediaPlaybackDataSourceExtension_SendRepeatModeChangeRequest(spMediaPlayer2.Get(), value));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnCoreWindowKeyDown(
    _In_ wuc::IKeyEventArgs* pArgs)
{
    // Ignore events if MTC is not enabled.
    if (m_transportControlsEnabled)
    {
        if (IsInLiveTree())
        {
            wsy::VirtualKey key = {};
            IFC_RETURN(pArgs->get_VirtualKey(&key));
            if (!m_controlPanelIsVisible) // Make sure only when MTC is not showing up on the screen.
            {
                if (XboxUtility::IsGamepadNavigationDirection(key) || XboxUtility::IsGamepadNavigationAccept(key))
                {
                    CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this->GetHandle());
                    if(pFocusManager && pFocusManager->GetFocusedElementNoRef() == nullptr)
                    {
                        if (m_tpPlayPauseButton)
                        {
                            BOOLEAN focused = FALSE;
                            IFC_RETURN(m_tpPlayPauseButton.Cast<ButtonBase>()->Focus(xaml::FocusState_Keyboard, &focused));
                            ASSERT(focused);
                        }
                    }
                    IFC_RETURN(StopControlPanelHideTimer());
                    IFC_RETURN(ShowControlPanel());
                    IFC_RETURN(StartControlPanelHideTimer());
                }
            }
        }
    }
    return S_OK;
}

IFACEMETHODIMP
MediaTransportControls::add_ThumbnailRequested(
    _In_ wf::ITypedEventHandler<xaml_controls::MediaTransportControls*, xaml_media::MediaTransportControlsThumbnailRequestedEventArgs*>* pValue,
    _Out_ EventRegistrationToken* ptToken)
{
    IFC_RETURN(MediaTransportControlsGenerated::add_ThumbnailRequested(pValue, ptToken));
    if (!m_isThumbnailEnabled)
    {
        m_isThumbnailEnabled = TRUE;
    }

    return S_OK;
}

IFACEMETHODIMP
MediaTransportControls::remove_ThumbnailRequested(_In_ EventRegistrationToken tToken)
{
    ThumbnailRequestedEventSourceType* pEventSource = nullptr;

    IFC_RETURN(MediaTransportControlsGenerated::remove_ThumbnailRequested(tToken));
    IFC_RETURN(MediaTransportControlsGenerated::GetThumbnailRequestedEventSourceNoRef(&pEventSource));
    if (!pEventSource->HasHandlers())
    {
        m_isThumbnailEnabled = FALSE;
    }

    return S_OK;
}
_Check_return_ HRESULT
MediaTransportControls::EnableValueChangedEventThrottlingOnSliderAutomation(bool value)
{
    if (m_tpMediaPositionSlider)
    {
        ctl::ComPtr<IAutomationPeer> spAutomationPeer;
        ctl::ComPtr<IRangeBaseAutomationPeer> spRangeBaseAutomationPeer;
        IFC_RETURN(m_tpMediaPositionSlider.Cast<Slider>()->GetOrCreateAutomationPeer(&spAutomationPeer));
        IFC_RETURN(spAutomationPeer.As(&spRangeBaseAutomationPeer));
        IFC_RETURN(spRangeBaseAutomationPeer.Cast<RangeBaseAutomationPeer>()->EnableValueChangedEventThrottling(value));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding
)
{
    IFC_RETURN(__super::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive)
    {
        IFC_RETURN(SubscribeMediaPlayerEvents());
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    IFC_RETURN(UnSubscribeMediaPlayerEvents());
#if false // DISABLE_COMPACT_OVERLAY
    if (m_isMiniView)
    {
        IFC_RETURN(SetMiniView(false));
    }
#endif // DISABLE_COMPACT_OVERLAY
    IFC_RETURN(__super::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnBorderSizeChanged()
{
    if (m_tpControlPanelVisibilityBorder)
    {
        // Clip the border for restrict animations rendering within Border.
        double height = 0;
        double width = 0;
        ctl::ComPtr<RectangleGeometry> spClipGeometry;
        wf::Rect clipRect = {};

        IFC_RETURN(m_tpControlPanelVisibilityBorder.Cast<Border>()->get_ActualHeight(&height));
        IFC_RETURN(m_tpControlPanelVisibilityBorder.Cast<Border>()->get_ActualWidth(&width));
        clipRect.Width = static_cast<float>(width);
        clipRect.Height = static_cast<float>(height);
        IFC_RETURN(ctl::make<RectangleGeometry>(&spClipGeometry));
        IFC_RETURN(spClipGeometry->put_Rect(clipRect));
        IFC_RETURN(m_tpControlPanelVisibilityBorder.Cast<Border>()->put_Clip(spClipGeometry.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateTimeAutomationProperties()
{
    wrl_wrappers::HString strAutomationName;
    // Update TimeElapsed/TimeRemaining Automate properties with actual values only when video is paused
    if (m_tpTimeElapsedElement && !ctl::is<xaml_controls::IContentControl>(m_tpTimeElapsedElement.Get()))
    {
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_TIME_ELAPSED, strAutomationName.ReleaseAndGetAddressOf()));
        // Make sure source loaded and content is not playing
        if (m_sourceLoaded && !m_isPlaying)
        {
            wrl_wrappers::HString  strTimeElapsedText;
            ctl::ComPtr<xaml_controls::ITextBlock> spTimeElapsedTextBlock;
            IFC_RETURN(m_tpTimeElapsedElement.As(&spTimeElapsedTextBlock));
            IFC_RETURN(spTimeElapsedTextBlock->get_Text(strTimeElapsedText.ReleaseAndGetAddressOf()));
            IFC_RETURN(strAutomationName.Concat(strTimeElapsedText, strAutomationName));
        }
        IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpTimeElapsedElement.Cast<FrameworkElement>(), strAutomationName));
    }

    if (m_tpTimeRemainingElement && !ctl::is<xaml_controls::IContentControl>(m_tpTimeRemainingElement.Get()))
    {
        IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_MEDIA_TIME_REMAINING, strAutomationName.ReleaseAndGetAddressOf()));
        // Make sure source loaded and content is not playing also not live content
        if (m_sourceLoaded && !m_isPlaying && !IsLiveContent())
        {
            wrl_wrappers::HString strTimeRemainingText;
            ctl::ComPtr<xaml_controls::ITextBlock> spTimeRemainingTextBlock;
            IFC_RETURN(m_tpTimeRemainingElement.As(&spTimeRemainingTextBlock));
            IFC_RETURN(spTimeRemainingTextBlock->get_Text(strTimeRemainingText.ReleaseAndGetAddressOf()));
            IFC_RETURN(strAutomationName.Concat(strTimeRemainingText, strAutomationName));
        }
        IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpTimeRemainingElement.Cast<FrameworkElement>(), strAutomationName));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnVisibilityVisualStateChanged(_In_ IVisualStateChangedEventArgs* pEventArgs)
{
    ctl::ComPtr<IVisualState> spVisualState;
    wrl_wrappers::HString strStateName;

    IFC_RETURN(pEventArgs->get_NewState(&spVisualState));
    if (spVisualState)
    {
        IFC_RETURN(spVisualState->get_Name(strStateName.GetAddressOf()));
        if (strStateName == wrl_wrappers::HStringReference(L"ControlPanelFadeIn"))
        {
            if (!m_controlPanelIsVisible)
            {
                // FadeIn Visual State called thru external, we need to call implicitly MTC to Show Panel
                m_isVSStateChangeExternal = TRUE;
                IFC_RETURN(ShowControlPanel());
            }
        }
        else
        if (strStateName == wrl_wrappers::HStringReference(L"ControlPanelFadeOut"))
        {
            if (m_controlPanelIsVisible)
            {
                // FadeOut Visual State called thru external, we need to call implicitly MTC to Hide Panel
                m_isVSStateChangeExternal = TRUE;
                IFC_RETURN(HideControlPanel());
            }
        }
    }
    return S_OK;
}

//
_Check_return_ HRESULT
MediaTransportControls::UpdateRepeatButtonUI()
{
    if (m_transportControlsEnabled  && m_wrOwnerParent.Get())
    {
        if (m_tpRepeatButton)
        {
            wrl_wrappers::HString strAutomationName;
            MediaPlaybackDataSourceExtension_RepeatMode repeatMode;
            IFC_RETURN(GetRepeatMode(&repeatMode));
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(
                (repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::One) ?
                    UIA_MEDIA_REPEAT_ONE :
                (repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::All) ?
                    UIA_MEDIA_REPEAT_ALL :
                    UIA_MEDIA_REPEAT_NONE,
                strAutomationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpRepeatButton.Cast<ToggleButton>(), strAutomationName));
            IFC_RETURN(UpdateTooltipText(m_tpRepeatButton.Cast<ToggleButton>(), strAutomationName));
            if (MTCParent_MediaPlayerElement == m_parentType)
            {
                IFC_RETURN(UpdateRepeatButtonUIFromMPE());
            }
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnRepeatButtonClicked()
{
    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        MediaPlaybackDataSourceExtension_RepeatMode repeatMode;
        IFC_RETURN(GetRepeatMode(&repeatMode));
        if (!m_spMediaPlaybackList) // if no playlist
        {
            IFC_RETURN(SetRepeatMode(
                repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::None ?
                    MediaPlaybackDataSourceExtension_RepeatMode::One :
                    MediaPlaybackDataSourceExtension_RepeatMode::None));
        }
        else
        {
            IFC_RETURN(SetRepeatMode(
                repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::None ?
                    MediaPlaybackDataSourceExtension_RepeatMode::One :
                    (repeatMode == MediaPlaybackDataSourceExtension_RepeatMode::One) ?
                        MediaPlaybackDataSourceExtension_RepeatMode::All : MediaPlaybackDataSourceExtension_RepeatMode::None));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SetTabIndex()
{
    ctl::ComPtr<xaml::IUIElement> spUIElement;
    if (m_isCompact)
    {
        int idx = 0;
        if (m_tpTHLeftSidePlayPauseButton)
        {
            IFC_RETURN(m_tpTHLeftSidePlayPauseButton.As(&spUIElement));
            IFC_RETURN(spUIElement->put_TabIndex(idx++));
        }
        if (m_tpMediaPositionSlider)
        {
            IFC_RETURN(m_tpMediaPositionSlider.As(&spUIElement));
            IFC_RETURN(spUIElement->put_TabIndex(idx++));
        }
        if (m_tpCommandBar)
        {
            IFC_RETURN(m_tpCommandBar.As(&spUIElement));
            IFC_RETURN(spUIElement->put_TabIndex(idx++));
        }
    }
    else
    {
        if (m_tpCommandBar)
        {
            IFC_RETURN(m_tpCommandBar.As(&spUIElement));
            IFC_RETURN(spUIElement->put_TabIndex(0));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::OnCompactOverlayButtonClicked()
{
#if false // DISABLE_COMPACT_OVERLAY
    if (m_transportControlsEnabled && m_wrOwnerParent.Get())
    {
        if (m_isFullScreen)
        {
            // In Full Screen Set the MiniView using existing fullwindow
            IFC_RETURN(SetMiniView(m_isFullWindow));
        }
        else
        {
            m_isMiniViewClicked = TRUE;
            IFC_RETURN(SetFullWindow(!m_isFullWindow));
        }
    }
#endif // DISABLE_COMPACT_OVERLAY
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::UpdateMiniViewUI()
{
#if false // DISABLE_FULL_WINDOW
    if (m_transportControlsEnabled  && m_wrOwnerParent.Get())
    {
        BOOLEAN isFullWindow = FALSE;
        IFC_RETURN(GetFullWindow(&isFullWindow));
        m_isFullWindow = isFullWindow;
        IFC_RETURN(SetMiniView(m_isFullWindow));
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaTransportControls::SetMiniView(_In_ bool bIsEnable)
{
#if false // DISABLE_COMPACT_OVERLAY
    ctl::ComPtr<wf::IAsyncOperation<bool>> spAsyncOperation;
    ctl::ComPtr<wuv::IApplicationView4> spAppView4;
    wrl_wrappers::HString strAutomationName;

    IFC_RETURN(GetMiniView(&spAppView4));
    if (spAppView4)
    {
        if (bIsEnable)
        {
            if (!m_isMiniView) // Enter MiniView
            {
                if (m_lastKnownMiniViewWidth > 0 && m_lastKnownMiniViewHeight > 0) //use the last known height/width for Mini View
                {
                    ctl::ComPtr<wuv::IViewModePreferences> viewModePreference;
                    ctl::ComPtr<wuv::IViewModePreferencesStatics> viewModePreferencesStatics;
                    wf::Size preferredSize = { static_cast<float>(m_lastKnownMiniViewWidth), static_cast<float>(m_lastKnownMiniViewHeight) };
                    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ViewModePreferences).Get(),
                        &viewModePreferencesStatics));
                    IFC_RETURN(viewModePreferencesStatics->CreateDefault(wuv::ApplicationViewMode::ApplicationViewMode_CompactOverlay, &viewModePreference));
                    IFC_RETURN(viewModePreference->put_ViewSizePreference(wuv::ViewSizePreference::ViewSizePreference_Custom));
                    IFC_RETURN(viewModePreference->put_CustomSize(preferredSize));
                    IFC_RETURN(spAppView4->TryEnterViewModeWithPreferencesAsync(wuv::ApplicationViewMode::ApplicationViewMode_CompactOverlay,
                        viewModePreference.Get(), &spAsyncOperation));
                }
                else
                {
                    IFC_RETURN(spAppView4->TryEnterViewModeAsync(wuv::ApplicationViewMode::ApplicationViewMode_CompactOverlay, &spAsyncOperation));
                }
                m_isMiniView = TRUE;
            }

        }
        else // Exit MiniView
        {
            wf::Rect layoutBounds = {};
            IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(GetHandle(), &layoutBounds));
            // Retain Last known MiniView Width/Height before exit the MiniView.
            m_lastKnownMiniViewWidth = layoutBounds.Width;
            m_lastKnownMiniViewHeight = layoutBounds.Height;

            if (m_isMiniView)
            {
                IFC_RETURN(spAppView4->TryEnterViewModeAsync(wuv::ApplicationViewMode::ApplicationViewMode_Default, &spAsyncOperation));
                m_isMiniView = FALSE;
            }
        }

        if (m_tpCompactOverlayButton)
        {
            IFC_RETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(m_isMiniView ? UIA_MEDIA_EXIT_MINIVIEW : UIA_MEDIA_MINIVIEW, strAutomationName.ReleaseAndGetAddressOf()));
            IFC_RETURN(DirectUI::AutomationProperties::SetNameStatic(m_tpCompactOverlayButton.Cast<Button>(), strAutomationName));
            IFC_RETURN(AddTooltip(m_tpCompactOverlayButton.Cast<Button>(), strAutomationName));
        }
    }
#endif // DISABLE_COMPACT_OVERLAY
    return S_OK;
}


_Check_return_ HRESULT
MediaTransportControls::GetMiniView(_Outptr_opt_ wuv::IApplicationView4** ppAppView)
{
#if false // DISABLE_COMPACT_OVERLAY
    *ppAppView = nullptr;

    ctl::ComPtr<wuv::IApplicationViewStatics2> spAppViewStatics;
    ctl::ComPtr<wuv::IApplicationView> spAppView;
    ctl::ComPtr<wuv::IApplicationView4> spAppView4;

    IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_ApplicationView).Get(), &spAppViewStatics));
    if (SUCCEEDED(spAppViewStatics->GetForCurrentView(&spAppView)))
    {
        IFC_RETURN(spAppView.As<wuv::IApplicationView4>(&spAppView4));
        *ppAppView = spAppView4.Detach();
    }
#endif // DISABLE_COMPACT_OVERLAY
    return S_OK;
}
