// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "MUX-ETWEvents.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include "TestCleanupWrapper.h"
#include <TreeHelper.h>
#include "MediaPlayerElementMTCCustomizationTests.h"
#include "SafeEventRegistration.h"
#include <FocusTestHelper.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace RuntimeFeatureBehavior;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        bool MediaPlayerElementMTCCustomizationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool MediaPlayerElementMTCCustomizationTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }


        bool MediaPlayerElementMTCCustomizationTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        MediaPlayerElement^ MediaPlayerElementMTCCustomizationTests::SetupMediaPlayerElementUI()
        {
            TestServices::WindowHelper->SetWindowSizeOverride(Size(600, 400));
            xaml_controls::MediaPlayerElement^ mpe;
            xaml_controls::Grid^ grid;
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating in XAML...");
                grid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <MediaPlayerElement x:Name='theMedia' AreTransportControlsEnabled='true' />"
                    L"</Grid>"));

                mpe = safe_cast<xaml_controls::MediaPlayerElement^>(grid->FindName(L"theMedia"));
                VERIFY_IS_NOT_NULL(mpe);
                auto testMediaTransportControls = mpe->TransportControls;
                VERIFY_IS_NOT_NULL(testMediaTransportControls);
                TestServices::WindowHelper->WindowContent = grid;
            });
            TestServices::WindowHelper->WaitForIdle();
            return mpe;
        }

        //------------------------------------------------------------------------
        // Test case: Volume Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::VolumeButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for VolumeButton visible by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeButtonVisible);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide VolumeButton");
                testMediaTransportControls->IsVolumeButtonVisible = false;
                LOG_OUTPUT(L"Verfiy for Hide VolumeButton");
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show VolumeButton");
                testMediaTransportControls->IsVolumeButtonVisible = true;
                LOG_OUTPUT(L"Verfiy for Show VolumeButton");
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for VolumeButton Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeEnabled);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable VolumeButton");
                testMediaTransportControls->IsVolumeEnabled = false;
                LOG_OUTPUT(L"Verfiy for VolumeButton Disable");
                VERIFY_IS_FALSE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable VolumeButton");
                testMediaTransportControls->IsVolumeEnabled = true;
                LOG_OUTPUT(L"Verfiy for VolumeButton Enable");
                VERIFY_IS_TRUE(volumeButton->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Zoom Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::ZoomButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Zoom Button visibility by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsZoomButtonVisible);
                auto zoomButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ZoomButton"));
                VERIFY_IS_NOT_NULL(zoomButton);
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Zoom Button");
                testMediaTransportControls->IsZoomButtonVisible = false;
                LOG_OUTPUT(L"Verfiy for Hide Zoom Button");
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Zoom Button");
                testMediaTransportControls->IsZoomButtonVisible = true;
                LOG_OUTPUT(L"Verfiy for Show Zoom Button");
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Zoom Button Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsZoomEnabled);
                auto zoomButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ZoomButton"));
                VERIFY_IS_NOT_NULL(zoomButton);
                VERIFY_IS_TRUE(zoomButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable Zoom Button");
                testMediaTransportControls->IsZoomEnabled = false;
                LOG_OUTPUT(L"Verfiy for Zoom Button Disable");
                VERIFY_IS_FALSE(zoomButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable Zoom Button");
                testMediaTransportControls->IsZoomEnabled = true;
                LOG_OUTPUT(L"Verfiy for Zoom Button Enable");
                VERIFY_IS_TRUE(zoomButton->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: FullWindow Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FullWindowButtonCustomizationTest()
        {
#if false // DISABLE_FULL_WINDOW
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for FullWindow Button visibility by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsFullWindowButtonVisible);
                auto fullWindowButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FullWindowButton"));
                VERIFY_IS_NOT_NULL(fullWindowButton);
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide FullWindow Button");
                testMediaTransportControls->IsFullWindowButtonVisible = false;
                LOG_OUTPUT(L"Verfiy for Hide FullWindow  Button");
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show FullWindow Button");
                testMediaTransportControls->IsFullWindowButtonVisible = true;
                LOG_OUTPUT(L"Verfiy for Show FullWindow Button");
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for FullWindow Button Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsFullWindowEnabled);
                auto fullWindowButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FullWindowButton"));
                VERIFY_IS_NOT_NULL(fullWindowButton);
                VERIFY_IS_TRUE(fullWindowButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable FullWindow Button");
                testMediaTransportControls->IsFullWindowEnabled = false;
                LOG_OUTPUT(L"Verfiy for FullWindow Button Disable");
                VERIFY_IS_FALSE(fullWindowButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable FullWindow Button");
                testMediaTransportControls->IsFullWindowEnabled = true;
                LOG_OUTPUT(L"Verfiy for FullWindow Button Enable");
                VERIFY_IS_TRUE(fullWindowButton->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
#endif // DISABLE_FULL_WINDOW
        }
        
        //------------------------------------------------------------------------
        // Test case: SeekProgressbar Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::SeekProgressbarTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar visibility by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsSeekBarVisible);
                auto slider = safe_cast<Slider^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ProgressSlider"));
                VERIFY_IS_NOT_NULL(slider);
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Seek Progressbar");
                testMediaTransportControls->IsSeekBarVisible = false;
                LOG_OUTPUT(L"Verify for Hide Seek Progressbar");
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Seek Progressbar");
                testMediaTransportControls->IsSeekBarVisible = true;
                LOG_OUTPUT(L"Verify for Show Seek Progressbar");
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsSeekEnabled);
                auto slider = safe_cast<Slider^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ProgressSlider"));
                VERIFY_IS_NOT_NULL(slider);
                VERIFY_IS_TRUE(slider->IsEnabled);

                LOG_OUTPUT(L"Call Disable Seek Progressbar");
                testMediaTransportControls->IsSeekEnabled = false;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar Disabled");
                VERIFY_IS_FALSE(slider->IsEnabled);

                LOG_OUTPUT(L"Call Enable Seek Progressbar");
                testMediaTransportControls->IsSeekEnabled = true;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar Enabled");
                VERIFY_IS_TRUE(slider->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Playback Rate Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::PlaybackRateTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Playback rate button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsPlaybackRateButtonVisible);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"PlaybackRateButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Playback rate button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Playback rate button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Playback rate button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Playback rate button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Playback rate button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Playback rate button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsPlaybackRateEnabled);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"PlaybackRateButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(button->IsEnabled);

                LOG_OUTPUT(L"Call Enable Playback rate button");
                testMediaTransportControls->IsPlaybackRateEnabled = true;
                LOG_OUTPUT(L"Verfiy for Playback rate button Enabled");
                VERIFY_IS_TRUE(button->IsEnabled);

                LOG_OUTPUT(L"Call Disable Playback rate button");
                testMediaTransportControls->IsPlaybackRateEnabled = false;
                LOG_OUTPUT(L"Verfiy for Playback rate button Disabled");
                VERIFY_IS_FALSE(button->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Fast forward Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FastForwardTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast forward button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastForwardButtonVisible);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FastForwardButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Fast forward button");
                testMediaTransportControls->IsFastForwardButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Fast forward button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Fast forward button");
                testMediaTransportControls->IsFastForwardButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Fast forward button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Fast forward button");
                testMediaTransportControls->IsFastForwardButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast forward button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastForwardEnabled);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FastForwardButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(button->IsEnabled);

                LOG_OUTPUT(L"Call Enable Fast forward button");
                testMediaTransportControls->IsFastForwardEnabled = true;
                LOG_OUTPUT(L"Verfiy for Fast forward button Enabled");
                VERIFY_IS_TRUE(button->IsEnabled);

                LOG_OUTPUT(L"Call Disable Fast forward button");
                testMediaTransportControls->IsFastForwardEnabled = false;
                LOG_OUTPUT(L"Verfiy for Fast forward button Disabled");
                VERIFY_IS_FALSE(button->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Fast Rewind Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FastRewindTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastRewindButtonVisible);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RewindButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Fast Rewind button");
                testMediaTransportControls->IsFastRewindButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Fast Rewind button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Fast Rewind button");
                testMediaTransportControls->IsFastRewindButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Fast Rewind button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Fast Rewind button");
                testMediaTransportControls->IsFastRewindButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastRewindEnabled);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RewindButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(button->IsEnabled);

                LOG_OUTPUT(L"Call Enable Fast Rewind button");
                testMediaTransportControls->IsFastRewindEnabled = true;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button Enabled");
                VERIFY_IS_TRUE(button->IsEnabled);

                LOG_OUTPUT(L"Call Disable Fast Rewind button");
                testMediaTransportControls->IsFastRewindEnabled = false;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button Disabled");
                VERIFY_IS_FALSE(button->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Stop Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::StopTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Stop button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopButtonVisible);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Stop button");
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Stop button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Stop button");
                testMediaTransportControls->IsStopButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Stop button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Stop button");
                testMediaTransportControls->IsStopButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Stop button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopEnabled);
                auto button = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(button->IsEnabled);

                LOG_OUTPUT(L"Call Enable Stop button");
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for Stop button Enabled");
                VERIFY_IS_TRUE(button->IsEnabled);

                LOG_OUTPUT(L"Call Disable Stop button");
                testMediaTransportControls->IsStopEnabled = false;
                LOG_OUTPUT(L"Verfiy for Stop button Disabled");
                VERIFY_IS_FALSE(button->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }


        //------------------------------------------------------------------------
        // Test case: FullWindow and Zoom Buttons Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FullWindowAndZoomButtonsCustomizationTest()
        {
#if false // DISABLE_FULL_WINDOW
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for FullWindow and Zoom Buttons visibility by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsFullWindowButtonVisible);
                auto fullWindowButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FullWindowButton"));
                VERIFY_IS_NOT_NULL(fullWindowButton);
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(testMediaTransportControls->IsZoomButtonVisible);
                auto zoomButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ZoomButton"));
                VERIFY_IS_NOT_NULL(zoomButton);
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide both FullWindow and Zoom Buttons");
                testMediaTransportControls->IsFullWindowButtonVisible = false;
                testMediaTransportControls->IsZoomButtonVisible = false;
                LOG_OUTPUT(L"Verfiy for Hide both FullWindow and Zoom Buttons");
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show FullWindow Button and Hide Zoom Button");
                testMediaTransportControls->IsFullWindowButtonVisible = true;
                LOG_OUTPUT(L"Verfiy for Show FullWindow Button but Hide Zoom Buttons");
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Hide FullWindow Button and Show Zoom Button");
                testMediaTransportControls->IsFullWindowButtonVisible = false;
                testMediaTransportControls->IsZoomButtonVisible = true;
                LOG_OUTPUT(L"Verfiy for Hide FullWindow Button and Show Zoom Button");
                VERIFY_IS_TRUE(fullWindowButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(zoomButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for FullWindow and Zoom Button Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsFullWindowEnabled);
                VERIFY_IS_TRUE(testMediaTransportControls->IsZoomEnabled);
                auto fullWindowButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FullWindowButton"));
                VERIFY_IS_NOT_NULL(fullWindowButton);
                VERIFY_IS_TRUE(fullWindowButton->IsEnabled);
                auto zoomButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ZoomButton"));
                VERIFY_IS_NOT_NULL(zoomButton);
                VERIFY_IS_TRUE(zoomButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable both FullWindow and Zoom Buttons");
                testMediaTransportControls->IsFullWindowEnabled = false;
                testMediaTransportControls->IsZoomEnabled = false;
                LOG_OUTPUT(L"Verfiy for FullWindow and Zoom Buttons Disable");
                VERIFY_IS_FALSE(fullWindowButton->IsEnabled);
                VERIFY_IS_FALSE(zoomButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable FullWindow Button and still Disable Zoom Button");
                testMediaTransportControls->IsFullWindowEnabled = true;
                LOG_OUTPUT(L"Verfiy for FullWindow Button Enable and Disable Zoom Button");
                VERIFY_IS_TRUE(fullWindowButton->IsEnabled);
                VERIFY_IS_FALSE(zoomButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable Zoom Button and Disable FullWindow Button");
                testMediaTransportControls->IsZoomEnabled = true;
                testMediaTransportControls->IsFullWindowEnabled = false;
                LOG_OUTPUT(L"Verfiy for FullWindow Button Disable and Enable ZoomButton");
                VERIFY_IS_FALSE(fullWindowButton->IsEnabled);
                VERIFY_IS_TRUE(zoomButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
#endif // DISABLE_FULL_WINDOW
        }
        
        //------------------------------------------------------------------------
        // Test case: SeekProgressbar and Volume button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::SeekProgressbarAndVoumeButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar visibility and Volume Button Visible by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsSeekBarVisible);
                auto slider = safe_cast<Slider^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ProgressSlider"));
                VERIFY_IS_NOT_NULL(slider);
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeButtonVisible);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide both Seek Progressbar and Volume Button");
                testMediaTransportControls->IsSeekBarVisible = false;
                testMediaTransportControls->IsVolumeButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide both Seek Progressbar and Volume Button");
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Seek Progressbar and Still hide Volume Button");
                testMediaTransportControls->IsSeekBarVisible = true;
                LOG_OUTPUT(L"Verify for Show Seek Progressbar and Hide Volume Button");
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Hide Volume Button and Show Seek Progressbar");
                testMediaTransportControls->IsSeekBarVisible = false;
                testMediaTransportControls->IsVolumeButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Seek Progressbar and Hide Volume Button");
                VERIFY_IS_TRUE(slider->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both Seek Progressbar and Volume Button Enable by default");
                VERIFY_IS_TRUE(testMediaTransportControls->IsSeekEnabled);
                auto slider = safe_cast<Slider^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ProgressSlider"));
                VERIFY_IS_NOT_NULL(slider);
                VERIFY_IS_TRUE(slider->IsEnabled);
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeEnabled);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable both Seek Progressbar and Volume Button");
                testMediaTransportControls->IsSeekEnabled = false;
                testMediaTransportControls->IsVolumeEnabled = false;
                LOG_OUTPUT(L"Verfiy for both Seek Progressbar and Volume Button Disabled");
                VERIFY_IS_FALSE(slider->IsEnabled);
                VERIFY_IS_FALSE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled Seek Progressbar but still Volume Button Disabled");
                testMediaTransportControls->IsSeekEnabled = true;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar Enabled and Volume Button Enabled");
                VERIFY_IS_TRUE(slider->IsEnabled);
                VERIFY_IS_FALSE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled Seek Progressbar and Enabled Volume Button");
                testMediaTransportControls->IsSeekEnabled = false;
                testMediaTransportControls->IsVolumeEnabled = true;
                LOG_OUTPUT(L"Verfiy for Seek Progressbar Disabled and Volume Button Enabled");
                VERIFY_IS_FALSE(slider->IsEnabled);
                VERIFY_IS_TRUE(volumeButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Playback rate button and Volume button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::PlaybackRateButtonAndVoumeButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Playback rate button hide and Volume Button Visible by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsPlaybackRateButtonVisible);
                auto rateButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"PlaybackRateButton"));
                VERIFY_IS_NOT_NULL(rateButton);
                VERIFY_IS_TRUE(rateButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeButtonVisible);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Show both Playback rate button and Hide Volume Button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = true;
                testMediaTransportControls->IsVolumeButtonVisible = false;
                LOG_OUTPUT(L"Verify for Show Playback rate button and Hide Volume Button");
                VERIFY_IS_TRUE(rateButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call hide both Playback rate button and  Volume Button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = false;
                LOG_OUTPUT(L"Verify for hide both Playback rate button and Volume Button");
                VERIFY_IS_TRUE(rateButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Volume Button and Hide Playback rate button");
                testMediaTransportControls->IsPlaybackRateButtonVisible = false;
                testMediaTransportControls->IsVolumeButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Playback rate button and Hide Volume Button");
                VERIFY_IS_TRUE(rateButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(volumeButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both Playback rate button and Volume Button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsPlaybackRateEnabled);
                auto rateButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"PlaybackRateButton"));
                VERIFY_IS_NOT_NULL(rateButton);
                VERIFY_IS_FALSE(rateButton->IsEnabled);
                VERIFY_IS_TRUE(testMediaTransportControls->IsVolumeEnabled);
                auto volumeButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"VolumeMuteButton"));
                VERIFY_IS_NOT_NULL(volumeButton);
                VERIFY_IS_TRUE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled Playback rate button but Volume Button Disabled");
                testMediaTransportControls->IsPlaybackRateEnabled = true;
                testMediaTransportControls->IsVolumeEnabled = false;
                LOG_OUTPUT(L"Verfiy for Playback rate button Enabled and Volume Button Disabled");
                VERIFY_IS_TRUE(rateButton->IsEnabled);
                VERIFY_IS_FALSE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Disable both Playback rate button and Volume Button");
                testMediaTransportControls->IsPlaybackRateEnabled = false;
                testMediaTransportControls->IsVolumeEnabled = false;
                LOG_OUTPUT(L"Verfiy for both Playback rate button and Volume Button Disabled");
                VERIFY_IS_FALSE(rateButton->IsEnabled);
                VERIFY_IS_FALSE(volumeButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled Playback rate button and Enabled Volume Button");
                testMediaTransportControls->IsPlaybackRateEnabled = false;
                testMediaTransportControls->IsVolumeEnabled = true;
                LOG_OUTPUT(L"Verfiy for Playback rate button Disabled and Volume Button Enabled");
                VERIFY_IS_FALSE(rateButton->IsEnabled);
                VERIFY_IS_TRUE(volumeButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Fast forward button and Stop button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FastForwardButtonAndStopButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast Forward button hide and Stop Button Hide by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastForwardButtonVisible);
                auto FFButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FastForwardButton"));
                VERIFY_IS_NOT_NULL(FFButton);
                VERIFY_IS_TRUE(FFButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopButtonVisible);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show both Fast Forward button and Stop Button");
                testMediaTransportControls->IsFastForwardButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Fast Forward button and Stop Button");
                VERIFY_IS_TRUE(FFButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call hide Stop Button and show Fast Forward button");
                testMediaTransportControls->IsFastForwardButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = false;
                LOG_OUTPUT(L"Verify for Show Fast Forward button and Hide Stop Button");
                VERIFY_IS_TRUE(FFButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Stop Button and Hide Fast Forward button");
                testMediaTransportControls->IsFastForwardButtonVisible = false;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Fast Forward button and Hide Stop Button");
                VERIFY_IS_TRUE(FFButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both Fast Forward button and Stop Button Disabled by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastForwardEnabled);
                auto FFButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"FastForwardButton"));
                VERIFY_IS_NOT_NULL(FFButton);
                VERIFY_IS_FALSE(FFButton->IsEnabled);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopEnabled);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled Fast Forward button but Stop Button Disabled");
                testMediaTransportControls->IsFastForwardEnabled = true;
                testMediaTransportControls->IsStopEnabled = false;
                LOG_OUTPUT(L"Verfiy for Fast Forward button Enabled and Stop Button Disabled");
                VERIFY_IS_TRUE(FFButton->IsEnabled);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable both Fast Forward button and Stop Button");
                testMediaTransportControls->IsFastForwardEnabled = true;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for both Fast Forward button and Stop Button Disabled");
                VERIFY_IS_TRUE(FFButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled Fast Forward button and Enabled Stop Button");
                testMediaTransportControls->IsFastForwardEnabled = false;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for Fast Forward button Disabled and Stop Button Enabled");
                VERIFY_IS_FALSE(FFButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Fast rewind button and Stop button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FastRewindButtonAndStopButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button hide and Stop Button hide by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastRewindButtonVisible);
                auto RWButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RewindButton"));
                VERIFY_IS_NOT_NULL(RWButton);
                VERIFY_IS_TRUE(RWButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopButtonVisible);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show both Fast Rewind button and Stop Button");
                testMediaTransportControls->IsFastRewindButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Fast Rewind button and Stop Button");
                VERIFY_IS_TRUE(RWButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call hide Fast Rewind button and show Stop Button");
                testMediaTransportControls->IsFastRewindButtonVisible = false;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for hide Fast Rewind button and show Stop Button");
                VERIFY_IS_TRUE(RWButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call show Fast Rewind button and hide Stop Button");
                testMediaTransportControls->IsFastRewindButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = false;
                LOG_OUTPUT(L"Verify for Show Fast Rewind button and Hide Stop Button");
                VERIFY_IS_TRUE(RWButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both Fast Rewind button and Stop Button Disable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsFastRewindEnabled);
                auto RWButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RewindButton"));
                VERIFY_IS_NOT_NULL(RWButton);
                VERIFY_IS_FALSE(RWButton->IsEnabled);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopEnabled);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled Fast Rewind button but Stop Button Disabled");
                testMediaTransportControls->IsFastRewindEnabled = true;
                testMediaTransportControls->IsStopEnabled = false;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button Enabled and Stop Button Disabled");
                VERIFY_IS_TRUE(RWButton->IsEnabled);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable both Fast Rewind button and Stop Button");
                testMediaTransportControls->IsFastRewindEnabled = true;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for both Fast Rewind button and Stop Button Disabled");
                VERIFY_IS_TRUE(RWButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled Fast Rewind button and Enabled Stop Button");
                testMediaTransportControls->IsFastRewindEnabled = false;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for Fast Rewind button Disabled and Stop Button Enabled");
                VERIFY_IS_FALSE(RWButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Skip forward and Backward Buttons Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::SkipForwardAndBackwardCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Skip forward and backward buttons visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsSkipBackwardButtonVisible);
                auto backbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"SkipBackwardButton"));
                VERIFY_IS_NOT_NULL(backbutton);
                VERIFY_IS_TRUE(backbutton->Visibility == xaml::Visibility::Collapsed);

                VERIFY_IS_FALSE(testMediaTransportControls->IsSkipForwardButtonVisible);
                auto forwardbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"SkipForwardButton"));
                VERIFY_IS_NOT_NULL(forwardbutton);
                VERIFY_IS_TRUE(forwardbutton->Visibility == xaml::Visibility::Collapsed);


                LOG_OUTPUT(L"Call Show Skip forward button");
                testMediaTransportControls->IsSkipForwardButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Skip forward button");
                VERIFY_IS_TRUE(forwardbutton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Show Skip backward button");
                testMediaTransportControls->IsSkipBackwardButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Skip backward button");
                VERIFY_IS_TRUE(backbutton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide skip forward button");
                testMediaTransportControls->IsSkipForwardButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Fast forward button");
                VERIFY_IS_TRUE(forwardbutton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Hide Skip backward button");
                testMediaTransportControls->IsSkipBackwardButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Fast forward button");
                VERIFY_IS_TRUE(backbutton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show skip forward and backward buttons");
                testMediaTransportControls->IsSkipForwardButtonVisible = true;
                testMediaTransportControls->IsSkipBackwardButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for skip backward button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsSkipBackwardEnabled);
                auto backbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"SkipBackwardButton"));
                VERIFY_IS_NOT_NULL(backbutton);
                VERIFY_IS_FALSE(backbutton->IsEnabled);

                LOG_OUTPUT(L"Verfiy for skip forward button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsSkipForwardEnabled);
                auto forwardbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"SkipForwardButton"));
                VERIFY_IS_NOT_NULL(forwardbutton);
                VERIFY_IS_FALSE(forwardbutton->IsEnabled);

                LOG_OUTPUT(L"Call Enable Skip forward button");
                testMediaTransportControls->IsSkipForwardEnabled = true;
                LOG_OUTPUT(L"Verfiy for Skip forward button Enabled");
                VERIFY_IS_TRUE(forwardbutton->IsEnabled);

                LOG_OUTPUT(L"Call Enable Skip Backward button");
                testMediaTransportControls->IsSkipBackwardEnabled = true;
                LOG_OUTPUT(L"Verfiy for Skip Backward button Enabled");
                VERIFY_IS_TRUE(backbutton->IsEnabled);

                LOG_OUTPUT(L"Call Disable Skip forward button");
                testMediaTransportControls->IsSkipForwardEnabled = false;
                LOG_OUTPUT(L"Verfiy for Skip forward button Disabled");
                VERIFY_IS_FALSE(forwardbutton->IsEnabled);

                LOG_OUTPUT(L"Call Disable Skip Backward button");
                testMediaTransportControls->IsSkipBackwardEnabled = false;
                LOG_OUTPUT(L"Verfiy for Skip Backward button Disabled");
                VERIFY_IS_FALSE(backbutton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Skip forward and Backward Buttons Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::NextAndPreviousTracksCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Next and Previous Track buttons visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsNextTrackButtonVisible);
                auto nextbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"NextTrackButton"));
                VERIFY_IS_NOT_NULL(nextbutton);
                VERIFY_IS_TRUE(nextbutton->Visibility == xaml::Visibility::Collapsed);

                VERIFY_IS_FALSE(testMediaTransportControls->IsPreviousTrackButtonVisible);
                auto previousbutton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"PreviousTrackButton"));
                VERIFY_IS_NOT_NULL(previousbutton);
                VERIFY_IS_TRUE(previousbutton->Visibility == xaml::Visibility::Collapsed);


                LOG_OUTPUT(L"Call Show next Track button");
                testMediaTransportControls->IsNextTrackButtonVisible = true;
                LOG_OUTPUT(L"Verify for next track button");
                VERIFY_IS_TRUE(nextbutton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Show previous track button");
                testMediaTransportControls->IsPreviousTrackButtonVisible = true;
                LOG_OUTPUT(L"Verify for previous track button");
                VERIFY_IS_TRUE(previousbutton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide next Track button");
                testMediaTransportControls->IsNextTrackButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide next Track  button");
                VERIFY_IS_TRUE(nextbutton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Hide previous track button");
                testMediaTransportControls->IsPreviousTrackButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide previous track button");
                VERIFY_IS_TRUE(previousbutton->Visibility == xaml::Visibility::Collapsed);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        void MediaPlayerElementMTCCustomizationTests::FullWindowModeShouldRestrictTabsToMTC()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ mpe;
            MediaTransportControls^ mtc;
            Button^ button1;
            Button^ button2;
            Button^ fullWindowButton;

            auto button2GotFocusEvent = std::make_shared<Event>();
            auto button2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

            TestServices::WindowHelper->SetWindowSizeOverride(Size(400, 400));

            xaml_controls::StackPanel^ root;
            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                    L"  <Button x:Name='button1'>button1</Button>"
                    L"  <MediaPlayerElement x:Name='theMedia' AreTransportControlsEnabled='true' />"
                    L"  <Button x:Name='button2'>button2</Button>"
                    L"</StackPanel>"));

                mpe = safe_cast<xaml_controls::MediaPlayerElement^>(root->FindName(L"theMedia"));
                button1 = safe_cast<xaml_controls::Button^>(root->FindName(L"button1"));
                button2 = safe_cast<xaml_controls::Button^>(root->FindName(L"button2"));
                mtc = mpe->TransportControls;

                button2GotFocusRegistration.Attach(button2, [&]() {
                    LOG_OUTPUT(L"Button 2 GotFocus fired!");
                    button2GotFocusEvent->Set();
                });

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            FocusTestHelper::EnsureFocus(button1, FocusState::Keyboard);

            LOG_OUTPUT(L"Hit tab 20 times and make sure focus lands on button2");
            int tabs = 0;
            while (!button2GotFocusEvent->HasFired())
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_LESS_THAN(tabs++, 20);
            }

            LOG_OUTPUT(L"ENTER full window mode");
            RunOnUIThread([&]()
            {
                fullWindowButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(mpe, L"FullWindowButton"));
                VERIFY_IS_NOT_NULL(fullWindowButton);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->Tap(fullWindowButton);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Hit tab 20 times and make sure focus NEVER lands on button2");
            button2GotFocusEvent->Reset();
            for (int i=0; i <20; ++i)
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
            }
            VERIFY_IS_FALSE(button2GotFocusEvent->HasFired());

            LOG_OUTPUT(L"EXIT full window mode");
            TestServices::InputHelper->Tap(fullWindowButton);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Hit tab 20 times and make sure focus lands on button2");
            tabs = 0;
            button2GotFocusEvent->Reset();
            while (!button2GotFocusEvent->HasFired())
            {
                TestServices::KeyboardHelper->Tab();
                TestServices::WindowHelper->WaitForIdle();
                VERIFY_IS_LESS_THAN(tabs++, 20);
            }


        }

        //------------------------------------------------------------------------
        // Test case: Repeat Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::RepeatTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Repeat button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsRepeatButtonVisible);
                auto button = safe_cast<AppBarToggleButton^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RepeatButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Repeat button");
                testMediaTransportControls->IsRepeatButtonVisible = true;
                LOG_OUTPUT(L"Verify for show Repeat button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide Repeat button");
                testMediaTransportControls->IsRepeatButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide Repeat button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show Repeat button");
                testMediaTransportControls->IsRepeatButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Repeat button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsRepeatEnabled);
                auto button = safe_cast<AppBarToggleButton^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RepeatButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(safe_cast<Control^>(button)->IsEnabled);

                LOG_OUTPUT(L"Call Enable Repeat button");
                testMediaTransportControls->IsRepeatEnabled = true;
                LOG_OUTPUT(L"Verfiy for Repeat button Enabled");
                VERIFY_IS_TRUE(safe_cast<Control^>(button)->IsEnabled);

                LOG_OUTPUT(L"Call Disable Repeat button");
                testMediaTransportControls->IsRepeatEnabled = false;
                LOG_OUTPUT(L"Verfiy for Repeat button Disabled");
                VERIFY_IS_FALSE(safe_cast<Control^>(button)->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: Repeat button and Stop button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::RepeatAndStopButtonCustomizationTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for Repeat button hide and Stop Button hide by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsRepeatButtonVisible);
                auto RepButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RepeatButton"));
                VERIFY_IS_NOT_NULL(RepButton);
                VERIFY_IS_TRUE(RepButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopButtonVisible);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show both Repeat button and Stop Button");
                testMediaTransportControls->IsRepeatButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show Repeat button and Stop Button");
                VERIFY_IS_TRUE(RepButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call hide Repeat button and show Stop Button");
                testMediaTransportControls->IsRepeatButtonVisible = false;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for hide Fast Rewind button and show Stop Button");
                VERIFY_IS_TRUE(RepButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call show Repeat button and hide Stop Button");
                testMediaTransportControls->IsRepeatButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = false;
                LOG_OUTPUT(L"Verify for Show Repeat button and Hide Stop Button");
                VERIFY_IS_TRUE(RepButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both Repeat button and Stop Button Disable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsRepeatEnabled);
                auto RepButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"RepeatButton"));
                VERIFY_IS_NOT_NULL(RepButton);
                VERIFY_IS_FALSE(RepButton->IsEnabled);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopEnabled);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled Repeat button but Stop Button Disabled");
                testMediaTransportControls->IsRepeatEnabled = true;
                testMediaTransportControls->IsStopEnabled = false;
                LOG_OUTPUT(L"Verfiy for Repeat button Enabled and Stop Button Disabled");
                VERIFY_IS_TRUE(RepButton->IsEnabled);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable both Repeat button and Stop Button");
                testMediaTransportControls->IsRepeatEnabled = true;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for both Repeat button and Stop Button Disabled");
                VERIFY_IS_TRUE(RepButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled Repeat button and Enabled Stop Button");
                testMediaTransportControls->IsRepeatEnabled = false;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for Repeat button Disabled and Stop Button Enabled");
                VERIFY_IS_FALSE(RepButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case:Show/Hide Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::ShowHideTest()
        {
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();
            Border^ borderPanel = nullptr;
            auto visibleEvent = std::make_shared<Event>();
            auto hideEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for ShowAndHideAutomatically will be true by default");
                VERIFY_IS_TRUE(testMediaTransportControls->ShowAndHideAutomatically);
                borderPanel = safe_cast<Border^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"ControlPanel_ControlPanelVisibilityStates_Border"));
                VERIFY_IS_NOT_NULL(borderPanel);
                LOG_OUTPUT(L"Call False on the ShowAndHideAutomatically Property to Stay MTC always");
                testMediaTransportControls->ShowAndHideAutomatically = false;
                LOG_OUTPUT(L"Verify for Border Opacity");
                VERIFY_IS_TRUE(borderPanel->Opacity == 1);

                borderPanel->RegisterPropertyChangedCallback(Border::OpacityProperty,
                    ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop) {
                    if (borderPanel->Opacity == 0)
                    {
                        hideEvent->Set();
                    }
                }));

                LOG_OUTPUT(L"Call Hide API");
                testMediaTransportControls->Hide();
            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Verify for Hide()");
            hideEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verify for Border Opacity should be zero");
                VERIFY_IS_TRUE(borderPanel->Opacity == 0);

                borderPanel->RegisterPropertyChangedCallback(Border::OpacityProperty,
                    ref new DependencyPropertyChangedCallback([&](DependencyObject^ sender, DependencyProperty^ prop) {
                    if (borderPanel->Opacity == 1)
                    {
                        visibleEvent->Set();
                    }
                }));

                LOG_OUTPUT(L"Call Show API");
                testMediaTransportControls->Show();

            });
            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Verify for Show()");
            visibleEvent->WaitForDefault();
        }
        //------------------------------------------------------------------------
        // Test case: CompactOverlay Button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::CompactOverlayTest()
        {
#if false // DISABLE_COMPACT_OVERLAY
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button visibility by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsCompactOverlayButtonVisible);
                auto button = safe_cast<AppBarButton^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"CompactOverlayButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show CompactOverlay button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = true;
                LOG_OUTPUT(L"Verify for show CompactOverlay button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call Hide CompactOverlay button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = false;
                LOG_OUTPUT(L"Verify for Hide CompactOverlay button");
                VERIFY_IS_TRUE(button->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show CompactOverlay button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = true;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {

                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button Enable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsCompactOverlayEnabled);
                auto button = safe_cast<AppBarButton^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"CompactOverlayButton"));
                VERIFY_IS_NOT_NULL(button);
                VERIFY_IS_FALSE(safe_cast<Control^>(button)->IsEnabled);

                LOG_OUTPUT(L"Call Enable CompactOverlay button");
                testMediaTransportControls->IsCompactOverlayEnabled = true;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button Enabled");
                VERIFY_IS_TRUE(safe_cast<Control^>(button)->IsEnabled);

                LOG_OUTPUT(L"Call Disable CompactOverlay button");
                testMediaTransportControls->IsCompactOverlayEnabled = false;
                LOG_OUTPUT(L"Verfiy for Repeat button Disabled");
                VERIFY_IS_FALSE(safe_cast<Control^>(button)->IsEnabled);

            });
            TestServices::WindowHelper->WaitForIdle();
