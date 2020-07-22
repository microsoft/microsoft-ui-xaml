// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp.Samples.Selection
{
    public class RepeaterItemAutomationPeer : FrameworkElementAutomationPeer, ISelectionItemProvider
    {
        private RepeaterItem _owner;
        public RepeaterItemAutomationPeer(FrameworkElement owner) : base(owner)
        {
            _owner = (RepeaterItem)owner;
        }

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.SelectionItem)
            {
                return this;
            }

            return base.GetPatternCore(patternInterface);
        }

        public void AddToSelection()
        {
            _owner.SelectionModel.Select(_owner.RepeatedIndex);
        }

        public void RemoveFromSelection()
        {
            _owner.SelectionModel.Deselect(_owner.RepeatedIndex);
        }

        public void Select()
        {
            _owner.SelectionModel.Select(_owner.RepeatedIndex);
        }

        public bool IsSelected
        {
            get
            {
                return _owner.SelectionModel.IsSelected(_owner.RepeatedIndex).Value;
            }
        }

        public IRawElementProviderSimple SelectionContainer
        {
            get
            {
                // TODO: could cache this to avoid the walk every time
                IRawElementProviderSimple containerPeer = null;
                FrameworkElement element = _owner;
                while (element != null && !(element is SelectionContainer))
                {
                    element = (FrameworkElement)element.Parent;
                }

                if (element != null)
                {
                    var container = (SelectionContainer)element;
                    containerPeer = ProviderFromPeer(CreatePeerForElement(container));
                }

                return containerPeer;
            }
        }

        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.ListItem;
        }

        protected override string GetLocalizedControlTypeCore()
        {
            return "Item";
        }
    }
}
