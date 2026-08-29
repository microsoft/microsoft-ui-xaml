// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

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
                RootCanvas.Background = new SolidColorBrush(Microsoft.UI.Colors.White);
            }
            else
            {
                RequestedTheme = ElementTheme.Dark;
                RootCanvas.Background = new SolidColorBrush(Microsoft.UI.Colors.Black);
            }
        }

        private void OnChangeDefaultPositionClick(object sender, RoutedEventArgs e)
        {
            if (TopCmdBar.DefaultLabelPosition == Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom)
            {
                TopCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed;
                BottomCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed;
            }
            else if (TopCmdBar.DefaultLabelPosition == Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Collapsed)
            {
                TopCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Right;
                BottomCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Right;
            }
            else
            {
                TopCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom;
                BottomCmdBar.DefaultLabelPosition = Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition.Bottom;
            }
        }
    }
}
