// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

            Button commandBarFlyoutButton = new Button() { Margin = new Thickness(10), Content = "CommandBarFlyout" };
            
            commandBarFlyoutButton.Click += delegate (object sender, RoutedEventArgs args)
            {
                Frame.Navigate(typeof(CommandBarFlyoutPage));
            };
            
            MainStackPanel.Children.Add(commandBarFlyoutButton);
        }
    }
}
