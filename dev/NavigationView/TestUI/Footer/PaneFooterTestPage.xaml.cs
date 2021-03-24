using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class PaneFooterTestPage : TestPage
    {
        public PaneFooterTestPage()
        {
            this.InitializeComponent();
        }

        private void FlipOrientation_Click(object sender, RoutedEventArgs e)
        {
            NavView.PaneDisplayMode = NavView.PaneDisplayMode == NavigationViewPaneDisplayMode.Top ? NavigationViewPaneDisplayMode.Auto : NavigationViewPaneDisplayMode.Top;
        }
    }
}
