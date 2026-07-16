// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Composition.SystemBackdrops;
using Windows.Media;
using Windows.Media.Core;
using Windows.Media.Playback;
using Windows.Foundation;

using System;


namespace WinUICsDesktopSampleApp
{
    public sealed partial class MarkupWindow : Window
    {
        private ContentDialog dialog = null;
        private MediaPlayerElement _mediaPlayerElement2 = null;
        private bool m_hasEverHadHandlers = false;

        static MicaBackdrop s_sharedMicaBackdrop;
        static MicaBackdrop s_sharedMicaAltBackdrop;
        static DesktopAcrylicBackdrop s_sharedDesktopAcrylicBackdrop;

        public MarkupWindow()
        {
            if (s_sharedMicaBackdrop == null)
            {
                s_sharedMicaBackdrop = new MicaBackdrop();
                s_sharedMicaAltBackdrop = new MicaBackdrop() { Kind = MicaKind.BaseAlt };
                s_sharedDesktopAcrylicBackdrop = new DesktopAcrylicBackdrop();
            }
            this.SystemBackdrop = s_sharedMicaBackdrop;
            this.InitializeComponent();
            this.Title = "WinUI Desktop - Markup Window";
        }

        private void ButtonCloseWindow_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }

        private async void OnOpenContentDialogButtonClick(object sender, RoutedEventArgs e)
        {
            dialog = new ContentDialog()
            {
                Title = "Example Content Dialog",
                Content = "Sample Content",
                CloseButtonText = "Ok"
            };

            dialog.Closed += ((_, _) =>
            {
                textBlockContentDialogStatus.Text = "Closed";
            });
            dialog.Opened += ((_, _) =>
            {
                textBlockContentDialogStatus.Text = "Opened";
            });

            dialog.XamlRoot = this.Content.XamlRoot;
            await dialog.ShowAsync();
        }

        private void OnCloseContentDialogButtonClick(object sender, RoutedEventArgs e)
        {
            if (dialog != null)
            {
                dialog.Hide();
            }
        }

        private void OnClickToggleMPE(object sender, RoutedEventArgs e)
        {
            mpeContainerStackPanel.Visibility = (bool)(sender as ToggleButton).IsChecked ? Visibility.Visible : Visibility.Collapsed;
        }

        private void SetMedia(MediaPlayerElement mediaPlayerElement, string filePath)
        {
            if (!String.IsNullOrEmpty(filePath))
            {
                mediaPlayerElement.Source = MediaSource.CreateFromUri(new Uri(filePath));
            }
        }

        private void changeStretch(MediaPlayerElement mediaPlayerElement, string stretchMode)
        {
            switch (stretchMode)
            {
                case "None":
                    mediaPlayerElement.Stretch = Stretch.None;
                    break;
                case "Fill":
                    mediaPlayerElement.Stretch = Stretch.Fill;
                    break;
                case "Uniform":
                    mediaPlayerElement.Stretch = Stretch.Uniform;
                    break;
                case "UniformToFill":
                    mediaPlayerElement.Stretch = Stretch.UniformToFill;
                    break;
            }
        }

        private void ChangeTransform(FrameworkElement element, string renderTransform)
        {
            switch (renderTransform)
            {
                case "None":
                    element.RenderTransform = null;
                    break;
                case "RotateTransform":
                    element.RenderTransform = new RotateTransform() { Angle = 45 };
                    break;
                case "ScaleTransform":
                    element.RenderTransform = new ScaleTransform() { ScaleX = 2, ScaleY = 2 };
                    break;
                case "SkewTransform":
                    element.RenderTransform = new SkewTransform() { AngleX = 30, AngleY = 0 };
                    break;
                case "TranslateTransform":
                    element.RenderTransform = new TranslateTransform() { X = 50, Y = 80 };
                    break;
            }
        }

        private void ChangeDimensions(FrameworkElement element, RangeBaseValueChangedEventArgs e)
        {
            if (element != null)
            {
                double delta = (double)e.NewValue - (double)e.OldValue;
                element.Height = element.ActualHeight + delta;
                element.Width = element.ActualWidth + delta;
            }
        }

