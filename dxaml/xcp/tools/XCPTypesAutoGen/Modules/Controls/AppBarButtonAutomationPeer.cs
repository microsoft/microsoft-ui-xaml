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
    [Guids(ClassGuid = "3e0388fe-6c14-481c-aed9-60374f929c03")]
    public class AppBarButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ButtonAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public AppBarButtonAutomationPeer(Microsoft.UI.Xaml.Controls.AppBarButton owner)
            : base(owner)
        { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [ClassFlags(CanConvertFromString = true)]
    [Guids(ClassGuid = "a2f76ecb-7451-4e9d-9c24-9136601332f8")]
    public class AppBarToggleButtonAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.ToggleButtonAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public AppBarToggleButtonAutomationPeer(Microsoft.UI.Xaml.Controls.AppBarToggleButton owner)
            : base(owner)
        { }
    }
}
