// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using MUXTestInfra.Shared.Infra;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class MenuBarTests
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
        public void VerifyAxeScanPasses()
        {
            using (var setup = new TestSetupHelper("MenuBar-Axe"))
            {
                AxeTestHelper.TestForAxeIssues();
            }
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void BasicMouseInteractionTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests")) 
            {
                CheckBox testFrameCheckbox = new CheckBox(FindElement.ByName("TestFrameCheckbox"));

                var fileButton = FindElement.ById<Button>("FileItem");
                var editButton = FindElement.ById<Button>("EditItem");

                // click and click 
                // From bug 17343407: this test is sometimes unreliable, use retries and see if that helps.
                InputHelper.LeftClick(fileButton);
                TestEnvironment.VerifyAreEqualWithRetry(20,
                    () => true,
                    () => FindCore.ByName("NewItem", shouldWait: false) != null); // The item should be in the tree

                InputHelper.LeftClick(fileButton);
                VerifyElement.NotFound("NewItem", FindBy.Id);

                // click and hover
                InputHelper.LeftClick(fileButton);
                VerifyElement.NotFound("Undo", FindBy.Name);
                InputHelper.MoveMouse(editButton, 0, 0);
                InputHelper.MoveMouse(editButton, 1, 1);
                VerifyElement.Found("Undo", FindBy.Name);

                testFrameCheckbox.Check();
            }
        }

        [TestMethod]
        public void AutomationPeerTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var fileButton = FindElement.ById<Button>("FileItem");
                var fileButtonEC = new ExpandCollapseImplementation(fileButton);

                // Invoke
                fileButton.Invoke();
                Wait.ForIdle();
                VerifyElement.Found("New", FindBy.Name);
                fileButton.Invoke();
                Wait.ForIdle();
                VerifyElement.NotFound("New", FindBy.Name);

                // Expand collapse
                fileButtonEC.Expand();
                Wait.ForIdle();
                VerifyElement.Found("New", FindBy.Name);
                fileButtonEC.Collapse();
                Wait.ForIdle();
                VerifyElement.NotFound("New", FindBy.Name);

                // Verify GetNameCore() is working if AutomationProperties.Name isn't set
                VerifyElement.Found("Format", FindBy.Name);
            }
        }

        [TestMethod]
        public void KeyboardNavigationWithArrowKeysTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests")) 
            {
                var editButton = FindElement.ById<Button>("EditItem");
                editButton.Invoke();
                VerifyElement.Found("Undo", FindBy.Name);

                KeyboardHelper.PressKey(Key.Left);
                VerifyElement.Found("New", FindBy.Name);

                KeyboardHelper.PressKey(Key.Escape);
                VerifyElement.NotFound("New", FindBy.Name);

                KeyboardHelper.PressKey(Key.Left);
                VerifyElement.NotFound("Word Wrap", FindBy.Name);

                KeyboardHelper.PressKey(Key.Enter);
                VerifyElement.Found("Word Wrap", FindBy.Name);

                KeyboardHelper.PressKey(Key.Escape);
                KeyboardHelper.PressKey(Key.Right);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.NotFound("Undo", FindBy.Name);

                KeyboardHelper.PressKey(Key.Enter);
                VerifyElement.Found("Undo", FindBy.Name);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                VerifyElement.NotFound("Item 1", FindBy.Name);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.Found("Item 1", FindBy.Name);
            }
        }

        [TestMethod]
        public void KeyboardNavigationWithArrowKeysWithDisabledItemTest()
        {
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var editButton = FindElement.ById<Button>("MenuBarPartiallyEnabledOne");
                editButton.Invoke();
                VerifyElement.Found("PartiallyEnabledFlyoutOne", FindBy.Name);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.Found("PartiallyEnabledFlyoutThree", FindBy.Name);

                KeyboardHelper.PressKey(Key.Left);
                VerifyElement.Found("PartiallyEnabledFlyoutOne", FindBy.Name);

                KeyboardHelper.PressKey(Key.Left);
                VerifyElement.Found("PartiallyEnabledFlyoutThree", FindBy.Name);

                KeyboardHelper.PressKey(Key.Left);
                VerifyElement.Found("PartiallyEnabledFlyoutOne", FindBy.Name);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.Found("PartiallyEnabledFlyoutThree", FindBy.Name);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.Found("PartiallyEnabledFlyoutOne", FindBy.Name);
            }
        }

        [TestMethod]
        public void KeyboardNavigationWithArrowKeysWithOnlyOneItemWorks()
        {
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var editButton = FindElement.ById<Button>("LoopTestBarOne");
                editButton.Invoke();
                VerifyElement.Found("LoopTestItemOne", FindBy.Name);

                KeyboardHelper.PressKey(Key.Right);
                VerifyElement.NotFound("LoopTestItemOne", FindBy.Name);
            }
        }
        
        [TestMethod]
        public void KeyboardNavigationWithAccessKeysTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                KeyboardHelper.PressDownModifierKey(ModifierKey.Alt);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Alt);
                Wait.ForIdle();

                TextInput.SendText("A"); // this clicks File menu bar item
                Wait.ForIdle();
                TextInput.SendText("B"); // this clicks New flyout item
                Wait.ForIdle();

                VerifyElement.Found("New Clicked", FindBy.Name);
            }
        }

        [TestMethod]
        public void KeyboardAcceleratorsTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                VerifyElement.NotFound("Undo Clicked", FindBy.Name);

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                Log.Comment("Send text z.");
                TextInput.SendText("z");
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                
                Wait.ForIdle();

                VerifyElement.Found("Undo Clicked", FindBy.Name);
            }
        }

        [TestMethod]
        public void AddRemoveMenuBarItemTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var addButton = FindElement.ById<Button>("AddMenuBarItemButton");
                var removeButton = FindElement.ById<Button>("RemoveMenuBarItemButton");

                Log.Comment("Verify that menu bar items can be added");
                addButton.Invoke();
                VerifyElement.Found("New Menu Bar Item", FindBy.Name);

                Log.Comment("Verify that menu bar items can be removed");
                removeButton.Invoke();
                VerifyElement.NotFound("New Menu Bar Item", FindBy.Name);

                Log.Comment("Verify that menu bar pre-existing items can be removed");
                VerifyElement.Found("Format", FindBy.Name);
                removeButton.Invoke();
                VerifyElement.NotFound("Format", FindBy.Name);
            }
        }

        [TestMethod]
        public void AddRemoveMenuFlyoutItemTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var addButton = FindElement.ById<Button>("AddFlyoutItemButton");
                var removeButton = FindElement.ById<Button>("RemoveFlyoutItemButton");
                var fileButton = FindElement.ById<Button>("FileItem");

                Log.Comment("Verify that menu flyout items can be added");
                addButton.Invoke();
                Wait.ForIdle();

                // open flyout
                fileButton.Invoke(); 
                Wait.ForIdle();

                VerifyElement.Found("New Flyout Item", FindBy.Name);

                // close flyout
                fileButton.Invoke(); 
                Wait.ForIdle();

                Log.Comment("Verify that menu flyout items can be removed");
                removeButton.Invoke();
                Wait.ForIdle();

                // open flyout
                fileButton.Invoke();
                Wait.ForIdle();

                VerifyElement.NotFound("New Flyout Item", FindBy.Name);

                // close flyout
                fileButton.Invoke(); 
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void MenuBarHeightTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var menuBar = FindElement.ById("SizedMenuBar");
                var menuBarItem = FindElement.ById<Button>("SizedMenuBarItem");

                Log.Comment("Verify that the size of the MenuBar can be set.");

                Verify.AreEqual(menuBar.BoundingRectangle.Height, 24);
                Verify.AreEqual(menuBarItem.BoundingRectangle.Height, 16);
            }
        }

        [TestMethod]
        public void HoveringBehaviorTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var menuBar = FindElement.ById("SizedMenuBar");
                var addButton = FindElement.ByName("AddItemsToEmptyMenuBar");

                addButton.Click();
                addButton.Click();
                addButton.Click();

                var help0 = FindElement.ByName<Button>("Help0");
                var help1 = FindElement.ByName<Button>("Help1");

                // This behavior seems to a bit unreliable, so repeat
                InputHelper.LeftClick(help0);
                TestEnvironment.VerifyAreEqualWithRetry(20,
                    () => true,
                    () => FindCore.ByName("Add0", shouldWait: false) != null); // The item should be in the tree

                // Check if hovering over the next button actually will show the correct item
                VerifyElement.NotFound("Add1", FindBy.Name);
                InputHelper.MoveMouse(help1, 0, 0);
                InputHelper.MoveMouse(help1, 1, 1);
                InputHelper.MoveMouse(help1, 5, 5);

                UIObject add1Element = null;
                ElementCache.Clear();
                var element = GetElement(ref add1Element, "Add1");
                Verify.IsNotNull(add1Element);
            }
        }

        [TestMethod]
        public void TabTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                var firstButton = FindElement.ByName<Button>("FirstButton");
                var fileButton = FindElement.ById<Button>("FileItem");

                firstButton.SetFocus();
                Wait.ForIdle();

                Log.Comment("Verify that pressing tab from previous control goes to the File item");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                
                Verify.AreEqual(true, fileButton.HasKeyboardFocus);
            }
        }

        [TestMethod]
        public void EmptyMenuBarItemNoPopupTest()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Comment("Skipping tests on phone, because menubar is not supported.");
                return;
            }
            using (var setup = new TestSetupHelper("MenuBar Tests"))
            {
                FindElement.ByName<Button>("NoChildrenFlyout").Click();
                VerifyElement.NotFound("Popup",FindBy.Name);

                FindElement.ByName<Button>("OneChildrenFlyout").Click();
                VerifyElement.Found("Popup", FindBy.Name);

                // Click twice to close flyout
                FindElement.ByName<Button>("RemoveItemsFromOneChildrenItem").Click();
                FindElement.ByName<Button>("RemoveItemsFromOneChildrenItem").Click();

                FindElement.ByName<Button>("OneChildrenFlyout").Click();
                VerifyElement.NotFound("Popup", FindBy.Name);
            }
        }

        private T GetElement<T>(ref T element, string elementName) where T : UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);
                element = FindElement.ByNameOrId<T>(elementName);
                Verify.IsNotNull(element);
            }
            return element;
        }
    }
}
