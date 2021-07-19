using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

namespace MUXControlsTestApp
{
    public sealed partial class ProgressBarReTemplatePage : TestPage
    {
        public ProgressBarReTemplatePage()
        {
            this.InitializeComponent();
            Loaded += ProgressBarReTemplatePage_Loaded;
        }

        private void ProgressBarReTemplatePage_Loaded(object sender, RoutedEventArgs e)
        {
            var layoutRoot = (Grid)VisualTreeHelper.GetChild(TestProgressBar, 0);

            VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentStateChanged += this.ProgressBarReTemplatePage_CurrentStateChanged;
            VisualStateText.Text = VisualStateManager.GetVisualStateGroups(layoutRoot)[0].CurrentState.Name;

            var progressBarRoot = VisualTreeHelper.GetChild(layoutRoot, 1);
            var clip = VisualTreeHelper.GetChild(progressBarRoot, 0);
            var indicator = VisualTreeHelper.GetChild(clip, 0) as Rectangle;

            indicator.SizeChanged += this.Indicator_SizeChanged;

            IndicatorWidthText.Text = indicator.ActualWidth.ToString();
            ROValueText.Text = TestProgressBar.Value.ToString();

            Loaded -= ProgressBarReTemplatePage_Loaded;
        }

        private void Indicator_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            IndicatorWidthText.Text = ((Rectangle)sender).ActualWidth.ToString();
        }

        private void ProgressBarReTemplatePage_CurrentStateChanged(object sender, VisualStateChangedEventArgs e)
        {
            VisualStateText.Text = e.NewState.Name;
        }

        public void UpdateMinMax_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Maximum = String.IsNullOrEmpty(MaximumInput.Text) ? Double.Parse(MaximumInput.PlaceholderText) : Double.Parse(MaximumInput.Text);
            TestProgressBar.Minimum = String.IsNullOrEmpty(MinimumInput.Text) ? Double.Parse(MinimumInput.PlaceholderText) : Double.Parse(MinimumInput.Text);
            ROValueText.Text = TestProgressBar.Value.ToString();
        }

        public void UpdateWidth_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Width = String.IsNullOrEmpty(WidthInput.Text) ? Double.Parse(WidthInput.PlaceholderText) : Double.Parse(WidthInput.Text);
        }

        public void UpdateValue_Click(object sender, RoutedEventArgs e)
        {
            TestProgressBar.Value = String.IsNullOrEmpty(ValueInput.Text) ? Double.Parse(ValueInput.PlaceholderText) : Double.Parse(ValueInput.Text);
            ROValueText.Text = TestProgressBar.Value.ToString();
        }

        public void ChangeValue_Click(object sender, RoutedEventArgs e)
        {
            if (TestProgressBar.Value + 1 > TestProgressBar.Maximum)
            {
                TestProgressBar.Value = (int)(TestProgressBar.Minimum + 0.5);
            }
            else
            {
                TestProgressBar.Value += 1;
            }
            ROValueText.Text = TestProgressBar.Value.ToString();
        }

        public void UpdatePadding_Click(object sender, RoutedEventArgs e)
        {
            double paddingLeft = String.IsNullOrEmpty(PaddingLeftInput.Text) ? Double.Parse(PaddingLeftInput.PlaceholderText) : Double.Parse(PaddingLeftInput.Text);
            double paddingRight = String.IsNullOrEmpty(PaddingRightInput.Text) ? Double.Parse(PaddingRightInput.PlaceholderText) : Double.Parse(PaddingRightInput.Text);

            TestProgressBar.Padding = new Thickness(paddingLeft, 0, paddingRight, 0);
            ROValueText.Text = TestProgressBar.Value.ToString();
        }
    }
}
