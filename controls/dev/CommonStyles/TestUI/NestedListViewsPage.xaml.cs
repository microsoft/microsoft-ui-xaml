// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;

namespace MUXControlsTestApp
{
    public sealed partial class NestedListViewsPage : TestPage
    {
        private ListView _outerListView = null;

        public NestedListViewsPage()
        {
            this.InitializeComponent();

            Loaded += NestedListViewsPage_Loaded;
        }

        private void UpdateOuterListView()
        { 
            if (cmbUseOuterListView != null)
            {
                if (_outerListView != null)
                {
                    _outerListView.Visibility = Visibility.Collapsed;
                }

                switch (cmbUseOuterListView.SelectedIndex)
                {
                    case 0:
                    {
                        _outerListView = outerListView1;
                        break;
                    }
                    case 1:
                    {
                        _outerListView = outerListView2;
                        break;
                    }
                    case 2:
                    {
                        _outerListView = outerListView3;
                        break;
                    }
                    case 3:
                    {
                        _outerListView = outerListView4;
                        break;
                    }
                }

                if (_outerListView != null)
                {
                    _outerListView.Visibility = Visibility.Visible;

                    ChkOuterListViewSingleSelectionFollowsFocus_IsCheckedChanged(null, null);
                    CmbOuterListViewSelectionMode_SelectionChanged(null, null);
                    CmbOuterListViewTabNavigation_SelectionChanged(null, null);
                    CmbOuterListViewHeader_SelectionChanged(null, null);
                    CmbOuterListViewFooter_SelectionChanged(null, null);
                }
            }
        }

        private void NestedListViewsPage_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateOuterListView();
        }

        private void CmbUseOuterListView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateOuterListView();
        }

        private void ChkOuterListViewSingleSelectionFollowsFocus_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_outerListView != null && chkOuterListViewSingleSelectionFollowsFocus != null)
            {
                _outerListView.SingleSelectionFollowsFocus = (bool)chkOuterListViewSingleSelectionFollowsFocus.IsChecked;
            }
        }

        private void CmbOuterListViewSelectionMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerListView != null && cmbOuterListViewSelectionMode != null)
            {
                _outerListView.SelectionMode = (ListViewSelectionMode)cmbOuterListViewSelectionMode.SelectedIndex;
            }
        }

        private void CmbOuterListViewTabNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerListView != null && cmbOuterListViewTabNavigation != null)
            {
                _outerListView.TabNavigation = (KeyboardNavigationMode)cmbOuterListViewTabNavigation.SelectedIndex;
            }
        }

        private void CmbOuterListViewHeader_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerListView != null && cmbOuterListViewHeader != null)
            {
                switch (cmbOuterListViewHeader.SelectedIndex)
                {
                    case 0: /*None*/
                        _outerListView.Header = null;
                        break;
                    case 1: /*Non-focusable*/
                        _outerListView.Header = new TextBlock()
                        {
                            FontSize = 18.0,
                            Margin = new Thickness(10.0),
                            Text = "Header"
                        };
                        break;
                    case 2: /*Focusable*/
                        _outerListView.Header = new Button()
                        {
                            Content = "Header",
                            FontSize = 18.0,
                            Margin = new Thickness(10.0)
                        };
                        break;
                }
            }
        }

        private void CmbOuterListViewFooter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerListView != null && cmbOuterListViewFooter != null)
            {
                switch (cmbOuterListViewFooter.SelectedIndex)
                {
                    case 0: /*None*/
                        _outerListView.Footer = null;
                        break;
                    case 1: /*Non-focusable*/
                        _outerListView.Footer = new TextBlock()
                        {
                            FontSize = 18.0,
                            Margin = new Thickness(10.0),
                            Text = "Footer"
                        };
                        break;
                    case 2: /*Focusable*/
                        _outerListView.Footer = new Button()
                        {
                            Content = "Footer",
                            FontSize = 18.0,
                            Margin = new Thickness(10.0)
                        };
                        break;
                }
            }
        }
    }
}
