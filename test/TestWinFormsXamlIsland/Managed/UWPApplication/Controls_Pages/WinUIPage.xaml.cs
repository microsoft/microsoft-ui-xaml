using Windows.Foundation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

namespace ManagedUWP
{
    public sealed partial class WinUIPage : Page
    {
        public WinUIPage()
        {
            this.InitializeComponent();
            rootPanel.RightTapped += RootPanel_RightTapped;
        }

        private void RootPanel_RightTapped(object sender, Windows.UI.Xaml.Input.RightTappedRoutedEventArgs e)
        {
            Point position = e.GetPosition(rootPanel);
            position.X += 155;
            position.Y += 45;

            FlyoutShowOptions flyoutShowOptions = new FlyoutShowOptions();
            flyoutShowOptions.Placement = FlyoutPlacementMode.Auto;
            flyoutShowOptions.Position = position;

            desktopCommandBarFlyout.ShowAt(rootPanel, flyoutShowOptions); ;
        }
    }
}
