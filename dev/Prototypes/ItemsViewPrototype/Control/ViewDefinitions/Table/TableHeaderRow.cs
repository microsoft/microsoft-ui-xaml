using System;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Input;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class TableHeaderRow : TableRow
    {
        public TableHeaderRow()
        {
            DefaultStyleKey = typeof(TableHeaderRow);
        }

        protected override UIElement GetColumnContent(ItemsViewColumnDefinitionBase columnDef)
        {
            return columnDef?.GetHeader();
        }

        protected override TableCell GenerateTableCell()
        {
            return new TableHeaderCell();
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new TableHeaderRowAutomationPeer(this);
        }

        protected override void OnPointerReleased(PointerRoutedEventArgs e)
        {
            if (!e.Handled)
            {
                foreach (var child in LayoutPanel.Children)
                {
                    TableHeaderCell tableHeaderCell = null;
                    if (child is TableHeaderCell)
                    {
                        tableHeaderCell = child as TableHeaderCell;
                        tableHeaderCell.UpdateSortState();
                    }
                }
            }
        }
    }
}
