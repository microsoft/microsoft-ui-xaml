// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "CornerRadius")]
    public sealed partial class CornerRadiusPage : TestPage
    {
        public ObservableCollection<string> AutoSuggestSource { get; private set; } = new ObservableCollection<string>();

        public CornerRadiusPage()
        {
            AutoSuggestSource.Add("Item 1");
            AutoSuggestSource.Add("Item 2");
            AutoSuggestSource.Add("Item 3");

            this.InitializeComponent();
        }

        private void ShowDialog_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton" };
            var result = dialog.ShowAsync();
        }


        private void ShowRoundedDialog_Click(object sender, RoutedEventArgs e)
        {
            var dialog = new ContentDialog { Title = "Title", Content = "Content", IsPrimaryButtonEnabled = true, PrimaryButtonText = "PrimaryButton", CornerRadius = new CornerRadius(10.0) };
            var result = dialog.ShowAsync();
        }
    }
}
