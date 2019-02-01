using System;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class TableRow : ItemContainer
    {
        public TableRow()
        {
            DefaultStyleKey = typeof(TableRow);
            DragAndDropBehavior.SetSource(this,
                new DragAndDropPayload() {
                    Data = this,
                    Operation = DragAndDropOperation.Copy
                });
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

        protected LayoutPanel LayoutPanel { get; set; }
        protected override void OnApplyTemplate()
        {
            LayoutPanel = GetTemplateChild("LayoutPanel") as LayoutPanel;

            this.GenerateColumnContent();
        }

        protected void GenerateColumnContent()
        {
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
                    var child = GetColumnContent(columnDef);
                    TableCell tableCell = null;
                    if (child is TableCell)
                    {
                        tableCell = child as TableCell;
                    }
                    else
                    {
                        tableCell = GenerateTableCell();
                        tableCell.Content = child;
                    }

                    tableCell.ColumnIndex = index;
                    LayoutPanel.Children.Add(tableCell);
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
                        tableCell = GenerateTableCell();
                        tableCell.Content = child;
                    }

                    tableCell.ColumnIndex = index;
                    LayoutPanel.Children.Add(tableCell);
                    ++index;
                }
            }
        }

        protected virtual UIElement GetColumnContent(ItemsViewColumnDefinitionBase columnDef)
        {
            return columnDef?.GetCellContent();
        }

        protected virtual TableCell GenerateTableCell()
        {
            return new TableCell();
        }

        internal void OnColumnDefinitionsChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            OnColumnDefinitionsChangedCore(sender, e);
        }

        protected virtual void OnColumnDefinitionsChangedCore(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            if (LayoutPanel != null)
            {
                switch (e.Action)
                {
                    case System.Collections.Specialized.NotifyCollectionChangedAction.Add:
                        var addIndex = e.NewStartingIndex;
                        for (int i = 0; i < e.NewItems.Count; i++)
                        {
                            var colDef = e.NewItems[i] as ItemsViewColumnDefinitionBase;
                            var cell = GenerateTableCell();
                            cell.ColumnIndex = e.NewStartingIndex + i;
                            cell.Content = GetColumnContent(colDef);
                            LayoutPanel.Children.Insert(e.NewStartingIndex + i, cell);
                        }
                        break;

                    case System.Collections.Specialized.NotifyCollectionChangedAction.Remove:
                        var removeIndex = e.OldStartingIndex;
                        var removeCount = e.OldItems.Count;
                        for (int i = 0; i < removeCount; i++)
                        {
                            LayoutPanel.Children.RemoveAt(removeIndex + i);
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

        ObservableCollection<ItemsViewColumnDefinitionBase> m_columnDefs;
    }
}
