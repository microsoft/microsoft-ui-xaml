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
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Hiding fullscreen button for Xaml Islands
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
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // Hiding fullscreen button for Xaml Islands
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
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
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
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CompactOverlayTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the CompactOverlay button.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CompactOverlayAndStopButtonCustomizationTest)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that the MediaTransportControls able to Show/Hide and Enable/Disable the combinations of the CompactOverlay button and stop button.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO: Port Full Screen work to WinAppSDK 1.x master
            END_TEST_METHOD()

        private:
            xaml_controls::MediaPlayerElement^ SetupMediaPlayerElementUI();
            
        };

    } } }
} } } }
