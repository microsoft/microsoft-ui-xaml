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

using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using MUXTestInfra.Shared.Infra;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class AutoSuggestBox
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }
        
        [TestMethod]
        public void CanSelectSuggestion()
        {
            using (var setup = new TestSetupHelper("AutoSuggestBox Tests"))
            {
                Edit autoSuggestBoxTextBox = new Edit(FindElement.ByNameAndClassName("AutoSuggestBox with suggestions", "TextBox"));
                FocusHelper.SetFocus(autoSuggestBoxTextBox);
                KeyboardHelper.EnterText(autoSuggestBoxTextBox, "test");
                KeyboardHelper.PressKey(Key.Enter);
                InvokeImplementation dolorItem = new InvokeImplementation(FindElement.ByNameAndClassName("dolor", "ListViewItem"));

                using (ValueChangedEventWaiter waiter = new ValueChangedEventWaiter(autoSuggestBoxTextBox, "dolor"))
                {
                    dolorItem.Invoke();
                }

                Verify.AreEqual("dolor", autoSuggestBoxTextBox.Value);
            }
        }

        [TestMethod]
        public void VerifyAxeScanPasses()
        {
            using (var setup = new TestSetupHelper("AutoSuggestBox-Axe"))
            {
                AxeTestHelper.TestForAxeIssues();
            }
        }
    }
}
