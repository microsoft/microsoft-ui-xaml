// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.System.Profile;

namespace MUXControls.TestAppUtils
{
    public enum DeviceType
    {
        Desktop,
        Phone
    }

    public enum OSVersion : ushort
    {
        Threshold2 = 2,
        Redstone1 = 3,
        Redstone2 = 4,
        Redstone3 = 5,
        Redstone4 = 6,
        Redstone5 = 7
    }

    public class PlatformConfiguration
    {
        const OSVersion MaxOSVersion = OSVersion.Redstone2;

        public static bool IsDevice(DeviceType type)
        {
            var deviceFamily = AnalyticsInfo.VersionInfo.DeviceFamily;

            if (type == DeviceType.Desktop && deviceFamily == "Windows.Desktop")
            {
                return true;
            }
            else if (type == DeviceType.Phone && deviceFamily == "Windows.Mobile")
            {
                return true;
            }

            return false;
        }

        public static bool IsOsVersion(OSVersion version)
        {
            // We can determine the OS version by checking for the presence of the Universal contract
            // corresonding to that version and the absense of the contract version corresonding to
            // the next OS version.
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", (ushort)version) &&
                ((version == MaxOSVersion) || !ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", (ushort)(version + 1)));
        }

        public static bool IsOsVersionGreaterThan(OSVersion version)
        {
            // We can determine that the OS version is greater than the specified version by checking for
            // the presence of the Universal contract corresonding to the next version.
            return ApiInformation.IsApiContractPresent("Windows.Foundation.UniversalApiContract", (ushort)(version + 1));
        }

        public static bool IsOsVersionGreaterThanOrEqual(OSVersion version)
        {
            return IsOsVersionGreaterThan(version) || IsOsVersion(version);
        }

        public static bool IsOSVersionLessThan(OSVersion version)
        {
            return !IsOsVersionGreaterThanOrEqual(version);
        }
    }
}
