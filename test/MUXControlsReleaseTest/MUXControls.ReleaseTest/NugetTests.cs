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
            TestEnvironment.Initialize(testContext, TestType.Nuget);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestType.Nuget);
        }

        [TestMethod]
        public void PullToRefreshLoadingTest()
        {
            using (var setup = new TestSetupHelper("PullToRefresh Tests"))
            {
                var textBlock = new TextBlock(FindElement.ByName("TestTextBlock"));
                Verify.IsNotNull(textBlock);
                Verify.AreEqual(textBlock.DocumentText, "Loaded");
            }
        }
    }
}
