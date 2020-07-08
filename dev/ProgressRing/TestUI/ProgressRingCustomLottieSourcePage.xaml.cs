using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;

namespace MUXControlsTestApp
{
    public sealed partial class ProgressRingCustomLottieSourcePage : TestPage
    {
        public ProgressRingCustomLottieSourcePage()
        {
            this.InitializeComponent();
            Loaded += ProgressRingCustomLottieSourcePage_Loaded;

            NavigateToMainPage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ProgressRingPage), 0); };
        }

        private void ProgressRingCustomLottieSourcePage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestCustomLottieSourceProgressRing, 0);

            var commonStatesGroup = VisualStateManager.GetVisualStateGroups(layoutRoot)[0];
            commonStatesGroup.CurrentStateChanged += this.ProgressRingCustomLottieSourcePage_CurrentStateChanged;
            VisualStateText.Text = commonStatesGroup.CurrentState.Name;

            //for (int i = 0; i < commonStatesGroup.Transitions[0].Storyboard.Children.Count; i++)
            //{
            //    // Change the animation to 0 duration to avoid timing issues in the test.
            //    commonStatesGroup.Transitions[0].Storyboard.Children[i].Duration = new Duration(TimeSpan.FromSeconds(0));
            //}

            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);

            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();
            OpacityText.Text = layoutRoot.Opacity.ToString();

            Loaded -= ProgressRingCustomLottieSourcePage_Loaded;
        }

        private void ProgressRingCustomLottieSourcePage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;

            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestCustomLottieSourceProgressRing, 0);
            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);
            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();
            OpacityText.Text = layoutRoot.Opacity.ToString();
        }

        public void UpdateMinMax_Click(object sender, RoutedEventArgs e)
        {
            TestCustomLottieSourceProgressRing.Maximum = String.IsNullOrEmpty(MaximumInput.Text) ? Double.Parse(MaximumInput.PlaceholderText) : Double.Parse(MaximumInput.Text);
            TestCustomLottieSourceProgressRing.Minimum = String.IsNullOrEmpty(MinimumInput.Text) ? Double.Parse(MinimumInput.PlaceholderText) : Double.Parse(MinimumInput.Text);
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestCustomLottieSourceProgressRing.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestCustomLottieSourceProgressRing.Height = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }

        public void UpdateValue_Click(object sender, RoutedEventArgs e)
        {
            TestCustomLottieSourceProgressRing.Value = String.IsNullOrEmpty(ValueInput.Text) ? Double.Parse(ValueInput.PlaceholderText) : Double.Parse(ValueInput.Text);
        }
        public void ChangeValue_Click(object sender, RoutedEventArgs e)
        {
            if (TestCustomLottieSourceProgressRing.Value + 1 > TestCustomLottieSourceProgressRing.Maximum)
            {
                TestCustomLottieSourceProgressRing.Value = (int)(TestCustomLottieSourceProgressRing.Minimum + 0.5);
            }
            else
            {
                TestCustomLottieSourceProgressRing.Value += 1;
            }
        }
    }
}
