// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;
using SelectionModelSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionModelSelectionChangedEventArgs;

namespace MUXControlsTestApp.Samples.Selection
{
    public class SelectionContainerAutomationPeer : FrameworkElementAutomationPeer, ISelectionProvider
    {
        private SelectionContainer _owner;

        public SelectionContainerAutomationPeer(FrameworkElement owner) :
            base(owner)
        {
            _owner = (SelectionContainer)owner;
        }

        protected override object GetPatternCore(PatternInterface patternInterface)
        {
            if (patternInterface == PatternInterface.Selection)
            {
                return this;
            }

            return base.GetPatternCore(patternInterface);
        }

        public IRawElementProviderSimple[] GetSelection()
        {
            List<IRawElementProviderSimple> selected = new List<IRawElementProviderSimple>();
            // TODO: Need to get peers here or work with accesibility team to figure 
            // out what to give here. The selected elements might not be realized.
            return selected.ToArray();
        }

        public bool CanSelectMultiple
        {
            get
            {
                return !_owner.SelectionModel.SingleSelect;
            }
        }

        public bool IsSelectionRequired
        {
            get
            {
                return false;
            }
        }

        internal void SelectionChanged(SelectionModelSelectionChangedEventArgs args)
        {
            RaiseAutomationEvent(AutomationEvents.SelectionPatternOnInvalidated);
        }
    }
}