        private void buttonLoadVideo1_Click(object sender, RoutedEventArgs e)
        {
            if (MewMediaPlayerCheckBox.IsChecked == true || !m_hasEverHadHandlers)
            {
                if (mediaPlayerElement1.MediaPlayer != null)
                {
                    MediaPlayer oldPlayer = mediaPlayerElement1.MediaPlayer;
                    oldPlayer.MediaOpened -= MediaPlayer_MediaOpened;
                    oldPlayer.MediaEnded -= MediaPlayer_MediaEnded;
                    oldPlayer.MediaFailed -= MediaPlayer_MediaFailed;
                    oldPlayer.BufferingStarted -= MediaPlayer_BufferingStarted;
                    oldPlayer.BufferingEnded -= MediaPlayer_BufferingEnded;
                    oldPlayer.CurrentStateChanged -= MediaPlayer_CurrentStateChanged;

                    oldPlayer.PlaybackSession.PlaybackStateChanged -= PlaybackSession_PlaybackStateChanged;
                    oldPlayer.PlaybackSession.BufferingProgressChanged  -= PlaybackSession_BufferingProgressChanged;
                    oldPlayer.PlaybackSession.BufferingStarted -= PlaybackSession_BufferingStarted;
                    oldPlayer.PlaybackSession.BufferingEnded -= PlaybackSession_BufferingEnded;
                    oldPlayer.PlaybackSession.DownloadProgressChanged -= PlaybackSession_DownloadProgressChanged;
                    oldPlayer.PlaybackSession.NaturalDurationChanged -= PlaybackSession_NaturalDurationChanged;
                    oldPlayer.PlaybackSession.NaturalVideoSizeChanged -= PlaybackSession_NaturalVideoSizeChanged;

                }

                MediaPlayer newPlayer = new MediaPlayer();

                mediaPlayerElement1.SetMediaPlayer(newPlayer);
                newPlayer.MediaOpened += MediaPlayer_MediaOpened;
                newPlayer.MediaEnded += MediaPlayer_MediaEnded;
                newPlayer.MediaFailed += MediaPlayer_MediaFailed;
                newPlayer.BufferingStarted += MediaPlayer_BufferingStarted;
                newPlayer.BufferingEnded += MediaPlayer_BufferingEnded;
                newPlayer.CurrentStateChanged += MediaPlayer_CurrentStateChanged;

                newPlayer.PlaybackSession.PlaybackStateChanged += PlaybackSession_PlaybackStateChanged;
                newPlayer.PlaybackSession.BufferingProgressChanged += PlaybackSession_BufferingProgressChanged;
                newPlayer.PlaybackSession.BufferingStarted += PlaybackSession_BufferingStarted;
                newPlayer.PlaybackSession.BufferingEnded += PlaybackSession_BufferingEnded;
                newPlayer.PlaybackSession.DownloadProgressChanged += PlaybackSession_DownloadProgressChanged;
                newPlayer.PlaybackSession.NaturalDurationChanged += PlaybackSession_NaturalDurationChanged;
                newPlayer.PlaybackSession.NaturalVideoSizeChanged += PlaybackSession_NaturalVideoSizeChanged;

                m_hasEverHadHandlers = true;
            }

            if (!String.IsNullOrEmpty(textBoxInputSource1.Text))
            {
                mediaPlayerElement1.MediaPlayer.Source = MediaSource.CreateFromUri(new Uri(textBoxInputSource1.Text));
            }
            // SetMedia(mediaPlayerElement1, textBoxInputSource1.Text);
        }

