// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using MUXControlsTestApp.Samples.Model;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

namespace MUXControlsTestApp
{
    public sealed partial class FlatItemsControlPage : TestPage
    {
        private ObservableCollection<Recipe> _colRecipes = null;
        private ItemsPanelTemplate _modernPanelTemplate = null;
        private DataTemplate[] _itemTemplates = new DataTemplate[3];

        public FlatItemsControlPage()
        {
            this.InitializeComponent();

            Loaded += FlatItemsControlPage_Loaded;
        }

        private void FlatItemsControlPage_Loaded(object sender, RoutedEventArgs e)
        {
            _colRecipes = new ObservableCollection<Recipe>();

            for (int itemIndex = 0; itemIndex < 250; itemIndex++)
            {
                BitmapImage bitmapImage = GetBitmapImage(itemIndex % 126 + 1);

                _colRecipes.Add(new Recipe()
                {
                    BitmapImage = bitmapImage,
                    Id = itemIndex
                });
            }

            UpdateItemsControlIsTabStop();
            UpdateItemsControlItemTemplate();
            UpdateItemsControlTabNavigation();
            UpdateItemsControlXYFocusKeyboardNavigation();
            UpdateItemsPanelType();

            itemsControl.ItemsSource = _colRecipes;
        }

        private void UpdateItemsPanelType()
        {
            try
            {
                if (itemsControl != null && cmbItemsControlItemsPanelType != null)
                {
                    switch (cmbItemsControlItemsPanelType.SelectedIndex)
                    {
                        case 0:
                            if (_modernPanelTemplate == null)
                            {
                                _modernPanelTemplate = itemsControl.ItemsPanel;
                            }
                            else
                            {
                                itemsControl.ItemsPanel = _modernPanelTemplate;
                            }
                            break;
                        case 1: // Use the virtualizingStackPanelItemsPanelTemplate resource
                            itemsControl.ItemsPanel = Resources["virtualizingStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            break;
                        case 2: // Use the stackPanelItemsPanelTemplate resource
                            itemsControl.ItemsPanel = Resources["stackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            break;
                    }

                    // itemsControl.ItemsPanelRoot is only updated asynchronously.
                    _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateModernPanelUIVisibility);
                    _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateItemsPanelProperties);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsPanelProperties()
        {
            UpdateItemsPanelOrientation();
            UpdateItemsPanelMargin();

            if (itemsControl.ItemsPanelRoot is ItemsWrapGrid)
            {
                UpdateItemsWrapGridMaximumRowsOrColumns();
            }
        }

        private void UpdateItemsControlIsTabStop()
        {
            if (itemsControl != null && chkItemsControlIsTabStop != null)
            {
                chkItemsControlIsTabStop.IsChecked = itemsControl.IsTabStop;
            }
        }

        private void UpdateItemsControlItemTemplate()
        {
            try
            {
                if (itemsControl != null && cmbItemsControlItemTemplate != null)
                {
                    if (cmbItemsControlItemTemplate.SelectedIndex == 0)
                    {
                        itemsControl.ItemTemplate = null;
                    }
                    else
                    {
                        int templateIndex = cmbItemsControlItemTemplate.SelectedIndex - 1;

                        if (_itemTemplates[templateIndex] == null)
                        {
                            _itemTemplates[templateIndex] = Resources["itemTemplate" + cmbItemsControlItemTemplate.SelectedIndex.ToString()] as DataTemplate;
                        }

                        itemsControl.ItemTemplate = _itemTemplates[templateIndex];
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsControlTabNavigation()
        {
            if (itemsControl != null && cmbItemsControlTabNavigation != null)
            {
                cmbItemsControlTabNavigation.SelectedIndex = (int)itemsControl.TabNavigation;
            }
        }

        private void UpdateItemsControlXYFocusKeyboardNavigation()
        {
            if (itemsControl != null && cmbItemsControlXYFocusKeyboardNavigation != null)
            {
                cmbItemsControlXYFocusKeyboardNavigation.SelectedIndex = (int)itemsControl.XYFocusKeyboardNavigation;
            }
        }

        private void UpdateItemsPanelOrientation()
        {
            if (itemsControl != null && cmbItemsPanelOrientation != null)
            {
                ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                if (itemsWrapGrid != null)
                {
                    cmbItemsPanelOrientation.SelectedIndex = (int)itemsWrapGrid.Orientation;
                }
                else
                {
                    ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        cmbItemsPanelOrientation.SelectedIndex = (int)itemsStackPanel.Orientation;
                    }
                    else
                    {
                        VirtualizingStackPanel virtualizingStackPanel = itemsControl.ItemsPanelRoot as VirtualizingStackPanel;

                        if (virtualizingStackPanel != null)
                        {
                            cmbItemsPanelOrientation.SelectedIndex = (int)virtualizingStackPanel.Orientation;
                        }
                        else
                        {
                            StackPanel stackPanel = itemsControl.ItemsPanelRoot as StackPanel;

                            if (stackPanel != null)
                            {
                                cmbItemsPanelOrientation.SelectedIndex = (int)stackPanel.Orientation;
                            }
                        }
                    }
                }
            }
        }

        private void UpdateItemsWrapGridMaximumRowsOrColumns()
        {
            try
            {
                if (itemsControl != null && nbItemsWrapGridMaximumRowsOrColumns != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null && itemsWrapGrid.MaximumRowsOrColumns != -1)
                    {
                        nbItemsWrapGridMaximumRowsOrColumns.Value = itemsWrapGrid.MaximumRowsOrColumns;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsPanelMargin()
        {
            try
            {
                if (itemsControl != null && txtItemsPanelMargin != null)
                {
                    Panel itemsPanel = itemsControl.ItemsPanelRoot;

                    if (itemsPanel != null)
                    {
                        txtItemsPanelMargin.Text = itemsPanel.Margin.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateModernPanelUIVisibility()
        {
            if (itemsControl.ItemsPanelRoot is ItemsWrapGrid)
            {
                tblItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Visible;
                nbItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Visible;
            }
            else
            {
                tblItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Collapsed;
                nbItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Collapsed;
            }
        }

        private void UpdateDataSourceItemCount()
        {
            if (itemsControl != null && txtDataSourceItemCount != null)
            {
                if (itemsControl.ItemsSource == null)
                {
                    txtDataSourceItemCount.Text = "0";
                }
                else if (itemsControl.ItemsSource == _colRecipes)
                {
                    txtDataSourceItemCount.Text = _colRecipes.Count.ToString();
                }
            }
        }

        private BitmapImage GetBitmapImage(int index)
        {
            Uri uri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", index));
            BitmapImage bitmapImage = new BitmapImage();
            bitmapImage.DecodePixelWidth = 120;
            bitmapImage.DecodePixelHeight = 120;
            bitmapImage.UriSource = uri;
            return bitmapImage;
        }

        private void DataSourceAddItem()
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null)
                {
                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        BitmapImage bitmapImage = GetBitmapImage(_colRecipes.Count % 126 + 1);

                        var recipe = new Recipe()
                        {
                            BitmapImage = bitmapImage,
                            Id = _colRecipes.Count
                        };

                        _colRecipes.Add(recipe);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void DataSourceInsertItem(int newItemIndex)
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null)
                {
                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        BitmapImage bitmapImage = GetBitmapImage(_colRecipes.Count % 126 + 1);

                        var recipe = new Recipe()
                        {
                            BitmapImage = bitmapImage,
                            Id = _colRecipes.Count
                        };

                        _colRecipes.Insert(newItemIndex, recipe);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void DataSourceRemoveAllItems()
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null)
                {
                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        _colRecipes.Clear();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void DataSourceRemoveItem(int oldItemIndex)
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null)
                {
                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        _colRecipes.RemoveAt(oldItemIndex);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void DataSourceReplaceItem(int itemIndex)
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null)
                {
                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        BitmapImage bitmapImage = GetBitmapImage(_colRecipes.Count % 126 + 1);

                        var recipe = new Recipe()
                        {
                            BitmapImage = bitmapImage,
                            Id = _colRecipes.Count
                        };

                        _colRecipes[itemIndex] = recipe;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetItemsControlTabNavigation_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsControlTabNavigation();
        }

        private void BtnSetItemsControlTabNavigation_Click(object sender, RoutedEventArgs e)
        {
            if (itemsControl != null && cmbItemsControlTabNavigation != null)
            {
                itemsControl.TabNavigation = (KeyboardNavigationMode)cmbItemsControlTabNavigation.SelectedIndex;
            }
        }

        private void BtnGetItemsControlXYFocusKeyboardNavigation_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsControlXYFocusKeyboardNavigation();
        }

        private void BtnSetItemsControlXYFocusKeyboardNavigation_Click(object sender, RoutedEventArgs e)
        {
            if (itemsControl != null && cmbItemsControlXYFocusKeyboardNavigation != null)
            {
                itemsControl.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbItemsControlXYFocusKeyboardNavigation.SelectedIndex;
            }
        }

        private void CmbItemsPresenterHeader_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsControl != null && cmbItemsPresenterHeader != null)
                {
                    ItemsPresenter itemsPresenter = FindElementOfTypeInSubtree<ItemsPresenter>(itemsControl);

                    if (itemsPresenter != null)
                    {
                        switch (cmbItemsPresenterHeader.SelectedIndex)
                        {
                            case 0: /*None*/
                                itemsPresenter.Header = null;
                                break;
                            case 1: /*No TabStop*/
                                itemsPresenter.Header = new TextBlock()
                                {
                                    FontSize = 18.0,
                                    Margin = new Thickness(10.0),
                                    Text = "ItemsPresenter Header"
                                };
                                break;
                            case 2: /*Single TabStop*/
                                itemsPresenter.Header = new Button()
                                {
                                    Content = "ItemsPresenter Header",
                                    FontSize = 18.0,
                                    Margin = new Thickness(10.0)
                                };
                                break;
                            case 3: /*Nested TabStops*/
                                itemsPresenter.Header = new Button()
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
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbItemsPresenterFooter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsControl != null && cmbItemsPresenterFooter != null)
                {
                    ItemsPresenter itemsPresenter = FindElementOfTypeInSubtree<ItemsPresenter>(itemsControl);

                    if (itemsPresenter != null)
                    {
                        switch (cmbItemsPresenterFooter.SelectedIndex)
                        {
                            case 0: /*None*/
                                itemsPresenter.Footer = null;
                                break;
                            case 1: /*No TabStop*/
                                itemsPresenter.Footer = new TextBlock()
                                {
                                    FontSize = 18.0,
                                    Margin = new Thickness(10.0),
                                    Text = "ItemsPresenter Footer"
                                };
                                break;
                            case 2: /*Single TabStop*/
                                itemsPresenter.Footer = new Button() 
                                {
                                    Content = "ItemsPresenter Footer",
                                    FontSize = 18.0,
                                    Margin = new Thickness(10.0)
                                };
                                break;
                            case 3: /*Nested TabStops*/
                                itemsPresenter.Footer = new Button()
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
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbItemsControlItemTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateItemsControlItemTemplate();
        }

        private void CmbItemsControlItemsPanelType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateItemsPanelType();
        }

        private void ChkItemsControlIsTabStop_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (itemsControl != null && chkItemsControlIsTabStop != null)
            {
                itemsControl.IsTabStop = (bool)chkItemsControlIsTabStop.IsChecked;
            }
        }

        private void BtnGetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelOrientation();
        }

        private void BtnSetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            if (itemsControl != null && cmbItemsPanelOrientation != null)
            {
                ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                if (itemsWrapGrid != null)
                {
                    itemsControl.ItemsSource = null;

                    itemsWrapGrid.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                }
                else
                {
                    ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        itemsControl.ItemsSource = null;

                        itemsStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                    }
                    else
                    {
                        VirtualizingStackPanel virtualizingStackPanel = itemsControl.ItemsPanelRoot as VirtualizingStackPanel;

                        if (virtualizingStackPanel != null)
                        {
                            virtualizingStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                        }
                        else
                        {
                            StackPanel stackPanel = itemsControl.ItemsPanelRoot as StackPanel;

                            if (stackPanel != null)
                            {
                                stackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                    }
                }

                itemsControl.ItemsSource = _colRecipes;
            }
        }

        private void BtnGetItemsPanelMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelMargin();
        }

        private void BtnSetItemsPanelMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsControl != null && txtItemsPanelMargin != null)
                {
                    Panel itemsPanel = itemsControl.ItemsPanelRoot;

                    if (itemsPanel != null)
                    {
                        itemsPanel.Margin = GetThicknessFromString(txtItemsPanelMargin.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void NbItemsWrapGridMaximumRowsOrColumns_ValueChanged(object sender, object e)
        {
            try
            {
                if (itemsControl != null && nbItemsWrapGridMaximumRowsOrColumns != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.MaximumRowsOrColumns = (int)nbItemsWrapGridMaximumRowsOrColumns.Value;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnDataSourceGetItemCount_Click(object sender, RoutedEventArgs e)
        {
            UpdateDataSourceItemCount();
        }

        private void BtnDataSourceSetItemCount_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsControl != null && itemsControl.ItemsSource != null && txtDataSourceItemCount != null)
                {
                    int newItemCount = int.Parse(txtDataSourceItemCount.Text);

                    if (itemsControl.ItemsSource == _colRecipes)
                    {
                        if (_colRecipes.Count < newItemCount)
                        {
                            var colRecipesEnd = new List<Recipe>();

                            for (int itemIndex = 0; itemIndex < newItemCount - _colRecipes.Count; itemIndex++)
                            {
                                BitmapImage bitmapImage = GetBitmapImage(itemIndex % 126 + 1);

                                colRecipesEnd.Add(new Recipe()
                                {
                                    BitmapImage = bitmapImage,
                                    Id = itemIndex
                                });
                            }

                            _colRecipes = new ObservableCollection<Recipe>(_colRecipes.Concat(colRecipesEnd));
                        }
                        else if (_colRecipes.Count > newItemCount)
                        {
                            _colRecipes = new ObservableCollection<Recipe>(_colRecipes.Take(newItemCount));
                        }

                        itemsControl.ItemsSource = _colRecipes;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnDataSourceAddItem_Click(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("DataSourceAdd");
            DataSourceAddItem();
        }

        private void BtnDataSourceInsertItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtDataSourceItemIndex != null)
                {
                    int newItemIndex = int.Parse(txtDataSourceItemIndex.Text);

                    AppendEventMessage("DataSourceInsert " + newItemIndex);
                    DataSourceInsertItem(newItemIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnDataSourceRemoveAllItems_Click(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("DataSourceRemoveAll");
            DataSourceRemoveAllItems();
        }

        private void BtnDataSourceRemoveItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtDataSourceItemIndex != null)
                {
                    int oldItemIndex = int.Parse(txtDataSourceItemIndex.Text);

                    AppendEventMessage("DataSourceRemove " + oldItemIndex);
                    DataSourceRemoveItem(oldItemIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnDataSourceReplaceItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtDataSourceItemIndex != null)
                {
                    int itemIndex = int.Parse(txtDataSourceItemIndex.Text);

                    AppendEventMessage("DataSourceReplace " + itemIndex);
                    DataSourceReplaceItem(itemIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkItemsControlProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svItemsControlProperties != null)
                svItemsControlProperties.Visibility = Visibility.Visible;
        }

        private void ChkItemsControlProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svItemsControlProperties != null)
                svItemsControlProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkDataSource_Checked(object sender, RoutedEventArgs e)
        {
            if (svDataSource != null)
                svDataSource.Visibility = Visibility.Visible;
        }

        private void ChkDataSource_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svDataSource != null)
                svDataSource.Visibility = Visibility.Collapsed;
        }

        private void ChkLogs_Checked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Visible;
        }

        private void ChkLogs_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Collapsed;
        }

        private void AppendEventMessage(string eventMessage)
        {
            lstLogs.Items.Add(eventMessage);
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private Thickness GetThicknessFromString(string thickness)
        {
            string[] lengths = thickness.Split(',');
            if (lengths.Length < 4)
                return new Thickness(
                    Convert.ToDouble(lengths[0]));
            else
                return new Thickness(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
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
