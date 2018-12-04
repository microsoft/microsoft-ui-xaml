// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS || USING_TESTNET
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class MenuBarTests
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
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    testFrameCheckbox.Uncheck();
                }

                var fileButton = FindElement.ById<Button>("FileItem");
                var editButton = FindElement.ById<Button>("EditItem");

                // click and click 
                // From bug 17343407: this test is sometimes unreliable, use retries and see if that helps.
                InputHelper.LeftClick(fileButton);
                TestEnvironment.VerifyAreEqualWithRetry(20,
                    () => FindCore.ByName("NewItem", shouldWait: false) != null, // The item should be in the tree
                    () => true);

                InputHelper.LeftClick(fileButton);
                VerifyElement.NotFound("NewItem", FindBy.Id);

                // overlay pass through element is only available from IFlyoutBase3 forward
                if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Controls.Primitives.IFlyoutBase3"))
                {
                    // click and hover
                    InputHelper.LeftClick(fileButton);
                    VerifyElement.NotFound("Undo", FindBy.Name);
                    InputHelper.MoveMouse(editButton, 0, 0);
                    InputHelper.MoveMouse(editButton, 1, 1);
                    VerifyElement.Found("Undo", FindBy.Name);
                }

                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    testFrameCheckbox.Check();
                }
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

                if (ApiInformation.IsTypePresent("Windows.UI.Xaml.IUIElement5")) // XYFocusNavigation is only availabe from IUElement5 foward
                {
                    KeyboardHelper.PressKey(Key.Right);
                    VerifyElement.NotFound("Undo", FindBy.Name);

                    KeyboardHelper.PressKey(Key.Enter);
                    VerifyElement.Found("Undo", FindBy.Name);
                }
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
                if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
                {
                    Log.Warning("Test is disabled on versions older that RS4");
                    return;
                }

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
                if (!ApiInformation.IsTypePresent("Windows.UI.Xaml.Input.KeyboardAccelerator"))
                {
                    Log.Warning("Test is disabled on versions that not support KeyboardAccelerator");
                    return;
                }

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
                Verify.AreEqual(menuBarItem.BoundingRectangle.Height, 24);
            }
        }
    }
}
