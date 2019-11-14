using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class TableHeaderRowAutomationPeer : ItemContainerAutomationPeer, IValueProvider
    {
        new private TableHeaderRow Owner => (TableHeaderRow)base.Owner;

        public TableHeaderRowAutomationPeer(TableHeaderRow owner) : base(owner)
        {
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Header;
        }

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.Value)
            {
                return this;
            }
            else if (patternInterface == PatternInterface.Selection || 
                patternInterface == PatternInterface.Invoke)
            {
                return null;
            }

            return base.GetPatternCore(patternInterface);
        }

        public void SetValue(string value)
        {
            throw new NotImplementedException();
        }

        public bool IsReadOnly => true;

        public string Value => String.Empty;
    }
}
