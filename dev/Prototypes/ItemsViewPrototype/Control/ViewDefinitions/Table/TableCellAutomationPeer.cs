using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Automation.Peers;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    class TableCellAutomationPeer : FrameworkElementAutomationPeer
    {
        new private TableCell Owner => (TableCell)base.Owner;

        public TableCellAutomationPeer(TableCell owner) : base(owner)
        { }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Text; // for now
        }

        protected override IList<AutomationPeer> GetChildrenCore()
        {
            return null;
        }
    }
}
