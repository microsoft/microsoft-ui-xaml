using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class ProgressRingStoryboardAnimationPage : TestPage
    {
        public ProgressRingStoryboardAnimationPage()
        {
            this.InitializeComponent();
            Loaded += ProgressRingPage_Loaded;

            NavigateToMainPage.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ProgressRingPage), 0); };
        }

        private void ProgressRingPage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestStoryboardAnimationProgressRing, 0);

            var commonStatesGroup = VisualStateManager.GetVisualStateGroups(layoutRoot)[1];
            commonStatesGroup.CurrentStateChanged += this.ProgressRingPage_CurrentStateChanged;
            VisualStateText.Text = commonStatesGroup.CurrentState.Name;

            Loaded -= ProgressRingPage_Loaded;
        }

        private void ProgressRingPage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestStoryboardAnimationProgressRing.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
            TestStoryboardAnimationProgressRing.Height = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }
    }
}
