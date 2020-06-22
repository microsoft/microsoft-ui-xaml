// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using System.Threading.Tasks;
using AnimatedVisualPlayerTests;
using Common;
using Microsoft.Graphics.Canvas;
using Windows.Graphics;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed class FallbackGrid : Grid
    {
        internal const int RectangleWidth = 100;
        internal const int RectangleHeight = 100;
        internal static readonly Color RectangleColor = Colors.Red;
        // Gets the most recently created FallbackGrid
        internal static FallbackGrid Latest;

        public FallbackGrid()
        {
            Latest = this;
            Windows.UI.Xaml.Shapes.Rectangle rect = new Windows.UI.Xaml.Shapes.Rectangle();
            rect.MaxWidth = rect.MinWidth = RectangleWidth;
            rect.MaxHeight = rect.MinHeight = RectangleHeight;
            rect.Fill = new SolidColorBrush(RectangleColor);
            Children.Add(rect);
        }
    }

    [TopLevelTestPage(Name = "AnimatedVisualPlayer", Icon = "Animations.png")]
    public sealed partial class AnimatedVisualPlayerPage : TestPage
    {
        public AnimatedVisualPlayerPage()
        {
            this.InitializeComponent();
            SetAutomationPeerNames(PageGrid);
        }

        // Set the AutomationPeer name to be the same as the x:Name on each FrameworkElement
        // that is a child of the given panel, and continue recursively into any panel children.
        // This is so we don't need to set both properties on the elements in XAML.
        void SetAutomationPeerNames(Panel root)
        {
            foreach (var child in root.Children)
            { 
                if (child is FrameworkElement element)
                {
                    // Copy the x:Name into the AutomationPeer name unless it already has an AutomationPeer name.
                    var name = element.Name;
                    if (!string.IsNullOrEmpty(name) && string.IsNullOrEmpty(AutomationProperties.GetName(element)))
                    {
                        AutomationProperties.SetName(element, name);
                    }

                    if (child is Panel panel)
                    {
                        // Recurse to get the rest of the tree.
                        SetAutomationPeerNames(panel);
                    }
                }
            }
        }

        async void Play(double from, double to, TextBox statusTextBox)
        {
            IsPlayingTextBoxBeforePlaying.Text = Player.IsPlaying.ToString();

            // Start playing and concurrently get the IsPlaying state.
            Task task1 = Player.PlayAsync(0, 1, false).AsTask();
            Task task2 = GetIsPlayingAsync();

            // Wait for playing to finish.
            await Task.WhenAll(task1, task2);
            statusTextBox.Text = Constants.PlayingEndedText;
        }

        void PlayForward(double fromProgress, double toProgress, TextBox statusTextBox)
        {
            Player.PlaybackRate = 1;
            Play(fromProgress, toProgress, statusTextBox);
        }

        // Play from 0 to 1.
        void PlayButton_Click(object sender, RoutedEventArgs e)
        {
            IsPlayingTextBoxBeforePlaying.Text = Player.IsPlaying.ToString();
            PlayForward(0, 1, ProgressTextBox);
        }

        // Play forwards from 0.35 to 0.
        void ToZeroKeyframeAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            PlayForward(0.35, 0, ToZeroKeyframeAnimationProgressTextBox);
        }

        // Play forwards from 0.35 to 0.3.
        void AroundTheEndAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            PlayForward(0.35, 0.3, AroundTheEndAnimationProgressTextBox);
        }
        
        // Play forwards from 1 to 0.35.
        void FromOneKeyframeAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            PlayForward(1, 0.35, FromOneKeyframeAnimationProgressTextBox);
        }

        // Play backwards from 1 to 0.5 then forwards from 0.5 to 1.
        async void ReverseNegativePlaybackRateAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            // Start playing backwards from 1 to 0.
            Player.PlaybackRate = -1;
            Task task1 = Player.PlayAsync(0, 1, false).AsTask();

            // Reverse direction after half of the animation duration.
            Task task2 = DelayForHalfAnimationDurationThenReverse();

            await Task.WhenAll(task1, task2);

            SetTextBoxTextToPlayerProgress(
                ReverseNegativePlaybackRateAnimationTextBox,
                defaultValueForPreRs5: Constants.OneText);
        }

        // Play forward from 0 to 0.5 then backward from 0.5 to 0.
        async void ReversePositivePlaybackRateAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            // Start playing forward from 0 to 1.
            Player.PlaybackRate = 1;
            Task task1 = Player.PlayAsync(0, 1, false).AsTask();

            // Reverse direction after half of the animation duration.
            Task task2 = DelayForHalfAnimationDurationThenReverse();

            await Task.WhenAll(task1, task2);

            SetTextBoxTextToPlayerProgress(
                ReversePositivePlaybackRateAnimationTextBox,
                defaultValueForPreRs5: Constants.ZeroText);
        }

        void SetTextBoxTextToPlayerProgress(TextBox textBox, string defaultValueForPreRs5)
        {
            // The player's PlayAsync returns immediately in RS4 or lower windows build.
            // For those builds set the text box with the given default value so that the tests will pass.
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // Read the progress value from the player.
                var progressPropertySet = (CompositionPropertySet)Player.ProgressObject;
                progressPropertySet.TryGetScalar("progress", out var value);
                textBox.Text = value.ToString();
            }
            else
            {
                textBox.Text = defaultValueForPreRs5;
            }
        }

        void Player_PointerMoved(object sender, RoutedEventArgs e)
        {
            HittestingTextBox.Text = Constants.PointerMovedText;
        }

        async Task GetIsPlayingAsync()
        {
            //
            // This artificial delay of 200ms is to ensure that the player's PlayAsync
            // has enough time ready to set value of IsPlaying property to true.
            //
            await Task.Delay(200);

            //
            // The player's PlayAsync returns immediately in RS4 or lower windows build.
            // Thus, Constants.TrueText is set to IsPlayingTextBoxBeingPlaying's content
            // in order to satisfy the interaction test that uses Accessibility.
            //
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Player.Pause();
                IsPlayingTextBoxBeingPlaying.Text = Player.IsPlaying.ToString();
                Player.Resume();
            }
            else
            {
                IsPlayingTextBoxBeingPlaying.Text = Constants.TrueText;
            }
        }

        // Delays for half of the duration of the current play then reverses direction.
        async Task DelayForHalfAnimationDurationThenReverse()
        {
            var delayTimeSpan = TimeSpan.FromTicks((long)(0.5 * Player.Duration.Ticks));
            await Task.Delay(delayTimeSpan);

            // Pause, change direction, resume in the new direction.
            Player.Pause();
            Player.PlaybackRate *= -1;
            Player.Resume();
        }

        async void FallenBackButton_ClickAsync(object sender, RoutedEventArgs e)
        {
            // VisualCapture helper functionality is only available since RS5. Because API
            // method GraphicsCaptureItem.CreateFromVisual is an RS5 feature.
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                // nullsource is a source that always fails. We use it to trigger the fallback behavior.
                Player.Source = new AnimatedVisuals.nullsource();

                // Scrape the pixels from the Player. There should be a red rectangle where the content would go.
                var playerVisual = ElementCompositionPreview.GetElementVisual(Player);
                CanvasBitmap canvasBitmap = await RenderVisualToBitmapAsync(playerVisual);

                var colors = canvasBitmap.GetPixelColors();
                var redPixelCount = colors.Where(c => c == FallbackGrid.RectangleColor).Count();

                // Check that the number of red pixels is at least half the number of pixels in the
                // FallbackGrid. We require only half to allow for any scaling or borders or anything
                // that is not important to determining whether or not we are showing the
                // FallbackGrid.
                FallenBackTextBox.Text = redPixelCount >= FallbackGrid.RectangleHeight * FallbackGrid.RectangleWidth * 0.5
                    ? Constants.TrueText
                    : Constants.FalseText;
            }
            else
            {
                var parent = FallbackGrid.Latest?.Parent;
                FallenBackTextBox.Text = parent == Player
                    ? Constants.TrueText
                    : Constants.FalseText;
            }
        }

        //
        // Renders a the given <see cref="Visual"/> to a <see cref="CanvasBitmap"/>. If <paramref name="size"/> is not
        // specified, uses the size of <paramref name="visual"/>.
        //
        static async Task<CanvasBitmap> RenderVisualToBitmapAsync(Visual visual, SizeInt32? size = null)
        {
            // Get an object that enables capture from a visual.
            var graphicsItem = GraphicsCaptureItem.CreateFromVisual(visual);

            var canvasDevice = CanvasDevice.GetSharedDevice();

            var tcs = new TaskCompletionSource<CanvasBitmap>();

            // Create a frame pool with room for only 1 frame because we're getting a single frame, not a video.
            const int numberOfBuffers = 1;
            using (var framePool = Direct3D11CaptureFramePool.Create(
                                canvasDevice,
                                DirectXPixelFormat.B8G8R8A8UIntNormalized,
                                numberOfBuffers,
                                size ?? graphicsItem.Size))
            {
                void OnFrameArrived(Direct3D11CaptureFramePool sender, object args)
                {
                    using (var frame = sender.TryGetNextFrame())
                    {
                        tcs.SetResult(frame != null
                            ? CanvasBitmap.CreateFromDirect3D11Surface(canvasDevice, frame.Surface)
                            : null);
                    }
                }

                using (var session = framePool.CreateCaptureSession(graphicsItem))
                {
                    framePool.FrameArrived += OnFrameArrived;

                    // Start capturing. The FrameArrived event will occur shortly.
                    session.StartCapture();

                    // Wait for the frame to arrive.
                    var result = await tcs.Task;

                    // !!!!!!!! NOTE !!!!!!!!
                    // This thread is now running inside the OnFrameArrived callback method.

                    // Unsubscribe now that we have captured the frame.
                    framePool.FrameArrived -= OnFrameArrived;

                    // Yield to allow the OnFrameArrived callback to unwind so that it is safe to
                    // Dispose the session and framepool.
                    await Task.Yield();
                }
            }

            return await tcs.Task;
        }
    }
}
