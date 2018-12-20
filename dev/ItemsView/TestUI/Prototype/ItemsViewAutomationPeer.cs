using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class ItemsViewAutomationPeer: FrameworkElementAutomationPeer, ISelectionProvider
    {
        private ItemsView m_owner;
        public ItemsViewAutomationPeer(ItemsView owner) : base(owner)
        {
            m_owner = owner;
        }

        public IRawElementProviderSimple[] GetSelection()
        {
            return new IRawElementProviderSimple[] { };
        }

        public bool CanSelectMultiple
        {
            get
            {
                return m_owner.Selector.CanSelectMultiple;
            }
        }

        public bool IsSelectionRequired
        {
            get
            {
                return m_owner.Selector.IsSelectionRequired;
            }
        }
    }
}
