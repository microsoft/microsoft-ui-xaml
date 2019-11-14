// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public class TestHelpers
    {
        [DllImport("dwmapi.dll")]
        static extern int DwmGetUnmetTabRequirements(IntPtr hwnd, out int requirements);

        public static bool SystemTabbedShellIsEnabled
        {
            get
            {
                if (!s_systemTabbedShellIsEnabledCached)
                {
                    int requirements = 0;

                    try
                    {
                        DwmGetUnmetTabRequirements(IntPtr.Zero, out requirements);
                        if (requirements == 0) // 0 = DWMTWR_NONE
                        {
                            s_systemTabbedShellIsEnabled = true;
                        }
                    }
                    catch(MissingMethodException)
                    {
                        Log.Error("DwmGetUnmetTabRequirements not available on this platform");
                    }

                    s_systemTabbedShellIsEnabledCached = true;
                }

                return s_systemTabbedShellIsEnabled;
            }
        }

        static bool s_systemTabbedShellIsEnabledCached;
        static bool s_systemTabbedShellIsEnabled;
    }
}
