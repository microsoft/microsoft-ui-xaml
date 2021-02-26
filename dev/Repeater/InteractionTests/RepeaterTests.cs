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

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void InsertAtStartBehavior()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsRepeater Tests" }))
            {
                FindElement.ByName("Basic Demo").Click();
                var addItemButton = FindElement.ByName("InsertAtStartButton");
                var itemCountLabel = FindElement.ByName("InsertAtStartChildCountLabel");

                for (int i = 0; i < 10; i++)
                {
                    // For performance reasons, invoking the button also reevaluates the children count.
                    // Technically there are i+1 children, but the button is one item behind.
                    // Since i starts at 0, everything lines up correctly in here and we don't have to invoke two buttons.
                    addItemButton.Click();
                    Wait.ForIdle();
                    Verify.AreEqual(i.ToString(), itemCountLabel.GetText());
                }
            }
        }
    }
}