        private void MediaPlayer_MediaOpened(MediaPlayer sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerLog_TextBox.Text += "MediaOpened, ";
            }));
        }

        private void MediaPlayer_BufferingEnded(MediaPlayer sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerLog_TextBox.Text += "BufferingEnded, ";
            }));
        }

        private void MediaPlayer_BufferingStarted(MediaPlayer sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerLog_TextBox.Text += "BufferingStarted, ";
            }));
        }

        private void MediaPlayer_MediaFailed(MediaPlayer sender, MediaPlayerFailedEventArgs args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerLog_TextBox.Text += "MediaFailed [" + args.ExtendedErrorCode.ToString() + "],";
            }));
        }

        private void MediaPlayer_MediaEnded(MediaPlayer sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerLog_TextBox.Text += "MediaEnded, ";
            }));
        }

        private void MediaPlayer_CurrentStateChanged(MediaPlayer sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlayerState_TextBox.Text = sender.CurrentState.ToString();
            }));
        }

        private void PlaybackSession_PlaybackStateChanged(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                try
                {
                    MediaPlaybackSessionState_TextBox.Text =
                        sender.PlaybackState.ToString() +
                        //"/BP=" + String.Format("{0:F6}", (double) sender.BufferingProgress) +
                        //"/DP=" + String.Format("{0:F6}", (double) sender.DownloadProgress);
                        //"/BP=" + Convert.ToString(sender.BufferingProgress) +
                        //"/DP=" + Convert.ToString(sender.DownloadProgress);
                        "/BP=" + Math.Floor(sender.BufferingProgress * 100).ToString() + "%" +
                        "/DP=" + Math.Floor(sender.DownloadProgress * 100).ToString() + "%";
                }
                catch (Exception)
                {
                    MediaPlaybackSessionState_TextBox.Text =
                        sender.PlaybackState.ToString() +
                        "/BP=??%" +
                        "/DP=??%";
                }
            }));
        }

        private void PlaybackSession_BufferingStarted(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                var typedArgs = args as MediaPlaybackSessionBufferingStartedEventArgs;
                MediaPlaybackSessionLog_TextBox.Text += "BufferingStarted [IsPlaybackInterruption:" + typedArgs.IsPlaybackInterruption + "], ";
            }));
        }

        private void PlaybackSession_BufferingEnded(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlaybackSessionLog_TextBox.Text += "BufferingEnded, ";
            }));
        }

        private void PlaybackSession_BufferingProgressChanged(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlaybackSessionLog_TextBox.Text += "BPChanged (" + Math.Floor(sender.BufferingProgress * 100).ToString() + "%)";
            }));
        }

        private void PlaybackSession_DownloadProgressChanged(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlaybackSessionLog_TextBox.Text += "DPChanged (" + Math.Floor(sender.DownloadProgress * 100).ToString() + "%)";
            }));
        }

        private void PlaybackSession_NaturalDurationChanged(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlaybackSessionLog_TextBox.Text += "DurationChanged (" + sender.NaturalDuration.ToString() + "), ";
            }));
        }

        private void PlaybackSession_NaturalVideoSizeChanged(MediaPlaybackSession sender, object args)
        {
            this.DispatcherQueue.TryEnqueue(new DispatcherQueueHandler(() =>
            {
                MediaPlaybackSessionLog_TextBox.Text += "NaturalSizeChanged (" + sender.NaturalVideoWidth + "," + sender.NaturalVideoHeight + "), ";
            }));
        }

        private void buttonLoadPosterSource1_Click(object sender, RoutedEventArgs e)
        {
            String stringPosterSource1 = textBoxPosterSource1.Text;
            if (String.IsNullOrEmpty(stringPosterSource1))
            {
                mediaPlayerElement1.PosterSource = null;
            }
            else
            {
                mediaPlayerElement1.PosterSource = new BitmapImage(new Uri(stringPosterSource1, UriKind.Absolute));
            }
        }

        private void buttonUpdateRasterizationScale_Click(object sender, RoutedEventArgs e)
        {
            mpe1StackPanel.RasterizationScale = Convert.ToDouble(textBox_SP_RasterizationScale.Text as string);
            mediaPlayerElement1.RasterizationScale = Convert.ToDouble(textBox_MPE_RasterizationScale.Text as string);
        }

        private void AddRemoveZoomButton_Click(object sender, RoutedEventArgs e)
        {
            mediaPlayerElement1.TransportControls.IsZoomEnabled = !mediaPlayerElement1.TransportControls.IsZoomEnabled;
        }

        private void UpdateZoomVisibility_Click(object sender, RoutedEventArgs e)
        {
            mediaPlayerElement1.TransportControls.IsZoomButtonVisible = !mediaPlayerElement1.TransportControls.IsZoomButtonVisible;
        }

        private void AddSecondMPE_Click(object sender, RoutedEventArgs e)
        {
            string xamlReaderString =
                @"<MediaPlayerElement xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        x:Name='mediaPlayerElement2'
                        AutoPlay='False'
                        Height='300'
                        Width='400'
                        AreTransportControlsEnabled='True'
                        Source='ms-appx:///media/fishes.wmv'
                        >
                        <MediaPlayerElement.TransportControls>
                            <MediaTransportControls
                                IsCompact='False'
                                IsFastForwardButtonVisible='True'
                                IsFastForwardEnabled='True'
                                IsFastRewindButtonVisible='True'
                                IsFastRewindEnabled='True'
                                IsPreviousTrackButtonVisible='True'
                                IsNextTrackButtonVisible='True'
                                IsPlaybackRateButtonVisible='True'
                                IsPlaybackRateEnabled='True'
                                IsRepeatButtonVisible='True'
                                IsRepeatEnabled='True'
                                IsSeekBarVisible='True'
                                IsSeekEnabled='True'
                                IsSkipBackwardButtonVisible='True'
                                IsSkipBackwardEnabled ='True'
                                IsSkipForwardButtonVisible='True'
                                IsSkipForwardEnabled ='True'
                                IsStopButtonVisible='True'
                                IsStopEnabled='True'
                                IsVolumeButtonVisible='True'
                                IsVolumeEnabled='True'
                                ShowAndHideAutomatically='True'/>
                            </MediaPlayerElement.TransportControls>
                    </MediaPlayerElement>";


            var newMPE = (MediaPlayerElement)XamlReader.Load(xamlReaderString);
            MediaPlayerElement currentMPE = mpe2Border.Child as MediaPlayerElement;
            if (currentMPE != null)
            {
                currentMPE.Source = null;
                currentMPE.MediaPlayer.Dispose();
            }

            mpe2Border.Child = newMPE;
            _mediaPlayerElement2 = newMPE;

            MediaContainer2.Visibility = Visibility.Visible;
        }

        private void buttonLoadVideo2_Click(object sender, RoutedEventArgs e)
        {
            SetMedia(_mediaPlayerElement2, textBoxInputSource2.Text);
        }

        private void buttonLoadNullVideo1_Click(object sender, RoutedEventArgs e)
        {
            mediaPlayerElement1.Source = null;
        }

        private void buttonLoadNullVideo2_Click(object sender, RoutedEventArgs e)
        {
            _mediaPlayerElement2.Source = null;
        }

        private void Mpe1StackPanelSizeSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            ChangeDimensions(mpe1StackPanel, e);
        }

        private void Mpe1SizeSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            ChangeDimensions(mediaPlayerElement1, e);
        }

        private void Mpe2SizeSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            ChangeDimensions(_mediaPlayerElement2, e);
        }

        private void StretchButton1_Checked(object sender, RoutedEventArgs e)
        {
            var stretchMode = (sender as RadioButton).Content.ToString();
            changeStretch(mediaPlayerElement1, stretchMode);
        }

        private void StretchButton2_Checked(object sender, RoutedEventArgs e)
        {
            var stretchMode = (sender as RadioButton).Content.ToString();
            changeStretch(_mediaPlayerElement2, stretchMode);
        }

        private void TransformButton1_Checked(object sender, RoutedEventArgs e)
        {
            var transform = (sender as RadioButton).Content.ToString();
            ChangeTransform(mpe1StackPanel, transform);
        }

        private void TransformButton2_Checked(object sender, RoutedEventArgs e)
        {
            var transform = (sender as RadioButton).Content.ToString();
            ChangeTransform(_mediaPlayerElement2, transform);
        }

        private void ToggleClip1_Checked(object sender, RoutedEventArgs e)
        {
            mpe1StackPanel.Clip = (bool)(sender as ToggleButton).IsChecked ? new RectangleGeometry { Rect = new Rect(0, 0, 200, 200) } : null;
        }

        private void ToggleClip2_Checked(object sender, RoutedEventArgs e)
        {
            _mediaPlayerElement2.Clip = (bool)(sender as ToggleButton).IsChecked ? new RectangleGeometry { Rect = new Rect(0, 0, 200, 200) } : null;
        }

        private async void OpenMPEPopupClicked(object sender, RoutedEventArgs e)
        {
            MediaPlayerElement mediaPlayerElement = new MediaPlayerElement()
            {
                AreTransportControlsEnabled = true,
                AutoPlay = true,
            };

            var sourcePath = textBoxInputSource1.Text;
            if (!String.IsNullOrEmpty(sourcePath))
            {
                mediaPlayerElement.Source = MediaSource.CreateFromUri(new Uri(sourcePath));
            }

            // Closest thing to a windowed popup
            dialog = new ContentDialog()
            {
                Title = "MPE in Content Dialog",
                Content = mediaPlayerElement,
                CloseButtonText = "Close"
            };

            dialog.XamlRoot = this.Content.XamlRoot;
            await dialog.ShowAsync();
        }

        private void SystemBackdropRadioButtons_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (sender is RadioButtons rb)
            {
                string selection = rb.SelectedItem as string;
                switch (selection)
                {
                    case "Mica":
                        this.SystemBackdrop = s_sharedMicaBackdrop;
                        break;
                    case "Mica(Alt)":
                        this.SystemBackdrop = s_sharedMicaAltBackdrop;
                        break;
                    case "DesktopAcrylic":
                        this.SystemBackdrop = s_sharedDesktopAcrylicBackdrop;
                        break;
                    case "None":
                        this.SystemBackdrop = null;
                        break;
                }
            }
        }
    }
}
