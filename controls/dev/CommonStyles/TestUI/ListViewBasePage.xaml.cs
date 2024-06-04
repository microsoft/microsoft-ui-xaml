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
    public sealed partial class ListViewBasePage : TestPage
    {
        private ObservableCollection<Recipe> _colRecipes = null;
        private ListViewBase _listViewBase = null;
        private ScrollViewer _scrollViewer = null;
        private ItemsPanelTemplate _modernPanelTemplate = null;

        public ListViewBasePage()
        {
            this.InitializeComponent();

            Loaded += ListViewBasePage_Loaded;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            bool useGridView = (bool)e.Parameter;

            _listViewBase = useGridView ? gridView : listView;
            _listViewBase.Visibility = Visibility.Visible;

            if (useGridView)
            {
                tblListViewBasePageTitle.Text = "GridView";
                tblItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Visible;
                nbItemsWrapGridMaximumRowsOrColumns.Visibility = Visibility.Visible;
            }
            else
            {
                tblListViewBasePageTitle.Text = "ListView";
            }

            base.OnNavigatedTo(e);
        }

        private void ListViewBasePage_Loaded(object sender, RoutedEventArgs e)
        {
            _scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(_listViewBase);

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

            UpdateItemsPanelType();
            UpdateListViewBaseSingleSelectionFollowsFocus();

            _listViewBase.ItemsSource = _colRecipes;
        }

        // Workaround to avoid the ListViewBase not being scrollable after an ItemsPanel change to VirtualizingStackPanel.
        private void RestoreScrollViewerWidth()
        {
            if (_scrollViewer != null)
            {
                _scrollViewer.Width = double.NaN;

                _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerSizeProperties);
            }
        }

        private void UpdateItemsPanelType()
        {
            try
            {
                if (_listViewBase != null && cmbListViewBaseItemsPanelType != null)
                {
                    switch (cmbListViewBaseItemsPanelType.SelectedIndex)
                    {
                        case 0:
                            if (_modernPanelTemplate == null)
                            {
                                _modernPanelTemplate = _listViewBase.ItemsPanel;
                            }
                            else
                            {
                                _listViewBase.ItemsPanel = _modernPanelTemplate;
                                UpdateModernPanelUIVisibility(Visibility.Visible);
                            }
                            break;
                        case 1: // Use the virtualizingStackPanelItemsPanelTemplate resource
                            _listViewBase.ItemsPanel = Resources["virtualizingStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateModernPanelUIVisibility(Visibility.Collapsed);
                            break;
                        case 2: // Use the stackPanelItemsPanelTemplate resource
                            _listViewBase.ItemsPanel = Resources["stackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateModernPanelUIVisibility(Visibility.Collapsed);
                            break;
                    }

                    // _listViewBase.ItemsPanelRoot is only updated asynchronously.
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

            if (_listViewBase.ItemsPanel == _modernPanelTemplate && _listViewBase == gridView)
            {
                UpdateItemsWrapGridMaximumRowsOrColumns();
            }

            // _listViewBase.ItemsPanelRoot is only updated asynchronously.
            _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerScrollOrientation);
            _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerProperties);
        }

        private void UpdateListViewBaseReorderMode()
        {
            if (_listViewBase != null && cmbListViewBaseReorderMode != null)
            {
                cmbListViewBaseReorderMode.SelectedIndex = (int)_listViewBase.ReorderMode;
            }
        }

        private void UpdateListViewBaseSelectionMode()
        {
            if (_listViewBase != null && cmbListViewBaseSelectionMode != null)
            {
                cmbListViewBaseSelectionMode.SelectedIndex = (int)_listViewBase.SelectionMode;
            }
        }

        private void UpdateListViewBaseSingleSelectionFollowsFocus()
        {
            if (_listViewBase != null && chkListViewBaseSingleSelectionFollowsFocus != null)
            {
                chkListViewBaseSingleSelectionFollowsFocus.IsChecked = _listViewBase.SingleSelectionFollowsFocus;
            }
        }

        private void UpdateListViewBaseTabNavigation()
        {
            if (_listViewBase != null && cmbListViewBaseTabNavigation != null)
            {
                cmbListViewBaseTabNavigation.SelectedIndex = (int)_listViewBase.TabNavigation;
            }
        }

        private void UpdateItemsPanelOrientation()
        {
            if (_listViewBase != null && cmbItemsPanelOrientation != null)
            {
                ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                if (itemsWrapGrid != null)
                {
                    cmbItemsPanelOrientation.SelectedIndex = (int)itemsWrapGrid.Orientation;
                }
                else
                {
                    ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        cmbItemsPanelOrientation.SelectedIndex = (int)itemsStackPanel.Orientation;
                    }
                    else
                    {
                        VirtualizingStackPanel virtualizingStackPanel = _listViewBase.ItemsPanelRoot as VirtualizingStackPanel;

                        if (virtualizingStackPanel != null)
                        {
                            cmbItemsPanelOrientation.SelectedIndex = (int)virtualizingStackPanel.Orientation;
                        }
                        else
                        {
                            StackPanel stackPanel = _listViewBase.ItemsPanelRoot as StackPanel;

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
                if (_listViewBase != null && nbItemsWrapGridMaximumRowsOrColumns != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

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
                if (_listViewBase != null && txtItemsPanelMargin != null)
                {
                    Panel itemsPanel = _listViewBase.ItemsPanelRoot;

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

        private void UpdateModernPanelUIVisibility(Visibility visibility)
        {
            if (_listViewBase == gridView)
            {
                tblItemsWrapGridMaximumRowsOrColumns.Visibility = visibility;
                nbItemsWrapGridMaximumRowsOrColumns.Visibility = visibility;
            }
        }

        private void UpdateScrollViewerScrollOrientation()
        {
            try
            {
                if (_listViewBase != null)
                {
                    Orientation scrollOrientation = Orientation.Horizontal;

                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        if (itemsWrapGrid.Orientation == Orientation.Horizontal)
                        {
                            scrollOrientation = Orientation.Vertical;
                        }
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            if (itemsStackPanel.Orientation == Orientation.Vertical)
                            {
                                scrollOrientation = Orientation.Vertical;
                            }
                        }
                        else
                        {
                            VirtualizingStackPanel virtualizingStackPanel = _listViewBase.ItemsPanelRoot as VirtualizingStackPanel;

                            if (virtualizingStackPanel != null)
                            {
                                if (virtualizingStackPanel.Orientation == Orientation.Vertical)
                                {
                                    scrollOrientation = Orientation.Vertical;
                                }
                            }
                            else
                            {
                                StackPanel stackPanel = _listViewBase.ItemsPanelRoot as StackPanel;

                                if (stackPanel != null && stackPanel.Orientation == Orientation.Vertical)
                                {
                                    scrollOrientation = Orientation.Vertical;
                                }
                            }
                        }
                    }

                    if (scrollOrientation == Orientation.Vertical)
                    {
                        ScrollViewer.SetHorizontalScrollBarVisibility(_listViewBase, ScrollBarVisibility.Disabled);
                        ScrollViewer.SetVerticalScrollBarVisibility(_listViewBase, ScrollBarVisibility.Auto);
                        ScrollViewer.SetHorizontalScrollMode(_listViewBase, ScrollMode.Disabled);
                        ScrollViewer.SetVerticalScrollMode(_listViewBase, ScrollMode.Enabled);
                        ScrollViewer.SetIsHorizontalRailEnabled(_listViewBase, false);
                        ScrollViewer.SetIsVerticalRailEnabled(_listViewBase, true);
                    }
                    else
                    {
                        ScrollViewer.SetHorizontalScrollBarVisibility(_listViewBase, ScrollBarVisibility.Auto);
                        ScrollViewer.SetVerticalScrollBarVisibility(_listViewBase, ScrollBarVisibility.Disabled);
                        ScrollViewer.SetHorizontalScrollMode(_listViewBase, ScrollMode.Enabled);
                        ScrollViewer.SetVerticalScrollMode(_listViewBase, ScrollMode.Disabled);
                        ScrollViewer.SetIsHorizontalRailEnabled(_listViewBase, true);
                        ScrollViewer.SetIsVerticalRailEnabled(_listViewBase, false);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerSizeProperties()
        {
            UpdateScrollViewerViewportWidth();
            UpdateScrollViewerViewportHeight();
            UpdateScrollViewerExtentWidth();
            UpdateScrollViewerExtentHeight();
            UpdateScrollViewerHorizontalOffset();
            UpdateScrollViewerVerticalOffset();
        }

        private void UpdateScrollViewerProperties()
        {
            if (_listViewBase != null && _scrollViewer != null && _listViewBase.ItemsPanelRoot is VirtualizingStackPanel)
            {
                // Workaround to avoid the ListViewBase not being scrollable after an ItemsPanel change to VirtualizingStackPanel.
                // Resizing the ScrollViewer temporarily fixes its manipulability.
                _scrollViewer.Width = _scrollViewer.ActualWidth + 1;

                _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, RestoreScrollViewerWidth);
            }

            UpdateScrollViewerSizeProperties();
            UpdateScrollViewerHorizontalSnapPointsType();
            UpdateScrollViewerVerticalSnapPointsType();
        }

        private void UpdateScrollViewerViewportWidth()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerViewportWidth != null)
                {
                    txtScrollViewerViewportWidth.Text = _scrollViewer.ViewportWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerViewportHeight()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerViewportHeight != null)
                {
                    txtScrollViewerViewportHeight.Text = _scrollViewer.ViewportHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerExtentWidth()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerExtentWidth != null)
                {
                    txtScrollViewerExtentWidth.Text = _scrollViewer.ExtentWidth.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerExtentHeight()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerExtentHeight != null)
                {
                    txtScrollViewerExtentHeight.Text = _scrollViewer.ExtentHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerHorizontalOffset()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerHorizontalOffset != null)
                {
                    txtScrollViewerHorizontalOffset.Text = _scrollViewer.HorizontalOffset.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerVerticalOffset()
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerVerticalOffset != null)
                {
                    txtScrollViewerVerticalOffset.Text = _scrollViewer.VerticalOffset.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerHorizontalSnapPointsType()
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerHorizontalSnapPointsType != null)
                {
                    cmbScrollViewerHorizontalSnapPointsType.SelectedIndex = (int)_scrollViewer.HorizontalSnapPointsType;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerVerticalSnapPointsType()
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerVerticalSnapPointsType != null)
                {
                    cmbScrollViewerVerticalSnapPointsType.SelectedIndex = (int)_scrollViewer.VerticalSnapPointsType;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateDataSourceItemCount()
        {
            if (_listViewBase != null && txtDataSourceItemCount != null)
            {
                if (_listViewBase.ItemsSource == null)
                {
                    txtDataSourceItemCount.Text = "0";
                }
                else if (_listViewBase.ItemsSource == _colRecipes)
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
                if (_listViewBase != null && _listViewBase.ItemsSource != null)
                {
                    if (_listViewBase.ItemsSource == _colRecipes)
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
                if (_listViewBase != null && _listViewBase.ItemsSource != null)
                {
                    if (_listViewBase.ItemsSource == _colRecipes)
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
                if (_listViewBase != null && _listViewBase.ItemsSource != null)
                {
                    if (_listViewBase.ItemsSource == _colRecipes)
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
                if (_listViewBase != null && _listViewBase.ItemsSource != null)
                {
                    if (_listViewBase.ItemsSource == _colRecipes)
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
                if (_listViewBase != null && _listViewBase.ItemsSource != null)
                {
                    if (_listViewBase.ItemsSource == _colRecipes)
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

        private void ListViewBase_ItemClick(object sender, ItemClickEventArgs e)
        {
            try
            {
                AppendEventMessage($"ListViewBase_ItemClick ClickedItem={e.ClickedItem.ToString()}");
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetListViewBaseReorderMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateListViewBaseReorderMode();
        }

        private void BtnSetListViewBaseReorderMode_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && cmbListViewBaseReorderMode != null)
            {
                _listViewBase.ReorderMode = (ListViewReorderMode)cmbListViewBaseReorderMode.SelectedIndex;
            }
        }

        private void BtnGetListViewBaseSelectionMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateListViewBaseSelectionMode();
        }

        private void BtnSetListViewBaseSelectionMode_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && cmbListViewBaseSelectionMode != null)
            {
                _listViewBase.SelectionMode = (ListViewSelectionMode) cmbListViewBaseSelectionMode.SelectedIndex;
            }
        }

        private void BtnGetListViewBaseTabNavigation_Click(object sender, RoutedEventArgs e)
        {
            UpdateListViewBaseTabNavigation();
        }

        private void BtnSetListViewBaseTabNavigation_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && cmbListViewBaseTabNavigation != null)
            {
                _listViewBase.TabNavigation = (KeyboardNavigationMode)cmbListViewBaseTabNavigation.SelectedIndex;
            }
        }

        private void CmbListViewBaseItemsPanelType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateItemsPanelType();
        }

        private void CmbListViewBaseHeader_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && cmbListViewBaseHeader != null)
                {
                    switch (cmbListViewBaseHeader.SelectedIndex)
                    {
                        case 0: /*None*/
                            _listViewBase.Header = null;
                            break;
                        case 1: /*Non-focusable*/
                            _listViewBase.Header = new TextBlock()
                            {
                                FontSize = 18.0,
                                Margin = new Thickness(10.0),
                                Text = "Header"
                            };
                            break;
                        case 2: /*Focusable*/
                            _listViewBase.Header = new Button()
                            {
                                Content = "Header",
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbListViewBaseFooter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && cmbListViewBaseFooter != null)
                {
                    switch (cmbListViewBaseFooter.SelectedIndex)
                    {
                        case 0: /*None*/
                            _listViewBase.Footer = null;
                            break;
                        case 1: /*Non-focusable*/
                            _listViewBase.Footer = new TextBlock()
                            {
                                FontSize = 18.0,
                                Margin = new Thickness(10.0),
                                Text = "Footer"
                            };
                            break;
                        case 2: /*Focusable*/
                            _listViewBase.Footer = new Button()
                            {
                                Content = "Footer",
                                FontSize = 18.0,
                                Margin = new Thickness(10.0)
                            };
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelOrientation();
        }

        private void BtnSetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && cmbItemsPanelOrientation != null)
            {
                ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                if (itemsWrapGrid != null)
                {
                    _listViewBase.ItemsSource = null;

                    itemsWrapGrid.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                }
                else
                {
                    ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                    if (itemsStackPanel != null)
                    {
                        _listViewBase.ItemsSource = null;

                        itemsStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                    }
                    else
                    {
                        VirtualizingStackPanel virtualizingStackPanel = _listViewBase.ItemsPanelRoot as VirtualizingStackPanel;

                        if (virtualizingStackPanel != null)
                        {
                            virtualizingStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                        }
                        else
                        {
                            StackPanel stackPanel = _listViewBase.ItemsPanelRoot as StackPanel;

                            if (stackPanel != null)
                            {
                                stackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                    }
                }

                _listViewBase.ItemsSource = _colRecipes;

                // _listViewBase.ItemsPanelRoot is only updated asynchronously.
                _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerScrollOrientation);
                _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerProperties);
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
                if (_listViewBase != null && txtItemsPanelMargin != null)
                {
                    Panel itemsPanel = _listViewBase.ItemsPanelRoot;

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

        private void BtnGetScrollViewerViewportWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerViewportWidth();
        }

        private void BtnGetScrollViewerViewportHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerViewportHeight();
        }

        private void BtnGetScrollViewerExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerExtentWidth();
        }

        private void BtnGetScrollViewerExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerExtentHeight();
        }

        private void BtnGetScrollViewerHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerHorizontalOffset();
        }

        private void BtnGetScrollViewerVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerVerticalOffset();
        }

        private void CmbScrollViewerHorizontalSnapPointsType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerHorizontalSnapPointsType != null)
                {
                    _scrollViewer.HorizontalSnapPointsType = (SnapPointsType)cmbScrollViewerHorizontalSnapPointsType.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbScrollViewerVerticalSnapPointsType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerVerticalSnapPointsType != null)
                {
                    _scrollViewer.VerticalSnapPointsType = (SnapPointsType)cmbScrollViewerVerticalSnapPointsType.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbScrollViewerHorizontalSnapPointsAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerHorizontalSnapPointsAlignment != null)
                {
                    _scrollViewer.HorizontalSnapPointsAlignment = (SnapPointsAlignment)cmbScrollViewerHorizontalSnapPointsAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbScrollViewerVerticalSnapPointsAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerVerticalSnapPointsAlignment != null)
                {
                    _scrollViewer.VerticalSnapPointsAlignment = (SnapPointsAlignment)cmbScrollViewerVerticalSnapPointsAlignment.SelectedIndex;
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
                if (_listViewBase != null && nbItemsWrapGridMaximumRowsOrColumns != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

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
                if (_listViewBase != null && _listViewBase.ItemsSource != null && txtDataSourceItemCount != null)
                {
                    int newItemCount = int.Parse(txtDataSourceItemCount.Text);

                    if (_listViewBase.ItemsSource == _colRecipes)
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

                        _listViewBase.ItemsSource = _colRecipes;
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

        private void ChkListViewBaseAllowDrop_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.AllowDrop = (bool)chkListViewBaseAllowDrop.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseCanDragItems_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.CanDragItems = (bool)chkListViewBaseCanDragItems.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseCanReorderItems_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.CanReorderItems = (bool)chkListViewBaseCanReorderItems.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseIsSwipeEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.IsSwipeEnabled = (bool)chkListViewBaseIsSwipeEnabled.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseIsItemClickEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.IsItemClickEnabled = (bool)chkListViewBaseIsItemClickEnabled.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseSingleSelectionFollowsFocus_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    _listViewBase.SingleSelectionFollowsFocus = (bool)chkListViewBaseSingleSelectionFollowsFocus.IsChecked;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkListViewBaseProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svListViewBaseProperties != null)
                svListViewBaseProperties.Visibility = Visibility.Visible;
        }

        private void ChkListViewBaseProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svListViewBaseProperties != null)
                svListViewBaseProperties.Visibility = Visibility.Collapsed;
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
