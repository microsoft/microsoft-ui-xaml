// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Threading.Tasks;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Windows.Foundation;
using Windows.Graphics.Imaging;
using Windows.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Windows.Media.Playback;
using Private.Infrastructure;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.Media
{
    public sealed class MediaPlayerElementTester: AbstractMediaTester<MediaPlayerElement>
    {
        public MediaPlayerElementTester()
        {
        }

        private MediaPlayer MediaPlayer { get; set; }
        private XamlControls.MediaPlayerPresenter Presenter { get; set; }

        protected override IEnumerable<Func<ElementTester<MediaPlayerElement>, Task>> DefaultActions
        {
            get
            {
                return new Func<ElementTester<MediaPlayerElement>, Task>[]
                {
                    MediaPlayerElementTester.TestProperties,
                    MediaPlayerElementTester.TestVideoIsRendered,
                    MediaPlayerElementTester.TestDoubleTappedEvent,
                    MediaPlayerElementTester.TestMediaEndedEvent,
                    MediaPlayerElementTester.TestClose,
                };
            }
        }

        private async Task Open(MediaPlayer mp)
        {
            using(var mediaOpenedEvent = new EventTester<MediaPlaybackSession, object>(mp.PlaybackSession, "PlaybackStateChanged"))
            {
                var currentPlaybackState = mp.PlaybackSession.PlaybackState;
                Log.Comment($"Media playback is {currentPlaybackState}");
                if (currentPlaybackState == MediaPlaybackState.Opening)
                {
                    Log.Comment("Wating for media to be opened");
                    await mediaOpenedEvent.VerifyEventRaised();
                }
                currentPlaybackState = mp.PlaybackSession.PlaybackState;
                Log.Comment("Media playback is " + currentPlaybackState.ToString());
            }
            TestServices.WindowHelper.WaitForIdle();
        }

        private async Task Seek(MediaPlayer mp)
        {
            Log.Comment($"Seek to {this.SeekPosition.Seconds} secs");
            mp.PlaybackSession.Position = this.SeekPosition;

            using(var pausedEvent = new EventTester<MediaPlaybackSession, object>(mp.PlaybackSession, "PlaybackStateChanged"))
            {
                TestServices.WindowHelper.WaitForIdle();
                var currentPlaybackState = mp.PlaybackSession.PlaybackState;
                while (currentPlaybackState != MediaPlaybackState.Paused)
                {
                    if (currentPlaybackState == MediaPlaybackState.Playing)
                    {
                        Verify.IsTrue(this.AutoPlay, "Autoplay must be enabled");
                        Log.Comment("AutoPlay enabled, pausing...");
                        mp.Pause();
                        await pausedEvent.VerifyEventRaised();
                    }
                    currentPlaybackState = mp.PlaybackSession.PlaybackState;
                }
                Log.Comment("Media playback is "+ currentPlaybackState.ToString());
            }

            Log.Comment($"Seek to {this.SeekPosition.Seconds} secs");
            mp.PlaybackSession.Position = this.SeekPosition;
            TestServices.WindowHelper.WaitForIdle();
        }

        private XamlControls.MediaPlayerPresenter GetPresenter(XamlControls.MediaPlayerElement mpe)
        {
            var presenter = default(XamlControls.MediaPlayerPresenter);
            UIExecutor.Execute(() =>
            {
                if (mpe.IsFullWindow)
                {
                    mpe.IsFullWindow = false;
                }
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                presenter = mpe.FindElementOfTypeInSubtree<XamlControls.MediaPlayerPresenter>();
                mpe.IsFullWindow = this.IsFullWindow;
            });

            TestServices.WindowHelper.WaitForIdle();
            return presenter;
        }

        private Task TestProperties(MediaPlayerElement mpe)
        {
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(this.AreTransportControlsEnabled, mpe.AreTransportControlsEnabled);
                if (this.AreTransportControlsEnabled)
                {
                    Verify.IsNotNull(mpe.TransportControls);
                }
                else
                {
                    // We create an MTC instance even when AreTransportControlsEnabled is false
                    Verify.IsNotNull(mpe.TransportControls);
                }

                Verify.AreEqual(this.IsFullWindow, mpe.IsFullWindow);
                Verify.AreEqual(this.Stretch, mpe.Stretch);
                Verify.AreEqual(this.AutoPlay, mpe.AutoPlay);

                var presenter = mpe.FindElementOfTypeInSubtree<XamlControls.MediaPlayerPresenter>();
                if (this.IsFullWindow)
                {
                    Verify.IsNull(presenter);
                }
                else
                {
                    Verify.IsNotNull(presenter, "presenter");
                    Verify.AreEqual(presenter.IsFullWindow, mpe.IsFullWindow);
                    Verify.AreEqual(presenter.Stretch, mpe.Stretch);
                    Verify.AreEqual(presenter.MediaPlayer, mpe.MediaPlayer);
                }

                if (this.AreTransportControlsEnabled)
                {
                    var mtc = mpe.FindElementOfTypeInSubtree<XamlControls.MediaTransportControls>();
                    if (this.IsFullWindow)
                    {
                        Verify.IsNull(mtc);
                        mtc = mpe.TransportControls;
                    }

                    Verify.IsNotNull(mtc, "mtc");
                    Verify.AreEqual(mtc, mpe.TransportControls, "mpe.TransportControls");
                }
            });
            return Task.FromResult(0);
        }

        public static Task TestProperties(ElementTester<MediaPlayerElement> t)
        {
            return ((MediaPlayerElementTester)t).TestProperties(t.Element);
        }

        private async Task TestDoubleTappedEvent(XamlControls.MediaPlayerElement mpe)
        {
            using (new EventTester<XamlControls.MediaPlayerElement, TappedRoutedEventArgs>(mpe, "Tapped", EventTesterOptions.None, setBVTflags: false))
            using (var doubleTappedEvent = new EventTester<XamlControls.MediaPlayerElement, DoubleTappedRoutedEventArgs>(mpe, "DoubleTapped"))
            {
                Log.Comment("Double-tapping the MediaPlayerElement");
                TestServicesExtensions.EnsureForegroundWindow();
                TestServices.InputHelper.DoubleTap(mpe);
                TestServices.WindowHelper.WaitForIdle();
                await doubleTappedEvent.VerifyEventRaised();
                Verify.AreEqual(1, doubleTappedEvent.ExecuteCount);
                var mpeSender = doubleTappedEvent.LastSender as XamlControls.MediaPlayerElement;
                Verify.IsNotNull(mpeSender);
                Verify.AreEqual(mpe, mpeSender);
            }
        }

        public static Task TestDoubleTappedEvent(ElementTester<MediaPlayerElement> t)
        {
            return ((MediaPlayerElementTester)t).TestDoubleTappedEvent(t.Element);
        }

        private async Task TestVideoIsRendered()
        {
            await base.TestVideoIsRenderedProtected(this.Presenter);
        }

        public static Task TestVideoIsRendered(ElementTester<MediaPlayerElement> t)
        {
            return ((MediaPlayerElementTester)t).TestVideoIsRendered();
        }

        private Task TestMediaEndedEvent(XamlControls.MediaPlayerElement mpe)
        {
            using(var mediaEndedEvent = new EventTester<MediaPlayer, object>(this.MediaPlayer, "MediaEnded"))
            {
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(this.MediaPlayer, mpe.MediaPlayer);
                    Log.Comment($"Playing from position {this.MediaPlayer.PlaybackSession.Position}");
                    this.MediaPlayer.Play();
                });
                TestServices.WindowHelper.WaitForIdle();
                mediaEndedEvent.Wait();
            }
            return Task.FromResult(0);
        }

        public static Task TestMediaEndedEvent(ElementTester<MediaPlayerElement> t)
        {
            return ((MediaPlayerElementTester)t).TestMediaEndedEvent(t.Element);
        }

        private Task TestClose()
        {
            TestServices.WindowHelper.WaitForIdle();
            {
                UIExecutor.Execute(() =>
                {
                    Verify.AreEqual(this.MediaPlayer, this.Element.MediaPlayer);
                    Log.Comment("Clearing MediaPlayer");
                    this.Element.SetMediaPlayer(null);
                    Verify.IsNull(this.Element.MediaPlayer);
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    #pragma warning disable 0618
                    MediaPlayerState currentPlayerState = this.MediaPlayer.CurrentState;
                    Verify.AreEqual(MediaPlayerState.Closed, currentPlayerState);
                    #pragma warning restore 0618
                    this.MediaPlayer = null;
                    this.Presenter = null;
                });
            }

            return Task.FromResult(0);
        }

        public static Task TestClose(ElementTester<MediaPlayerElement> t)
        {
            return ((MediaPlayerElementTester)t).TestClose();
        }

        protected override async Task BeforeTestActions()
        {
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(this.AutoPlay, this.Element.AutoPlay);
                this.MediaPlayer = this.Element.MediaPlayer;
                Verify.AreEqual(this.AutoPlay, this.MediaPlayer.AutoPlay);
            });

            TestServices.WindowHelper.WaitForIdle();
            await this.Open(this.MediaPlayer);
            await this.Seek(this.MediaPlayer);
            this.Presenter = this.GetPresenter(this.Element);
            Verify.IsNotNull(this.Presenter, "this.Presenter");
        }
    }
}
