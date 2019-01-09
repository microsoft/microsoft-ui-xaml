using System;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class ItemsViewAutomationPeer: FrameworkElementAutomationPeer, IItemContainerProvider, ISelectionProvider
    {
        new private ItemsView Owner => (ItemsView)base.Owner;
        public ItemsViewAutomationPeer(ItemsView owner) : base(owner)
        {
        }

        #region IItemContainerProvider

        public IRawElementProviderSimple FindItemByProperty(
            IRawElementProviderSimple startAfter,
            AutomationProperty automationProperty,
            object value)
        {
            throw new NotImplementedException();
        }

        #endregion

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.ItemContainer)
            {
                return this;
            }
            else if (patternInterface == PatternInterface.Selection)
            {
                return this;
            }

            return base.GetPatternCore(patternInterface);
        }

        protected override bool IsContentElementCore()
        {
            return true;
        }

        protected override bool IsControlElementCore()
        {
            return true; ;
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.List;
        }

        public IRawElementProviderSimple[] GetSelection()
        {
            return new IRawElementProviderSimple[] { };
        }

        public bool CanSelectMultiple
        {
            get
            {
                return this.Owner.Selector.CanSelectMultiple;
            }
        }

        public bool IsSelectionRequired
        {
            get
            {
                return this.Owner.Selector.IsSelectionRequired;
            }
        }
    }
}
