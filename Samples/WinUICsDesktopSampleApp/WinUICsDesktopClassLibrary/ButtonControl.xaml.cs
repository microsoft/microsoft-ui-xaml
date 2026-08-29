// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace WinUICsDesktopClassLibrary
{
    public sealed partial class ButtonControl : UserControl
    {
        public ButtonControl()
        {
            this.InitializeComponent();
        }

        private void myButton_Click(object sender, RoutedEventArgs e)
        {
            TestTextBlock.Text = "Clicked";
        }

        public TextBlock NavigationPaneStatus()
        {
            return NavigationPaneStatusBlk;
        }
    }
}
