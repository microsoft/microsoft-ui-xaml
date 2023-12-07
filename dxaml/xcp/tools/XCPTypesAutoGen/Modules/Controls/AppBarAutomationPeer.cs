// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Automation.Peers
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IExpandCollapseProvider), Version = 1)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IToggleProvider))]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IWindowProvider), Version = 1)]
    [Guids(ClassGuid = "47452960-ab99-4331-8c7f-93fd769b560a")]
    public class AppBarAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public AppBarAutomationPeer(Microsoft.UI.Xaml.Controls.AppBar owner)
            : base(owner)
        { }
    }
}
