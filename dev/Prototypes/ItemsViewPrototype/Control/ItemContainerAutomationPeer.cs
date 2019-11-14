using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace DEPControlsTestApp.ItemsViewPrototype
{
    public class ItemContainerAutomationPeer : FrameworkElementAutomationPeer, ISelectionItemProvider, IInvokeProvider
    {
        new private ItemContainer Owner => (ItemContainer)base.Owner;

        public ItemContainerAutomationPeer(ItemContainer owner) : base(owner)
        {
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.ListItem;
        }

        protected override string GetNameCore()
        {
            var name = base.GetNameCore();

            if (String.IsNullOrWhiteSpace(name))
            {
                var children = this.GetChildren();
                if (children.Count == 0)
                    return name;

                // Use the name of the first two things found
                StringBuilder stringBuilder = new StringBuilder();
                for (int i = 0, j = 0; i < children.Count && j < 2; i++)
                {
                    var child = children[i];
                    var childName = child.GetName();
                    if (!String.IsNullOrWhiteSpace(childName))
                    {
                        stringBuilder.Append(childName);
                        stringBuilder.Append(' ');
                        j++;
                    }
                }

                name = stringBuilder.ToString();
            }

            return name;
        }

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.SelectionItem &&
                this.Owner.Selector != null)
                return this;
            else if (patternInterface == PatternInterface.Invoke)
                return this;

            return base.GetPatternCore(patternInterface);
        }

        #region position/size/level

        protected override int GetPositionInSetCore()
        {
            var index = this.Owner.GetIndexPath();
            return index.GetAt(index.GetSize() - 1) + 1;
        }

        protected override int GetSizeOfSetCore()
        {
            var repeater = (ItemsRepeater)this.Owner.Parent;
            return repeater.ItemsSourceView.Count;
        }

        protected override int GetLevelCore()
        {
            var index = this.Owner.GetIndexPath();
            var level = index.GetSize();
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
            var wasSelected = this.IsSelected;
            this.Owner.Selector?.Delegate?.OnPrimaryInteractionAction(this.Owner.GetIndexPath(), true, false);
            if (!wasSelected && this.IsSelected)
            {
                this.RaiseAutomationEvent(AutomationEvents.SelectionItemPatternOnElementAddedToSelection);
            }
        }

        public void RemoveFromSelection()
        {
            var wasSelected = this.IsSelected;
            this.Owner.Selector?.Model.DeselectAt(this.Owner.GetIndexPath());
            if (wasSelected && !this.IsSelected)
            {
                this.RaiseAutomationEvent(AutomationEvents.SelectionItemPatternOnElementRemovedFromSelection);
            }
        }

        public void Select()
        {
            var wasSelected = this.IsSelected;
            this.Owner.Selector.Model.SelectAt(this.Owner.GetIndexPath());
            if (!wasSelected && this.IsSelected)
            {
                this.RaiseAutomationEvent(AutomationEvents.SelectionItemPatternOnElementSelected);
            }
        }

        public bool IsSelected
        {
            get
            {
                return this.Owner.IsSelected;
            }
        }

        public IRawElementProviderSimple SelectionContainer
        {
            get
            {
                var peer = FrameworkElementAutomationPeer.FromElement(this.Owner.Selector.SelectorView) as FrameworkElementAutomationPeer;
                return ProviderFromPeer(peer);
            }
        }

        public void Invoke()
        {
            throw new NotImplementedException();
        }

        #endregion
    }
}
