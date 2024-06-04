// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.ApplicationModel
{
    [Imported("windows.applicationmodel.core.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class SuspendingEventArgs
    {
    }

    [Imported("windows.applicationmodel.core.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class LeavingBackgroundEventArgs
    {
    }

    [Imported("windows.applicationmodel.core.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class EnteredBackgroundEventArgs
    {
    }


}
