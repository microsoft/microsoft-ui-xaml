// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public class SampleSuggestions
    {
        public ObservableCollection<string> Suggestions
        {
            get
            {
                return new ObservableCollection< string > { "Suggestion 1", "Suggestion 2", "Suggestion 3", "Suggestion 4" };
            }
        }
    }

    public sealed partial class RevealListPage : TestPage
    {
        public RevealListPage()
        {
            this.InitializeComponent();
        }

        public SemanticZoomTemplateItem[] DataTemplateList
        {
            get
            {
                return new SemanticZoomTemplateItem[]
                {
                    new SemanticZoomTemplateItem("Lists", "ListViewTest"),
                    new SemanticZoomTemplateItem("Multi Selection Lists", "MultiListViewTest"),
                    new SemanticZoomTemplateItem("Grid View", "GridViewTest"),
                    new SemanticZoomTemplateItem("Auto Suggestion Box", "AutoSuggestBoxTest"),
                    new SemanticZoomTemplateItem("Combo Box", "ComboBoxTest"),
                };
            }
        }

        private void OnBorderColorButtonClick(object sender, RoutedEventArgs e)
        {
            if (RootCanvas.Background is Windows.UI.Xaml.Media.ImageBrush)
            {
                if (RequestedTheme == ElementTheme.Dark)
                    RootCanvas.Background = new SolidColorBrush(Colors.Black);
                else
                    RootCanvas.Background = new SolidColorBrush(Colors.White);
            }
            else
            {
                RootCanvas.Background = Resources["BGImageBrush"] as Windows.UI.Xaml.Media.ImageBrush;
            }
        }

        private void OnBackgroundColorButtonClick(object sender, RoutedEventArgs e)
        {
            if (RequestedTheme == ElementTheme.Dark)
            {
                RequestedTheme = ElementTheme.Light;
                RootCanvas.Background = new SolidColorBrush(Colors.White);
            }
            else
            {
                RequestedTheme = ElementTheme.Dark;
                RootCanvas.Background = new SolidColorBrush(Colors.Black);
            }
        }
    }
}
