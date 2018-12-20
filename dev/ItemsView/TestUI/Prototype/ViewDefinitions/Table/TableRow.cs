using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;

#if !BUILD_WINDOWS
using LayoutBase = Microsoft.UI.Xaml.Controls.LayoutBase;
using VirtualizingLayoutBase = Microsoft.UI.Xaml.Controls.VirtualizingLayoutBase;
using ViewGenerator = Microsoft.UI.Xaml.Controls.ViewGenerator;
using Repeater = Microsoft.UI.Xaml.Controls.Repeater;
#endif

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class TableRow : ItemsViewItem
    {
        public TableRow()
        {
            DefaultStyleKey = typeof(TableRow);
        }

        public ObservableCollection<ItemsViewColumnDefinitionBase> ColumnDefinitions
        {
            get { return m_columnDefs; }
            set
            {
                if (m_columnDefs != null)
                {
                    m_columnDefs.CollectionChanged -= OnColumnDefinitionsChanged;
                }

                m_columnDefs = value;
                m_columnDefs.CollectionChanged += OnColumnDefinitionsChanged;
            }
        }

        protected override void OnApplyTemplate()
        {
            m_layoutPanel = GetTemplateChild("LayoutPanel") as LayoutPanel;
            m_layoutPanel.Layout = Layout;

            // Children is for inline usage only in Markup. If ColumnDefinitions and Children are
            // used at the same time we will throw. This property exposure is up for debate.
            if (ColumnDefinitions != null && Children.Count > 0)
            {
                throw new InvalidOperationException("Cannot have both Children and ColumnDefinitions at the same time");
            }

            if (ColumnDefinitions != null)
            {
                int index = 0;
                foreach (var columnDef in ColumnDefinitions)
                {
                    var child = columnDef.GetCellContent();
                    TableCell tableCell = null;
                    if (child is TableCell)
                    {
                        tableCell = child as TableCell;
                    }
                    else
                    {
                        tableCell = new TableCell();
                        tableCell.Content = child;
                    }

                    tableCell.ColumnIndex = index;
                    m_layoutPanel.Children.Add(tableCell);
                    ++index;
                }
            }
            else
            {
                int index = 0;
                foreach (var child in Children)
                {
                    TableCell tableCell = null;
                    if (child is TableCell)
                    {
                        tableCell = child as TableCell;
                    }
                    else
                    {
                        tableCell = new TableCell();
                        tableCell.Content = child;
                    }

                    tableCell.ColumnIndex = index;
                    m_layoutPanel.Children.Add(tableCell);
                    ++index;
                }
            }
        }

        internal void OnColumnDefinitionsChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnColumnDefinitionsChangedCore(sender, e);
        }

        protected virtual void OnColumnDefinitionsChangedCore(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (m_layoutPanel != null)
            {
                switch (e.Action)
                {
                    case System.Collections.Specialized.NotifyCollectionChangedAction.Add:
                        var addIndex = e.NewStartingIndex;
                        for (int i = 0; i < e.NewItems.Count; i++)
                        {
                            var colDef = e.NewItems[i] as ItemsViewColumnDefinitionBase;
                            var cell = new TableCell();
                            cell.ColumnIndex = e.NewStartingIndex + i;
                            cell.Content = colDef.GetCellContent();
                            m_layoutPanel.Children.Insert(e.NewStartingIndex + i, cell);
                        }
                        break;

                    case System.Collections.Specialized.NotifyCollectionChangedAction.Remove:
                        var removeIndex = e.OldStartingIndex;
                        var removeCount = e.OldItems.Count;
                        for (int i = 0; i < removeCount; i++)
                        {
                            m_layoutPanel.Children.RemoveAt(removeIndex + i);
                        }
                        break;

                    case System.Collections.Specialized.NotifyCollectionChangedAction.Move:
                        throw new NotImplementedException();

                    case System.Collections.Specialized.NotifyCollectionChangedAction.Replace:
                        throw new NotImplementedException();

                    case System.Collections.Specialized.NotifyCollectionChangedAction.Reset:
                        throw new NotImplementedException();

                    default:
                        throw new NotImplementedException();
                }
            }
        }

        LayoutPanel m_layoutPanel;
        ObservableCollection<ItemsViewColumnDefinitionBase> m_columnDefs;
    }
}
