// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.UI.Composition;
using AnimatedVisualPlayerTests;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;


#if !BUILD_WINDOWS
using AnimatedVisualPlayer = Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer;
#endif

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class AnimatedVisualPlayerPage : TestPage
    {
        public AnimatedVisualPlayerPage()
        {
            this.InitializeComponent();

            ToLeakTestPageButton.Click += delegate {
                AnimatedVisualPlayer_TestUI.LeakTestObjects.PlayerWeakRef = new WeakReference<AnimatedVisualPlayer>(Player);
                this.Frame.NavigateWithoutAnimation(typeof(AnimatedVisualPlayerLeakTestPage), null);
            };

            for (var i = 0; i < 3; i++)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }            
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

        private void LeakTestCheckButton_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            object target = AnimatedVisualPlayer_TestUI.LeakTestObjects.PlayerWeakRef;
            if (target != null)
            {
                LeakTestCheckTextBox.Text = Constants.NoLeakText;
            }
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

        private bool IsRS5OrHigher()
        {
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", 7);
        }
    }
}
