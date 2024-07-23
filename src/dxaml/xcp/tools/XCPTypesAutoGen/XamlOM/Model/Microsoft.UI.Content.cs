// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Microsoft.UI.Content
{
    [Imported("microsoft.ui.content.idl")]
    [WindowsTypePattern]
    public sealed class DesktopChildSiteBridge
    {
    }

    [Imported("microsoft.ui.content.idl")]
    [WindowsTypePattern]
    public interface IContentIslandEnvironment
    {
    }

    [Imported("microsoft.ui.content.idl")]
    [DefaultInterfaceName("IContentIslandEnvironment")]
    [WindowsTypePattern]
    public sealed class ContentIslandEnvironment
    {
    }

    [Imported("microsoft.ui.content.idl")]
    [DefaultInterfaceName("IContentIsland")]
    [WindowsTypePattern]
    public sealed class ContentIsland
    {
    }

    [Imported("microsoft.ui.content.idl")]
    [DefaultInterfaceName("IContentCoordinateConverter")]
    [WindowsTypePattern]
    public sealed class ContentCoordinateConverter
    {
    }
}
