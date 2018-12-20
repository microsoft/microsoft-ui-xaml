using System;
using System.Diagnostics;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class TableHeaderRow : TableRow
    {
        public TableHeaderRow()
        {
            DefaultStyleKey = typeof(TableHeaderRow);
        }

        protected override void OnApplyTemplate()
        {
            m_layoutPanel = GetTemplateChild("LayoutPanel") as LayoutPanel;
            m_layoutPanel.Layout = Layout;

            if (ColumnDefinitions != null && Children.Count > 0)
            {
                throw new InvalidOperationException("Cannot have both Children and ColumnDefinitions at the same time");
            }

            if (ColumnDefinitions != null)
            {
                int index = 0;
                foreach (var columnDef in ColumnDefinitions)
                {
                    var child = columnDef.GetHeader();
                    TableHeaderCell tableCell = null;
                    if (child is TableHeaderCell)
                    {
                        tableCell = child as TableHeaderCell;
                    }
                    else
                    {
                        tableCell = new TableHeaderCell();
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
                    TableHeaderCell tableCell = null;
                    if (child is TableHeaderCell)
                    {
                        tableCell = child as TableHeaderCell;
                    }
                    else
                    {
                        tableCell = new TableHeaderCell();
                        tableCell.Content = child;
                    }

                    tableCell.ColumnIndex = index;
                    m_layoutPanel.Children.Add(tableCell);
                    ++index;
                }
            }
        }

        protected override void OnColumnDefinitionsChangedCore(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
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
                            var cell = new TableHeaderCell();
                            cell.ColumnIndex = e.NewStartingIndex + i;
                            cell.Content = colDef.GetHeader();
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
    }
}
