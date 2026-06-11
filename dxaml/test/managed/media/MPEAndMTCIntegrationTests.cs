// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;
using Windows.Graphics.Imaging;
using Windows.UI.Composition;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Windows.Media.Playback;
using Windows.Media.Core;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.Media
{
    [TestClass]
    public class MPEAndMTCIntegrationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        private void ClickControl(Button button, string controlName)
        {
            using(var clickedEvent = new EventTester<Button, RoutedEventArgs>(button, "Click"))
            {
                Log.Comment("Click the " + controlName + "button");
                TestServices.InputHelper.Tap(button);
                TestServices.WindowHelper.WaitForIdle();
                clickedEvent.Wait();
                Verify.AreEqual(1, clickedEvent.ExecuteCount);
                var sender = clickedEvent.LastSender;
                Verify.IsNotNull(sender);
                Verify.AreEqual(button, sender, "Click event fired on "+ controlName + " button");
            }
        }

        private void SetupMediaPlayerElementUI(out MediaPlayerElement mpe, out MediaPlayer mp)
        {
            MediaPlayerElement testMpe = null;
            MediaPlayer testMp = null;
            bool isOpen = false;
            bool isPause = false;
            Log.Comment("Create Media Player Element.");
            UIExecutor.Execute(() =>
            {
                Grid grid = (Grid)XamlMarkup.XamlReader.Load(@"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                    <MediaPlayerElement x:Name='theMedia' AreTransportControlsEnabled='true' Source='ms-appx:///resources/native/external/foundation/graphics/Media/blueframe_video.mp4' />
                    </Grid>");
                Verify.IsNotNull(grid);
                TestServices.WindowHelper.WindowContent = grid;
                testMpe = grid.FindName("theMedia") as MediaPlayerElement;
                Verify.IsNotNull(testMpe);
                testMp = testMpe.MediaPlayer;
                Verify.IsNotNull(testMp);
                if (testMp.PlaybackSession.PlaybackState == MediaPlaybackState.Paused)
                {
                    isPause = true;
                }
                else
                if (testMp.PlaybackSession.PlaybackState == MediaPlaybackState.Opening)
                {
                    Log.Comment("Playback State  " + testMp.PlaybackSession.PlaybackState.ToString());
                    isOpen = true;
                }
            });
            TestServices.WindowHelper.WaitForIdle();
            if (!isPause)
            {
                if (!isOpen)
                {
                    using (var playbackStateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMp.PlaybackSession, "PlaybackStateChanged"))
                    {
                        Log.Comment("Playback State Change to " + testMp.PlaybackSession.PlaybackState.ToString());
                        Log.Comment("Wait for Opening State");
                        playbackStateChangedEvent.Wait(TimeSpan.FromSeconds(5));
                        Log.Comment("Playback State Change to " + testMp.PlaybackSession.PlaybackState.ToString());
                    }
                }
                if (testMp.PlaybackSession.PlaybackState == MediaPlaybackState.Opening)
                {
                    using (var playbackStateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMp.PlaybackSession, "PlaybackStateChanged"))
                    {
                        Log.Comment("Wait for Paused State");
                        playbackStateChangedEvent.Wait(TimeSpan.FromSeconds(5));
                        Log.Comment("Playback State Change to " + testMp.PlaybackSession.PlaybackState.ToString());
                    }
                }
            }
            mpe = testMpe;
            mp = testMp;
        }

        private void ShowMTCOnTap(ref MediaPlayerElement mpe, ref Border borderPanel)
        {
            MediaPlayerElement testMpe = mpe;
            var visibleEvent = new AutoResetEvent(false);
            Border BorderPanel = borderPanel;
            UIElement uiElement = testMpe as UIElement;
            long token = 0;
            bool isMTCVisible = false;
            UIExecutor.Execute(() =>
            {
                if (BorderPanel.Opacity != 0)
                {
                    Log.Comment("MTC still visible");
                    isMTCVisible = true;
                }
            });
            TestServices.WindowHelper.WaitForIdle();

            if (isMTCVisible)
            {
                return;
            }
            try
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Tap on MTC");
                    DependencyPropertyChangedCallback depChangedCallback = (source, property) =>
                    {
                        if (BorderPanel.Opacity == 1)
                        {
                            visibleEvent.Set();
                        }
                    };

                    token = BorderPanel.RegisterPropertyChangedCallback(Border.OpacityProperty, depChangedCallback);
                });
                TestServices.WindowHelper.WaitForIdle();

                using(var tappedEvent = new EventTester<UIElement, TappedRoutedEventArgs>(uiElement, "Tapped"))
                {
                    TestServices.InputHelper.Tap(testMpe);
                    TestServices.WindowHelper.WaitForIdle();
                    tappedEvent.Wait();
                    Verify.IsTrue(visibleEvent.WaitOne(TimeSpan.FromSeconds(5)));
                    Verify.AreEqual(1, tappedEvent.ExecuteCount);
                    var sender = tappedEvent.LastSender;
                    Verify.IsNotNull(sender);
                    Verify.AreEqual(uiElement, sender, "MediaPlayerElement Tapped for Show MTC");
                }
            }
            finally
            {
                UIExecutor.Execute(() =>
                {
                    BorderPanel.UnregisterPropertyChangedCallback(Border.OpacityProperty, token);
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        private void VerifyPlayPause(ref MediaPlayerElement mpe, ref MediaPlayer mp, ref Button playPauseButton, ref Border borderPanel)
        {

            MediaPlayerElement testMpe = mpe;
            MediaPlayer testMp = mp;
            Button PlayPauseButton = playPauseButton;

            using (var playbackStateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMp.PlaybackSession, "PlaybackStateChanged"))
            {

                ClickControl(playPauseButton, "PlayPause");
                playbackStateChangedEvent.Wait(TimeSpan.FromSeconds(5));
                Log.Comment("Current State Change");
                Log.Comment("Verify  play state.");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("State  " + testMp.PlaybackSession.PlaybackState.ToString());
                    Verify.IsTrue(MediaPlaybackState.Playing == testMp.PlaybackSession.PlaybackState, "Should be Playback State");
                });
                TestServices.WindowHelper.WaitForIdle();

                ShowMTCOnTap(ref testMpe, ref borderPanel);
                Log.Comment("Click the pause button.");
                ClickControl(playPauseButton, "PlayPause");
                playbackStateChangedEvent.Wait(TimeSpan.FromSeconds(5));
                Log.Comment("Current State Change");
                Log.Comment("Verify  Pause state.");

                UIExecutor.Execute(() =>
                {
                    Log.Comment("State  " + testMp.PlaybackSession.PlaybackState.ToString());
                    Verify.IsTrue(MediaPlaybackState.Paused == testMp.PlaybackSession.PlaybackState, "Should be Paused State");
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates PlayPause Button on Media Player Element with MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40007406: MPE crashes when the rewind button, followed by the show playback rate button is pressed. 
        public void PlayPauseButtonTest()
        {
            MediaPlayerElement mpe = null;
            Button playPauseButton = null;
            Border borderPanel = null;
            MediaPlayer testMediaPlayer = null;
            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for Play button button visibility.");
            UIExecutor.Execute(() =>
            {
                playPauseButton = mpe.FindNameInSubtree("PlayPauseButton") as Button;
                Verify.IsNotNull(playPauseButton);
                Visibility visibility = playPauseButton.Visibility;
                Verify.IsTrue(visibility == Visibility.Visible);

                borderPanel = mpe.FindNameInSubtree("ControlPanel_ControlPanelVisibilityStates_Border") as Border;
                Verify.IsNotNull(borderPanel);
            });
            TestServices.WindowHelper.WaitForIdle();
            VerifyPlayPause(ref mpe, ref testMediaPlayer, ref playPauseButton, ref borderPanel);
        }

        [TestMethod]
        [TestProperty("Description", "Validates FullWindow Button on Media Player Element with MediaTransport Control.")]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void FullWindowButtonTest()
        {

            MediaPlayerElement mpe = null;
            Button playPauseButton = null;
            Button fullWindowButton = null;
            Border borderPanel =  null;
            MediaPlayer testMediaPlayer = null;
            var FullWindowChangeEvent = new AutoResetEvent(false);
            long token = 0;
            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for Play button button visibility.");
            UIExecutor.Execute(() =>
            {
                playPauseButton = mpe.FindNameInSubtree("PlayPauseButton") as Button;
                Verify.IsNotNull(playPauseButton);
                Visibility visibility = playPauseButton.Visibility;
                Verify.IsTrue(visibility == Visibility.Visible);

                borderPanel = mpe.FindNameInSubtree("ControlPanel_ControlPanelVisibilityStates_Border") as Border;
                Verify.IsNotNull(borderPanel);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Check for Full Window button button visibility");
            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(mpe.IsFullWindow, "Not FullWindow");
                fullWindowButton = mpe.FindNameInSubtree("FullWindowButton") as Button;
                Verify.IsNotNull(fullWindowButton);
                Verify.IsTrue(fullWindowButton.Visibility == Visibility.Visible);
                DependencyPropertyChangedCallback depChangedCallback = (source, property) =>
                {
                    Log.Comment("FullWindows Property change");
                    FullWindowChangeEvent.Set();
                };
                token = mpe.RegisterPropertyChangedCallback(MediaPlayerElement.IsFullWindowProperty, depChangedCallback);
            });
            TestServices.WindowHelper.WaitForIdle();
            ClickControl(fullWindowButton, "FullWindow");
            Verify.IsTrue(FullWindowChangeEvent.WaitOne(TimeSpan.FromSeconds(5)));

            UIExecutor.Execute(() =>
            {
                Log.Comment("Verify  FullWindow state");
                Verify.IsTrue(mpe.IsFullWindow, "In FullWindow");
            });
            TestServices.WindowHelper.WaitForIdle();

            // For some reason WaitForIdle isn't waiting for the animation to complete.
            // Quick an dirty fix: Tick 100 times
            TestServices.WindowHelper.SynchronouslyTickUIThread(100);

            Log.Comment("Verify PlayPause in FullWindow");
            VerifyPlayPause(ref mpe, ref testMediaPlayer, ref playPauseButton, ref borderPanel);

            Log.Comment("Click the Exit FullWindow button");
            FullWindowChangeEvent.Reset();
            ClickControl(fullWindowButton, "Exit FullWindow");
            Verify.IsTrue(FullWindowChangeEvent.WaitOne(TimeSpan.FromSeconds(5)));

            UIExecutor.Execute(() =>
            {
                Log.Comment("Verify  Non FullWindow state");
                Verify.IsFalse(mpe.IsFullWindow, "Not FullWindow");
                mpe.UnregisterPropertyChangedCallback(MediaPlayerElement.IsFullWindowProperty, token);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates Zoom Button on Media Player Element with MediaTransport Control.")]

        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void ZoomButtonTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button zoomButton = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for Zoom button button visibility");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(XamlMedia.Stretch.Uniform == mpe.Stretch, "XamlMedia.Stretch.Uniform == mpe.Stretch");
                zoomButton = mpe.FindNameInSubtree("ZoomButton") as Button;
                Verify.IsNotNull(zoomButton);
                Verify.IsTrue(zoomButton.Visibility == Visibility.Visible);
                Log.Comment("Check for Black border should exist");
            });
            TestServices.WindowHelper.WaitForIdle();

            ClickControl(zoomButton, "Zoom");
            Log.Comment("Verify  Zoom state");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(XamlMedia.Stretch.UniformToFill == mpe.Stretch, "XamlMedia.Stretch.UniformToFill == mpe.Stretch");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates Seeking on Media Player Element with MediaTransport Control .")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void SeekingTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Slider slider = null;
            TimeSpan position = new TimeSpan();

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for ProgressBar visibility");
            UIExecutor.Execute(() =>
            {
                slider = mpe.FindNameInSubtree("ProgressSlider") as Slider;
                Verify.IsNotNull(slider);
                Verify.IsTrue(slider.Visibility == Visibility.Visible);
                position = testMediaPlayer.PlaybackSession.Position;
            });
            TestServices.WindowHelper.WaitForIdle();
            using (var seekCompletedEvent = new EventTester<MediaPlaybackSession, object>(testMediaPlayer.PlaybackSession, "SeekCompleted"))
            {
                Log.Comment("Click the Slider ");
                TestServices.InputHelper.Tap(slider);
                seekCompletedEvent.Wait();
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Seek Completed");
                    Log.Comment("Verify change in the Media Position");
                    Verify.IsTrue(position.TotalMilliseconds != testMediaPlayer.PlaybackSession.Position.TotalMilliseconds);
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates Volume Change on Media Player Element with MediaTransport Control .")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void VolumeChangeTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Slider slider = null;
            Button volumeButton = null;
            Flyout flyout = null;
            double volume = 0;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);
            using (var  volumeChangedEvent = new EventTester<MediaPlayer, object>(testMediaPlayer, "VolumeChanged"))
            {
                Log.Comment("Check for VolumeButton visibility");
                UIExecutor.Execute(() =>
                {
                    volume = testMediaPlayer.Volume;
                    volumeButton =  mpe.FindNameInSubtree("VolumeMuteButton") as Button;
                    Verify.IsNotNull(volumeButton);
                });
                TestServices.WindowHelper.WaitForIdle();
                ClickControl(volumeButton, "Volume");
                Log.Comment("Check for VolumeSlider visibility");
                UIExecutor.Execute(() =>
                {
                    flyout = (Flyout)volumeButton.Flyout;
                    Verify.IsNotNull(flyout);
                    slider = (Slider)VisualTreeUtils.GetVisualChildByNameFromOpenPopups("VolumeSlider", volumeButton);
                    Verify.IsNotNull(slider);
                    Verify.IsTrue(slider.Visibility == Visibility.Visible);
                });
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Click the Volume Slider");
                TestServices.InputHelper.Tap(slider);
                volumeChangedEvent.Wait();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Set Volume Change Event");
                    Log.Comment("Verify change in the volume Position");
                    Verify.IsTrue(volume != testMediaPlayer.Volume, "volume != testMediaPlayer.Volume" );
                    // close the flyout before exiting.
                    flyout.Hide();
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates Mute Change on Media Player Element with MediaTransport Control .")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        public void VolumeMuteTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button volumeButton = null;
            Flyout flyout = null;
            Button muteButton = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for VolumeButton visibility");
            UIExecutor.Execute(() =>
            {
                volumeButton = mpe.FindNameInSubtree("VolumeMuteButton") as Button;
                Verify.IsNotNull(volumeButton);
                Verify.IsTrue(volumeButton.Visibility == Visibility.Visible);
                Log.Comment("Verify VOlume is not Mute");
                Verify.IsFalse(testMediaPlayer.IsMuted, "IsMuted");
            });
            TestServices.WindowHelper.WaitForIdle();
            ClickControl(volumeButton, "Volume");

            using (var  muteChangedEvent = new EventTester<MediaPlayer, object>(testMediaPlayer, "IsMutedChanged"))
            {
                Log.Comment("Check for MuteButton visibility");
                UIExecutor.Execute(() =>
                {
                    flyout = (Flyout)volumeButton.Flyout;
                    Verify.IsNotNull(flyout);
                    muteButton = (Button)VisualTreeUtils.GetVisualChildByNameFromOpenPopups("AudioMuteButton", volumeButton);
                    Verify.IsNotNull(muteButton);
                    Verify.IsTrue(muteButton.Visibility == Visibility.Visible);
                });
                TestServices.WindowHelper.WaitForIdle();
                ClickControl(muteButton, "Volume");
                muteChangedEvent.Wait();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Set Mute Change Event");
                    Log.Comment("Verify the Mute");
                    Verify.IsTrue(testMediaPlayer.IsMuted, "IsMuted");
                    // close the flyout before exiting.
                    flyout.Hide();
                });
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Validates BackButton Test on FullWindow in Media Player Element with MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void BackButtonTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button fullWindowButton = null;
            var FullWindowChangeEvent = new AutoResetEvent(false);
            long token = 0;
            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for Full Window and set FullWindow");
            UIExecutor.Execute(() =>
            {
                Verify.IsFalse(mpe.IsFullWindow);
                fullWindowButton = mpe.FindNameInSubtree("FullWindowButton") as Button;
                Verify.IsNotNull(fullWindowButton);
                Verify.IsTrue(Visibility.Visible == fullWindowButton.Visibility);
            });
            TestServices.WindowHelper.WaitForIdle();
            ClickControl(fullWindowButton, "FullWindow");

            Log.Comment("Verify  FullWindows state");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(mpe.IsFullWindow);
                DependencyPropertyChangedCallback depChangedCallback = (source, property) =>
                {
                     Log.Comment("FullWindows Property change");
                    FullWindowChangeEvent.Set();
                    Verify.IsFalse(mpe.IsFullWindow, "IsFullWindow");
                };
                token = mpe.RegisterPropertyChangedCallback(MediaPlayerElement.IsFullWindowProperty, depChangedCallback);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Press Back button to reset full window");

            bool backButtonPressHandled = false;
            TestServices.Utilities.InjectBackButtonPress(out backButtonPressHandled);
            Verify.IsTrue(backButtonPressHandled, "backButtonPressHandled");
            Verify.IsTrue(FullWindowChangeEvent.WaitOne(TimeSpan.FromSeconds(5)));

            Log.Comment("In non-full window, back button presses should not get handled");
            TestServices.Utilities.InjectBackButtonPress(out backButtonPressHandled);
            Verify.IsFalse(backButtonPressHandled, "backButtonPressHandled");
            UIExecutor.Execute(() =>
            {
                mpe.UnregisterPropertyChangedCallback(MediaPlayerElement.IsFullWindowProperty, token);
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates SkipForward/Skip Backward and Stop Buttons Test on MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void SkipForwardBackwardAndStopTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button fastForwardButton = null;
            Button fastBackwardButton = null;
            Button stopButton = null;
            TimeSpan position = new TimeSpan();

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            UIExecutor.Execute(() =>
            {
                MediaTransportControls testMediaTransportControls = mpe.TransportControls;
                Log.Comment("Hide Default buttons for fit testing buttons in the screen");
                testMediaTransportControls.IsVolumeButtonVisible = false;
                testMediaTransportControls.IsZoomButtonVisible = false;
                // TODO: Integrate Full Screen work to WinAppSDK 1.x main
                // testMediaTransportControls.IsFullWindowButtonVisible = false;
                Log.Comment("Call Show Skip forward, Skip Backward and stop buttons");
                testMediaTransportControls.IsSkipBackwardButtonVisible = true;
                testMediaTransportControls.IsSkipBackwardEnabled = true;
                testMediaTransportControls.IsSkipForwardButtonVisible = true;
                testMediaTransportControls.IsSkipForwardEnabled = true;
                testMediaTransportControls.IsStopButtonVisible = true;
                testMediaTransportControls.IsStopEnabled = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Verify Visibility of the Skip forward, Skip Backward and stop buttons");
            UIExecutor.Execute(() =>
            {
                position = testMediaPlayer.PlaybackSession.Position;
                fastForwardButton = mpe.FindNameInSubtree("SkipForwardButton") as Button;
                Verify.IsNotNull(fastForwardButton);
                Verify.IsTrue(Visibility.Visible == fastForwardButton.Visibility);
                fastBackwardButton = mpe.FindNameInSubtree("SkipBackwardButton") as Button;
                Verify.IsNotNull(fastBackwardButton);
                Verify.IsTrue(Visibility.Visible == fastBackwardButton.Visibility);
                stopButton = mpe.FindNameInSubtree("StopButton") as Button;
                Verify.IsNotNull(stopButton);
                Verify.IsTrue(Visibility.Visible == stopButton.Visibility);
            });
            TestServices.WindowHelper.WaitForIdle();
            ClickControl(fastForwardButton, "Skip Forward");
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(position.TotalMilliseconds != testMediaPlayer.PlaybackSession.Position.TotalMilliseconds,
                                                       "position.TotalMilliseconds != testMediaPlayer.PlaybackSession.Position.TotalMilliseconds");
                position = testMediaPlayer.PlaybackSession.Position;
            });
            TestServices.WindowHelper.WaitForIdle();

            ClickControl(fastBackwardButton, "Skip Backward");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(position.TotalMilliseconds != testMediaPlayer.PlaybackSession.Position.TotalMilliseconds,
                                                    "position.TotalMilliseconds != testMediaPlayer.PlaybackSession.Position.TotalMilliseconds");
            });
            TestServices.WindowHelper.WaitForIdle();

            ClickControl(stopButton, "Stop");
            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(0 == testMediaPlayer.PlaybackSession.Position.TotalMilliseconds, "0 == testMediaPlayer.PlaybackSession.Position.TotalMilliseconds");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates PlaybackRate button Test on MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void PlaybackRateTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button playbackrateButton = null;
            MenuFlyout flyout = null;
            MenuFlyoutItem flyoutItem = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            UIExecutor.Execute(() =>
            {
                MediaTransportControls testMediaTransportControls = mpe.TransportControls;
                Log.Comment("Hide Default buttons for fit testing buttons in the screen");
                testMediaTransportControls.IsVolumeButtonVisible = false;
                testMediaTransportControls.IsZoomButtonVisible = false;
                // TODO: Integrate Full Screen work to WinAppSDK 1.x main
                // testMediaTransportControls.IsFullWindowButtonVisible = false;
                Log.Comment("Call Show PlaybackRate button");
                testMediaTransportControls.IsPlaybackRateButtonVisible = true;
                testMediaTransportControls.IsPlaybackRateEnabled = true;
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Verify Visibility of the PlaybackRate button");
            UIExecutor.Execute(() =>
            {
                playbackrateButton = mpe.FindNameInSubtree("PlaybackRateButton") as Button;
                Verify.IsNotNull(playbackrateButton);
                Verify.IsTrue(Visibility.Visible == playbackrateButton.Visibility);
            });
            TestServices.WindowHelper.WaitForIdle();

            ClickControl(playbackrateButton, "PlaybackRate");
            UIExecutor.Execute(() =>
            {
                flyout = (MenuFlyout)playbackrateButton.Flyout;
                Verify.IsNotNull(flyout);
                // Playbackrate Flyout items should equal to 5.
                Verify.IsTrue(5 == flyout.Items.Count, "5 == flyout.Items.Count");
                // Select the Playback rate 0.5 which exist as second item in the Menu.
                flyoutItem = (MenuFlyoutItem)flyout.Items[1];
                Verify.IsNotNull(flyoutItem);
                Verify.IsTrue(1 == testMediaPlayer.PlaybackSession.PlaybackRate);
            });
            TestServices.WindowHelper.WaitForIdle();

            Log.Comment("Tap the item in the Flyout List");
            using (var playbackRateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMediaPlayer.PlaybackSession, "PlaybackRateChanged"))
            {
                UIElement uiElement = flyoutItem as UIElement;
                using (var tappedEvent = new EventTester<UIElement, TappedRoutedEventArgs>(uiElement, "Tapped"))
                {
                    TestServices.InputHelper.Tap(flyoutItem);
                    TestServices.WindowHelper.WaitForIdle();
                    tappedEvent.Wait();
                    playbackRateChangedEvent.Wait();
                    Verify.AreEqual(1, tappedEvent.ExecuteCount);
                    var sender = tappedEvent.LastSender;
                    Verify.IsNotNull(sender);
                    Verify.AreEqual(uiElement, sender, "Click event fired on Flyout Item");
                    Log.Comment("Playback Rate Changed on the MediaPlayer");
                }
            }

            UIExecutor.Execute(() =>
            {
                Verify.IsTrue(0.5 == testMediaPlayer.PlaybackSession.PlaybackRate, "0.5 == testMediaPlayer.PlaybackSession.PlaybackRate");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates Error Message UI Test on MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void ErrorMessageUITest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Border errorBorder = null;
            TextBlock errorTextBlock = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);
            using (var mediaFailedEvent = new EventTester<MediaPlayer, MediaPlayerFailedEventArgs>(testMediaPlayer, "MediaFailed"))
            {
                UIExecutor.Execute(() =>
                {
                    errorBorder = mpe.FindNameInSubtree("ErrorBorder") as Border;
                    errorTextBlock = mpe.FindNameInSubtree("ErrorTextBlock") as TextBlock;
                    Verify.IsNotNull(errorBorder);
                    Verify.IsNotNull(errorTextBlock);
                    Verify.IsTrue(Visibility.Collapsed == errorBorder.Visibility);
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Set the invalid source ");
                    Uri invalidUri = new Uri("ms-appx:///Assets/invalidfile.mp4");
                    MediaSource source = MediaSource.CreateFromUri(invalidUri);
                    MediaPlaybackItem item = new MediaPlaybackItem(source);
                    mpe.Source = item;
                });
                TestServices.WindowHelper.WaitForIdle();
                mediaFailedEvent.Wait();
            }
            UIExecutor.Execute(() =>
            {
                Log.Comment("MediaFailed Event Fired");
                Verify.IsTrue(Visibility.Visible == errorBorder.Visibility,"Visibility.Visible == errorBorder.Visibility");
                Verify.IsFalse(errorTextBlock.Text.Length == 0, "errorTextBlock.Text.Length == 0");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates MediaTransportControls buttons should drop out on Width change Test.")]
        [TestProperty("TestPass:ExcludeOn", "OneCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void DropOutOnWidthChangeTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button zoomButton = null;
            Button castButton = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            FrameworkElement frmElement = mpe as FrameworkElement;
            using (var sizeChangedEvent = new EventTester<FrameworkElement, SizeChangedEventArgs>(frmElement, "SizeChanged"))
            {
                UIExecutor.Execute(() =>
                {
                    zoomButton = mpe.FindNameInSubtree("ZoomButton") as Button;
                    castButton = mpe.FindNameInSubtree("CastButton") as Button;
                    Verify.IsNotNull(zoomButton);
                    Verify.IsNotNull(castButton);
                    Verify.IsTrue(Visibility.Visible == zoomButton.Visibility, "Visibility.Visible == zoomButton.Visibility");
                    Verify.IsTrue(Visibility.Visible == castButton.Visibility, "Visibility.Visible == castButton.Visibility");
                    Log.Comment("Resize the Width to 100");
                    mpe.Width = 100;
                });
                TestServices.WindowHelper.WaitForIdle();
                sizeChangedEvent.Wait();
                var sender = sizeChangedEvent.LastSender;
                Verify.IsNotNull(sender);
                Verify.AreEqual(frmElement, sender, "MediaPlayerElement Size Changed");
            }

            UIExecutor.Execute(() =>
            {
                Log.Comment("Cast and Zoom button should be collapsed.");
                Verify.IsTrue(Visibility.Collapsed == zoomButton.Visibility, "Visibility.Collapsed == zoomButton.Visibility");
                Verify.IsTrue(Visibility.Collapsed == castButton.Visibility, "Visibility.Collapsed == castButton.Visibility");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Validates FastForward on MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public void FastForwardTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button fastForwardButton = null;
            Border borderPanel = null;
            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Play the Video");
                testMediaPlayer.Play();
                MediaTransportControls testMediaTransportControls = mpe.TransportControls;
                Log.Comment("Hide Default buttons for fit testing buttons in the screen");
                testMediaTransportControls.IsVolumeButtonVisible = false;
                testMediaTransportControls.IsZoomButtonVisible = false;
                // TODO: Integrate Full Screen work to WinAppSDK 1.x main
                // testMediaTransportControls.IsFullWindowButtonVisible = false;
                Log.Comment("Call Show Fast forward");
                testMediaTransportControls.IsFastForwardButtonVisible = true;
                testMediaTransportControls.IsFastForwardEnabled = true;
            });
            TestServices.WindowHelper.WaitForIdle();
            using (var playbackRateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMediaPlayer.PlaybackSession, "PlaybackRateChanged"))
            {
                Log.Comment("Verify Visibility of the Fast forward Button");
                UIExecutor.Execute(() =>
                {
                    fastForwardButton = mpe.FindNameInSubtree("FastForwardButton") as Button;
                    Verify.IsNotNull(fastForwardButton);
                    Verify.IsTrue(Visibility.Visible == fastForwardButton.Visibility);
                    Verify.IsTrue(testMediaPlayer.PlaybackSession.PlaybackRate == 1, "testMediaPlayer.PlaybackSession.PlaybackRate == 1");
                    borderPanel = mpe.FindNameInSubtree("ControlPanel_ControlPanelVisibilityStates_Border") as Border;
                    Verify.IsNotNull(borderPanel);
                });
                TestServices.WindowHelper.WaitForIdle();
                ShowMTCOnTap(ref mpe, ref borderPanel);
                ClickControl(fastForwardButton, "Fast Forward");
                playbackRateChangedEvent.Wait();
            }
            UIExecutor.Execute(() =>
            {
                Log.Comment("Playback Rate change to " + testMediaPlayer.PlaybackSession.PlaybackRate.ToString());
                Verify.IsTrue(testMediaPlayer.PlaybackSession.PlaybackRate == 2, "testMediaPlayer.PlaybackSession.PlaybackRate == 2");
            });
            TestServices.WindowHelper.WaitForIdle();
        }
        [TestMethod]
        [TestProperty("Description", "Validates Rewind on MediaTransport Control.")]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Ignore", "True")] // TODO 40007406: MPE crashes when the rewind button, followed by the show playback rate button is pressed. 
        public void RewindTest()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            Button rewindButton = null;
            Border borderPanel = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Play the Video");
                testMediaPlayer.Play();
                MediaTransportControls testMediaTransportControls = mpe.TransportControls;
                Log.Comment("Hide Default buttons for fit testing buttons in the screen");
                testMediaTransportControls.IsVolumeButtonVisible = false;
                testMediaTransportControls.IsZoomButtonVisible = false;
                // TODO: Integrate Full Screen work to WinAppSDK 1.x main
                // testMediaTransportControls.IsFullWindowButtonVisible = false;
                Log.Comment("Call Show Rewind button");
                testMediaTransportControls.IsFastRewindButtonVisible = true;
                testMediaTransportControls.IsFastRewindEnabled = true;
            });
            TestServices.WindowHelper.WaitForIdle();
            using (var playbackRateChangedEvent = new EventTester<MediaPlaybackSession, object>(testMediaPlayer.PlaybackSession, "PlaybackRateChanged"))
            {
                Log.Comment("Verify Visibility of the Rewind Buttons");
                UIExecutor.Execute(() =>
                {
                    rewindButton = mpe.FindNameInSubtree("RewindButton") as Button;
                    Verify.IsNotNull(rewindButton);
                    Verify.IsTrue(Visibility.Visible == rewindButton.Visibility);
                    Verify.IsTrue(testMediaPlayer.PlaybackSession.PlaybackRate == 1, "testMediaPlayer.PlaybackSession.PlaybackRate == 1");
                    borderPanel = mpe.FindNameInSubtree("ControlPanel_ControlPanelVisibilityStates_Border") as Border;
                    Verify.IsNotNull(borderPanel);
                });
                TestServices.WindowHelper.WaitForIdle();
                ShowMTCOnTap(ref mpe, ref borderPanel);
                ClickControl(rewindButton, "Rewind");
                playbackRateChangedEvent.Wait();
            }

            UIExecutor.Execute(() =>
            {
                Log.Comment("Playback Rate change to " + testMediaPlayer.PlaybackSession.PlaybackRate.ToString());
                Verify.IsTrue(testMediaPlayer.PlaybackSession.PlaybackRate == -2, "testMediaPlayer.PlaybackSession.PlaybackRate == -2");
            });
            TestServices.WindowHelper.WaitForIdle();
        }

        [TestMethod]
        [TestProperty("Description", "Reset MTC on Reset MediaPlayer.")]
        public void ResetMTCOnResetMediaPlayer()
        {
            MediaPlayerElement mpe = null;
            MediaPlayer testMediaPlayer = null;
            TextBlock timeRemaining = null;

            SetupMediaPlayerElementUI(out mpe, out testMediaPlayer);

            Log.Comment("Check for ProgressBar visibility");
            UIExecutor.Execute(() =>
            {
                timeRemaining = mpe.FindNameInSubtree("TimeRemainingElement") as TextBlock;
                Verify.IsNotNull(timeRemaining);
                Verify.IsTrue(timeRemaining.Visibility == Visibility.Visible);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("timeRemaining " + timeRemaining.Text);
                Verify.IsTrue(timeRemaining.Text != "");
                Log.Comment("Reset MediaPlayer ");
                mpe.SetMediaPlayer(null);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Verify timeRemaining should reset ");
                Verify.IsTrue(timeRemaining.Text == "");
            });
        }
    }
}
