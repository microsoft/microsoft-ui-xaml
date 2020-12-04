using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

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
using MUXTestInfra.Shared.Infra;
using Axe.Windows.Automation;
using System.Diagnostics;

namespace MUXTestInfra.Shared.Infra
{
    public class AxeTestHelper
    {
        private static IScanner scanner = null;
        public static IScanner AxeScanner
        {
            get
            {
                if(scanner == null)
                {
                    LoadScanner();
                }
                return scanner;
            }
        }

        private static void LoadScanner()
        {
            var processes = Process.GetProcessesByName("MUXControlsTestApp");
            Verify.IsTrue(processes.Length > 0);

            var config = Config.Builder.ForProcessId(processes[0].Id).Build();

            scanner = ScannerFactory.CreateScanner(config);
        }

        public static void TestForAxeIssues()
        {
            var result = AxeScanner.Scan();
            Verify.AreEqual(0, result.ErrorCount, "Found " + result.ErrorCount + " Axe errors.");
        }
    }
}
