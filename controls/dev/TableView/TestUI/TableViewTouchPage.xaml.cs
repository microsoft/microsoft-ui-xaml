// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using Facts = Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewTestPageFacts;

namespace MUXControlsTestApp
{
    // Secondary TableView page for touch interaction tests, reached from TableViewPage's navigateToTouch button.
    public sealed partial class TableViewTouchPage : TestPage
    {
        private const int BasicItemCount = 6;
        private const int GroupedItemCount = 6;

        private readonly ObservableCollection<TableViewTestPerson> _basicItems = new ObservableCollection<TableViewTestPerson>();
        private readonly ObservableCollection<TableViewTestPerson> _groupedItems = new ObservableCollection<TableViewTestPerson>();
        private readonly ObservableCollection<TableViewTestPerson> _scrollingItems = new ObservableCollection<TableViewTestPerson>();

        private ScrollViewer _bodyScroller;
        private ScrollViewer _headerScroller;

        public TableViewTouchPage()
        {
            if (!Application.Current.Resources.MergedDictionaries.OfType<TabularControlsResources>().Any())
            {
                Application.Current.Resources.MergedDictionaries.Add(new TabularControlsResources());
            }

            InitializeComponent();

            for (int i = 0; i < BasicItemCount; i++)
            {
                _basicItems.Add(new TableViewTestPerson(i, Facts.BasicName(i), Facts.BasicAge(i), Facts.CityOf(i)));
            }

            for (int i = 0; i < GroupedItemCount; i++)
            {
                _groupedItems.Add(new TableViewTestPerson(i, Facts.GroupedName(i), Facts.GroupedAge(i), Facts.CityOf(i)));
            }

            for (int i = 0; i < Facts.ScrollingItemCount; i++)
            {
                _scrollingItems.Add(new TableViewTestPerson(i, Facts.ScrollingName(i), Facts.ScrollingAge(i), Facts.CityOf(i)));
            }

            TouchBasicTableView.ItemsSource = _basicItems;
            TouchGroupedTableView.ItemsSource = TableViewSource.From(_groupedItems)
                .GroupBy(item => ((TableViewTestPerson)item).City);
            TouchScrollingTableView.ItemsSource = _scrollingItems;

            TouchScrollingTableView.LayoutUpdated += OnScrollingTableViewLayoutUpdated;
        }

        // Template parts exist only after the first layout pass, so resolve them lazily (same as TableViewPage).
        private void OnScrollingTableViewLayoutUpdated(object sender, object e)
        {
            if (_bodyScroller != null)
            {
                return;
            }

            var scroller = FindDescendantByName<ScrollViewer>(TouchScrollingTableView, "PART_BodyScroller");
            if (scroller == null)
            {
                return;
            }

            _bodyScroller = scroller;
            _headerScroller = FindDescendantByName<ScrollViewer>(TouchScrollingTableView, "PART_HeaderScroller");
            TouchScrollingTableView.LayoutUpdated -= OnScrollingTableViewLayoutUpdated;

            scroller.ViewChanged += (s, args) => ReportScrollOffsets();

            // The header sync settles after ViewChanged returns, so refresh on LayoutUpdated too.
            TouchScrollingTableView.LayoutUpdated += (s, args) => ReportScrollOffsets();

            ReportScrollOffsets();
        }

        private void ReportScrollOffsets()
        {
            if (_bodyScroller == null)
            {
                return;
            }

            string text = string.Format(
                CultureInfo.InvariantCulture,
                "H={0:F0};V={1:F0};HeaderH={2}",
                _bodyScroller.HorizontalOffset,
                _bodyScroller.VerticalOffset,
                _headerScroller == null
                    ? "<none>"
                    : _headerScroller.HorizontalOffset.ToString("F0", CultureInfo.InvariantCulture));

            // Write only on change: LayoutUpdated also drives this, so an unconditional write would loop layout.
            if (TouchScrollOffsetTextBlock.Text != text)
            {
                TouchScrollOffsetTextBlock.Text = text;
            }
        }

        private static T FindDescendantByName<T>(DependencyObject root, string name) where T : FrameworkElement
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T typed && typed.Name == name)
                {
                    return typed;
                }

                var found = FindDescendantByName<T>(child, name);
                if (found != null)
                {
                    return found;
                }
            }

            return null;
        }
    }
}
