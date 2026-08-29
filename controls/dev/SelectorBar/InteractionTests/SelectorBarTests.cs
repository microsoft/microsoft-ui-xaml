// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class SelectorBarTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verify SelectorBarItem selection through mouse click and keyboard arrow.")]
        public void SelectorBarItemSelection()
        {
            using (var setup = new TestSetupHelper(new[] { "SelectorBar Tests", "navigateToSample" }))
            {
                Wait.ForIdle();
                Log.Comment("Expecting SelectorBar.SelectedItem: selectorBarItemFavorites");
                Verify.AreEqual("selectorBarItemFavorites", SelectorBarSelectedItem());

                Log.Comment("Retrieving selectorBarItemShared");
                var selectorBarItemShared = FindElement.ById("selectorBarItemShared");
                Verify.IsNotNull(selectorBarItemShared, "Verifying that SelectorBarItem selectorBarItemShared was found");

                InputHelper.LeftClick(selectorBarItemShared);
                Wait.ForIdle();
                Log.Comment("Expecting SelectorBar.SelectedItem: selectorBarItemShared");
                Verify.AreEqual("selectorBarItemShared", SelectorBarSelectedItem());

                Log.Comment("Retrieving selectorBarItemRemote");
                var selectorBarItemRemote = FindElement.ById("selectorBarItemRemote");
                Verify.IsNotNull(selectorBarItemRemote, "Verifying that SelectorBarItem selectorBarItemRemote was found");

                KeyboardHelper.PressKey(Key.Left);
                Wait.ForIdle();
                Log.Comment("Expecting SelectorBar.SelectedItem: selectorBarItemRemote");
                Verify.AreEqual("selectorBarItemRemote", SelectorBarSelectedItem());
            }
        }

        private string SelectorBarSelectedItem()
        {
            Log.Comment("Retrieving tblSelectedSelectorBarItem");
            Edit tblSelectedSelectorBarItem = new Edit(FindElement.ById("tblSelectedSelectorBarItem"));
            Verify.IsNotNull(tblSelectedSelectorBarItem, "Verifying that IsSelectedTextBlock was found");

            string selectedItem = tblSelectedSelectorBarItem.GetText();
            Log.Comment("SelectedItem: " + selectedItem);
            return selectedItem;
        }
    }
}
