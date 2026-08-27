// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RadioMenuFlyoutItemTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyItemsInTheSameGroupUncheckEachOther()
        {
            RunOnUIThread.Execute(() =>
            {
                var first = new RadioMenuFlyoutItem() { GroupName = "VerifySameGroup" };
                var second = new RadioMenuFlyoutItem() { GroupName = "VerifySameGroup" };

                first.IsChecked = true;
                second.IsChecked = true;

                Verify.IsFalse(first.IsChecked, "Checking the second item should uncheck the first one in the same group.");
                Verify.IsTrue(second.IsChecked, "The second item should be checked.");
            });
        }

        [TestMethod]
        [TestProperty("RegressionBug", "11098")]
        public void VerifyGroupNameSetAfterIsCheckedDoesNotLeakRegistration()
        {
            RunOnUIThread.Execute(() =>
            {
                // Mimics the XAML attribute order IsChecked="True" GroupName="...", where IsChecked is
                // applied while GroupName is still the default empty string.
                var itemInNamedGroup = new RadioMenuFlyoutItem();
                itemInNamedGroup.IsChecked = true;
                itemInNamedGroup.GroupName = "VerifyGroupNameSetAfterIsChecked";

                // This item belongs to the default group, which is the group the item above was
                // momentarily registered under.
                var itemInDefaultGroup = new RadioMenuFlyoutItem();
                itemInDefaultGroup.IsChecked = true;

                Verify.IsTrue(itemInNamedGroup.IsChecked,
                    "An item that moved to another group should not be unchecked by an item in the default group.");
                Verify.IsTrue(itemInDefaultGroup.IsChecked, "The item in the default group should be checked.");
            });
        }

        [TestMethod]
        [TestProperty("RegressionBug", "11098")]
        public void VerifyChangingGroupNameWhileCheckedMovesRegistration()
        {
            RunOnUIThread.Execute(() =>
            {
                var movedItem = new RadioMenuFlyoutItem() { GroupName = "VerifyMoveGroupOne" };
                movedItem.IsChecked = true;

                movedItem.GroupName = "VerifyMoveGroupTwo";

                // The first group must no longer refer to the moved item.
                var itemInFirstGroup = new RadioMenuFlyoutItem() { GroupName = "VerifyMoveGroupOne" };
                itemInFirstGroup.IsChecked = true;

                Verify.IsTrue(movedItem.IsChecked,
                    "The moved item should not be unchecked by an item joining the group it left.");
                Verify.IsTrue(itemInFirstGroup.IsChecked, "The item in the first group should be checked.");

                // ...and the second group must now refer to it.
                var itemInSecondGroup = new RadioMenuFlyoutItem() { GroupName = "VerifyMoveGroupTwo" };
                itemInSecondGroup.IsChecked = true;

                Verify.IsFalse(movedItem.IsChecked,
                    "The moved item should be unchecked by an item joining the group it moved to.");
                Verify.IsTrue(itemInSecondGroup.IsChecked, "The item in the second group should be checked.");
            });
        }

        [TestMethod]
        [TestProperty("RegressionBug", "11098")]
        public void VerifyUnloadingAMovedItemKeepsAnotherItemsRegistration()
        {
            StackPanel panel = null;
            RadioMenuFlyoutItem movedItem = null;
            RadioMenuFlyoutItem groupOwner = null;

            RunOnUIThread.Execute(() =>
            {
                movedItem = new RadioMenuFlyoutItem() { GroupName = "VerifyUnloadGroupOne" };
                movedItem.IsChecked = true;

                panel = new StackPanel();
                panel.Children.Add(movedItem);

                Content = panel;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                movedItem.GroupName = "VerifyUnloadGroupTwo";

                groupOwner = new RadioMenuFlyoutItem() { GroupName = "VerifyUnloadGroupTwo" };
                groupOwner.IsChecked = true;

                Verify.IsFalse(movedItem.IsChecked, "The moved item should have been unchecked by the new group owner.");

                // Unloading the moved item must not drop the registration that now belongs to groupOwner.
                panel.Children.Remove(movedItem);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var newItem = new RadioMenuFlyoutItem() { GroupName = "VerifyUnloadGroupTwo" };
                newItem.IsChecked = true;

                Verify.IsFalse(groupOwner.IsChecked,
                    "The group owner should still have been registered for its group, and so be unchecked by the new item.");
                Verify.IsTrue(newItem.IsChecked, "The new item should be checked.");
            });
        }
    }
}
