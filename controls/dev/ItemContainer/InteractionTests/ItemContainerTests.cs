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
using Windows.Media.Core;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ItemContainerTests
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

#if MUX_PRERELEASE
        [TestMethod]
        [TestProperty("Description", "Verify ItemInvoked via mouse")]
        [TestProperty("Ignore", "True")] // Bug 45606616
        public void ItemContainerSelectionMouse()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemContainer Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving RoutedEventsItemContainer");
                var routedEventsItemContainer = FindElement.ById("RoutedEventsItemContainer");
                Verify.IsNotNull(routedEventsItemContainer, "Verifying that RoutedEventsItemContainer was found");

                InputHelper.LeftClick(routedEventsItemContainer);

                Verify.AreEqual(IsItemContainerSelected(), "True");
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Bug 45606616
        [TestProperty("Description", "Verify ItemInvoked via tap")]
        public void ItemContainerSelectionTap()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemContainer Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving RoutedEventsItemContainer");
                var routedEventsItemContainer = FindElement.ById("RoutedEventsItemContainer");
                Verify.IsNotNull(routedEventsItemContainer, "Verifying that RoutedEventsItemContainer was found");

                InputHelper.Tap(routedEventsItemContainer);

                Verify.AreEqual(IsItemContainerSelected(), "True");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify ItemInvoked via Enter key")]
        [TestProperty("Ignore", "True")] // Bug 45606616
        public void ItemContainerSelectionEnterKey()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemContainer Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving RoutedEventsItemContainer");
                var routedEventsItemContainer = FindElement.ById("RoutedEventsItemContainer");
                Verify.IsNotNull(routedEventsItemContainer, "Verifying that RoutedEventsItemContainer was found");

                routedEventsItemContainer.SetFocus();

                KeyboardHelper.PressKey(Key.Enter);

                Verify.AreEqual(IsItemContainerSelected(), "True");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verify ItemInvoked via Space key")]
        [TestProperty("Ignore", "True")] // Bug 45606616
        public void ItemContainerSelectionSpaceKey()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemContainer Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving RoutedEventsItemContainer");
                var routedEventsItemContainer = FindElement.ById("RoutedEventsItemContainer");
                Verify.IsNotNull(routedEventsItemContainer, "Verifying that RoutedEventsItemContainer was found");

                routedEventsItemContainer.SetFocus();

                KeyboardHelper.PressKey(Key.Space);

                Verify.AreEqual(IsItemContainerSelected(), "True");
            }
        }
#endif

        [TestMethod]
        [TestProperty("Description", "Verify CanUserSelect == UserCannotSelect disabled selection")]
        public void ItemContainerCanUserSelect()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemContainer Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving RoutedEventsItemContainer");
                var routedEventsItemContainer = FindElement.ById("RoutedEventsItemContainer");
                Verify.IsNotNull(routedEventsItemContainer, "Verifying that RoutedEventsItemContainer was found");

                Log.Comment("Retrieving CanUserSelectComboBox");
                ComboBox canUserSelectComboBox = new ComboBox(FindElement.ById("CanUserSelectComboBox"));
                Verify.IsNotNull(canUserSelectComboBox, "Verifying that CanUserSelectComboBox was found");

                Log.Comment("Changing CanUserSelectComboBox selection to 'UserCannotSelect'");
                canUserSelectComboBox.SelectItemByName("UserCannotSelect");
                Log.Comment("Selection is now {0}", canUserSelectComboBox.Selection[0].Name);

                InputHelper.LeftClick(routedEventsItemContainer);

                Verify.AreEqual(IsItemContainerSelected(), "False");
            }
        }

        private string IsItemContainerSelected()
        {
            Log.Comment("Retrieving btnGetIsSelected");
            Button btnGetIsSelected = new Button(FindElement.ById("btnGetIsSelected"));
            Verify.IsNotNull(btnGetIsSelected, "Verifying that btnGetIsSelected was found");

            Log.Comment("Retrieving IsSelectedTextBlock");
            Edit isSelectedTextBlock = new Edit(FindElement.ById("IsSelectedTextBlock"));
            Verify.IsNotNull(isSelectedTextBlock, "Verifying that IsSelectedTextBlock was found");

            btnGetIsSelected.Invoke();
            Wait.ForIdle();

            return isSelectedTextBlock.GetText();
        }
    }
}
