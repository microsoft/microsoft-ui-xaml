// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using Pager = Microsoft.UI.Xaml.Controls.Pager;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls;
using Windows.Devices.AllJoyn;
using System.Diagnostics;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Pager")]
    public sealed partial class PagerPage : TestPage
    {
        private ObservableCollection<string> EventHistory = new ObservableCollection<string>();
        public int LastPage = -1;
        public PagerPage()
        {
            this.InitializeComponent();
            PagerEventListViewDisplay.ItemsSource = EventHistory;
            this.Loaded += OnLoad;
        }

        private void OnLoad(object sender, RoutedEventArgs args)
        {
            PagerDisplayModeComboBox.SelectionChanged += OnDisplayModeChanged;
            FirstPageButtonVisibilityComboBox.SelectionChanged += OnFirstButtonVisibilityChanged;
            PreviousPageButtonVisibilityComboBox.SelectionChanged += OnPreviousButtonVisibilityChanged;
            NextPageButtonVisibilityComboBox.SelectionChanged += OnNextButtonVisibilityChanged;
            LastPageButtonVisibilityComboBox.SelectionChanged += OnLastButtonVisibilityChanged;

            NumberBoxVisibilityTextBlock.Text = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            ComboBoxVisibilityTextBlock.Text = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void OnPageChanged(PrototypePager sender, PageChangedEventArgs args)
        {
            EventHistory.Add($"Page changed from page {LastPage} to page {args.CurrentPage}");
            LastPage = args.CurrentPage;

            FirstPageButtonVisibilityTextBlock.Text = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            PreviousPageButtonVisibilityTextBlock.Text = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            NextPageButtonVisibilityTextBlock.Text = TestPager.NextPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            LastPageButtonVisibilityTextBlock.Text = TestPager.LastPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";

            FirstPageButtonIsEnabledCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.IsEnabled;
            PreviousPageButtonIsEnabledCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.IsEnabled;
            NextPageButtonIsEnabledCheckBox.IsChecked = TestPager.NextPageButtonTestHook.IsEnabled;
            LastPageButtonIsEnabledCheckBox.IsChecked = TestPager.LastPageButtonTestHook.IsEnabled;
        }

        private void ClearEventDisplay(object sender, RoutedEventArgs args)
        {
            EventHistory.Clear();
        }

        private void OnDisplayModeChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = PagerDisplayModeComboBox.SelectedItem;

            if (item == this.AutoDisplayModeItem)
            {
                TestPager.PagerDisplayMode = PrototypePager.PagerDisplayModes.Auto;
            }
            else if (item == this.NumberBoxDisplayModeItem)
            {
                TestPager.PagerDisplayMode = PrototypePager.PagerDisplayModes.NumberBox;
            }
            else if (item == this.ComboBoxDisplayModeItem)
            {
                TestPager.PagerDisplayMode = PrototypePager.PagerDisplayModes.ComboBox;
            }
            else if (item == this.NumberPanelDisplayModeItem)
            { 
                TestPager.PagerDisplayMode = PrototypePager.PagerDisplayModes.NumberPanel;
            }

            NumberBoxVisibilityTextBlock.Text = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            ComboBoxVisibilityTextBlock.Text = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
            NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void OnFirstButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = FirstPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.AutoFirstPageButtonVisibilityItem)
            {
                TestPager.FirstPageButtonVisibility = PrototypePager.ButtonVisibilityMode.Auto;
            }
            else if (item == this.NoneFirstPageButtonVisibilityItem)
            {
                TestPager.FirstPageButtonVisibility = PrototypePager.ButtonVisibilityMode.None;
            }
            else if (item == this.AlwaysVisibleFirstPageButtonVisibilityItem)
            {
                TestPager.FirstPageButtonVisibility = PrototypePager.ButtonVisibilityMode.AlwaysVisible;
            }
            else if (item == this.HiddenOnEdgeFirstPageButtonVisibilityItem)
            {
                TestPager.FirstPageButtonVisibility = PrototypePager.ButtonVisibilityMode.HiddenOnEdge;
            }

            FirstPageButtonVisibilityTextBlock.Text = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
        }

        private void OnPreviousButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = PreviousPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.AutoPreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousPageButtonVisibility = PrototypePager.ButtonVisibilityMode.Auto;
            }
            else if (item == this.NonePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousPageButtonVisibility = PrototypePager.ButtonVisibilityMode.None;
            }
            else if (item == this.AlwaysVisiblePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousPageButtonVisibility = PrototypePager.ButtonVisibilityMode.AlwaysVisible;
            }
            else if (item == this.HiddenOnEdgePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousPageButtonVisibility = PrototypePager.ButtonVisibilityMode.HiddenOnEdge;
            }

            PreviousPageButtonVisibilityTextBlock.Text = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
        }

        private void OnNextButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = NextPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.AutoNextPageButtonVisibilityItem)
            {
                TestPager.NextPageButtonVisibility = PrototypePager.ButtonVisibilityMode.Auto;
            }
            else if (item == this.NoneNextPageButtonVisibilityItem)
            {
                TestPager.NextPageButtonVisibility = PrototypePager.ButtonVisibilityMode.None;
            }
            else if (item == this.AlwaysVisibleNextPageButtonVisibilityItem)
            {
                TestPager.NextPageButtonVisibility = PrototypePager.ButtonVisibilityMode.AlwaysVisible;
            }
            else if (item == this.HiddenOnEdgeNextPageButtonVisibilityItem)
            {
                TestPager.NextPageButtonVisibility = PrototypePager.ButtonVisibilityMode.HiddenOnEdge;
            }

            NextPageButtonVisibilityTextBlock.Text = TestPager.NextPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
        }
        private void OnLastButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = LastPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.AutoLastPageButtonVisibilityItem)
            {
                TestPager.LastPageButtonVisibility = PrototypePager.ButtonVisibilityMode.Auto;
            }
            else if (item == this.NoneLastPageButtonVisibilityItem)
            {
                TestPager.LastPageButtonVisibility = PrototypePager.ButtonVisibilityMode.None;
            }
            else if (item == this.AlwaysVisibleLastPageButtonVisibilityItem)
            {
                TestPager.LastPageButtonVisibility = PrototypePager.ButtonVisibilityMode.AlwaysVisible;
            }
            else if (item == this.HiddenOnEdgeLastPageButtonVisibilityItem)
            {
                TestPager.LastPageButtonVisibility = PrototypePager.ButtonVisibilityMode.HiddenOnEdge;
            }

            LastPageButtonVisibilityTextBlock.Text = TestPager.LastPageButtonTestHook.Visibility == Visibility.Collapsed ? "Collapsed" : "Visible";
        }
    }
}