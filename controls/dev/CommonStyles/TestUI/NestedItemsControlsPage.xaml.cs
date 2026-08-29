// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public sealed partial class NestedItemsControlsPage : TestPage
    {
        private ItemsControl _outerItemsControl = null;

        public NestedItemsControlsPage()
        {
            this.InitializeComponent();

            Loaded += NestedItemsControlsPage_Loaded;
        }

        private void UpdateOuterItemsControl()
        { 
            if (cmbUseOuterItemsControl != null)
            {
                if (_outerItemsControl != null)
                {
                    _outerItemsControl.Visibility = Visibility.Collapsed;
                }

                switch (cmbUseOuterItemsControl.SelectedIndex)
                {
                    case 0:
                    {
                        _outerItemsControl = outerItemsControl1;
                        break;
                    }
                    case 1:
                    {
                        _outerItemsControl = outerItemsControl2;
                        break;
                    }
                    case 2:
                    {
                        _outerItemsControl = outerItemsControl3;
                        break;
                    }
                    case 3:
                    {
                        _outerItemsControl = outerItemsControl4;
                        break;
                    }
                }

                if (_outerItemsControl != null)
                {
                    _outerItemsControl.Visibility = Visibility.Visible;

                    ChkOuterItemsControlIsTabStop_IsCheckedChanged(null, null);
                    CmbOuterItemsControlTabNavigation_SelectionChanged(null, null);
                    CmbOuterItemsControlXYFocusKeyboardNavigation_SelectionChanged(null, null);
                }
            }
        }

        private void NestedItemsControlsPage_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateOuterItemsControl();
        }

        private void CmbUseOuterItemsControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateOuterItemsControl();
        }

        private void ChkOuterItemsControlIsTabStop_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (_outerItemsControl != null && chkOuterItemsControlIsTabStop != null)
            {
                _outerItemsControl.IsTabStop = (bool)chkOuterItemsControlIsTabStop.IsChecked;
            }
        }

        private void CmbOuterItemsControlTabNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerItemsControl != null && cmbOuterItemsControlTabNavigation != null)
            {
                _outerItemsControl.TabNavigation = (KeyboardNavigationMode)cmbOuterItemsControlTabNavigation.SelectedIndex;
            }
        }

        private void CmbOuterItemsControlXYFocusKeyboardNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerItemsControl != null && cmbOuterItemsControlXYFocusKeyboardNavigation != null)
            {
                _outerItemsControl.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbOuterItemsControlXYFocusKeyboardNavigation.SelectedIndex;
            }
        }

        private void CmbOuterItemsPresenterHeader_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerItemsControl != null && cmbOuterItemsPresenterHeader != null)
            {
                ItemsPresenter outerItemsPresenter = FindElementOfTypeInSubtree<ItemsPresenter>(_outerItemsControl);

                if (outerItemsPresenter != null)
                {
                    switch (cmbOuterItemsPresenterHeader.SelectedIndex)
                    {
                        case 0: /*None*/
                            outerItemsPresenter.Header = null;
                            break;
                        case 1: /*No TabStop*/
                            outerItemsPresenter.Header = new TextBlock()
                            {
                                FontSize = 18.0,
                                Margin = new Thickness(10.0),
                                Text = "ItemsPresenter Header"
                            };
                            break;
                        case 2: /*Single TabStop*/
                            outerItemsPresenter.Header = new Button()
                            {
                                Content = "ItemsPresenter Header",
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                        case 3: /*Nested TabStops*/
                            outerItemsPresenter.Header = new Button()
                            {
                                Content = new Button()
                                {
                                    Content = "ItemsPresenter Header",
                                    Margin = new Thickness(10.0)
                                },
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                    }
                }
            }
        }

        private void CmbOuterItemsPresenterFooter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (_outerItemsControl != null && cmbOuterItemsPresenterFooter != null)
            {
                ItemsPresenter outerItemsPresenter = FindElementOfTypeInSubtree<ItemsPresenter>(_outerItemsControl);

                if (outerItemsPresenter != null)
                {
                    switch (cmbOuterItemsPresenterFooter.SelectedIndex)
                    {
                        case 0: /*None*/
                            outerItemsPresenter.Footer = null;
                            break;
                        case 1: /*No TabStop*/
                            outerItemsPresenter.Footer = new TextBlock()
                            {
                                FontSize = 18.0,
                                Margin = new Thickness(10.0),
                                Text = "ItemsPresenter Footer"
                            };
                            break;
                        case 2: /*Single TabStop*/
                            outerItemsPresenter.Footer = new Button()
                            {
                                Content = "ItemsPresenter Footer",
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                        case 3: /*Nested TabStops*/
                            outerItemsPresenter.Footer = new Button()
                            {
                                Content = new Button()
                                {
                                    Content = "ItemsPresenter Footer",
                                    Margin = new Thickness(10.0)
                                },
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                    }
                }
            }
        }

        private static T FindElementOfTypeInSubtree<T>(DependencyObject element) where T : DependencyObject
        {
            if (element == null)
                return null;

            if (element is T)
                return (T)element;

            int childrenCount = VisualTreeHelper.GetChildrenCount(element);
            for (int i = 0; i < childrenCount; i++)
            {
                var result = FindElementOfTypeInSubtree<T>(VisualTreeHelper.GetChild(element, i));
                if (result != null)
                    return result;
            }

            return null;
        }
    }
}
