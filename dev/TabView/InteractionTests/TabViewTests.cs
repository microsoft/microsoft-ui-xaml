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
    public class TabViewTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void SelectionTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Verify content is displayed for initially selected tab.");
                UIObject tabContent = FindElement.ByName("FirstTabContent");
                Verify.IsNotNull(tabContent);

                Log.Comment("Changing selection.");
                UIObject lastTab = FindElement.ByName("LastTab");
                lastTab.Click();
                Wait.ForIdle();

                Log.Comment("Verify content is displayed for newly selected tab.");
                tabContent = FindElement.ByName("LastTabContent");
                Verify.IsNotNull(tabContent);

                Log.Comment("Verify that setting SelectedItem changes selection.");
                Button selectItemButton = FindElement.ByName<Button>("SelectItemButton");
                selectItemButton.InvokeAndWait();

                TextBlock selectedIndexTextBlock = FindElement.ByName<TextBlock>("SelectedIndexTextBlock");
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "1");

                Log.Comment("Verify that setting SelectedIndex changes selection.");
                Button selectIndexButton = FindElement.ByName<Button>("SelectIndexButton");
                selectIndexButton.InvokeAndWait();
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "2");
            }
        }

        [TestMethod]
        public void AddRemoveTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Adding tab.");
                Button addTabButton = FindElement.ByName<Button>("Add New Tab");
                addTabButton.InvokeAndWait();

                ElementCache.Refresh();
                UIObject newTab = FindElement.ByName("New Tab 1");
                Verify.IsNotNull(newTab);

                Log.Comment("Removing tab.");
                Button removeTabButton = FindElement.ByName<Button>("RemoveTabButton");
                removeTabButton.InvokeAndWait();

                ElementCache.Refresh();
                newTab = FindElement.ByName("New Tab 1");
                Verify.IsNull(newTab);
            }
        }

        [TestMethod]
        public void TabSizeTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject smallerTab = FindElement.ByName("FirstTab");
                UIObject largerTab = FindElement.ByName("LongHeaderTab");

                Log.Comment("Equal size tabs should all be the same size.");
                Verify.AreEqual(smallerTab.BoundingRectangle.Width, largerTab.BoundingRectangle.Width);

                Log.Comment("Changing tab width mode to SizeToContent.");
                ComboBox tabWidthComboBox = FindElement.ByName<ComboBox>("TabWidthComboBox");
                tabWidthComboBox.SelectItemByName("SizeToContent");
                Wait.ForIdle();

                Log.Comment("Tab with larger content should be wider.");
                Verify.IsGreaterThan(largerTab.BoundingRectangle.Width, smallerTab.BoundingRectangle.Width);
            }
        }

        [TestMethod]
        public void CloseSelectionTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                TextBlock selectedIndexTextBlock = FindElement.ByName<TextBlock>("SelectedIndexTextBlock");
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "0");

                Log.Comment("When the selected tab is closed, selection should move to the next one.");
                closeButton.InvokeAndWait();
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "0");

                Log.Comment("Select last tab.");
                UIObject lastTab = FindElement.ByName("LastTab");
                lastTab.Click();
                Wait.ForIdle();
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "3");

                Log.Comment("When the selected tab is last and is closed, selection should move to the previous item.");
                closeButton = FindCloseButton(lastTab);
                Verify.IsNotNull(closeButton);
                closeButton.InvokeAndWait();
                Verify.AreEqual(selectedIndexTextBlock.DocumentText, "2");
            }
        }

        [TestMethod]
        public void IsCloseableTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                Log.Comment("Setting IsCloseable=false on the first tab.");
                CheckBox isCloseableCheckBox = FindElement.ByName<CheckBox>("IsCloseableCheckBox");
                isCloseableCheckBox.Uncheck();
                Wait.ForIdle();

                ElementCache.Refresh();
                closeButton = FindCloseButton(firstTab);
                Verify.IsNull(closeButton);

                Log.Comment("Setting IsCloseable=true on the first tab.");
                isCloseableCheckBox.Check();
                Wait.ForIdle();

                ElementCache.Refresh();
                closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                Log.Comment("Setting CanCloseTabs=false on the TabView.");
                CheckBox canCloseCheckBox = FindElement.ByName<CheckBox>("CanCloseCheckBox");
                canCloseCheckBox.Uncheck();
                Wait.ForIdle();

                ElementCache.Refresh();
                closeButton = FindCloseButton(firstTab);
                Verify.IsNull(closeButton);
            }
        }

        [TestMethod]
        public void CancelTabClosingTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                UIObject firstTab = FindElement.ByName("FirstTab");
                Button closeButton = FindCloseButton(firstTab);
                Verify.IsNotNull(closeButton);

                CheckBox cancelCloseCheckBox = FindElement.ByName<CheckBox>("CancelCloseCheckBox");
                cancelCloseCheckBox.Check();
                Wait.ForIdle();

                Log.Comment("Clicking close button should not close tab if app returns cancel = true.");
                closeButton.InvokeAndWait();

                ElementCache.Refresh();
                firstTab = TryFindElement.ByName("FirstTab");
                Verify.IsNotNull(firstTab);

                cancelCloseCheckBox.Uncheck();
                Wait.ForIdle();

                Log.Comment("Clicking close button should close tab if app doesn't handle TabClosing event.");
                closeButton.InvokeAndWait();

                ElementCache.Refresh();
                firstTab = TryFindElement.ByName("FirstTab");
                Verify.IsNull(firstTab);
            }
        }

        [TestMethod]
        public void AddButtonTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Add new tab button should be visible.");
                var addButton = FindElement.ByName("Add New Tab");
                Verify.IsNotNull(addButton);

                CheckBox isAddButtonVisibleCheckBox = FindElement.ByName<CheckBox>("IsAddButtonVisibleCheckBox");
                isAddButtonVisibleCheckBox.Uncheck();
                Wait.ForIdle();

                ElementCache.Refresh();
                Log.Comment("Add new tab button should not be visible.");
                addButton = TryFindElement.ByName("Add New Tab");
                Verify.IsNull(addButton);
            }
        }

        [TestMethod]
        public void KeyboardTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ functionality (specifically, KeyboardAccelerators)");
                return;
            }

            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                Log.Comment("Set focus inside the TabView");
                UIObject tabContent = FindElement.ByName("FirstTabContent");
                tabContent.SetFocus();

                Log.Comment("Verify that pressing ctrl-f4 closes the tab");
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                TextInput.SendText("{F4}");
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                Wait.ForIdle();

                ElementCache.Refresh();
                UIObject firstTab = TryFindElement.ByName("FirstTab");
                Verify.IsNull(firstTab);
            }
        }

        [TestMethod]
        public void DragOutsideTest()
        {
            using (var setup = new TestSetupHelper("TabView Tests"))
            {
                TextBlock dragOutsideTextBlock = FindElement.ByName<TextBlock>("TabDraggedOutsideTextBlock");
                Verify.AreEqual(dragOutsideTextBlock.DocumentText, "");

                Log.Comment("Drag tab out");
                UIObject firstTab = TryFindElement.ByName("FirstTab");
                InputHelper.DragDistance(firstTab, 50, Direction.South);
                Wait.ForIdle();

                Log.Comment("Verify event fired");
                Verify.AreEqual(dragOutsideTextBlock.DocumentText, "Home");
            }
        }

        Button FindCloseButton(UIObject tabItem)
        {
            foreach (UIObject elem in tabItem.Children)
            {
                if (elem.ClassName.Equals("Button"))
                {
                    Log.Comment("Found close button for object " + tabItem.Name);
                    return new Button(elem);
                }
            }
            Log.Comment("Did not find close button for object " + tabItem.Name);
            return null;
        }
    }
}
