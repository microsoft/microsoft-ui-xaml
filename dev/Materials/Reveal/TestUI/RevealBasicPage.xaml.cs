// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

using System.Linq;

namespace MUXControlsTestApp
{
    public sealed partial class RevealBasicPage : TestPage
    {
        public RevealBasicPage()
        {
            this.InitializeComponent();
        }

        private void OnBackgroundColorButtonClick(object sender, RoutedEventArgs e)
        {
            if (RequestedTheme == ElementTheme.Dark)
            {
                RequestedTheme = ElementTheme.Light;
                RootCanvas.Background = new SolidColorBrush(Colors.White);
            }
            else
            {
                RequestedTheme = ElementTheme.Dark;
                RootCanvas.Background = new SolidColorBrush(Colors.Black);
            }
        }

        private void OnChangeDefaultPositionClick(object sender, RoutedEventArgs e)
        {
            if (TopCmdBar.DefaultLabelPosition == Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom)
            {
                TopCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed;
                BottomCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed;
            }
            else if (TopCmdBar.DefaultLabelPosition == Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed)
            {
                TopCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Right;
                BottomCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Right;
            }
            else
            {
                TopCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom;
                BottomCmdBar.DefaultLabelPosition = Windows.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom;
            }
        }
    }
}
