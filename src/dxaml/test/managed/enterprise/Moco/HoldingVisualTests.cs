// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public class HoldingVisualTests : XamlTestsBase
    {
        private const uint HOLD_DURATION = 2000;
        private const double DRAG_VELOCITY_FACTOR = 0.1;
        private TimeSpan timeout = new TimeSpan(0, 1, 0);

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void VerifySingleSelectionNoDrag()
        {
            this.RunTest(
                false, /* canDrag */
                false, /* canReorder */
                false, /* isMultipleSelection */
                null, /* expectedPrimaryStates */
                null, /* expectedSecondaryStates */
                null /* expectedTargetStates */);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // No Shell in onecore for drag visuals.
        public void VerifySingleSelectionWithDrag()
        {
            Queue<string> expectedPrimaryStates = new Queue<string>(new[] { "NotDragging", "Dragging", "NotDragging", "Dragging", "DraggedPlaceholder" });
            Queue<string> expectedTargetStates = new Queue<string>(new[] { "NotDragging", "DraggingTarget", "NotDragging", "DraggingTarget" });
            this.RunTest(
                true, /* canDrag */
                false, /* canReorder */
                false, /* isMultipleSelection */
                expectedPrimaryStates,
                null, /* expectedSecondaryStates */
                expectedTargetStates);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // No Shell in onecore for drag visuals.
        [TestProperty("Hosting:Mode", "UAP")] // Investigate test failures from IXP 9/10 drop: VerifySingleSelectionWithReorder, VerifyMultipleSelectionWithReorder
        public void VerifySingleSelectionWithReorder()
        {
            Queue<string> expectedPrimaryStates = new Queue<string>(new[] { "NotDragging", "Reordering", "NotDragging", "Reordering", "Reordering", "ReorderedPlaceholder" });
            Queue<string> expectedTargetStates = new Queue<string>(new[] { "NotDragging", "ReorderingTarget", "NotDragging", "ReorderingTarget", "ReorderingTarget" });
            this.RunTest(
                false, /* canDrag */
                true, /* canReorder */
                false, /* isMultipleSelection */
                expectedPrimaryStates,
                null, /* expectedSecondaryStates */
                expectedTargetStates);
        }

        [TestMethod]
        public void VerifyMultipleSelectionNoDrag()
        {
            this.RunTest(
                false, /* canDrag */
                false, /* canReorder */
                true, /* isMultipleSelection */
                null, /* expectedPrimaryStates */
                null, /* expectedSecondaryStates */
                null /* expectedTargetStates */);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // No Shell in onecore for drag visuals.
        public void VerifyMultipleSelectionWithDrag()
        {
            Queue<string> expectedPrimaryStates = new Queue<string>(new[] { "NotDragging", "Dragging", "NotDragging", "Dragging", "MultipleDraggingPrimary", "DraggedPlaceholder" });
            Queue<string> expectedSecondaryStates = new Queue<string>(new[] { "NotDragging", "DraggingTarget", "NotDragging", "DraggingTarget", "MultipleDraggingSecondary" });
            Queue<string> expectedTargetStates = new Queue<string>(new[] { "NotDragging", "DraggingTarget", "NotDragging", "DraggingTarget" });
            this.RunTest(
                true, /* canDrag */
                false, /* canReorder */
                true, /* isMultipleSelection */
                expectedPrimaryStates,
                expectedSecondaryStates,
                expectedTargetStates);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] // No Shell in onecore for drag visuals.
        [TestProperty("Hosting:Mode", "UAP")] // Investigate test failures from IXP 9/10 drop: VerifySingleSelectionWithReorder, VerifyMultipleSelectionWithReorder
        public void VerifyMultipleSelectionWithReorder()
        {
            Queue<string> expectedPrimaryStates = new Queue<string>(new[] { "NotDragging", "Reordering", "NotDragging", "Reordering", "MultipleReorderingPrimary", "ReorderedPlaceholder" });
            Queue<string> expectedSecondaryStates = new Queue<string>(new[] { "NotDragging", "ReorderingTarget", "NotDragging", "ReorderingTarget", "ReorderingTarget" });
            Queue<string> expectedTargetStates = new Queue<string>(new[] { "NotDragging", "ReorderingTarget", "NotDragging", "ReorderingTarget", "ReorderingTarget" });
            this.RunTest(
                true, /* canDrag */
                true, /* canReorder */
                true, /* isMultipleSelection */
                expectedPrimaryStates,
                expectedSecondaryStates,
                expectedTargetStates);
        }

        #region Helpers

        private void RunTest(bool canDrag, bool canReorder, bool isMultipleSelection, Queue<string> expectedPrimaryStates, Queue<string> expectedSecondaryStates, Queue<string> expectedTargetStates)
        {
            Grid root = null;

            ListViewItem primary = null;
            VisualStateGroup primaryDragStates = null;
            AutoResetEvent primaryVerified = new AutoResetEvent(false);

            ListViewItem secondary = null;
            VisualStateGroup secondaryDragStates = null;
            AutoResetEvent secondaryVerified = new AutoResetEvent(false);

            ListViewItem target = null;
            VisualStateGroup targetDragStates = null;
            AutoResetEvent targetVerified = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                root = this.SetupTestListView(canDrag, canReorder, isMultipleSelection);
                primary = (ListViewItem)root.FindName("primary");
                if (isMultipleSelection)
                {
                    secondary = (ListViewItem)root.FindName("secondary");
                }
                target = (ListViewItem)root.FindName("target");
                TestServices.WindowHelper.WindowContent = root;
            });
            TestServices.WindowHelper.WaitForIdle();

            if (isMultipleSelection)
            {
                UIExecutor.Execute(() =>
                {
                    primary.IsSelected = true;
                    secondary.IsSelected = true;
                });
                TestServices.WindowHelper.WaitForIdle();
            }

            UIExecutor.Execute(() =>
            {
                Log.Comment("Setting up state change verifier for Primary.");
                primaryDragStates = this.GetDragStates(primary);
                primaryDragStates.CurrentStateChanged += (s, e) =>
                {
                    Log.Comment("Primary State Changed: {0}->{1}", e.OldState.Name, e.NewState.Name);

                    if (!canDrag && !canReorder)
                    {
                        Verify.Fail("Expected NO state change on a ListView that cannot drag and cannot reorder. But, the test caught a state change for Primary!!!");
                    }

                    if (expectedPrimaryStates.Count > 1)
                    {
                        Verify.AreEqual(expectedPrimaryStates.Dequeue(), e.OldState.Name);
                        Verify.AreEqual(expectedPrimaryStates.Peek(), e.NewState.Name);
                    }
                    else
                    {
                        Log.Comment("All state changes verified for Primary.");
                        primaryVerified.Set();
                    }
                };
                Log.Comment("Handler has been set up for Primary DragStates, now listening for state changes.");

                if (isMultipleSelection)
                {
                    Log.Comment("Setting up state change verifier for Secondary.");
                    secondaryDragStates = this.GetDragStates(secondary);
                    secondaryDragStates.CurrentStateChanged += (s, e) =>
                    {
                        Log.Comment("Secondary State Changed: {0}->{1}", e.OldState.Name, e.NewState.Name);

                        if (!canDrag && !canReorder)
                        {
                            Verify.Fail("Expected NO state change on a ListView that cannot drag and cannot reorder. But, the test caught a state change for Secondary!!!");
                        }

                        if (expectedSecondaryStates.Count > 1)
                        {
                            Verify.AreEqual(expectedSecondaryStates.Dequeue(), e.OldState.Name);
                            Verify.AreEqual(expectedSecondaryStates.Peek(), e.NewState.Name);
                        }
                        else
                        {
                            Log.Comment("All state changes verified for Secondary.");
                            secondaryVerified.Set();
                        }
                    };
                    Log.Comment("Handler has been set up for Secondary DragStates, now listening for state changes.");
                }

                Log.Comment("Setting up state change verifier for Target.");
                targetDragStates = this.GetDragStates(target);
                targetDragStates.CurrentStateChanged += (s, e) =>
                {
                    Log.Comment("Target State Changed: {0}->{1}", e.OldState.Name, e.NewState.Name);

                    if (!canDrag && !canReorder)
                    {
                        Verify.Fail("Expected NO state change on a ListView that cannot drag and cannot reorder. But, the test caught a state change for Target!!!");
                    }

                    if (expectedTargetStates.Count > 1)
                    {
                        Verify.AreEqual(expectedTargetStates.Dequeue(), e.OldState.Name);
                        Verify.AreEqual(expectedTargetStates.Peek(), e.NewState.Name);
                    }
                    else
                    {
                        Log.Comment("All state changes verified for Target.");
                        targetVerified.Set();
                    }
                };
                Log.Comment("Handler has been set up for Target DragStates, now listening for state changes.");
            });
            TestServices.WindowHelper.WaitForIdle();

            // Scenario 1: Press, Hold, and Release
            TestServices.InputHelper.Hold(primary, HOLD_DURATION);
            TestServices.WindowHelper.WaitForIdle();

            // Scenario 2: Press, Hold, and Drag/Reorder
            if (canReorder)
            {
                TestServices.InputHelper.PressHoldAndPanFromCenter(primary, 0, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
            }
            else if (canDrag)
            {
                TestServices.InputHelper.PressHoldAndPanFromCenter(primary, 100, 100, DRAG_VELOCITY_FACTOR, HOLD_DURATION);
                TestServices.WindowHelper.WaitForIdle();
            }

            if (canDrag || canReorder)
            {
                primaryVerified.WaitOne(this.timeout);
                if (isMultipleSelection)
                {
                    secondaryVerified.WaitOne(this.timeout);
                }
                targetVerified.WaitOne(this.timeout);
            }
        }

        private Grid SetupTestListView(bool canDrag, bool canReorder, bool isMultipleSelection)
        {
            string xamlFormat = @"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>
    <ListView x:Name='list' Width='200' HorizontalAlignment='Center' VerticalAlignment='Center' ItemContainerStyle='{{ThemeResource ListViewItemExpanded}}' {0} {1} {2}>
        <ListViewItem x:Name='primary' Background='Gray'>Primary</ListViewItem>
        <ListViewItem x:Name='target' Background='Aqua'>Target</ListViewItem>
        {3}
    </ListView>
</Grid>
";
            string xaml = string.Format(
                xamlFormat,
                canDrag ? "CanDragItems='true'" : string.Empty,
                canReorder ? "CanReorderItems='true'" : string.Empty,
                isMultipleSelection ? "SelectionMode='Extended'" : string.Empty,
                isMultipleSelection ? "<ListViewItem x:Name='secondary' Background='Green'>Secondary</ListViewItem>" : string.Empty);

            Log.Comment("Test ListView XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        private VisualStateGroup GetDragStates(ListViewItem item)
        {
            VisualStateGroup dragStates = null;

            FrameworkElement itemTemplate = (FrameworkElement)VisualTreeHelper.GetChild(item, 0);
            Log.Comment("Item template root: {0}", itemTemplate.Name);

            var groups = VisualStateManager.GetVisualStateGroups(itemTemplate);
            Verify.IsNotNull(groups, "Cannot find visual state groups!!!");
            Verify.IsGreaterThan(groups.Count, 0, "Groups count was not greater than 0!!!");

            foreach (VisualStateGroup g in groups)
            {
                Log.Comment("Group Name: {0}", g.Name);
                if (g.Name.Equals("DragStates", StringComparison.OrdinalIgnoreCase))
                {
                    Log.Comment("Found Drag States.");
                    dragStates = g;
                    break;
                }
            }
            Verify.IsNotNull(dragStates, "Cannot find drag states group!!!");

            return dragStates;
        }

        #endregion
    }
}
