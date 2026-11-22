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
#include <ControlHelper.h>
#include <MediaHelperMPE.h>
#include <Collection.h>
#include <algorithm>
#include <atomic>
#include <mutex>

#undef GetClassName // Conflicts with automation peer method of the same name

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

        //------------------------------------------------------------------------
        // Helpers for the playback tests below: a MediaPlayerElement with transport controls and an app-owned MediaPlayer
        // (AutoPlay off) hosted in a wide window so the optional buttons are not dropped. Buttons are invoked through UIA
        // and every asynchronous MediaPlayer/transport-controls update is awaited through an event with a bounded wait.
        //------------------------------------------------------------------------
        namespace
        {
            namespace wmpb = ::Windows::Media::Playback;
            namespace wmcore = ::Windows::Media::Core;
            namespace uia_provider = Microsoft::UI::Xaml::Automation::Provider;

            const INT64 c_hnsPerSecond = 10000000;
            const std::chrono::milliseconds c_eventTimeout(15000);

            struct TransportControlsScene
            {
                xaml_controls::Grid^ root = nullptr;
                xaml_controls::MediaPlayerElement^ mpe = nullptr;
                wmpb::MediaPlayer^ player = nullptr;
                xaml_controls::MediaTransportControls^ mtc = nullptr;

                ~TransportControlsScene()
                {
                    // Stop playback and release the XAML objects on the UI thread.
                    try
                    {
                        RunOnUIThread([this]()
                        {
                            if (player != nullptr)
                            {
                                try
                                {
                                    player->Pause();
                                }
                                catch (Platform::Exception^)
                                {
                                    // The player may have been closed by the test.
                                }
                            }
                            mtc = nullptr;
                            mpe = nullptr;
                            player = nullptr;
                            root = nullptr;
                        });
                    }
                    catch (...)
                    {
                        WEX::Logging::Log::Warning(L"Failed to release the MediaPlayerElement scene on the UI thread");
                    }
                }
            };

            void CreateTransportControlsScene(TransportControlsScene& scene, std::function<void(xaml_controls::MediaTransportControls^)> configure = nullptr)
            {
                TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(1000, 500));
                RunOnUIThread([&]()
                {
                    scene.root = ref new xaml_controls::Grid();
                    scene.mpe = ref new xaml_controls::MediaPlayerElement();
                    scene.player = ref new wmpb::MediaPlayer();
                    scene.player->AutoPlay = false;
                    scene.mpe->SetMediaPlayer(scene.player);
                    scene.mpe->AreTransportControlsEnabled = true;
                    scene.mtc = scene.mpe->TransportControls;
                    VERIFY_IS_NOT_NULL(scene.mtc);
                    if (configure)
                    {
                        configure(scene.mtc);
                    }
                    scene.root->Children->Append(scene.mpe);
                    TestServices::WindowHelper->WindowContent = scene.root;
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            void SetSource(TransportControlsScene& scene, std::function<wmpb::IMediaPlaybackSource^()> createSource, bool waitForOpened = true)
            {
                struct OpenResult
                {
                    Event done;
                    std::atomic<bool> failed{ false };
                    std::atomic<int> error{ 0 };
                    std::atomic<HRESULT> extendedError{ S_OK };
                };
                auto result = std::make_shared<OpenResult>();
                auto openedRegistration = CreateSafeEventRegistration(wmpb::MediaPlayer, MediaOpened);
                auto failedRegistration = CreateSafeEventRegistration(wmpb::MediaPlayer, MediaFailed);
                RunOnUIThread([&]()
                {
                    openedRegistration.Attach(
                        scene.player,
                        ref new wf::TypedEventHandler<wmpb::MediaPlayer^, Platform::Object^>([result](wmpb::MediaPlayer^, Platform::Object^)
                    {
                        result->done.Set();
                    }));
                    failedRegistration.Attach(
                        scene.player,
                        ref new wf::TypedEventHandler<wmpb::MediaPlayer^, wmpb::MediaPlayerFailedEventArgs^>([result](wmpb::MediaPlayer^, wmpb::MediaPlayerFailedEventArgs^ args)
                    {
                        result->error.store(static_cast<int>(args->Error));
                        result->extendedError.store(args->ExtendedErrorCode.Value);
                        result->failed.store(true);
                        result->done.Set();
                    }));
                    scene.mpe->Source = createSource();
                });
                if (waitForOpened)
                {
                    result->done.WaitFor(c_eventTimeout);
                    VERIFY_IS_FALSE(result->failed.load(), WEX::Common::String().Format(
                        L"Precondition: the media opens (MediaFailed: MediaPlayerError %d, extended error 0x%08X)", result->error.load(), static_cast<unsigned int>(result->extendedError.load())));
                }
                TestServices::WindowHelper->WaitForIdle();
            }

            wmpb::MediaPlaybackItem^ CreateMediaItem(Platform::String^ fileName)
            {
                auto uri = ref new wf::Uri(MediaHelperMPE::GetDefaultResourcePath() + fileName);
                return ref new wmpb::MediaPlaybackItem(wmcore::MediaSource::CreateFromUri(uri));
            }

            // Must be called on the UI thread.
            template <typename T>
            T^ FindPart(xaml::FrameworkElement^ root, Platform::String^ name)
            {
                auto part = dynamic_cast<T^>(TreeHelper::GetVisualChildByName(root, name));
                VERIFY_IS_NOT_NULL(part, WEX::Common::String().Format(L"Template part '%s'", name->Data()));
                return part;
            }

            bool IsSameObject(Platform::Object^ first, Platform::Object^ second)
            {
                if (first == nullptr || second == nullptr)
                {
                    return first == second;
                }
                Microsoft::WRL::ComPtr<IUnknown> firstIdentity;
                Microsoft::WRL::ComPtr<IUnknown> secondIdentity;
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
                return firstIdentity.Get() == secondIdentity.Get();
            }

            void VerifyText(const wchar_t* expected, Platform::String^ actual, const wchar_t* what)
            {
                VERIFY_IS_TRUE(actual != nullptr && wcscmp(expected, actual->Data()) == 0,
                    WEX::Common::String().Format(L"%s: expected '%s', actual '%s'", what, expected, actual == nullptr ? L"<null>" : actual->Data()));
            }

            bool HasName(xaml::DependencyObject^ element, const wchar_t* name)
            {
                return wcscmp(xaml_automation::AutomationProperties::GetName(element)->Data(), name) == 0;
            }

            // Waits until predicate() holds; it is re-evaluated on the UI thread whenever 'property' changes on 'target'.
            void WaitForPropertyCondition(xaml::DependencyObject^ target, xaml::DependencyProperty^ property, std::function<bool()> predicate, const wchar_t* description)
            {
                auto satisfied = std::make_shared<Event>();
                long long token = 0;
                bool registered = false;
                RunOnUIThread([&]()
                {
                    if (predicate())
                    {
                        satisfied->Set();
                    }
                    else
                    {
                        token = target->RegisterPropertyChangedCallback(property, ref new xaml::DependencyPropertyChangedCallback(
                            [satisfied, predicate](xaml::DependencyObject^, xaml::DependencyProperty^)
                        {
                            if (predicate())
                            {
                                satisfied->Set();
                            }
                        }));
                        registered = true;
                    }
                });

                auto unregister = wil::scope_exit([&]()
                {
                    if (registered)
                    {
                        try
                        {
                            RunOnUIThread([&]() { target->UnregisterPropertyChangedCallback(property, token); });
                        }
                        catch (...)
                        {
                            WEX::Logging::Log::Warning(L"Failed to unregister a property changed callback");
                        }
                    }
                });

                LOG_OUTPUT(L"Waiting for: %s", description);
                satisfied->WaitFor(c_eventTimeout);
            }

            void WaitForAutomationName(xaml::DependencyObject^ element, const wchar_t* name)
            {
                std::wstring expected(name);
                WaitForPropertyCondition(element, xaml_automation::AutomationProperties::NameProperty,
                    [element, expected]() { return HasName(element, expected.c_str()); },
                    WEX::Common::String().Format(L"automation name '%s'", name));
            }

            void WaitForIsEnabled(xaml_controls::Control^ control, bool isEnabled, const wchar_t* description)
            {
                WaitForPropertyCondition(control, xaml_controls::Control::IsEnabledProperty,
                    [control, isEnabled]() { return control->IsEnabled == isEnabled; }, description);
            }

            void WaitForVisibility(xaml::UIElement^ element, xaml::Visibility visibility, const wchar_t* description)
            {
                WaitForPropertyCondition(element, xaml::UIElement::VisibilityProperty,
                    [element, visibility]() { return element->Visibility == visibility; }, description);
            }

            // Runs the action and waits until the session reports the requested state.
            void DoAndWaitForPlaybackState(wmpb::MediaPlayer^ player, wmpb::MediaPlaybackState state, std::function<void()> action)
            {
                auto reached = std::make_shared<Event>();
                auto registration = CreateSafeEventRegistration(wmpb::MediaPlaybackSession, PlaybackStateChanged);
                RunOnUIThread([&]()
                {
                    registration.Attach(
                        player->PlaybackSession,
                        ref new wf::TypedEventHandler<wmpb::MediaPlaybackSession^, Platform::Object^>([reached, state](wmpb::MediaPlaybackSession^ session, Platform::Object^)
                    {
                        if (session->PlaybackState == state)
                        {
                            reached->Set();
                        }
                    }));
                });
                action();
                RunOnUIThread([&]()
                {
                    if (player->PlaybackSession->PlaybackState == state)
                    {
                        reached->Set();
                    }
                });
                reached->WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
            }

            // Runs the action and waits until the playback rate satisfies the predicate; returns the new rate.
            double DoAndWaitForPlaybackRate(wmpb::MediaPlayer^ player, std::function<bool(double)> predicate, std::function<void()> action)
            {
                auto changed = std::make_shared<Event>();
                auto rate = std::make_shared<std::atomic<double>>(0.0);
                auto registration = CreateSafeEventRegistration(wmpb::MediaPlaybackSession, PlaybackRateChanged);
                RunOnUIThread([&]()
                {
                    registration.Attach(
                        player->PlaybackSession,
                        ref new wf::TypedEventHandler<wmpb::MediaPlaybackSession^, Platform::Object^>([changed, rate, predicate](wmpb::MediaPlaybackSession^ session, Platform::Object^)
                    {
                        const double newRate = session->PlaybackRate;
                        if (predicate(newRate))
                        {
                            rate->store(newRate);
                            changed->Set();
                        }
                    }));
                });
                action();
                changed->WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
                return rate->load();
            }

            void DoAndWaitForSeekCompleted(wmpb::MediaPlayer^ player, std::function<void()> action)
            {
                auto completed = std::make_shared<Event>();
                auto registration = CreateSafeEventRegistration(wmpb::MediaPlaybackSession, SeekCompleted);
                RunOnUIThread([&]()
                {
                    registration.Attach(
                        player->PlaybackSession,
                        ref new wf::TypedEventHandler<wmpb::MediaPlaybackSession^, Platform::Object^>([completed](wmpb::MediaPlaybackSession^, Platform::Object^)
                    {
                        completed->Set();
                    }));
                });
                action();
                completed->WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
            }

            void InvokeButton(TransportControlsScene& scene, Platform::String^ name)
            {
                xaml_primitives::ButtonBase^ button = nullptr;
                RunOnUIThread([&]()
                {
                    button = FindPart<xaml_primitives::ButtonBase>(scene.mpe, name);
                });
                LOG_OUTPUT(L"Invoking %s", name->Data());
                ControlHelper::DoClickUsingAP(button);
                TestServices::WindowHelper->WaitForIdle();
            }

            void PlayAndWaitForTransportControls(TransportControlsScene& scene)
            {
                DoAndWaitForPlaybackState(scene.player, wmpb::MediaPlaybackState::Playing, [&]()
                {
                    RunOnUIThread([&]() { scene.player->Play(); });
                });
                xaml_primitives::ButtonBase^ playPause = nullptr;
                RunOnUIThread([&]() { playPause = FindPart<xaml_primitives::ButtonBase>(scene.mpe, L"PlayPauseButton"); });
                // The transport controls learn about the state change asynchronously; the button name follows it.
                WaitForAutomationName(playPause, L"Pause");
            }

            // Invokes a MenuFlyoutItem through UIA and waits for its Click and for the flyout to close.
            void InvokeMenuFlyoutItem(xaml_controls::MenuFlyout^ flyout, unsigned int index)
            {
                auto clicked = std::make_shared<Event>();
                auto closed = std::make_shared<Event>();
                auto clickRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, Click);
                auto closedRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyout, Closed);
                RunOnUIThread([&]()
                {
                    auto item = safe_cast<xaml_controls::MenuFlyoutItem^>(flyout->Items->GetAt(index));
                    LOG_OUTPUT(L"Invoking menu item '%s'", item->Text->Data());
                    clickRegistration.Attach(item, ref new xaml::RoutedEventHandler([clicked](Platform::Object^, xaml::RoutedEventArgs^) { clicked->Set(); }));
                    closedRegistration.Attach(flyout, ref new wf::EventHandler<Platform::Object^>([closed](Platform::Object^, Platform::Object^) { closed->Set(); }));
                    auto peer = safe_cast<xaml_automation_peers::MenuFlyoutItemAutomationPeer^>(
                        xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(item));
                    peer->Invoke();
                });
                clicked->WaitFor(c_eventTimeout);
                closed->WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
            }

            std::vector<std::wstring> GetMenuItemTexts(xaml_controls::MenuFlyout^ flyout)
            {
                std::vector<std::wstring> texts;
                for (unsigned int i = 0; i < flyout->Items->Size; i++)
                {
                    texts.push_back(safe_cast<xaml_controls::MenuFlyoutItem^>(flyout->Items->GetAt(i))->Text->Data());
                }
                return texts;
            }

            void VerifyMenuItemTexts(const std::vector<std::wstring>& expected, const std::vector<std::wstring>& actual)
            {
                VERIFY_ARE_EQUAL(expected.size(), actual.size(), L"Number of menu items");
                for (size_t i = 0; i < expected.size(); i++)
                {
                    VERIFY_IS_TRUE(expected[i] == actual[i],
                        WEX::Common::String().Format(L"Menu item %zu: expected '%s', actual '%s'", i, expected[i].c_str(), actual[i].c_str()));
                }
            }

            // UIA value of the seek bar: "position [H hour(s)] M minute(s) S second(s)" (en-US resources).
            std::wstring FormatSeekPosition(INT64 totalSeconds)
            {
                auto component = [](INT64 value, const wchar_t* singular, const wchar_t* plural)
                {
                    return std::to_wstring(value) + L" " + (value == 1 ? singular : plural);
                };
                const INT64 hours = totalSeconds / 3600;
                const INT64 minutes = (totalSeconds % 3600) / 60;
                const INT64 seconds = totalSeconds % 60;
                std::wstring result = L"position ";
                if (hours > 0)
                {
                    result += component(hours, L"hour", L"hours") + L" ";
                }
                return result + component(minutes, L"minute", L"minutes") + L" " + component(seconds, L"second", L"seconds");
            }

            // Seeks the paused player and waits until the transport controls moved the seek bar to the new position.
            void SeekAndWaitForSeekBar(TransportControlsScene& scene, xaml_controls::Slider^ slider, INT64 positionHns)
            {
                DoAndWaitForSeekCompleted(scene.player, [&]()
                {
                    RunOnUIThread([&]()
                    {
                        wf::TimeSpan position;
                        position.Duration = positionHns;
                        scene.player->PlaybackSession->Position = position;
                    });
                });

                auto player = scene.player;
                WaitForPropertyCondition(slider, xaml_primitives::RangeBase::ValueProperty, [slider, player]()
                {
                    auto session = player->PlaybackSession;
                    const double range = slider->Maximum - slider->Minimum;
                    const double expected = slider->Minimum + range * static_cast<double>(session->Position.Duration) / static_cast<double>(session->NaturalDuration.Duration);
                    return slider->Value > slider->Minimum && std::abs(slider->Value - expected) <= range * 0.001;
                }, L"the seek bar to reach the playback position");
            }

            void VerifySeekBarValue(TransportControlsScene& scene, uia_provider::IValueProvider^ valueProvider)
            {
                RunOnUIThread([&]()
                {
                    const INT64 seconds = scene.player->PlaybackSession->Position.Duration / c_hnsPerSecond;
                    VerifyText(FormatSeekPosition(seconds).c_str(), valueProvider->Value, L"Seek bar UIA value");
                });
            }

            // In-memory MediaStreamSource with a small uncompressed (NV12) video stream and silent 16-bit stereo PCM for each
            // audio stream; it supports seeking. The video stream keeps the source playable on machines without an audio device.
            struct SyntheticStream
            {
                Microsoft::WRL::ComPtr<IUnknown> identity;
                INT64 nextTimestamp = 0;
                INT64 sampleDuration = 0;
                unsigned int sampleBytes = 0;
            };

            struct SyntheticSourceState
            {
                std::mutex lock;
                std::vector<SyntheticStream> streams;
                INT64 duration = 0;
            };

            const unsigned int c_sampleRate = 48000;
            const unsigned int c_audioSampleBytes = c_sampleRate / 50 * 2 /*channels*/ * 2 /*bytes per sample*/; // 20 ms
            const INT64 c_audioSampleDuration = c_hnsPerSecond / 50;
            const unsigned int c_videoWidth = 64;
            const unsigned int c_videoHeight = 64;
            const unsigned int c_videoSampleBytes = c_videoWidth * c_videoHeight * 3 / 2; // NV12
            const INT64 c_videoSampleDuration = c_hnsPerSecond / 10; // 10 frames per second

            Microsoft::WRL::ComPtr<IUnknown> GetIdentity(Platform::Object^ object)
            {
                Microsoft::WRL::ComPtr<IUnknown> identity;
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(object)->QueryInterface(IID_PPV_ARGS(&identity)));
                return identity;
            }

            wmcore::AudioStreamDescriptor^ CreatePcmDescriptor(Platform::String^ label, Platform::String^ name, Platform::String^ language)
            {
                auto descriptor = ref new wmcore::AudioStreamDescriptor(::Windows::Media::MediaProperties::AudioEncodingProperties::CreatePcm(c_sampleRate, 2, 16));
                if (label != nullptr)
                {
                    descriptor->Label = label;
                }
                if (name != nullptr)
                {
                    descriptor->Name = name;
                }
                if (language != nullptr)
                {
                    descriptor->Language = language;
                }
                return descriptor;
            }

            wmcore::MediaStreamSource^ CreateSyntheticSource(const std::vector<wmcore::AudioStreamDescriptor^>& audioDescriptors, INT64 durationHns)
            {
                auto state = std::make_shared<SyntheticSourceState>();
                state->duration = durationHns;

                auto videoProperties = ::Windows::Media::MediaProperties::VideoEncodingProperties::CreateUncompressed(
                    ::Windows::Media::MediaProperties::MediaEncodingSubtypes::Nv12, c_videoWidth, c_videoHeight);
                videoProperties->FrameRate->Numerator = 10;
                videoProperties->FrameRate->Denominator = 1;
                auto videoDescriptor = ref new wmcore::VideoStreamDescriptor(videoProperties);

                SyntheticStream video;
                video.identity = GetIdentity(videoDescriptor);
                video.sampleDuration = c_videoSampleDuration;
                video.sampleBytes = c_videoSampleBytes;
                state->streams.push_back(video);

                auto source = ref new wmcore::MediaStreamSource(videoDescriptor);
                for (auto descriptor : audioDescriptors)
                {
                    SyntheticStream audio;
                    audio.identity = GetIdentity(descriptor);
                    audio.sampleDuration = c_audioSampleDuration;
                    audio.sampleBytes = c_audioSampleBytes;
                    state->streams.push_back(audio);
                    source->AddStreamDescriptor(descriptor);
                }
                wf::TimeSpan duration;
                duration.Duration = durationHns;
                source->Duration = duration;
                source->CanSeek = true;
                wf::TimeSpan noBuffering;
                noBuffering.Duration = 0;
                source->BufferTime = noBuffering;

                source->Starting += ref new wf::TypedEventHandler<wmcore::MediaStreamSource^, wmcore::MediaStreamSourceStartingEventArgs^>(
                    [state](wmcore::MediaStreamSource^, wmcore::MediaStreamSourceStartingEventArgs^ args)
                {
                    auto request = args->Request;
                    wf::TimeSpan start;
                    start.Duration = (request->StartPosition != nullptr) ? request->StartPosition->Value.Duration : 0;
                    {
                        std::lock_guard<std::mutex> guard(state->lock);
                        for (auto& stream : state->streams)
                        {
                            stream.nextTimestamp = start.Duration;
                        }
                    }
                    request->SetActualStartPosition(start);
                });

                source->SampleRequested += ref new wf::TypedEventHandler<wmcore::MediaStreamSource^, wmcore::MediaStreamSourceSampleRequestedEventArgs^>(
                    [state](wmcore::MediaStreamSource^, wmcore::MediaStreamSourceSampleRequestedEventArgs^ args)
                {
                    auto request = args->Request;
                    Microsoft::WRL::ComPtr<IUnknown> identity;
                    if (FAILED(reinterpret_cast<IInspectable*>(request->StreamDescriptor)->QueryInterface(IID_PPV_ARGS(&identity))))
                    {
                        return; // No sample: end of stream.
                    }

                    wf::TimeSpan timestamp;
                    wf::TimeSpan sampleDuration;
                    unsigned int sampleBytes = 0;
                    {
                        std::lock_guard<std::mutex> guard(state->lock);
                        SyntheticStream* stream = nullptr;
                        for (auto& candidate : state->streams)
                        {
                            if (candidate.identity.Get() == identity.Get())
                            {
                                stream = &candidate;
                            }
                        }
                        if (stream == nullptr || stream->nextTimestamp >= state->duration)
                        {
                            return; // No sample: end of stream.
                        }
                        timestamp.Duration = stream->nextTimestamp;
                        sampleDuration.Duration = stream->sampleDuration;
                        sampleBytes = stream->sampleBytes;
                        stream->nextTimestamp += stream->sampleDuration;
                    }

                    auto writer = ref new ::Windows::Storage::Streams::DataWriter();
                    writer->WriteBytes(ref new Platform::Array<unsigned char>(sampleBytes));
                    auto sample = wmcore::MediaStreamSample::CreateFromBuffer(writer->DetachBuffer(), timestamp);
                    sample->Duration = sampleDuration;
                    request->Sample = sample;
                });

                return source;
            }

            Platform::String^ GetExpectedErrorText(wmpb::MediaPlayerError error)
            {
                switch (error)
                {
                case wmpb::MediaPlayerError::Aborted: return L"Error: Video playback was aborted";
                case wmpb::MediaPlayerError::NetworkError: return L"Network Error: Unable to retrieve video";
                case wmpb::MediaPlayerError::DecodingError: return L"Error: Video could not be decoded";
                case wmpb::MediaPlayerError::SourceNotSupported: return L"Error: Unsupported video type or invalid file path";
                default: return L"Error: An unknown error occurred";
                }
            }

            Platform::String^ GetExpectedErrorText(wmpb::MediaPlaybackItemErrorCode error)
            {
                switch (error)
                {
                case wmpb::MediaPlaybackItemErrorCode::Aborted: return L"Error: Video playback was aborted";
                case wmpb::MediaPlaybackItemErrorCode::NetworkError: return L"Network Error: Unable to retrieve video";
                case wmpb::MediaPlaybackItemErrorCode::DecodeError: return L"Error: Video could not be decoded";
                case wmpb::MediaPlaybackItemErrorCode::SourceNotSupportedError: return L"Error: Unsupported video type or invalid file path";
                default: return L"Error: An unknown error occurred";
                }
            }

            const wchar_t* GetFastForwardName(double rate)
            {
                if (rate == 2.0) return L"Fast forward in 2X";
                if (rate == 4.0) return L"Fast forward in 4X";
                if (rate == 8.0) return L"Fast forward in 8X";
                if (rate == 16.0) return L"Fast forward in 16X";
                return L"Fast forward";
            }

            const wchar_t* GetRewindName(double rate)
            {
                if (rate == -2.0) return L"Rewind in 2X";
                if (rate == -4.0) return L"Rewind in 4X";
                if (rate == -8.0) return L"Rewind in 8X";
                if (rate == -16.0) return L"Rewind in 16X";
                return L"Rewind";
            }
        }

        //------------------------------------------------------------------------
        // Test case: the seek bar automation peer reports the position as a read-only UIA value
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::SeekSliderAutomationPeerExposesReadOnlyPositionValue()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);

            xaml_controls::Slider^ slider = nullptr;
            uia_provider::IValueProvider^ valueProvider = nullptr;

            LOG_OUTPUT(L"No source: there is no duration, so the UIA value follows the player position (0).");
            RunOnUIThread([&]()
            {
                slider = FindPart<xaml_controls::Slider>(scene.mpe, L"ProgressSlider");
                auto peer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(slider);
                VERIFY_IS_NOT_NULL(peer);
                VerifyText(L"SeekSliderAutomationPeer", peer->GetClassName(), L"Seek bar peer class name");

                valueProvider = dynamic_cast<uia_provider::IValueProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::Value));
                VERIFY_IS_NOT_NULL(valueProvider, L"The seek bar exposes the Value pattern");
                VERIFY_IS_NOT_NULL(dynamic_cast<uia_provider::IRangeValueProvider^>(peer->GetPattern(xaml_automation_peers::PatternInterface::RangeValue)),
                    L"The seek bar still exposes the RangeValue pattern of a Slider");
                VERIFY_IS_NULL(peer->GetPattern(xaml_automation_peers::PatternInterface::Invoke), L"No Invoke pattern on the seek bar");

                VerifyText(L"position 0 minutes 0 seconds", valueProvider->Value, L"Seek bar UIA value without media");

                slider->Value = (slider->Minimum + slider->Maximum) / 2;
                VerifyText(L"position 0 minutes 0 seconds", valueProvider->Value, L"Without a duration the UIA value reports the player position, not the seek bar value");
                slider->Value = slider->Minimum;
            });
            // Stop here (instead of dereferencing null later) if the seek bar or its Value pattern is missing.
            VERIFY_IS_NOT_NULL(slider, L"Precondition: the seek bar was found");
            VERIFY_IS_NOT_NULL(valueProvider, L"Precondition: the seek bar exposes the Value pattern");

            LOG_OUTPUT(L"Open blueframe_video.mp4 (7 s) and seek while paused.");
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });
            RunOnUIThread([&]() { scene.mtc->Show(); });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(slider->IsEnabled);
                VERIFY_IS_FALSE(valueProvider->IsReadOnly, L"An enabled seek bar is not read-only");
            });

            SeekAndWaitForSeekBar(scene, slider, 1500 * 10000);
            VerifySeekBarValue(scene, valueProvider);
            SeekAndWaitForSeekBar(scene, slider, 5500 * 10000);
            VerifySeekBarValue(scene, valueProvider);

            // With a known duration the UIA value is derived from the seek bar value, not from the player position:
            // move the seek bar to the middle and read the value in the same UI thread turn, before the resulting seek completes.
            LOG_OUTPUT(L"Move the seek bar to the middle and read the UIA value before the seek completes.");
            DoAndWaitForSeekCompleted(scene.player, [&]()
            {
                RunOnUIThread([&]()
                {
                    auto session = scene.player->PlaybackSession;
                    const INT64 playerSecondsBefore = session->Position.Duration / c_hnsPerSecond;
                    const double range = slider->Maximum - slider->Minimum;
                    slider->Value = slider->Minimum + range / 2;
                    const INT64 sliderSeconds = static_cast<INT64>((slider->Value - slider->Minimum) / range * static_cast<double>(session->NaturalDuration.Duration)) / c_hnsPerSecond;
                    VERIFY_ARE_EQUAL(5, playerSecondsBefore, L"Precondition: the player was at 5.5 s before the move");
                    VERIFY_ARE_EQUAL(3, sliderSeconds, L"Precondition: half of the 7 s media is 3 s");
                    VerifyText(FormatSeekPosition(sliderSeconds).c_str(), valueProvider->Value, L"Seek bar UIA value follows the seek bar value");
                });
            });

            RunOnUIThread([&]()
            {
                const double valueBefore = slider->Value;
                const INT64 positionBefore = scene.player->PlaybackSession->Position.Duration;
                bool rejected = false;
                try
                {
                    valueProvider->SetValue(L"position 0 minutes 2 seconds");
                }
                catch (Platform::Exception^ e)
                {
                    rejected = FAILED(e->HResult);
                }
                VERIFY_IS_TRUE(rejected, L"Setting the seek bar value through UIA is rejected");
                VERIFY_ARE_EQUAL(valueBefore, slider->Value, L"A rejected UIA SetValue does not move the seek bar");
                VERIFY_ARE_EQUAL(positionBefore, scene.player->PlaybackSession->Position.Duration, L"A rejected UIA SetValue does not seek");

                scene.mtc->IsSeekEnabled = false;
                VERIFY_IS_FALSE(slider->IsEnabled, L"IsSeekEnabled=false disables the seek bar");
                VERIFY_IS_TRUE(valueProvider->IsReadOnly, L"A disabled seek bar is read-only");
            });
        }

        //------------------------------------------------------------------------
        // Test case: the seek bar automation value includes hours for long media
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::SeekSliderValueIncludesHoursForLongMedia()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);

            const INT64 twoHoursAndAHalf = 9000 * c_hnsPerSecond;
            SetSource(scene, [twoHoursAndAHalf]()
            {
                // Video only: the seek bar behavior does not depend on audio.
                return ref new wmpb::MediaPlaybackItem(wmcore::MediaSource::CreateFromMediaStreamSource(CreateSyntheticSource({}, twoHoursAndAHalf)));
            });
            RunOnUIThread([&]() { scene.mtc->Show(); });
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::Slider^ slider = nullptr;
            uia_provider::IValueProvider^ valueProvider = nullptr;
            RunOnUIThread([&]()
            {
                slider = FindPart<xaml_controls::Slider>(scene.mpe, L"ProgressSlider");
                valueProvider = dynamic_cast<uia_provider::IValueProvider^>(
                    xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(slider)->GetPattern(xaml_automation_peers::PatternInterface::Value));
                VERIFY_IS_NOT_NULL(valueProvider);
            });
            VERIFY_IS_NOT_NULL(slider, L"Precondition: the seek bar was found");
            VERIFY_IS_NOT_NULL(valueProvider, L"Precondition: the seek bar exposes the Value pattern");

            LOG_OUTPUT(L"1:01:01 -> singular components.");
            SeekAndWaitForSeekBar(scene, slider, 3661 * c_hnsPerSecond + c_hnsPerSecond / 2);
            VerifySeekBarValue(scene, valueProvider);
            RunOnUIThread([&]() { VerifyText(L"position 1 hour 1 minute 1 second", valueProvider->Value, L"Seek bar UIA value at 1:01:01"); });

            LOG_OUTPUT(L"1:05:03 -> 303 seconds into the hour are 5 minutes and 3 seconds.");
            SeekAndWaitForSeekBar(scene, slider, 3903 * c_hnsPerSecond + c_hnsPerSecond / 2);
            RunOnUIThread([&]() { VerifyText(L"position 1 hour 5 minutes 3 seconds", valueProvider->Value, L"Seek bar UIA value at 1:05:03"); });

            LOG_OUTPUT(L"2:02:02 -> plural components.");
            SeekAndWaitForSeekBar(scene, slider, 7322 * c_hnsPerSecond + c_hnsPerSecond / 2);
            VerifySeekBarValue(scene, valueProvider);
            RunOnUIThread([&]() { VerifyText(L"position 2 hours 2 minutes 2 seconds", valueProvider->Value, L"Seek bar UIA value at 2:02:02"); });
        }

        //------------------------------------------------------------------------
        // Test case: the play/pause button plays and pauses the player
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::PlayPauseButtonTogglesPlaybackState()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);
            // The source is set after the transport controls subscribed to the player (MediaPlayer SourceChanged path).
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            xaml_primitives::ButtonBase^ playPause = nullptr;
            RunOnUIThread([&]()
            {
                scene.player->IsLoopingEnabled = true;
                playPause = FindPart<xaml_primitives::ButtonBase>(scene.mpe, L"PlayPauseButton");
                VERIFY_IS_TRUE(wmpb::MediaPlaybackState::Paused == scene.player->PlaybackSession->PlaybackState, L"The opened media is paused");
                VERIFY_IS_TRUE(HasName(playPause, L"Play"), L"A paused player shows the Play action");
            });

            DoAndWaitForPlaybackState(scene.player, wmpb::MediaPlaybackState::Playing, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
            WaitForAutomationName(playPause, L"Pause");

            DoAndWaitForPlaybackState(scene.player, wmpb::MediaPlaybackState::Paused, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
            WaitForAutomationName(playPause, L"Play");
        }

        //------------------------------------------------------------------------
        // Test case: fast forward trick play and reset through play/pause
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::FastForwardTrickModeRatesUpdateButtonNameAndPlayResets()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene, [](xaml_controls::MediaTransportControls^ mtc)
            {
                mtc->IsFastForwardButtonVisible = true;
                mtc->IsFastForwardEnabled = true;
            });
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            xaml_primitives::ButtonBase^ fastForward = nullptr;
            RunOnUIThread([&]()
            {
                scene.player->IsLoopingEnabled = true;
                fastForward = FindPart<xaml_primitives::ButtonBase>(scene.mpe, L"FastForwardButton");
                VERIFY_IS_TRUE(HasName(fastForward, L"Fast forward"));
            });
            VERIFY_IS_NOT_NULL(fastForward, L"Precondition: the fast forward button was found");
            PlayAndWaitForTransportControls(scene);
            WaitForIsEnabled(fastForward, true, L"fast forward enabled while playing");

            const double firstRate = DoAndWaitForPlaybackRate(scene.player, [](double rate) { return rate != 1.0; }, [&]() { InvokeButton(scene, L"FastForwardButton"); });
            VERIFY_ARE_EQUAL(2.0, firstRate, L"The first fast forward step plays at 2x");
            WaitForAutomationName(fastForward, GetFastForwardName(firstRate));

            const double secondRate = DoAndWaitForPlaybackRate(scene.player, [firstRate](double rate) { return rate != firstRate; }, [&]() { InvokeButton(scene, L"FastForwardButton"); });
            VERIFY_IS_TRUE(secondRate > firstRate, WEX::Common::String().Format(L"The second fast forward step is faster (%f)", secondRate));
            WaitForAutomationName(fastForward, GetFastForwardName(secondRate));

            LOG_OUTPUT(L"Play/pause while in trick mode resumes normal playback.");
            const double resetRate = DoAndWaitForPlaybackRate(scene.player, [](double rate) { return rate == 1.0; }, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
            VERIFY_ARE_EQUAL(1.0, resetRate);
            WaitForAutomationName(fastForward, L"Fast forward");
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(wmpb::MediaPlaybackState::Playing == scene.player->PlaybackSession->PlaybackState, L"Leaving trick mode keeps playing");
            });
        }

        //------------------------------------------------------------------------
        // Test case: rewind trick play, or skip back when reverse playback is not supported
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::RewindTrickModeOrSkipBackFallback()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene, [](xaml_controls::MediaTransportControls^ mtc)
            {
                mtc->IsFastRewindButtonVisible = true;
                mtc->IsFastRewindEnabled = true;
            });
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            xaml_primitives::ButtonBase^ rewind = nullptr;
            bool reverseSupported = false;
            RunOnUIThread([&]()
            {
                scene.player->IsLoopingEnabled = true;
                rewind = FindPart<xaml_primitives::ButtonBase>(scene.mpe, L"RewindButton");
                VERIFY_IS_TRUE(HasName(rewind, L"Rewind"));
                reverseSupported = scene.player->PlaybackSession->IsSupportedPlaybackRateRange(-2.0, -2.0);
                LOG_OUTPUT(L"Reverse playback at -2x supported: %d", reverseSupported);
            });
            VERIFY_IS_NOT_NULL(rewind, L"Precondition: the rewind button was found");

            DoAndWaitForSeekCompleted(scene.player, [&]()
            {
                RunOnUIThread([&]()
                {
                    wf::TimeSpan position;
                    position.Duration = 5 * c_hnsPerSecond;
                    scene.player->PlaybackSession->Position = position;
                });
            });
            PlayAndWaitForTransportControls(scene);
            WaitForIsEnabled(rewind, true, L"rewind enabled while playing");

            if (reverseSupported)
            {
                // Entering reverse playback must not also skip back: capture the position when the rate turns negative.
                struct RewindObservation
                {
                    Event reverse;
                    std::atomic<double> rate{ 0.0 };
                    std::atomic<INT64> positionAtRateChange{ -1 };
                };
                auto observation = std::make_shared<RewindObservation>();
                auto rateRegistration = CreateSafeEventRegistration(wmpb::MediaPlaybackSession, PlaybackRateChanged);
                INT64 positionBeforePress = 0;
                RunOnUIThread([&]()
                {
                    positionBeforePress = scene.player->PlaybackSession->Position.Duration;
                    rateRegistration.Attach(
                        scene.player->PlaybackSession,
                        ref new wf::TypedEventHandler<wmpb::MediaPlaybackSession^, Platform::Object^>([observation](wmpb::MediaPlaybackSession^ session, Platform::Object^)
                    {
                        const double newRate = session->PlaybackRate;
                        if (newRate < 0 && observation->positionAtRateChange.load() < 0)
                        {
                            observation->rate.store(newRate);
                            observation->positionAtRateChange.store(session->Position.Duration);
                            observation->reverse.Set();
                        }
                    }));
                });
                InvokeButton(scene, L"RewindButton");
                observation->reverse.WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
                rateRegistration.Detach();

                const double rate = observation->rate.load();
                VERIFY_ARE_EQUAL(-2.0, rate, L"The first rewind step plays at -2x");
                const INT64 positionAtRateChange = observation->positionAtRateChange.load();
                VERIFY_IS_TRUE(positionBeforePress >= 4 * c_hnsPerSecond, WEX::Common::String().Format(L"Precondition: playing from about 5 s (position %lld)", positionBeforePress));
                VERIFY_IS_TRUE(positionAtRateChange >= positionBeforePress - 2 * c_hnsPerSecond,
                    WEX::Common::String().Format(L"Reverse playback does not skip back: position %lld before the press, %lld when the rate turned negative", positionBeforePress, positionAtRateChange));
                WaitForAutomationName(rewind, GetRewindName(rate));

                const double resetRate = DoAndWaitForPlaybackRate(scene.player, [](double rate) { return rate == 1.0; }, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
                VERIFY_ARE_EQUAL(1.0, resetRate);
                WaitForAutomationName(rewind, L"Rewind");
            }
            else
            {
                // Without reverse playback the transport controls fall back to skipping back 10 s (clamped to the start).
                DoAndWaitForSeekCompleted(scene.player, [&]() { InvokeButton(scene, L"RewindButton"); });
                RunOnUIThread([&]()
                {
                    const INT64 position = scene.player->PlaybackSession->Position.Duration;
                    VERIFY_IS_TRUE(position < 2 * c_hnsPerSecond, WEX::Common::String().Format(L"Rewind skipped back to the start (position %lld)", position));
                    VERIFY_ARE_EQUAL(1.0, scene.player->PlaybackSession->PlaybackRate, L"The playback rate is unchanged");
                    VERIFY_IS_TRUE(HasName(rewind, L"Rewind"));
                });
            }
        }

        //------------------------------------------------------------------------
        // Test case: command manager behaviors gate the skip, playback rate and repeat buttons
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::CommandManagerBehaviorsGateSkipRateAndRepeatButtons()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene, [](xaml_controls::MediaTransportControls^ mtc)
            {
                mtc->IsVolumeButtonVisible = false;
                mtc->IsZoomButtonVisible = false;
                mtc->IsSkipForwardButtonVisible = true;
                mtc->IsSkipForwardEnabled = true;
                mtc->IsSkipBackwardButtonVisible = true;
                mtc->IsSkipBackwardEnabled = true;
                mtc->IsPlaybackRateButtonVisible = true;
                mtc->IsPlaybackRateEnabled = true;
                mtc->IsRepeatButtonVisible = true;
                mtc->IsRepeatEnabled = true;
            });
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            xaml_controls::Control^ skipForward = nullptr;
            xaml_controls::Control^ skipBackward = nullptr;
            xaml_controls::Control^ playbackRate = nullptr;
            xaml_controls::Control^ repeat = nullptr;
            RunOnUIThread([&]()
            {
                skipForward = FindPart<xaml_controls::Control>(scene.mpe, L"SkipForwardButton");
                skipBackward = FindPart<xaml_controls::Control>(scene.mpe, L"SkipBackwardButton");
                playbackRate = FindPart<xaml_controls::Control>(scene.mpe, L"PlaybackRateButton");
                repeat = FindPart<xaml_controls::Control>(scene.mpe, L"RepeatButton");

                VERIFY_IS_TRUE(skipForward->IsEnabled);
                VERIFY_IS_TRUE(skipBackward->IsEnabled);
                VERIFY_IS_TRUE(playbackRate->IsEnabled);
                VERIFY_IS_TRUE(repeat->IsEnabled);
                VERIFY_IS_TRUE(HasName(repeat, L"Repeat None"));

                LOG_OUTPUT(L"Command manager disables position, rate and auto-repeat commands.");
                auto commandManager = scene.player->CommandManager;
                commandManager->PositionBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Never;
                commandManager->RateBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Never;
                commandManager->AutoRepeatModeBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Never;
            });
            WaitForIsEnabled(skipForward, false, L"skip forward disabled by the position behavior");
            WaitForIsEnabled(skipBackward, false, L"skip backward disabled by the position behavior");
            WaitForIsEnabled(playbackRate, false, L"playback rate disabled by the rate behavior");
            WaitForIsEnabled(repeat, false, L"repeat disabled by the auto-repeat behavior");

            LOG_OUTPUT(L"Command manager enables them again.");
            RunOnUIThread([&]()
            {
                auto commandManager = scene.player->CommandManager;
                commandManager->PositionBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Always;
                commandManager->RateBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Always;
                commandManager->AutoRepeatModeBehavior->EnablingRule = wmpb::MediaCommandEnablingRule::Always;
            });
            WaitForIsEnabled(skipForward, true, L"skip forward enabled by the position behavior");
            WaitForIsEnabled(skipBackward, true, L"skip backward enabled by the position behavior");
            WaitForIsEnabled(playbackRate, true, L"playback rate enabled by the rate behavior");
            WaitForIsEnabled(repeat, true, L"repeat enabled by the auto-repeat behavior");

            LOG_OUTPUT(L"The transport controls API still disables an allowed command.");
            RunOnUIThread([&]() { scene.mtc->IsSkipForwardEnabled = false; });
            WaitForIsEnabled(skipForward, false, L"skip forward disabled by IsSkipForwardEnabled");
            RunOnUIThread([&]() { VERIFY_IS_TRUE(skipBackward->IsEnabled, L"Skip backward is unaffected"); });

            LOG_OUTPUT(L"Looping the player is reflected as Repeat One.");
            RunOnUIThread([&]() { scene.player->IsLoopingEnabled = true; });
            WaitForAutomationName(repeat, L"Repeat One");
            RunOnUIThread([&]() { scene.player->IsLoopingEnabled = false; });
            WaitForAutomationName(repeat, L"Repeat None");
        }

        //------------------------------------------------------------------------
        // Test case: a media failure shows the localized error
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::MediaFailedShowsLocalizedError()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);

            auto failed = std::make_shared<Event>();
            auto error = std::make_shared<std::atomic<int>>(-1);
            auto failedRegistration = CreateSafeEventRegistration(wmpb::MediaPlayer, MediaFailed);
            RunOnUIThread([&]()
            {
                failedRegistration.Attach(
                    scene.player,
                    ref new wf::TypedEventHandler<wmpb::MediaPlayer^, wmpb::MediaPlayerFailedEventArgs^>([failed, error](wmpb::MediaPlayer^, wmpb::MediaPlayerFailedEventArgs^ args)
                {
                    error->store(static_cast<int>(args->Error));
                    failed->Set();
                }));
            });

            LOG_OUTPUT(L"A caption file is not a playable media file.");
            SetSource(scene, []() { return CreateMediaItem(L"cc_bad.ttm"); }, false /*waitForOpened*/);
            failed->WaitFor(c_eventTimeout);

            auto expectedText = GetExpectedErrorText(static_cast<wmpb::MediaPlayerError>(error->load()));
            LOG_OUTPUT(L"MediaPlayerError %d -> '%s'", error->load(), expectedText->Data());

            xaml_controls::TextBlock^ errorText = nullptr;
            xaml::UIElement^ errorBorder = nullptr;
            RunOnUIThread([&]()
            {
                errorText = FindPart<xaml_controls::TextBlock>(scene.mpe, L"ErrorTextBlock");
                errorBorder = FindPart<xaml::FrameworkElement>(scene.mpe, L"ErrorBorder");
            });
            WaitForPropertyCondition(errorText, xaml_controls::TextBlock::TextProperty, [errorText]() { return errorText->Text->Length() > 0; }, L"error text");
            WaitForVisibility(errorBorder, xaml::Visibility::Visible, L"error border shown in the Error state");
            RunOnUIThread([&]()
            {
                VerifyText(expectedText->Data(), errorText->Text, L"Error text for the reported MediaPlayerError");
                VerifyText(expectedText->Data(), xaml_automation::AutomationProperties::GetName(errorText), L"Error text automation name");
            });
        }

        //------------------------------------------------------------------------
        // Test case: next/previous track through a MediaPlaybackList and an item failure
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::PlaybackListNextPreviousAndItemFailure()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene, [](xaml_controls::MediaTransportControls^ mtc)
            {
                mtc->IsNextTrackButtonVisible = true;
                mtc->IsPreviousTrackButtonVisible = true;
            });

            xaml_controls::TextBlock^ errorText = nullptr;
            xaml::UIElement^ errorBorder = nullptr;
            RunOnUIThread([&]()
            {
                errorText = FindPart<xaml_controls::TextBlock>(scene.mpe, L"ErrorTextBlock");
                errorBorder = FindPart<xaml::FrameworkElement>(scene.mpe, L"ErrorBorder");
                VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, errorBorder->Visibility, L"No error before a source is set");
            });
            VERIFY_IS_NOT_NULL(errorText, L"Precondition: the error text part was found");
            VERIFY_IS_NOT_NULL(errorBorder, L"Precondition: the error border part was found");

            // A playlist whose only item cannot be opened: the failure is the current item's failure.
            // The list is set empty first so the transport controls subscribe to it before the item starts opening, and the
            // error border is observed from before the failure so a later item change cannot hide the error state from the test.
            LOG_OUTPUT(L"A playlist with a single broken item reports the item failure.");
            auto brokenList = ref new wmpb::MediaPlaybackList();
            auto itemFailed = std::make_shared<Event>();
            auto itemError = std::make_shared<std::atomic<int>>(-1);
            auto failedRegistration = CreateSafeEventRegistration(wmpb::MediaPlaybackList, ItemFailed);
            struct ErrorShown
            {
                Event shown;
                Platform::String^ text = nullptr;
            };
            auto errorShown = std::make_shared<ErrorShown>();
            long long visibilityToken = 0;
            RunOnUIThread([&]()
            {
                failedRegistration.Attach(
                    brokenList,
                    ref new wf::TypedEventHandler<wmpb::MediaPlaybackList^, wmpb::MediaPlaybackItemFailedEventArgs^>([itemFailed, itemError](wmpb::MediaPlaybackList^, wmpb::MediaPlaybackItemFailedEventArgs^ args)
                {
                    itemError->store(static_cast<int>(args->Error->ErrorCode));
                    itemFailed->Set();
                }));
                auto observedText = errorText;
                visibilityToken = errorBorder->RegisterPropertyChangedCallback(xaml::UIElement::VisibilityProperty, ref new xaml::DependencyPropertyChangedCallback(
                    [errorShown, observedText](xaml::DependencyObject^ sender, xaml::DependencyProperty^)
                {
                    if (safe_cast<xaml::UIElement^>(sender)->Visibility == xaml::Visibility::Visible && errorShown->text == nullptr)
                    {
                        errorShown->text = observedText->Text;
                        errorShown->shown.Set();
                    }
                }));
            });
            auto unregisterVisibility = wil::scope_exit([&]()
            {
                try
                {
                    RunOnUIThread([&]() { errorBorder->UnregisterPropertyChangedCallback(xaml::UIElement::VisibilityProperty, visibilityToken); });
                }
                catch (...)
                {
                    WEX::Logging::Log::Warning(L"Failed to unregister the error border visibility callback");
                }
            });

            SetSource(scene, [brokenList]() { return brokenList; }, false /*waitForOpened*/);
            RunOnUIThread([&]()
            {
                brokenList->Items->Append(CreateMediaItem(L"cc_bad.ttm"));
                if (brokenList->CurrentItem == nullptr)
                {
                    brokenList->MoveTo(0);
                }
            });
            itemFailed->WaitFor(c_eventTimeout);
            auto expectedText = GetExpectedErrorText(static_cast<wmpb::MediaPlaybackItemErrorCode>(itemError->load()));
            LOG_OUTPUT(L"MediaPlaybackItemErrorCode %d -> '%s'", itemError->load(), expectedText->Data());
            LOG_OUTPUT(L"Waiting for: error border shown for the failed item");
            errorShown->shown.WaitFor(c_eventTimeout);
            RunOnUIThread([&]()
            {
                VerifyText(expectedText->Data(), errorShown->text, L"Error text shown with the error border for the failed playback item");
                errorShown->text = nullptr;
            });
            unregisterVisibility.reset();

            // A playlist of two playable items: opening it clears the error, next/previous move through the items.
            LOG_OUTPUT(L"Next and previous track move through a playlist of two playable items.");
            auto firstItem = CreateMediaItem(L"blueframe_video.mp4");
            auto secondItem = CreateMediaItem(L"blueframe_video.mp4");
            auto list = ref new wmpb::MediaPlaybackList();
            list->Items->Append(firstItem);
            list->Items->Append(secondItem);
            SetSource(scene, [list]() { return list; });
            WaitForVisibility(errorBorder, xaml::Visibility::Collapsed, L"error cleared when playable media opens");
            RunOnUIThread([&]() { VERIFY_ARE_EQUAL(0u, list->CurrentItemIndex); });

            auto moveTo = [&](Platform::String^ buttonName, wmpb::MediaPlaybackItem^ expectedItem)
            {
                auto reached = std::make_shared<Event>();
                auto expectedIdentity = GetIdentity(expectedItem);
                auto currentItemRegistration = CreateSafeEventRegistration(wmpb::MediaPlaybackList, CurrentItemChanged);
                RunOnUIThread([&]()
                {
                    currentItemRegistration.Attach(
                        list,
                        ref new wf::TypedEventHandler<wmpb::MediaPlaybackList^, wmpb::CurrentMediaPlaybackItemChangedEventArgs^>([reached, expectedIdentity](wmpb::MediaPlaybackList^, wmpb::CurrentMediaPlaybackItemChangedEventArgs^ args)
                    {
                        Microsoft::WRL::ComPtr<IUnknown> identity;
                        if (args->NewItem != nullptr &&
                            SUCCEEDED(reinterpret_cast<IInspectable*>(args->NewItem)->QueryInterface(IID_PPV_ARGS(&identity))) &&
                            identity.Get() == expectedIdentity.Get())
                        {
                            reached->Set();
                        }
                    }));
                });
                InvokeButton(scene, buttonName);
                reached->WaitFor(c_eventTimeout);
                TestServices::WindowHelper->WaitForIdle();
            };

            moveTo(L"NextTrackButton", secondItem);
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(1u, list->CurrentItemIndex, L"Next track advanced the playback list");
                VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, errorBorder->Visibility, L"No error for a playable item");
            });

            moveTo(L"PreviousTrackButton", firstItem);
            RunOnUIThread([&]() { VERIFY_ARE_EQUAL(0u, list->CurrentItemIndex, L"Previous track went back to the first item"); });

            LOG_OUTPUT(L"Replacing the playlist with a single item keeps the transport controls working.");
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });
            DoAndWaitForPlaybackState(scene.player, wmpb::MediaPlaybackState::Playing, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
            RunOnUIThread([&]() { VERIFY_ARE_EQUAL(xaml::Visibility::Collapsed, errorBorder->Visibility); });
        }

        //------------------------------------------------------------------------
        // Test case: the closed caption menu
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::CCSelectionFlyoutListsTracksAndSwitchesPresentationMode()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene, [](xaml_controls::MediaTransportControls^ mtc)
            {
                mtc->IsVolumeButtonVisible = false;
                mtc->IsZoomButtonVisible = false;
            });

            // Three caption tracks: labeled, language only, and neither; the first one is shown.
            auto labeled = ref new wmcore::TimedMetadataTrack(L"cc-labeled", L"en", wmcore::TimedMetadataKind::Caption);
            labeled->Label = L"English CC";
            auto languageOnly = ref new wmcore::TimedMetadataTrack(L"cc-fr", L"fr", wmcore::TimedMetadataKind::Caption);
            auto untitled = ref new wmcore::TimedMetadataTrack(L"cc-untitled", L"", wmcore::TimedMetadataKind::Subtitle);
            auto mediaSource = wmcore::MediaSource::CreateFromUri(ref new wf::Uri(MediaHelperMPE::GetDefaultResourcePath() + L"blueframe_video.mp4"));
            mediaSource->ExternalTimedMetadataTracks->Append(labeled);
            mediaSource->ExternalTimedMetadataTracks->Append(languageOnly);
            mediaSource->ExternalTimedMetadataTracks->Append(untitled);
            auto item = ref new wmpb::MediaPlaybackItem(mediaSource);

            // The external tracks are reported by the playback item either right away or through TimedMetadataTracksChanged.
            auto tracksAdded = std::make_shared<Event>();
            auto tracksRegistration = CreateSafeEventRegistration(wmpb::MediaPlaybackItem, TimedMetadataTracksChanged);
            RunOnUIThread([&]()
            {
                tracksRegistration.Attach(
                    item,
                    ref new wf::TypedEventHandler<wmpb::MediaPlaybackItem^, wfc::IVectorChangedEventArgs^>([tracksAdded](wmpb::MediaPlaybackItem^ sender, wfc::IVectorChangedEventArgs^)
                {
                    if (sender->TimedMetadataTracks->Size == 3)
                    {
                        tracksAdded->Set();
                    }
                }));
            });

            SetSource(scene, [item]() { return item; });
            RunOnUIThread([&]()
            {
                if (item->TimedMetadataTracks->Size == 3)
                {
                    tracksAdded->Set();
                }
            });
            LOG_OUTPUT(L"Waiting for the playback item to report the three caption tracks.");
            tracksAdded->WaitFor(c_eventTimeout);
            RunOnUIThread([&]() { item->TimedMetadataTracks->SetPresentationMode(0, wmpb::TimedMetadataTrackPresentationMode::PlatformPresented); });
            TestServices::WindowHelper->WaitForIdle();

            xaml_controls::Button^ ccButton = nullptr;
            RunOnUIThread([&]() { ccButton = FindPart<xaml_controls::Button>(scene.mpe, L"CCSelectionButton"); });
            VERIFY_IS_NOT_NULL(ccButton, L"Precondition: the closed caption button was found");
            WaitForVisibility(ccButton, xaml::Visibility::Visible, L"closed caption button shown for media with caption tracks");

            auto openMenu = [&]() -> xaml_controls::MenuFlyout^
            {
                xaml_controls::MenuFlyout^ flyout = nullptr;
                InvokeButton(scene, L"CCSelectionButton");
                RunOnUIThread([&]()
                {
                    flyout = dynamic_cast<xaml_controls::MenuFlyout^>(ccButton->Flyout);
                    VERIFY_IS_NOT_NULL(flyout, L"The closed caption button opens a MenuFlyout");
                });
                return flyout;
            };

            LOG_OUTPUT(L"Menu with the first track shown.");
            auto flyout = openMenu();
            RunOnUIThread([&]() { VerifyMenuItemTexts({ L"Off", L"English CC (On)", L"fr", L"untitled" }, GetMenuItemTexts(flyout)); });

            LOG_OUTPUT(L"Select the language-only track.");
            InvokeMenuFlyoutItem(flyout, 2);
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(wmpb::TimedMetadataTrackPresentationMode::Hidden == item->TimedMetadataTracks->GetPresentationMode(0), L"The previous track is hidden");
                VERIFY_IS_TRUE(wmpb::TimedMetadataTrackPresentationMode::PlatformPresented == item->TimedMetadataTracks->GetPresentationMode(1), L"The selected track is presented");
            });

            flyout = openMenu();
            RunOnUIThread([&]() { VerifyMenuItemTexts({ L"Off", L"English CC", L"fr (On)", L"untitled" }, GetMenuItemTexts(flyout)); });

            LOG_OUTPUT(L"Turn captions off.");
            InvokeMenuFlyoutItem(flyout, 0);
            RunOnUIThread([&]()
            {
                for (unsigned int i = 0; i < 3; i++)
                {
                    VERIFY_IS_TRUE(wmpb::TimedMetadataTrackPresentationMode::PlatformPresented != item->TimedMetadataTracks->GetPresentationMode(i), L"No track is presented");
                }
            });

            LOG_OUTPUT(L"Without a shown track the menu has no Off entry.");
            flyout = openMenu();
            RunOnUIThread([&]()
            {
                VerifyMenuItemTexts({ L"English CC", L"fr", L"untitled" }, GetMenuItemTexts(flyout));
                flyout->Hide();
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: the audio track menu
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::AudioTrackFlyoutNamesTracksAndSwitchesSelection()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);

            wmpb::MediaPlaybackItem^ item = nullptr;
            SetSource(scene, [&item]()
            {
                std::vector<wmcore::AudioStreamDescriptor^> descriptors =
                {
                    CreatePcmDescriptor(L"Commentary", L"Director", L"en-US"),
                    CreatePcmDescriptor(nullptr, L"Descriptive", L"de-DE"),
                    CreatePcmDescriptor(nullptr, nullptr, L"fr-FR"),
                    CreatePcmDescriptor(L"Commentary", nullptr, nullptr),
                };
                item = ref new wmpb::MediaPlaybackItem(wmcore::MediaSource::CreateFromMediaStreamSource(CreateSyntheticSource(descriptors, 60 * c_hnsPerSecond)));
                return item;
            });

            xaml_controls::Button^ audioButton = nullptr;
            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(4u, item->AudioTracks->Size, L"Precondition: one audio track per stream descriptor");
                audioButton = FindPart<xaml_controls::Button>(scene.mpe, L"AudioTracksSelectionButton");
            });
            VERIFY_IS_NOT_NULL(item, L"Precondition: the playback item was created");
            VERIFY_IS_NOT_NULL(audioButton, L"Precondition: the audio track button was found");
            WaitForVisibility(audioButton, xaml::Visibility::Visible, L"audio track button shown for media with several audio tracks");

            auto openMenu = [&]() -> xaml_controls::MenuFlyout^
            {
                xaml_controls::MenuFlyout^ flyout = nullptr;
                InvokeButton(scene, L"AudioTracksSelectionButton");
                RunOnUIThread([&]()
                {
                    flyout = dynamic_cast<xaml_controls::MenuFlyout^>(audioButton->Flyout);
                    VERIFY_IS_NOT_NULL(flyout, L"The audio track button opens a MenuFlyout");
                });
                return flyout;
            };

            // A track is named by its label, else its name, else a generated tag that starts with the track index;
            // the selected track is marked with " (On)".
            auto verifyMenu = [&](xaml_controls::MenuFlyout^ flyout)
            {
                RunOnUIThread([&]()
                {
                    auto texts = GetMenuItemTexts(flyout);
                    VERIFY_ARE_EQUAL(static_cast<size_t>(item->AudioTracks->Size), texts.size(), L"One menu item per audio track");
                    const int selected = item->AudioTracks->SelectedIndex;
                    for (unsigned int i = 0; i < texts.size(); i++)
                    {
                        auto track = item->AudioTracks->GetAt(i);
                        std::wstring text = texts[i];
                        const std::wstring onSuffix = L" (On)";
                        const bool markedOn = text.size() > onSuffix.size() && text.compare(text.size() - onSuffix.size(), onSuffix.size(), onSuffix) == 0;
                        VERIFY_ARE_EQUAL(static_cast<int>(i) == selected, markedOn, WEX::Common::String().Format(L"Only the selected track is marked: '%s'", text.c_str()));
                        if (markedOn)
                        {
                            text.resize(text.size() - onSuffix.size());
                        }

                        if (track->Label != nullptr && track->Label->Length() > 0)
                        {
                            VERIFY_IS_TRUE(text == track->Label->Data(), WEX::Common::String().Format(L"Track %u is named by its label: '%s'", i, text.c_str()));
                        }
                        else if (track->Name != nullptr && track->Name->Length() > 0)
                        {
                            VERIFY_IS_TRUE(text == track->Name->Data(), WEX::Common::String().Format(L"Track %u is named by its name: '%s'", i, text.c_str()));
                        }
                        else
                        {
                            const std::wstring index = std::to_wstring(i);
                            VERIFY_IS_TRUE(text.compare(0, index.size(), index) == 0, WEX::Common::String().Format(L"Unnamed track %u gets a tag that starts with its index: '%s'", i, text.c_str()));
                        }
                    }
                });
            };

            auto flyout = openMenu();
            verifyMenu(flyout);

            LOG_OUTPUT(L"Select the second audio track.");
            InvokeMenuFlyoutItem(flyout, 1);
            RunOnUIThread([&]() { VERIFY_ARE_EQUAL(1, item->AudioTracks->SelectedIndex, L"The chosen audio track is selected"); });

            flyout = openMenu();
            verifyMenu(flyout);
            RunOnUIThread([&]() { flyout->Hide(); });
            TestServices::WindowHelper->WaitForIdle();
        }

        //------------------------------------------------------------------------
        // Test case: ThumbnailRequested while seeking
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::ThumbnailRequestedRaisedWhenSeekBarMoves()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            auto file = concurrency::create_task(::Windows::Storage::StorageFile::GetFileFromPathAsync(MediaHelperMPE::GetDefaultResourcePath() + L"barcelona.jpg")).get();
            ::Windows::Storage::Streams::IInputStream^ thumbnailStream = concurrency::create_task(file->OpenReadAsync()).get();

            struct ThumbnailRequests
            {
                std::atomic<int> count{ 0 };
                Platform::Object^ sender = nullptr;
            };
            auto requests = std::make_shared<ThumbnailRequests>();
            auto thumbnailRegistration = CreateSafeEventRegistration(xaml_controls::MediaTransportControls, ThumbnailRequested);

            xaml_controls::Slider^ slider = nullptr;
            RunOnUIThread([&]()
            {
                thumbnailRegistration.Attach(
                    scene.mtc,
                    ref new wf::TypedEventHandler<xaml_controls::MediaTransportControls^, xaml_media::MediaTransportControlsThumbnailRequestedEventArgs^>(
                        [requests, thumbnailStream](xaml_controls::MediaTransportControls^ sender, xaml_media::MediaTransportControlsThumbnailRequestedEventArgs^ args)
                {
                    requests->sender = sender;
                    ++requests->count;
                    args->SetThumbnailImage(thumbnailStream);
                }));
                slider = FindPart<xaml_controls::Slider>(scene.mpe, L"ProgressSlider");
                VERIFY_ARE_EQUAL(0, requests->count.load(), L"No ThumbnailRequested before the seek bar moves");
            });

            // Moving the seek bar seeks the player and raises ThumbnailRequested synchronously from the value change.
            auto moveSeekBar = [&](double fraction, int expectedCount)
            {
                DoAndWaitForSeekCompleted(scene.player, [&]()
                {
                    RunOnUIThread([&]()
                    {
                        slider->Value = slider->Minimum + (slider->Maximum - slider->Minimum) * fraction;
                        VERIFY_ARE_EQUAL(expectedCount, requests->count.load(), L"Each seek bar move raises ThumbnailRequested once");
                        VERIFY_IS_TRUE(IsSameObject(scene.mtc, requests->sender), L"ThumbnailRequested is raised by the transport controls");
                    });
                });
                RunOnUIThread([&]()
                {
                    auto session = scene.player->PlaybackSession;
                    const double expected = static_cast<double>(session->NaturalDuration.Duration) * fraction;
                    const double actual = static_cast<double>(session->Position.Duration);
                    VERIFY_IS_TRUE(std::abs(expected - actual) <= c_hnsPerSecond / 2,
                        WEX::Common::String().Format(L"The seek bar move seeks the player: expected %.0f, actual %.0f (100 ns units)", expected, actual));
                });
            };

            LOG_OUTPUT(L"Move the seek bar to the middle, then to three quarters.");
            moveSeekBar(0.5, 1);
            moveSeekBar(0.75, 2);

            RunOnUIThread([&]() { requests->sender = nullptr; });
        }

        //------------------------------------------------------------------------
        // Test case: closing the MediaPlayer before unloading the element
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::ClosedMediaPlayerDoesNotBreakTransportControlsTeardown()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            auto unloaded = std::make_shared<Event>();
            auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::MediaPlayerElement, Unloaded);
            RunOnUIThread([&]()
            {
                unloadedRegistration.Attach(scene.mpe, ref new xaml::RoutedEventHandler([unloaded](Platform::Object^, xaml::RoutedEventArgs^) { unloaded->Set(); }));

                LOG_OUTPUT(L"The app closes its MediaPlayer, then removes the element.");
                delete scene.player; // IClosable::Close
                scene.root->Children->Clear();
            });
            unloaded->WaitFor(c_eventTimeout);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"The element comes back with a new player and the transport controls drive it.");
            RunOnUIThread([&]()
            {
                scene.player = ref new wmpb::MediaPlayer();
                scene.player->AutoPlay = false;
                scene.mpe->SetMediaPlayer(scene.player);
                scene.root->Children->Append(scene.mpe);
            });
            TestServices::WindowHelper->WaitForIdle();
            SetSource(scene, []() { return CreateMediaItem(L"blueframe_video.mp4"); });
            DoAndWaitForPlaybackState(scene.player, wmpb::MediaPlaybackState::Playing, [&]() { InvokeButton(scene, L"PlayPauseButton"); });
        }

        //------------------------------------------------------------------------
        // Test case: transport controls moved to another element
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::TransportControlsMovedToAnotherElementFollowNewPlayer()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene first;
            CreateTransportControlsScene(first);
            SetSource(first, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            TransportControlsScene second;
            RunOnUIThread([&]()
            {
                second.mpe = ref new xaml_controls::MediaPlayerElement();
                second.player = ref new wmpb::MediaPlayer();
                second.player->AutoPlay = false;
                second.mpe->SetMediaPlayer(second.player);
                first.root->Children->Append(second.mpe);
            });
            TestServices::WindowHelper->WaitForIdle();
            SetSource(second, []() { return CreateMediaItem(L"blueframe_video.mp4"); });

            LOG_OUTPUT(L"Move the (template-applied) transport controls from the first element to the second.");
            RunOnUIThread([&]()
            {
                auto movedControls = first.mtc;
                first.mpe->TransportControls = ref new xaml_controls::MediaTransportControls();
                first.mtc = first.mpe->TransportControls;
                second.mpe->AreTransportControlsEnabled = true;
                second.mpe->TransportControls = movedControls;
                second.mtc = movedControls;
            });
            TestServices::WindowHelper->WaitForIdle();

            xaml_primitives::ButtonBase^ playPause = nullptr;
            RunOnUIThread([&]()
            {
                playPause = FindPart<xaml_primitives::ButtonBase>(second.mpe, L"PlayPauseButton");
                VERIFY_IS_TRUE(TreeHelper::IsAncestorOf(second.mtc, playPause), L"The play/pause button belongs to the moved transport controls");
            });
            DoAndWaitForPlaybackState(second.player, wmpb::MediaPlaybackState::Playing, [&]() { ControlHelper::DoClickUsingAP(playPause); });
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(wmpb::MediaPlaybackState::Paused == first.player->PlaybackSession->PlaybackState, L"The first element's player is not controlled anymore");
            });
            WaitForAutomationName(playPause, L"Pause");
        }

        //------------------------------------------------------------------------
        // Test case: the seek bar is disabled while a media break plays
        //------------------------------------------------------------------------
        void MediaPlayerElementMTCCustomizationTests::MediaBreakDisablesSeekBarWhileBreakPlays()
        {
            TestCleanupWrapper cleanup;
            TransportControlsScene scene;
            CreateTransportControlsScene(scene);

            auto breakStarted = std::make_shared<Event>();
            auto breakEnded = std::make_shared<Event>();
            auto startedRegistration = CreateSafeEventRegistration(wmpb::MediaBreakManager, BreakStarted);
            auto endedRegistration = CreateSafeEventRegistration(wmpb::MediaBreakManager, BreakEnded);
            RunOnUIThread([&]()
            {
                auto breakManager = scene.player->BreakManager;
                startedRegistration.Attach(
                    breakManager,
                    ref new wf::TypedEventHandler<wmpb::MediaBreakManager^, wmpb::MediaBreakStartedEventArgs^>([breakStarted](wmpb::MediaBreakManager^, wmpb::MediaBreakStartedEventArgs^) { breakStarted->Set(); }));
                endedRegistration.Attach(
                    breakManager,
                    ref new wf::TypedEventHandler<wmpb::MediaBreakManager^, wmpb::MediaBreakEndedEventArgs^>([breakEnded](wmpb::MediaBreakManager^, wmpb::MediaBreakEndedEventArgs^) { breakEnded->Set(); }));
            });

            SetSource(scene, []()
            {
                auto content = CreateMediaItem(L"blueframe_video.mp4");
                auto preroll = ref new wmpb::MediaBreak(wmpb::MediaBreakInsertionMethod::Interrupt);
                preroll->PlaybackList->Items->Append(CreateMediaItem(L"blueframe_video.mp4"));
                content->BreakSchedule->PrerollBreak = preroll;
                return content;
            }, false /*waitForOpened*/);

            xaml_controls::Control^ slider = nullptr;
            RunOnUIThread([&]()
            {
                slider = FindPart<xaml_controls::Control>(scene.mpe, L"ProgressSlider");
                scene.player->Play();
            });

            breakStarted->WaitFor(c_eventTimeout);
            WaitForIsEnabled(slider, false, L"seek bar disabled while the break plays");

            breakEnded->WaitFor(std::chrono::milliseconds(30000));
            WaitForIsEnabled(slider, true, L"seek bar enabled again after the break");
        }
    } } }
} } } }
