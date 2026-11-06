// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;

namespace WinUICsDesktopSampleApp.WindowPlacement
{
    public sealed partial class MarkupWindow : Window
    {
        public MarkupWindow()
        {
            InitializeComponent();
        }

        private void OnClose(object sender, RoutedEventArgs e) => Close();
    }
}
