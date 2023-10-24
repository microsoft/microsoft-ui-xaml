// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Automation.Provider.IWindowProvider))]
    [Guids(ClassGuid = "cf6c94c5-1a3c-4269-b8d7-e5cb4a4f7a01")]
    internal class FullWindowMediaRootAutomationPeer
     : Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
    {
        [FactoryMethodName("CreateInstanceWithOwner")]
        public FullWindowMediaRootAutomationPeer(Microsoft.UI.Xaml.FrameworkElement owner)
            : base(owner) { }
    }
}

