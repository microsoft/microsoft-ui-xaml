// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Windows.UI.ViewManagement;
using System.Collections.ObjectModel;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Dispatching;

namespace MUXControlsTestApp
{
    public class RevealItem
    {
        public string Header;
        public string SubHeader;
        public string Icon;
    }

    public sealed partial class RevealSimpleListPage : TestPage
    {
        bool accent;
        Color accentColor;
        UISettings settings = new UISettings();
        ObservableCollection<RevealItem> source = new ObservableCollection<RevealItem>();

        public RevealSimpleListPage()
        {
            this.InitializeComponent();

            source.Add(new RevealItem { Header = "System", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Devices", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Network & Internet", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Personalization", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Apps", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Accounts", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Time & Language", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Gaming", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Ease of Access", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Privacy", SubHeader = "Display, notifications, power", Icon = "\uE105" });
            source.Add(new RevealItem { Header = "Update & Security", SubHeader = "Display, notifications, power", Icon = "\uE105" });

            RevealList.ItemsSource = source;
            this.accent = false;

            settings.ColorValuesChanged += Settings_ColorValuesChanged;

            StyleChooser_SelectionChanged(null, null);
        }

        private void Settings_ColorValuesChanged(UISettings sender, object args)
        {
            var ignored = this.DispatcherQueue.TryEnqueue(DispatcherQueuePriority.Normal, OnColorValuesChanged);
        }

        private void OnColorValuesChanged()
        {
            this.accentColor = this.settings.UIElementColor(Windows.UI.ViewManagement.UIElementType.Highlight);
            if (this.accent)
            {
                RootCanvas.Background = new SolidColorBrush(this.accentColor);
            }
            else
            {
                if (RequestedTheme == ElementTheme.Dark)
                    RootCanvas.Background = new SolidColorBrush(Microsoft.UI.Colors.Black);
                else
                    RootCanvas.Background = new SolidColorBrush(Microsoft.UI.Colors.White);
            }
        }

        private void BorderColorButton_Click(object sender, RoutedEventArgs e)
        {
            if (this.accent)
            {
                this.accent = false;
            }
            else
            {
                this.accent = true;
            }
            OnColorValuesChanged();
        }

        private void BackgroundColorButton_Click(object sender, RoutedEventArgs e)
        {
            if (RequestedTheme == ElementTheme.Dark)
            {
                RequestedTheme = ElementTheme.Light;
            }
            else
            {
                RequestedTheme = ElementTheme.Dark;
            }
            OnColorValuesChanged();
        }

        private void StyleChooser_SelectionChanged(object sender, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            if (RevealList != null)
            {
                RevealList.ItemTemplate = (DataTemplate)Resources[(string)StyleChooser.SelectedItem];
                RevealList.ItemContainerStyle = (Style)Resources[(string)RevealStyleChooser.SelectedItem];
            }
        }
    }
}
