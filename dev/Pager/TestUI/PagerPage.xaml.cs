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

            NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
            NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void NumberOfPagesSetterButtonClicked(object sender, RoutedEventArgs args)
        {
            TestPager.NumberOfPages = 100;
        }
        private void OnPageChanged(PrototypePager sender, PageChangedEventArgs args)
        {
            EventHistory.Add($"Page changed from page {args.PreviousPage} to page {args.CurrentPage}");

            FirstPageButtonVisibilityCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Visible;
            PreviousPageButtonVisibilityCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Visible;
            NextPageButtonVisibilityCheckBox.IsChecked = TestPager.NextPageButtonTestHook.Visibility == Visibility.Visible;
            LastPageButtonVisibilityCheckBox.IsChecked = TestPager.LastPageButtonTestHook.Visibility == Visibility.Visible;

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

            NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
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

            FirstPageButtonVisibilityCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Visible;
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

            PreviousPageButtonVisibilityCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Visible;
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

            NextPageButtonVisibilityCheckBox.IsChecked = TestPager.NextPageButtonTestHook.Visibility == Visibility.Visible;
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

            LastPageButtonVisibilityCheckBox.IsChecked = TestPager.LastPageButtonTestHook.Visibility == Visibility.Visible;
        }
    }
}