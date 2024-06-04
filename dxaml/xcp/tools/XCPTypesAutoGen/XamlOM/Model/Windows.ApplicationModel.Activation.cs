// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.ApplicationModel.Activation
{
    [Imported("windows.applicationmodel.activation.events.idl")]
    [WindowsTypePattern]
    public interface IActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")] 
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class LaunchActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class FileActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class SearchActivatedEventArgs
    {
    }

    [Imported("sharingextension.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class ShareTargetActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class FileOpenPickerActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class FileSavePickerActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class CachedFileUpdaterActivatedEventArgs
    {
    }

    [Imported("windows.applicationmodel.activation.events.idl")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [WindowsTypePattern]
    public sealed class BackgroundActivatedEventArgs
    {
    }
}

