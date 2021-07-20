// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ContentDialog")]
    public sealed partial class ContentDialogPage : TestPage
    {
        public ContentDialogPage()
        {
            this.InitializeComponent();
        }

        private void ShowDialog_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton", SecondaryButtonText = "SecondaryButton", CloseButtonText = "CloseButton" };
            _ = dialog.ShowAsync();
        }

        private void ShowBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog {
                Title = "Title",
                Content = "I am testing border thickness",
                IsPrimaryButtonEnabled = true,
                BorderThickness = new Thickness(10, 20, 10, 20),
                CloseButtonText = "CloseButton",
                BorderBrush = new SolidColorBrush(Color.FromArgb(255, 255, 0, 0))
            };
            _ = dialog.ShowAsync();
        }
    }
}
