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
    public class NugetTestsCX
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestType.NugetCX);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestType.NugetCX);
        }

        [TestMethod]
        public void BasicTestCX()
        {
            var textBlock = new TextBlock(FindElement.ByName("TestTextBlock"));
            Verify.IsNotNull(textBlock);
            Verify.AreEqual(textBlock.DocumentText, "Loaded");
        }

        [TestMethod]
        public void RepeaterNoCrashTest()
        {
            var button = new Button(FindElement.ByName("AddItemsButton"));
            button.Click();

            var item3 = FindElement.ByName("Item3");
            Verify.IsNotNull(item3);
        }
    }
}