// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;
using SelectionModelSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionModelSelectionChangedEventArgs;

namespace MUXControlsTestApp.Samples.Selection
{
    public class SelectionContainer : ContentControl
    {
        public SelectionContainer()
        {
            IsTabStop = false;
        }

        public SelectionModel SelectionModel
        {
            get { return (SelectionModel)GetValue(SelectionModelProperty); }
            set { SetValue(SelectionModelProperty, value); }
        }

        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.Register("SelectionModel", typeof(SelectionModel), typeof(SelectionContainer), new PropertyMetadata(null, new PropertyChangedCallback(OnSelectionModelChanged)));

        private static void OnSelectionModelChanged(DependencyObject sender, DependencyPropertyChangedEventArgs args)
        {
            if(args.OldValue != null)
            {
                var old = (SelectionModel)args.OldValue;
                old.SelectionChanged -= (sender as SelectionContainer).OnSelectionChanged;
            }

            if(args.NewValue != null)
            {
                var newValue = (SelectionModel)args.NewValue;
                newValue.SelectionChanged += (sender as SelectionContainer).OnSelectionChanged;
            }
        }

        private void OnSelectionChanged(SelectionModel sender, SelectionModelSelectionChangedEventArgs args)
        {
            if(AutomationPeer.ListenerExists(AutomationEvents.SelectionPatternOnInvalidated))
            {
                var peer = (SelectionContainerAutomationPeer)FrameworkElementAutomationPeer.CreatePeerForElement(this);
                peer.SelectionChanged(args);
            }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new SelectionContainerAutomationPeer(this);
        }
    }
}
