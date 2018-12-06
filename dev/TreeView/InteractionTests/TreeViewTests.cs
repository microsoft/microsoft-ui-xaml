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

#if BUILD_WINDOWS
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
    public class TreeViewTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
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

                UIObject ItemRoot = LabelFirstItem();

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                InputHelper.Tap(ItemRoot);

                // Should be expanded now
                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                InputHelper.Tap(ItemRoot);

                // Should be collapsed now
                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void ExpandCollapseTest_NodeMode()
        {
           ExpandCollapseTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
        public void ExpandCollapseViaAutomationTest_NodeMode()
        {
            ExpandCollapseViaAutomationTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
        public void TreeViewItemClickTest_NodeMode()
        {
            TreeViewItemClickTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewItemClickTest_ContentMode()
        {
            TreeViewItemClickTest(isContentMode:true);
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
        public void TreeViewKeyDownLeftToRightTest_NodeMode()
        {
            TreeViewKeyDownLeftToRightTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        public void TreeViewKeyDownRightToLeftTest_NodeMode()
        {
            TreeViewKeyDownRightToLeftTest();
        }

        [TestMethod]
        public void TreeViewKeyDownRightToLeftTest_ContentMode()
        {
            TreeViewKeyDownRightToLeftTest(isContentMode:true);
        }

        private void TreeViewSelectedItemTest(bool isContentMode = false)
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

                ClickButton("GetSelected");
                Verify.AreEqual("ItemSelected:Root", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewSelectedItemTest_NodeMode()
        {
            TreeViewSelectedItemTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewSelectedItemTest_ContentMode()
        {
            TreeViewSelectedItemTest(isContentMode:true);
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
        [TestProperty("Platform", "Desktop")]
        public void TreeViewSwappingNodesTest_NodeMode()
        {
            TreeViewSwappingNodesTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
        public void TreeViewExpandingEventTest_NodeMode()
        {
            TreeViewExpandingEventTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0 | ", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0 | ", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0 | ", ReadResult());

                InputHelper.Tap(ItemRoot);

                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.2 | Root.0 | ", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewKeyboardReorderTest_NodeMode()
        {
            TreeViewKeyboardReorderTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2 | ", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewDragAndDropOnNode_NodeMode()
        {
            TreeViewDragAndDropOnNode();
        }

        [TestMethod]
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

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2 | ", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetItemCommonStates");
                Verify.AreEqual("Selected Selected Selected Selected", ReadResult().Trim());
            }
        }

        [TestMethod]
        public void TreeViewDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewDragAndDropBetweenNodes();
        }

        [TestMethod]
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
                Verify.AreEqual("Num. Selected: 0", ReadResult());

                InputHelper.Tap(ItemRoot);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                ClickButton("AddSecondLevelOfNodes");

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 6", ReadResult());

                ClickButton("ModifySecondLevelOfNode");
                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 6", ReadResult());

                ClickButton("RemoveSecondLevelOfNode");
                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 5", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewCollectionChangesEffectSelectedNodesTest_NodeMode()
        {
            TreeViewCollectionChangesEffectSelectedNodesTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying that we found a UIElement called Root.0");

                UIObject dropUIObject = FindElement.ById("Root.2");
                Verify.IsNotNull(dropUIObject, "Verifying that we found a UIElement called Root.2");

                InputHelper.DragToTarget(dragUIObject, dropUIObject);

                ClickButton("GetItemCount");
                Verify.AreEqual("2", ReadResult());

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.2 | Root.0 | Root.1 | ", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewMultiSelectDragAndDropOnNode_NodeMode()
        {
            TreeViewMultiSelectDragAndDropOnNode();
        }

        [TestMethod]
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
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.1");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.2 | Root.0 | Root.1 | ", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 2", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewMultiSelectDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewMultiSelectDragAndDropBetweenNodes();
        }

        [TestMethod]
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
                Verify.AreEqual("Num. Selected: 4", ReadResult());

                UIObject dragUIObject = FindElement.ById("Root.1");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2 | ", ReadResult());

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 4", ReadResult());
            }
        }

        [TestMethod]
        public void ValidateCannotDragWithinSelectedSubtree_NodeMode()
        {
            ValidateCannotDragWithinSelectedSubtree();
        }

        [TestMethod]
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
                Verify.AreEqual("Num. Selected: 3", ReadResult());

                ClickButton("LabelItems");
                UIObject dragUIObject = FindElement.ById("Root.1.0");

                var height = dragUIObject.BoundingRectangle.Height;
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.South);

                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.1.1 | Root.0 | Root.0.0 | Root.1.0 | Root.1.2 | Root.2 | ", ReadResult());
                Wait.ForIdle();

                ClickButton("GetItemCount");
                Verify.AreEqual("8", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 3", ReadResult());
            }
        }

        [TestMethod]
        public void ValidateMultiSelectDragDropRootsAndNonRoots_NodeMode()
        {
            ValidateMultiSelectDragDropRootsAndNonRoots();
        }

        [TestMethod]
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
        public void TreeViewDragCrash_NodeMode()
        {
            TreeViewDragCrash();
        }

        [TestMethod]
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

                InputHelper.DragToTarget(dragUIObject, dropTarget);

                var droppedItem = new TextBlock(FindElement.ByName("DropTargetTextBlock")).DocumentText;
                Verify.AreEqual("Root.2", droppedItem);
            }
        }

        [TestMethod]
        public void DragItemOutOfTreeView_NodeMode()
        {
            DragItemOutOfTreeView();
        }

        [TestMethod]
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
        public void DragItemOntoTreeView_NodeMode()
        {
            DragItemOntoTreeView();
        }

        [TestMethod]
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
        public void TreeViewNoReorderIntoChildren_NodeMode()
        {
            TreeViewNoReorderIntoChildren();
        }

        [TestMethod]
        public void TreeViewNoReorderIntoChildren_ContentMode()
        {
            TreeViewNoReorderIntoChildren(isContentMode:true);
        }

        [TestMethod]
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
                Verify.AreEqual("Num. Selected: 4", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Down);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetItemCount");
                Verify.AreEqual("4", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 1", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);
                KeyboardHelper.PressKey(Key.Left);

                ClickButton("GetItemCount");
                Verify.AreEqual("1", ReadResult());

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 1", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .75, ItemRoot.BoundingRectangle.Height * .5);
                KeyboardHelper.PressKey(Key.Space);

                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 4", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewMultiSelectKeyboardingTest_NodeMode()
        {
            TreeViewMultiSelectKeyboardingTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                Verify.AreEqual("Num. Selected: 4", ReadResult());

                Log.Comment("Retrieve first item as generic UIElement");
                UIObject Item0 = FindElement.ById("Root.0");
                Verify.IsNotNull(ItemRoot, "Verifying that we found a UIElement called Root.0");

                InputHelper.Tap(Item0, Item0.BoundingRectangle.Width * .1, Item0.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                // tap on partial selected node will set it to unselected state
                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .05, ItemRoot.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 0", ReadResult());

                InputHelper.Tap(ItemRoot, ItemRoot.BoundingRectangle.Width * .05, ItemRoot.BoundingRectangle.Height * .5);
                ClickButton("GetSelected");
                Verify.AreEqual("Num. Selected: 4", ReadResult());
            }
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
        public void TreeViewMultiSelectItemTest_NodeMode()
        {
            TreeViewMultiSelectItemTest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                VerifyGamepadAPress(treeRoot, "4", "Num. Selected: 0");

                // Second A selects root and all children
                VerifyGamepadAPress(treeRoot, "4", "Num. Selected: 4");

                // Third A unselects
                VerifyGamepadAPress(treeRoot, "4", "Num. Selected: 0");

                // Fourth A collapses
                VerifyGamepadAPress(treeRoot, "1", "Num. Selected: 0");

                // Fifth A selects 
                VerifyGamepadAPress(treeRoot, "1", "Num. Selected: 4");

                // Sixth A unselects 
                VerifyGamepadAPress(treeRoot, "1", "Num. Selected: 0");

                // Seventh A expands again
                VerifyGamepadAPress(treeRoot, "4", "Num. Selected: 0");
            }
        }

        [TestMethod]
        public void TreeViewMultiSelectGamepadTest_NodeMode()
        {
            TreeViewMultiSelectGamepadTest();
        }

        [TestMethod]
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
        [TestProperty("Platform", "Desktop")]
        public void TreeViewItemUIATest_NodeMode()
        {
            TreeViewItemUIATest();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
        [TestProperty("Platform", "Desktop")]
        public void ValidateExpandingRaisedOnItemHavingUnrealizedChildren_NodeMode()
        {
            ValidateExpandingRaisedOnItemHavingUnrealizedChildren();
        }

        [TestMethod]
        [TestProperty("Platform", "Desktop")]
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
                Verify.AreEqual("DragItemsStarting:Root->DragItemsCompleted:Root", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewDragItemTest_NodeMode()
        {
            TreeViewDragItemTest();
        }

        [TestMethod]
        public void TreeViewDragItemTest_ContentMode()
        {
            TreeViewDragItemTest(isContentMode:true);
        }

        private void TreeViewDragMultipleItemsTest(bool isContentMode = false)
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
                Verify.AreEqual("Num. Selected: 2", ReadResult());

                UIObject dragUIObject = FindElement.ByName("Root.0");
                Verify.IsNotNull(dragUIObject, "Verifying Root.0 is found");
                InputHelper.DragDistance(dragUIObject, dragUIObject.BoundingRectangle.Height / 2, Direction.South);
                Verify.AreEqual("DragItemsStarting:Root.0|Root.1->DragItemsCompleted:Root.0|Root.1", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewDragMultipleItemsTest_NodeMode()
        {
            TreeViewDragMultipleItemsTest();
        }

        [TestMethod]
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
        public void TreeViewDragEnterOverLeaveTest_NodeMode()
        {
            TreeViewDragEnterOverLeaveTest();
        }

        [TestMethod]
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
        public void TreeViewDragEnterOverDropTest_NodeMode()
        {
            TreeViewDragEnterOverDropTest();
        }

        [TestMethod]
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
        public void TreeViewDisableItemDragTest_NodeMode()
        {
            TreeViewDisableItemDragTest();
        }

        [TestMethod]
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
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2 | ", ReadResult());

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
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2 | ", ReadResult());

                Log.Comment("Verify cannot drag onto a node");
                // Drag Root.0 onto Root.1
                var root1 = FindElement.ById("Root.1");
                InputHelper.DragToTarget(dragUIObject, root1);

                ClickButton("GetChildrenOrder");
                // Verify nodes are still in the same order
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2 | ", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewDisableItemReorderTest_NodeMode()
        {
            TreeViewDisableItemReorderTest();
        }

        [TestMethod]
        public void TreeViewDisableItemReorderTest_ContentMode()
        {
            TreeViewDisableItemReorderTest(isContentMode:true);
        }

        [TestMethod]
        public void TreeViewDisableItemReorderKeyboardTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var ItemRoot = LabelFirstItem();
                InputHelper.Tap(ItemRoot);

                ClickButton("LabelItems");
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.0 | Root.1 | Root.2 | ", ReadResult());

                ClickButton("DisableClickToExpand");

                var root0 = FindElement.ById("Root.0");
                Verify.IsNotNull(root0, "Verifying Root.0 is found");

                // Verify we can reorder items using keyboard by default
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2 | ", ReadResult());

                ClickButton("DisableItemReorder");

                // Verify reorder is disabled
                InputHelper.Tap(root0);
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift);
                ClickButton("GetChildrenOrder");
                Verify.AreEqual("Root | Root.1 | Root.0 | Root.2 | ", ReadResult());
            }
        }

        [TestMethod]
        public void TreeViewItemTemplateSelectorTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                Wait.ForIdle();
                using (var nextPage = new TestSetupHelper("ItemTemplateSelectorTestPage"))
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
        }

        //Bug 16396926
        [TestMethod]
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
        public void TreeViewPartialSelectionTest_NodeMode()
        {
            TreeViewPartialSelectionTest();
        }

        [TestMethod]
        public void TreeViewPartialSelectionTest_ContentMode()
        {
            TreeViewPartialSelectionTest(isContentMode:true);
        }

        [TestMethod]
        public void TreeViewSelectedNodeVectorTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                var root = new TreeItem(LabelFirstItem());
                root.Expand();
                Wait.ForIdle();

                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("p|s|u|s|", ReadResult());

                ClickButton("ToggleSelectedNodes");
                ClickButton("GetMultiSelectCheckBoxStates");
                Verify.AreEqual("u|u|u|u|", ReadResult());
            }
        }

        // Regression test for bug 16833853
        [TestMethod]
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
        public void TreeViewDataLateInitTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                Wait.ForIdle();
                using (var nextPage = new TestSetupHelper("TreeViewLateDataInitTestPage"))
                {
                    ClickButton("InitializeItemsSource");
                    Wait.ForIdle();
                    UIObject node1 = FindElement.ByName("Root.0");
                    Verify.IsNotNull(node1, "Verify data binding");
                }
            }
        }

        private void ClickButton(string buttonName)
        {
            var button = new Button(FindElement.ByName(buttonName));
            button.Invoke();
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
            TapOnTreeViewAt(40, 20, "GetFlyoutItemCount");
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
