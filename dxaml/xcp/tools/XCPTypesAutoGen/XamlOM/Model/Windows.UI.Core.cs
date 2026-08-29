// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.UI.Core
{
    [Imported("windows.ui.core.corewindow.idl")]
    [WindowsTypePattern]
    public struct CorePhysicalKeyStatus
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class CoreWindow
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class CoreDispatcher
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class WindowActivatedEventArgs
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class CoreWindowEventArgs
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class WindowSizeChangedEventArgs
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class VisibilityChangedEventArgs
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [WindowsTypePattern]
    public sealed class PointerEventArgs
    {
    }

    [Imported("windows.ui.core.corewindow.idl")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public enum CoreCursorType
    {
        Arrow = 0,
        Cross = 1,
        Custom = 2,
        Hand = 3,
        Help = 4,
        IBeam = 5,
        SizeAll = 6,
        SizeNortheastSouthwest = 7,
        SizeNorthSouth = 8,
        SizeNorthwestSoutheast = 9,
        SizeWestEast = 10,
        UniversalNo = 11,
        UpArrow = 12,
        Wait = 13,
        Pin = 14,
        Person = 15,
    }
}

