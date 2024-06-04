using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using System.Numerics;
using Common;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class PreRunTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("IgnoreInContinuousIntegration", "True")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Desktop))
            {
                TestEnvironment.Initialize(testContext);
            }
        }

        [TestCleanup]
        public void TestCleanup()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Desktop))
            {
                TestCleanupHelper.Cleanup();
            }
        }

        [TestMethod]
        public void PreRunSentinel()
        {
            Log.Comment("Pre-run test");
        }
    }
}