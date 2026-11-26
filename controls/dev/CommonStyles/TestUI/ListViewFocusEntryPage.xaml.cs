// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ListView Focus Entry")]
    public sealed partial class ListViewFocusEntryPage : TestPage
    {
        private ListView _focusEntryList;
        private ScrollViewer _scrollViewer;
        private bool _isVertical;
        private double _lastOffset;
        private int _offsetChanges;
        private int _disabledIndex = -1;
        private double? _resetDiagnosticsAtOffset;

        // Animated views can settle a fraction of a pixel away from the requested offset, which is not a visible move.
        private const double OffsetTolerance = 0.5;

        private double CurrentOffset => _scrollViewer == null ? 0 :
            (_isVertical ? _scrollViewer.VerticalOffset : _scrollViewer.HorizontalOffset);

        public ListViewFocusEntryPage()
        {
            InitializeComponent();
            _focusEntryList = CreateFocusEntryList(itemCount: 39);
            FocusEntryListHost.Child = _focusEntryList;
            GotFocus += (sender, args) =>
            {
                var focusedElement = FocusManager.GetFocusedElement(XamlRoot) as DependencyObject;
                FocusEntryFocusedElementType.Text = focusedElement?.GetType().Name ?? "";
                var item = focusedElement.FindVisualParentByType<ListViewItem>();
                if (item != null)
                {
                    FocusEntryFocusedIndex.Text = _focusEntryList.IndexFromContainer(item).ToString(CultureInfo.InvariantCulture);
                    FocusEntryOffsetAtFocus.Text = CurrentOffset.ToString(CultureInfo.InvariantCulture);
                }
                else
                {
                    FocusEntryFocusedIndex.Text = "-1";
                }
            };
            Loaded += (sender, args) =>
            {
                if (_scrollViewer == null)
                {
                    _scrollViewer = _focusEntryList.FindVisualChildByType<ScrollViewer>();
                    _scrollViewer.ViewChanged += OnViewChanged;
                }
            };
        }

        private ListView CreateFocusEntryList(int itemCount)
        {
            var list = (ListView)((DataTemplate)Resources["FocusEntryListTemplate"]).LoadContent();
            list.ItemsSource = Enumerable.Range(1, itemCount).Select(value => value.ToString()).ToList();
            list.ContainerContentChanging += (sender, args) =>
            {
                if (!args.InRecycleQueue && args.Phase == 0 && args.ItemContainer is Control itemContainer)
                {
                    itemContainer.IsEnabled = args.ItemIndex != _disabledIndex;
                }
            };
            list.SelectionChanged += (sender, args) =>
            {
                if (list == _focusEntryList)
                {
                    FocusEntrySelectedIndex.Text = list.SelectedIndex.ToString(CultureInfo.InvariantCulture);
                }
            };
            list.GotFocus += (sender, args) =>
            {
                if (list == _focusEntryList)
                {
                    FocusEntryListEverFocused.Text = "True";
                }
            };
            return list;
        }

        private void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            double offset = CurrentOffset;
            if (Math.Abs(offset - _lastOffset) > OffsetTolerance)
            {
                _offsetChanges++;
                _lastOffset = offset;
                FocusEntryOffset.Text = offset.ToString(CultureInfo.InvariantCulture);
                FocusEntryChanges.Text = _offsetChanges.ToString(CultureInfo.InvariantCulture);
                FocusEntryOffsetHistory.Text +=
                    $"{(FocusEntryOffsetHistory.Text.Length == 0 ? "" : ",")}{offset.ToString(CultureInfo.InvariantCulture)}";
            }

            if (_resetDiagnosticsAtOffset.HasValue && !args.IsIntermediate &&
                Math.Abs(offset - _resetDiagnosticsAtOffset.Value) < OffsetTolerance)
            {
                _resetDiagnosticsAtOffset = null;
                ResetDiagnostics();
            }
        }

        private void ResetDiagnostics()
        {
            _offsetChanges = 0;
            FocusEntryChanges.Text = "0";
            FocusEntryOffsetAtFocus.Text = "";
            FocusEntryOffsetHistory.Text = "";
        }

        private void ResetFocusEntryList_Click(object sender, RoutedEventArgs args)
        {
            // Move focus off the old ListView before replacing it so focus-on-removal cannot land inside the new one.
            BeforeListFocusTarget.Focus(FocusState.Programmatic);

            if (_scrollViewer != null)
            {
                _scrollViewer.ViewChanged -= OnViewChanged;
                _scrollViewer = null;
            }

            _disabledIndex = -1;
            _resetDiagnosticsAtOffset = null;
            _lastOffset = 0;

            bool vertical = VerticalScrolling.IsChecked == true;
            _isVertical = vertical;

            // Recreate the ListView so ResetWithoutFocus exercises a control that has never had
            // a focused item and no ScrollViewer offset carries over between orientations.
            var list = CreateFocusEntryList(FiveItems.IsChecked == true ? 5 : 39);
            list.Width = vertical ? 200 : 600;
            list.Height = vertical ? 300 : 150;
            ScrollViewer.SetHorizontalScrollMode(list, vertical ? ScrollMode.Disabled : ScrollMode.Auto);
            ScrollViewer.SetVerticalScrollMode(list, vertical ? ScrollMode.Auto : ScrollMode.Disabled);
            ScrollViewer.SetHorizontalScrollBarVisibility(list, vertical ? ScrollBarVisibility.Disabled : ScrollBarVisibility.Hidden);
            ScrollViewer.SetVerticalScrollBarVisibility(list, vertical ? ScrollBarVisibility.Hidden : ScrollBarVisibility.Disabled);
            list.ItemTemplate = FocusableChildren.IsChecked == true ?
                (DataTemplate)Resources["FocusableChildrenTemplate"] : null;

            FocusEntryListEverFocused.Text = "False";
            _focusEntryList = list;
            FocusEntryListHost.Child = list;
            list.UpdateLayout();

            var panel = (ItemsWrapGrid)list.ItemsPanelRoot;
            panel.Orientation = vertical ? Orientation.Horizontal : Orientation.Vertical;
            panel.CacheLength = CacheLength.SelectedIndex == 0 ? 0 : 2;
            list.SelectedIndex = 0;
            list.UpdateLayout();

            _scrollViewer = list.FindVisualChildByType<ScrollViewer>();
            _scrollViewer.ViewChanged += OnViewChanged;

            FocusEntryOffset.Text = "0";
            FocusEntryFocusedIndex.Text = "-1";
            FocusEntryFocusedElementType.Text = "";

            if (ResetWithoutFocus.IsChecked != true)
            {
                ((ListViewItem)list.ContainerFromIndex(0)).Focus(FocusState.Keyboard);
            }
            ResetDiagnostics();
        }

        // Mirrors an app that selects an item and positions the viewport itself, before focus enters the list.
        private void PrepareFocusEntry_Click(object sender, RoutedEventArgs args)
        {
            Control anchor = sender == PrepareFocusEntryAfter ? AfterListFocusTarget : BeforeListFocusTarget;
            anchor.Focus(FocusState.Keyboard);

            bool vertical = _isVertical;
            double itemSize = vertical ? 100 : 200;
            double viewportSize = vertical ? _focusEntryList.Height : _focusEntryList.Width;
            int position = FocusEntryPosition.SelectedIndex;
            double offset =
                position == 0 ? 10 * itemSize :
                position == 1 ? 11 * itemSize - viewportSize :
                position == 2 ? 0 :
                position == 3 ? 15 * itemSize :
                position == 5 ? 10 * itemSize - 20 :
                position == 6 ? 11 * itemSize - viewportSize + 20 :
                10.5 * itemSize;

            _focusEntryList.SelectedIndex = 10;
            if (Math.Abs(CurrentOffset - offset) < OffsetTolerance)
            {
                ResetDiagnostics();
            }
            else
            {
                _resetDiagnosticsAtOffset = offset;
                // Animated, like an app reacting to a selection change.
                _scrollViewer.ChangeView(vertical ? null : offset, vertical ? offset : null, null);
            }
        }

        // Makes the selected item unable to take focus, then moves focus next to the list (before it to Tab in,
        // after it to Shift+Tab in). Selection and the last focused item are left untouched.
        private void DisableSelectedItem_Click(object sender, RoutedEventArgs args)
        {
            Control anchor = sender == DisableSelectedItemFocusBefore ? BeforeListFocusTarget : AfterListFocusTarget;
            anchor.Focus(FocusState.Keyboard);

            _disabledIndex = _focusEntryList.SelectedIndex;
            if (_focusEntryList.ContainerFromIndex(_disabledIndex) is Control container)
            {
                container.IsEnabled = false;
            }
        }

        private void FocusListFromCode_Click(object sender, RoutedEventArgs args)
        {
            _focusEntryList.Focus(FocusState.Keyboard);
        }

        private void LeaveListFocusTarget_Click(object sender, RoutedEventArgs args)
        {
            ((Control)sender).Focus(FocusState.Keyboard);
        }
    }
}
