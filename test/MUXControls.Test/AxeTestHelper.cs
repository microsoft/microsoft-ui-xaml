// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif
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
                if (scanner == null)
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
