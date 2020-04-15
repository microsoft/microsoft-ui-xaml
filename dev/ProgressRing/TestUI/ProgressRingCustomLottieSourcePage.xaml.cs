using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class ProgressRingCustomLottieSourcePage : TestPage
    {
        public ProgressRingCustomLottieSourcePage()
        {
            this.InitializeComponent();
            Loaded += ProgressRingPage_Loaded;

            NavigateToMainPage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ProgressRingPage), 0); };
        }

        private void ProgressRingPage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestCustomLottieSourceProgressRing, 0);

            var commonStatesGroup = VisualStateManager.GetVisualStateGroups(layoutRoot)[0];
            commonStatesGroup.CurrentStateChanged += this.ProgressRingPage_CurrentStateChanged;
            VisualStateText.Text = commonStatesGroup.CurrentState.Name;
            foreach (var state in commonStatesGroup.States)
            {
                // Change the animation to 0 duration to avoid timing issues in the test.
                state.Storyboard.Children[0].Duration = new Duration(TimeSpan.FromSeconds(0));
            }

            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);

            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();
            OpacityText.Text = layoutRoot.Opacity.ToString();

            Loaded -= ProgressRingPage_Loaded;
        }

        private void ProgressRingPage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;

            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestCustomLottieSourceProgressRing, 0);
            var animatedVisualPlayer = (Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer)VisualTreeHelper.GetChild(layoutRoot, 0);
            IsPlayingText.Text = animatedVisualPlayer.IsPlaying.ToString();
            OpacityText.Text = layoutRoot.Opacity.ToString();
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestCustomLottieSourceProgressRing.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestCustomLottieSourceProgressRing.Height = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }
    }
}
