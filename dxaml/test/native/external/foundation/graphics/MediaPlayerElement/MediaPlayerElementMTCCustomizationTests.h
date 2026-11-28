// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        class MediaPlayerElementMTCCustomizationTests : public WEX::TestClass<MediaPlayerElementMTCCustomizationTests>
        {
        public:
            BEGIN_TEST_CLASS(MediaPlayerElementMTCCustomizationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
           
            BEGIN_TEST_METHOD(VolumeButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Volume Control.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ZoomButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Zoom Control.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FullWindowButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the FullWindow Control")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SeekProgressbarTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Seek Progressbar.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlaybackRateTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Playback rate button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastForwardTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the fast forward button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastRewindTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the fast rewind button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StopTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Stop button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FullWindowAndZoomButtonsCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the FullWindow and Zoom Buttons.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SeekProgressbarAndVoumeButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the Seek ProgressBar and Volume Button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlaybackRateButtonAndVoumeButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the playback rate button and volume button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastForwardButtonAndStopButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the fast forward button and stop button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastRewindButtonAndStopButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the fast rewind button and stop button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SkipForwardAndBackwardCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the skip backward and forward buttons")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NextAndPreviousTracksCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide Next and Previous Tracks buttons")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FullWindowModeShouldRestrictTabsToMTC)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that tabs are confined to the MTC when in full-window mode (and only then)")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RepeatTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the Repeat button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RepeatAndStopButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the Repeat button and stop button.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ShowHideTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide using APIs.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CompactOverlayTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the CompactOverlay button.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CompactOverlayAndStopButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the CompactOverlay button and stop button.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SeekSliderAutomationPeerExposesReadOnlyPositionValue)
                TEST_METHOD_PROPERTY(L"Description", L"The MediaTransportControls seek bar automation peer exposes the playback position as a read-only UIA Value.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SeekSliderValueIncludesHoursForLongMedia)
                TEST_METHOD_PROPERTY(L"Description", L"The seek bar automation value includes hours for positions past one hour.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlayPauseButtonTogglesPlaybackState)
                TEST_METHOD_PROPERTY(L"Description", L"Invoking the MediaTransportControls play/pause button plays and pauses the MediaPlayer and updates the button name.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastForwardTrickModeRatesUpdateButtonNameAndPlayResets)
                TEST_METHOD_PROPERTY(L"Description", L"Fast forward enters trick play with increasing rates named on the button; play/pause returns to the normal rate.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RewindTrickModeOrSkipBackFallback)
                TEST_METHOD_PROPERTY(L"Description", L"Rewind enters reverse trick play when the rate is supported, otherwise it skips back.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CommandManagerBehaviorsGateSkipRateAndRepeatButtons)
                TEST_METHOD_PROPERTY(L"Description", L"MediaPlaybackCommandManager behaviors and the MediaTransportControls APIs together decide whether skip, playback rate and repeat buttons are enabled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaFailedShowsLocalizedError)
                TEST_METHOD_PROPERTY(L"Description", L"A media failure shows the localized error message for the reported error in the transport controls.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlaybackListNextPreviousAndItemFailure)
                TEST_METHOD_PROPERTY(L"Description", L"A failing MediaPlaybackList item is reported by the transport controls, and the next/previous track buttons move through a playlist.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CCSelectionFlyoutListsTracksAndSwitchesPresentationMode)
                TEST_METHOD_PROPERTY(L"Description", L"The closed caption menu lists the caption tracks, marks the visible one and switches the presented track.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AudioTrackFlyoutNamesTracksAndSwitchesSelection)
                TEST_METHOD_PROPERTY(L"Description", L"The audio track menu names each audio track, marks the selected one and switches the selected track.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThumbnailRequestedRaisedWhenSeekBarMoves)
                TEST_METHOD_PROPERTY(L"Description", L"Each move of the seek bar seeks the player and raises ThumbnailRequested from the transport controls.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClosedMediaPlayerDoesNotBreakTransportControlsTeardown)
                TEST_METHOD_PROPERTY(L"Description", L"Closing the MediaPlayer before the element unloads does not break the transport controls, which keep working with a new player.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TransportControlsMovedToAnotherElementFollowNewPlayer)
                TEST_METHOD_PROPERTY(L"Description", L"Transport controls moved to another MediaPlayerElement control that element's MediaPlayer.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaBreakDisablesSeekBarWhileBreakPlays)
                TEST_METHOD_PROPERTY(L"Description", L"The seek bar is disabled while a media break plays and re-enabled afterwards.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnsupportedTrickRatesFallBackToSkipOrDisableOrHideButtons)
                TEST_METHOD_PROPERTY(L"Description", L"Without trick play rates, fast forward and rewind skip 30 s forward and 10 s back, and FastPlayFallbackBehaviour Disable/Hide disable or hide the buttons.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TrickModeButtonNamesFollowEveryRateUntilAppResetsRate)
                TEST_METHOD_PROPERTY(L"Description", L"Each fast forward and rewind press steps the trick play rate (2x, 4x, 8x) and the button name follows it; the app setting the original rate leaves trick play.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaStreamSourceDecodeErrorsShowLocalizedErrors)
                TEST_METHOD_PROPERTY(L"Description", L"A decode error reported by a MediaStreamSource, for the player source and for a playback list item, shows the localized decode error.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(KeyboardScrubbingShowsThumbnailToolTipAndCompletesDeferredRequests)
                TEST_METHOD_PROPERTY(L"Description", L"Arrow keys on the seek bar show the thumbnail tooltip while the key is down and raise ThumbnailRequested, whose deferral can be completed after the handler returned.")
            END_TEST_METHOD()

        private:
            xaml_controls::MediaPlayerElement^ SetupMediaPlayerElementUI();
            
        };

    } } }
} } } }
