using Microsoft.UI.Xaml;

namespace ChartAppCsPackaged
{
    public sealed partial class MainWindow : Window
    {
        public MainWindow()
        {
            this.InitializeComponent();

            var codeChart = new Microsoft.UI.Xaml.Controls.Charts.Chart();
            CodeChartHost.Children.Add(codeChart);

            StatusText.Text = $"Created markup chart: {MarkupChart != null}; code chart count: {CodeChartHost.Children.Count}.";
        }

        private void ToggleTheme_Click(object sender, RoutedEventArgs e)
        {
            RootGrid.RequestedTheme = RootGrid.RequestedTheme == ElementTheme.Dark
                ? ElementTheme.Light
                : ElementTheme.Dark;
        }
    }
}
