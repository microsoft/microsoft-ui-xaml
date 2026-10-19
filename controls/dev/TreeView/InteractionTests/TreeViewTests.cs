// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

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
    // Class-level [TestProperty("Ignore", "True")] (added when MUXControlsTestApp became a desktop .NET app)
    // removed: a /runignoredtests trial of the 88 ignored tests passed 83/88 unmodified; the
    // 5 failures were test-side (stale check-box tap geometry, fixed in ToggleTreeViewItemCheckBox) or infra
    // (MUXTestInfra AppWatcher 15-minute app-lifetime limit; TestCleanup's RestartAppIfLongRunning schedules an
    // app restart after 10 minutes). TreeViewFastReorderTest and TreeViewDataLateInitTest were re-enabled too.
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
            RestartAppIfLongRunning();
        }

        // MUXTestInfra's AppWatcherThread fails the test host once one app instance has run for 15 minutes
        // (Application.cs: "Test app process ran for an unexpectedly long time"). This class has ~90 tests that share
        // one instance, so whichever test straddles that mark fails with 0xE0434352 before it starts. Once the
        // instance is older than the threshold, ask the next TestSetupHelper to restart it. Same mitigation as the
        // TableView interaction tests (TableViewInteractionTestHelpers.RestartAppIfLongRunning), which can't be
        // called from here because that file is only compiled when FeatureTableViewEnabled is set.
        private static readonly TimeSpan s_appRestartThreshold = TimeSpan.FromMinutes(10);

        private static void RestartAppIfLongRunning()
        {
            try
            {
                var process = TestEnvironment.Application.Process;
                if (process != null && !process.HasExited && DateTime.Now - process.StartTime > s_appRestartThreshold)
                {
                    Log.Comment("Test app has run for over {0} minutes; scheduling a restart before the AppWatcherThread limit.",
                        s_appRestartThreshold.TotalMinutes);
                    TestEnvironment.ShouldRestartApplication = true;
                }
            }
            catch (Exception e) when (e is InvalidOperationException || e is System.ComponentModel.Win32Exception)
            {
                // The process could not be queried; leave the restart decision to the infra.
            }
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

        private void SetContentMode(bool isContentMode)
        {
            if(isContentMode)
            {
                ClickButton("SetContentMode");
            }
        }

        private void ExpandCollapseTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: tap Root to expand, add second-level nodes, then tap Root.0, Root and Root.1 to
        //   expand/collapse (the page's ItemInvoked handler toggles IsExpanded).
        // Expected: GetItemCount (flattened list size) is 1, 4, 5 (Root.0 open), 1 (Root closed), 5 (Root.0
        //   still open), 8 (Root.1 open), and 8 again after a collapse+expand of Root.
        // Failure means: ViewModel no longer adds/removes the right descendants on IsExpanded changes
        //   (TreeViewNodeIsExpandedPropertyChanged / AddNodeDescendantsToView) or loses nested expansion state.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseTest_NodeMode()
        {
           ExpandCollapseTest();
        }

        // Scenario: Content mode (SetContentMode): tap Root to expand, add second-level nodes, then tap Root.0,
        //   Root and Root.1 to expand/collapse (the page's ItemInvoked handler toggles IsExpanded).
        // Expected: GetItemCount (flattened list size) is 1, 4, 5 (Root.0 open), 1 (Root closed), 5 (Root.0
        //   still open), 8 (Root.1 open), and 8 again after a collapse+expand of Root.
        // Failure means: ViewModel no longer adds/removes the right descendants on IsExpanded changes
        //   (TreeViewNodeIsExpandedPropertyChanged / AddNodeDescendantsToView) or loses nested expansion state.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseTest_ContentMode()
        {
           ExpandCollapseTest(isContentMode:true);
        }

        private void ExpandCollapseViaAutomationTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: expand/collapse Root through the UIA ExpandCollapse pattern, then through the
        //   UIA Invoke pattern, waiting for the expand/collapse UIA events.
        // Expected: GetItemCount is 1, then 4 after Expand, 1 after Collapse, 4 after the first Invoke and 1
        //   after the second.
        // Failure means: TreeViewItemAutomationPeer::Expand/Collapse (TreeView::Expand/Collapse) or Invoke ->
        //   ItemInvoked no longer changes the node's expansion, so the flattened list keeps the wrong size.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseViaAutomationTest_NodeMode()
        {
            ExpandCollapseViaAutomationTest();
        }

        // Scenario: Content mode (SetContentMode): expand/collapse Root through the UIA ExpandCollapse pattern,
        //   then through the UIA Invoke pattern, waiting for the expand/collapse UIA events.
        // Expected: GetItemCount is 1, then 4 after Expand, 1 after Collapse, 4 after the first Invoke and 1
        //   after the second.
        // Failure means: TreeViewItemAutomationPeer::Expand/Collapse (TreeView::Expand/Collapse) or Invoke ->
        //   ItemInvoked no longer changes the node's expansion, so the flattened list keeps the wrong size.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ExpandCollapseViaAutomationTest_ContentMode()
        {
            ExpandCollapseViaAutomationTest(isContentMode:true);
        }

        private void TreeViewItemClickTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: tap the collapsed Root item once.
        // Expected: GetItemCount is 1 before the tap; Results reads "ItemClicked:Root" (ItemInvoked raised for
        //   Root).
        // Failure means: TreeView::OnItemClick no longer raises ItemInvoked, or InvokedItem is not the clicked
        //   node (node mode) / data item (content mode).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemClickTest_NodeMode()
        {
            TreeViewItemClickTest();
        }

        // Scenario: Content mode (SetContentMode): tap the collapsed Root item once.
        // Expected: GetItemCount is 1 before the tap; Results reads "ItemClicked:Root" (ItemInvoked raised for
        //   Root).
        // Failure means: TreeView::OnItemClick no longer raises ItemInvoked, or InvokedItem is not the clicked
        //   node (node mode) / data item (content mode).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemClickTest_ContentMode()
        {
            TreeViewItemClickTest(isContentMode:true);
        }

        // Scenario: TreeView hosted in a Flyout (TreeViewInFlyout): tap Root's chevron to expand, hide and
        //   reopen the flyout (root node replaced), then expand again.
        // Expected: GetFlyoutItemCount is 4 after each expansion.
        // Failure means: Chevron tap (TreeViewItem::OnExpandCollapseChevronPointerPressed) or replacing
        //   RootNodes[0] no longer expands a TreeView in a re-shown flyout.
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

        // Regression test for bug 15801893
        // Scenario: Open the flyout TreeView, expand Root via the chevron, Left + Escape to collapse and
        //   dismiss, reopen, expand again, then Shift+Tab out of the item.
        // Expected: The app is still alive and GetFlyoutItemCount is 4.
        // Failure means: Shift+Tab from a TreeViewItem in a reopened flyout crashes the app again.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
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
            if (IsPhoneDevice())
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

        // Scenario: Node mode, LTR, DisableClickToExpand: tap Root, then Right/Left keys, gamepad DPad
        //   Left/Right and left-thumbstick Left/Right on Root and its first child.
        // Expected: Tap only invokes (count 1); Right expands (4), Right+Enter -> "ItemClicked:Root.0",
        //   Left+Enter -> "ItemClicked:Root", Left collapses (1); DPad/thumbstick Right -> 4, Left -> 1.
        // Failure means: TreeViewItem::OnKeyDown / IsExpandCollapse / HandleExpandCollapse no longer map
        //   Right/Left (keyboard or gamepad) to expand / go to first child / go to parent / collapse in LTR.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyDownLeftToRightTest_NodeMode()
        {
            TreeViewKeyDownLeftToRightTest();
        }

        // Scenario: Content mode (SetContentMode), LTR, DisableClickToExpand: tap Root, then Right/Left keys,
        //   gamepad DPad Left/Right and left-thumbstick Left/Right on Root and its first child.
        // Expected: Tap only invokes (count 1); Right expands (4), Right+Enter -> "ItemClicked:Root.0",
        //   Left+Enter -> "ItemClicked:Root", Left collapses (1); DPad/thumbstick Right -> 4, Left -> 1.
        // Failure means: TreeViewItem::OnKeyDown / IsExpandCollapse / HandleExpandCollapse no longer map
        //   Right/Left (keyboard or gamepad) to expand / go to first child / go to parent / collapse in LTR.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyDownLeftToRightTest_ContentMode()
        {
            TreeViewKeyDownLeftToRightTest(isContentMode: true);
        }

        private void TreeViewKeyDownRightToLeftTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: ChangeFlowDirection sets RightToLeft, DisableClickToExpand; same steps as the
        //   LTR test with Left/Right (keys, DPad, thumbstick) mirrored.
        // Expected: Left expands (4) and moves to the first child ("ItemClicked:Root.0"); Right returns to Root
        //   ("ItemClicked:Root") and collapses (1); DPad/thumbstick Left -> 4, Right -> 1.
        // Failure means: TreeViewItem::HandleExpandCollapse no longer honors FlowDirection RightToLeft
        //   (flowDirectionReversed), so keyboard/gamepad expand and collapse act in the wrong direction.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewKeyDownRightToLeftTest_NodeMode()
        {
            TreeViewKeyDownRightToLeftTest();
        }

        // Scenario: Content mode (SetContentMode): ChangeFlowDirection sets RightToLeft, DisableClickToExpand;
        //   same steps as the LTR test with Left/Right (keys, DPad, thumbstick) mirrored.
        // Expected: Left expands (4) and moves to the first child ("ItemClicked:Root.0"); Right returns to Root
        //   ("ItemClicked:Root") and collapses (1); DPad/thumbstick Left -> 4, Right -> 1.
        // Failure means: TreeViewItem::HandleExpandCollapse no longer honors FlowDirection RightToLeft
        //   (flowDirectionReversed), so keyboard/gamepad expand and collapse act in the wrong direction.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewKeyDownRightToLeftTest_ContentMode()
        {
            TreeViewKeyDownRightToLeftTest(isContentMode:true);
        }

        private void TreeViewSwappingNodesTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: tap Root to expand, then MoveNodesToNewTreeView moves the expanded root into the
        //   second TreeView and removes the first tree from the page.
        // Expected: Results is "ItemClicked:Root"; GetTree2ItemCount is 6 (2 existing items + Root and its 3
        //   children).
        // Failure means: An expanded node moved to another TreeView loses its expanded descendants in the new
        //   ViewModel (ViewModel::TreeViewNodeVectorChanged / AddNodeDescendantsToView).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewSwappingNodesTest_NodeMode()
        {
            TreeViewSwappingNodesTest();
        }

        // Scenario: Content mode (SetContentMode): tap Root to expand, then MoveNodesToNewTreeView moves the
        //   expanded root into the second TreeView and removes the first tree from the page.
        // Expected: Results is "ItemClicked:Root"; GetTree2ItemCount is 6 (2 existing items + Root and its 3
        //   children).
        // Failure means: An expanded node moved to another TreeView loses its expanded descendants in the new
        //   ViewModel (ViewModel::TreeViewNodeVectorChanged / AddNodeDescendantsToView).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewSwappingNodesTest_ContentMode()
        {
            TreeViewSwappingNodesTest(isContentMode:true);
        }

        private void TreeViewExpandingEventTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: SetUpExpandingNodeEvent adds a "Virtualized" child with HasUnrealizedChildren
        //   whose Expanding handler adds 3 children; expand it with Down x4 + Right.
        // Expected: Tapping Root gives "ItemClicked:Root" and 5 items; after Right on the virtualized node
        //   Results is "Loaded" (Expanding raised) and GetItemCount is 8.
        // Failure means: TreeView::OnNodeExpanding no longer raises Expanding for a node with
        //   HasUnrealizedChildren, or children added in the handler are not inserted into the flattened list.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewExpandingEventTest_NodeMode()
        {
            TreeViewExpandingEventTest();
        }

        // Scenario: Content mode (SetContentMode): SetUpExpandingNodeEvent adds a "Virtualized" child with
        //   HasUnrealizedChildren whose Expanding handler adds 3 children; expand it with Down x4 + Right.
        // Expected: Tapping Root gives "ItemClicked:Root" and 5 items; after Right on the virtualized node
        //   Results is "Loaded" (Expanding raised) and GetItemCount is 8.
        // Failure means: TreeView::OnNodeExpanding no longer raises Expanding for a node with
        //   HasUnrealizedChildren, or children added in the handler are not inserted into the flattened list.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewExpandingEventTest_ContentMode()
        {
            TreeViewExpandingEventTest(isContentMode:true);
        }

        private void TreeViewKeyboardReorderTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode, DisableClickToExpand: expand Root; Alt+Shift+Down twice on Root.0, then
        //   Alt+Shift+Up on the first child and Alt+Shift+Down on the last child and on Root.
        // Expected: GetChildrenOrder is "Root | Root.1 | Root.2 | Root.0" after the two moves and stays so
        //   after the boundary attempts; GetItemCount stays 4.
        // Failure means: TreeViewItem::HandleReorder / ReorderItems no longer move the focused node within its
        //   parent, move it past the first/last sibling, or desync ItemsSource and node order (content mode).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyboardReorderTest_NodeMode()
        {
            TreeViewKeyboardReorderTest();
        }

        // Scenario: Content mode (SetContentMode), DisableClickToExpand: expand Root; Alt+Shift+Down twice on
        //   Root.0, then Alt+Shift+Up on the first child and Alt+Shift+Down on the last child and on Root.
        // Expected: GetChildrenOrder is "Root | Root.1 | Root.2 | Root.0" after the two moves and stays so
        //   after the boundary attempts; GetItemCount stays 4.
        // Failure means: TreeViewItem::HandleReorder / ReorderItems no longer move the focused node within its
        //   parent, move it past the first/last sibling, or desync ItemsSource and node order (content mode).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewKeyboardReorderTest_ContentMode()
        {
            TreeViewKeyboardReorderTest(isContentMode:true);
        }

        private void TreeViewDragAndDropOnNode(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: expand Root, DisableClickToExpand, then touch-drag Root.0 onto Root.1
        //   (InputHelper.DragToTarget).
        // Expected: GetItemCount drops from 4 to 3 (Root.0 now a child of collapsed Root.1); GetChildrenOrder
        //   is "Root | Root.1 | Root.0 | Root.2".
        // Failure means: TreeViewItem::OnDrop no longer reparents the dragged node as the last child of the
        //   drop-target node, or TreeViewItem::OnDragEnter/OnDragOver stop accepting the Move.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragAndDropOnNode_NodeMode()
        {
            TreeViewDragAndDropOnNode();
        }

        // Scenario: Content mode (SetContentMode): expand Root, DisableClickToExpand, then touch-drag Root.0
        //   onto Root.1 (InputHelper.DragToTarget).
        // Expected: GetItemCount drops from 4 to 3 (Root.0 now a child of collapsed Root.1); GetChildrenOrder
        //   is "Root | Root.1 | Root.0 | Root.2".
        // Failure means: TreeViewItem::OnDrop no longer reparents the dragged node as the last child of the
        //   drop-target node, or TreeViewItem::OnDragEnter/OnDragOver stop accepting the Move.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragAndDropOnNode_ContentMode()
        {
            TreeViewDragAndDropOnNode(isContentMode:true);
        }

        private void TreeViewDragAndDropBetweenNodes(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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
                // Drag distance is 1.75 times the height of a TreeViewItem 
                var distance = (int)(height * 1.75);

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

        // Scenario: Node mode: expand Root, click Root.0, then touch-drag it 1.75 item heights south (between
        //   Root.1 and Root.2).
        // Expected: GetChildrenOrder becomes "Root | Root.1 | Root.0 | Root.2" (5 retries); GetItemCount is 4;
        //   GetItemCommonStates (samples container 0) is all "Normal".
        // Failure means: TreeViewList::OnDragOver slot computation or TreeViewList::OnDrop / MoveNodeInto no
        //   longer moves the node between siblings, or an item stays in a drag visual state.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewDragAndDropBetweenNodes();
        }

        // Scenario: Open the TreeView page and measure the collapsed Root item.
        // Expected: Root's UIA bounding rectangle height is 28 DIPs, scaled by the test window DPI (+/-1 px).
        // Failure means: TreeViewItem default template/density resources (min height, padding, margins)
        //   changed.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDensityChange()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                UIObject root = LabelFirstItem();
                VerifyHeightInDips(root.BoundingRectangle.Height, 28, "Root item");
            }
        }

        // Scenario: Content mode (SetContentMode): expand Root, click Root.0, then touch-drag it 1.75 item
        //   heights south (between Root.1 and Root.2).
        // Expected: GetChildrenOrder becomes "Root | Root.1 | Root.0 | Root.2" (5 retries); GetItemCount is 4;
        //   GetItemCommonStates (samples container 0) is all "Normal".
        // Failure means: TreeViewList::OnDragOver slot computation or TreeViewList::OnDrop / MoveNodeInto no
        //   longer moves the node between siblings, or an item stays in a drag visual state.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragAndDropBetweenNodes_ContentMode()
        {
            TreeViewDragAndDropBetweenNodes(isContentMode:true);
        }

        private void TreeViewCollectionChangesEffectSelectedNodesTest(bool isContentMode = false)
        {
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

        // Scenario: Node mode, multi-select: select Root.0 and Root.1 with Space, then AddSecondLevelOfNodes,
        //   ModifySecondLevelOfNode and RemoveSecondLevelOfNode.
        // Expected: GetSelected: none; "Root.0, Root.1"; added children of selected nodes become selected;
        //   "THIS IS NEW" replaces Root.0.0; the removed child leaves the selection.
        // Failure means: ViewModel::SelectedNodeChildrenChanged no longer propagates selection to
        //   added/replaced children or drop removed ones from SelectedNodes/SelectedItems.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewCollectionChangesEffectSelectedNodesTest_NodeMode()
        {
            TreeViewCollectionChangesEffectSelectedNodesTest();
        }

        // Scenario: Content mode (SetContentMode), multi-select: select Root.0 and Root.1 with Space, then
        //   AddSecondLevelOfNodes, ModifySecondLevelOfNode and RemoveSecondLevelOfNode.
        // Expected: GetSelected: none; "Root.0, Root.1"; added children of selected nodes become selected;
        //   "THIS IS NEW" replaces Root.0.0; the removed child leaves the selection.
        // Failure means: ViewModel::SelectedNodeChildrenChanged no longer propagates selection to
        //   added/replaced children or drop removed ones from SelectedNodes/SelectedItems.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewCollectionChangesEffectSelectedNodesTest_ContentMode()
        {
            TreeViewCollectionChangesEffectSelectedNodesTest(isContentMode:true);
        }

        private void TreeViewMultiSelectDragAndDropOnNode(bool isContentMode = false)
        {
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

        // Scenario: Node mode, multi-select: expand Root, select Root.0 and Root.1 with Space, then touch-drag
        //   Root.0 onto Root.2.
        // Expected: GetSelected is "Root.0, Root.1"; afterwards GetItemCount is 2 (both under collapsed Root.2)
        //   and GetChildrenOrder is "Root | Root.2 | Root.0 | Root.1".
        // Failure means: TreeViewItem::OnDrop's multi-select branch (GetRootsOfSelectedSubtrees + append) no
        //   longer moves every selected subtree into the drop-target node.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewMultiSelectDragAndDropOnNode_NodeMode()
        {
            TreeViewMultiSelectDragAndDropOnNode();
        }

        // Scenario: Content mode (SetContentMode), multi-select: expand Root, select Root.0 and Root.1 with
        //   Space, then touch-drag Root.0 onto Root.2.
        // Expected: GetSelected is "Root.0, Root.1"; afterwards GetItemCount is 2 (both under collapsed Root.2)
        //   and GetChildrenOrder is "Root | Root.2 | Root.0 | Root.1".
        // Failure means: TreeViewItem::OnDrop's multi-select branch (GetRootsOfSelectedSubtrees + append) no
        //   longer moves every selected subtree into the drop-target node.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewMultiSelectDragAndDropOnNode_ContentMode()
        {
            TreeViewMultiSelectDragAndDropOnNode(isContentMode:true);
        }

        private void TreeViewMultiSelectDragAndDropBetweenNodes(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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
                // Drag distance is 1.75 times the height of a TreeViewItem 
                var distance = (int)(height * 1.75);

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

        // Scenario: Node mode, multi-select: select Root.0 and Root.1, then touch-drag Root.1 1.75 item heights
        //   south (below Root.2).
        // Expected: GetChildrenOrder is "Root | Root.2 | Root.0 | Root.1", GetItemCount stays 4 and GetSelected
        //   is still "Root.0, Root.1".
        // Failure means: TreeViewList::OnDrop's multi-select path (MoveNodeInto for each selected subtree root,
        //   in order) no longer moves all selected nodes to the drop slot, or the move loses their selection.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewMultiSelectDragAndDropBetweenNodes_NodeMode()
        {
            TreeViewMultiSelectDragAndDropBetweenNodes();
        }

        // Scenario: Content mode (SetContentMode), multi-select: select Root.0 and Root.1, then touch-drag
        //   Root.1 1.75 item heights south (below Root.2).
        // Expected: GetChildrenOrder is "Root | Root.2 | Root.0 | Root.1", GetItemCount stays 4 and GetSelected
        //   is still "Root.0, Root.1".
        // Failure means: TreeViewList::OnDrop's multi-select path (MoveNodeInto for each selected subtree root,
        //   in order) no longer moves all selected nodes to the drop slot, or the move loses their selection.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewMultiSelectDragAndDropBetweenNodes_ContentMode()
        {
            TreeViewMultiSelectDragAndDropBetweenNodes(isContentMode:true);
        }

        private void ValidateCannotDragWithinSelectedSubtree(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode, multi-select: Space on Root selects Root and its children, then touch-drag
        //   Root.1 1.45 item heights south (inside the selected subtree).
        // Expected: GetChildrenOrder stays "Root | Root.0 | Root.1 | Root.2", GetItemCount stays 4 and all four
        //   nodes remain selected.
        // Failure means: TreeViewList::OnDragOver / TreeViewItem::OnDragOver no longer reject drops on or
        //   beside selected nodes, so a selected subtree can be dropped into itself.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateCannotDragWithinSelectedSubtree_NodeMode()
        {
            ValidateCannotDragWithinSelectedSubtree();
        }

        // Scenario: Content mode (SetContentMode), multi-select: Space on Root selects Root and its children,
        //   then touch-drag Root.1 1.45 item heights south (inside the selected subtree).
        // Expected: GetChildrenOrder stays "Root | Root.0 | Root.1 | Root.2", GetItemCount stays 4 and all four
        //   nodes remain selected.
        // Failure means: TreeViewList::OnDragOver / TreeViewItem::OnDragOver no longer reject drops on or
        //   beside selected nodes, so a selected subtree can be dropped into itself.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateCannotDragWithinSelectedSubtree_ContentMode()
        {
            ValidateCannotDragWithinSelectedSubtree(isContentMode:true);
        }

        private void ValidateMultiSelectDragDropRootsAndNonRoots(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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
                // Drag distance is 1.75 times the height of a TreeViewItem 
                var distance = (int)(height * 1.75);

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

        // Scenario: Node mode, multi-select: select subtree Root.0 (+Root.0.0) and nested leaf Root.1.0 by
        //   keyboard, then drag Root.1.0 1.75 item heights south.
        // Expected: Selection is "Root.0, Root.0.0, Root.1.0" before and after; order becomes "Root | Root.1 |
        //   Root.1.1 | Root.0 | Root.0.0 | Root.1.0 | Root.1.2 | Root.2"; count 8.
        // Failure means: TreeViewList::GetRootsOfSelectedSubtrees / MoveNodeInto mishandle a selection mixing a
        //   subtree root and a nested leaf from different parents (wrong parent, order or lost nodes).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void ValidateMultiSelectDragDropRootsAndNonRoots_NodeMode()
        {
            ValidateMultiSelectDragDropRootsAndNonRoots();
        }

        // Scenario: Content mode (SetContentMode), multi-select: select subtree Root.0 (+Root.0.0) and nested
        //   leaf Root.1.0 by keyboard, then drag Root.1.0 1.75 item heights south.
        // Expected: Selection is "Root.0, Root.0.0, Root.1.0" before and after; order becomes "Root | Root.1 |
        //   Root.1.1 | Root.0 | Root.0.0 | Root.1.0 | Root.1.2 | Root.2"; count 8.
        // Failure means: TreeViewList::GetRootsOfSelectedSubtrees / MoveNodeInto mishandle a selection mixing a
        //   subtree root and a nested leaf from different parents (wrong parent, order or lost nodes).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void ValidateMultiSelectDragDropRootsAndNonRoots_ContentMode()
        {
            ValidateMultiSelectDragDropRootsAndNonRoots(isContentMode:true);
        }

        private void TreeViewDragCrash(bool isContentMode = false)
        {
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

        // Scenario: Node mode: expand Root, SizeTreeViewsForDrags, then touch-drag the last item Root.2 90px
        //   south past the end of the list to hit every DragOver path.
        // Expected: No crash; GetItemCount is still 4.
        // Failure means: TreeViewList::OnDragOver (insertion index clamping when belowIndex is -1 or past the
        //   list) or UpdateDropTargetDropEffect crashes while dragging below the last item.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragCrash_NodeMode()
        {
            TreeViewDragCrash();
        }

        // Scenario: Content mode (SetContentMode): expand Root, SizeTreeViewsForDrags, then touch-drag the last
        //   item Root.2 90px south past the end of the list to hit every DragOver path.
        // Expected: No crash; GetItemCount is still 4.
        // Failure means: TreeViewList::OnDragOver (insertion index clamping when belowIndex is -1 or past the
        //   list) or UpdateDropTargetDropEffect crashes while dragging below the last item.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TreeViewDragCrash_ContentMode()
        {
            TreeViewDragCrash(isContentMode:true);
        }

        private void DragItemOutOfTreeView(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Per-method Ignore removed in this change. Node mode: SetupDragToDropTarget, expand Root,
        //   drag Root.2 onto the external "DropTarget" Border (retried).
        // Expected: DropTargetTextBlock reads "Root.2": the external target received the dragged item's data.
        // Failure means: TreeViewList::OnDragItemsStarting / TreeView::OnListControlDragItemsStarting no longer
        //   expose the dragged node/item (or its data package) to an external drop target.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void DragItemOutOfTreeView_NodeMode()
        {
            DragItemOutOfTreeView();
        }

        // Scenario: Content mode (SetContentMode): SetupDragToDropTarget (DragItemsStarting puts item names in
        //   the data), expand Root, drag Root.2 onto the external "DropTarget" Border (retried).
        // Expected: DropTargetTextBlock reads "Root.2": the external target received the dragged item's data.
        // Failure means: TreeViewList::OnDragItemsStarting / TreeView::OnListControlDragItemsStarting no longer
        //   expose the dragged node/item (or its data package) to an external drop target.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void DragItemOutOfTreeView_ContentMode()
        {
            DragItemOutOfTreeView(isContentMode:true);
        }

        private void DragItemOntoTreeView(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: SetupDragToDropTarget, then drag the external DraggableElement (text "Test
        //   Item", Copy) onto TestTreeView / ContentModeTestTreeView.
        // Expected: Results reads "Dropped: Test Item": the TreeView's Drop event received the external data.
        // Failure means: TreeViewList/TreeViewItem OnDragOver/OnDrop handle (or reject) external drags with no
        //   DraggedTreeViewNode, so the app's TreeView DragOver/Drop handlers no longer run.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void DragItemOntoTreeView_NodeMode()
        {
            DragItemOntoTreeView();
        }

        // Scenario: Content mode (SetContentMode): SetupDragToDropTarget, then drag the external
        //   DraggableElement (text "Test Item", Copy) onto TestTreeView / ContentModeTestTreeView.
        // Expected: Results reads "Dropped: Test Item": the TreeView's Drop event received the external data.
        // Failure means: TreeViewList/TreeViewItem OnDragOver/OnDrop handle (or reject) external drags with no
        //   DraggedTreeViewNode, so the app's TreeView DragOver/Drop handlers no longer run.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void DragItemOntoTreeView_ContentMode()
        {
            DragItemOntoTreeView(isContentMode:true);
        }

        private void TreeViewNoReorderIntoChildren(bool isContentMode = false)
        {
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

                var topBottomMargin = 4; // To account for the 2px top and 2px bottom margin for TreeViewItem
                var height = dragUIObject.BoundingRectangle.Height + topBottomMargin; 
                // 5% from the edge of the next item 
                var distance = (int)(height * 1.45);

                Log.Comment("Starting Drag...distance:" + distance);
                InputHelper.DragDistance(dragUIObject, distance, Direction.North);

                ClickButton("GetItemCount");
                Verify.AreEqual("7", ReadResult());
            }
        }

        // Scenario: Node mode: SetupNoReorderNodes sets AllowDrop=false on Root.0 and gives it 3 children;
        //   expand it (7 items) and drag Root.1 north 1.45 heights into Root.0's children.
        // Expected: GetItemCount is still 7 (only the count is asserted, not the order).
        // Failure means: Dragging over a child of an AllowDrop=false container (TreeViewList::OnDragOver
        //   allowReorder check) crashes or loses/duplicates nodes.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNoReorderIntoChildren_NodeMode()
        {
            TreeViewNoReorderIntoChildren();
        }

        // Scenario: Content mode (SetContentMode): SetupNoReorderNodes sets AllowDrop=false on Root.0 and gives
        //   it 3 children; expand it (7 items) and drag Root.1 north 1.45 heights into Root.0's children.
        // Expected: GetItemCount is still 7 (only the count is asserted, not the order).
        // Failure means: Dragging over a child of an AllowDrop=false container (TreeViewList::OnDragOver
        //   allowReorder check) crashes or loses/duplicates nodes.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewNoReorderIntoChildren_ContentMode()
        {
            TreeViewNoReorderIntoChildren(isContentMode:true);
        }

        // Scenario: Open the flyout TreeView and expand Root, tap the TreeViewInFlyout button to dismiss it,
        //   then ChangeSelectionAfterFlyout sets IsSelected=false on the cached flyout TreeViewItem.
        // Expected: GetFlyoutItemCount is 4 before dismissing; afterwards only that the app does not crash.
        // Failure means: TreeViewItem::OnIsSelectedChanged / UpdateSelection crashes for a container whose
        //   flyout (and TreeView/list control) is closed.
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
            if (IsPhoneDevice())
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

        // Scenario: Node mode, multi-select, DisableClickToExpand: tap Root at 75% width, then
        //   Space/Right/Down/Left combinations on Root, Root.0 and Root.2.
        // Expected: Space on Root selects all 4 (count 1); Space on Root.0 -> "Root.1, Root.2"; on Root.2 ->
        //   "Root.1"; Left collapses (1) keeping "Root.1"; Space on partial Root selects all 4.
        // Failure means: TreeViewItem::OnKeyDown Space handling in multi-select, or ViewModel::UpdateSelection
        //   descendant/ancestor propagation, regressed; or collapsing drops the selection.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectKeyboardingTest_NodeMode()
        {
            TreeViewMultiSelectKeyboardingTest();
        }

        // Scenario: Content mode (SetContentMode), multi-select, DisableClickToExpand: tap Root at 75% width,
        //   then Space/Right/Down/Left combinations on Root, Root.0 and Root.2.
        // Expected: Space on Root selects all 4 (count 1); Space on Root.0 -> "Root.1, Root.2"; on Root.2 ->
        //   "Root.1"; Left collapses (1) keeping "Root.1"; Space on partial Root selects all 4.
        // Failure means: TreeViewItem::OnKeyDown Space handling in multi-select, or ViewModel::UpdateSelection
        //   descendant/ancestor propagation, regressed; or collapsing drops the selection.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectKeyboardingTest_ContentMode()
        {
            TreeViewMultiSelectKeyboardingTest(isContentMode:true);
        }

        private void TreeViewMultiSelectItemTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

                ToggleTreeViewItemCheckBox(ItemRoot, 1);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());

                Log.Comment("Retrieve first item as generic UIElement");
                UIObject Item0 = FindElement.ById("Root.0");
                Verify.IsNotNull(Item0, "Verifying that we found a UIElement called Root.0");

                ToggleTreeViewItemCheckBox(Item0, 2);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root.1, Root.2", ReadResult());

                // tap on partial selected node will set it to unselected state
                ToggleTreeViewItemCheckBox(ItemRoot, 1);
                ClickButton("GetSelected");
                Verify.AreEqual("Nothing selected", ReadResult());

                ToggleTreeViewItemCheckBox(ItemRoot, 1);
                ClickButton("GetSelected");
                Verify.AreEqual("Selected: Root, Root.0, Root.1, Root.2", ReadResult());
            }
        }

        // Scenario: Node mode, multi-select: expand Root, then tap check boxes with ToggleTreeViewItemCheckBox
        //   (template offsets x window DPI scale): Root, Root.0, Root twice.
        // Expected: Count 1 -> 4; checking Root selects all 4; unchecking Root.0 -> "Root.1, Root.2"; tapping
        //   the partial Root -> "Nothing selected"; tapping it again selects all 4.
        // Failure means: TreeViewItem::OnCheckToggle -> ViewModel::SelectByIndex/UpdateSelection no longer
        //   propagates the check state to descendants/ancestors, or a partial box no longer clears the subtree.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectItemTest_NodeMode()
        {
            TreeViewMultiSelectItemTest();
        }

        // Scenario: Content mode (SetContentMode), multi-select: expand Root, then tap check boxes with
        //   ToggleTreeViewItemCheckBox (template offsets x window DPI scale): Root, Root.0, Root twice.
        // Expected: Count 1 -> 4; checking Root selects all 4; unchecking Root.0 -> "Root.1, Root.2"; tapping
        //   the partial Root -> "Nothing selected"; tapping it again selects all 4.
        // Failure means: TreeViewItem::OnCheckToggle -> ViewModel::SelectByIndex/UpdateSelection no longer
        //   propagates the check state to descendants/ancestors, or a partial box no longer clears the subtree.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewMultiSelectItemTest_ContentMode()
        {
            TreeViewMultiSelectItemTest(isContentMode:true);
        }

        private void TreeViewMultiSelectGamepadTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode, multi-select: press gamepad A on Root seven times (VerifyGamepadAPress reads
        //   count and selection after each).
        // Expected: A cycles expand, select all, unselect, collapse, select all, unselect, expand: counts
        //   4,4,4,1,1,1,4 with selection none/all/none/none/all/none/none.
        // Failure means: TreeViewItem::HandleGamepadAInMultiselectMode / ToggleSelection no longer alternate
        //   expansion and selection for a non-leaf node (m_expansionCycled).
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewMultiSelectGamepadTest_NodeMode()
        {
            TreeViewMultiSelectGamepadTest();
        }

        // Scenario: Content mode (SetContentMode), multi-select: press gamepad A on Root seven times
        //   (VerifyGamepadAPress reads count and selection after each).
        // Expected: A cycles expand, select all, unselect, collapse, select all, unselect, expand: counts
        //   4,4,4,1,1,1,4 with selection none/all/none/none/all/none/none.
        // Failure means: TreeViewItem::HandleGamepadAInMultiselectMode / ToggleSelection no longer alternate
        //   expansion and selection for a non-leaf node (m_expansionCycled).
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewMultiSelectGamepadTest_ContentMode()
        {
            TreeViewMultiSelectGamepadTest(isContentMode:true);
        }

        private void TreeViewItemUIATest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: expand Root, switch to multi-select, focus Root.1 and get the Selection pattern
        //   of its SelectionContainer (the TreeViewList peer).
        // Expected: Selection pattern exists, CanSelectMultiple is true, IsSelectionRequired is false,
        //   GetSelection is empty.
        // Failure means: TreeViewListAutomationPeer (GetPatternCore / CanSelectMultiple / IsSelectionRequired /
        //   GetSelection) or TreeViewItemAutomationPeer::SelectionContainer regressed in multi-select mode.
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

        // Scenario: Node mode: expand Root, read Root.1's UIA properties, select via tap and SelectionItem in
        //   single mode, then Space/RemoveFromSelection/AddToSelection in multi-select.
        // Expected: Root.1: PositionInSet 2, SizeOfSet 3, Level 2, LeafNode; Root Expanded; IsSelected follows
        //   each step; AddToSelection on Root selects Root.1 too.
        // Failure means: TreeViewItemAutomationPeer (PositionInSet/SizeOfSet/Level cores, ExpandCollapseState,
        //   IsSelected, Add/RemoveFromSelection) misreports structure or selection.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemUIATest_NodeMode()
        {
            TreeViewItemUIATest();
        }

        // Scenario: Content mode (SetContentMode): expand Root, read Root.1's UIA properties, select via tap
        //   and SelectionItem in single mode, then Space/RemoveFromSelection/AddToSelection in multi-select.
        // Expected: Root.1: PositionInSet 2, SizeOfSet 3, Level 2, LeafNode; Root Expanded; IsSelected follows
        //   each step; AddToSelection on Root selects Root.1 too.
        // Failure means: TreeViewItemAutomationPeer (PositionInSet/SizeOfSet/Level cores, ExpandCollapseState,
        //   IsSelected, Add/RemoveFromSelection) misreports structure or selection.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewItemUIATest_ContentMode()
        {
            TreeViewItemUIATest(isContentMode:true);
        }

        private void ValidateExpandingRaisedOnItemHavingUnrealizedChildren(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: UIA-expand Root, SetRoot1HasUnrealizedChildren on childless Root.1 (hooks
        //   Expanding), then UIA-expand Root.1.
        // Expected: GetItemCount is 4 after Root expands; Results reads "Expanding Raised".
        // Failure means: Expanding is no longer raised for a childless node with HasUnrealizedChildren
        //   (TreeView::OnNodeExpanding / TreeViewItemAutomationPeer::Expand).
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateExpandingRaisedOnItemHavingUnrealizedChildren_NodeMode()
        {
            ValidateExpandingRaisedOnItemHavingUnrealizedChildren();
        }

        // Scenario: Content mode (SetContentMode): UIA-expand Root, SetRoot1HasUnrealizedChildren on childless
        //   Root.1 (hooks Expanding), then UIA-expand Root.1.
        // Expected: GetItemCount is 4 after Root expands; Results reads "Expanding Raised".
        // Failure means: Expanding is no longer raised for a childless node with HasUnrealizedChildren
        //   (TreeView::OnNodeExpanding / TreeViewItemAutomationPeer::Expand).
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void ValidateExpandingRaisedOnItemHavingUnrealizedChildren_ContentMode()
        {
            ValidateExpandingRaisedOnItemHavingUnrealizedChildren(isContentMode:true);
        }

        private void TreeViewDragItemTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: SetupDragDropHandlersForApiTest logs DragItemsStarting/Completed; touch-drag the
        //   collapsed Root one item height south.
        // Expected: Results is "DragItemsStarting:Root\nDragItemsCompleted:Root" (no NewParent line for a root
        //   item).
        // Failure means: TreeView::OnListControlDragItemsStarting/Completed no longer raise the TreeView drag
        //   events with the dragged node/item, or NewParentItem is set for a root-level item.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragItemTest_NodeMode()
        {
            TreeViewDragItemTest();
        }

        // Scenario: Content mode (SetContentMode): SetupDragDropHandlersForApiTest logs
        //   DragItemsStarting/Completed; touch-drag the collapsed Root one item height south.
        // Expected: Results is "DragItemsStarting:Root\nDragItemsCompleted:Root" (no NewParent line for a root
        //   item).
        // Failure means: TreeView::OnListControlDragItemsStarting/Completed no longer raise the TreeView drag
        //   events with the dragged node/item, or NewParentItem is set for a root-level item.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TreeViewDragItemTest_ContentMode()
        {
            TreeViewDragItemTest(isContentMode:true);
        }

        private void TreeViewDragMultipleItemsTest(bool isContentMode = false)
        {
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

        // Scenario: Node mode, multi-select + API drag handlers: select Root.0 and Root.1 with Space, then
        //   touch-drag Root.0 onto Root.2.
        // Expected: Results is "DragItemsStarting:Root.0|Root.1\nDragItemsCompleted:Root.0|Root.1\nNewParent:
        //   Root.2".
        // Failure means: TreeViewList::OnDragItemsStarting no longer puts all selected nodes/items in
        //   args.Items, or TreeView::OnListControlDragItemsCompleted reports the wrong NewParentItem.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragMultipleItemsTest_NodeMode()
        {
            TreeViewDragMultipleItemsTest();
        }

        // Scenario: Content mode (SetContentMode), multi-select + API drag handlers: select Root.0 and Root.1
        //   with Space, then touch-drag Root.0 onto Root.2.
        // Expected: Results is "DragItemsStarting:Root.0|Root.1\nDragItemsCompleted:Root.0|Root.1\nNewParent:
        //   Root.2".
        // Failure means: TreeViewList::OnDragItemsStarting no longer puts all selected nodes/items in
        //   args.Items, or TreeView::OnListControlDragItemsCompleted reports the wrong NewParentItem.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragMultipleItemsTest_ContentMode()
        {
            TreeViewDragMultipleItemsTest(isContentMode:true);
        }

        private void TreeViewDragEnterOverLeaveTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: API drag handlers on the second tree; drag the external draggable element to the
        //   first tree, passing over (and leaving) the second tree.
        // Expected: Results is "DragEnter->DragOver->DragLeave" from the second tree's handlers.
        // Failure means: TreeViewList::OnDragEnter/OnDragOver/OnDragLeave (or TreeViewItem overrides) swallow
        //   external drag events, so TreeView DragEnter/DragOver/DragLeave no longer reach the app.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragEnterOverLeaveTest_NodeMode()
        {
            TreeViewDragEnterOverLeaveTest();
        }

        // Scenario: Content mode (SetContentMode): API drag handlers on the second tree; drag the external
        //   draggable element to the first tree, passing over (and leaving) the second tree.
        // Expected: Results is "DragEnter->DragOver->DragLeave" from the second tree's handlers.
        // Failure means: TreeViewList::OnDragEnter/OnDragOver/OnDragLeave (or TreeViewItem overrides) swallow
        //   external drag events, so TreeView DragEnter/DragOver/DragLeave no longer reach the app.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragEnterOverLeaveTest_ContentMode()
        {
            TreeViewDragEnterOverLeaveTest(isContentMode:true);
        }

        private void TreeViewDragEnterOverDropTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: API drag handlers on the second tree; drag the external draggable element and
        //   drop it on the second tree.
        // Expected: Results is "DragEnter->DragOver->Drop" from the second tree's handlers.
        // Failure means: TreeViewList::OnDrop / TreeViewItem::OnDrop (or the drag-enter/over overrides) swallow
        //   an external drop, so the TreeView Drop event no longer reaches the app.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragEnterOverDropTest_NodeMode()
        {
            TreeViewDragEnterOverDropTest();
        }

        // Scenario: Content mode (SetContentMode): API drag handlers on the second tree; drag the external
        //   draggable element and drop it on the second tree.
        // Expected: Results is "DragEnter->DragOver->Drop" from the second tree's handlers.
        // Failure means: TreeViewList::OnDrop / TreeViewItem::OnDrop (or the drag-enter/over overrides) swallow
        //   an external drop, so the TreeView Drop event no longer reaches the app.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewDragEnterOverDropTest_ContentMode()
        {
            TreeViewDragEnterOverDropTest(isContentMode:true);
        }

        private void TreeViewDisableItemDragTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: API drag handlers, DisableItemDrag (CanDragItems=false), expand Root, then
        //   touch-drag Root.1 onto Root.2.
        // Expected: Results stays empty: DragItemsStarting/DragItemsCompleted were never raised.
        // Failure means: TreeView.CanDragItems=false no longer reaches the inner TreeViewList, so items can
        //   still be dragged.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewDisableItemDragTest_NodeMode()
        {
            TreeViewDisableItemDragTest();
        }

        // Scenario: Content mode (SetContentMode): API drag handlers, DisableItemDrag (CanDragItems=false),
        //   expand Root, then touch-drag Root.1 onto Root.2.
        // Expected: Results stays empty: DragItemsStarting/DragItemsCompleted were never raised.
        // Failure means: TreeView.CanDragItems=false no longer reaches the inner TreeViewList, so items can
        //   still be dragged.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewDisableItemDragTest_ContentMode()
        {
            TreeViewDisableItemDragTest(isContentMode:true);
        }

        private void TreeViewDisableItemReorderTest(bool isContentMode = false)
        {
            if (IsPhoneDevice())
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

        // Scenario: Node mode: expand Root, DisableItemReorder (CanReorderItems=false), drag Root.0 1.5 heights
        //   south (between nodes), then drag Root.0 onto Root.1.
        // Expected: GetChildrenOrder stays "Root | Root.0 | Root.1 | Root.2" before and after both drags.
        // Failure means: TreeViewList::OnDragOver / TreeViewItem::OnDragEnter/OnDragOver no longer honor
        //   CanReorderItems=false, allowing reorder or reparenting by drag.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewDisableItemReorderTest_NodeMode()
        {
            TreeViewDisableItemReorderTest();
        }

        // Scenario: Content mode (SetContentMode): expand Root, DisableItemReorder (CanReorderItems=false),
        //   drag Root.0 1.5 heights south (between nodes), then drag Root.0 onto Root.1.
        // Expected: GetChildrenOrder stays "Root | Root.0 | Root.1 | Root.2" before and after both drags.
        // Failure means: TreeViewList::OnDragOver / TreeViewItem::OnDragEnter/OnDragOver no longer honor
        //   CanReorderItems=false, allowing reorder or reparenting by drag.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewDisableItemReorderTest_ContentMode()
        {
            TreeViewDisableItemReorderTest(isContentMode:true);
        }

        // Scenario: Node mode: expand Root, DisableClickToExpand, Alt+Shift+Down on Root.0; then
        //   DisableItemReorder (CanReorderItems=false) and repeat.
        // Expected: First reorder gives "Root | Root.1 | Root.0 | Root.2"; with reorder disabled the order is
        //   unchanged.
        // Failure means: TreeViewItem::IsInReorderMode / HandleReorder no longer reorder by keyboard by default
        //   or ignore CanReorderItems=false.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Navigate to ItemTemplateSelectorTestPage (TreeView with ItemTemplateSelector and
        //   ItemContainerStyleSelector setting Height 50/60).
        // Expected: Items "Template1" and "Template2" exist with heights 46 and 56 DIPs (style height minus 4px
        //   margins), scaled by the test window DPI (+/-1 px).
        // Failure means: TreeView no longer forwards ItemTemplateSelector / ItemContainerStyleSelector to the
        //   inner TreeViewList and its TreeViewItem containers.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewItemTemplateSelectorTest()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "ItemTemplateSelectorTestPage" }))
            {
                Log.Comment("ItemTemplateSelector test page is ready");
                UIObject node1 = FindElement.ByName("Template1");
                Verify.IsNotNull(node1, "Verifying template 1 is set");
                UIObject node2 = FindElement.ByName("Template2");
                Verify.IsNotNull(node2, "Verifying template 2 is set");

                var topBottomMargin = 4; // To account for the 2px top and 2px bottom margins for TreeViewItem

                // Verify item container styles are set correctly by checking heights
                VerifyHeightInDips(node1.BoundingRectangle.Height, 50 - topBottomMargin, "Template1 item");
                VerifyHeightInDips(node2.BoundingRectangle.Height, 60 - topBottomMargin, "Template2 item");
            }
        }

        //Bug 16396926
        // Scenario: Node mode: AddNodeWithEmpyUnrealizedChildren adds Root.3 (HasUnrealizedChildren,
        //   IsExpanded, no children); keyboard: Right expands Root, Down x4 to Root.3, Right on it.
        // Expected: No crash; GetItemCount is 5 (Root + 4 children, nothing added under Root.3).
        // Failure means: TreeViewItem::HandleExpandCollapse / ViewModel crash on an expanded node with
        //   HasUnrealizedChildren but an empty Children vector.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Node mode: expand Root, Root.0, Root.1 (8 items), multi-select, tap check boxes via
        //   ToggleTreeViewItemCheckBox (DPI-scaled): Root.0.0, Root.1.0, Root.1 twice, Root.2.
        // Expected: GetMultiSelectCheckBoxStates (pre-order, u/s/p): u*8 -> p|s|s|u.. -> p|s|s|p|s|u.. ->
        //   partial Root.1 clears to p|s|s|u.. -> p|s|s|s|s|s|s|u -> s*8.
        // Failure means: ViewModel::UpdateSelectionStateOfAncestors/Descendants (SelectionStateBasedOnChildren)
        //   compute wrong partial states, or TreeViewItem::OnCheckToggle ignores the box.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewPartialSelectionTest_NodeMode()
        {
            TreeViewPartialSelectionTest();
        }

        // Scenario: Content mode (SetContentMode): expand Root, Root.0, Root.1 (8 items), multi-select, tap
        //   check boxes via ToggleTreeViewItemCheckBox (DPI-scaled): Root.0.0, Root.1.0, Root.1 twice, Root.2.
        // Expected: GetMultiSelectCheckBoxStates (pre-order, u/s/p): u*8 -> p|s|s|u.. -> p|s|s|p|s|u.. ->
        //   partial Root.1 clears to p|s|s|u.. -> p|s|s|s|s|s|s|u -> s*8.
        // Failure means: ViewModel::UpdateSelectionStateOfAncestors/Descendants (SelectionStateBasedOnChildren)
        //   compute wrong partial states, or TreeViewItem::OnCheckToggle ignores the box.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewPartialSelectionTest_ContentMode()
        {
            TreeViewPartialSelectionTest(isContentMode:true);
        }

        // Scenario: Node mode, single select: UIA-expand Root, tap Root.0, then ToggleRoot0Selection sets
        //   TreeView.SelectedNode = null.
        // Expected: GetSelected is "Selected: Root.0" (SelectedNode/Nodes/Item/Items consistent), then "Nothing
        //   selected".
        // Failure means: TreeView::SelectedNode getter/setter no longer reflects or clears the single selection
        //   made by tap.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Node mode: UIA-expand Root; ToggleSelectedNodes (switches to Multiple) adds then removes
        //   Root.0 and Root.2 in SelectedNodes; then tap Root.0 and press Space.
        // Expected: Check boxes u|u|u|u| -> p|s|u|s| -> u|u|u|u|; GetSelected "Nothing selected", then
        //   "Selected: Root.0".
        // Failure means: SelectedTreeNodeVector Append/RemoveAt no longer update node/container selection
        //   states, or UI selection no longer updates SelectedNodes (ViewModel::UpdateSelection).
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Content mode, single select: UIA-expand Root, tap Root.0, then ToggleRoot0Selection sets
        //   TreeView.SelectedItem = null.
        // Expected: GetSelected is "Selected: Root.0", then "Nothing selected".
        // Failure means: TreeView::OnPropertyChanged(SelectedItem) -> ViewModel::SelectSingleItem no longer
        //   clears (or reports) the content-mode selection.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewSelectedItemTest()
        {
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

        // Scenario: Content mode: UIA-expand Root; ToggleSelectedNodes (switches to Multiple) adds then removes
        //   Root.0 and Root.2 in SelectedItems; then tap Root.0 and press Space.
        // Expected: Check boxes u|u|u|u| -> p|s|u|s| -> u|u|u|u|; GetSelected "Nothing selected", then
        //   "Selected: Root.0".
        // Failure means: SelectedItemsVector Append/RemoveAt no longer update node/container selection states,
        //   or UI selection no longer updates SelectedItems.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewSelectedItemsTest()
        {
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
        // Scenario: 5 iterations (flaky repro): AddExtraNodes adds Node 1..50, SelectLastRootNode sets
        //   SelectedNode to the off-screen Node 50, then tap Node 1.
        // Expected: GetSelected is "Selected: Node 50", then "Selected: Node 1" in every iteration.
        // Failure means: Ignored (PC-7, product bug): in Single mode TreeView.SelectedItem only follows the inner
        //   ListView's SelectionChanged, so selecting the unrealized Node 50 leaves SelectedItem stale until it is
        //   realized. A pass on slow agents would mean SelectedItem is now synced from the ViewModel selection.
        [TestMethod]
        [TestProperty("TreeViewTestSuite", "B")]
        // PC-7 (product bug): in Single mode TreeView.SelectedItem is only synced from the inner ListView's
        // SelectionChanged, which needs a realized container; selecting an unrealized node via SelectedNode leaves
        // SelectedItem stale until realization. Timing-dependent: fails consistently on Win10 RS5 x86chk CI agents
        // (PR #12227), passes on faster Win11 agents. Re-enable once SelectedItem is updated from
        // the ViewModel selection change.
        [TestProperty("Ignore", "True")]
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
        // Scenario: Node mode: add second-level nodes, expand Root and Root.0, hold the focused UIA element of
        //   Root.0.0, then RemoveSecondLevelOfNode removes that node.
        // Expected: Before: PositionInSet 1, SizeOfSet 1, Level 3; after removal the stale peer reports -1 for
        //   all three.
        // Failure means: TreeViewItemAutomationPeer::GetPositionInSetCore/GetSizeOfSetCore/GetLevelCore crash
        //   or return stale values for a container whose node was removed.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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
        // Scenario: Node mode: focus Root's peer in the default Single mode, then ToggleSelectionMode twice
        //   (Single -> Multiple -> None).
        // Expected: Root's peer supports SelectionItem in Single mode and no longer supports it in None mode.
        // Failure means: TreeViewItemAutomationPeer::GetPatternCore no longer omits SelectionItem when
        //   SelectionMode is None (or omits it in Single).
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Node mode, multi-select: focus the collapsed Root and press Space three times while
        //   listening for UIA selection events.
        // Expected: IsSelected starts false; 1st Space fires IsSelected changed (true), 2nd
        //   ElementRemovedFromSelection (false), 3rd ElementAddedToSelection (true).
        // Failure means: TreeViewItem::OnCheckToggle -> RaiseSelectionChangeEvents no longer raises the
        //   SelectionItem UIA events or the IsSelected property change.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Node mode: UIA-expand Root, AddInheritedTreeViewNode appends a TreeViewNode2 (C# subclass
        //   of TreeViewNode), then LabelItems.
        // Expected: An item with AutomationId "Inherited from TreeViewNode" exists (a container was realized
        //   for it).
        // Failure means: TreeViewNode can no longer be subclassed from app code, or derived nodes get no
        //   container (TreeView::ContainerFromNode / ViewModel).
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Re-enabled in this change. Node mode: AddExtraNodes (51 roots), focus Node 1, send 50 rapid
        //   Alt+Shift+Down then 50 Alt+Shift+Up (SendKeysDelay 10ms).
        // Expected: GetItemCount is 51 before and after: the app still responds and no node was lost or
        //   duplicated.
        // Failure means: TreeViewItem::HandleReorder / ReorderItems crash or corrupt the flattened list under
        //   rapid repeated keyboard reorders.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewFastReorderTest()
        {
            using (var setup = new TestSetupHelper("TreeView Tests"))
            {
                ClickButton("AddExtraNodes");
                ClickButton("LabelItems");
                ClickButton("GetItemCount");
                Verify.AreEqual("51", ReadResult());
                var node = FindElement.ById("Node 1");
                Verify.IsNotNull(node);
                node.SetFocus();
                Log.Comment("Move down node 1");
                var originalDelay = Keyboard.SendKeysDelay;
                Keyboard.SendKeysDelay = 10;
                try
                {
                    KeyboardHelper.PressKey(Key.Down, ModifierKey.Alt | ModifierKey.Shift, 50);
                    Log.Comment("Move up node 1");
                    KeyboardHelper.PressKey(Key.Up, ModifierKey.Alt | ModifierKey.Shift, 50);
                }
                finally
                {
                    Keyboard.SendKeysDelay = originalDelay;
                }

                // The app must still respond and no node may be lost or duplicated by the rapid reorders.
                ClickButton("GetItemCount");
                Verify.AreEqual("51", ReadResult());
                Log.Comment("App didn't crash");
            }
        }

        // Scenario: Re-enabled in this change. TreeViewLateDataInitTestPage: the TreeView's ItemsSource is
        //   bound (OneWay) to a null collection; InitializeItemsSource assigns it after load.
        // Expected: An item named "Root" appears.
        // Failure means: TreeView::OnPropertyChanged(ItemsSource) / TreeViewNode::ItemsSource no longer
        //   populate the tree when ItemsSource is first set late.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
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

        // Scenario: Navigate to TreeViewNodeInMarkupTestPage, whose RootNodes (Root > Root.0) are declared in
        //   XAML.
        // Expected: An item named "Root" exists.
        // Failure means: TreeViewNode objects declared in markup (TreeView.RootNodes / TreeViewNode.Children)
        //   no longer load.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void TreeViewNodeInMarkupTest()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewNodeInMarkupTestPage" }))
            {
                UIObject root = FindElement.ByName("Root");
                Verify.IsNotNull(root, "Verify root node content");
            }
        }

        // Scenario: Content mode: ClearNodes empties ItemsSource, AddRootNode twice adds Root0 and Root1, then
        //   drag Root1 onto Root0 and UIA-expand Root0.
        // Expected: GetItemCount 2, then 1 after the drop; GetChildrenOrder (ItemsSource and nodes in sync) is
        //   "Root0 | Root1".
        // Failure means: TreeViewItem::OnDrop no longer moves a root-level item into another item in both the
        //   node tree and the bound ItemsSource (TreeViewNodeVector with updateItemsSource).
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void TreeViewRootNodeBindingTest()
        {
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

        // Regression test for https://github.com/microsoft/microsoft-ui-xaml/issues/1182
        // Scenario: Content mode: add second-level items, drag Root.1.2 into Root.1.1, drag Root.1 into Root.0,
        //   expand Root.0/Root.1/Root.1.1, then collapse and re-expand Root.0.
        // Expected: GetChildrenOrder is "Root | Root.0 | Root.0.0 | Root.1 | Root.1.0 | Root.1.1 | Root.1.2 |
        //   Root.2" before and after the collapse/expand.
        // Failure means: Re-expanding a node with moved multi-level children re-syncs from ItemsSource wrongly
        //   (TreeViewNode::SyncChildrenNodesWithItemsSource / OnItemsSourceChanged), dropping nested items.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TreeViewWithMultiLevelChildrenExpandCollapseTest()
        {
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

        // Regression test for https://github.com/microsoft/microsoft-ui-xaml/issues/1790
        // Scenario: Content mode: ResetItemsSource assigns a new ItemsSource with Root expanded;
        //   ResetItemsSourceAsync builds one off the UI thread (Root and Root.1 expanded) and rebinds.
        // Expected: GetItemCount is 1, then 4, then 6.
        // Failure means: TreeView::OnPropertyChanged(ItemsSource) / ViewModel no longer rebuild the flattened
        //   view honoring IsExpanded of a newly assigned ItemsSource.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ItemsSourceResyncTest()
        {
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

        // Scenario: Content mode: SwapItemsSource to the 2-item collection, swap back to the original, then
        //   ExpandRootNode toggles the original root's IsExpanded.
        // Expected: GetItemCount is 2 after the first swap and 4 after swapping back and expanding.
        // Failure means: Switching back to a previously used ItemsSource leaves stale node/ItemsSource
        //   subscriptions, so its root no longer expands (TreeViewNode::ItemsSource / OnItemsSourceChanged).
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ItemsSourceSwitchForthAndBackTest()
        {
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

        // Scenario: Node mode: a button's Content is x:Bind TwoWay to TestTreeView.SelectedItem; set Content to
        //   Root.1's node, then SelectRoot2Item sets SelectedItem to Root.2.
        // Expected: ReadBindingResult (button content;SelectedItem) is "Root.1;Root.1", then "Root.2;Root.2".
        // Failure means: TreeView.SelectedItem no longer accepts sets from a two-way binding or no longer
        //   notifies the binding (TreeView::OnPropertyChanged(SelectedItem) -> ViewModel::SelectSingleItem).
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

        // Scenario: TreeViewUnrealizedChildrenTestPage (children created in Expanding, removed in Collapsed):
        //   tap the first item to select it, then tap its chevron to expand it.
        // Expected: SelectedItemName is "Item: 0; layer: 3" before and after the expansion.
        // Failure means: Inserting children realized during Expanding shifts the single selection to another
        //   item (ViewModel/TreeViewList selection index not adjusted).
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

        // Scenario: TreeViewItemsSourceTestPage (multi-select, TreeViewItem.ItemsSource bound in the template):
        //   UIA AddToSelection on Item 1 and Item 2, then Expand, Collapse, Expand Item 2.
        // Expected: No crash (no explicit assertion; uses UIA only).
        // Failure means: Expanding a selected item whose children come from TreeViewItem.ItemsSource crashes
        //   again (TreeViewItem::SetItemsSource / ViewModel selection propagation; bug 39499253).
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("Ignore", "False")] // Doesn't use input, so it's safe.
        [TestProperty("RegressionBug", "39499253")]
        public void CanExpandTreeViewItemWithItemSourceSet()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewItemsSourceTestPage" }))
            {
                UIObject item1 = FindElement.ByName("Item 1");
                UIObject item2 = FindElement.ByName("Item 2");

                item1.SetFocus();
                AutomationElement item1Peer = AutomationElement.FocusedElement;
                var item1SelectionItemPeer = (SelectionItemPattern)item1Peer.GetCurrentPattern(SelectionItemPattern.Pattern);

                item2.SetFocus();
                AutomationElement item2Peer = AutomationElement.FocusedElement;
                var item2SelectionItemPeer = (SelectionItemPattern)item2Peer.GetCurrentPattern(SelectionItemPattern.Pattern);
                var item2ExpandCollapseItemPeer = (ExpandCollapsePattern)item2Peer.GetCurrentPattern(ExpandCollapsePattern.Pattern);

                item1SelectionItemPeer.AddToSelection();
                item2SelectionItemPeer.AddToSelection();

                Wait.ForIdle();

                item2ExpandCollapseItemPeer.Expand();

                Wait.ForIdle();

                item2ExpandCollapseItemPeer.Collapse();

                Wait.ForIdle();

                // If the bug repros, the app will crash here and the test will fail. If we get no crash, we can say the test passes.
                item2ExpandCollapseItemPeer.Expand();

                Wait.ForIdle();
            }
        }

        // Scenario: TreeViewItemsSourceTestPage: SetUpContainerReuseTest1 binds 60 roots with children,
        //   ExpandSecondItem expands Root1; scroll to the end (recycling containers) and back to the top.
        // Expected: Root1's UIA ExpandCollapseState is Expanded before and after scrolling.
        // Failure means: Container recycling (TreeViewList::PrepareContainerForItemOverride /
        //   OnContainerContentChanging) writes a stale IsExpanded back to the node, collapsing it.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("Ignore", "False")]
        public void IsExpandedIsProperlyPreservedDuringItemRecycling()
        {
            using (var setup = new TestSetupHelper(new[] { "TreeView Tests", "TreeViewItemsSourceTestPage" }))
            {
                ClickButton("SetUpContainerReuseTest1");
                ClickButton("ExpandSecondItem");

                UIObject root1 = FindElement.ByName("Root1");
                Verify.IsNotNull(root1, "Verifying that we found a UIElement called Root1");

                root1.SetFocus();
                AutomationElement root1Peer = AutomationElement.FocusedElement;

                // Verify that root1 is expanded
                var root1ExpandCollapseItemPeer = (ExpandCollapsePattern)root1Peer.GetCurrentPattern(ExpandCollapsePattern.Pattern);
                Verify.AreEqual(ExpandCollapseState.Expanded, root1ExpandCollapseItemPeer.Current.ExpandCollapseState);

                ClickButton("ScrollToEndOfContentTreeView");
                Wait.ForIdle();
                Wait.ForSeconds(5);
                ClickButton("ScrollToTopOfContentTreeView");
                Wait.ForIdle();

                // Verify tree view item is still expanded
                ElementCache.Refresh();
                root1 = FindElement.ByName("Root1");
                Verify.IsNotNull(root1, "Verifying that we found a UIElement called Root1");

                root1.SetFocus();
                root1Peer = AutomationElement.FocusedElement;

                root1ExpandCollapseItemPeer = (ExpandCollapsePattern)root1Peer.GetCurrentPattern(ExpandCollapsePattern.Pattern);
                Verify.AreEqual(ExpandCollapseState.Expanded, root1ExpandCollapseItemPeer.Current.ExpandCollapseState);
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

        // Taps the item's multi-select check box. In the item template (TreeViewItem.xaml) the check box sits after the
        // item's 4px left presenter margin, the indentation (16px per level below the root, see
        // TreeViewItem::UpdateIndentation) and its own 10px left margin, and is 32px wide. Tap its center. These are XAML
        // effective pixels, while InputHelper.Tap offsets are physical pixels relative to the UIA bounding rectangle, so
        // scale by the test window's DPI (equal to the app's RasterizationScale), read at tap time so it is independent of
        // the current page and of app restarts. The former width-proportional offset (5% of the item width per level)
        // missed the box on narrower items.
        private void ToggleTreeViewItemCheckBox(UIObject item, int level)
        {
            const double itemLeftMargin = 4;
            const double indentationPerLevel = 16;
            const double checkBoxLeftMargin = 10;
            const double checkBoxWidth = 32;
            double scale = GetTestWindowScale();
            double x = (itemLeftMargin + indentationPerLevel * (level - 1) + checkBoxLeftMargin + checkBoxWidth / 2) * scale;
            InputHelper.Tap(item, x, item.BoundingRectangle.Height * .5);
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern uint GetDpiForWindow(IntPtr hwnd);

        // Verifies a UIA bounding-rectangle height (physical px) against an expected XAML height in DIPs, scaled by
        // the test window DPI and rounded, allowing +/-1 px for layout rounding.
        private static void VerifyHeightInDips(int actualPx, double expectedDips, string what)
        {
            double scale = GetTestWindowScale();
            int expectedPx = (int)Math.Round(expectedDips * scale);
            Log.Comment(what + ": height " + actualPx + " px, expected " + expectedDips + " DIPs * " + scale + " = " + expectedPx + " px (+/-1)");
            Verify.IsTrue(Math.Abs(actualPx - expectedPx) <= 1, what + " height " + actualPx + " px should be " + expectedPx + " px (+/-1)");
        }

        // DPI scale of the running test app's window (1.0 at 96 DPI). Falls back to 1.0 with a warning if the window
        // handle is not available.
        private static double GetTestWindowScale()
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            uint dpi = hwnd != IntPtr.Zero ? GetDpiForWindow(hwnd) : 0;
            if (dpi == 0)
            {
                Log.Warning("Could not read the test window DPI; assuming scale 1.");
                return 1.0;
            }
            double scale = dpi / 96.0;
            Log.Comment("Test window DPI " + dpi + " (scale " + scale + ")");
            return scale;
        }
    }
}
