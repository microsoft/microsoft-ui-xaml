// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "AutoSuggestBox", Icon = "AutoSuggestBox.png")]
    [AxeScanTestPage(Name = "AutoSuggestBox-Axe")]
    public sealed partial class AutoSuggestBoxPage : TestPage
    {
        string[] suggestions =
        {
            "Lorem",
            "ipsum",
            "dolor",
            "sit",
            "amet"
        };

        public AutoSuggestBoxPage()
        {
            this.InitializeComponent();
        }

        private void AutoSuggestBox_QuerySubmitted(Windows.UI.Xaml.Controls.AutoSuggestBox sender, Windows.UI.Xaml.Controls.AutoSuggestBoxQuerySubmittedEventArgs args)
        {
            sender.ItemsSource = string.IsNullOrWhiteSpace(args.QueryText) ? null : suggestions;
        }
    }
}
