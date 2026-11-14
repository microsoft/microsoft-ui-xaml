// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;

namespace ChartsSample
{
    // A StackPanel that UI Automation exposes as a named group (set AutomationProperties.Name),
    // so Narrator announces the group when focus enters it. Plain panels have no automation peer.
    public sealed partial class GroupPanel : StackPanel
    {
        protected override AutomationPeer OnCreateAutomationPeer() => new GroupPanelAutomationPeer(this);

        private sealed partial class GroupPanelAutomationPeer : FrameworkElementAutomationPeer
        {
            public GroupPanelAutomationPeer(GroupPanel owner) : base(owner)
            {
            }

            protected override AutomationControlType GetAutomationControlTypeCore() => AutomationControlType.Group;

            protected override string GetClassNameCore() => nameof(GroupPanel);

            protected override bool IsControlElementCore() => true;
        }
    }
}
