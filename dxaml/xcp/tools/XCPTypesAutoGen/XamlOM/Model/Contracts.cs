// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Windows.Foundation
{
    [ExternalIdl]
    [ContractVersion(1, xamlDirectVersion: 1)]
    [ContractVersion(2, xamlDirectVersion: 1)]
    [ContractVersion(3, xamlDirectVersion: 1)]
    [ContractVersion(4, xamlDirectVersion: 1)]
    [ContractVersion(5, xamlDirectVersion: 1)]
    [ContractVersion(6, xamlDirectVersion: 1)]
    [ContractVersion(7, xamlDirectVersion: 1)]
    [ContractVersion(8, xamlDirectVersion: 1)]
    public class UniversalApiContract : Contract { };
}

namespace Microsoft.UI.Xaml
{
    [PrivateApiContract]
    [ContractVersion(1)]
    public class PrivateApiContract : Contract { };

    [ContractVersion(1, xamlDirectVersion: 1)] // WinAppSDK 1.0
    [ContractVersion(2, xamlDirectVersion: 1)] // WinAppSDK 1.1
    [ContractVersion(3, xamlDirectVersion: 1)] // WinAppSDK 1.2
    [ContractVersion(4, xamlDirectVersion: 1)] // WinAppSDK 1.3
    [ContractVersion(5, xamlDirectVersion: 1)] // WinAppSDK 1.4
    public class WinUIContract : Contract
    {
        public const int LatestVersion = 5;
    };

    [ContractVersion(1)]
    [ContractVersion(2)]
    [ContractVersion(3)]
    [ContractVersion(4)]
    [ContractVersion(5)]
    public class XamlContract : Contract { };
}
