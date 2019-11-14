using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    internal class TableHeaderCellAutomationPeer : FrameworkElementAutomationPeer
    {
        new private TableHeaderCell Owner => (TableHeaderCell)base.Owner;

        public TableHeaderCellAutomationPeer(TableHeaderCell owner) : base(owner)
        {
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.HeaderItem;
        }
    }
}
