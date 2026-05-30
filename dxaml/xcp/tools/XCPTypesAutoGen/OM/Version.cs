// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if SILVERLIGHTXASD_BUILD
namespace SilverlightXasd
#else
namespace OM
#endif
{
    public enum ContractOSVersion : long
    {
        WinThreshold = 0x1,
        WinThreshold2 = 0x2,
        WinRedstone1 = 0x3,
        WinRedstone2 = 0x4,
        WinRedstone3 = 0x5,
        WinRedstone4 = 0x6,
        WinRedstone5 = 0x7,
    }

    public static class OSVersions
    {
        public const int VelocityFeatureVersionLast = int.MaxValue - 1;
        public const int VelocityFeatureVersionFirst = VelocityFeatureVersionLast - 1000;
        public const int InitialUAPVersion = 1;
    }
}
