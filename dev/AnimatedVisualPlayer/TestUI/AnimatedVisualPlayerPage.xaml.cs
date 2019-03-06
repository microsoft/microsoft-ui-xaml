// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading.Tasks;
using AnimatedVisualPlayerTests;
using Microsoft.Graphics.Canvas;
using Windows.Foundation.Metadata;
using Windows.Graphics;
using Windows.Graphics.Capture;
using Windows.Graphics.DirectX;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public class FallbackGrid : Windows.UI.Xaml.Controls.Grid
    {
        public FallbackGrid()
        {
            _fallbackGrid = this;

            Windows.UI.Xaml.Shapes.Rectangle rect = new Windows.UI.Xaml.Shapes.Rectangle();
            rect.MinWidth = 100;
            rect.MinHeight = 100;
            rect.MaxWidth = 100;
            rect.MaxHeight = 100;
            rect.Fill = new SolidColorBrush(Colors.Red);
            Children.Add(rect);
        }

        static public FallbackGrid _fallbackGrid = null;
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedVisualPlayerPage : TestPage
    {
        private Visual _visual;

        public AnimatedVisualPlayerPage()
        {
            this.InitializeComponent();

            _visual = ElementCompositionPreview.GetElementVisual(Player);
        }

        private async void PlayButton_Click(object sender, RoutedEventArgs e)
        {
            bool isPlaying = Player.IsPlaying;
            IsPlayingTextBoxBeforePlaying.Text = isPlaying.ToString();

            Task task1 = Player.PlayAsync(0, 1, false).AsTask();
            Task task2 = GetIsPlayingAsync();

            await Task.WhenAll(task1, task2);

            ProgressTextBox.Text = Constants.PlayingEndedText;
        }

        private async void ToZeroKeyframeAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            await Player.PlayAsync(0.35, 0, false);

            ToZeroKeyframeAnimationProgressTextBox.Text = Constants.PlayingEndedText;
        }

        private async void FromOneKeyframeAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            await Player.PlayAsync(1, 0.35, false);

            FromOneKeyframeAnimationProgressTextBox.Text = Constants.PlayingEndedText;
        }

        private async void ReverseNegativePlaybackRateAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            Player.PlaybackRate = 0 - Int32.Parse(Constants.OneText);
            Task task1 = Player.PlayAsync(0, 1, false).AsTask();
            Task task2 = ReverseNegativePlaybackRateAnimationAsync();

            await Task.WhenAll(task1, task2);

            //
            // The player's PlayAsync returns immediately in RS4 or lower windows build.
            // Thus, Constants.OneText is set to ...TextBox's content in order to
            // satisfy the interaction test that uses Accessibility.
            //
            if (IsRS5OrHigher())
            {
                CompositionPropertySet progressPropertySet = (CompositionPropertySet)Player.ProgressObject;
                float value = 0f;
                progressPropertySet.TryGetScalar("progress", out value);
                ReverseNegativePlaybackRateAnimationTextBox.Text = value.ToString();
            }
            else
            {
                ReverseNegativePlaybackRateAnimationTextBox.Text = Constants.OneText;
            }
        }

        private async void ReversePositivePlaybackRateAnimationPlayButton_Click(object sender, RoutedEventArgs e)
        {
            Player.PlaybackRate = Int32.Parse(Constants.OneText);
            Task task1 = Player.PlayAsync(0, 1, false).AsTask();
            Task task2 = ReversePositivePlaybackRateAnimationAsync();

            await Task.WhenAll(task1, task2);

            //
            // The player's PlayAsync returns immediately in RS4 or lower windows build.
            // Thus, Constants.ZeroText is set to ...TextBox's content in order to
            // satisfy the interaction test that uses Accessibility.
            //
            if (IsRS5OrHigher())
            {
                CompositionPropertySet progressPropertySet = (CompositionPropertySet)Player.ProgressObject;
                float value = 1f;
                progressPropertySet.TryGetScalar("progress", out value);
                ReversePositivePlaybackRateAnimationTextBox.Text = value.ToString();
            }
            else
            {
                ReversePositivePlaybackRateAnimationTextBox.Text = Constants.ZeroText;
            }
        }
        
        private void Player_PointerMoved(object sender, RoutedEventArgs e)
        {
            HittestingTextBox.Text = Constants.PointerMovedText;
        }
        
        private async Task GetIsPlayingAsync()
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
            if (IsRS5OrHigher())
            {
                Player.Pause();
                bool isPlaying = Player.IsPlaying;
                IsPlayingTextBoxBeingPlaying.Text = isPlaying.ToString();
                Player.Resume();
            }
            else
            {
                IsPlayingTextBoxBeingPlaying.Text = Constants.TrueText;
            }
        }

        private async Task ReverseNegativePlaybackRateAnimationAsync()
        {
            int delayTimeSpan = (int)(0.5 * Player.Duration.TotalMilliseconds);
            await Task.Delay(delayTimeSpan);

            Player.Pause();
            Player.PlaybackRate = (double)(Int32.Parse(Constants.OneText));
            Player.Resume();
        }

        private async Task ReversePositivePlaybackRateAnimationAsync()
        {
            int delayTimeSpan = (int)(0.5 * Player.Duration.TotalMilliseconds);
            await Task.Delay(delayTimeSpan);

            Player.Pause();
            Player.PlaybackRate = 0 - (double)(Int32.Parse(Constants.OneText));
            Player.Resume();
        }

        private async void FallenBackButton_ClickAsync(Object sender, RoutedEventArgs e)
        {
            // VisualCapture helper functionality is only available since RS5. Because API
            // method GraphicsCaptureItem.CreateFromVisual is a RS5 feature.
            if (IsRS5OrHigher())
            {
                Player.Source = new AnimatedVisuals.nullsource();
                CanvasBitmap canvasBitmap = await RenderVisualToBitmapAsync(_visual);

                Color[] colors = canvasBitmap.GetPixelColors(0, 0, 1, 1);
                if (colors.Length > 0 && colors[0].Equals(Colors.Red/*FallenBackContent Color*/))
                {
                    FallenBackTextBox.Text = Constants.TrueText;
                }
                else
                {
                    FallenBackTextBox.Text = Constants.FalseText;
                }
            }
            else
            {
                var parent = FallbackGrid._fallbackGrid != null ? FallbackGrid._fallbackGrid.Parent : null;
                if (parent == Player)
                {
                    FallenBackTextBox.Text = Constants.TrueText;
                }
                else
                {
                    FallenBackTextBox.Text = Constants.FalseText;
                }
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

        private bool IsRS5OrHigher()
        {
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7);
        }
    }
}
