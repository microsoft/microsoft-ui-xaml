// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.ApplicationModel;
using Windows.Foundation.Metadata;

namespace MUXControls.TestAppUtils
{
    public static class DesignModeHelpers
    {
        private static bool s_inDesignModeCached;
        private static bool s_inDesignMode;
        private static bool GetIsInDesignMode()
        {
            if (DesignMode.DesignModeEnabled) return true;

            if (ApiInformation.IsPropertyPresent("Windows.ApplicationModel.DesignMode", "DesignMode2Enabled"))
            {
                if (DesignMode.DesignMode2Enabled) return true;
            }

            return false;
        }

        public static bool IsInDesignMode
        {
            get
            {
                if (!s_inDesignModeCached)
                {
                    s_inDesignMode = GetIsInDesignMode();
                    s_inDesignModeCached = true;
                }

                return s_inDesignMode;
            }
        }
    }
}
