// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class SampleControlTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestApplicationInfo.MUXExperimentalTestApp);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TestApplicationInfo.MUXExperimentalTestApp);
        }

        [TestMethod]
        public void BasicTest()
        {
            Log.Comment("SampleControl Basic Test");
            using (var setup = new TestSetupHelper("SampleControl Tests"))
            {
                Log.Comment("Retrieve SampleControl as generic UIElement");
                UIObject sampleControl = FindElement.ByName("TestSampleControl");
                Verify.IsNotNull(sampleControl, "Verifying that we found a UIElement called TestSampleControl");
            }
        }
    }
}
