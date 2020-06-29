// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

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
    public class TreeViewTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("TestSuite", "A")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        private bool IsPhoneDevice()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is disabled on phone (bug #11244792/11318650).");
                return true;
            }

            return false;
        }

        private bool IsLowerThanRS5()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone4))
            {
                return false;
            }

            Log.Warning("xBind/content mode on TreeView only works for RS5 and up");
            return true;
        }

        private bool IsLowerThanRS2()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Drag and drop/game pad not supported on RS1");
                return true;
            }

            return false;
        }

        private void SetContentMode(bool isContentMode)
        {
            if(isContentMode)
            {
                ClickButton("SetContentMode");
            }
        }

        private void ExpandCollapseTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() || 
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject itemRoot = LabelFirstItem();

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(itemRoot);

                // Should be expanded now
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("AddSecondLevelOfNodes");
                ClickButton("LabelItems");

                var root0 = FindElement.ById("Root.0");
                InputHelper.Tap(root0);
                ClickButton("GetItemCount");
                Verify.AreEqual("5", ReadResult());

                InputHelper.Tap(itemRoot);
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(itemRoot);
                ClickButton("GetItemCount");
                Verify.AreEqual("5", ReadResult());

                var root1 = FindElement.ById("Root.1");
                InputHelper.Tap(root1);
                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());

                // Collapse and expand
                InputHelper.Tap(itemRoot);
                InputHelper.Tap(itemRoot);
                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseTest_NodeMode()
        {
           ExpandCollapseTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseTest_ContentMode()
        {
           ExpandCollapseTest(isContentMode:true);
        }

        private void ExpandCollapseViaAutomationTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject itemRoot = LabelFirstItem();

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                Log.Comment("Expand using UIA ExpandCollapse pattern");
                var expandCollapseImplementation = new ExpandCollapseImplementation(itemRoot);
                var expandWaiter = expandCollapseImplementation.GetExpandedWaiter();
                expandCollapseImplementation.Expand();
                expandWaiter.Wait(2000);

                // Should be expanded now
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                Log.Comment("Collapse using UIA ExpandCollapse pattern");
                var collapseWaiter = expandCollapseImplementation.GetCollapsedWaiter();
                expandCollapseImplementation.Collapse();
                collapseWaiter.Wait(2000);

                // Should be collapsed now
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                Log.Comment("Expand using UIA Invoke Pattern");
                var invokeImplementation = new InvokeImplementation(itemRoot);
                expandWaiter.Reset();
                invokeImplementation.Invoke();
                expandWaiter.Wait(2000);

                // Should be expanded now
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                Log.Comment("Collapse using UIA Invoke Pattern");
                collapseWaiter.Reset();
                invokeImplementation.Invoke();
                collapseWaiter.Wait(2000);

                // Should be collapsed now
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseViaAutomationTest_NodeMode()
        {
            ExpandCollapseViaAutomationTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseViaAutomationTest_ContentMode()
        {
            ExpandCollapseViaAutomationTest(isContentMode:true);
        }

        private void TreeViewItemClickTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject ItemRoot = LabelFirstItem();

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                // Should be expanded now
                Verify.AreEqual("ItemClicked:Root", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemClickTest_NodeMode()
        {
            TreeViewItemClickTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemClickTest_ContentMode()
        {
            TreeViewItemClickTest(isContentMode:true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void FlyoutTreeViewItemClickTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone (bug #11244792).");
                    return;
                }

                // click button to popup flyout treeview
                ClickButton("TreeViewInFlyout");

                // expand tree
                TapOnFlyoutTreeViewRootItemChevron();

                ClickButton("GetFlyoutItemCount");
                Verify.AreEqual("4", ReadResult());

                // click button to hide flyout treeview
                ClickButton("TreeViewInFlyout");

                // click button to popup flyout treeview again
                ClickButton("TreeViewInFlyout");

                //Expand once more
                TapOnFlyoutTreeViewRootItemChevron();
                ClickButton("GetFlyoutItemCount");
                Verify.AreEqual("4", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        // Regression test for bug 15801893
        public void FlyoutTreeViewItemTabTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone (bug #11244792).");
                    return;
                }

                // click button to popup flyout treeview
                ClickButton("TreeViewInFlyout");

                // Give a little bit of time for the
                // flyout to show up.
                Wait.ForIdle();

                // expand tree
                TapOnFlyoutTreeViewRootItemChevron();

                // collapse and dismiss
                KeyboardHelper.PressKey(Key.Left);
                KeyboardHelper.PressKey(Key.Escape);

                // click button to popup flyout treeview
                ClickButton("TreeViewInFlyout");

                // Give a little bit of time for the
                // flyout to show up.
                Wait.ForIdle();

                // expand tree
                TapOnFlyoutTreeViewRootItemChevron();

                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);

                // Verify that app did not crash.
                ClickButton("GetFlyoutItemCount");
                Verify.AreEqual("4", ReadResult());
            }
        }

        private void TreeViewKeyDownLeftToRightTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject ItemRoot = LabelFirstItem();

                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                Verify.AreEqual("ItemClicked:Root", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Right);

                ClickButton("GetItemCount");
                Log.Comment("Verify that ItemRoot has expanded.");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Enter);
                Log.Comment("Verify that we are on first child.");
                Verify.AreEqual("ItemClicked:Root.0", ReadResult());

                KeyboardHelper.PressKey(Key.Left);
                KeyboardHelper.PressKey(Key.Enter);
                Log.Comment("Verify that we are on Root.");
                Verify.AreEqual("ItemClicked:Root", ReadResult());

                KeyboardHelper.PressKey(Key.Left);

                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                // Validate Gamepad left/right dpad
                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.DPadRight);
                Log.Comment("Verify that ItemRoot has expanded.");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.DPadLeft);
                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                // Validate Gamepad left/right on left thumbstick
                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.LeftThumbstickRight);
                Log.Comment("Verify that ItemRoot has expanded.");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.LeftThumbstickLeft);
                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyDownLeftToRightTest_NodeMode()
        {
            TreeViewKeyDownLeftToRightTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyDownLeftToRightTest_ContentMode()
        {
            TreeViewKeyDownLeftToRightTest(isContentMode: true);
        }

        private void TreeViewKeyDownRightToLeftTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("ChangeFlowDirection");
                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                Verify.AreEqual("ItemClicked:Root", ReadResult());
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Left);
                ClickButton("GetItemCount");
                Log.Comment("Verify that ItemRoot has expanded.");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Left);
                KeyboardHelper.PressKey(Key.Enter);
                Log.Comment("Verify that we are on first child.");
                Verify.AreEqual("ItemClicked:Root.0", ReadResult());

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Enter);
                Log.Comment("Verify that we are on Root.");
                Verify.AreEqual("ItemClicked:Root", ReadResult());

                KeyboardHelper.PressKey(Key.Right);

                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                // Validate Gamepad left/right
                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.DPadLeft);
                Log.Comment("Verify that ItemRoot has expanded.");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.DPadRight);
                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                // Validate Gamepad left/right on left thumbstick
                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.LeftThumbstickLeft);
                Log.Comment("Verify that ItemRoot has expanded.");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ItemRoot.SetFocus();
                GamepadHelper.PressButton(ItemRoot, GamepadButton.LeftThumbstickRight);
                Log.Comment("Verify that ItemRoot has collapsed.");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewKeyDownRightToLeftTest_NodeMode()
        {
            TreeViewKeyDownRightToLeftTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewKeyDownRightToLeftTest_ContentMode()
        {
            TreeViewKeyDownRightToLeftTest(isContentMode:true);
        }

        private void TreeViewSwappingNodesTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                // Should be expanded now
                Verify.AreEqual("ItemClicked:Root", ReadResult());

                ClickButton("MoveNodesToNewTreeView");

                Wait.ForIdle();
                ClickButton("GetTree2ItemCount");
                Verify.AreEqual("6", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewSwappingNodesTest_NodeMode()
        {
            TreeViewSwappingNodesTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewSwappingNodesTest_ContentMode()
        {
            TreeViewSwappingNodesTest(isContentMode:true);
        }

        private void TreeViewExpandingEventTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("SetUpExpandingNodeEvent");

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                // Should be expanded now
                Verify.AreEqual("ItemClicked:Root", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("5", ReadResult());

                ClickButton("DisableClickToExpand");

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Right);

                Verify.AreEqual("Loaded", ReadResult());
                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewExpandingEventTest_NodeMode()
        {
            TreeViewExpandingEventTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewExpandingEventTest_ContentMode()
        {
            TreeViewExpandingEventTest(isContentMode:true);
        }

        private void TreeViewKeyboardReorderTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                Verify.AreEqual("ItemClicked:Root", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Enter);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyboardReorderTest_NodeMode()
        {
            TreeViewKeyboardReorderTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyboardReorderTest_ContentMode()
        {
            TreeViewKeyboardReorderTest(isContentMode:true);
        }

        private void TreeViewDragAndDropOnNode(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying that we found a UIElement called Root.0");

                UIObject dropUIObject = FindElement.ById("Root.1");
                Verify.IsNotNull(dropUIObject, "Verifying that we found a UIElement called Root.1");

                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                ClickButton("GetItemCount");
                Verify.AreEqual("3", ReadResult());

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragAndDropOnNode_NodeMode()
        {
            TreeViewDragAndDropOnNode();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragAndDropOnNode_ContentMode()
        {
            TreeViewDragAndDropOnNode(isContentMode:true);
        }

        private void TreeViewDragAndDropBetweenNodes(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.0");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Click on the item to help make the drag more reliable");
                dragUIObject.Click();

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                TestEnvironment.VerifyAreEqualWithRetry(
                    5,
                    () => "Root | Root.1 | Root.0 | Root.2",
                    () => {
                        ClickButton("GetChildrenOrder");
                        return ReadResult();
                    });

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetItemCommonStates");
                Verify.AreEqual("Normal Normal Normal Normal", ReadResult().Trim());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewDragAndDropBetweenNodes();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDensityChange()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                UIObject root = LabelFirstItem();
                int height = root.BoundingRectangle.Height;
                Verify.AreEqual(height, 32);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragAndDropBetweenNodes_ContentMode()
        {
            TreeViewDragAndDropBetweenNodes(isContentMode:true);
        }

        private void TreeViewCollectionChangesEffectSelectedNodesTest(bool isContentMode = false)
        {
            if(isContentMode && IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ToggleSelectionMode");

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1", ReadResult());

                ClickButton("AddSecondLevelOfNodes");

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.0.0, Root.1, Root.1.0, Root.1.1, Root.1.2", ReadResult());

                ClickButton("ModifySecondLevelOfNode");
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1, Root.1.0, Root.1.1, Root.1.2, THIS IS NEW", ReadResult());

                ClickButton("RemoveSecondLevelOfNode");
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1, Root.1.0, Root.1.1, Root.1.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewCollectionChangesEffectSelectedNodesTest_NodeMode()
        {
            TreeViewCollectionChangesEffectSelectedNodesTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewCollectionChangesEffectSelectedNodesTest_ContentMode()
        {
            TreeViewCollectionChangesEffectSelectedNodesTest(isContentMode:true);
        }

        private void TreeViewMultiSelectDragAndDropOnNode(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (isContentMode && IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ToggleSelectionMode");

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying that we found a UIElement called Root.0");

                UIObject dropUIObject = FindElement.ById("Root.2");
                Verify.IsNotNull(dropUIObject, "Verifying that we found a UIElement called Root.2");

                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                ClickButton("GetItemCount");
                Verify.AreEqual("2", ReadResult());

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.2 | Root.0 | Root.1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewMultiSelectDragAndDropOnNode_NodeMode()
        {
            TreeViewMultiSelectDragAndDropOnNode();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewMultiSelectDragAndDropOnNode_ContentMode()
        {
            TreeViewMultiSelectDragAndDropOnNode(isContentMode:true);
        }

        private void TreeViewMultiSelectDragAndDropBetweenNodes(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() ||
                (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ToggleSelectionMode");

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.1");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.2 | Root.0 | Root.1", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewMultiSelectDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewMultiSelectDragAndDropBetweenNodes();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewMultiSelectDragAndDropBetweenNodes_ContentMode()
        {
            TreeViewMultiSelectDragAndDropBetweenNodes(isContentMode:true);
        }

        private void ValidateCannotDragWithinSelectedSubtree(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
               (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ToggleSelectionMode");

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.1");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateCannotDragWithinSelectedSubtree_NodeMode()
        {
            ValidateCannotDragWithinSelectedSubtree();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateCannotDragWithinSelectedSubtree_ContentMode()
        {
            ValidateCannotDragWithinSelectedSubtree(isContentMode:true);
        }

        private void ValidateMultiSelectDragDropRootsAndNonRoots(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() ||
               (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ToggleSelectionMode");
                ClickButton("AddSecondLevelOfNodes");

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Space);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.0.0, Root.1.0", ReadResult());

                ClickButton("LabelItems");
                UIObject dragUIObject = FindElement.ById("Root.1.0");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.1.1 | Root.0 | Root.0.0 | Root.1.0 | Root.1.2 | Root.2", ReadResult());
                Wait.ForIdle();

                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.0.0, Root.1.0", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateMultiSelectDragDropRootsAndNonRoots_NodeMode()
        {
            ValidateMultiSelectDragDropRootsAndNonRoots();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateMultiSelectDragDropRootsAndNonRoots_ContentMode()
        {
            ValidateMultiSelectDragDropRootsAndNonRoots(isContentMode:true);
        }

        private void TreeViewDragCrash(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (isContentMode && IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("SizeTreeViewsForDrags");
                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.2");
                Verify.IsNotNull(dragUIObject);

                //Drag along the entire TreeView, to cover all possible DragOver scenarios
                var distance = 90;

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                Log.Comment("Congratulations, No Crash!");

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragCrash_NodeMode()
        {
            TreeViewDragCrash();
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragCrash_ContentMode()
        {
            TreeViewDragCrash(isContentMode:true);
        }

        private void DragItemOutOfTreeView(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() || IsLowerThanRS2() ||
               (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("SetupDragToDropTarget");
                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.2");
                Verify.IsNotNull(dragUIObject);

                UIObject dropTarget = FindElement.ByName("DropTarget");
                Verify.IsNotNull(dropTarget);

                Log.Comment($"Dragging from {dragUIObject} to {dropTarget}");
                InputHelper.DragToTarget(dragUIObject, dropTarget);

                TestEnvironment.VerifyAreEqualWithRetry(
                    5,
                    () => "Root.2",
                    () => new TextBlock(FindElement.ByName("DropTargetTextBlock")).DocumentText,
                    () => {
                        // The drag&drop operation may not have worked because the input is unreliable, just try again.
                        InputHelper.DragToTarget(dragUIObject, dropTarget);
                    });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void DragItemOutOfTreeView_NodeMode()
        {
            DragItemOutOfTreeView();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void DragItemOutOfTreeView_ContentMode()
        {
            DragItemOutOfTreeView(isContentMode:true);
        }

        private void DragItemOntoTreeView(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() || IsLowerThanRS2() ||
               (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("SetupDragToDropTarget");
                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                UIObject dragUIObject = FindElement.ByName("DraggableElement");
                Verify.IsNotNull(dragUIObject);

                UIObject dropTarget = isContentMode ? FindElement.ByName("ContentModeTestTreeView") : FindElement.ByName("TestTreeView");

                Verify.IsNotNull(dropTarget);

                InputHelper.DragToTarget(dragUIObject, dropTarget);

                Verify.AreEqual("Dropped: Test Item", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void DragItemOntoTreeView_NodeMode()
        {
            DragItemOntoTreeView();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void DragItemOntoTreeView_ContentMode()
        {
            DragItemOntoTreeView(isContentMode:true);
        }

        private void TreeViewNoReorderIntoChildren(bool isContentMode = false)
        {
            if(isContentMode && IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());
                ClickButton("SetupNoReorderNodes");

                UIObject lockedItem = FindElement.ById("Root.0");
                InputHelper.Tap(lockedItem);
                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("7", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.1");
                Verify.IsNotNull(dragUIObject);

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.North);

                ClickButton("GetItemCount");
                Verify.AreEqual("7", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNoReorderIntoChildren_NodeMode()
        {
            TreeViewNoReorderIntoChildren();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNoReorderIntoChildren_ContentMode()
        {
            TreeViewNoReorderIntoChildren(isContentMode:true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void FlyoutTreeViewSelectionChangedCrash()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone (bug #11244792).");
                    return;
                }
                // click button to popup flyout treeview
                ClickButton("TreeViewInFlyout");

                // expand tree
                TapOnFlyoutTreeViewRootItemChevron();

                ClickButton("GetFlyoutItemCount");
                Verify.AreEqual("4", ReadResult());

                // tap button to hide flyout treeview
                UIObject flyoutButton = FindElement.ByName("TreeViewInFlyout");
                Verify.IsNotNull(flyoutButton, "Verifying that we found the button");
                InputHelper.Tap(flyoutButton);
                Wait.ForIdle();

                //Click Button to test for crash
                ClickButton("ChangeSelectionAfterFlyout");

                Log.Comment("Congratulations, no crash!");
            }
        }

        private void TreeViewMultiSelectKeyboardingTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("ToggleSelectionMode");
                ClickButton("DisableClickToExpand");

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                KeyboardHelper.PressKey(Key.Space);
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.1, Root.2", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.1", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);
                KeyboardHelper.PressKey(Key.Left);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.1", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectKeyboardingTest_NodeMode()
        {
            TreeViewMultiSelectKeyboardingTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectKeyboardingTest_ContentMode()
        {
            TreeViewMultiSelectKeyboardingTest(isContentMode:true);
        }

        private void TreeViewMultiSelectItemTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("ToggleSelectionMode");
                ClickButton("LabelItems");

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());
                ClickButton("LabelItems");

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .05, ItemRoot.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());

                Log.Comment("Retrieve first item as generic UIElement");
                UIObject Item0 = FindElement.ById("Root.0");
                Verify.IsNotNull(ItemRoot, "Verifying that we found a UIElement called Root.0");

                InputHelper.Tap(Item0, Item0.BoundingRectangle.Width * .1, Item0.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.1, Root.2", ReadResult());

                // tap on partial selected node will set it to unselected state
                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .05, ItemRoot.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .05, ItemRoot.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectItemTest_NodeMode()
        {
            TreeViewMultiSelectItemTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectItemTest_ContentMode()
        {
            TreeViewMultiSelectItemTest(isContentMode:true);
        }

        private void TreeViewMultiSelectGamepadTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() || IsLowerThanRS2() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("ToggleSelectionMode");
                UIObject treeRoot = LabelFirstItem();

                // Tree is collapsed, and nothing is selected
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                // First A expands the tree
                VerifyGamepadAPress(treeRoot, "4", "Nothing selected");

                // Second A selects root and all children
                VerifyGamepadAPress(treeRoot, "4", "Selected: Root, Root.0, Root.1, Root.2");

                // Third A unselects
                VerifyGamepadAPress(treeRoot, "4", "Nothing selected");

                // Fourth A collapses
                VerifyGamepadAPress(treeRoot, "1", "Nothing selected");

                // Fifth A selects 
                VerifyGamepadAPress(treeRoot, "1", "Selected: Root, Root.0, Root.1, Root.2");

                // Sixth A unselects 
                VerifyGamepadAPress(treeRoot, "1", "Nothing selected");

                // Seventh A expands again
                VerifyGamepadAPress(treeRoot, "4", "Nothing selected");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewMultiSelectGamepadTest_NodeMode()
        {
            TreeViewMultiSelectGamepadTest();
        }

        //Test failures with keyboard/gamepad/mousewheel input #269
        //[TestMethod]
        //[TestProperty("TestSuite", "B")]
        public void TreeViewMultiSelectGamepadTest_ContentMode()
        {
            TreeViewMultiSelectGamepadTest(isContentMode:true);
        }

        private void TreeViewItemUIATest(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");

                Log.Comment("Retrieve first item as generic UIElement");
                UIObject Item1 = FindElement.ById("Root.1");
                Verify.IsNotNull(Item1, "Verifying that we found a UIElement called Root.1");

                Item1.SetFocus();
                AutomationElement itemPeer = AutomationElement.FocusedElement;

                // object valuePatternAsObject;
                int positionInSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);
                int level = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.LevelProperty);

                Verify.AreEqual(2, positionInSet, "Position in set.");
                Verify.AreEqual(3, sizeOfSet, "Size of set.");
                Verify.AreEqual(2, level, "Level in tree.");

                var expandCollapsePattern = (ExpandCollapsePattern)itemPeer.GetCurrentPattern(ExpandCollapsePattern.Pattern);
                Verify.AreEqual(ExpandCollapseState.LeafNode, expandCollapsePattern.Current.ExpandCollapseState);

                ItemRoot.SetFocus();
                AutomationElement rootPeer = AutomationElement.FocusedElement;
                expandCollapsePattern = (ExpandCollapsePattern)rootPeer.GetCurrentPattern(ExpandCollapsePattern.Pattern);
                Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapsePattern.Current.ExpandCollapseState);

                // Select root and check selection pattern.
                InputHelper.Tap(ItemRoot);
                var rootSelectionItemPattern = (SelectionItemPattern)rootPeer.GetCurrentPattern(SelectionItemPattern.Pattern);
                Verify.AreEqual(true, rootSelectionItemPattern.Current.IsSelected);

                // Expand and look at IsSelected for root and first child. Root should still be selected and first child should not.
                InputHelper.Tap(ItemRoot);
                ClickButton("LabelItems");
                UIObject item1 = FindElement.ById("Root.1");
                Verify.IsNotNull(item1, "Verifying that we found a UIElement called Root.1");

                item1.SetFocus();
                AutomationElement item1Peer = AutomationElement.FocusedElement;
                var item1SelectionItemPattern = (SelectionItemPattern)item1Peer.GetCurrentPattern(SelectionItemPattern.Pattern);
                Verify.AreEqual(false, item1SelectionItemPattern.Current.IsSelected);
                Verify.AreEqual(true, rootSelectionItemPattern.Current.IsSelected);

                // Select item1
                item1SelectionItemPattern.AddToSelection();
                Verify.AreEqual(true, item1SelectionItemPattern.Current.IsSelected);

                // Go to multi select mode
                ClickButton("ToggleSelectionMode");

                ClickButton("LabelItems");
                item1 = FindElement.ById("Root.1");
                Verify.IsNotNull(item1, "Verifying that we found a UIElement called Root.1");

                item1.SetFocus();
                KeyboardHelper.PressKey(Key.Space);
                item1Peer = AutomationElement.FocusedElement;
                item1SelectionItemPattern = (SelectionItemPattern)item1Peer.GetCurrentPattern(SelectionItemPattern.Pattern);
                Verify.AreEqual(true, item1SelectionItemPattern.Current.IsSelected);
                Verify.AreEqual(false, rootSelectionItemPattern.Current.IsSelected);

                // Deselect item1
                item1SelectionItemPattern.RemoveFromSelection();
                Verify.AreEqual(false, item1SelectionItemPattern.Current.IsSelected);

                // Select Root, which should select all its children as well.
                rootSelectionItemPattern.AddToSelection();
                Verify.AreEqual(true, rootSelectionItemPattern.Current.IsSelected);
                Verify.AreEqual(true, item1SelectionItemPattern.Current.IsSelected);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewListMultipleSelectionUIATest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(false);

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");
                ClickButton("ToggleSelectionMode");

                Log.Comment("Retrieve first item as generic UIElement");
                UIObject Item1 = FindElement.ById("Root.1");
                Verify.IsNotNull(Item1, "Verifying that we found a UIElement called Root.1");

                Item1.SetFocus();
                AutomationElement itemPeer = AutomationElement.FocusedElement;

                var selectionItemPeer = (SelectionItemPattern)itemPeer.GetCurrentPattern(SelectionItemPattern.Pattern);
                var treeViewListPeer = selectionItemPeer.Current.SelectionContainer;

                var multipleSelectionPattern = (SelectionPattern)treeViewListPeer.GetCurrentPattern(SelectionPattern.Pattern);
                Verify.IsNotNull(multipleSelectionPattern);
                Verify.IsTrue(multipleSelectionPattern.Current.CanSelectMultiple);
                Verify.IsFalse(multipleSelectionPattern.Current.IsSelectionRequired);
                var elements = multipleSelectionPattern.Current.GetSelection();
                Verify.IsNotNull(elements);
                Verify.AreEqual(0, elements.Length);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemUIATest_NodeMode()
        {
            TreeViewItemUIATest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemUIATest_ContentMode()
        {
            TreeViewItemUIATest(isContentMode:true);
        }

        private void ValidateExpandingRaisedOnItemHavingUnrealizedChildren(bool isContentMode = false)
        {
            if (IsPhoneDevice() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                var itemRoot = new TreeItem(LabelFirstItem());
                itemRoot.Expand();
                Wait.ForIdle();

                ClickButton("DisableClickToExpand");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("SetRoot1HasUnrealizedChildren");

                ClickButton("LabelItems");
                var root1 = new TreeItem(FindElement.ById("Root.1"));
                Verify.IsNotNull(root1, "Verifying root.1 is found");
                root1.Expand();
                Wait.ForIdle();

                Verify.AreEqual("Expanding Raised", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateExpandingRaisedOnItemHavingUnrealizedChildren_NodeMode()
        {
            ValidateExpandingRaisedOnItemHavingUnrealizedChildren();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateExpandingRaisedOnItemHavingUnrealizedChildren_ContentMode()
        {
            ValidateExpandingRaisedOnItemHavingUnrealizedChildren(isContentMode:true);
        }

        private void TreeViewDragItemTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() || IsLowerThanRS2() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("SetupDragDropHandlersForApiTest");
                UIObject dragUIObject = LabelFirstItem();
                InputHelper.DragDistance(dragUIObject, dragUIObject.BoundingRectangle.Height, Direction.South);
                Verify.AreEqual("DragItemsStarting:Root\nDragItemsCompleted:Root", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragItemTest_NodeMode()
        {
            TreeViewDragItemTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragItemTest_ContentMode()
        {
            TreeViewDragItemTest(isContentMode:true);
        }

        private void TreeViewDragMultipleItemsTest(bool isContentMode = false)
        {
            // InputHelper.DragToTarget() does not work properly on lower versions
            if (IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("ToggleSelectionMode");
                ClickButton("SetupDragDropHandlersForApiTest");

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("DisableClickToExpand");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0, Root.1", ReadResult());

                UIObject dragUIObject = FindElement.ByName("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying Root.0 is found");
                UIObject dropUIObject = FindElement.ByName("Root.2");
                Verify.IsNotNull(dropUIObject, "Verifying Root.2 is found");
                InputHelper.DragToTarget(dragUIObject, dropUIObject);
                Verify.AreEqual("DragItemsStarting:Root.0|Root.1\nDragItemsCompleted:Root.0|Root.1\nNewParent: Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragMultipleItemsTest_NodeMode()
        {
            TreeViewDragMultipleItemsTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragMultipleItemsTest_ContentMode()
        {
            TreeViewDragMultipleItemsTest(isContentMode:true);
        }

        private void TreeViewDragEnterOverLeaveTest(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() || IsLowerThanRS2() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("SetupDragDropHandlersForApiTest");

                UIObject dragUIObject = isContentMode ? FindElement.ByName("DraggableElement2") : FindElement.ByName("DraggableElement");
                Verify.IsNotNull(dragUIObject, "Verifying draggable element is found");

                UIObject dropUIObject = isContentMode ? FindElement.ByName("ContentModeTestTreeView") : FindElement.ByName("TestTreeView");
                Verify.IsNotNull(dropUIObject, "Verifying TreeView is found");

                // drag draggable into TestTreeView to test TestTreeView2's DragLeave event
                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                Verify.AreEqual("DragEnter->DragOver->DragLeave", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragEnterOverLeaveTest_NodeMode()
        {
            TreeViewDragEnterOverLeaveTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragEnterOverLeaveTest_ContentMode()
        {
            TreeViewDragEnterOverLeaveTest(isContentMode:true);
        }

        private void TreeViewDragEnterOverDropTest(bool isContentMode = false)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                // TODO 19727004: Re-enable this on versions below RS5 after fixing the bug where mouse click-and-drag doesn't work.
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            if (IsPhoneDevice() || IsLowerThanRS2() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("SetupDragDropHandlersForApiTest");

                UIObject dragUIObject = isContentMode ? FindElement.ByName("DraggableElement2") : FindElement.ByName("DraggableElement");
                Verify.IsNotNull(dragUIObject, "Verifying draggable element is found");

                UIObject dropUIObject = isContentMode ? FindElement.ByName("ContentModeTestTreeView2") : FindElement.ByName("TestTreeView2");
                Verify.IsNotNull(dropUIObject, "Verifying TestTreeView2 is found");

                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                Verify.AreEqual("DragEnter->DragOver->Drop", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragEnterOverDropTest_NodeMode()
        {
            TreeViewDragEnterOverDropTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragEnterOverDropTest_ContentMode()
        {
            TreeViewDragEnterOverDropTest(isContentMode:true);
        }

        private void TreeViewDisableItemDragTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() || IsLowerThanRS2() ||
              (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("SetupDragDropHandlersForApiTest");
                ClickButton("DisableItemDrag");

                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                ClickButton("LabelItems");
                UIObject dragUIObject = FindElement.ById("Root.1");
                Verify.IsNotNull(dragUIObject, "Verifying that we found UIElement root1");

                UIObject dropUIObject = FindElement.ById("Root.2");
                Verify.IsNotNull(dropUIObject, "Verifying that we found UIElement root2");

                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                // Verify drag item events (DragItemsStarting, DragItemsCompleted) didn't get called
                Verify.AreEqual("", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDisableItemDragTest_NodeMode()
        {
            TreeViewDisableItemDragTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDisableItemDragTest_ContentMode()
        {
            TreeViewDisableItemDragTest(isContentMode:true);
        }

        private void TreeViewDisableItemReorderTest(bool isContentMode = false)
        {
            if (IsPhoneDevice() || IsLowerThanRS2() ||
             (isContentMode && IsLowerThanRS5()))
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                UIObject ItemRoot = LabelFirstItem();

                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2", ReadResult());

                ClickButton("DisableItemReorder");

                UIObject dragUIObject = FindElement.ById("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying Root.0 is found");

                var height = dragUIObject.BoundingRectangle.Height;

                Log.Comment("Verify cannot drag between nodes");
                // Drag Root.0 to the middle of Root.1 and Root.2
                var distance = (int)(height * 1.5);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                // Verify nodes are still in the same order
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2", ReadResult());

                Log.Comment("Verify cannot drag onto a node");
                // Drag Root.0 onto Root.1
                var root1 = FindElement.ById("Root.1");
                InputHelper.DragToTarget(dragUIObject, root1);

                ClickButton("GetChildrenOrder");
                // Verify nodes are still in the same order
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDisableItemReorderTest_NodeMode()
        {
            TreeViewDisableItemReorderTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDisableItemReorderTest_ContentMode()
        {
            TreeViewDisableItemReorderTest(isContentMode:true);
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDisableItemReorderKeyboardTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2", ReadResult());

                ClickButton("DisableClickToExpand");

                var root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");

                // Verify we can reorder items using keyboard by default
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2", ReadResult());

                ClickButton("DisableItemReorder");

                // Verify reorder is disabled
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewItemTemplateSelectorTest()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "ItemTemplateSelectorTestPage" }))
            {
                Log.Comment("ItemTemplateSelector test page is ready");
                UIObject node1 = FindElement.ByName("Template1");
                Verify.IsNotNull(node1, "Verifying template 1 is set");
                UIObject node2 = FindElement.ByName("Template2");
                Verify.IsNotNull(node2, "Verifying template 2 is set");

                // Verify item container styles are set correctly by checking heights
                Verify.AreEqual(node1.BoundingRectangle.Height, 50);
                Verify.AreEqual(node2.BoundingRectangle.Height, 60);
            }
        }

        //Bug 16396926
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewExpandNodeWithUnrealizedChildrenCrash()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("AddNodeWithEmpyUnrealizedChildren");
                ClickButton("DisableClickToExpand");
                UIObject root = LabelFirstItem();
                InputHelper.Tap(root);

                // Expand root
                KeyboardHelper.PressKey(Key.Right);
                // Move down to root.3
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                // Expand root.3
                KeyboardHelper.PressKey(Key.Right);

                ClickButton("GetItemCount");
                Verify.AreEqual("5", ReadResult());
            }
        }

        private void TreeViewPartialSelectionTest(bool isContentMode = false)
        {
            if(isContentMode && IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(isContentMode);

                UIObject ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("AddSecondLevelOfNodes");
                // expand all nodes
                ClickButton("LabelItems");
                UIObject root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");
                UIObject root1 = FindElement.ById("Root.1");
                Verify.IsNotNull(root1, "Verifying Root.1 is found");
                InputHelper.Tap(root0);
                InputHelper.Tap(root1);
                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());
                ClickButton("LabelItems");

                ClickButton("DisableClickToExpand");
                ClickButton("ToggleSelectionMode");

                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|u|u|u|u|", ReadResult());

                UIObject root00 = FindElement.ById("Root.0.0");
                Verify.IsNotNull(root00, "Verifying Root.0.0 is found");
                ToggleTreeViewItemCheckBox(root00, 3);
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|s|u|u|u|u|u|", ReadResult());

                UIObject root10 = FindElement.ById("Root.1.0");
                Verify.IsNotNull(root10, "Verifying Root.1.0 is found");
                ToggleTreeViewItemCheckBox(root10, 3);
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|s|p|s|u|u|u|", ReadResult());

                ToggleTreeViewItemCheckBox(root1, 2);
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|s|u|u|u|u|u|", ReadResult());

                ToggleTreeViewItemCheckBox(root1, 2);
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|s|s|s|s|s|u|", ReadResult());

                UIObject root2 = FindElement.ById("Root.2");
                Verify.IsNotNull(root2, "Verifying Root.2 is found");
                ToggleTreeViewItemCheckBox(root2, 2);
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("s|s|s|s|s|s|s|s|", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewPartialSelectionTest_NodeMode()
        {
            TreeViewPartialSelectionTest();
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewPartialSelectionTest_ContentMode()
        {
            TreeViewPartialSelectionTest(isContentMode:true);
        }

        [TestMethod]
        [TestProperty("TreeViewTestSuite", "A")]
        public void TreeViewSelectedNodeTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                ClickButton("LabelItems");
                UIObject root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");

                InputHelper.Tap(root0);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0", ReadResult());

                ClickButton("ToggleRoot0Selection");

                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TreeViewTestSuite", "A")]
        public void TreeViewSelectedNodesTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                //SelectedNodes vector changes should be reflected on UI
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|u|s|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());

                // UI changes should update SelectedNodes vector
                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());

                ClickButton("LabelItems");
                UIObject root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TreeViewTestSuite", "B")]
        public void TreeViewSelectedItemTest()
        {
            // databinding is only available on RS5+
            if(IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                ClickButton("LabelItems");
                UIObject root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");

                InputHelper.Tap(root0);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0", ReadResult());

                ClickButton("ToggleRoot0Selection");

                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TreeViewTestSuite", "B")]
        public void TreeViewSelectedItemsTest()
        {
            // databinding is only available on RS5+
            if (IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                //SelectedItems vector changes should be reflected on UI
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|u|s|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());

                // UI changes should update SelectedItems vector
                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());

                ClickButton("LabelItems");
                UIObject root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.0", ReadResult());
            }
        }

        // test for #1756 https://github.com/microsoft/microsoft-ui-xaml/issues/1756
        [TestMethod]
        [TestProperty("TreeViewTestSuite", "B")]
        public void TreeViewSelectRegressionTest()
        {
            // Running 5 times since the the bug doesn't repro consistently.
            for(int i = 0; i < 5; i++)
            {
                using (var setup = new TestSetupHelper("TreeView Tests"))
                {
                    ClickButton("AddExtraNodes");
                    ClickButton("LabelItems");

                    ClickButton("SelectLastRootNode");
                    ClickButton("GetSelected");
                    Verify.AreEqual("Selected: Node 50", ReadResult());

                    UIObject node1 = FindElement.ByName("Node 1");
                    Verify.IsNotNull(node1, "Verifying Node 1 is found");
                    InputHelper.Tap(node1);

                    ClickButton("GetSelected");
                    Verify.AreEqual("Selected: Node 1", ReadResult());
                }
            }
        }

        // Regression test for bug 16833853
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNonexistNodeAutomationPeerTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("AddSecondLevelOfNodes");
                var root = LabelFirstItem();
                InputHelper.Tap(root);

                ClickButton("LabelItems");
                var root0 = FindElement.ById("Root.0");
                InputHelper.Tap(root0);

                ClickButton("LabelItems");
                UIObject root00 = FindElement.ById("Root.0.0");
                root00.SetFocus();

                AutomationElement itemPeer = AutomationElement.FocusedElement;
                int positionInSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                int sizeOfSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);
                int level = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.LevelProperty);
                Verify.AreEqual(1, positionInSet, "Position in set.");
                Verify.AreEqual(1, sizeOfSet, "Size of set.");
                Verify.AreEqual(3, level, "Level in tree.");

                ClickButton("RemoveSecondLevelOfNode");
                positionInSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.PositionInSetProperty);
                sizeOfSet = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.SizeOfSetProperty);
                level = (int)itemPeer.GetCurrentPropertyValue(AutomationElement.LevelProperty);
                Verify.AreEqual(-1, positionInSet, "Position in set.");
                Verify.AreEqual(-1, sizeOfSet, "Size of set.");
                Verify.AreEqual(-1, level, "Level in tree.");
            }
        }

        // Regression test for bug 17644036
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewItemAutomationPeerSelectionItemPatternTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("AddSecondLevelOfNodes");
                var root = LabelFirstItem();
                root.SetFocus();
                AutomationElement rootPeer = AutomationElement.FocusedElement;

                // single selection mode by default
                int index = Array.IndexOf(rootPeer.GetSupportedPatterns(), SelectionItemPatternIdentifiers.Pattern);
                Verify.IsGreaterThanOrEqual(index, 0);

                // click twice to switch to none selection mode
                ClickButton("ToggleSelectionMode");
                ClickButton("ToggleSelectionMode");

                index = Array.IndexOf(rootPeer.GetSupportedPatterns(), SelectionItemPatternIdentifiers.Pattern);
                Verify.IsLessThan(index, 0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewSelectionChangeUIAEventsTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("ToggleSelectionMode");
                var root = LabelFirstItem();
                root.SetFocus();
                AutomationElement rootPeer = AutomationElement.FocusedElement;

                var selectionItemPattern = rootPeer.GetCurrentPattern(SelectionItemPattern.Pattern) as SelectionItemPattern;
                Verify.AreEqual(false, selectionItemPattern.Current.IsSelected);

                using (var propertyChangedEventWaiter = new PropertyChangedEventWaiter(root, UIProperty.Get("SelectionItem.IsSelected")))
                {
                    KeyboardHelper.PressKey(Key.Space);
                    propertyChangedEventWaiter.Wait();
                    Log.Comment("SelectionItem.IsSelected property change event fired");
                    Verify.AreEqual(true, selectionItemPattern.Current.IsSelected);
                }

                using (var automationEventWaiter = new AutomationEventWaiter(SelectionItemPattern.ElementRemovedFromSelectionEvent, root, Scope.Element))
                {
                    KeyboardHelper.PressKey(Key.Space);
                    automationEventWaiter.Wait();
                    Log.Comment("SelectionItemPattern.ElementRemovedFromSelectionEvent fired");
                    Verify.AreEqual(false, selectionItemPattern.Current.IsSelected);
                }

                using (var automationEventWaiter = new AutomationEventWaiter(SelectionItemPattern.ElementAddedToSelectionEvent, root, Scope.Element))
                {
                    KeyboardHelper.PressKey(Key.Space);
                    automationEventWaiter.Wait();
                    Log.Comment("SelectionItemPattern.ElementAddedToSelectionEvent fired");
                    Verify.AreEqual(true, selectionItemPattern.Current.IsSelected);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNodeInheritenceTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var itemRoot = new TreeItem(LabelFirstItem());
                itemRoot.Expand();
                Wait.ForIdle();

                ClickButton("AddInheritedTreeViewNode");
                ClickButton("LabelItems");
                var node = FindElement.ById("Inherited from TreeViewNode");
                Verify.IsNotNull(node, "Verify TreeViewNode content is correct");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewFastReorderTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("AddExtraNodes");
                ClickButton("LabelItems");
                var node = FindElement.ById("Node 1");
                Verify.IsNotNull(node);
                node.SetFocus();
                Log.Comment("Move down node 1");
                var originalDelay = Keyboard.SendKeysDelay;
                Keyboard.SendKeysDelay = 10;

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift, 50);
                Log.Comment("Move up node 1");
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Alt | ModifierKey.Shift, 50);
                Log.Comment("App didn't crash");

                Keyboard.SendKeysDelay = originalDelay;
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDataLateInitTest()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewLateDataInitTestPage" }))
            {
                ClickButton("InitializeItemsSource");
                Wait.ForIdle();
                UIObject node1 = FindElement.ByName("Root");
                Verify.IsNotNull(node1, "Verify data binding");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewNodeInMarkupTest()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewNodeInMarkupTestPage" }))
            {
                UIObject root = FindElement.ByName("Root");
                Verify.IsNotNull(root, "Verify root node content");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewRootNodeBindingTest()
        {
            // TreeView databinding only works on RS5+
            if(IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                ClickButton("ClearNodes");
                ClickButton("AddRootNode");
                ClickButton("AddRootNode");
                ClickButton("GetItemCount");
                Verify.AreEqual("2", ReadResult());

                ClickButton("LabelItems");
                var root0 = FindElement.ById("Root0");
                var root1 = FindElement.ById("Root1");

                Log.Comment("Drag Root1 onto Root0...");
                InputHelper.DragToTarget(root1, root0);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                var rootTreeItem = new TreeItem(root0);
                rootTreeItem.Expand();
                Wait.ForIdle();
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root0 | Root1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        // Regression test for https://github.com/microsoft/microsoft-ui-xaml/issues/1182
        public void TreeViewWithMultiLevelChildrenExpandCollapseTest()
        {
            // TreeView databinding only works on RS5+
            if (IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                ClickButton("AddSecondLevelOfNodes");
                ClickButton("LabelItems");

                var root1 = new TreeItem(FindElement.ByName("Root.1"));
                root1.Expand();
                Wait.ForIdle();

                Log.Comment("Drag Root.1.2 into Root.1.1");
                var root11 = new TreeItem(FindElement.ByName("Root.1.1"));
                var root12 = new TreeItem(FindElement.ByName("Root.1.2"));
                InputHelper.DragToTarget(root12, root11);

                var root0 = new TreeItem(FindElement.ByName("Root.0"));
                Log.Comment("Drag Root.1 into Root.0");
                InputHelper.DragToTarget(root1, root0);

                root0.Expand();
                Wait.ForIdle();
                root1.Expand();
                Wait.ForIdle();
                root11.Expand();
                Wait.ForIdle();

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.0.0 | Root.1 | Root.1.0 | Root.1.1 | Root.1.2 | Root.2", ReadResult());

                root0.Collapse();
                Wait.ForIdle();
                root0.Expand();
                Wait.ForIdle();

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.0.0 | Root.1 | Root.1.0 | Root.1.1 | Root.1.2 | Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        // Regression test for https://github.com/microsoft/microsoft-ui-xaml/issues/1790
        public void ItemsSourceResyncTest()
        {
            // TreeView databinding only works on RS5+
            if (IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("ResetItemsSource");
                Wait.ForIdle();
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("ResetItemsSourceAsync");
                Wait.ForIdle();
                ClickButton("GetItemCount");
                Verify.AreEqual("6", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ItemsSourceSwitchForthAndBackTest()
        {
            // TreeView databinding only works on RS5+
            if (IsLowerThanRS5())
            {
                return;
            }

            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                SetContentMode(true);

                ClickButton("SwapItemsSource");
                Wait.ForIdle();
                ClickButton("GetItemCount");
                Verify.AreEqual("2", ReadResult());

                ClickButton("SwapItemsSource");
                Wait.ForIdle();
                ClickButton("ExpandRootNode");
                Wait.ForIdle();
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite","C")]
        public void SelectedItemBindingsWork()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var setContentButton = new Button(FindElement.ByName("TwoWayBoundButton"));
                var setSelectedItemButton = new Button(FindElement.ByName("SelectRoot2Item"));
                var readResultButton = new Button(FindElement.ByName("ReadBindingResult"));

                setContentButton.Click();
                Wait.ForIdle();

                readResultButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("Root.1;Root.1",ReadResult());

                setSelectedItemButton.Click();
                Wait.ForIdle();
                readResultButton.Click();
                Wait.ForIdle();
                Verify.AreEqual("Root.2;Root.2", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SingleSelectWithUnrealizedChildrenDoesNotMoveSelection()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewUnrealizedChildrenTestPage" }))
            {
                TapOnTreeViewAt(50, 12, "GetSelectedItemName");

                Log.Comment("Selecting item");
                ClickButton("GetSelectedItemName");
                Wait.ForIdle();

                Log.Comment("Verifying current selection");
                var textBlock = new TextBlock(FindElement.ByName("SelectedItemName"));
                Verify.AreEqual("Item: 0; layer: 3", textBlock.GetText());

                Log.Comment("Expanding selected item");
                TapOnTreeViewAt(12, 12, "GetSelectedItemName");
                Wait.ForIdle();

                Log.Comment("Verifying selection again");
                ClickButton("GetSelectedItemName");
                Wait.ForIdle();
                textBlock = new TextBlock(FindElement.ByName("SelectedItemName"));
                Verify.AreEqual("Item: 0; layer: 3", textBlock.GetText());
            }
        }

        private void ClickButton(string buttonName)
        {
            var button = new Button(FindElement.ByName(buttonName));
            button.InvokeAndWait();
            Wait.ForIdle();
        }

        private string ReadResult()
        {
            var results = new TextBlock(FindElement.ByName("Results"));
            return results.DocumentText;
        }

        private void TapOnTreeViewAt(double x, double y, string buttonName)
        {
            // Note: Unable to get the treeview UIObject. Using the button above and accounting
            // for its height as a workaround.
            UIObject buttonAboveTreeView = FindElement.ByName(buttonName);
            Verify.IsNotNull(buttonAboveTreeView, "Verifying that we found a UIElement called" + buttonName);

            InputHelper.Tap(buttonAboveTreeView, x, buttonAboveTreeView.BoundingRectangle.Height + y);
            Wait.ForIdle();
        }

        private void TapOnFlyoutTreeViewRootItemChevron()
        {
            // Chevron has 12px left padding, and it's 12px wide. 
            // 18 is the center point of chevron.
            TapOnTreeViewAt(18, 20, "GetFlyoutItemCount");
        }

        private UIObject LabelFirstItem()
        {
            ClickButton("LabelItems");
            Log.Comment("Retrieve first item as generic UIElement");
            UIObject ItemRoot = FindElement.ById("Root");
            Verify.IsNotNull(ItemRoot, "Verifying that we found a UIElement called Root");
            return ItemRoot;
        }

        private void VerifyGamepadAPress(UIObject node, string expectedItemCount, string expectedSelection)
        {
            node.SetFocus();
            GamepadHelper.PressButton(node, GamepadButton.A);
            Wait.ForIdle();
            ClickButton("GetItemCount");
            Verify.AreEqual(expectedItemCount, ReadResult());
            ClickButton("GetSelected");
            Verify.AreEqual(expectedSelection, ReadResult());
        }

        private void ToggleTreeViewItemCheckBox(UIObject item, int level)
        {
            InputHelper.Tap(item, item.BoundingRectangle.Width * 0.05 * level, item.BoundingRectangle.Height * .5);
        }
    }
}
