using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp.ItemsViewPrototype
{
    public class ItemsViewItemAutomationPeer : FrameworkElementAutomationPeer, ISelectionItemProvider
    {
        private ItemsViewItem m_owner;

        public ItemsViewItemAutomationPeer(ItemsViewItem owner) : base(owner)
        {
            m_owner = owner;
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.ListItem;
        }

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.SelectionItem &&
                m_owner.Selector != null)
                return this;
            //else if (patternInterface == PatternInterface.Invoke)
            //    return this;

            return base.GetPatternCore(patternInterface);
        }

        #region position/size/level

        protected override int GetPositionInSetCore()
        {
            var index = m_owner.GetIndexPath();
            return index.GetAt(index.GetSize() - 1) + 1;
        }

        protected override int GetSizeOfSetCore()
        {
            var repeater = (Repeater)m_owner.Parent;
            return repeater.DataSource.GetSize();
        }

        protected override int GetLevelCore()
        {
            var index = m_owner.GetIndexPath();
            var level =  index.GetSize();
            // If just single level, we don't want to read out 
            // the level at all. This seems to be how ListView 
            // does it.
            if (level == 1)
                return -1;
            else
                return level;
        }

        #endregion

        #region ISelectionItemProvider

        public void AddToSelection()
        {
            m_owner.Selector.Model.SelectAt(m_owner.GetIndexPath());
        }

        public void RemoveFromSelection()
        {
            m_owner.Selector.Model.DeselectAt(m_owner.GetIndexPath());
        }

        public void Select()
        {
            m_owner.Selector.Model.SelectAt(m_owner.GetIndexPath());
        }

        public bool IsSelected
        {
            get
            {
                return m_owner.IsSelected;
            }
        }

        public IRawElementProviderSimple SelectionContainer
        {
            get
            {
                var peer = FrameworkElementAutomationPeer.FromElement(m_owner.Selector.SelectionContainer) as FrameworkElementAutomationPeer;
                return ProviderFromPeer(peer);
            }
        }

        #endregion
    }
}
