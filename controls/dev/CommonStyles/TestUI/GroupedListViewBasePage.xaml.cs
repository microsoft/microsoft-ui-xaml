// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
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
    public sealed partial class GroupedListViewBasePage : TestPage
    {
        private GroupedListViewBaseViewModel _groupedListViewBaseViewModel = new GroupedListViewBaseViewModel();
        private CollectionViewSource _cvs = new CollectionViewSource();
        private ListViewBase _listViewBase = null;
        private ScrollViewer _scrollViewer = null;

        public GroupedListViewBasePage()
        {
            this.InitializeComponent();

            _cvs.Source = _groupedListViewBaseViewModel.Items;
            _cvs.IsSourceGrouped = true;
            _cvs.ItemsPath = new PropertyPath("Items");

            Loaded += GroupedListViewBasePage_Loaded;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            bool useGridView = (bool)e.Parameter;

            tblGroupedListViewBasePageTitle.Text = useGridView ? "Grouped GridView" : "Grouped ListView";
            _listViewBase = useGridView ? gridView : listView;
            _listViewBase.Visibility = Visibility.Visible;
            _listViewBase.ItemsSource = _cvs.View;

            if (cmbListViewBaseItemsPanelType != null)
            {
                cmbListViewBaseItemsPanelType.SelectedIndex = useGridView ? 0 : 1;
            }

            base.OnNavigatedTo(e);
        }

        private void GroupedListViewBasePage_Loaded(object sender, RoutedEventArgs e)
        {
            _scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(_listViewBase);

            if (_scrollViewer != null)
            {
                _scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
            }

            UpdateItemsPanelType();
            UpdateListViewBaseProperties();
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

        private void ScrollViewer_ViewChanged(object sender, object e)
        {
            try
            {
                if (_scrollViewer != null && txtScrollViewerHorizontalOffset != null && txtScrollViewerVerticalOffset != null)
                {
                    txtScrollViewerHorizontalOffset.Text = _scrollViewer.HorizontalOffset.ToString();
                    txtScrollViewerVerticalOffset.Text = _scrollViewer.VerticalOffset.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
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

        private void UpdateItemsWrapGridUIVisibility(Visibility visibility)
        {
            tblItemsWrapGridProperties.Visibility = visibility;
            tblItemsWrapGridMaximumRowsOrColumns.Visibility = visibility;
            nbItemsWrapGridMaximumRowsOrColumns.Visibility = visibility;
            tblItemsWrapGridItemWidth.Visibility = visibility;
            txtItemsWrapGridItemWidth.Visibility = visibility;
            btnGetItemsWrapGridItemWidth.Visibility = visibility;
            btnSetItemsWrapGridItemWidth.Visibility = visibility;
            tblItemsWrapGridItemHeight.Visibility = visibility;
            txtItemsWrapGridItemHeight.Visibility = visibility;
            btnGetItemsWrapGridItemHeight.Visibility = visibility;
            btnSetItemsWrapGridItemHeight.Visibility = visibility;
        }

        private void UpdateItemsPanelType()
        {
            try
            {
                if (_listViewBase != null && cmbListViewBaseItemsPanelType != null)
                {
                    switch (cmbListViewBaseItemsPanelType.SelectedIndex)
                    {
                        case 0: // Use the wrapGridItemsPanelTemplate resource
                            _listViewBase.ItemsPanel = Resources["itemsWrapGridItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Visible);
                            break;
                        case 1: // Use the stackPanelItemsPanelTemplate resource
                            _listViewBase.ItemsPanel = Resources["itemsStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Collapsed);
                            break;
                        case 2: // Use the virtualizingStackPanelItemsPanelTemplate resource
                            _listViewBase.ItemsPanel = Resources["virtualizingStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Collapsed);
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
            UpdateItemsPanelGroupHeaderPlacement();
            UpdateItemsPanelMargin();
            UpdateItemsPanelGroupPadding();
            UpdateItemsPanelAreStickyGroupHeadersEnabled();
            UpdateItemsWrapGridCacheLength();
            UpdateItemsWrapGridItemWidth();
            UpdateItemsWrapGridItemHeight();

            // _listViewBase.ItemsPanelRoot is only updated asynchronously.
            _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerScrollOrientation);
            _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerProperties);
        }

        private void UpdateListViewBaseProperties()
        {
            UpdateListViewBaseSelectionMode();
            UpdateListViewBaseSingleSelectionFollowsFocus();
            UpdateListViewBaseTabNavigation();
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
            UpdateScrollViewerHorizontalScrollMode();
            UpdateScrollViewerVerticalScrollMode();
            UpdateScrollViewerHorizontalSnapPointsType();
            UpdateScrollViewerVerticalSnapPointsType();
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
            try
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

        private void UpdateItemsPanelGroupHeaderPlacement()
        {
            try
            {
                if (_listViewBase != null && cmbItemsPanelGroupHeaderPlacement != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        cmbItemsPanelGroupHeaderPlacement.SelectedIndex = (int)itemsWrapGrid.GroupHeaderPlacement;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            cmbItemsPanelGroupHeaderPlacement.SelectedIndex = (int)itemsStackPanel.GroupHeaderPlacement;
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

        private void UpdateItemsPanelGroupPadding()
        {
            try
            {
                if (_listViewBase != null && txtItemsPanelGroupPadding != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        txtItemsPanelGroupPadding.Text = itemsWrapGrid.GroupPadding.ToString();
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            txtItemsPanelGroupPadding.Text = itemsStackPanel.GroupPadding.ToString();
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

        private void UpdateItemsPanelAreStickyGroupHeadersEnabled()
        {
            try
            {
                if (_listViewBase != null && chkItemsPanelAreStickyGroupHeadersEnabled != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked = itemsWrapGrid.AreStickyGroupHeadersEnabled;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked = itemsStackPanel.AreStickyGroupHeadersEnabled;
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

        private void UpdateItemsWrapGridCacheLength()
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridCacheLength != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        txtItemsWrapGridCacheLength.Text = itemsWrapGrid.CacheLength.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsWrapGridItemWidth()
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridItemWidth != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        txtItemsWrapGridItemWidth.Text = itemsWrapGrid.ItemWidth.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsWrapGridItemHeight()
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridItemHeight != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        txtItemsWrapGridItemHeight.Text = itemsWrapGrid.ItemHeight.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
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

                    // Let the ScrollViewer layout itself before reading its updated properties.
                    _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerProperties);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerHorizontalScrollMode()
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerHorizontalScrollMode != null)
                {
                    cmbScrollViewerHorizontalScrollMode.SelectedIndex = (int)_scrollViewer.HorizontalScrollMode;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewerVerticalScrollMode()
        {
            try
            {
                if (_scrollViewer != null && cmbScrollViewerVerticalScrollMode != null)
                {
                    cmbScrollViewerVerticalScrollMode.SelectedIndex = (int)_scrollViewer.VerticalScrollMode;
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

        private void CmbListViewBaseItemsPanelType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateItemsPanelType();
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
                        case 2: /*Non-focusable*/
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
                        case 2: /*Non-focusable*/
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

        private void ChkItemsPanelAreStickyGroupHeadersEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.AreStickyGroupHeadersEnabled = (bool)chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            itemsStackPanel.AreStickyGroupHeadersEnabled = (bool)chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked;
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

        private void BtnGetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelOrientation();
        }

        private void BtnSetItemsPanelOrientation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && cmbItemsPanelOrientation != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        if (itemsWrapGrid != null && itemsWrapGrid.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                        {
                            _listViewBase.ItemsSource = null;

                            itemsWrapGrid.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                        }
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            if (itemsStackPanel != null && itemsStackPanel.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                            {
                                _listViewBase.ItemsSource = null;

                                itemsStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                        else
                        {
                            VirtualizingStackPanel virtualizingStackPanel = _listViewBase.ItemsPanelRoot as VirtualizingStackPanel;

                            if (virtualizingStackPanel != null && virtualizingStackPanel.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                            {
                                _listViewBase.ItemsSource = null;

                                virtualizingStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                    }

                    if (_listViewBase.ItemsSource != _cvs.View)
                    {
                        _listViewBase.ItemsSource = _cvs.View;
                    }

                    // _listViewBase.ItemsPanelRoot is only updated asynchronously.
                    _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerScrollOrientation);
                    _ = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, UpdateScrollViewerProperties);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetItemsPanelGroupHeaderPlacement_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelGroupHeaderPlacement();
        }

        private void BtnSetItemsPanelGroupHeaderPlacement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && cmbItemsPanelGroupHeaderPlacement != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.GroupHeaderPlacement = (GroupHeaderPlacement)cmbItemsPanelGroupHeaderPlacement.SelectedIndex;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            itemsStackPanel.GroupHeaderPlacement = (GroupHeaderPlacement)cmbItemsPanelGroupHeaderPlacement.SelectedIndex;
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

        private void BtnGetItemsPanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelGroupPadding();
        }

        private void BtnSetItemsPanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && txtItemsPanelGroupPadding != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.GroupPadding = GetThicknessFromString(txtItemsPanelGroupPadding.Text);
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = _listViewBase.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            itemsStackPanel.GroupPadding = GetThicknessFromString(txtItemsPanelGroupPadding.Text);
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

        private void BtnGetItemsWrapGridCacheLength_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsWrapGridCacheLength();
        }

        private void BtnSetItemsWrapGridCacheLength_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridCacheLength != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.CacheLength = double.Parse(txtItemsWrapGridCacheLength.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetItemsWrapGridItemWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsWrapGridItemWidth();
        }

        private void BtnSetItemsWrapGridItemWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridItemWidth != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.ItemWidth = double.Parse(txtItemsWrapGridItemWidth.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetItemsWrapGridItemHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsWrapGridItemHeight();
        }

        private void BtnSetItemsWrapGridItemHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_listViewBase != null && txtItemsWrapGridItemHeight != null)
                {
                    ItemsWrapGrid itemsWrapGrid = _listViewBase.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.ItemHeight = double.Parse(txtItemsWrapGridItemHeight.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnResetItemsSource_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && _listViewBase.ItemsSource != null)
            {
                _listViewBase.ItemsSource = null;
            }
        }

        private void BtnSetItemsSource_Click(object sender, RoutedEventArgs e)
        {
            if (_listViewBase != null && _listViewBase.ItemsSource != _cvs.View)
            {
                _listViewBase.ItemsSource = _cvs.View;
            }
        }

        private void BtnGetScrollViewerHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerHorizontalScrollMode();
        }

        private void BtnSetScrollViewerHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            if (_scrollViewer != null && cmbScrollViewerHorizontalScrollMode != null)
            {
                _scrollViewer.HorizontalScrollMode = (ScrollMode)cmbScrollViewerHorizontalScrollMode.SelectedIndex;
            }
        }

        private void BtnGetScrollViewerVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewerVerticalScrollMode();
        }

        private void BtnSetScrollViewerVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            if (_scrollViewer != null && cmbScrollViewerVerticalScrollMode != null)
            {
                _scrollViewer.VerticalScrollMode = (ScrollMode)cmbScrollViewerVerticalScrollMode.SelectedIndex;
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

        private void ChkProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (svProperties != null)
                svProperties.Visibility = Visibility.Visible;
        }

        private void ChkProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svProperties != null)
                svProperties.Visibility = Visibility.Collapsed;
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

    public class GroupedListViewBaseItem
    {
        public GroupedListViewBaseItem() : this(">EmptyId<") { }

        public GroupedListViewBaseItem(string id)
        {
            Id = id;
        }

        public string Id
        {
            get;
            set;
        }
    }

    public class GroupedListViewBaseGroupItem
    {
        public GroupedListViewBaseGroupItem() : this(">EmptyName<") { }

        public GroupedListViewBaseGroupItem(string name)
        {
            Name = name;
            Items = new ObservableCollection<GroupedListViewBaseItem>();
        }

        public string Name
        {
            get;
            set;
        }

        public ObservableCollection<GroupedListViewBaseItem> Items
        {
            get;
            set;
        }
    }

    public class GroupedListViewBaseViewModel
    {
        public GroupedListViewBaseViewModel()
        {
            Items = new ObservableCollection<GroupedListViewBaseGroupItem>();

            GroupedListViewBaseGroupItem groupA = new GroupedListViewBaseGroupItem("Group A");
            groupA.Items.Add(new GroupedListViewBaseItem("idA1"));
            groupA.Items.Add(new GroupedListViewBaseItem("idA2"));
            groupA.Items.Add(new GroupedListViewBaseItem("idA3"));
            groupA.Items.Add(new GroupedListViewBaseItem("idA4"));
            groupA.Items.Add(new GroupedListViewBaseItem("idA5"));
            groupA.Items.Add(new GroupedListViewBaseItem("idA6"));
            Items.Add(groupA);

            GroupedListViewBaseGroupItem groupB = new GroupedListViewBaseGroupItem("Group B");
            groupB.Items.Add(new GroupedListViewBaseItem("idB1"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB2"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB3"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB4"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB5"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB6"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB7"));
            groupB.Items.Add(new GroupedListViewBaseItem("idB8"));
            Items.Add(groupB);

            GroupedListViewBaseGroupItem groupC = new GroupedListViewBaseGroupItem("Group C");
            groupC.Items.Add(new GroupedListViewBaseItem("idC1"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC2"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC3"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC4"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC5"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC6"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC7"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC8"));
            groupC.Items.Add(new GroupedListViewBaseItem("idC9"));
            Items.Add(groupC);

            GroupedListViewBaseGroupItem groupD = new GroupedListViewBaseGroupItem("Group D");
            groupD.Items.Add(new GroupedListViewBaseItem("idD1"));
            groupD.Items.Add(new GroupedListViewBaseItem("idD2"));
            groupD.Items.Add(new GroupedListViewBaseItem("idD3"));
            groupD.Items.Add(new GroupedListViewBaseItem("idD4"));
            groupD.Items.Add(new GroupedListViewBaseItem("idD5"));
            groupD.Items.Add(new GroupedListViewBaseItem("idD6"));
            Items.Add(groupD);

            GroupedListViewBaseGroupItem groupE = new GroupedListViewBaseGroupItem("Group E");
            groupE.Items.Add(new GroupedListViewBaseItem("idE1"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE2"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE3"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE4"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE5"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE6"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE7"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE8"));
            groupE.Items.Add(new GroupedListViewBaseItem("idE9"));
            groupE.Items.Add(new GroupedListViewBaseItem("idEA"));
            groupE.Items.Add(new GroupedListViewBaseItem("idEB"));
            groupE.Items.Add(new GroupedListViewBaseItem("idEC"));
            groupE.Items.Add(new GroupedListViewBaseItem("idED"));
            groupE.Items.Add(new GroupedListViewBaseItem("idEE"));
            groupE.Items.Add(new GroupedListViewBaseItem("idEF"));
            Items.Add(groupE);

            GroupedListViewBaseGroupItem groupF = new GroupedListViewBaseGroupItem("Group F");
            groupF.Items.Add(new GroupedListViewBaseItem("idF1"));
            groupF.Items.Add(new GroupedListViewBaseItem("idF2"));
            groupF.Items.Add(new GroupedListViewBaseItem("idF3"));
            Items.Add(groupF);

            GroupedListViewBaseGroupItem groupG = new GroupedListViewBaseGroupItem("Group G");
            groupG.Items.Add(new GroupedListViewBaseItem("idG1"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG2"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG3"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG4"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG5"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG6"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG7"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG8"));
            groupG.Items.Add(new GroupedListViewBaseItem("idG9"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGA"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGB"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGC"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGD"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGE"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGF"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGG"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGH"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGI"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGJ"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGK"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGL"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGM"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGN"));
            groupG.Items.Add(new GroupedListViewBaseItem("idGO"));
            Items.Add(groupG);

            GroupedListViewBaseGroupItem groupH = new GroupedListViewBaseGroupItem("Group H");
            groupH.Items.Add(new GroupedListViewBaseItem("idH1"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH2"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH3"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH4"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH5"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH6"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH7"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH8"));
            groupH.Items.Add(new GroupedListViewBaseItem("idH9"));
            groupH.Items.Add(new GroupedListViewBaseItem("idhA"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHB"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHC"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHD"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHE"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHF"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHG"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHH"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHI"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHJ"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHK"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHL"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHM"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHN"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHO"));
            groupH.Items.Add(new GroupedListViewBaseItem("idhP"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHQ"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHR"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHS"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHT"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHU"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHV"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHW"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHX"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHY"));
            groupH.Items.Add(new GroupedListViewBaseItem("idHZ"));
            Items.Add(groupH);

            GroupedListViewBaseGroupItem groupI = new GroupedListViewBaseGroupItem("Group I");
            groupI.Items.Add(new GroupedListViewBaseItem("idI1"));
            groupI.Items.Add(new GroupedListViewBaseItem("idI2"));
            groupI.Items.Add(new GroupedListViewBaseItem("idI3"));
            Items.Add(groupI);
        }

        public ObservableCollection<GroupedListViewBaseGroupItem> Items
        {
            get;
            set;
        }
    }
}
