// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarElement))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarOverflowElement), Order = 2)]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "d55d37e9-45f0-4d39-baa7-a9815cc48415")]
    public class AppBarElementContainer
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public AppBarElementContainer() { }
    }
}
