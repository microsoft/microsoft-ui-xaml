// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
﻿using System;

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
    public class RadioMenuFlyoutItemTests
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

        List<string> Items = new List<string>
        {
            "Red",
            "Orange",
            "Yellow",
            "Green",
            "Blue",
            "Indigo",
            "Violet",
            "Compact",
            "Normal",
            "Expanded"
        };

        [TestMethod]
        public void BasicTest()
        {
            using (var setup = new TestSetupHelper("RadioMenuFlyoutItem Tests"))
            {
                Log.Comment("Verify initial states");
                VerifySelectedItems("Orange", "Compact", "Name");

                InvokeItem("FlyoutButton", "YellowItem");
                VerifySelectedItems("Yellow", "Compact", "Name");

                InvokeItem("FlyoutButton", "ExpandedItem");
                VerifySelectedItems("Yellow", "Expanded", "Name");

                Log.Comment("Verify you can't uncheck an item");
                InvokeItem("FlyoutButton", "YellowItem");
                VerifySelectedItems("Yellow", "Expanded", "Name");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 36060166: Re-enable after fixing unreliability.
        public void SubMenuTest()
        {
            using (var setup = new TestSetupHelper("RadioMenuFlyoutItem Tests"))
            {
                InvokeSubItem("SubMenuFlyoutButton", "RadioSubMenu", "ArtistNameItem");
                VerifySelectedItems("Orange", "Compact", "ArtistName");

                InvokeItem("SubMenuFlyoutButton", "DateItem");
                VerifySelectedItems("Orange", "Compact", "Date");
            }
        }

        public void InvokeItem(string flyoutButtonName, string itemName)
        {
            Log.Comment("Open flyout by clicking " + flyoutButtonName);
            Button flyoutButton = FindElement.ByName<Button>(flyoutButtonName);
            flyoutButton.InvokeAndWait();

            Log.Comment("Toggle item: " + itemName);
            ToggleButton toggleMenuItem = FindElement.ByName<ToggleButton>(itemName);
            toggleMenuItem.ToggleAndWait();
        }

        public void InvokeSubItem(string flyoutButtonName, string menuName, string itemName)
        {
            Log.Comment("Open flyout by clicking " + flyoutButtonName);
            Button flyoutButton = FindElement.ByName<Button>(flyoutButtonName);
            flyoutButton.InvokeAndWait();

            Log.Comment("Open Menu: " + menuName);
            Button menuItem = FindElement.ByName<Button>(menuName);
            menuItem.InvokeAndWait();

            Log.Comment("Toggle item: " + itemName);
            ToggleButton toggleMenuItem = FindElement.ByName<ToggleButton>(itemName);
            toggleMenuItem.ToggleAndWait();
        }

        public void VerifySelectedItems(string item1, string item2, string item3)
        {
            foreach (string item in Items)
            {
                TextBlock itemState = FindElement.ByName<TextBlock>(item + "State");

                if (item == item1 || item == item2 || item == item3)
                {
                    Verify.AreEqual(itemState.DocumentText, "Checked", "Verify " + item + " is checked");
                }
                else
                {
                    Verify.AreEqual(itemState.DocumentText, "Unchecked", "Verify " + item + " is unchecked");
                }
            }
        }
    }
}
