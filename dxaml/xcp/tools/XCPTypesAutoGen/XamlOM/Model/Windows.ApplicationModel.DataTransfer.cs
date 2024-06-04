// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;
using XamlOM.NewBuilders;

namespace Windows.ApplicationModel.DataTransfer
{
    [Imported("datapackage.idl")]
    [WindowsTypePattern]
    [Guids(ClassGuid = "643cd87c-02ab-4145-8690-69f812a08f59")]
    public sealed class DataPackage
    {
    }

    [Imported("datapackage.idl")]
    [WindowsTypePattern]
    public sealed class DataPackageView
    {
    }

    [Imported("datapackage.idl")]
    [WindowsTypePattern]
    public enum DataPackageOperation
    {
        [NativeValueName("XcpDataPackageOperation_None")]
        DataPackageOperation_None    = 0,
        [NativeValueName("XcpDataPackageOperation_Copy")]
        DataPackageOperation_Copy    = 1,
        [NativeValueName("XcpDataPackageOperation_Move")]
        DataPackageOperation_Move    = 2,
        [NativeValueName("XcpDataPackageOperation_Link")]
        DataPackageOperation_Link    = 4
    }
}
