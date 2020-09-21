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
using System.Collections.Specialized;
using Windows.Foundation;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "PagerControl")]
    public sealed partial class PagerControlPage : TestPage
    {

        public PagerControlPage()
        {
            this.InitializeComponent();
            this.Loaded += OnLoad;
        }

        private void OnLoad(object sender, RoutedEventArgs args)
        {
            PagerDisplayModeComboBox.SelectionChanged += OnDisplayModeChanged;
            FirstPageButtonVisibilityComboBox.SelectionChanged += OnFirstButtonVisibilityChanged;
            PreviousPageButtonVisibilityComboBox.SelectionChanged += OnPreviousButtonVisibilityChanged;
            NextPageButtonVisibilityComboBox.SelectionChanged += OnNextButtonVisibilityChanged;
            LastPageButtonVisibilityComboBox.SelectionChanged += OnLastButtonVisibilityChanged;

            //TestPager.NumberPanelDisplayTestHook.ElementPrepared += OnElementPrepared;


            //NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            //ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
            //NumberPanelVisibilityCheckBox.IsChecked = TestPager.NumberPanelDisplayTestHook.Visibility == Visibility.Visible;
            //NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            //ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
            UpdateNumberPanelContentTextBlock(this, null);
        }

        private void OnElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            var element = args.Element as FrameworkElement;
            if (element.Tag != null)
            {
                element.Name = "Page Button " + element.Tag;
            }
        }

        private void UpdateNumberPanelContentTextBlock(object sender, NotifyCollectionChangedEventArgs args)
        {
            NumberPanelContentTextBlock.Text = "";
            //foreach (var item in TestPager.NumberPanelDisplayTestHook.ItemsSource as ObservableCollection<object>)
            //{
            //    if (item.GetType() == typeof(SymbolIcon))
            //    {
            //        NumberPanelContentTextBlock.Text += (item as SymbolIcon).Symbol;
            //    }
            //    else
            //    {
            //        NumberPanelContentTextBlock.Text += item;
            //    }
            //}
        }

        private void NumberOfPagesSetterButtonClicked(object sender, RoutedEventArgs args)
        {
            if (TestPager.NumberOfPages == 5)
            {
                TestPager.NumberOfPages = 100;
                NumberOfPagesSetterButton.Content = "Set NumberOfPages to 5";
            }
            else
            {
                TestPager.NumberOfPages = 5;
                NumberOfPagesSetterButton.Content = "Set NumberOfPages to 100";
            }

            //NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            //ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
            //NumberPanelVisibilityCheckBox.IsChecked = TestPager.NumberPanelDisplayTestHook.Visibility == Visibility.Visible;
            //NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            //ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void IncreaseNumberOfPagesButtonClicked(object sender, RoutedEventArgs args)
        {
            TestPager.NumberOfPages += 1;

            //NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            //ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
            //NumberPanelVisibilityCheckBox.IsChecked = TestPager.NumberPanelDisplayTestHook.Visibility == Visibility.Visible;
            //NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            //ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void OnSelectedIndexChanged(PagerControl sender, PagerControlSelectedIndexChangedEventArgs args)
        {
            UpdateNumberPanelContentTextBlock(this, null);
            PreviousPageTextBlock.Text = args.PreviousPageIndex.ToString();
            CurrentPageTextBlock.Text = args.NewPageIndex.ToString();

            //FirstPageButtonVisibilityCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Visible;
            //PreviousPageButtonVisibilityCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Visible;
            //NextPageButtonVisibilityCheckBox.IsChecked = TestPager.NextPageButtonTestHook.Visibility == Visibility.Visible;
            //LastPageButtonVisibilityCheckBox.IsChecked = TestPager.LastPageButtonTestHook.Visibility == Visibility.Visible;

            //FirstPageButtonIsEnabledCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.IsEnabled;
            //PreviousPageButtonIsEnabledCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.IsEnabled;
            //NextPageButtonIsEnabledCheckBox.IsChecked = TestPager.NextPageButtonTestHook.IsEnabled;
            //LastPageButtonIsEnabledCheckBox.IsChecked = TestPager.LastPageButtonTestHook.IsEnabled;
        }

        private void OnDisplayModeChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = PagerDisplayModeComboBox.SelectedItem;

            if (item == this.AutoDisplayModeItem)
            {
                TestPager.DisplayMode = PagerControlDisplayMode.Auto;
            }
            else if (item == this.NumberBoxDisplayModeItem)
            {
                TestPager.DisplayMode = PagerControlDisplayMode.NumberBox;
            }
            else if (item == this.ComboBoxDisplayModeItem)
            {
                TestPager.DisplayMode = PagerControlDisplayMode.ComboBox;
            }
            else if (item == this.NumberPanelDisplayModeItem)
            {
                TestPager.DisplayMode = PagerControlDisplayMode.ButtonPanel;
            }

            //NumberBoxVisibilityCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.Visibility == Visibility.Visible;
            //ComboBoxVisibilityCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.Visibility == Visibility.Visible;
            //NumberPanelVisibilityCheckBox.IsChecked = TestPager.NumberPanelDisplayTestHook.Visibility == Visibility.Visible;
            //NumberBoxIsEnabledCheckBox.IsChecked = TestPager.NumberBoxDisplayTestHook.IsEnabled;
            //ComboBoxIsEnabledCheckBox.IsChecked = TestPager.ComboBoxDisplayTestHook.IsEnabled;
        }

        private void OnFirstButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = FirstPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.NoneFirstPageButtonVisibilityItem)
            {
                TestPager.FirstButtonVisibility = PagerControlButtonVisibility.Hidden;
            }
            else if (item == this.AlwaysVisibleFirstPageButtonVisibilityItem)
            {
                TestPager.FirstButtonVisibility = PagerControlButtonVisibility.Visible;
            }
            else if (item == this.HiddenOnEdgeFirstPageButtonVisibilityItem)
            {
                TestPager.FirstButtonVisibility = PagerControlButtonVisibility.HiddenOnEdge;
            }

            //FirstPageButtonVisibilityCheckBox.IsChecked = TestPager.FirstPageButtonTestHook.Visibility == Visibility.Visible;
        }

        private void OnPreviousButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = PreviousPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.NonePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousButtonVisibility = PagerControlButtonVisibility.Hidden;
            }
            else if (item == this.AlwaysVisiblePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousButtonVisibility = PagerControlButtonVisibility.Visible;
            }
            else if (item == this.HiddenOnEdgePreviousPageButtonVisibilityItem)
            {
                TestPager.PreviousButtonVisibility = PagerControlButtonVisibility.HiddenOnEdge;
            }

            //PreviousPageButtonVisibilityCheckBox.IsChecked = TestPager.PreviousPageButtonTestHook.Visibility == Visibility.Visible;
        }

        private void OnNextButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = NextPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.NoneNextPageButtonVisibilityItem)
            {
                TestPager.NextButtonVisibility = PagerControlButtonVisibility.Hidden;
            }
            else if (item == this.AlwaysVisibleNextPageButtonVisibilityItem)
            {
                TestPager.NextButtonVisibility = PagerControlButtonVisibility.Visible;
            }
            else if (item == this.HiddenOnEdgeNextPageButtonVisibilityItem)
            {
                TestPager.NextButtonVisibility = PagerControlButtonVisibility.HiddenOnEdge;
            }

            //NextPageButtonVisibilityCheckBox.IsChecked = TestPager.NextPageButtonTestHook.Visibility == Visibility.Visible;
        }
        private void OnLastButtonVisibilityChanged(object sender, SelectionChangedEventArgs e)
        {
            var item = LastPageButtonVisibilityComboBox.SelectedItem;

            if (item == this.NoneLastPageButtonVisibilityItem)
            {
                TestPager.LastButtonVisibility = PagerControlButtonVisibility.Hidden;
            }
            else if (item == this.AlwaysVisibleLastPageButtonVisibilityItem)
            {
                TestPager.LastButtonVisibility = PagerControlButtonVisibility.Visible;
            }
            else if (item == this.HiddenOnEdgeLastPageButtonVisibilityItem)
            {
                TestPager.LastButtonVisibility = PagerControlButtonVisibility.HiddenOnEdge;
            }

            //LastPageButtonVisibilityCheckBox.IsChecked = TestPager.LastPageButtonTestHook.Visibility == Visibility.Visible;
        }
    }
}