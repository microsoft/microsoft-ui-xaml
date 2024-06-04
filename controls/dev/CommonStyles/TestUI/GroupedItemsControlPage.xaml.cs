// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Text;
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
    public sealed partial class GroupedItemsControlPage : TestPage
    {
        private GroupedItemsControlViewModel _groupedItemsControlViewModel = new GroupedItemsControlViewModel();
        private CollectionViewSource _cvs = new CollectionViewSource();
        private DataTemplate[] _headerTemplates = new DataTemplate[3];
        private DataTemplate[] _itemTemplates = new DataTemplate[3];

        public GroupedItemsControlPage()
        {
            this.InitializeComponent();

            _cvs.Source = _groupedItemsControlViewModel.Items;
            _cvs.IsSourceGrouped = true;
            _cvs.ItemsPath = new PropertyPath("Items");

            Loaded += GroupedItemsControlPage_Loaded;
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            itemsControl.ItemsSource = _cvs.View;

            if (cmbItemsControlItemsPanelType != null)
            {
                cmbItemsControlItemsPanelType.SelectedIndex = 0;
            }

            base.OnNavigatedTo(e);
        }

        private void GroupedItemsControlPage_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelType();
            UpdateItemsControlProperties();
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
                if (itemsControl != null && cmbItemsControlItemsPanelType != null)
                {
                    switch (cmbItemsControlItemsPanelType.SelectedIndex)
                    {
                        case 0: // Use the wrapGridItemsPanelTemplate resource
                            itemsControl.ItemsPanel = Resources["itemsWrapGridItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Visible);
                            break;
                        case 1: // Use the stackPanelItemsPanelTemplate resource
                            itemsControl.ItemsPanel = Resources["itemsStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Collapsed);
                            break;
                        case 2: // Use the virtualizingStackPanelItemsPanelTemplate resource
                            itemsControl.ItemsPanel = Resources["virtualizingStackPanelItemsPanelTemplate"] as ItemsPanelTemplate;
                            UpdateItemsWrapGridUIVisibility(Visibility.Collapsed);
                            break;
                    }

                    // itemsControl.ItemsPanelRoot is only updated asynchronously.
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
        }

        private void UpdateItemsControlProperties()
        {
            UpdateItemsControlIsTabStop();
            UpdateItemsControlHeaderTemplate();
            UpdateItemsControlItemTemplate();
            UpdateItemsControlTabNavigation();
            UpdateItemsControlXYFocusKeyboardNavigation();
        }

        private void UpdateItemsControlIsTabStop()
        {
            if (itemsControl != null && chkItemsControlIsTabStop != null)
            {
                chkItemsControlIsTabStop.IsChecked = itemsControl.IsTabStop;
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

        private void UpdateItemsControlHeaderTemplate()
        {
            try
            {
                if (itemsControl != null && cmbItemsControlHeaderTemplate != null)
                {
                    if (cmbItemsControlHeaderTemplate.SelectedIndex == 0)
                    {
                        if (itemsControl.GroupStyle.Count > 0)
                        {
                            itemsControl.GroupStyle[0].HeaderTemplate = null;
                        }
                    }
                    else
                    {
                        int templateIndex = cmbItemsControlHeaderTemplate.SelectedIndex - 1;

                        if (_headerTemplates[templateIndex] == null)
                        {
                            _headerTemplates[templateIndex] = Resources["headerTemplate" + cmbItemsControlHeaderTemplate.SelectedIndex.ToString()] as DataTemplate;
                        }

                        if (itemsControl.GroupStyle.Count == 0)
                        {
                            itemsControl.GroupStyle.Add(new GroupStyle());
                        }

                        itemsControl.GroupStyle[0].HeaderTemplate = _headerTemplates[templateIndex];
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
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

        private void UpdateItemsPanelOrientation()
        {
            try
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
                if (itemsControl != null && cmbItemsPanelGroupHeaderPlacement != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        cmbItemsPanelGroupHeaderPlacement.SelectedIndex = (int)itemsWrapGrid.GroupHeaderPlacement;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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

        private void UpdateItemsPanelGroupPadding()
        {
            try
            {
                if (itemsControl != null && txtItemsPanelGroupPadding != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        txtItemsPanelGroupPadding.Text = itemsWrapGrid.GroupPadding.ToString();
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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
                if (itemsControl != null && chkItemsPanelAreStickyGroupHeadersEnabled != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked = itemsWrapGrid.AreStickyGroupHeadersEnabled;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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
                if (itemsControl != null && txtItemsWrapGridCacheLength != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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
                if (itemsControl != null && txtItemsWrapGridItemWidth != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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
                if (itemsControl != null && txtItemsWrapGridItemHeight != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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

        private void CmbItemsControlHeaderTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateItemsControlHeaderTemplate();
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
                                    FontSize = 22.0,
                                    FontWeight = FontWeights.Bold,
                                    Margin = new Thickness(10.0),
                                    Text = "ItemsPresenter Header"
                                };
                                break;
                            case 2: /*Single TabStop*/
                                itemsPresenter.Header = new Button()
                                {
                                    Content = "ItemsPresenter Header",
                                    FontSize = 22.0,
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
                                    FontSize = 22.0,
                                    FontWeight = FontWeights.Bold,
                                    Margin = new Thickness(10.0),
                                    Text = "ItemsPresenter Footer"
                                };
                                break;
                            case 2: /*Single TabStop*/
                                itemsPresenter.Footer = new Button()
                                {
                                    Content = "ItemsPresenter Footer",
                                    FontSize = 22.0,
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

        private void ChkItemsPanelAreStickyGroupHeadersEnabled_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsControl != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.AreStickyGroupHeadersEnabled = (bool)chkItemsPanelAreStickyGroupHeadersEnabled.IsChecked;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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
                if (itemsControl != null && cmbItemsPanelOrientation != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        if (itemsWrapGrid != null && itemsWrapGrid.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                        {
                            itemsControl.ItemsSource = null;

                            itemsWrapGrid.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                        }
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

                        if (itemsStackPanel != null)
                        {
                            if (itemsStackPanel != null && itemsStackPanel.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                            {
                                itemsControl.ItemsSource = null;

                                itemsStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                        else
                        {
                            VirtualizingStackPanel virtualizingStackPanel = itemsControl.ItemsPanelRoot as VirtualizingStackPanel;

                            if (virtualizingStackPanel != null && virtualizingStackPanel.Orientation != (Orientation)cmbItemsPanelOrientation.SelectedIndex)
                            {
                                itemsControl.ItemsSource = null;

                                virtualizingStackPanel.Orientation = (Orientation)cmbItemsPanelOrientation.SelectedIndex;
                            }
                        }
                    }

                    if (itemsControl.ItemsSource != _cvs.View)
                    {
                        itemsControl.ItemsSource = _cvs.View;
                    }
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
                if (itemsControl != null && cmbItemsPanelGroupHeaderPlacement != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.GroupHeaderPlacement = (GroupHeaderPlacement)cmbItemsPanelGroupHeaderPlacement.SelectedIndex;
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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

        private void BtnGetItemsPanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsPanelGroupPadding();
        }

        private void BtnSetItemsPanelGroupPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsControl != null && txtItemsPanelGroupPadding != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

                    if (itemsWrapGrid != null)
                    {
                        itemsWrapGrid.GroupPadding = GetThicknessFromString(txtItemsPanelGroupPadding.Text);
                    }
                    else
                    {
                        ItemsStackPanel itemsStackPanel = itemsControl.ItemsPanelRoot as ItemsStackPanel;

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
                if (itemsControl != null && txtItemsWrapGridCacheLength != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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
                if (itemsControl != null && txtItemsWrapGridItemWidth != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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
                if (itemsControl != null && txtItemsWrapGridItemHeight != null)
                {
                    ItemsWrapGrid itemsWrapGrid = itemsControl.ItemsPanelRoot as ItemsWrapGrid;

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
            if (itemsControl != null && itemsControl.ItemsSource != null)
            {
                itemsControl.ItemsSource = null;
            }
        }

        private void BtnSetItemsSource_Click(object sender, RoutedEventArgs e)
        {
            if (itemsControl != null && itemsControl.ItemsSource != _cvs.View)
            {
                itemsControl.ItemsSource = _cvs.View;
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

    public class GroupedItemsControlItem
    {
        public GroupedItemsControlItem() : this(">EmptyId<") { }

        public GroupedItemsControlItem(string id)
        {
            Id = id;
        }

        public string Id
        {
            get;
            set;
        }
    }

    public class GroupedItemsControlGroupItem
    {
        public GroupedItemsControlGroupItem() : this(">EmptyName<") { }

        public GroupedItemsControlGroupItem(string name)
        {
            Name = name;
            Items = new ObservableCollection<GroupedItemsControlItem>();
        }

        public string Name
        {
            get;
            set;
        }

        public ObservableCollection<GroupedItemsControlItem> Items
        {
            get;
            set;
        }
    }

    public class GroupedItemsControlViewModel
    {
        public GroupedItemsControlViewModel()
        {
            Items = new ObservableCollection<GroupedItemsControlGroupItem>();

            GroupedItemsControlGroupItem groupA = new GroupedItemsControlGroupItem("Group A");
            groupA.Items.Add(new GroupedItemsControlItem("idA1"));
            groupA.Items.Add(new GroupedItemsControlItem("idA2"));
            groupA.Items.Add(new GroupedItemsControlItem("idA3"));
            groupA.Items.Add(new GroupedItemsControlItem("idA4"));
            groupA.Items.Add(new GroupedItemsControlItem("idA5"));
            groupA.Items.Add(new GroupedItemsControlItem("idA6"));
            Items.Add(groupA);

            GroupedItemsControlGroupItem groupB = new GroupedItemsControlGroupItem("Group B");
            groupB.Items.Add(new GroupedItemsControlItem("idB1"));
            groupB.Items.Add(new GroupedItemsControlItem("idB2"));
            groupB.Items.Add(new GroupedItemsControlItem("idB3"));
            groupB.Items.Add(new GroupedItemsControlItem("idB4"));
            groupB.Items.Add(new GroupedItemsControlItem("idB5"));
            groupB.Items.Add(new GroupedItemsControlItem("idB6"));
            groupB.Items.Add(new GroupedItemsControlItem("idB7"));
            groupB.Items.Add(new GroupedItemsControlItem("idB8"));
            Items.Add(groupB);

            GroupedItemsControlGroupItem groupC = new GroupedItemsControlGroupItem("Group C");
            groupC.Items.Add(new GroupedItemsControlItem("idC1"));
            groupC.Items.Add(new GroupedItemsControlItem("idC2"));
            groupC.Items.Add(new GroupedItemsControlItem("idC3"));
            groupC.Items.Add(new GroupedItemsControlItem("idC4"));
            groupC.Items.Add(new GroupedItemsControlItem("idC5"));
            groupC.Items.Add(new GroupedItemsControlItem("idC6"));
            groupC.Items.Add(new GroupedItemsControlItem("idC7"));
            groupC.Items.Add(new GroupedItemsControlItem("idC8"));
            groupC.Items.Add(new GroupedItemsControlItem("idC9"));
            Items.Add(groupC);

            GroupedItemsControlGroupItem groupD = new GroupedItemsControlGroupItem("Group D");
            groupD.Items.Add(new GroupedItemsControlItem("idD1"));
            groupD.Items.Add(new GroupedItemsControlItem("idD2"));
            groupD.Items.Add(new GroupedItemsControlItem("idD3"));
            groupD.Items.Add(new GroupedItemsControlItem("idD4"));
            groupD.Items.Add(new GroupedItemsControlItem("idD5"));
            groupD.Items.Add(new GroupedItemsControlItem("idD6"));
            Items.Add(groupD);

            GroupedItemsControlGroupItem groupE = new GroupedItemsControlGroupItem("Group E");
            groupE.Items.Add(new GroupedItemsControlItem("idE1"));
            groupE.Items.Add(new GroupedItemsControlItem("idE2"));
            groupE.Items.Add(new GroupedItemsControlItem("idE3"));
            groupE.Items.Add(new GroupedItemsControlItem("idE4"));
            groupE.Items.Add(new GroupedItemsControlItem("idE5"));
            groupE.Items.Add(new GroupedItemsControlItem("idE6"));
            groupE.Items.Add(new GroupedItemsControlItem("idE7"));
            groupE.Items.Add(new GroupedItemsControlItem("idE8"));
            groupE.Items.Add(new GroupedItemsControlItem("idE9"));
            groupE.Items.Add(new GroupedItemsControlItem("idEA"));
            groupE.Items.Add(new GroupedItemsControlItem("idEB"));
            groupE.Items.Add(new GroupedItemsControlItem("idEC"));
            groupE.Items.Add(new GroupedItemsControlItem("idED"));
            groupE.Items.Add(new GroupedItemsControlItem("idEE"));
            groupE.Items.Add(new GroupedItemsControlItem("idEF"));
            Items.Add(groupE);

            GroupedItemsControlGroupItem groupF = new GroupedItemsControlGroupItem("Group F");
            groupF.Items.Add(new GroupedItemsControlItem("idF1"));
            groupF.Items.Add(new GroupedItemsControlItem("idF2"));
            groupF.Items.Add(new GroupedItemsControlItem("idF3"));
            Items.Add(groupF);

            GroupedItemsControlGroupItem groupG = new GroupedItemsControlGroupItem("Group G");
            groupG.Items.Add(new GroupedItemsControlItem("idG1"));
            groupG.Items.Add(new GroupedItemsControlItem("idG2"));
            groupG.Items.Add(new GroupedItemsControlItem("idG3"));
            groupG.Items.Add(new GroupedItemsControlItem("idG4"));
            groupG.Items.Add(new GroupedItemsControlItem("idG5"));
            groupG.Items.Add(new GroupedItemsControlItem("idG6"));
            groupG.Items.Add(new GroupedItemsControlItem("idG7"));
            groupG.Items.Add(new GroupedItemsControlItem("idG8"));
            groupG.Items.Add(new GroupedItemsControlItem("idG9"));
            groupG.Items.Add(new GroupedItemsControlItem("idGA"));
            groupG.Items.Add(new GroupedItemsControlItem("idGB"));
            groupG.Items.Add(new GroupedItemsControlItem("idGC"));
            groupG.Items.Add(new GroupedItemsControlItem("idGD"));
            groupG.Items.Add(new GroupedItemsControlItem("idGE"));
            groupG.Items.Add(new GroupedItemsControlItem("idGF"));
            groupG.Items.Add(new GroupedItemsControlItem("idGG"));
            groupG.Items.Add(new GroupedItemsControlItem("idGH"));
            groupG.Items.Add(new GroupedItemsControlItem("idGI"));
            groupG.Items.Add(new GroupedItemsControlItem("idGJ"));
            groupG.Items.Add(new GroupedItemsControlItem("idGK"));
            groupG.Items.Add(new GroupedItemsControlItem("idGL"));
            groupG.Items.Add(new GroupedItemsControlItem("idGM"));
            groupG.Items.Add(new GroupedItemsControlItem("idGN"));
            groupG.Items.Add(new GroupedItemsControlItem("idGO"));
            Items.Add(groupG);

            GroupedItemsControlGroupItem groupH = new GroupedItemsControlGroupItem("Group H");
            groupH.Items.Add(new GroupedItemsControlItem("idH1"));
            groupH.Items.Add(new GroupedItemsControlItem("idH2"));
            groupH.Items.Add(new GroupedItemsControlItem("idH3"));
            groupH.Items.Add(new GroupedItemsControlItem("idH4"));
            groupH.Items.Add(new GroupedItemsControlItem("idH5"));
            groupH.Items.Add(new GroupedItemsControlItem("idH6"));
            groupH.Items.Add(new GroupedItemsControlItem("idH7"));
            groupH.Items.Add(new GroupedItemsControlItem("idH8"));
            groupH.Items.Add(new GroupedItemsControlItem("idH9"));
            groupH.Items.Add(new GroupedItemsControlItem("idhA"));
            groupH.Items.Add(new GroupedItemsControlItem("idHB"));
            groupH.Items.Add(new GroupedItemsControlItem("idHC"));
            groupH.Items.Add(new GroupedItemsControlItem("idHD"));
            groupH.Items.Add(new GroupedItemsControlItem("idHE"));
            groupH.Items.Add(new GroupedItemsControlItem("idHF"));
            groupH.Items.Add(new GroupedItemsControlItem("idHG"));
            groupH.Items.Add(new GroupedItemsControlItem("idHH"));
            groupH.Items.Add(new GroupedItemsControlItem("idHI"));
            groupH.Items.Add(new GroupedItemsControlItem("idHJ"));
            groupH.Items.Add(new GroupedItemsControlItem("idHK"));
            groupH.Items.Add(new GroupedItemsControlItem("idHL"));
            groupH.Items.Add(new GroupedItemsControlItem("idHM"));
            groupH.Items.Add(new GroupedItemsControlItem("idHN"));
            groupH.Items.Add(new GroupedItemsControlItem("idHO"));
            groupH.Items.Add(new GroupedItemsControlItem("idhP"));
            groupH.Items.Add(new GroupedItemsControlItem("idHQ"));
            groupH.Items.Add(new GroupedItemsControlItem("idHR"));
            groupH.Items.Add(new GroupedItemsControlItem("idHS"));
            groupH.Items.Add(new GroupedItemsControlItem("idHT"));
            groupH.Items.Add(new GroupedItemsControlItem("idHU"));
            groupH.Items.Add(new GroupedItemsControlItem("idHV"));
            groupH.Items.Add(new GroupedItemsControlItem("idHW"));
            groupH.Items.Add(new GroupedItemsControlItem("idHX"));
            groupH.Items.Add(new GroupedItemsControlItem("idHY"));
            groupH.Items.Add(new GroupedItemsControlItem("idHZ"));
            Items.Add(groupH);

            GroupedItemsControlGroupItem groupI = new GroupedItemsControlGroupItem("Group I");
            groupI.Items.Add(new GroupedItemsControlItem("idI1"));
            groupI.Items.Add(new GroupedItemsControlItem("idI2"));
            groupI.Items.Add(new GroupedItemsControlItem("idI3"));
            Items.Add(groupI);
        }

        public ObservableCollection<GroupedItemsControlGroupItem> Items
        {
            get;
            set;
        }
    }
}
