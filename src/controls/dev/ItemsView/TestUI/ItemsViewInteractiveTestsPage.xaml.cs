// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp.Samples.Model;

namespace MUXControlsTestApp
{
    public sealed partial class ItemsViewInteractiveTestsPage : TestPage
    {
        private string _lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";
        private List<string> _fullLogs = new List<string>();
        private int _scrollViewResetCorrelationId = -1;
        private int _scrollViewOffsetChangeCorrelationId = -1;
        private int _scrollViewZoomFactorChangeCorrelationId = -1;
        private ObservableCollection<Entity> _colEntities = null;
        private List<Entity> _lstEntities = null;
        private List<Entity> _lstLargeEntities = null;
        private DataTemplate[] _entityTemplates = new DataTemplate[6];
        private LinedFlowLayout _linedFlowLayout = null;
        private StackLayout _stackLayout = null;
        private UniformGridLayout _uniformGridLayout = null;
        private UIElement _oldFocusedElement = null;
        private LinedFlowLayoutItemCollectionTransitionProvider _linedFlowLayoutItemCollectionTransitionProvider = null;

        public ItemsViewInteractiveTestsPage()
        {
            this.InitializeComponent();

            Loaded += ItemsViewInteractiveTestsPage_Loaded;
            KeyDown += ItemsViewInteractiveTestsPage_KeyDown;

            btnGetCurrentItemIndex.GettingFocus += BtnGetCurrentItemIndex_GettingFocus;
        }

        private void ItemsViewInteractiveTestsPage_Loaded(object sender, RoutedEventArgs e)
        {
            itemsView.ItemInvoked += ItemsView_ItemInvoked;

            UpdateItemsViewLayout();
            UpdateItemsViewIsItemInvokedEnabled();
            UpdateItemsViewSelectionMode();
            UpdateItemsInfoRequestedCheckBoxes();
            UpdateAppAnimatorCheckBox();

            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(this.itemsView);

            if (scrollView != null)
            {
                scrollView.StateChanged += ScrollView_StateChanged;
                scrollView.ViewChanged += ScrollView_ViewChanged;
                scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
                scrollView.ZoomCompleted += ScrollView_ZoomCompleted;

                ScrollPresenter scrollPresenter = scrollView.ScrollPresenter;

                if (scrollPresenter != null)
                {
                    AutomationProperties.SetAutomationId(scrollPresenter, "PART_ScrollPresenter");
                    AutomationProperties.SetName(scrollPresenter, "PART_ScrollPresenter");
                }
            }
        }

