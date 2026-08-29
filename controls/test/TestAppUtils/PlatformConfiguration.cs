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

    public class PlatformConfiguration
    {
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
    }
}
