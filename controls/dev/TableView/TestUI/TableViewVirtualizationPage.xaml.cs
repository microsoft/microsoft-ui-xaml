// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using Facts = Microsoft.UI.Xaml.Tests.MUXControls.TableViewShared.TableViewVirtualizationPageFacts;

namespace MUXControlsTestApp
{
    public sealed class TableViewVirtualizationItem : INotifyPropertyChanged
    {
        private string _name;
        private int _quantity;

        public TableViewVirtualizationItem(int id)
        {
            Id = id;
            _name = Facts.ItemName(id);
            _quantity = Facts.ItemQuantity(id);
            Code = Facts.ItemCode(id);
        }

        public int Id { get; }

        public string Code { get; }

        public string Name
        {
            get => _name;
            set
            {
                if (_name != value)
                {
                    _name = value;
                    Raise(nameof(Name));
                }
            }
        }

        public int Quantity
        {
            get => _quantity;
            set
            {
                if (_quantity != value)
                {
                    _quantity = value;
                    Raise(nameof(Quantity));
                }
            }
        }

        public bool IsSeeded => _name == Facts.ItemName(Id) && _quantity == Facts.ItemQuantity(Id);

        public event PropertyChangedEventHandler PropertyChanged;

        private void Raise(string propertyName) =>
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    // Reached from TableViewPage's navigateToVirtualization button; deliberately not a top-level page.
    public sealed partial class TableViewVirtualizationPage : TestPage
    {
        private readonly ObservableCollection<TableViewVirtualizationItem> _items = new ObservableCollection<TableViewVirtualizationItem>();

        // Every item ever created, so "modified" still sees an item after it is removed from the source.
        private readonly List<TableViewVirtualizationItem> _allItems = new List<TableViewVirtualizationItem>();
        private readonly List<string> _editEndings = new List<string>();
        private readonly TableViewVirtualizationItem _firstItem;
        private int _beginningEditCount;
        private bool _refreshQueued;
        private ScrollViewer _bodyScroller;
        private ItemsRepeater _rowsRepeater;

        public TableViewVirtualizationPage()
        {
            if (!Application.Current.Resources.MergedDictionaries.OfType<TabularControlsResources>().Any())
            {
                Application.Current.Resources.MergedDictionaries.Add(new TabularControlsResources());
            }

            InitializeComponent();

            for (int i = 0; i < Facts.ItemCount; i++)
            {
                var item = new TableViewVirtualizationItem(i);
                item.PropertyChanged += (s, e) => QueueReadoutRefresh();
                _items.Add(item);
                _allItems.Add(item);
            }

            _firstItem = _items[0];
            LargeEditableTable.ItemsSource = _items;

            LargeEditableTable.BeginningEdit += (s, args) =>
            {
                _beginningEditCount++;
                QueueReadoutRefresh();
            };

            // A recycle-driven close posts this to the dispatcher (TableView::PostEditNotification), so it can
            // arrive after the layout pass that closed the edit; the Low-priority refresh below follows it.
            LargeEditableTable.CellEditEnding += (s, args) =>
            {
                var item = args.Item as TableViewVirtualizationItem;
                _editEndings.Add(args.EditAction + "/" + (item == null ? "?" : item.Id.ToString(CultureInfo.InvariantCulture)));
                QueueReadoutRefresh();
            };

            // Realization, recycling and editor teardown all happen in layout, so this keeps IsEditing and the
            // editor list current without a polling timer.
            LargeEditableTable.LayoutUpdated += (s, args) => RefreshReadout();

            QueueReadoutRefresh();
        }

        private void OnRemoveFirstItemClick(object sender, RoutedEventArgs e)
        {
            if (_items.Count > 0)
            {
                _items.RemoveAt(0);
            }
        }

        // Template parts exist only once the table has been measured, so they are resolved lazily.
        private void EnsureTemplateParts()
        {
            if (_bodyScroller == null)
            {
                _bodyScroller = FindDescendantByName<ScrollViewer>(LargeEditableTable, "PART_BodyScroller");
                if (_bodyScroller != null)
                {
                    _bodyScroller.ViewChanged += (s, args) => QueueReadoutRefresh();
                }
            }

            if (_rowsRepeater == null)
            {
                _rowsRepeater = FindDescendantByName<ItemsRepeater>(LargeEditableTable, "PART_RowsRepeater");
            }
        }

