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
    [ContractVersion(8)] // WinAppSDK 1.7
    [ContractVersion(9)] // WinAppSDK 1.8
    [ContractVersion(10)] // WinAppSDK 2.0
    [ContractVersion(11)] // WinAppSDK 2.2
    [ContractVersion(12)] // WinAppSDK 3.0
    public class WinUIContract : Contract
    {
        // Put new APIs in the Experimental contract version. Move into the specific
        // WinAppSDK version after completing API review and determining the stable
        // release version.
        public const int Experimental  = 12;

        public const int WinAppSDK_2_0 = 10;
        public const int WinAppSDK_2_2 = 11;

        public const int WinAppSDK_3_0 = 12;
    };

    [ContractVersion(1)]
    [ContractVersion(2)]
    [ContractVersion(3)]
    [ContractVersion(4)]
    [ContractVersion(5)]
    [ContractVersion(6)]
    [ContractVersion(7)]
    [ContractVersion(8)]
    [ContractVersion(9)]
    [ContractVersion(10)]
    [ContractVersion(11)]
    [ContractVersion(12)]
    public class XamlContract : Contract { };
}
