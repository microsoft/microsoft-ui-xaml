// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

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
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton" };
            _ = dialog.ShowAsync();
        }
    }
}
