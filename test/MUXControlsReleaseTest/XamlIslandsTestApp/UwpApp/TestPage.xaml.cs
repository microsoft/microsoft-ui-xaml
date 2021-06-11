using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace UwpApp
{
    public sealed partial class TestPage : Page
    {
        public TestPage()
        {
            this.InitializeComponent();
        }

        private void OnTestButtonClick(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;
            button.Content = "Clicked!";
        }

        private void OnMenuFlyoutButtonClick(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;

            MenuFlyout.ShowAt(button, new FlyoutShowOptions() {
                Placement = FlyoutPlacementMode.BottomEdgeAlignedRight
            });
        }

        private void OnCommandBarFlyoutButtonClick(object sender, RoutedEventArgs e)
        {
            Button button = (Button)sender;

            CommandBarFlyout.ShowAt(button, new FlyoutShowOptions() {
                Placement = FlyoutPlacementMode.BottomEdgeAlignedLeft
            });
        }
    }
}
