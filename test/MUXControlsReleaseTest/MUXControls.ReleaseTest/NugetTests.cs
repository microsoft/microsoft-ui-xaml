// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace MUXControls.ReleaseTest
{
    [TestClass]
    public class NugetTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplicationInfo.NugetPackageTestApp);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            PGOManager.PGOSweepIfInstrumented(TestEnvironment.TestContext.TestName);
            TestEnvironment.AssemblyCleanupWorker(TestApplicationInfo.NugetPackageTestApp);
        }

        [TestMethod]
        public void PullToRefreshLoadingTest()
        {
            using (var setup = new TestSetupHelper("PullToRefresh Tests"))
            {
                var textBlock = new TextBlock(FindElement.ByName("TestTextBlock"));
                Verify.IsNotNull(textBlock);
                Verify.AreEqual(textBlock.DocumentText, "Loaded");

                var button = new Button(FindElement.ById("RequestRefresh"));
                button.Invoke();
            }
        }

        [TestMethod]
        public void CompactDictionaryNoCrashTest()
        {
            using (var setup = new TestSetupHelper("CompactDictionary Tests"))
            {
                var textBlock = new TextBlock(FindElement.ByName("CompactTextBox"));
                Verify.IsNotNull(textBlock);
            }
        }

        [TestMethod]
        public void RepeaterBasicTests()
        {
            using (var setup = new TestSetupHelper("Repeater Tests"))
            {
                var AddItemsButton = new Button(FindElement.ByName("AddItemsButton"));
                AddItemsButton.Click();

                Verify.IsNotNull(FindElement.ByName("Item1"));
                Verify.IsNotNull(FindElement.ByName("Item2"));
                Verify.IsNotNull(FindElement.ByName("Item3"));

                var clearOutputButton = new Button(FindElement.ByName("ClearOutputButton"));
                clearOutputButton.Click();

                var removeItemButton = new Button(FindElement.ByName("RemoveItemButton"));
                removeItemButton.Click();
                var outputText = new TextBlock(FindElement.ByName("OutputText"));
                Verify.IsTrue(outputText.DocumentText.Contains("Element Cleared"));

                clearOutputButton.Click();

                var gridLayoutButton = new Button(FindElement.ByName("UniformGridLayoutButton"));
                gridLayoutButton.Click();
                Verify.IsTrue(outputText.DocumentText.Contains("Element Cleared"));
                Verify.IsTrue(outputText.DocumentText.Contains("0Prepared"));
                Verify.IsTrue(outputText.DocumentText.Contains("1Prepared"));
            }
        }

        // This test covers loading nav view, reveal and acrylic brushes for PGO training
        [TestMethod]
        public void NavigationViewBasicTest()
        {
            using (var setup = new TestSetupHelper("NavigationView Tests"))
            {
                var button = new Button(FindElement.ByName("Close Navigation"));
                button.Click();
                Verify.IsNotNull(FindElement.ByName("Item1"));
                Verify.IsNotNull(FindElement.ByName("Item2"));
                Verify.IsNotNull(FindElement.ByName("Item3"));
                button.Click();
            }
        }

        [TestMethod]
        public void VerifyNavigationViewCustomHeaderContentMargins()
        {
            using (var setup = new TestSetupHelper("NavigationView with custom resources Tests"))
            {
                var tblTopHeaderContentMarginResult = new TextBlock(FindElement.ByName("tblTopHeaderContentMarginResult"));
                Verify.AreEqual("16,3,0,7", tblTopHeaderContentMarginResult.GetText());

                var tblLeftMinimalHeaderContentMarginResult = new TextBlock(FindElement.ByName("tblLeftMinimalHeaderContentMarginResult"));
                Verify.AreEqual("18,1,0,0", tblLeftMinimalHeaderContentMarginResult.GetText());
            }
        }
    }
}
