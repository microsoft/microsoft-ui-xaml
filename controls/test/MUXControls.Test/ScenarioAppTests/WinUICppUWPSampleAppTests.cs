using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using System;
using System.Numerics;
using Common;
using System.Threading.Tasks;
using Microsoft.Windows.Apps.Test.Foundation;
using System.Threading;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    #if MUX_PRERELEASE // Only run with UWP
    [TestClass]
    #endif
    public class WinUICppUWPSampleAppTests
    {
        public static TestApplicationInfo WinUICppUWPSampleApp
        {
            get
            {
                return new TestApplicationInfo(
                    "WinUICppUWPSampleApp",
                    "WinUICppUWPSampleApp_6f07fta6qpts2!App",
                    "WinUICppUWPSampleApp_6f07fta6qpts2",
                    "WinUICppUWPSampleApp",
                    "WinUICppUWPSampleApp.exe",
                    "WinUICppUWPSampleApp",
                    isUwpApp: true,
                    TestApplicationInfo.MUXCertSerialNumber,
                    TestApplicationInfo.MUXBaseAppxDir);
            }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "ScenarioTestSuite")]
        [TestProperty("IgnoreForValidateWindowsAppSDK", "True")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Class")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, WinUICppUWPSampleApp);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            
        }

        [ClassCleanupAttribute]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(WinUICppUWPSampleApp);
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled due to #36781520
        public void SimpleLaunchTest()
        {
            var textBlock = new TextBlock(FindElement.ByName("TestTextBlock"));
            Verify.IsNotNull(textBlock);

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Loaded",
                () => textBlock.DocumentText);

            Log.Comment("Clicking button");
            var button = new Button(FindElement.ByName("MyButton"));
            button.Click();

            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            TestEnvironment.VerifyAreEqualWithRetry(20,
                () => "Clicked",
                () => textBlock.DocumentText);
        }

        [TestMethod]
        [Description("Performs simple verification of x:Uid property substitution")]
        [TestProperty("Ignore", "True")] // Disabled due to #36781520
        public void SimpleXUidVerification()
        {
            // Verify string on Page 1 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 1!");

            Log.Comment("Navigating to Page 2");
            var button = new Button(FindElement.ByName("navigationButton"));
            button.Click();
            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            ElementCache.Clear();

            // Verify string on Page 2 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 2!");

            Log.Comment("Navigating to Page 1");
            button = new Button(FindElement.ByName("navigationButton"));
            button.Click();
            Log.Comment("Wait For Idle");
            Wait.ForIdle();

            ElementCache.Clear();

            // Verify string on Page 1 (exercises x:Uid)
            WinUISampleAppTestsUtils.VerifyText("TestTextBlock", "Loaded");
            WinUISampleAppTestsUtils.VerifyText("titleTextBlock", "Welcome to Page 1!");
        }
    }
}