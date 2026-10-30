// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using Windows.Foundation;
using Windows.System;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ListView Keyboard Scrolling")]
    public sealed partial class ListViewKeyboardScrollingPage : TestPage
    {
        private ScrollViewer _scrollViewer;
        private double _lastOffset;
        private int _offsetChanges;
        private bool _cancelFocusChanges;
        private bool _redirectFocusChanges;
        private bool _disableFocusScrollingDuringFocus;
        private int _bringRequests;
        private int _ancestorRequests;
        private int _viewChangingEvents;
        private int _viewChangedEvents;
        private int _selectionChanges;
        private int _focusChanges;
        private int _preparedContainers;
        private int _navigationKeyCount;
        private double? _resetDiagnosticsAtOffset;
        private bool _scrollAwayOnSelectionChanged;

        private double CurrentOffset => VerticalScrolling.IsChecked == true ?
            _scrollViewer.VerticalOffset : _scrollViewer.HorizontalOffset;

        public ListViewKeyboardScrollingPage()
        {
            InitializeComponent();
            KeyboardScrollingList.AddHandler(UIElement.KeyDownEvent, new KeyEventHandler(OnKeyboardScrollKeyDown), true);
            KeyboardScrollingList.ItemsSource = Enumerable.Range(1, 39).Select(value => value.ToString()).ToList();
            KeyboardScrollingList.ChoosingGroupHeaderContainer += (sender, args) =>
            {
                if (VaryHeaderHeights.IsChecked == true)
                {
                    var header = args.GroupHeaderContainer ?? new ListViewHeaderItem();
                    header.Height = 50 + 30 * args.GroupIndex;
                    args.GroupHeaderContainer = header;
                }
                else
                {
                    args.GroupHeaderContainer?.ClearValue(FrameworkElement.HeightProperty);
                }
            };
            KeyboardScrollingList.ContainerContentChanging += (sender, args) =>
            {
                if (!args.InRecycleQueue && args.Phase == 0)
                {
                    _preparedContainers++;
                    KeyboardScrollPreparedContainers.Text = _preparedContainers.ToString(CultureInfo.InvariantCulture);
                    KeyboardScrollPreparedIndices.Text +=
                        $"{(KeyboardScrollPreparedIndices.Text.Length == 0 ? "" : ",")}{args.ItemIndex}";
                }
            };
            Loaded += OnLoaded;
            KeyboardScrollingList.SelectionChanged += (sender, args) =>
            {
                _selectionChanges++;
                KeyboardScrollSelectionChanges.Text = _selectionChanges.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollSelectedIndex.Text = KeyboardScrollingList.SelectedIndex.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollSelectedCount.Text = KeyboardScrollingList.SelectedItems.Count.ToString(CultureInfo.InvariantCulture);
                if (_scrollAwayOnSelectionChanged)
                {
                    // Recycles the containers near the start of the list while keyboard navigation is still in progress.
                    KeyboardScrollingList.ScrollIntoView(KeyboardScrollingList.Items[KeyboardScrollingList.Items.Count - 1]);
                    KeyboardScrollingList.UpdateLayout();
                }
            };
            KeyboardScrollingList.GettingFocus += (sender, args) =>
            {
                if (_cancelFocusChanges)
                {
                    args.Cancel = true;
                }
                else if (_redirectFocusChanges)
                {
                    args.TrySetNewFocusedElement(RedirectFocusTarget);
                }

                if (_disableFocusScrollingDuringFocus)
                {
                    _scrollViewer.BringIntoViewOnFocusChange = false;
                }
            };
            GotFocus += (sender, args) =>
            {
                var focusedElement = FocusManager.GetFocusedElement(XamlRoot);
                KeyboardScrollFocusSequence.Text +=
                    $"{(KeyboardScrollFocusSequence.Text.Length == 0 ? "" : ",")}{focusedElement?.GetType().Name}" +
                    (focusedElement is ListViewItem focusedItem ? $":{KeyboardScrollingList.IndexFromContainer(focusedItem)}" : "");
                if (focusedElement is ListViewItem item)
                {
                    _focusChanges++;
                    KeyboardScrollFocusChanges.Text = _focusChanges.ToString(CultureInfo.InvariantCulture);
                    KeyboardScrollFocusedIndex.Text = KeyboardScrollingList.IndexFromContainer(item).ToString(CultureInfo.InvariantCulture);
                    KeyboardScrollFocusedKind.Text = "Item";
                    KeyboardScrollOffsetAtFocus.Text = CurrentOffset.ToString(CultureInfo.InvariantCulture);
                    UpdateFocusedBounds();
                }
                else if (focusedElement is ListViewHeaderItem)
                {
                    _focusChanges++;
                    KeyboardScrollFocusChanges.Text = _focusChanges.ToString(CultureInfo.InvariantCulture);
                    KeyboardScrollFocusedIndex.Text = "-1";
                    KeyboardScrollFocusedKind.Text = "Header";
                    UpdateFocusedBounds();
                }
                else
                {
                    KeyboardScrollFocusedIndex.Text = "-1";
                    KeyboardScrollFocusedKind.Text = "Other";
                }
            };
            KeyboardScrollingList.BringIntoViewRequested += (sender, args) =>
            {
                if (args.TargetElement == _scrollViewer)
                {
                    _bringRequests++;
                    KeyboardScrollBringRequests.Text = _bringRequests.ToString(CultureInfo.InvariantCulture);
                }
            };
            KeyboardScrollingRoot.BringIntoViewRequested += (sender, args) =>
            {
                if (args.TargetElement == KeyboardScrollingOuterScroller)
                {
                    _ancestorRequests++;
                    KeyboardScrollAncestorRequests.Text = _ancestorRequests.ToString(CultureInfo.InvariantCulture);
                }
            };
        }

        private void OnLoaded(object sender, RoutedEventArgs args)
        {
            _scrollViewer = KeyboardScrollingList.FindVisualChildByType<ScrollViewer>();
            _scrollViewer.ViewChanged += OnViewChanged;
            _scrollViewer.ViewChanging += (sender, args) =>
            {
                _viewChangingEvents++;
                KeyboardScrollViewChangingEvents.Text = _viewChangingEvents.ToString(CultureInfo.InvariantCulture);
            };
        }

        private void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            _viewChangedEvents++;
            KeyboardScrollViewChangedEvents.Text = _viewChangedEvents.ToString(CultureInfo.InvariantCulture);
            double offset = CurrentOffset;
            if (Math.Abs(offset - _lastOffset) > 0.01)
            {
                _offsetChanges++;
                _lastOffset = offset;
                KeyboardScrollOffset.Text = offset.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollChanges.Text = _offsetChanges.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollOffsetHistory.Text +=
                    $"{(KeyboardScrollOffsetHistory.Text.Length == 0 ? "" : ",")}{offset.ToString(CultureInfo.InvariantCulture)}";
                UpdateFocusedBounds();
            }

            if (_resetDiagnosticsAtOffset.HasValue && !args.IsIntermediate &&
                Math.Abs(offset - _resetDiagnosticsAtOffset.Value) < 0.01)
            {
                _resetDiagnosticsAtOffset = null;
                ResetScrollDiagnostics();
            }
        }

        private void ResetScrollDiagnostics()
        {
            _offsetChanges = 0;
            _bringRequests = 0;
            _ancestorRequests = 0;
            _viewChangingEvents = 0;
            _viewChangedEvents = 0;
            _selectionChanges = 0;
            _focusChanges = 0;
            _preparedContainers = 0;
            KeyboardScrollChanges.Text = "0";
            KeyboardScrollBringRequests.Text = "0";
            KeyboardScrollAncestorRequests.Text = "0";
            KeyboardScrollViewChangingEvents.Text = "0";
            KeyboardScrollViewChangedEvents.Text = "0";
            KeyboardScrollSelectionChanges.Text = "0";
            KeyboardScrollFocusChanges.Text = "0";
            KeyboardScrollPreparedContainers.Text = "0";
            KeyboardScrollPreparedIndices.Text = "";
            KeyboardScrollFocusSequence.Text = "";
            KeyboardScrollOffsetAtFocus.Text = "";
            KeyboardScrollOffsetHistory.Text = "";
        }

        private void UpdateFocusedBounds()
        {
            if (FocusManager.GetFocusedElement(XamlRoot) is FrameworkElement element &&
                (element is ListViewItem || element is ListViewHeaderItem))
            {
                Rect bounds = element.TransformToVisual(KeyboardScrollingList).TransformBounds(
                    new Rect(0, 0, element.ActualWidth, element.ActualHeight));
                KeyboardScrollFocusedTop.Text = bounds.Y.ToString(CultureInfo.InvariantCulture);
            }
        }

        private void KeyboardScrollingList_PreviewKeyDown(object sender, KeyRoutedEventArgs args)
        {
            if (IsNavigationKey(args.Key))
            {
                _navigationKeyCount++;
                ResetScrollDiagnostics();
                if (ProbeLayout.IsChecked == true)
                {
                    LayoutProbe.ResetAndInvalidate();
                }
            }
        }

        private void OnKeyboardScrollKeyDown(object sender, KeyRoutedEventArgs args)
        {
            if (IsNavigationKey(args.Key))
            {
                KeyboardScrollLayoutAtKeyDown.Text = LayoutProbe.MeasureCalls.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollOffsetAtKeyDown.Text = CurrentOffset.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollChangesAtKeyDown.Text = _offsetChanges.ToString(CultureInfo.InvariantCulture);
                KeyboardScrollKeysProcessed.Text = _navigationKeyCount.ToString(CultureInfo.InvariantCulture);
            }
        }

        private static bool IsNavigationKey(VirtualKey key) =>
            key == VirtualKey.Left || key == VirtualKey.Right || key == VirtualKey.Up || key == VirtualKey.Down ||
            key == VirtualKey.PageUp || key == VirtualKey.PageDown || key == VirtualKey.Home || key == VirtualKey.End;

        private void CacheLength_SelectionChanged(object sender, SelectionChangedEventArgs args)
        {
            if (KeyboardScrollingList?.ItemsPanelRoot is ItemsWrapGrid panel)
            {
                panel.CacheLength = CacheLength.SelectedIndex == 0 ? 0 : 2;
            }
        }

        private void ResetKeyboardScrolling_Click(object sender, RoutedEventArgs args)
        {
            _cancelFocusChanges = false;
            _redirectFocusChanges = false;
            _disableFocusScrollingDuringFocus = false;
            _resetDiagnosticsAtOffset = null;
            _scrollAwayOnSelectionChanged = false;
            bool vertical = VerticalScrolling.IsChecked == true;
            KeyboardScrollingList.Width = vertical ? 200 : PartialViewport.IsChecked == true ? 550 : 600;
            KeyboardScrollingList.Height = vertical ? PartialViewport.IsChecked == true ? 250 : 300 : 150;
            KeyboardScrollingList.SelectionMode = ExtendedSelection.IsChecked == true ?
                ListViewSelectionMode.Extended : ListViewSelectionMode.Single;
            ScrollViewer.SetHorizontalScrollMode(KeyboardScrollingList, vertical ? ScrollMode.Disabled : ScrollMode.Auto);
            ScrollViewer.SetVerticalScrollMode(KeyboardScrollingList, vertical ? ScrollMode.Auto : ScrollMode.Disabled);
            ScrollViewer.SetHorizontalScrollBarVisibility(KeyboardScrollingList, vertical ? ScrollBarVisibility.Disabled : ScrollBarVisibility.Hidden);
            ScrollViewer.SetVerticalScrollBarVisibility(KeyboardScrollingList, vertical ? ScrollBarVisibility.Hidden : ScrollBarVisibility.Disabled);
            ((ItemsWrapGrid)KeyboardScrollingList.ItemsPanelRoot).Orientation = vertical ? Orientation.Horizontal : Orientation.Vertical;
            ((ItemsWrapGrid)KeyboardScrollingList.ItemsPanelRoot).AreStickyGroupHeadersEnabled = DisableStickyHeaders.IsChecked != true;
            var items = Enumerable.Range(1, 39).Select(value => value.ToString()).ToList();
            KeyboardScrollingList.GroupStyle.Clear();
            if (GroupedItems.IsChecked == true)
            {
                var source = new CollectionViewSource
                {
                    IsSourceGrouped = true,
                    Source = new[] { items.Take(13).ToList(), items.Skip(13).Take(13).ToList(), items.Skip(26).ToList() }
                };
                KeyboardScrollingList.ItemsSource = source.View;
                var headerStyle = new Style(typeof(ListViewHeaderItem));
                headerStyle.Setters.Add(new Setter(FrameworkElement.HeightProperty, 50.0));
                headerStyle.Setters.Add(new Setter(FrameworkElement.WidthProperty, vertical ? 200.0 : 80.0));
                headerStyle.Setters.Add(new Setter(FrameworkElement.MarginProperty, new Thickness(0)));
                headerStyle.Setters.Add(new Setter(Control.PaddingProperty, new Thickness(0)));
                KeyboardScrollingList.GroupStyle.Add(new GroupStyle
                {
                    HeaderTemplate = (DataTemplate)Resources["KeyboardScrollGroupHeader"],
                    HeaderContainerStyle = headerStyle
                });
            }
            else
            {
                KeyboardScrollingList.ItemsSource = items;
            }
            if (LargeFocusMargin.IsChecked == true || ScaleItems.IsChecked == true)
            {
                var style = new Style(typeof(ListViewItem));
                if (LargeFocusMargin.IsChecked == true)
                {
                    style.Setters.Add(new Setter(Control.FocusVisualMarginProperty, new Thickness(-40)));
                }
                if (ScaleItems.IsChecked == true)
                {
                    style.Setters.Add(new Setter(UIElement.RenderTransformProperty,
                        new ScaleTransform { ScaleX = vertical ? 1 : 0.5, ScaleY = vertical ? 0.5 : 1 }));
                }
                KeyboardScrollingList.ItemContainerStyle = style;
            }
            else
            {
                KeyboardScrollingList.ItemContainerStyle = null;
            }
            _scrollViewer.BringIntoViewOnFocusChange = DisableFocusScrolling.IsChecked != true;
            int index = CancelFocusChanges.IsChecked == true || RedirectFocusChanges.IsChecked == true ||
                DisableFocusScrollingDuringFocus.IsChecked == true ? 2 : 0;
            KeyboardScrollingList.SelectedIndex = index;
            KeyboardScrollingList.ScrollIntoView(KeyboardScrollingList.Items[index]);
            KeyboardScrollingList.UpdateLayout();
            ((ListViewItem)KeyboardScrollingList.ContainerFromIndex(index)).Focus(FocusState.Keyboard);
            KeyboardScrollLastContainerRealized.Text =
                (KeyboardScrollingList.ContainerFromIndex(38) != null).ToString();
            _cancelFocusChanges = CancelFocusChanges.IsChecked == true;
            _redirectFocusChanges = RedirectFocusChanges.IsChecked == true;
            _disableFocusScrollingDuringFocus = DisableFocusScrollingDuringFocus.IsChecked == true;
            _scrollAwayOnSelectionChanged = ScrollAwayOnSelectionChanged.IsChecked == true;
            _navigationKeyCount = 0;
            KeyboardScrollKeysProcessed.Text = "0";
        }

        private void PublicScrollIntoView_Click(object sender, RoutedEventArgs args)
        {
            KeyboardScrollingList.ScrollIntoView(KeyboardScrollingList.Items[10]);
        }

        // Mirrors an app that selects an item and positions the viewport itself, before focus enters the list.
        private void PrepareFocusEntry_Click(object sender, RoutedEventArgs args)
        {
            Control anchor = sender == PrepareFocusEntryAfter ? AfterListFocusTarget : RedirectFocusTarget;
            anchor.Focus(FocusState.Keyboard);

            bool vertical = VerticalScrolling.IsChecked == true;
            double itemSize = vertical ? 100 : 200;
            double viewportSize = vertical ? KeyboardScrollingList.Height : KeyboardScrollingList.Width;
            int position = FocusEntryPosition.SelectedIndex;
            double offset =
                position == 0 ? 10 * itemSize :
                position == 1 ? 11 * itemSize - viewportSize :
                position == 2 ? 0 :
                position == 3 ? 15 * itemSize :
                10.5 * itemSize;

            KeyboardScrollingList.SelectedIndex = 10;
            if (Math.Abs(CurrentOffset - offset) < 0.01)
            {
                ResetScrollDiagnostics();
            }
            else
            {
                _resetDiagnosticsAtOffset = offset;
                // Animated, like an app reacting to a selection change.
                _scrollViewer.ChangeView(vertical ? null : offset, vertical ? offset : null, null);
            }
        }
    }

    public sealed class KeyboardScrollingLayoutProbe : Grid
    {
        public int MeasureCalls { get; private set; }

        public void ResetAndInvalidate()
        {
            MeasureCalls = 0;
            InvalidateMeasure();
        }

        protected override Size MeasureOverride(Size availableSize)
        {
            MeasureCalls++;
            return base.MeasureOverride(availableSize);
        }
    }
}