#endif // DISABLE_COMPACT_OVERLAY
        }
        
        //------------------------------------------------------------------------
        // Test case: CompactOverlay button and Stop button Customizaton Test
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::CompactOverlayAndStopButtonCustomizationTest()
        {
#if false // DISABLE_COMPACT_OVERLAY
            TestCleanupWrapper cleanup;

            MediaPlayerElement^ testMediaPlayerElement = SetupMediaPlayerElementUI();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button hide and Stop Button hide by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsCompactOverlayButtonVisible);
                auto coButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"CompactOverlayButton"));
                VERIFY_IS_NOT_NULL(coButton);
                VERIFY_IS_TRUE(coButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopButtonVisible);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);

                LOG_OUTPUT(L"Call Show both CompactOverlay button and Stop Button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for Show CompactOverlay button and Stop Button");
                VERIFY_IS_TRUE(coButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call hide CompactOverlay button and show Stop Button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = false;
                testMediaTransportControls->IsStopButtonVisible = true;
                LOG_OUTPUT(L"Verify for hide CompactOverlay button and show Stop Button");
                VERIFY_IS_TRUE(coButton->Visibility == xaml::Visibility::Collapsed);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Visible);

                LOG_OUTPUT(L"Call show CompactOverlay button and hide Stop Button");
                testMediaTransportControls->IsCompactOverlayButtonVisible = true;
                testMediaTransportControls->IsStopButtonVisible = false;
                LOG_OUTPUT(L"Verify for Show CompactOverlay button and Hide Stop Button");
                VERIFY_IS_TRUE(coButton->Visibility == xaml::Visibility::Visible);
                VERIFY_IS_TRUE(stopButton->Visibility == xaml::Visibility::Collapsed);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto testMediaTransportControls = testMediaPlayerElement->TransportControls;
                LOG_OUTPUT(L"Verfiy for both CompactOverlay button and Stop Button Disable by default");
                VERIFY_IS_FALSE(testMediaTransportControls->IsRepeatEnabled);
                auto coButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"CompactOverlayButton"));
                VERIFY_IS_NOT_NULL(coButton);
                VERIFY_IS_FALSE(coButton->IsEnabled);
                VERIFY_IS_FALSE(testMediaTransportControls->IsStopEnabled);
                auto stopButton = safe_cast<Button^>(TreeHelper::GetVisualChildByName(testMediaPlayerElement, L"StopButton"));
                VERIFY_IS_NOT_NULL(stopButton);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enabled CompactOverlay button but Stop Button Disabled");
                testMediaTransportControls->IsCompactOverlayEnabled = true;
                testMediaTransportControls->IsStopEnabled = false;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button Enabled and Stop Button Disabled");
                VERIFY_IS_TRUE(coButton->IsEnabled);
                VERIFY_IS_FALSE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Enable both CompactOverlay button and Stop Button");
                testMediaTransportControls->IsCompactOverlayEnabled = true;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for both CompactOverlay button and Stop Button Disabled");
                VERIFY_IS_TRUE(coButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);

                LOG_OUTPUT(L"Call Disabled CompactOverlay button and Enabled Stop Button");
                testMediaTransportControls->IsCompactOverlayEnabled = false;
                testMediaTransportControls->IsStopEnabled = true;
                LOG_OUTPUT(L"Verfiy for CompactOverlay button Disabled and Stop Button Enabled");
                VERIFY_IS_FALSE(coButton->IsEnabled);
                VERIFY_IS_TRUE(stopButton->IsEnabled);
            });
            TestServices::WindowHelper->WaitForIdle();
#endif // DISABLE_COMPACT_OVERLAY
        }
    } } }
} } } }
