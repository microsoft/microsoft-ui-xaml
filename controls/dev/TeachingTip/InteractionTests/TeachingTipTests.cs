// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using System.Numerics;
using Common;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Microsoft.Windows.Apps.Test.Foundation;

using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.TeachingTipTestPageElements;
using System.Drawing;
using System.Threading;
using System.Diagnostics;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TeachingTipTests
    {
        // The longest observed animated view change took 5.4 seconds, so 9 seconds is picked
        // as the default timeout so there is a reasonable margin for reliability.
        const double defaultAnimatedViewChangeTimeout = 9000;

        TeachingTipTestPageElements elements;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("IsolationLevel", "Method")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        public TestContext TestContext { get; set; }

        // Scenario: for the tip in resources and in the visual tree, close it programmatically, with the header X,
        //           programmatically with light dismiss on, by light dismiss (click outside, then Esc), and with the
        //           footer close button.
        // Expected: the logged Closing/Closed events report Programmatic, CloseButton (after "Close Button Clicked"),
        //           Programmatic, LightDismiss, LightDismiss and CloseButton, in order.
        // A failure means: apps get the wrong TeachingTipCloseReason or the events in the wrong order.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CloseReasonIsAccurate()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    var message0 = GetTeachingTipDebugMessage(0);
                    var message1 = GetTeachingTipDebugMessage(1);
                    Verify.IsTrue(message0.ToString().Contains("Closing"));
                    Verify.IsTrue(message0.ToString().Contains("Programmatic"));
                    Verify.IsTrue(message1.ToString().Contains("Closed"));
                    Verify.IsTrue(message1.ToString().Contains("Programmatic"));

                    SetHeroContent(HeroContentOptions.NoContent);
                    OpenTeachingTip();
                    PressXCloseButton();
                    var message2 = GetTeachingTipDebugMessage(2);
                    var message3 = GetTeachingTipDebugMessage(3);
                    var message4 = GetTeachingTipDebugMessage(4);
                    Verify.IsTrue(message2.ToString().Contains("Close Button Clicked"));
                    Verify.IsTrue(message3.ToString().Contains("Closing"));
                    Verify.IsTrue(message3.ToString().Contains("CloseButton"));
                    Verify.IsTrue(message4.ToString().Contains("Closed"));
                    Verify.IsTrue(message4.ToString().Contains("CloseButton"));

                    EnableLightDismiss(true);
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    var message5 = GetTeachingTipDebugMessage(5);
                    var message6 = GetTeachingTipDebugMessage(6);
                    Verify.IsTrue(message5.ToString().Contains("Closing"));
                    Verify.IsTrue(message5.ToString().Contains("Programmatic"));
                    Verify.IsTrue(message6.ToString().Contains("Closed"));
                    Verify.IsTrue(message6.ToString().Contains("Programmatic"));

                    OpenTeachingTip();
                    CloseTeachingTipByLightDismiss(useEscKey: false);
                    var message7 = GetTeachingTipDebugMessage(7);
                    var message8 = GetTeachingTipDebugMessage(8);
                    Verify.IsTrue(message7.ToString().Contains("Closing"));
                    Verify.IsTrue(message7.ToString().Contains("LightDismiss"));
                    Verify.IsTrue(message8.ToString().Contains("Closed"));
                    Verify.IsTrue(message8.ToString().Contains("LightDismiss"));

                    OpenTeachingTip();
                    CloseTeachingTipByLightDismiss(useEscKey: true);
                    var message9 = GetTeachingTipDebugMessage(9);
                    var message10 = GetTeachingTipDebugMessage(10);
                    Verify.IsTrue(message9.ToString().Contains("Closing"));
                    Verify.IsTrue(message9.ToString().Contains("LightDismiss"));
                    Verify.IsTrue(message10.ToString().Contains("Closed"));
                    Verify.IsTrue(message10.ToString().Contains("LightDismiss"));

                    SetCloseButtonContent(CloseButtonContentOptions.ShortText);
                    OpenTeachingTip();
                    PressTipCloseButton();
                    var message11 = GetTeachingTipDebugMessage(11);
                    var message12 = GetTeachingTipDebugMessage(12);
                    var message13 = GetTeachingTipDebugMessage(13);
                    Verify.IsTrue(message11.ToString().Contains("Close Button Clicked"));
                    Verify.IsTrue(message12.ToString().Contains("Closing"));
                    Verify.IsTrue(message12.ToString().Contains("CloseButton"));
                    Verify.IsTrue(message13.ToString().Contains("Closed"));
                    Verify.IsTrue(message13.ToString().Contains("CloseButton"));

                    ClearTeachingTipDebugMessages();
                }
            }
        }

        // Scenario: open a targeted tip and remove its target button from the tree.
        // Expected: the tip's content is unloaded (the page's "content unloaded" check box turns on).
        // A failure means: a tip stays open after its target is removed from the page.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TargetUnloadingClosesTeachingTip()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                elements.GetSetTargetButton().InvokeAndWait();
                OpenTeachingTip();

                CheckBox unloadedCheckbox = elements.GetTeachingTipContentUnloadedCheck();
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.Off);

                // Removing target button from visual tree
                Button remove = elements.GetRemoveOpenButtonFromVisualTreeButton();
                remove.InvokeAndWait();

                // Target unloaded, TeachingTip must do the same
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.On);
            }
        }

        // Scenario: set and then remove the tip's target, open it, and remove the former target button from the tree.
        // Expected: the tip's content stays loaded.
        // A failure means: a tip closes because an element it no longer targets was unloaded.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void PreviousTargetUnloadingLeavesTeachingTipOpen()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                elements.GetSetTargetButton().InvokeAndWait();
                elements.GetRemoveTargetButton().InvokeAndWait();
                OpenTeachingTip();

                CheckBox unloadedCheckbox = elements.GetTeachingTipContentUnloadedCheck();
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.Off);


                // Removing target button from visual tree
                Button remove = elements.GetRemoveOpenButtonFromVisualTreeButton();
                remove.InvokeAndWait();

                // We expect the teaching tip to still be upon since it has no target
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.Off);
            }
        }

        // Scenario: open a tip and remove the TeachingTip itself from the tree.
        // Expected: its content is unloaded and IsOpen turns off.
        // A failure means: removing a TeachingTip leaves its popup on screen.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TeachingTipRemovalClosesPopup()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                ScrollTargetIntoView();
                OpenTeachingTip();

                CheckBox unloadedCheckbox = elements.GetTeachingTipContentUnloadedCheck();
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.Off);

                // Finding the button to remove the teaching tip
                Button removeButton = elements.GetRemoveTeachingTipButton();

                // Removing teaching tip
                
                removeButton.InvokeAndWait();
                Verify.IsTrue(unloadedCheckbox.ToggleState == ToggleState.On);
                Verify.IsTrue(elements.GetIsOpenCheckBox().ToggleState == ToggleState.Off);
            }
        }

        // Scenario: for both tip locations, open the tip and scroll the target by 10, -20 and 10 pixels with the
        //           TipFollowsTarget hook off, on, then off.
        // Expected: when off, the tip's vertical offset does not change; when on, it moves up and down with the target
        //           and returns exactly to its original offset; after turning it off again it stays put.
        // A failure means: tips drift when they should not follow the target, or do not follow it when they should.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void TipCanFollowTarget()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    Wait.ForIdle();
                    OpenTeachingTip();
                    double initialTipVerticalOffset = GetTipVerticalOffset();
                    double initialScrollViewerVerticalOffset = GetScrollViewerVerticalOffset();

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should not follow the target by default");
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should not follow the target by default");
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should not follow the target by default");

                    SetTipFollowsTarget(true);

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Verify.IsLessThan(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Verify.IsGreaterThan(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should return to its original position with the target");

                    SetTipFollowsTarget(false);

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should stop following the target");
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should stop following the target");
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Verify.AreEqual(initialTipVerticalOffset, GetTipVerticalOffset(), "Tip should stop following the target");
                }
            }
        }

        // Scenario: Ignored (#2219, unreliable): scroll, then unmaximize and maximize the window with Win+Down/Win+Up,
        //           and repeat for a tip on the window edge.
        // Expected: the tip's vertical offset decreases after the resize; the edge tip's horizontal offset decreases
        //           after restoring and increases after maximizing. The API test OpenTipsRepositionWhenWindowIsResized
        //           covers this reliably.
        // A failure means: open tips are not repositioned when the window is resized.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Ignore", "True")] // #2219 Unreliable test: TeachingTipTests.TipFollowsTargetOnWindowResize 
        public void TipFollowsTargetOnWindowResize()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    Wait.ForIdle();
                    OpenTeachingTip();
                    double initialTipVerticalOffset = GetTipVerticalOffset();
                    double initialScrollViewerVerticalOffset = GetScrollViewerVerticalOffset();

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);

                    //Unmaximize then maximize the window, to force a window size changed event.
                    KeyboardHelper.PressKey(Key.Down, ModifierKey.Windows);
                    Wait.ForIdle();
                    KeyboardHelper.PressKey(Key.Up, ModifierKey.Windows);
                    Wait.ForIdle();

                    Verify.IsLessThan(GetTipVerticalOffset(), initialTipVerticalOffset);
                }

                // Test for bug #1547
                // Maximize window first.
                var getOnEdgeOffsetButton = elements.GetTeachingTipOnEdgeOffsetButton();
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Windows);
                Wait.ForIdle();

                // Open TeachingTip
                elements.GetOpenTeachingTipOnEdgeButton().InvokeAndWait();
                
                // Get offset values
                getOnEdgeOffsetButton.InvokeAndWait();
                double oldXOffset = elements.GetTeachingTipOnEdgeHorizontalOffset();

                // "Restore" window width (aka unminimize)
                KeyboardHelper.PressKey(Key.Down, ModifierKey.Windows);
                getOnEdgeOffsetButton.InvokeAndWait();
                Verify.IsLessThan(elements.GetTeachingTipOnEdgeHorizontalOffset(), oldXOffset);
                
                // Update values
                getOnEdgeOffsetButton.InvokeAndWait();
                oldXOffset = elements.GetTeachingTipOnEdgeHorizontalOffset();

                // Maximize again
                KeyboardHelper.PressKey(Key.Up, ModifierKey.Windows);
                getOnEdgeOffsetButton.InvokeAndWait();
                Verify.IsGreaterThan(elements.GetTeachingTipOnEdgeHorizontalOffset(), oldXOffset);
            }
        }

        // Scenario: Ignored (#3125): with Auto placement, shrink the test window (and then screen) bounds around the
        //           target step by step, also for an out-of-root tip with ReturnTopForOutOfWindowPlacement on and off.
        // Expected: the chosen placement goes through Top, Bottom, LeftTop, LeftBottom, RightBottom, RightTop, TopLeft,
        //           TopRight, BottomLeft, BottomRight, Center, Left and Right; out-of-root with the shortcut on always
        //           gives Top.
        // A failure means: the Auto placement order changed.
        [TestMethod] 
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // Disabled as per tracking issue #3125
        public void AutoPlacement()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    var targetRect = GetTargetBounds();
                    TestAutoPlacementForWindowOrScreenBounds(targetRect, true);
                    
                    SetShouldConstrainToRootBounds(false);
                    TestAutoPlacementForWindowOrScreenBounds(targetRect, false, "Top");

                    SetReturnTopForOutOfWindowPlacement(false);
                    TestAutoPlacementForWindowOrScreenBounds(targetRect, false);

                    SetReturnTopForOutOfWindowPlacement(true);
                }
            }
        }

        // Scenario: Ignored. Data-driven over tip location and 5 cases: with test window bounds around the target, all
        //           sides available (case 1) or no room on the left, top, right or bottom (cases 2-5); set each
        //           preferred placement.
        // Expected: case 1 honors every preference; cases 2-5 move preferences that need the missing side to the
        //           expected fallback placement.
        // A failure means: preferred placements are not honored, or fall back to the wrong side.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")]
        [TestProperty("data:TipLocationOptions", "{ResourceDictionary, VisualTree}")]
        [TestProperty("data:TestCase", "{1, 2, 3, 4, 5}")]
        public void SpecifiedPlacement()
        {
            var tipLocationOptionsStr = this.TestContext.DataRow["TipLocationOptions"] as string;
            TipLocationOptions location = Enum.Parse<TipLocationOptions>(tipLocationOptionsStr);

            var testCaseStr = this.TestContext.DataRow["TestCase"] as string;

            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                SetTeachingTipLocation(location);

                ScrollTargetIntoView();
                ScrollBy(10);

                SetHeroContent(HeroContentOptions.NoContent);

                var targetRect = GetTargetBounds();

                // The following might not always work, so repeat.
                UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);
                
                switch(testCaseStr)
                {
                    case "1":
                        {
                            // All positions are valid
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "2":
                        {
                            // Eliminate left of the target
                            UseTestBounds(targetRect.W - 120, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "3":
                        {
                            // Eliminate top of the target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 1, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "4":
                        {
                            // Eliminate right of the target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 500, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Left");
                        }
                        break;
                    case "5":
                        {
                            // Eliminate bottom of target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 501, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    default:
                        {
                            Verify.Fail($"Unknown TestCase {testCaseStr}");
                        }
                        break;
                }
            }
        }

        // Scenario: Ignored. Same data-driven cases as SpecifiedPlacement with the page in RightToLeft.
        // Expected: the same expectations with horizontal placements mirrored.
        // A failure means: preferred placements are not mirrored or fall back to the wrong side in right-to-left apps.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")]
        [TestProperty("data:TipLocationOptions", "{ResourceDictionary, VisualTree}")]
        [TestProperty("data:TestCase", "{1, 2, 3, 4, 5}")]
        public void SpecifiedPlacementRTL()
        {
            var tipLocationOptionsStr = this.TestContext.DataRow["TipLocationOptions"] as string;
            TipLocationOptions location = Enum.Parse<TipLocationOptions>(tipLocationOptionsStr);

            var testCaseStr = this.TestContext.DataRow["TestCase"] as string;

            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                SetTeachingTipLocation(location);

                ScrollTargetIntoView();
                ScrollBy(10);

                elements.GetPageRTLCheckbox().Check();

                SetHeroContent(HeroContentOptions.NoContent);

                var targetRect = GetTargetBounds();

                // The following might not always work, so repeat.
                UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                switch (testCaseStr)
                {
                    case "1":
                        {
                            // All positions are valid
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "2":
                        {
                            // Eliminate left of the target
                            UseTestBounds(targetRect.W - 120, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "3":
                        {
                            // Eliminate top of the target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 1, targetRect.Y + 1000, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("Bottom");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("BottomRight");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    case "4":
                        {
                            // Eliminate right of the target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 500, targetRect.Z + 1000, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("BottomLeft");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Left");
                        }
                        break;
                    case "5":
                        {
                            // Eliminate bottom of target
                            UseTestBounds(targetRect.W - 500, targetRect.X - 500, targetRect.Y + 1000, targetRect.Z + 501, targetRect, true);

                            SetPreferredPlacement(PlacementOptions.Top);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Bottom);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.Left);
                            VerifyPlacement("Right");
                            SetPreferredPlacement(PlacementOptions.Right);
                            VerifyPlacement("Left");
                            SetPreferredPlacement(PlacementOptions.TopRight);
                            VerifyPlacement("TopLeft");
                            SetPreferredPlacement(PlacementOptions.TopLeft);
                            VerifyPlacement("TopRight");
                            SetPreferredPlacement(PlacementOptions.BottomRight);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.BottomLeft);
                            VerifyPlacement("Top");
                            SetPreferredPlacement(PlacementOptions.LeftTop);
                            VerifyPlacement("RightTop");
                            SetPreferredPlacement(PlacementOptions.LeftBottom);
                            VerifyPlacement("RightBottom");
                            SetPreferredPlacement(PlacementOptions.RightTop);
                            VerifyPlacement("LeftTop");
                            SetPreferredPlacement(PlacementOptions.RightBottom);
                            VerifyPlacement("LeftBottom");
                            SetPreferredPlacement(PlacementOptions.Center);
                            VerifyPlacement("Center");
                        }
                        break;
                    default:
                        {
                            Verify.Fail($"Unknown TestCase {testCaseStr}");
                        }
                        break;
                }

                elements.GetPageRTLCheckbox().Uncheck();                
            }
        }


        // Scenario: for both tip locations, open and close a tip without an icon, then remove the icon from an open
        //           tip.
        // Expected: the tip opens without an icon, closes, and stays open when its icon is removed.
        // A failure means: a tip without an icon cannot open, or removing the icon closes or crashes it.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NoIconDoesNotCrash()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);

                    SetIcon(IconOptions.NoIcon);
                    OpenTeachingTip();
                    Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "A tip without an icon should open");
                    CloseTeachingTipProgrammatically();
                    Verify.AreEqual(ToggleState.Off, elements.GetIsOpenCheckBox().ToggleState);
                    SetIcon(IconOptions.People);
                    OpenTeachingTip();
                    SetIcon(IconOptions.NoIcon);
                    Wait.ForIdle();
                    Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "Removing the icon must not close an open tip");
                    CloseTeachingTipProgrammatically();
                }
            }
        }


        // Scenario: for both tip locations, toggle ShouldConstrainToRootBounds while the tip is closed and while it is
        //           open.
        // Expected: an out-of-root tip uses Top for the Auto placement; changing the setting while open does not close
        //           the tip, and it applies after the tip reopens.
        // A failure means: switching between in-window and out-of-root tips closes an open tip or is never applied.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CanSwitchShouldConstrainToRootBounds()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    SetShouldConstrainToRootBounds(false);
                    OpenTeachingTip();
                    // Out-of-root tips skip the space checks and use Top for the Auto placement.
                    Verify.AreEqual("Top", GetEffectivePlacement(), "Unconstrained tip placement");
                    CloseTeachingTipProgrammatically();
                    SetShouldConstrainToRootBounds(true);
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    OpenTeachingTip();
                    SetShouldConstrainToRootBounds(false);
                    Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "Changing ShouldConstrainToRootBounds must not close an open tip");
                    CloseTeachingTipProgrammatically();
                    OpenTeachingTip();
                    Verify.AreEqual("Top", GetEffectivePlacement(), "The new setting applies when the tip reopens");
                    SetShouldConstrainToRootBounds(true);
                    CloseTeachingTipProgrammatically();
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                }
            }
        }


        // Scenario: for both tip locations, show the tip inside a 10x10 test window, then as an out-of-root tip with a
        //           10x10 test screen.
        // Expected: the in-window tip does not open (the log shows Closed with reason Programmatic); the out-of-root
        //           tip opens with the Top placement.
        // A failure means: tips that cannot fit are shown, or out-of-root tips are refused.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TipsWhichDoNotFitDoNotOpen()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    UseTestWindowBounds(10, 10, 10, 10);


                    elements.GetShowButton().InvokeAndWait();

                    var message1 = GetTeachingTipDebugMessage(1);
                    Verify.IsTrue(message1.ToString().Contains("Closed"));
                    Verify.IsTrue(message1.ToString().Contains("Programmatic"));

                    UseTestScreenBounds(10, 10, 10, 10);
                    SetShouldConstrainToRootBounds(false);

                    OpenTeachingTip();

                    VerifyPlacement("Top");

                    ClearTeachingTipDebugMessages();
                }
            }
        }


        // Scenario: for both tip locations, open a tip with an action button and switch its theme Light, Dark, Light.
        // Expected: the button and content text are black, then white, then black (alpha ignored).
        // A failure means: tip content does not follow the requested theme, leaving text unreadable.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void VerifyTheming()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    SetActionButtonContentTo("Small text");

                    ScrollTargetIntoView();
                    OpenTeachingTip();

                    var themingComboBox = elements.GetThemingComboBox();
                    themingComboBox.SelectItemByName("Light");

                    // Substring removes the opacity of the color, as that may vary
                    Verify.AreEqual("000000", elements.GetEffectiveForegroundOfTeachingTipButtonTextBlock().GetText().Substring(3), "Default button foreground should be black");
                    Verify.AreEqual("000000", elements.GetEffectiveForegroundOfTeachingTipContentTextBlock().GetText().Substring(3), "Default content foreground should be black");

                    // Change to Dark, make sure the font switches to light
                    themingComboBox.SelectItemByName("Dark");

                    Verify.AreEqual("FFFFFF", elements.GetEffectiveForegroundOfTeachingTipButtonTextBlock().GetText().Substring(3), "Default button foreground should be white");
                    Verify.AreEqual("FFFFFF", elements.GetEffectiveForegroundOfTeachingTipContentTextBlock().GetText().Substring(3), "Default content foreground should be white");

                    // Change to Light, make sure the font switches to dark
                    themingComboBox.SelectItemByName("Light");

                    Verify.AreEqual("000000", elements.GetEffectiveForegroundOfTeachingTipButtonTextBlock().GetText().Substring(3), "Default button foreground should be black");
                    Verify.AreEqual("000000", elements.GetEffectiveForegroundOfTeachingTipContentTextBlock().GetText().Substring(3), "Default content foreground should be black");
                }
            }
        }

        // Scenario: Ignored (#1769): open a tip and set Title, then Subtitle, to an empty string.
        // Expected: both are Visible at first; an empty Title collapses only the title, and an empty Subtitle collapses
        //           the subtitle too.
        // A failure means: empty title/subtitle lines are still shown.
        [TestMethod] //Disabled with issue #1769
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")]
        public void SettingTitleOrSubtitleToEmptyStringCollapsesTextBox()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);
                    ScrollTargetIntoView();
                    OpenTeachingTip();
                    Verify.AreEqual("Visible", elements.GetTitleVisibilityTextBlock().GetText());
                    Verify.AreEqual("Visible", elements.GetSubtitleVisibilityTextBlock().GetText());
                    SetTitle(TitleContentOptions.No);
                    Verify.AreEqual("Collapsed", elements.GetTitleVisibilityTextBlock().GetText());
                    Verify.AreEqual("Visible", elements.GetSubtitleVisibilityTextBlock().GetText());
                    SetSubtitle(SubtitleContentOptions.No);
                    Verify.AreEqual("Collapsed", elements.GetTitleVisibilityTextBlock().GetText());
                    Verify.AreEqual("Collapsed", elements.GetSubtitleVisibilityTextBlock().GetText());
                }
            }
        }

        // Scenario: open a page that declares a TeachingTip in XAML with IsOpen=true, then invoke its close button.
        // Expected: the tip is open on load, its content and close button are found through UI Automation, and invoking
        //           the button closes the tip and it goes idle.
        // A failure means: tips declared open in XAML do not open on load or cannot be closed.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void TestTeachingTipInXamlPage()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTipInXamlPage Test" }))
            {
                var elements = new TeachingTipTestPageElements();
                Wait.ForIdle();

                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "isOpenCheckBox.ToggleState");

                var teachingTipContent = FindElement.ByName<TextBlock>("TeachingTipContent");
                Verify.IsNotNull(teachingTipContent, "teachingTipContent");

                var teachingTipCloseButton = FindElement.ByName<Button>("TeachingTipCloseButton");
                Verify.IsNotNull(teachingTipCloseButton, "teachingTipCloseButton");

                teachingTipCloseButton.Invoke();

                WaitForUnchecked(elements.GetIsOpenCheckBox());
                WaitForChecked(elements.GetIsIdleCheckBox());
                Wait.ForIdle();
            }
        }

        private void TestAutoPlacementForWindowOrScreenBounds(Vector4 targetRect, bool forWindowBounds)
        {
            TestAutoPlacementForWindowOrScreenBounds(targetRect, forWindowBounds, null);
        }

        private void TestAutoPlacementForWindowOrScreenBounds(Vector4 targetRect, bool forWindowBounds, string valueOverride)
        {
            Log.Comment($"TestAutoPlacementForWindowOrScreenBounds {targetRect}, {forWindowBounds}, {valueOverride}");
            UseTestBounds(targetRect.W - 329, targetRect.X - 340, targetRect.Y + 656, targetRect.Z + 680, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "Top");
            UseTestBounds(targetRect.W - 329, targetRect.X - 336, targetRect.Y + 656, targetRect.Z + 680, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "Bottom");
            UseTestBounds(targetRect.W - 329, targetRect.X - 318, targetRect.Y + 659, targetRect.Z + 640, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "LeftTop");
            UseTestBounds(targetRect.W - 329, targetRect.X - 100, targetRect.Y + 659, targetRect.Z + 403, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "LeftBottom");
            UseTestBounds(targetRect.W - 327, targetRect.X - 100, targetRect.Y + 659, targetRect.Z + 403, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "RightBottom");
            UseTestBounds(targetRect.W - 327, targetRect.X - 300, targetRect.Y + 659, targetRect.Z + 603, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "RightTop");
            UseTestBounds(targetRect.W - 327, targetRect.X - 340, targetRect.Y + 349, targetRect.Z + 608, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "TopLeft");
            UseTestBounds(targetRect.W - 20, targetRect.X - 340, targetRect.Y + 348, targetRect.Z + 608, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "TopRight");
            UseTestBounds(targetRect.W - 327, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 444, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "BottomLeft");
            UseTestBounds(targetRect.W - 20, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 444, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "BottomRight");
            UseTestBounds(targetRect.W - 327, targetRect.X - 318, targetRect.Y + 650, targetRect.Z + 444, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "Center");

            // Remove the hero content;
            SetHeroContent(HeroContentOptions.NoContent);

            UseTestBounds(targetRect.W - 329, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 20, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "Left");
            UseTestBounds(targetRect.W - 19, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 20, targetRect, forWindowBounds);
            VerifyPlacement(valueOverride ?? "Right");

            SetHeroContent(HeroContentOptions.RedSquare);
        }

        private void VerifyPlacement(String placement)
        {
            OpenTeachingTip();
            Verify.AreEqual(placement, GetEffectivePlacement(), "VerifyPlacement");
            CloseTeachingTipProgrammatically();
        }

        // Scenario: for both tip locations, open the tip, clear its AutomationProperties.Name and set it again.
        // Expected: the popup's UIA name is the tip's name, falls back to the title ("We've Added Auto Saving!") when
        //           cleared, and returns to the name when set again.
        // A failure means: screen readers announce the tip popup with a missing or stale name.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void AutomationNameIsForwardedToPopup()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                foreach(TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    OpenTeachingTip();
                    Verify.IsNotNull(FindElement.ByNameAndClassName(location == TipLocationOptions.VisualTree ? "TeachingTipInVisualTree" : "TeachingTipInResources", "Popup"));
                    SetAutomationName(AutomationNameOptions.None);
                    Verify.IsNotNull(FindElement.ByNameAndClassName("We've Added Auto Saving!", "Popup"));
                    SetAutomationName(location == TipLocationOptions.VisualTree ? AutomationNameOptions.VisualTree : AutomationNameOptions.Resources);
                    Verify.IsNotNull(FindElement.ByNameAndClassName(location == TipLocationOptions.VisualTree ? "TeachingTipInVisualTree" : "TeachingTipInResources", "Popup"),
                        "Restoring the explicit name should be forwarded to the popup");
                    CloseTeachingTipProgrammatically();
                }
            }
        }

        // Scenario: Ignored: open a tip and close and reopen it using only the keyboard (F6 then Enter), with and
        //           without CloseButtonContent, and use F6 and Tab to return to the page and close it.
        // Expected: each keyboard sequence closes the tip.
        // A failure means: keyboard users cannot reach and use the tip's close button with F6.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        [TestProperty("Ignore", "True")]
        public void F6PutsFocusOnCloseButton()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                ScrollTargetIntoView();
                ScrollBy(10);
                OpenTeachingTip();
                CloseOpenAndCloseWithJustKeyboardViaF6();
                SetCloseButtonContent(CloseButtonContentOptions.ShortText);
                OpenTeachingTip();
                CloseOpenAndCloseWithJustKeyboardViaF6();
                OpenTeachingTip();
                UseF6ToReturnToTestPageToCloseTip();
                SetCloseButtonContent(CloseButtonContentOptions.NoText);
            }
        }

        // Scenario: on the focus page, put focus on the open button and press Tab with the tip closed, open, and closed
        //           with light dismiss enabled.
        // Expected: focus stays on the open button each time.
        // A failure means: a TeachingTip adds itself to the page's tab order.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VerifyTeachingTipNotIncludedInTabOrder()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTipFocus Test" }))
            {
                var elements = new TeachingTipFocusTestPageElements();
                var openTeachingTipButton = elements.GetOpenButton();

                FocusHelper.SetFocus(openTeachingTipButton);

                Log.Comment("Verify open button has keyboard focus.");
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                Log.Comment("Verify that a closed teaching tip does not appear in tab order.");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                Log.Comment("Verify that an opened teaching tip does not appear in tab order.");
                OpenTeachingTip();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                Log.Comment("Switch to light-dismissable teaching tip.");
                CloseTeachingTipProgrammatically();
                EnableLightDismiss();

                Log.Comment("Verify that a closed light-dismissable teaching tip does not appear in tab order.");

                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                void OpenTeachingTip()
                {
                    if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.On)
                    {
                        openTeachingTipButton.InvokeAndWait();
                        WaitForChecked(elements.GetIsOpenCheckBox());
                        WaitForChecked(elements.GetIsIdleCheckBox());
                    }
                }

                void CloseTeachingTipProgrammatically()
                {
                    if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
                    {
                        elements.GetCloseButton().InvokeAndWait();
                        WaitForUnchecked(elements.GetIsOpenCheckBox());
                        WaitForChecked(elements.GetIsIdleCheckBox());
                    }
                }

                void EnableLightDismiss()
                {
                    var isLightDismissEnabledCheckBox = elements.GetIsLightDismissEnabledCheckBox();
                    if (isLightDismissEnabledCheckBox.ToggleState != ToggleState.On)
                    {
                        isLightDismissEnabledCheckBox.Check();
                        WaitForChecked(isLightDismissEnabledCheckBox);
                    }
                }
            }
        }

        // Scenario: open a tip with short action and close button content.
        // Expected: the UIA names of ActionButton and CloseButton are "A:Short Text." and "C:Short Text.".
        // A failure means: screen readers announce the tip buttons without their text.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VerifyTeachingTipButtonsNameAutomationProperty()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                SetActionButtonContentTo("Small text");
                SetCloseButtonContent(CloseButtonContentOptions.ShortText);

                OpenTeachingTip();

                var actionButton = FindElement.ById("ActionButton");
                var closeButton = FindElement.ById("CloseButton");

                Log.Comment("Verify that action and close buttons content presenter text will update the buttons name automation property");
                Verify.AreEqual(actionButton.Name, "A:Short Text.");
                Verify.AreEqual(closeButton.Name, "C:Short Text.");
            }
        }

        // Scenario: on the focus page, open a normal tip and press F6 twice; then enable light dismiss, open it and
        //           press Esc.
        // Expected: F6 moves focus to the tip's close button and back to the open button; a light-dismiss tip takes
        //           focus on its close button when it opens, and Esc returns focus to the open button.
        // A failure means: keyboard focus does not move into or out of the tip as designed.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VerifyTeachingTipGetsFocus()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTipFocus Test" }))
            {
                var elements = new TeachingTipFocusTestPageElements();
                var openTeachingTipButton = elements.GetOpenButton();

                FocusHelper.SetFocus(openTeachingTipButton);
                OpenTeachingTip();

                Log.Comment("Verify that an opened teaching tip get focus on F6");
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                var closeButton = elements.GetTeachingTipCloseButton();
                Verify.IsTrue(closeButton.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                Log.Comment("Switch to light-dismissable teaching tip.");
                CloseTeachingTipProgrammatically();
                EnableLightDismiss();

                Log.Comment("Verify that a light-dismissable teaching tip gets focus on opening");
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);
                OpenTeachingTip();
                Wait.ForIdle();
                closeButton = elements.GetTeachingTipCloseButton();
                Verify.IsTrue(closeButton.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Escape);
                Wait.ForIdle();
                Verify.IsTrue(openTeachingTipButton.HasKeyboardFocus);

                void OpenTeachingTip()
                {
                    if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.On)
                    {
                        openTeachingTipButton.InvokeAndWait();
                        WaitForChecked(elements.GetIsOpenCheckBox());
                        WaitForChecked(elements.GetIsIdleCheckBox());
                    }
                }

                void CloseTeachingTipProgrammatically()
                {
                    if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
                    {
                        elements.GetCloseButton().InvokeAndWait();
                        WaitForUnchecked(elements.GetIsOpenCheckBox());
                        WaitForChecked(elements.GetIsIdleCheckBox());
                    }
                }

                void EnableLightDismiss()
                {
                    var isLightDismissEnabledCheckBox = elements.GetIsLightDismissEnabledCheckBox();
                    if (isLightDismissEnabledCheckBox.ToggleState != ToggleState.On)
                    {
                        isLightDismissEnabledCheckBox.Check();
                        WaitForChecked(isLightDismissEnabledCheckBox);
                    }
                }
            }
        }

        // Scenario: open the tip, switch its target, and switch back.
        // Expected: switching moves the tip vertically while it stays open; switching back restores exactly the
        //           original offset.
        // A failure means: changing Target does not reposition an open tip or closes it.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void VerifyTeachingTipTargetChange()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();

                OpenTeachingTip();

                double oldYOffset = GetTipVerticalOffset();

                elements.GetSwitchTargetButton().InvokeAndWait();

                double newYOffset = GetTipVerticalOffset();

                Verify.IsFalse(oldYOffset == newYOffset);
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "Changing the target must not close the tip");

                // Switching back to the original target must restore the original position, showing the tip tracks its current target.
                elements.GetSwitchTargetButton().InvokeAndWait();
                Verify.AreEqual(oldYOffset, GetTipVerticalOffset(), "Tip position for the original target");
            }
        }

        // Scenario: with focus on the page and a tip open without CloseButtonContent, press F6, then Enter, then F6
        //           again with no tip open.
        // Expected: F6 focuses the header X button and is handled (the page's F6 log stays empty); Enter logs Close
        //           Button Clicked, Closing and Closed with reason CloseButton, and focus returns to the show button;
        //           with no tip open F6 reaches the page once.
        // A failure means: F6 targets a hidden button, is not marked handled, or focus is not restored after closing.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void F6FocusesHeaderCloseButtonWhenThereIsNoCloseButtonContent()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                SetTeachingTipLocation(TipLocationOptions.VisualTree);
                ScrollTargetIntoView();
                OpenTeachingTip();
                ClearTeachingTipDebugMessages();

                var showButton = elements.GetShowButton();
                FocusHelper.SetFocus(showButton);
                Wait.ForIdle();
                Verify.IsTrue(showButton.HasKeyboardFocus, "Focus should start on the page");

                // Without CloseButtonContent the header (X) button is the tip's only close button.
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                Verify.IsTrue(elements.GetTeachingTipAlternateCloseButton().HasKeyboardFocus, "F6 should focus the header close button");
                // The open tip handles F6, so the page's PreviewKeyDown logger (tunneling after the tip's handler) must not see it.
                Verify.AreEqual(0, CountTeachingTipDebugMessages("Page KeyDown: F6"), "F6 should be handled by the open tip");

                KeyboardHelper.PressKey(Key.Enter);
                WaitForTipClosed();
                Verify.IsTrue(GetTeachingTipDebugMessage(0).ToString().Contains("Close Button Clicked"));
                Verify.IsTrue(GetTeachingTipDebugMessage(1).ToString().Contains("CloseButton"));
                Verify.IsTrue(GetTeachingTipDebugMessage(2).ToString().Contains("Closed"));
                Verify.IsTrue(GetTeachingTipDebugMessage(2).ToString().Contains("CloseButton"));

                Wait.ForIdle();
                Verify.IsTrue(showButton.HasKeyboardFocus, "Focus should return to the element that had focus before F6");

                // Positive control: with no tip open, F6 is not handled and reaches the page.
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                Verify.AreEqual(1, CountTeachingTipDebugMessages("Page KeyDown: F6"), "An unhandled F6 should reach the page");
            }
        }

        // Scenario: open a light-dismiss tip, move focus back to the page, press F6, then press F6 again with focus
        //           inside the tip.
        // Expected: the first F6 focuses the tip's first focusable element (the content CheckBox) and is handled; the
        //           second keeps focus on the CheckBox (the tip has no element to return to); the tip stays open and
        //           the page's F6 log stays empty.
        // A failure means: F6 does not move keyboard users into a light-dismiss tip, or sends focus somewhere
        //                  unexpected.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void F6MovesFocusIntoOpenLightDismissTip()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                SetTeachingTipLocation(TipLocationOptions.VisualTree);
                ScrollTargetIntoView();
                EnableLightDismiss(true);
                OpenTeachingTip();
                ClearTeachingTipDebugMessages();

                // An opening light-dismiss tip takes focus; move it back to the page so F6 starts outside the tip.
                var showButton = elements.GetShowButton();
                FocusHelper.SetFocus(showButton);
                Wait.ForIdle();
                Verify.IsTrue(showButton.HasKeyboardFocus, "Precondition: focus is on the page, outside the tip");
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "Precondition: moving focus to the page does not dismiss the tip");

                // The tip's content has a CheckBox, its first focusable element; F6 moves focus there and is handled.
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                var contentCheckBox = FindElement.ById("CancelClosesCheckBoxInVisualTree");
                Verify.IsNotNull(contentCheckBox, "The tip's content CheckBox");
                Verify.IsTrue(contentCheckBox.HasKeyboardFocus, "F6 should focus the first focusable element of the light-dismiss tip");
                Verify.IsFalse(showButton.HasKeyboardFocus, "Focus should have left the page");
                Verify.AreEqual(0, CountTeachingTipDebugMessages("Page KeyDown: F6"), "F6 should be handled by the open tip");
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "F6 must not close the tip");

                // F6 again, now with focus inside the tip. Focus came from the light-dismiss branch, which records no element to
                // return to (TeachingTip::HandleF6Clicked), so the tip's popup handler has nowhere to send focus: focus stays in the
                // tip, the tip stays open, and the key does not reach the page.
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                Verify.IsTrue(contentCheckBox.HasKeyboardFocus, "F6 inside the tip with no element to return to keeps focus in the tip");
                Verify.AreEqual(0, CountTeachingTipDebugMessages("Page KeyDown: F6"), "F6 pressed inside the tip does not reach the page");
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "F6 inside the tip must not close it");

                CloseTeachingTipProgrammatically();
            }
        }

        // Scenario: open a light-dismiss tip with no content and no buttons, put focus on the page and press F6.
        // Expected: F6 reaches the page's F6 log exactly once (not handled), focus stays on the show button and the tip
        //           stays open.
        // A failure means: the tip swallows F6 when it has nothing to focus, or moves focus to a hidden button.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void F6IsNotHandledByLightDismissTipWithNothingFocusable()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                SetTeachingTipLocation(TipLocationOptions.VisualTree);
                ScrollTargetIntoView();

                // No content and no button content: with light dismiss both close buttons are collapsed, so nothing in the tip can take focus.
                elements.GetContentComboBox().SelectItemByName("No Content");
                elements.GetSetContentButton().InvokeAndWait();
                EnableLightDismiss(true);
                OpenTeachingTip();
                ClearTeachingTipDebugMessages();

                var showButton = elements.GetShowButton();
                FocusHelper.SetFocus(showButton);
                Wait.ForIdle();
                Verify.IsTrue(showButton.HasKeyboardFocus, "Precondition: focus is on the page, outside the tip");
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "Precondition: moving focus to the page does not dismiss the tip");

                // With nothing to focus, the tip leaves F6 unhandled, so it reaches the page's PreviewKeyDown logger.
                KeyboardHelper.PressKey(Key.F6);
                Wait.ForIdle();
                Verify.AreEqual(1, CountTeachingTipDebugMessages("Page KeyDown: F6"), "F6 should not be handled by a tip with nothing focusable");
                Verify.IsTrue(showButton.HasKeyboardFocus, "Focus should stay on the page");
                Verify.AreEqual(ToggleState.On, elements.GetIsOpenCheckBox().ToggleState, "F6 must not close the tip");

                CloseTeachingTipProgrammatically();
            }
        }

        // Scenario: with UIA WindowOpened/WindowClosed listeners registered, open and close a light-dismiss tip.
        // Expected: WindowOpened and then WindowClosed arrive from the tip within 5s.
        // A failure means: assistive technology is not told that a light-dismiss tip (a UIA window) opened or closed.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void LightDismissTipRaisesUiaWindowOpenedAndClosedEvents()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                SetTeachingTipLocation(TipLocationOptions.VisualTree);
                ScrollTargetIntoView();
                EnableLightDismiss(true);

                // A light-dismiss tip reports itself as a UIA window. The waiters register UIA listeners, which the tip
                // checks (AutomationPeer.ListenerExists) before raising WindowOpened/WindowClosed.
                UIObject appWindow = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
                Verify.IsNotNull(appWindow, "Test app window");
                var tipCondition = UICondition.CreateFromName("TeachingTipInVisualTree");

                using (var openedWaiter = new AutomationEventWaiter(WindowPattern.WindowOpenedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetShowButton().InvokeAndWait();
                    WaitForTipOpened();
                    Verify.IsTrue(openedWaiter.TryWait(TimeSpan.FromSeconds(5)), "Opening a light-dismiss tip should raise UIA WindowOpened");
                }

                using (var closedWaiter = new AutomationEventWaiter(WindowPattern.WindowClosedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetCloseButton().InvokeAndWait();
                    WaitForTipClosed();
                    Verify.IsTrue(closedWaiter.TryWait(TimeSpan.FromSeconds(5)), "Closing a light-dismiss tip should raise UIA WindowClosed");
                }
            }
        }

        // Scenario: with the same UIA listeners registered, open and close a tip without light dismiss, then enable
        //           light dismiss and do it again.
        // Expected: no WindowOpened or WindowClosed arrives within 2s for the normal tip; both arrive within 5s once
        //           light dismiss is on (positive control).
        // A failure means: tips that are not UIA windows announce themselves as windows.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void NonLightDismissTipDoesNotRaiseUiaWindowEvents()
        {
            using (var setup = new TestSetupHelper(new[] { "TeachingTip Tests", "TeachingTip Test" }))
            {
                elements = new TeachingTipTestPageElements();
                SetTeachingTipLocation(TipLocationOptions.VisualTree);
                ScrollTargetIntoView();
                EnableLightDismiss(false);

                UIObject appWindow = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
                Verify.IsNotNull(appWindow, "Test app window");
                var tipCondition = UICondition.CreateFromName("TeachingTipInVisualTree");

                // With UIA listeners registered, a tip without light dismiss is not reported as a window, so neither event is raised.
                // The bounded waits are long compared to the event latency shown by the positive control below.
                using (var openedWaiter = new AutomationEventWaiter(WindowPattern.WindowOpenedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetShowButton().InvokeAndWait();
                    WaitForTipOpened();
                    Verify.IsFalse(openedWaiter.TryWait(TimeSpan.FromSeconds(2)), "Opening a tip without light dismiss must not raise UIA WindowOpened");
                }

                using (var closedWaiter = new AutomationEventWaiter(WindowPattern.WindowClosedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetCloseButton().InvokeAndWait();
                    WaitForTipClosed();
                    Verify.IsFalse(closedWaiter.TryWait(TimeSpan.FromSeconds(2)), "Closing a tip without light dismiss must not raise UIA WindowClosed");
                }

                // Positive control: the same listeners receive both events once light dismiss is enabled.
                EnableLightDismiss(true);
                using (var openedWaiter = new AutomationEventWaiter(WindowPattern.WindowOpenedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetShowButton().InvokeAndWait();
                    WaitForTipOpened();
                    Verify.IsTrue(openedWaiter.TryWait(TimeSpan.FromSeconds(5)), "Positive control: WindowOpened for a light-dismiss tip");
                }

                using (var closedWaiter = new AutomationEventWaiter(WindowPattern.WindowClosedEvent, appWindow, Scope.Descendants, tipCondition))
                {
                    elements.GetCloseButton().InvokeAndWait();
                    WaitForTipClosed();
                    Verify.IsTrue(closedWaiter.TryWait(TimeSpan.FromSeconds(5)), "Positive control: WindowClosed for a light-dismiss tip");
                }
            }
        }

        private void CloseOpenAndCloseWithJustKeyboardViaF6()
        {
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipClosed();
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipOpened();
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipClosed();
        }
        private void UseF6ToReturnToTestPageToCloseTip()
        {
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Tab);
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Tab);
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipClosed();
        }

        private void ScrollTargetIntoView()
        {
            elements.GetBringTargetIntoViewButton().Invoke();
            Wait.ForIdle();
        }

        private void OpenTeachingTip()
        {
            if(elements.GetIsOpenCheckBox().ToggleState != ToggleState.On)
            {
                elements.GetShowButton().InvokeAndWait();
                WaitForChecked(elements.GetIsOpenCheckBox());
                WaitForChecked(elements.GetIsIdleCheckBox());
            }
        }

        private void CloseTeachingTipProgrammatically()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                elements.GetCloseButton().InvokeAndWait();
                WaitForTipClosed();
            }
        }

        private void CloseTeachingTipByLightDismiss(bool useEscKey)
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                if (useEscKey)
                {
                    KeyboardHelper.PressKey(Key.Escape);
                }
                else
                {
                    InputHelper.LeftClick(elements.GetActionButtonContentComboBox());
                }

                WaitForTipClosed();
            }
        }

        private void PressXCloseButton()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                InputHelper.LeftClick(elements.GetTeachingTipAlternateCloseButton());
                WaitForTipClosed();
            }
        }

        private void PressTipCloseButton()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                InputHelper.LeftClick(elements.GetTeachingTipCloseButton());
                WaitForTipClosed();
            }
        }

        private void WaitForTipOpened()
        {
            WaitForChecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());
        }

        private void WaitForTipClosed()
        {
            WaitForUnchecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());
        }

        private void SetTeachingTipLocation(TipLocationOptions location)
        {
            switch(location)
            {
                case TipLocationOptions.ResourceDictionary:
                    elements.GetTipLocationComboBox().SelectItemByName("Resources");
                    break;
                default:
                    elements.GetTipLocationComboBox().SelectItemByName("VisualTree");
                    break;
            }
            elements.GetSetTipLocationButton().Invoke();
            //If a tip was open this action would cause that tip to close, so wait until that happens
            WaitForUnchecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());

            SetTipIsTargeted(true);
        }

        private void EnableLightDismiss(bool enable)
        {
            if(enable)
            {
                elements.GetIsLightDismissEnabledComboBox().SelectItemByName("True");
            }
            else
            {
                elements.GetIsLightDismissEnabledComboBox().SelectItemByName("False");
            }
            elements.GetIsLightDismissEnabledButton().InvokeAndWait();
        }

        private void SetShouldConstrainToRootBounds(bool constrain)
        {
            if(constrain)
            {
                elements.GetShouldConstrainToRootBoundsComboBox().SelectItemByName("True");
            }
            else
            {
                elements.GetShouldConstrainToRootBoundsComboBox().SelectItemByName("False");
            }
            elements.GetShouldConstrainToRootBoundsButton().InvokeAndWait();
        }

        private void SetCloseButtonContent(CloseButtonContentOptions closeButtonContent)
        {
            switch(closeButtonContent)
            {
                case CloseButtonContentOptions.NoText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("No text");
                    break;
                case CloseButtonContentOptions.ShortText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("Small text");
                    break; 
                case CloseButtonContentOptions.LongText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("Long text");
                    break;
            }
            elements.GetSetCloseButtonContentButton().InvokeAndWait();
        }

        private void SetPreferredPlacement(PlacementOptions placement)
        {
            switch (placement)
            {
                case PlacementOptions.Top:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Top");
                    break;
                case PlacementOptions.Bottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Bottom");
                    break;
                case PlacementOptions.Left:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Left");
                    break;
                case PlacementOptions.Right:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Right");
                    break;
                case PlacementOptions.TopRight:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("TopRight");
                    break;
                case PlacementOptions.TopLeft:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("TopLeft");
                    break;
                case PlacementOptions.BottomRight:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("BottomRight");
                    break;
                case PlacementOptions.BottomLeft:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("BottomLeft");
                    break;
                case PlacementOptions.LeftTop:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("LeftTop");
                    break;
                case PlacementOptions.LeftBottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("LeftBottom");
                    break;
                case PlacementOptions.RightTop:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("RightTop");
                    break;
                case PlacementOptions.RightBottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("RightBottom");
                    break;
                case PlacementOptions.Center:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Center");
                    break;
                default:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Auto");
                    break;
            }
            elements.GetSetPreferredPlacementButton().InvokeAndWait();
        }

        private void SetHeroContent(HeroContentOptions heroContent)
        {
            switch (heroContent)
            {
                case HeroContentOptions.RedSquare:
                    elements.GetHeroContentComboBox().SelectItemByName("Red Square");
                    break;
                case HeroContentOptions.BlueSquare:
                    elements.GetHeroContentComboBox().SelectItemByName("Blue Square");
                    break;
                case HeroContentOptions.Image:
                    elements.GetHeroContentComboBox().SelectItemByName("Image");
                    break;
                default:
                    elements.GetHeroContentComboBox().SelectItemByName("No Content");
                    break;
            }
            elements.GetSetHeroContentButton().InvokeAndWait();
        }

        private void SetTitle(TitleContentOptions title)
        {
            switch(title)
            {
                case TitleContentOptions.Long:
                    elements.GetTitleComboBox().SelectItemByName("Long text");
                    break;
                case TitleContentOptions.Small:
                    elements.GetTitleComboBox().SelectItemByName("Samell text");
                    break;
                case TitleContentOptions.No:
                    elements.GetTitleComboBox().SelectItemByName("No title");
                    break;
            }
            elements.GetSetTitleButton().InvokeAndWait();
        }

        private void SetSubtitle(SubtitleContentOptions subtitle)
        {
            switch (subtitle)
            {
                case SubtitleContentOptions.Long:
                    elements.GetSubtitleComboBox().SelectItemByName("Long text");
                    break;
                case SubtitleContentOptions.Small:
                    elements.GetSubtitleComboBox().SelectItemByName("Small text");
                    break;
                case SubtitleContentOptions.No:
                    elements.GetSubtitleComboBox().SelectItemByName("No subtitle");
                    break;
            }
            elements.GetSetSubtitleButton().InvokeAndWait();
        }

        private void SetTipIsTargeted(bool targeted)
        {
            if(targeted)
            {
                elements.GetSetTargetButton().InvokeAndWait();
            }
            else
            {
                elements.GetRemoveTargetButton().InvokeAndWait();
            }
        }

        private void SetIcon(IconOptions icon)
        {
            switch(icon)
            {
                case IconOptions.People:
                    elements.GetIconComboBox().SelectItemByName("People Icon");
                    break;
                default:
                    elements.GetIconComboBox().SelectItemByName("No Icon");
                    break;
            }
            elements.GetSetIconButton().InvokeAndWait();
        }

        private double GetTipVerticalOffset()
        {
            return double.Parse(elements.GetPopupVerticalOffsetTextBlock().GetText());
        }

        private double GetScrollViewerVerticalOffset()
        {
            return double.Parse(elements.GetScrollViewerOffsetTextBox().GetText());
        }

        private void ScrollBy(double ammount)
        {
            double initialOffset = double.Parse(elements.GetScrollViewerOffsetTextBox().GetText());
            elements.GetScrollViewerOffsetTextBox().SetValue((initialOffset + ammount).ToString());
            elements.GetScrollViewerOffsetButton().InvokeAndWait();
        }

        private void UseTestBounds(double x, double y, double width, double height, Vector4 targetRect, bool forWindowBounds)
        {
            if (forWindowBounds)
            {
                UseTestWindowBounds(x, y, width, height);
            }
            else
            {
                UseTestWindowBounds(targetRect.W - 1, targetRect.X - 1, targetRect.Y + 1, targetRect.Z + 1);
                UseTestScreenBounds(x, y, width, height);
            }
        }

        private void UseTestWindowBounds(double x, double y, double width, double height)
        {
            elements.GetTestWindowBoundsXTextBox().SetValue(x.ToString());
            elements.GetTestWindowBoundsYTextBox().SetValue(y.ToString());
            elements.GetTestWindowBoundsWidthTextBox().SetValue(width.ToString());
            elements.GetTestWindowBoundsHeightTextBox().SetValue(height.ToString());

            elements.GetUseTestWindowBoundsCheckbox().Uncheck();

            elements.GetUseTestWindowBoundsCheckbox().Check();
        }

        private void UseTestScreenBounds(double x, double y, double width, double height)
        {
            elements.GetTestScreenBoundsXTextBox().SetValue(x.ToString());
            elements.GetTestScreenBoundsYTextBox().SetValue(y.ToString());
            elements.GetTestScreenBoundsWidthTextBox().SetValue(width.ToString());
            elements.GetTestScreenBoundsHeightTextBox().SetValue(height.ToString());

            elements.GetUseTestWindowBoundsCheckbox().Uncheck();
            elements.GetUseTestScreenBoundsCheckbox().Uncheck();
            elements.GetUseTestScreenBoundsCheckbox().Check();
            elements.GetUseTestWindowBoundsCheckbox().Check();
        }

        private void SetTipFollowsTarget(bool tipFollowsTarget)
        {
            if(tipFollowsTarget)
            {
                elements.GetTipFollowsTargetCheckBox().Check();
            }
            else
            {
                elements.GetTipFollowsTargetCheckBox().Uncheck();
            }
        }

        private void SetReturnTopForOutOfWindowPlacement(bool returnTopForOutOfWindowPlacement)
        {
            if (returnTopForOutOfWindowPlacement)
            {
                elements.GetReturnTopForOutOfWindowPlacementCheckBox().Check();
            }
            else
            {
                elements.GetReturnTopForOutOfWindowPlacementCheckBox().Uncheck();
            }
        }

        Vector4 GetTargetBounds()
        {
            elements.GetTargetBoundsButton().InvokeAndWait();

            var retVal = new Vector4();
            retVal.W = (int)Math.Floor(double.Parse(elements.GetTargetXOffsetTextBlock().GetText()));
            retVal.X = (int)Math.Floor(double.Parse(elements.GetTargetYOffsetTextBlock().GetText()));
            retVal.Y = (int)Math.Floor(double.Parse(elements.GetTargetWidthTextBlock().GetText()));
            retVal.Z = (int)Math.Floor(double.Parse(elements.GetTargetHeightTextBlock().GetText()));
            return retVal;
        }

        private string GetEffectivePlacement()
        {
            try
            {
                // The first call to this can sometimes return a stale value or throw an exception (E_UNEXPECTED)
                // on older OSes like RS3 and earlier. Call it once and ignore the value, then call it again seems
                // to be all that's needed to work around. Presumably this is happening because the app is changing
                // the TextBlock's value in quick succession and there's either a UIA caching bug or a XAML framework
                // issue that has since been fixed.
                elements.GetEffectivePlacementTextBlock().GetText();
            }
            catch { }
            return elements.GetEffectivePlacementTextBlock().GetText();
        }

        private void SetAutomationName(AutomationNameOptions automationName)
        {
            switch(automationName)
            {
                case AutomationNameOptions.VisualTree:
                    elements.GetAutomationNameComboBox().SelectItemByName("TeachingTipInVisualTree");
                    break;
                case AutomationNameOptions.Resources:
                    elements.GetAutomationNameComboBox().SelectItemByName("TeachingTipInResources");
                    break;
                default:
                    elements.GetAutomationNameComboBox().SelectItemByName("None");
                    break;
            }
            elements.GetSetAutomationNameButton().InvokeAndWait();
        }

        private void SetActionButtonContentTo(string option)
        {
            var actionButtonComboBox = elements.GetActionButtonContentComboBox();
            actionButtonComboBox.SelectItemByName(option);
            elements.GetSetActionButtonContentButton().InvokeAndWait();
        }

        // The test UI has a list box which the teaching tip populates with messages about which events have fired and other useful
        // Debugging info. This method returns the message at the provided index, which helps testing that events were received in
        // the expected order.
        private ListBoxItem GetTeachingTipDebugMessage(int index)
        {
            var count = elements.GetLstTeachingTipEvents().Items.Count;
            if (count <= index)
            {
                Log.Comment($"TeachingTipEvents list only has {count} items, waiting a little bit longer to see if they show up");
                Task.Delay(TimeSpan.FromMilliseconds(250)).Wait();
            }
            return elements.GetLstTeachingTipEvents().Items[index];
        }

        private int CountTeachingTipDebugMessages(string text)
        {
            int count = 0;
            foreach (var item in elements.GetLstTeachingTipEvents().Items)
            {
                if (item.ToString().Contains(text))
                {
                    count++;
                }
            }
            return count;
        }

        private void ClearTeachingTipDebugMessages()
        {
            elements.GetBtnClearTeachingTipEvents().InvokeAndWait();
        }

        private bool WaitForChecked(CheckBox checkBox, double millisecondsTimeout = 2000, bool throwOnError = true)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.On, millisecondsTimeout, throwOnError);
        }

        private bool WaitForUnchecked(CheckBox checkBox, double millisecondsTimeout = 2000, bool throwOnError = true)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.Off, millisecondsTimeout, throwOnError);
        }

        private bool WaitForCheckBoxUpdated(CheckBox checkBox, ToggleState state, double millisecondsTimeout, bool throwOnError)
        {
            using (UIEventWaiter waiter = checkBox.GetToggledWaiter())
            {
                Log.Comment(checkBox.Name + " Checked: " + checkBox.ToggleState);
                if (checkBox.ToggleState == state)
                {
                    return true;
                }
                else
                {
                    Log.Comment("Waiting for toggle state to change to {0} for {1}ms", state, millisecondsTimeout);
                    waiter.TryWait(TimeSpan.FromMilliseconds(millisecondsTimeout));
                }
                if (checkBox.ToggleState != state)
                {
                    Log.Warning(checkBox.Name + " value never changed");
                    if (throwOnError)
                    {
                        throw new WaiterException();
                    }
                    else
                    {
                        return false;
                    }
                }
                return true;
            }
        }

        private int WaitForOffsetUpdated(
            double expectedValue,
            double millisecondsTimeout = defaultAnimatedViewChangeTimeout,
            bool failOnError = true)
        {
            Log.Comment("WaitForOffsetUpdated with expectedValue: " + expectedValue);

            int warningCount = 0;
            bool success = WaitForOffsetToSettle(elements.GetScrollViewerOffsetTextBox(), millisecondsTimeout, failOnError);
            double value = Convert.ToDouble(elements.GetScrollViewerOffsetTextBox().GetText());
            bool goodValue = value == expectedValue;
            Verify.IsTrue(goodValue);
            return warningCount;
        }

        private bool WaitForOffsetToSettle(Edit text, double millisecondsTimeout, bool failOnError)
        {
            Wait.ForIdle();

            const double millisecondsNormalStepTimeout = 100;
            const double millisecondsIdleStepTimeout = 600;
            ValueChangedEventWaiter waiter = new ValueChangedEventWaiter(text);
            int unsuccessfulWaits = 0;
            int maxUnsuccessfulWaits = (int)(millisecondsIdleStepTimeout / millisecondsNormalStepTimeout);

            Log.Comment("Original State: " + elements.GetScrollViewerStateTextBox().GetText());
            Log.Comment("Original Offset: " + text.Value);

            // When the initial State is still Idle, use a longer timeout to allow it to transition out of Idle.
            double millisecondsWait = (elements.GetScrollViewerStateTextBox().GetText() == "Idle") ? millisecondsIdleStepTimeout : millisecondsNormalStepTimeout;
            double millisecondsCumulatedWait = 0;

            do
            {
                Log.Comment("Waiting for Offset change.");
                waiter.Reset();
                if (waiter.TryWait(TimeSpan.FromMilliseconds(millisecondsWait)))
                {
                    unsuccessfulWaits = 0;
                }
                else
                {
                    unsuccessfulWaits++;
                }
                millisecondsCumulatedWait += millisecondsWait;
                millisecondsWait = millisecondsNormalStepTimeout;

                Log.Comment("Current State: " + elements.GetScrollViewerStateTextBox().GetText());
                Log.Comment("Current Offset: " + text.Value);

                Wait.ForIdle();
            }
            while (elements.GetScrollViewerStateTextBox().GetText() != "Idle" &&
                   millisecondsCumulatedWait < millisecondsTimeout &&
                   unsuccessfulWaits <= maxUnsuccessfulWaits);

            if (elements.GetScrollViewerStateTextBox().GetText() == "Idle")
            {
                Log.Comment("Idle State reached after " + millisecondsCumulatedWait + " out of " + millisecondsTimeout + " milliseconds. Final Offset: " + text.Value);
                return true;
            }
            else
            {
                string message = unsuccessfulWaits > maxUnsuccessfulWaits ?
                    "Offset has not changed within " + millisecondsIdleStepTimeout + " milliseconds outside of Idle State." :
                    "Idle State was not reached within " + millisecondsTimeout + " milliseconds.";
                if (failOnError)
                {
                    Log.Error(message);
                }
                else
                {
                    Log.Warning(message);
                }

                return false;
            }
        }
    }
}
