// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using XamlOM;

namespace Windows.Foundation
{
    [ExternalIdl]
    [ContractVersion(1)]
    [ContractVersion(2)]
    [ContractVersion(3)]
    [ContractVersion(4)]
    [ContractVersion(5)]
    [ContractVersion(6)]
    [ContractVersion(7)]
    [ContractVersion(8)]
    public class UniversalApiContract : Contract { };
}

namespace Microsoft.UI.Xaml
{
    [PrivateApiContract]
    [ContractVersion(1)]
    public class PrivateApiContract : Contract { };

    [ContractVersion(1)] // WinAppSDK 1.0
    [ContractVersion(2)] // WinAppSDK 1.1
    [ContractVersion(3)] // WinAppSDK 1.2
    [ContractVersion(4)] // WinAppSDK 1.3
    [ContractVersion(5)] // WinAppSDK 1.4
    [ContractVersion(6)] // WinAppSDK 1.5
    [ContractVersion(7)] // WinAppSDK 1.6
    public class WinUIContract : Contract
    {
        public const int LatestVersion = 7;
    };

    [ContractVersion(1)]
    [ContractVersion(2)]
    [ContractVersion(3)]
    [ContractVersion(4)]
    [ContractVersion(5)]
    [ContractVersion(6)]
    [ContractVersion(7)]
    public class XamlContract : Contract { };
}
