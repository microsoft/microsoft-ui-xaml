// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using System;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public sealed partial class ListViewAnchoringPage : TestPage
    {
        [Flags]
        private enum QueuedOperationType
        {
            None = 0,
            PageShrinkSize = 1,
            SourceInsertItem = 2,
            SourceAppendItem = 4,
            SourceRemoveFirstItem = 8,
            SourceRemoveLastItem = 16
        }

        private GroupedListViewBaseViewModel _groupedListViewBaseViewModel;
        private CollectionViewSource _cvs;
        private ItemsPanelTemplate _itemsStackPanelTemplate;
        private DispatcherTimer _pageOperationTimer;
        private int _pageOperationCount;
        private QueuedOperationType _pageOperations = QueuedOperationType.None;

        public ListViewAnchoringPage()
        {
            this.InitializeComponent();

            _pageOperationTimer = new DispatcherTimer();
            _pageOperationTimer.Interval = new TimeSpan(0, 0, 0, 0, 125 /*ms*/);
            _pageOperationTimer.Tick += PageOperationTimer_Tick;

            Loaded += ListViewAnchoringPage_Loaded;
        }

        private void ListViewAnchoringPage_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView != null)
            {
                _itemsStackPanelTemplate = listView.ItemsPanel;
            }
        }

        private void PageOperationTimer_Tick(object sender, object e)
        {
            try
            {
                _pageOperationCount++;

                int maxOperationCount = 10;

                if (txtPageOperationCount != null)
                {
                    maxOperationCount = int.Parse(txtPageOperationCount.Text);
                }

                if ((_pageOperations & QueuedOperationType.PageShrinkSize) == QueuedOperationType.PageShrinkSize)
                {
                    Width = ActualWidth - 10.0;
                    Height = ActualHeight - 10.0;
                }

                if (listView != null)
                {
                    if ((_pageOperations & QueuedOperationType.SourceInsertItem) == QueuedOperationType.SourceInsertItem)
                    {
                        if (listView.ItemsSource == null)
                        {
                            listView.ItemsSource = new ObservableCollection<GroupedListViewBaseItem>()
                            {
                                new GroupedListViewBaseItem("ListView Item 0")
                            };
                        }
                        else
                        {
                            ObservableCollection<GroupedListViewBaseItem> source = listView.ItemsSource as ObservableCollection<GroupedListViewBaseItem>;
                            if (source != null)
                            {
                                source.Insert(0, new GroupedListViewBaseItem("ListView Item " + source.Count));
                            }
                            else
                            {
                                int count = _groupedListViewBaseViewModel.Items.Count;
                                string groupName = ((char)(65 + count)).ToString();
                                GroupedListViewBaseGroupItem group = new GroupedListViewBaseGroupItem("Group " + groupName);
                                group.Items.Add(new GroupedListViewBaseItem("id" + groupName + "1"));
                                _groupedListViewBaseViewModel.Items.Insert(0, group);
                            }
                        }
                    }

                    if ((_pageOperations & QueuedOperationType.SourceAppendItem) == QueuedOperationType.SourceAppendItem)
                    {
                        if (listView.ItemsSource == null)
                        {
                            listView.ItemsSource = new ObservableCollection<GroupedListViewBaseItem>()
                            {
                                new GroupedListViewBaseItem("ListView Item 0")
                            };
                        }
                        else
                        {
                            ObservableCollection<GroupedListViewBaseItem> source = listView.ItemsSource as ObservableCollection<GroupedListViewBaseItem>;
                            if (source != null)
                            {
                                source.Add(new GroupedListViewBaseItem("ListView Item " + source.Count));
                            }
                            else
                            {
                                int count = _groupedListViewBaseViewModel.Items.Count;
                                string groupName = ((char)(65 + count)).ToString();
                                GroupedListViewBaseGroupItem group = new GroupedListViewBaseGroupItem("Group " + groupName);
                                group.Items.Add(new GroupedListViewBaseItem("id" + groupName + "1"));
                                _groupedListViewBaseViewModel.Items.Add(group);
                            }
                        }
                    }

                    if (listView.ItemsSource != null &&
                        (_pageOperations & QueuedOperationType.SourceRemoveFirstItem) == QueuedOperationType.SourceRemoveFirstItem)
                    {
                        ObservableCollection<GroupedListViewBaseItem> source = listView.ItemsSource as ObservableCollection<GroupedListViewBaseItem>;
                        if (source != null)
                        {
                            if (source.Count > 0)
                            {
                                source.RemoveAt(0);
                            }
                        }
                        else
                        {
                            int count = _groupedListViewBaseViewModel.Items.Count;
                            if (count > 0)
                            {
                                GroupedListViewBaseGroupItem group = _groupedListViewBaseViewModel.Items[0];
                                group.Items.RemoveAt(0);
                                if (group.Items.Count == 0)
                                {
                                    _groupedListViewBaseViewModel.Items.RemoveAt(0);
                                }
                            }
                        }
                    }

                    if (listView.ItemsSource != null &&
                        (_pageOperations & QueuedOperationType.SourceRemoveLastItem) == QueuedOperationType.SourceRemoveLastItem)
                    {
                        ObservableCollection<GroupedListViewBaseItem> source = listView.ItemsSource as ObservableCollection<GroupedListViewBaseItem>;
                        if (source != null)
                        {
                            if (source.Count > 0)
                            {
                                source.RemoveAt(source.Count - 1);
                            }
                        }
                        else
                        {
                            int count = _groupedListViewBaseViewModel.Items.Count;
                            if (count > 0)
                            {
                                GroupedListViewBaseGroupItem group = _groupedListViewBaseViewModel.Items[count - 1];
                                group.Items.RemoveAt(group.Items.Count - 1);
                                if (group.Items.Count == 0)
                                {
                                    _groupedListViewBaseViewModel.Items.RemoveAt(count - 1);
                                }
                            }
                        }
                    }

                    if (_pageOperationCount == maxOperationCount)
                    {
                        _pageOperations = QueuedOperationType.None;

                        if (_pageOperationTimer != null)
                        {
                            _pageOperationTimer.Stop();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void CmbListViewItemsPanelType_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (listView != null && cmbListViewItemsPanelType != null)
                {
                    switch (cmbListViewItemsPanelType.SelectedIndex)
                    {
                        case 0:
                            if (_itemsStackPanelTemplate != null && _itemsStackPanelTemplate != listView.ItemsPanel)
                            {
                                listView.ItemsPanel = _itemsStackPanelTemplate;
                            }
                            break;
                        case 1: // Use the itemsWrapGridItemsPanelTemplate resource
                            listView.ItemsPanel = Resources["itemsWrapGridItemsPanelTemplate"] as ItemsPanelTemplate;
                            break;
                        case 2: // Use the stackPanelItemsPanelTemplate resource
                            listView.ItemsPanel = Resources["stackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetItemsStackPanelItemsUpdatingScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        cmbItemsStackPanelItemsUpdatingScrollMode.SelectedIndex = (int)itemsStackPanel.ItemsUpdatingScrollMode;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetItemsStackPanelItemsUpdatingScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.ItemsUpdatingScrollMode = (ItemsUpdatingScrollMode)cmbItemsStackPanelItemsUpdatingScrollMode.SelectedIndex;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetModernCollectionBasePanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtModernCollectionBasePanelGroupPadding != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        txtModernCollectionBasePanelGroupPadding.Text = itemsStackPanel.GroupPadding.ToString();
                    }
                    else
                    { 
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            txtModernCollectionBasePanelGroupPadding.Text = itemsWrapGrid.GroupPadding.ToString();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetModernCollectionBasePanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtModernCollectionBasePanelGroupPadding != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.GroupPadding = GetThicknessFromString(txtModernCollectionBasePanelGroupPadding.Text);
                    }
                    else
                    { 
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            itemsWrapGrid.GroupPadding = GetThicknessFromString(txtModernCollectionBasePanelGroupPadding.Text);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetModernCollectionBasePanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && cmbModernCollectionBasePanelVerticalAlignment != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        cmbModernCollectionBasePanelVerticalAlignment.SelectedIndex = (int)itemsStackPanel.VerticalAlignment;
                    }
                    else
                    {
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            cmbModernCollectionBasePanelVerticalAlignment.SelectedIndex = (int)itemsWrapGrid.VerticalAlignment;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetModernCollectionBasePanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && cmbModernCollectionBasePanelVerticalAlignment != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.VerticalAlignment = (VerticalAlignment)cmbModernCollectionBasePanelVerticalAlignment.SelectedIndex;
                    }
                    else
                    {
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            itemsWrapGrid.VerticalAlignment = (VerticalAlignment)cmbModernCollectionBasePanelVerticalAlignment.SelectedIndex;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetStackPanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && cmbStackPanelVerticalAlignment != null)
                {
                    StackPanel stackPanel = GetStackPanel();

                    if (stackPanel != null)
                    {
                        cmbStackPanelVerticalAlignment.SelectedIndex = (int)stackPanel.VerticalAlignment;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetStackPanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && cmbStackPanelVerticalAlignment != null)
                {
                    StackPanel stackPanel = GetStackPanel();

                    if (stackPanel != null)
                    {
                        stackPanel.VerticalAlignment = (VerticalAlignment)cmbStackPanelVerticalAlignment.SelectedIndex;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetModernCollectionBasePanelGroupHeaderPlacement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbModernCollectionBasePanelGroupHeaderPlacement != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        cmbModernCollectionBasePanelGroupHeaderPlacement.SelectedIndex = (int)itemsStackPanel.GroupHeaderPlacement;
                    }
                    else
                    {
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            cmbModernCollectionBasePanelGroupHeaderPlacement.SelectedIndex = (int)itemsWrapGrid.GroupHeaderPlacement;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetModernCollectionBasePanelGroupHeaderPlacement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbModernCollectionBasePanelGroupHeaderPlacement != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.GroupHeaderPlacement = (GroupHeaderPlacement)cmbModernCollectionBasePanelGroupHeaderPlacement.SelectedIndex;
                    }
                    else
                    {
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            itemsWrapGrid.GroupHeaderPlacement = (GroupHeaderPlacement)cmbModernCollectionBasePanelGroupHeaderPlacement.SelectedIndex;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void ChkModernCollectionBasePanelAreStickyGroupHeadersEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (chkModernCollectionBasePanelAreStickyGroupHeadersEnabled != null)
                {
                    ItemsStackPanel itemsStackPanel = GetItemsStackPanel();

                    if (itemsStackPanel != null)
                    {
                        itemsStackPanel.AreStickyGroupHeadersEnabled = (bool)chkModernCollectionBasePanelAreStickyGroupHeadersEnabled.IsChecked;
                    }
                    else
                    {
                        ItemsWrapGrid itemsWrapGrid = GetItemsWrapGrid();

                        if (itemsWrapGrid != null)
                        {
                            itemsWrapGrid.AreStickyGroupHeadersEnabled = (bool)chkModernCollectionBasePanelAreStickyGroupHeadersEnabled.IsChecked;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetListViewFlatItemsSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    listView.ItemsSource = new ObservableCollection<GroupedListViewBaseItem>()
                    {
                        new GroupedListViewBaseItem("ListView Item 0"),
                        new GroupedListViewBaseItem("ListView Item 1"),
                        new GroupedListViewBaseItem("ListView Item 2"),
                        new GroupedListViewBaseItem("ListView Item 3"),
                        new GroupedListViewBaseItem("ListView Item 4"),
                        new GroupedListViewBaseItem("ListView Item 5")
                    };
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetListViewGroupedItemsSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    _groupedListViewBaseViewModel = new GroupedListViewBaseViewModel();
                    _cvs = new CollectionViewSource();

                    _cvs.Source = _groupedListViewBaseViewModel.Items;
                    _cvs.IsSourceGrouped = true;
                    _cvs.ItemsPath = new PropertyPath("Items");

                    listView.ItemsSource = _cvs.View;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnClearListViewItemsSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    ObservableCollection<GroupedListViewBaseItem> source = listView.ItemsSource as ObservableCollection<GroupedListViewBaseItem>;
                    if (source != null)
                    {
                        source.Clear();
                    }
                    else if (_groupedListViewBaseViewModel != null)
                    {
                        _groupedListViewBaseViewModel.Items.Clear();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnResetListViewItemsSource_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null)
                {
                    listView.ItemsSource = null;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetListViewSize_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtListViewWidth != null && txtListViewHeight != null)
                {
                    txtListViewWidth.Text = "AW: " + listView.ActualWidth.ToString();
                    txtListViewHeight.Text = "AH: " + listView.ActualHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetScrollViewerVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtScrollViewerVerticalAnchorRatio != null)
                {
                    ScrollViewer scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(listView);

                    if (scrollViewer != null)
                    {
                        txtScrollViewerVerticalAnchorRatio.Text = scrollViewer.VerticalAnchorRatio.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetScrollViewerVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtScrollViewerVerticalAnchorRatio != null)
                {
                    ScrollViewer scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(listView);

                    if (scrollViewer != null)
                    {
                        scrollViewer.VerticalAnchorRatio = double.Parse(txtScrollViewerVerticalAnchorRatio.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetScrollViewerVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtScrollViewerVerticalOffset != null)
                {
                    ScrollViewer scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(listView);

                    if (scrollViewer != null)
                    {
                        txtScrollViewerVerticalOffset.Text = scrollViewer.VerticalOffset.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetScrollViewerVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtScrollViewerVerticalOffset != null)
                {
                    ScrollViewer scrollViewer = FindElementOfTypeInSubtree<ScrollViewer>(listView);

                    if (scrollViewer != null)
                    {
                        scrollViewer.ChangeView(null, double.Parse(txtScrollViewerVerticalOffset.Text), null, true);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetListViewItemActualHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtListViewItemIndex != null && txtListViewItemActualHeight != null)
                {
                    int listViewItemIndex = int.Parse(txtListViewItemIndex.Text);
                    ListViewItem listViewItem = listView.ContainerFromIndex(listViewItemIndex) as ListViewItem;
                    txtListViewItemActualHeight.Text = listViewItem.ActualHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetListViewItemHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtListViewItemIndex != null && txtListViewItemHeight != null)
                {
                    int listViewItemIndex = int.Parse(txtListViewItemIndex.Text);
                    ListViewItem listViewItem = listView.ContainerFromIndex(listViewItemIndex) as ListViewItem;
                    txtListViewItemHeight.Text = listViewItem.Height.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetListViewItemHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView != null && txtListViewItemIndex != null && txtListViewItemHeight != null)
                {
                    int listViewItemIndex = int.Parse(txtListViewItemIndex.Text);
                    ListViewItem listViewItem = listView.ContainerFromIndex(listViewItemIndex) as ListViewItem;
                    listViewItem.Height = double.Parse(txtListViewItemHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnGetPageSize_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtPageWidth != null && txtPageHeight != null)
                {
                    txtPageWidth.Text = ActualWidth.ToString();
                    txtPageHeight.Text = ActualHeight.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSetPageSize_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtPageWidth != null && txtPageHeight != null)
                {
                    Width = double.Parse(txtPageWidth.Text);
                    Height = double.Parse(txtPageHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnPageShrinkSize_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_pageOperationTimer != null)
                {
                    _pageOperations |= QueuedOperationType.PageShrinkSize;
                    _pageOperationCount = 0;
                    _pageOperationTimer.Start();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSourceInsertItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_pageOperationTimer != null)
                {
                    _pageOperations |= QueuedOperationType.SourceInsertItem;
                    _pageOperationCount = 0;
                    _pageOperationTimer.Start();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSourceAppendItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_pageOperationTimer != null)
                {
                    _pageOperations |= QueuedOperationType.SourceAppendItem;
                    _pageOperationCount = 0;
                    _pageOperationTimer.Start();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSourceRemoveFirstItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_pageOperationTimer != null)
                {
                    _pageOperations |= QueuedOperationType.SourceRemoveFirstItem;
                    _pageOperationCount = 0;
                    _pageOperationTimer.Start();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnSourceRemoveLastItem_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (_pageOperationTimer != null)
                {
                    _pageOperations |= QueuedOperationType.SourceRemoveLastItem;
                    _pageOperationCount = 0;
                    _pageOperationTimer.Start();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private ItemsStackPanel GetItemsStackPanel()
        {
            if (listView != null)
            {
                return listView.ItemsPanelRoot as ItemsStackPanel;
            }
            return null;
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

        private ItemsWrapGrid GetItemsWrapGrid()
        {
            if (listView != null)
            {
                return listView.ItemsPanelRoot as ItemsWrapGrid;
            }
            return null;
        }

        private StackPanel GetStackPanel()
        {
            if (listView != null)
            {
                return listView.ItemsPanelRoot as StackPanel;
            }
            return null;
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