        // Low priority so the readout describes the settled state, after focus and layout have moved.
        private void QueueReadoutRefresh()
        {
            if (_refreshQueued)
            {
                return;
            }

            _refreshQueued = true;
            DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, () =>
            {
                _refreshQueued = false;
                RefreshReadout();
            });
        }

        // Assigns only on change: the assignment re-lays out the TextBox, which raises LayoutUpdated again.
        private void RefreshReadout()
        {
            if (VirtualizationReadout == null || LargeEditableTable == null)
            {
                return;
            }

            EnsureTemplateParts();

            var editors = new List<string>();
            int firstVisible = int.MaxValue;
            int lastVisible = int.MinValue;

            if (_rowsRepeater != null)
            {
                int count = VisualTreeHelper.GetChildrenCount(_rowsRepeater);
                for (int i = 0; i < count; i++)
                {
                    var element = VisualTreeHelper.GetChild(_rowsRepeater, i) as UIElement;
                    var row = element as TableViewRow ?? (element == null ? null : FindDescendant<TableViewRow>(element));
                    var item = row?.DataContext as TableViewVirtualizationItem;
                    if (item == null)
                    {
                        continue;
                    }

                    // A cell's display visual is a TextBlock, so any TextBox inside a row is an open editor.
                    var textBoxes = new List<TextBox>();
                    CollectDescendants(row, textBoxes);
                    foreach (var editor in textBoxes)
                    {
                        editors.Add(item.Id.ToString(CultureInfo.InvariantCulture) + "/" + editor.Text);
                    }

                    if (IsInViewport(row))
                    {
                        firstVisible = global::System.Math.Min(firstVisible, item.Id);
                        lastVisible = global::System.Math.Max(lastVisible, item.Id);
                    }
                }
            }

            string modified = string.Join(",", _allItems.Where(item => !item.IsSeeded).Select(item => item.Id.ToString(CultureInfo.InvariantCulture)));

            string text =
                Facts.EditingField + "=" + LargeEditableTable.IsEditing + ";" +
                Facts.BeginningEditField + "=" + _beginningEditCount.ToString(CultureInfo.InvariantCulture) + ";" +
                Facts.EditEndingField + "=" + JoinOrNone(_editEndings) + ";" +
                Facts.FirstItemField + "=" + _firstItem.Name + ";" +
                Facts.ModifiedField + "=" + (modified.Length == 0 ? Facts.NoneValue : modified) + ";" +
                Facts.EditorsField + "=" + JoinOrNone(editors) + ";" +
                Facts.VisibleField + "=" + (firstVisible > lastVisible
                    ? Facts.NoneValue
                    : firstVisible.ToString(CultureInfo.InvariantCulture) + "-" + lastVisible.ToString(CultureInfo.InvariantCulture)) + ";" +
                Facts.OffsetField + "=" + (_bodyScroller == null
                    ? Facts.NoneValue
                    : _bodyScroller.VerticalOffset.ToString("F0", CultureInfo.InvariantCulture));

            if (VirtualizationReadout.Text != text)
            {
                VirtualizationReadout.Text = text;
            }
        }

        // A row the repeater holds outside its layout window (for example one pinned because it has focus) is
        // arranged far off-screen, so it never counts as visible.
        private bool IsInViewport(TableViewRow row)
        {
            if (_bodyScroller == null || row.Visibility != Visibility.Visible || row.ActualHeight <= 0)
            {
                return false;
            }

            double top;
            try
            {
                top = row.TransformToVisual(_bodyScroller).TransformPoint(new Windows.Foundation.Point(0, 0)).Y;
            }
            catch (global::System.ArgumentException)
            {
                return false;
            }

            return top + row.ActualHeight > 0.5 && top < _bodyScroller.ViewportHeight - 0.5;
        }

        private static string JoinOrNone(List<string> entries) =>
            entries.Count == 0 ? Facts.NoneValue : string.Join(",", entries);

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

        private static T FindDescendant<T>(DependencyObject root) where T : class
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match)
                {
                    return match;
                }

                var nested = FindDescendant<T>(child);
                if (nested != null)
                {
                    return nested;
                }
            }

            return null;
        }

        private static void CollectDescendants<T>(DependencyObject root, List<T> results) where T : class
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match)
                {
                    results.Add(match);
                }

                CollectDescendants(child, results);
            }
        }
    }
}