        private void ItemsViewInteractiveTestsPage_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.G)
            {
                GetFullLog();
            }
            else if (e.Key == Windows.System.VirtualKey.C)
            {
                ClearFullLog();
            }
        }

        private void ChkHandleItemsInfoRequested_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            UpdateItemsInfoRequestedHandler();
        }

        private void ChkUseFastPath_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            UpdateItemsInfoRequestedHandler();
        }

        private void ChkUseAppAnimator_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            UpdateAppAnimator();
        }

        private void ChkLogItemsRepeaterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ItemsRepeater",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ItemsRepeater",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsRepeaterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ItemsRepeater",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ItemsRepeater",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogItemsRepeaterMessages.IsChecked == false && chkLogItemsViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogItemsRepeaterMessages.IsChecked == false && chkLogItemsViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ItemsView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ItemsView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ItemsView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ItemsView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void CmbItemTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsView != null)
                {
                    if (cmbItemTemplate.SelectedIndex == 0)
                    {
                        itemsView.ItemTemplate = null;
                    }
                    else
                    {
                        int templateIndex = cmbItemTemplate.SelectedIndex - 1;

                        if (_entityTemplates[templateIndex] == null)
                        {
                            _entityTemplates[templateIndex] = Resources["entityTemplate" + cmbItemTemplate.SelectedIndex.ToString()] as DataTemplate;
                        }

                        itemsView.ItemTemplate = _entityTemplates[templateIndex];
                    }

                    UpdateLinedFlowLayoutLineHeight();
                    UpdateItemsInfoRequestedCheckBoxes();
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void CmbItemsSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsView != null)
                {
                    switch (cmbItemsSource.SelectedIndex)
                    {
                        case 0:
                            itemsView.ItemsSource = null;
                            break;
                        case 1:
                            if (_lstEntities == null)
                            {
                                _lstEntities = new List<Entity>(
                                                Enumerable.Range(0, 50).Select(k =>
                                                    new Entity
                                                    {
                                                        ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                                        Description = k + " - " + _lorem.Substring(0, k)
                                                    }));
                            }
                            itemsView.ItemsSource = _lstEntities;
                            break;
                        case 2:
                            if (_lstLargeEntities == null)
                            {
                                _lstLargeEntities = new List<Entity>(
                                                Enumerable.Range(0, 1000).Select(k =>
                                                    new Entity
                                                    {
                                                        ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                                        Description = k + " - " + _lorem.Substring(0, k % 50 + 1)
                                                    }));
                            }
                            itemsView.ItemsSource = _lstLargeEntities;
                            break;
                        case 3:
                            if (_colEntities == null)
                            {
                                _colEntities = new ObservableCollection<Entity>(
                                                Enumerable.Range(0, 25).Select(k =>
                                                    new Entity
                                                    {
                                                        ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                                                        Description = k + " - " + _lorem.Substring(0, 2 * k)
                                                    }));
                            }
                            itemsView.ItemsSource = _colEntities;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void CmbLayout_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsView != null)
                {
                    switch (cmbLayout.SelectedIndex)
                    {
                        case 0:
                            itemsView.Layout = null;
                            break;
                        case 1:
                            if (_linedFlowLayout == null)
                            {
                                _linedFlowLayout = Resources["linedFlowLayout"] as LinedFlowLayout;
                            }
                            itemsView.Layout = _linedFlowLayout;
                            break;
                        case 2:
                            itemsView.Layout = _stackLayout;
                            break;
                        case 3:
                            if (_uniformGridLayout == null)
                            {
                                _uniformGridLayout = Resources["uniformGridLayout"] as UniformGridLayout;
                            }
                            itemsView.Layout = _uniformGridLayout;
                            break;
                    }

                    UpdateLinedFlowLayoutLineHeight();
                    UpdateItemsInfoRequestedCheckBoxes();
                    UpdateAppAnimatorCheckBox();
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void ChkIsItemInvokedEnabled_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.IsItemInvokedEnabled = true;
            }
        }

        private void ChkIsItemInvokedEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.IsItemInvokedEnabled = false;
            }
        }

        private void CmbSelectionMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsView != null && cmbSelectionMode != null && itemsView.SelectionMode != (ItemsViewSelectionMode)cmbSelectionMode.SelectedIndex)
                {
                    itemsView.SelectionMode = (ItemsViewSelectionMode)cmbSelectionMode.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void ItemsView_ItemInvoked(ItemsView sender, ItemsViewItemInvokedEventArgs args)
        {
            _fullLogs.Add($"ItemsView.ItemInvoked InvokedItem={args.InvokedItem}");
        }

        private void LinedFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            _fullLogs.Add($"LinedFlowLayout.ItemsInfoRequested ItemsRangeStartIndex={args.ItemsRangeStartIndex}, ItemsRangeRequestedLength={args.ItemsRangeRequestedLength}");

            if (cmbItemTemplate.SelectedIndex != 5 || itemsView.ItemsSource == null)
            {
                return;
            }

            args.MinWidth = 80.0;

            double[] desiredAspectRatios = new double[chkUseFastPath.IsChecked == true ? (itemsView.ItemsSource as IReadOnlyCollection<Entity>).Count : args.ItemsRangeRequestedLength];

            if (chkUseFastPath.IsChecked == true)
            {
                args.ItemsRangeStartIndex = 0;
            }

            for (int index = 0; index < desiredAspectRatios.Length; index++)
            {
                desiredAspectRatios[index] = 1.0;
            }

            args.SetDesiredAspectRatios(desiredAspectRatios);
        }

        private void ScrollView_StateChanged(ScrollView scrollView, object args)
        {
            this.txtScrollViewState.Text = scrollView.State.ToString();
            _fullLogs.Add($"StateChanged S={scrollView.State.ToString()}");
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollView_ViewChanged(ScrollView scrollView, object args)
        {
            this.txtScrollViewHorizontalOffset.Text = scrollView.HorizontalOffset.ToString();
            this.txtScrollViewVerticalOffset.Text = scrollView.VerticalOffset.ToString();
            this.txtScrollViewZoomFactor.Text = scrollView.ZoomFactor.ToString();
            _fullLogs.Add($"ViewChanged H={this.txtScrollViewHorizontalOffset.Text}, V={this.txtScrollViewVerticalOffset.Text}, ZF={this.txtScrollViewZoomFactor.Text}");
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollView_ScrollCompleted(ScrollView scrollView, ScrollingScrollCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

            _fullLogs.Add($"ScrollCompleted. OffsetsChangeCorrelationId={args.CorrelationId}, Result={result}");
            chkLogUpdated.IsChecked = false;

            if (args.CorrelationId == _scrollViewOffsetChangeCorrelationId)
            {
                this.txtStatus.Text = "ScrollView scrolled";
                _scrollViewOffsetChangeCorrelationId = -1;
            }
        }

        private void ScrollView_ZoomCompleted(ScrollView scrollView, ScrollingZoomCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

            _fullLogs.Add($"ZoomCompleted. ZoomFactorChangeCorrelationId={args.CorrelationId}, Result={result}");
            chkLogUpdated.IsChecked = false;

            if (args.CorrelationId == _scrollViewZoomFactorChangeCorrelationId)
            {
                this.txtStatus.Text = "ScrollView zoomed";
                _scrollViewZoomFactorChangeCorrelationId = -1;
            }
            else if (args.CorrelationId == _scrollViewResetCorrelationId)
            {
                this.txtStatus.Text = "ScrollView reset";
                _scrollViewResetCorrelationId = -1;
            }
        }

        private void BtnGetCurrentItemIndex_GettingFocus(UIElement sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            _oldFocusedElement = args.OldFocusedElement as UIElement;
        }

        private void BtnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            GetFullLog();
        }

        private void BtnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            ClearFullLog();
        }

        private void BtnScrollViewScrollTo_Click(object sender, RoutedEventArgs e)
        {
            this.txtStatus.Text = "Scrolling ScrollView ...";

            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            double horizontalOffset = double.Parse(txtScrollViewHorizontalOffset.Text);
            double verticalOffset = double.Parse(txtScrollViewVerticalOffset.Text);

            int viewChangeCorrelationId = scrollView.ScrollTo(horizontalOffset, verticalOffset, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            _fullLogs.Add($"ScrollTo requested. Id={viewChangeCorrelationId}");

            chkLogUpdated.IsChecked = false;
            _scrollViewOffsetChangeCorrelationId = viewChangeCorrelationId;
        }

        private void BtnScrollViewZoomTo_Click(object sender, RoutedEventArgs e)
        {
            this.txtStatus.Text = "Zooming ScrollView ...";

            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            float zoomFactor = float.Parse(txtScrollViewZoomFactor.Text);

            int viewChangeCorrelationId = scrollView.ZoomTo(zoomFactor, System.Numerics.Vector2.Zero, new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            _fullLogs.Add($"ZoomTo requested. Id={viewChangeCorrelationId}");

            chkLogUpdated.IsChecked = false;
            _scrollViewZoomFactorChangeCorrelationId = viewChangeCorrelationId;
        }

        private void BtnResetScrollView_Click(object sender, RoutedEventArgs e)
        {
            this.txtStatus.Text = "Resetting ScrollView ...";

            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            int viewChangeCorrelationId = scrollView.ScrollTo(0.0, 0.0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            _fullLogs.Add($"ScrollTo requested. Id={viewChangeCorrelationId}");

            viewChangeCorrelationId = scrollView.ZoomTo(1.0f, System.Numerics.Vector2.Zero, new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            _fullLogs.Add($"ZoomTo requested. Id={viewChangeCorrelationId}");

            chkLogUpdated.IsChecked = false;
            _scrollViewResetCorrelationId = viewChangeCorrelationId;
        }

        private void BtnGetCurrentItemIndex_Click(object sender, RoutedEventArgs e)
        {
            this.txtStatus.Text = "GetCurrentItemIndex ...";

            int currentItemIndex = itemsView.CurrentItemIndex;

            txtCurrentItemIndex.Text = currentItemIndex.ToString();

            this.txtStatus.Text = "GetCurrentItemIndex. Done.";

            if (_oldFocusedElement != null)
            {
                _oldFocusedElement.Focus(FocusState.Programmatic);
                _oldFocusedElement = null;
            }
        }

        private void BtnInsertItems_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtItemsSourceIndexes != null && txtStatus != null)
                {
                    txtStatus.Text = "InsertItems ...";

                    string[] indexes = txtItemsSourceIndexes.Text.Split(',');

                    foreach (string index in indexes)
                    {
                        InsertItem(int.Parse(index));
                    }

                    txtStatus.Text = "InsertItems. Done.";

                    if (_oldFocusedElement != null)
                    {
                        _oldFocusedElement.Focus(FocusState.Programmatic);
                        _oldFocusedElement = null;
                    }
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void BtnRemoveItems_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtItemsSourceIndexes != null && txtStatus != null)
                {
                    txtStatus.Text = "RemoveItems ...";

                    string[] indexes = txtItemsSourceIndexes.Text.Split(',');

                    foreach (string index in indexes)
                    {
                        RemoveItem(int.Parse(index));
                    }

                    txtStatus.Text = "RemoveItems. Done.";
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void InsertItem(int index)
        {
            try
            {
                if (itemsView != null && cmbItemsSource != null)
                {
                    switch (cmbItemsSource.SelectedIndex)
                    {
                        case 1:                            
                            if (_lstEntities != null)
                            {
                                int rank = _lstEntities.Count;
                                Entity entity = new Entity
                                {
                                    ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", rank % 8 + 1)),
                                    Description = rank + " - " + _lorem.Substring(0, rank)
                                };

                                _lstEntities.Insert(index, entity);
                            }
                            break;
                        case 2:
                            if (_lstLargeEntities != null)
                            {
                                int rank = _lstLargeEntities.Count;
                                Entity entity = new Entity
                                {
                                    ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", rank % 8 + 1)),
                                    Description = rank + " - " + _lorem.Substring(0, rank % 50 + 1)
                                };

                                _lstLargeEntities.Insert(index, entity);
                            }
                            break;
                        case 3:
                            if (_colEntities != null)
                            {
                                int rank = _colEntities.Count;
                                Entity entity = new Entity
                                {
                                    ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", rank % 8 + 1)),
                                    Description = rank + " - " + _lorem.Substring(0, 2 * rank)
                                };

                                _colEntities.Insert(index, entity);
                            }
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void RemoveItem(int index)
        {
            try
            {
                if (itemsView != null && cmbItemsSource != null)
                {
                    switch (cmbItemsSource.SelectedIndex)
                    {
                        case 1:
                            if (_lstEntities != null)
                            {
                                _lstEntities.RemoveAt(index);
                            }
                            break;
                        case 2:
                            if (_lstLargeEntities != null)
                            {
                                _lstLargeEntities.RemoveAt(index);
                            }
                            break;
                        case 3:
                            if (_colEntities != null)
                            {
                                _colEntities.RemoveAt(index);
                            }
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                _fullLogs.Add(ex.ToString());
            }
        }

        private void UpdateItemsInfoRequestedCheckBoxes()
        {
            chkHandleItemsInfoRequested.Visibility = chkUseFastPath.Visibility = cmbLayout.SelectedIndex == 1 ? Visibility.Visible : Visibility.Collapsed;
            chkHandleItemsInfoRequested.IsEnabled = chkUseFastPath.IsEnabled = cmbItemTemplate.SelectedIndex == 5;
            if (!chkHandleItemsInfoRequested.IsEnabled)
            {
                chkHandleItemsInfoRequested.IsChecked = chkUseFastPath.IsEnabled = false;
            }
        }

        private void UpdateAppAnimatorCheckBox()
        {
            if (cmbLayout.SelectedIndex == 1)
            {
                chkUseAppAnimator.Visibility = Visibility.Visible;
                if (itemsView != null)
                {
                    chkUseAppAnimator.IsChecked = itemsView.ItemTransitionProvider != null;
                }
            }
            else
            {
                chkUseAppAnimator.Visibility = Visibility.Collapsed;
            }
        }

        private void UpdateItemsInfoRequestedHandler()
        {
            if (_linedFlowLayout != null)
            {
                _linedFlowLayout.ItemsInfoRequested -= LinedFlowLayout_ItemsInfoRequested;

                if (chkHandleItemsInfoRequested.IsChecked == true || chkUseFastPath.IsChecked == true)
                {
                    _linedFlowLayout.ItemsInfoRequested += LinedFlowLayout_ItemsInfoRequested;
                }

                _linedFlowLayout.InvalidateItemsInfo();
            }
        }

        private void UpdateAppAnimator()
        {
            if (itemsView != null && chkUseAppAnimator != null)
            {
                if (chkUseAppAnimator.IsChecked == true)
                {
                    if (itemsView.ItemTransitionProvider == null)
                    {
                        if (_linedFlowLayoutItemCollectionTransitionProvider == null)
                        {
                            _linedFlowLayoutItemCollectionTransitionProvider = new LinedFlowLayoutItemCollectionTransitionProvider();
                        }

                        itemsView.ItemTransitionProvider = _linedFlowLayoutItemCollectionTransitionProvider;
                    }
                }
                else
                {
                    if (itemsView.ItemTransitionProvider != null)
                    {
                        itemsView.ItemTransitionProvider = null;
                    }
                }
            }
        }

        private void UpdateItemsViewIsItemInvokedEnabled()
        {
            if (itemsView != null)
            {
                chkIsItemInvokedEnabled.IsChecked = itemsView.IsItemInvokedEnabled;
            }
        }

        private void UpdateItemsViewLayout()
        {
            if (itemsView != null)
            {
                if (itemsView.Layout is StackLayout stackLayout)
                {
                    _stackLayout = stackLayout;
                    if (cmbLayout != null)
                    {
                        cmbLayout.SelectedIndex = 2;
                    }
                }
                else
                {
                    _stackLayout = Resources["stackLayout"] as StackLayout;
                    if (itemsView.Layout == null && cmbLayout != null)
                    {
                        cmbLayout.SelectedIndex = 0;
                    }
                }
            }
        }

        private void UpdateItemsViewSelectionMode()
        {
            if (itemsView != null && cmbSelectionMode != null)
            {
                cmbSelectionMode.SelectedIndex = (int)itemsView.SelectionMode;
            }
        }

        private void UpdateLinedFlowLayoutLineHeight()
        {
            if (_linedFlowLayout != null)
            {
                switch (cmbItemTemplate.SelectedIndex)
                {
                    case 2:
                        _linedFlowLayout.LineHeight = 100.0;
                        break;
                    case 3:
                        _linedFlowLayout.LineHeight = 192.0;
                        break;
                    case 4:
                        _linedFlowLayout.LineHeight = 96.0;
                        break;
                    case 5:
                        _linedFlowLayout.LineHeight = 80.0;
                        break;
                    default:
                        _linedFlowLayout.LineHeight = double.NaN;
                        break;
                }
            }
        }

        private void GetFullLog()
        {
            this.txtStatus.Text = "GetFullLog. Populating cmbFullLog...";
            chkLogCleared.IsChecked = false;
            foreach (string log in _fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
            chkLogUpdated.IsChecked = true;
            this.txtStatus.Text = "GetFullLog. Done.";
        }

        private void ClearFullLog()
        {
            this.txtStatus.Text = "ClearFullLog. Clearing cmbFullLog & fullLogs...";
            chkLogUpdated.IsChecked = false;
            _fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
            chkLogCleared.IsChecked = true;
            this.txtStatus.Text = "ClearFullLog. Done.";
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;
            FrameworkElement fe = sender as FrameworkElement;

            if (fe != null)
            {
                senderName = "s:" + fe.Name + ", ";
            }

            _fullLogs.Add((args.IsVerboseLevel ? "Verbose: " : "Info: ") + senderName + "m:" + msg);
        }
    }
}
