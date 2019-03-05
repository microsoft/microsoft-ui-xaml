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


#if !BUILD_WINDOWS
#endif

namespace MUXControlsTestApp
{
    public class FallbackGrid : Windows.UI.Xaml.Controls.Grid
    {
        public FallbackGrid()
        {
            isInstantiated = true;

            Windows.UI.Xaml.Shapes.Rectangle rect = new Windows.UI.Xaml.Shapes.Rectangle();
            rect.MinWidth = 100;
            rect.MinHeight = 100;
            rect.MaxWidth = 100;
            rect.MaxHeight = 100;
            rect.Fill = new SolidColorBrush(Colors.Red);
            Children.Add(rect);
        }

        static public bool isInstantiated = false;
    }

    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedVisualPlayerPage : TestPage
    {
        // Capture API objects.
        private SizeInt32 _lastSize;
        private GraphicsCaptureItem _item;
        private Direct3D11CaptureFramePool _framePool;
        private GraphicsCaptureSession _session;

        // Non-API related members.
        private Visual _visual;
        private CanvasDevice _canvasDevice;

        public AnimatedVisualPlayerPage()
        {
            this.InitializeComponent();
            Setup();
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

        private void FallenBackButton_Click(Object sender, RoutedEventArgs e)
        {
            // Because API method GraphicsCaptureItem.CreateFromVisual is only available since RS5.
            // We only test FallenBackContent RS5 onward for simplicity.
            if (IsRS5OrHigher())
            {
                Player.Source = new AnimatedVisuals.nullsource();
                StartCapture();
            }
            else
            {
                if (FallbackGrid.isInstantiated == true)
                {
                    FallenBackTextBox.Text = Constants.TrueText;
                }
                else
                {
                    FallenBackTextBox.Text = Constants.FalseText;
                }
            }
        }

        private bool IsRS5OrHigher()
        {
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7);
        }

        private void Setup()
        {
            _canvasDevice = new CanvasDevice();
            _visual = ElementCompositionPreview.GetElementVisual(Player);
        }

        public void StartCapture()
        {
            // GraphicsCaptureItem is RS4(1803) API. CreateFromVisual is RS5(1809) API method.
            var item = GraphicsCaptureItem.CreateFromVisual(_visual);
            if (item != null)
            {
                StartCaptureInternal(item);
            }
        }

        private void StartCaptureInternal(GraphicsCaptureItem item)
        {
            // Stop the previous capture if we had one.
            StopCapture();

            _item = item;
            _lastSize = _item.Size;

            _framePool = Direct3D11CaptureFramePool.Create(
               _canvasDevice, // D3D device 
               DirectXPixelFormat.B8G8R8A8UIntNormalized, // Pixel format 
               2, // Number of frames 
               _item.Size); // Size of the buffers 

            _framePool.FrameArrived += (s, a) =>
            {
                // The FrameArrived event is raised for every frame on the thread
                // that created the Direct3D11CaptureFramePool. This means we 
                // don't have to do a null-check here, as we know we're the only 
                // one dequeueing frames in our application.  

                // NOTE: Disposing the frame retires it and returns  
                // the buffer to the pool.

                using (var frame = _framePool.TryGetNextFrame())
                {
                    ProcessFrame(frame);
                }
            };

            _item.Closed += (s, a) =>
            {
                StopCapture();
            };

            _session = _framePool.CreateCaptureSession(_item);
            _session.StartCapture();
        }

        public void StopCapture()
        {
            _session?.Dispose();
            _framePool?.Dispose();
            _item = null;
            _session = null;
            _framePool = null;
        }

        private void ProcessFrame(Direct3D11CaptureFrame frame)
        {
            // Resize and device-lost leverage the same function on the
            // Direct3D11CaptureFramePool. Refactoring it this way avoids 
            // throwing in the catch block below (device creation could always 
            // fail) along with ensuring that resize completes successfully and 
            // isn’t vulnerable to device-lost.   
            bool needsReset = false;
            bool recreateDevice = false;

            if ((frame.ContentSize.Width != _lastSize.Width) ||
                (frame.ContentSize.Height != _lastSize.Height))
            {
                needsReset = true;
                _lastSize = frame.ContentSize;
            }

            try
            {
                // Take the D3D11 surface and draw it into a  
                // Composition surface.

                // Convert our D3D11 surface into a Win2D object.
                var canvasBitmap = CanvasBitmap.CreateFromDirect3D11Surface(
                    _canvasDevice,
                    frame.Surface);

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

            // This is the device-lost convention for Win2D.
            catch (Exception e) when (_canvasDevice.IsDeviceLost(e.HResult))
            {
                // We lost our graphics device. Recreate it and reset 
                // our Direct3D11CaptureFramePool.  
                needsReset = true;
                recreateDevice = true;
            }

            if (needsReset)
            {
                ResetFramePool(frame.ContentSize, recreateDevice);
            }
        }

        private void ResetFramePool(SizeInt32 size, bool recreateDevice)
        {
            do
            {
                try
                {
                    if (recreateDevice)
                    {
                        _canvasDevice = new CanvasDevice();
                    }

                    _framePool.Recreate(
                        _canvasDevice,
                        DirectXPixelFormat.B8G8R8A8UIntNormalized,
                        2,
                        size);
                }
                // This is the device-lost convention for Win2D.
                catch (Exception e) when (_canvasDevice.IsDeviceLost(e.HResult))
                {
                    _canvasDevice = null;
                    recreateDevice = true;
                }
            } while (_canvasDevice == null);
        }

    }
}
