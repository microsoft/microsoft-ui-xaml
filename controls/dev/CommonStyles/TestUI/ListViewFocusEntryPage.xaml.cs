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
        private ScrollViewer _scrollViewer;
        private double _lastOffset;
        private int _offsetChanges;
        private int _disabledIndex = -1;
        private double? _resetDiagnosticsAtOffset;

        private double CurrentOffset => VerticalScrolling.IsChecked == true ?
            _scrollViewer.VerticalOffset : _scrollViewer.HorizontalOffset;

        public ListViewFocusEntryPage()
        {
            InitializeComponent();
            FocusEntryList.ItemsSource = Enumerable.Range(1, 39).Select(value => value.ToString()).ToList();
            FocusEntryList.ContainerContentChanging += (sender, args) =>
            {
                if (!args.InRecycleQueue && args.Phase == 0 && args.ItemContainer is Control itemContainer)
                {
                    itemContainer.IsEnabled = args.ItemIndex != _disabledIndex;
                }
            };
            FocusEntryList.SelectionChanged += (sender, args) =>
            {
                FocusEntrySelectedIndex.Text = FocusEntryList.SelectedIndex.ToString(CultureInfo.InvariantCulture);
            };
            GotFocus += (sender, args) =>
            {
                if (FocusManager.GetFocusedElement(XamlRoot) is ListViewItem item)
                {
                    FocusEntryFocusedIndex.Text = FocusEntryList.IndexFromContainer(item).ToString(CultureInfo.InvariantCulture);
                    FocusEntryOffsetAtFocus.Text = CurrentOffset.ToString(CultureInfo.InvariantCulture);
                }
                else
                {
                    FocusEntryFocusedIndex.Text = "-1";
                }
            };
            Loaded += (sender, args) =>
            {
                _scrollViewer = FocusEntryList.FindVisualChildByType<ScrollViewer>();
                _scrollViewer.ViewChanged += OnViewChanged;
            };
        }

        private void OnViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            double offset = CurrentOffset;
            if (Math.Abs(offset - _lastOffset) > 0.01)
            {
                _offsetChanges++;
                _lastOffset = offset;
                FocusEntryOffset.Text = offset.ToString(CultureInfo.InvariantCulture);
                FocusEntryChanges.Text = _offsetChanges.ToString(CultureInfo.InvariantCulture);
                FocusEntryOffsetHistory.Text +=
                    $"{(FocusEntryOffsetHistory.Text.Length == 0 ? "" : ",")}{offset.ToString(CultureInfo.InvariantCulture)}";
            }

            if (_resetDiagnosticsAtOffset.HasValue && !args.IsIntermediate &&
                Math.Abs(offset - _resetDiagnosticsAtOffset.Value) < 0.01)
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
            _disabledIndex = -1;
            _resetDiagnosticsAtOffset = null;
            bool vertical = VerticalScrolling.IsChecked == true;
            FocusEntryList.Width = vertical ? 200 : 600;
            FocusEntryList.Height = vertical ? 300 : 150;
            ScrollViewer.SetHorizontalScrollMode(FocusEntryList, vertical ? ScrollMode.Disabled : ScrollMode.Auto);
            ScrollViewer.SetVerticalScrollMode(FocusEntryList, vertical ? ScrollMode.Auto : ScrollMode.Disabled);
            ScrollViewer.SetHorizontalScrollBarVisibility(FocusEntryList, vertical ? ScrollBarVisibility.Disabled : ScrollBarVisibility.Hidden);
            ScrollViewer.SetVerticalScrollBarVisibility(FocusEntryList, vertical ? ScrollBarVisibility.Hidden : ScrollBarVisibility.Disabled);
            var panel = (ItemsWrapGrid)FocusEntryList.ItemsPanelRoot;
            panel.Orientation = vertical ? Orientation.Horizontal : Orientation.Vertical;
            panel.CacheLength = CacheLength.SelectedIndex == 0 ? 0 : 2;
            FocusEntryList.ItemsSource = Enumerable.Range(1, FiveItems.IsChecked == true ? 5 : 39).Select(value => value.ToString()).ToList();
            FocusEntryList.SelectedIndex = 0;
            FocusEntryList.UpdateLayout();
            if (ResetWithoutFocus.IsChecked != true)
            {
                ((ListViewItem)FocusEntryList.ContainerFromIndex(0)).Focus(FocusState.Keyboard);
            }
            ResetDiagnostics();
        }

        // Mirrors an app that selects an item and positions the viewport itself, before focus enters the list.
        private void PrepareFocusEntry_Click(object sender, RoutedEventArgs args)
        {
            Control anchor = sender == PrepareFocusEntryAfter ? AfterListFocusTarget : BeforeListFocusTarget;
            anchor.Focus(FocusState.Keyboard);

            bool vertical = VerticalScrolling.IsChecked == true;
            double itemSize = vertical ? 100 : 200;
            double viewportSize = vertical ? FocusEntryList.Height : FocusEntryList.Width;
            int position = FocusEntryPosition.SelectedIndex;
            double offset =
                position == 0 ? 10 * itemSize :
                position == 1 ? 11 * itemSize - viewportSize :
                position == 2 ? 0 :
                position == 3 ? 15 * itemSize :
                10.5 * itemSize;

            FocusEntryList.SelectedIndex = 10;
            if (Math.Abs(CurrentOffset - offset) < 0.01)
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

            _disabledIndex = FocusEntryList.SelectedIndex;
            if (FocusEntryList.ContainerFromIndex(_disabledIndex) is Control container)
            {
                container.IsEnabled = false;
            }
        }

        private void FocusListFromCode_Click(object sender, RoutedEventArgs args)
        {
            FocusEntryList.Focus(FocusState.Keyboard);
        }
    }
}
