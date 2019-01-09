using System;
using System.Diagnostics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;

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
    }
}
