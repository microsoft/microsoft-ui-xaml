// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
namespace MUXControlsAdhocApp
{
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();

            repeater.Click += delegate (object sender, RoutedEventArgs args)
            {
                Frame.Navigate(typeof(RepeaterPages.PerfComparisonPage));
            };

#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
            Button commandBarFlyoutButton = new Button() { Margin = new Thickness(10), Content = "CommandBarFlyout" };
            
            commandBarFlyoutButton.Click += delegate (object sender, RoutedEventArgs args)
            {
                Frame.Navigate(typeof(CommandBarFlyoutPage));
            };
            
            MainStackPanel.Children.Add(commandBarFlyoutButton);
#endif
        }

        private void RichEditBox_SelectionChanged(object sender, RoutedEventArgs e)
        {
            var selection = richEditBox.Document.Selection;
            var bold = selection.CharacterFormat.Bold;
            var italic = selection.CharacterFormat.Italic;
            var underline = selection.CharacterFormat.Underline;
            var length = selection.Length;
            Log("bold={0}, italic={1}, underline={2}, length={3}", bold, italic, underline, length);
        }

        private void Log(string msg, params object[] args)
        {
            var msg2 = string.Format(msg, args);
            Debug.WriteLine(msg2);
            tb1.Text = msg2;
        }
    }
}
