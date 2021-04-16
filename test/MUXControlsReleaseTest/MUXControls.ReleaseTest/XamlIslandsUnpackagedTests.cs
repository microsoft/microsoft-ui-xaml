// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

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

namespace MUXControls.ReleaseTest
{
    [TestClass]
    public class XamlIslandsUnpackagedTests : XamlIslandsTestsBase
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5))
            {
                // UIA in Xaml islands is only available in 19H1
                return;
            }

            TestEnvironment.Initialize(testContext, TestApplicationInfo.XamlIslandsTestAppUnpackaged); 
        }

        [TestCleanup]
        public void TestCleanup()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5))
            {
                // UIA in Xaml islands is only available in 19H1
                return;
            }

            TestEnvironment.AssemblyCleanupWorker(TestApplicationInfo.XamlIslandsTestAppUnpackaged);
        }
    }
}
