// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

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
            var contentDialog = CreateContentDialog();
            contentDialog.XamlRoot = this.XamlRoot;
            _ = contentDialog.ShowAsync();
        }

        private void ShowBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog {
                Title = "Title",
                Content = "I am testing border thickness",
                IsPrimaryButtonEnabled = true,
                BorderThickness = new Thickness(10, 20, 10, 20),
                CloseButtonText = "CloseButton",
                BorderBrush = new SolidColorBrush(Color.FromArgb(255, 255, 0, 0)),
                XamlRoot = this.XamlRoot
            };
            _ = CreateContentDialog().ShowAsync();
        }

        private void ShowInPlaceDialog_Click(object sender, RoutedEventArgs e)
        {
            var contentDialog = CreateContentDialog();
            MainPanel.Children.Add(contentDialog);
            _ = contentDialog.ShowAsync();
        }

        private static ContentDialog CreateContentDialog()
        {
            return new ContentDialog
            {
                Title = "Title",
                Content = "Content",
                IsPrimaryButtonEnabled = true,
                PrimaryButtonText = "PrimaryButton",
                SecondaryButtonText = "SecondaryButton",
                CloseButtonText = "CloseButton"
            };
        }
    }
}
