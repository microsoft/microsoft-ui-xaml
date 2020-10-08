// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
    public class RepeaterTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestMethod]
        public void VerifyUniformGridLayoutDoesNotCreateHoles()
        {
            using (var setup = new TestSetupHelper("ItemsRepeater Tests"))
            {
                // Open page
                FindElement.ByName("UniformGridLayoutDemo").Click();
                
                // Scroll down
                var scrollviewer = FindElement.ByName("RepeaterScrollViewer");
                var repeaterHeightButton = FindElement.ByName("GetRepeaterActualHeightButton");
                repeaterHeightButton.Click();
                // Get actual repeater height
                var oldActualRepeaterHeight = double.Parse(FindElement.ByName("RepeaterActualHeightLabel").GetText());

                InputHelper.RotateWheel(scrollviewer, -2000);
                Wait.ForIdle();

                // Scroll back up again, users don't scroll large offsets at once but rather small chunks in succession.
                for(int i=0;i<5;i++)
                {
                    InputHelper.RotateWheel(scrollviewer, 50);
                    Wait.ForIdle();
                }

                repeaterHeightButton.Click();
                Verify.IsTrue(Math.Abs(oldActualRepeaterHeight - 
                    double.Parse(FindElement.ByName("RepeaterActualHeightLabel").GetText())) < 1,
                    "Repeater heights did not match. This indicates that there are holes in layout as the repeater now needed more/less space.");
            }
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }
    }
}
