// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Windows.Foundation;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;

using Microsoft.UI.Xaml.Controls;
using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Windows.Media.Playback;
using Windows.Media.Core;
using Private.Infrastructure;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Foundation.Graphics.Media
{
    [TestClass]
    public class MediaPlayerElementTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            // Make sure you always call XamlTestsBase.SetupBase
            // here to ensure the test services are initialized.
            XamlTestsBase.SetupBase(context);
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        public string ResourcesPath
        {
            get
            {
                return XamlTestsBase.Context.TestDeploymentDir + @"\resources\native\external\foundation\graphics\media\";
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlayWithMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlayWithMTCWUC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                DCompRendering = DCompRendering.WUCCompleteSynchronousCompTree,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")] // inconsistent failure in WPF islands
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlayWithNoMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                Stretch = XamlMedia.Stretch.UniformToFill,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlayFullWindowNoMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                Stretch = XamlMedia.Stretch.Uniform,
                IsFullWindow = true,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlayFullWindowWithMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                IsFullWindow = true,
                Stretch = XamlMedia.Stretch.Fill,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // inconsistent failure in WPF island mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanAutoPlayWithMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                AutoPlay = true,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanAutoPlayWithNoMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AutoPlay = true,
                Stretch = XamlMedia.Stretch.UniformToFill,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanAutoPlayFullWindowNoMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                IsFullWindow = true,
                AutoPlay = true,
                Stretch = XamlMedia.Stretch.Uniform,
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanAutoPlayFullWindowWithMTC()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                IsFullWindow = true,
                AutoPlay = true,
                Stretch = XamlMedia.Stretch.Fill,
            };
            await tester.DoTest();
        }

        private IDisposable SetupZoomInVideo(XamlControls.MediaPlayerElement mpe, global::Windows.Foundation.Rect rect)
        {
            var mp = default(MediaPlayer);
            UIExecutor.Execute(() =>
            {
                mp = mpe.MediaPlayer;
            });

            Log.Comment(string.Format("Setting NormalizedSourceRect: {0},{1},{2},{3}", rect.X, rect.Y, rect.Width, rect.Height));
            var playbackSession = mp.PlaybackSession;
            playbackSession.NormalizedSourceRect = rect;
            return null;
        }

        private Task TestVideoIsZoomed(ElementTester<MediaPlayerElement> t, global::Windows.Foundation.Rect rect)
        {
            var mp = default(MediaPlayer);
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(t.Element, "mpe");
                mp = t.Element.MediaPlayer;
                Verify.IsNotNull(mp, "mp");
            });
            var playbackSession = mp.PlaybackSession;
            Verify.AreEqual(playbackSession.NormalizedSourceRect, rect);
            return Task.FromResult(0);
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public async Task CanZoomInVideo()
        {
            var rect = new global::Windows.Foundation.Rect()
            {
                X = 0.25,
                Y = 0.25,
                Width = 0.75,
                Height = 0.75,
            };

            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                AutoPlay = false,
                Stretch = XamlMedia.Stretch.Uniform,
                SetupAction = (mpe) => this.SetupZoomInVideo(mpe, rect),
                AsyncTestActions =
                {
                    (t) => this.TestVideoIsZoomed(t, rect),
                },
            };
            await tester.DoTest();
        }

        private IDisposable SetCoreWindowBoundsMode()
        {
            UIExecutor.Execute(() =>
            {
                TestServices.WindowHelper.WindowContent = new XamlControls.Grid();
                var v = global::Windows.UI.ViewManagement.ApplicationView.GetForCurrentView();
                v.SetDesiredBoundsMode(global::Windows.UI.ViewManagement.ApplicationViewBoundsMode.UseCoreWindow);
            });
            return null; // TestServices.Utilities.MockPlatform(DEVICEFAMILYINFOENUM.XBOX);
        }

        private Task TestMTCHasMargin(ElementTester<MediaPlayerElement> t)
        {
            var mpe = t.Element;
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var grid = mpe.TransportControls.FindNameInSubtree("ControlPanelGrid") as XamlControls.Grid;
                Verify.IsNotNull(grid);
                Verify.AreNotEqual(grid.Margin.Left, 0);
                Verify.AreNotEqual(grid.Margin.Right, 0);
                Verify.AreNotEqual(grid.Margin.Bottom, 0);
                Verify.AreEqual(grid.Margin.Top, 0);
            });

            return Task.FromResult(0);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // FullWindow mode is not yet supported in WPF hosting mode
        [TestProperty("Ignore", "TRUE")]
        public async Task MTCGetsMarginInFullScreenOnXbox()
        {
            var tester = new MediaPlayerElementTester()
            {
                PreSetupAction = () => this.SetCoreWindowBoundsMode(),
                AreTransportControlsEnabled = true,
                AutoPlay = true,
                IsFullWindow = true,
                Stretch = XamlMedia.Stretch.Uniform,
                AsyncTestActions =
                {
                    this.TestMTCHasMargin,
                },
            };
            await tester.DoTest();
        }

        private IDisposable SetPlayList(XamlControls.MediaPlayerElement mpe)
        {
            var mp = default(MediaPlayer);
            UIExecutor.Execute(() =>
            {
                mp = mpe.MediaPlayer;
            });

            var videoUrls = new string[]
            {
                this.ResourcesPath + "blueframe_video.mp4",
            };
            var sources = videoUrls.Select(url => MediaSource.CreateFromUri(new Uri(url)));
            var items = sources.Select(s => new MediaPlaybackItem(s));
            var list = new MediaPlaybackList();
            foreach (var i in items)
            {
                list.Items.Add(i);
            }
            mp.Source = list;
            return null;
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")] // fails in WPF mode
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task CanPlaylist()
        {
            var tester = new MediaPlayerElementTester()
            {
                AutoPlay = false,
                IsFullWindow = false,
                Stretch = XamlMedia.Stretch.Uniform,
                Source = null,
                AreTransportControlsEnabled = true,
                SetupAction = this.SetPlayList,
            };
            await tester.DoTest();
        }

        private Task VerifyResizeToVideoSize(ElementTester<MediaPlayerElement> t, Size videoSize)
        {
            var mpe = t.Element;
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(mpe.ActualWidth, videoSize.Width, "mpe.ActualWidth == videoSize.Width");
                Verify.AreEqual(mpe.ActualHeight, videoSize.Height, "mpe.ActualHeight == videoSize.Height");
            });

            return Task.FromResult(0);
        }

        [TestMethod]
        public async Task ResizeToVideoSize()
        {
            Size expectedSize = new Size
            {
                Height = 480.0,
                Width = 854.0,
            };

            var tester = new MediaPlayerElementTester()
            {
                AutoPlay = false,
                IsFullWindow = false,
                Stretch = XamlMedia.Stretch.Uniform,
                AreTransportControlsEnabled = true,
                Width = double.NaN,
                Height = double.NaN,
                GridWidth = "Auto",
                GridHeight = "Auto",
                AsyncTestActions =
                {
                    (t) => this.VerifyResizeToVideoSize(t, expectedSize),
                },
            };
            await tester.DoTest();
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "WPF")] // TODO: Remove this entire test only after Fullscreen media works in ApplicationWindow and Win32 XAML Islands is complete
        [TestProperty("Ignore", "True")] // TODO 40532721: Re-enable or remove disabled tests in WinUI3
        public async Task TestFullWindowButtonDisabled()
        {
            var tester = new MediaPlayerElementTester()
            {
                AreTransportControlsEnabled = true,
                AutoPlay = false,
                Stretch = XamlMedia.Stretch.Uniform,
                AsyncTestActions =
                {
                    (ElementTester<MediaPlayerElement> t) => 
                    {
                            var mpe = t.Element;
                            TestServices.WindowHelper.WaitForIdle();
                            UIExecutor.Execute(() =>
                            {
                                var button = mpe.TransportControls.FindNameInSubtree("FullWindowButton") as XamlControls.Button;
                                Verify.IsNotNull(button);
                                Verify.AreEqual(button.Visibility, Visibility.Collapsed);
                                Verify.AreEqual(button.IsEnabled, false);
                            });

                            // explicitly asking to make full window button visible
                            UIExecutor.Execute(() =>
                            {
                                // TODO: Integrate Full Screen work to WinAppSDK 1.x main
                                // mpe.TransportControls.IsFullWindowButtonVisible = true;
                            });
                            TestServices.WindowHelper.WaitForIdle();
                            
                            UIExecutor.Execute(() =>
                            {
                                var button = mpe.TransportControls.FindNameInSubtree("FullWindowButton") as XamlControls.Button;
                                Verify.IsNotNull(button);
                                Verify.AreEqual(button.Visibility, Visibility.Collapsed); //it should still be collapsed
                            });

                            return Task.FromResult(0);
                    },
                },
            };
            await tester.DoTest();
        }
    }
}
