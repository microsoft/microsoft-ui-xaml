// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
    [TestClass]
    public class TitleBarTests
    {
        // TitleBarPageWindow is a separate top-level window whose TitleBar is set as the window title bar
        // (ExtendsContentIntoTitleBar + SetTitleBar), so its buttons live in the non-client area.
        private const string TitleBarWindowTitle = "Windowing TitleBar";
        private static readonly UICondition TitleBarWindowCondition = UICondition.CreateFromName(TitleBarWindowTitle);

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

        // Scenario: real-mouse double-click on an empty, non-interactive part of the TitleBar in its own window.
        // Expected: the window caption handles it and the window maximizes. This is the control case that proves the
        //           "not part of caption" checks in the other tests can fail.
        // A failure means: the TitleBar area no longer acts as the window caption (drag/maximize broken), or the test
        //                  machine is not delivering real input.
        [TestMethod]
        // Closing the test app after visiting the TitleBar page currently fail-fasts the app (PC-3), and a test-host crash
        // on app restart would cascade into unrelated tests. Isolate each method in its own app process.
        [TestProperty("IsolationLevel", "Method")]
        public void EmptyTitleBarAreaActsAsWindowCaption()
        {
            RunInTitleBarTestWindow(window =>
            {
                // Sanity check for the passthrough tests below: a double-click on a non-interactive part of the
                // TitleBar is handled by the window caption and maximizes the window.
                UIObject titleBar = FindIn(window, "WindowingTitleBar");
                Verify.AreEqual(WindowVisualState.Normal, window.WindowVisualState);
                InputHelper.LeftDoubleClick(titleBar, titleBar.BoundingRectangle.Width / 2, titleBar.BoundingRectangle.Height / 2);
                Wait.ForIdle();
                Verify.AreEqual(WindowVisualState.Maximized, window.WindowVisualState);
            });
        }

        // Scenario: show the back button at runtime and real-mouse double-click it.
        // Expected: BackRequested fires twice, PaneToggleRequested does not fire, and the window stays Normal (the
        //           caption did not handle the clicks).
        // A failure means: product bug PC-1 (currently the case): the back button shown at runtime has no clickable
        //                  region, so the double-click maximizes the window instead. Ignored until PC-1 is fixed.
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Ignore", "True")] // PC-1: passthrough regions are not refreshed after layout when the TitleBar size is unchanged (UpdateDragRegion runs before layout in OnPropertyChanged). Re-enable when fixed.
        public void BackButtonIsClickableAndNotPartOfCaption()
        {
            RunInTitleBarTestWindow(window =>
            {
                GetCheckBox(window, "IsBackButtonVisibleCheckBox").Check();
                Wait.ForIdle();

                // If the back button is not registered as a passthrough region, the window caption handles the
                // double-click (maximizing the window) and swallows the second click.
                DoubleClick(FindIn(window, "PART_BackButton"));
                Verify.AreEqual("2", FindIn(window, "BackRequestedCountTextBox").Name);
                Verify.AreEqual(string.Empty, FindIn(window, "PaneToggleButtonRequestedCountTextBox").Name);
                Verify.AreEqual(WindowVisualState.Normal, window.WindowVisualState, "Clicking the back button must not act on the window caption");
            });
        }

        // Scenario: show the pane toggle button at runtime and real-mouse double-click it.
        // Expected: PaneToggleRequested fires twice, BackRequested does not fire, and the window stays Normal.
        // A failure means: product bug PC-1 (currently the case): the pane toggle button shown at runtime is not
        //                  clickable. Ignored until PC-1 is fixed.
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Ignore", "True")] // PC-1: passthrough regions are not refreshed after layout when the TitleBar size is unchanged (UpdateDragRegion runs before layout in OnPropertyChanged). Re-enable when fixed.
        public void PaneToggleButtonIsClickableAndNotPartOfCaption()
        {
            RunInTitleBarTestWindow(window =>
            {
                GetCheckBox(window, "IsPaneToggleButtonVisibleCheckbox").Check();
                Wait.ForIdle();

                DoubleClick(FindIn(window, "PART_PaneToggleButton"));
                Verify.AreEqual("2", FindIn(window, "PaneToggleButtonRequestedCountTextBox").Name);
                Verify.AreEqual(string.Empty, FindIn(window, "BackRequestedCountTextBox").Name);
                Verify.AreEqual(WindowVisualState.Normal, window.WindowVisualState, "Clicking the pane toggle button must not act on the window caption");
            });
        }

        // Scenario: show custom content with two buttons in the TitleBar and real-mouse double-click each of them.
        // Expected: each double-click reaches the button (click count goes to 2, then 4) and the window stays Normal.
        // A failure means: buttons placed in TitleBar content are treated as window caption, so users cannot click
        //                  them.
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        public void ContentButtonsAreClickableAndNotPartOfCaption()
        {
            RunInTitleBarTestWindow(window =>
            {
                GetCheckBox(window, "CustomContentCheckBox").Check();
                Wait.ForIdle();

                DoubleClick(FindIn(window, "TitleBarContentLeftButton"));
                Verify.AreEqual("2", FindIn(window, "ContentButtonClickCountTextBlock").Name);

                DoubleClick(FindIn(window, "TitleBarContentRightButton"));
                Verify.AreEqual("4", FindIn(window, "ContentButtonClickCountTextBlock").Name);
                Verify.AreEqual(WindowVisualState.Normal, window.WindowVisualState, "Clicking content buttons must not act on the window caption");
            });
        }

        // Scenario: inspect the TitleBar through UI Automation in the running app, then change its Title.
        // Expected: the element has control type TitleBar and its name equals Title; after changing Title, both the
        //           automation name and the top-level window title update.
        // A failure means: assistive technology sees the wrong role/name, or the window title shown by the shell is
        //                  not synced with the TitleBar.
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        public void TitleBarExposesTitleBarControlTypeAndTitleName()
        {
            RunInTitleBarTestWindow(window =>
            {
                UIObject titleBar = FindIn(window, "WindowingTitleBar");
                Verify.AreEqual(ControlType.TitleBar, titleBar.ControlType);
                Verify.AreEqual(TitleBarWindowTitle, titleBar.Name);

                // Changing Title updates both the TitleBar automation name and the window title.
                const string newTitle = "Updated TitleBar Title";
                new Edit(FindIn(window, "TitleTextBox")).SetValue(newTitle);
                Click(FindIn(window, "TitleButton"));

                Verify.AreEqual(newTitle, FindIn(window, "WindowingTitleBar").Name);
                Verify.IsTrue(
                    UIObject.Root.Children.TryFind(UICondition.CreateFromName(newTitle), out UIObject _),
                    "The TitleBar window title should follow TitleBar.Title");
            });
        }

        // Scenario: drag the window by its TitleBar with the real mouse (raising WindowRectChanged) and read the
        //           registered Icon region before and after the move.
        // Expected: the window moves, WindowRectChanged is raised, and the Icon region still matches the icon's
        //           window-relative bounds.
        // A failure means: the icon region is lost or wrong after the window moves. This is a smoke test: on current
        //                  OS builds a missing WindowRectChanged handler is not observable (verified with a stubbed
        //                  handler).
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        public void IconRegionIsKeptWhenWindowIsDraggedByCaption()
        {
            RunInTitleBarTestWindow(window =>
            {
                string before = ReadIconRegion(window);
                Verify.AreNotEqual("none", before, "The default TitleBarPageWindow icon should be registered as the icon region");

                // Smoke test: drag the empty TitleBar area so the window moves through the non-client caption and
                // InputNonClientPointerSource.WindowRectChanged is raised, then read the icon region immediately
                // (before the readout updates any UI) and check that it is still registered at the icon's
                // window-relative bounds. On the validation VM the platform does not clear the icon region on such a
                // move, so a TitleBar that ignored WindowRectChanged would also pass (verified with a throwaway stub).
                var startBounds = window.BoundingRectangle;
                InputHelper.MouseDragDistance(FindIn(window, "WindowingTitleBar"), 120, Direction.South);
                Wait.ForIdle();

                var endBounds = window.BoundingRectangle;
                Log.Comment(string.Format("Window bounds before {0}, after {1}", startBounds, endBounds));
                Verify.AreNotEqual(startBounds.Y, endBounds.Y, "Dragging the caption should move the window");

                string after = ReadIconRegion(window);
                Verify.AreNotEqual("0", FindIn(window, "WindowRectChangedCountTextBlock").Name, "WindowRectChanged should have been raised");
                Verify.AreEqual(before, after, "The icon region should still match the icon after the window moved");
            });
        }

        private static string ReadIconRegion(Window window)
        {
            new InvokeImplementation(FindIn(window, "ReadIconRegionButton")).Invoke();
            Wait.ForIdle();
            string region = FindIn(window, "IconRegionTextBlock").Name;
            Log.Comment("Icon region: " + region);
            return region;
        }

        // Scenario: a fully populated TitleBar in its own window; real-mouse click the main test window, click the
        //           TitleBar window again, then disable the back button and click the main test window once more.
        // Expected: all parts switch to their *Deactivated visual states when the window loses activation and back to
        //           *Visible when it is reactivated; a disabled back button keeps its regular state while the rest
        //           deactivates.
        // A failure means: the TitleBar does not show the standard inactive-window look, or gets stuck in it. If the
        //                  InputActivation entry is the mismatch, the window never lost or regained the foreground,
        //                  which points at the test machine rather than TitleBar.
        // TitleBar follows InputActivationListener, which only changes when the window really gains or loses the
        // foreground. Window.Activate() from an API test does not guarantee that in the lab (the test app may not own
        // the foreground), so activation is driven here with real mouse clicks.
        [TestMethod]
        [TestProperty("IsolationLevel", "Method")]
        public void PartsShowDeactivatedVisualStatesWhileWindowIsInactive()
        {
            RunInTitleBarTestWindow(window =>
            {
                GetCheckBox(window, "IsBackButtonVisibleCheckBox").Check();
                GetCheckBox(window, "IsPaneToggleButtonVisibleCheckbox").Check();
                GetCheckBox(window, "LeftHeaderCheckBox").Check();
                GetCheckBox(window, "CustomContentCheckBox").Check();
                GetCheckBox(window, "RightHeaderCheckBox").Check();
                new Edit(FindIn(window, "SubtitleTextBox")).SetValue("Activation subtitle");
                Click(FindIn(window, "SetSubtitleButton"));

                var activatedStates = new (string Group, string State)[]
                {
                    ("InputActivation", "Activated"),
                    ("BackButtonGroup", "BackButtonVisible"),
                    ("PaneToggleButtonGroup", "PaneToggleButtonVisible"),
                    ("IconGroup", "IconVisible"),
                    ("TitleTextGroup", "TitleTextVisible"),
                    ("SubtitleTextGroup", "SubtitleTextVisible"),
                    ("LeftHeaderGroup", "LeftHeaderVisible"),
                    ("ContentGroup", "ContentVisible"),
                    ("RightHeaderGroup", "RightHeaderVisible"),
                };
                var deactivatedStates = activatedStates.Select(s => (s.Group, s.State.Replace("Visible", "Deactivated").Replace("Activated", "Deactivated"))).ToArray();

                ActivateTitleBarWindow(window);
                WaitForVisualStates(window, activatedStates);

                Log.Comment("Clicking the main test window to deactivate the TitleBar window.");
                ActivateMainTestWindow();
                WaitForVisualStates(window, deactivatedStates);

                Log.Comment("Clicking the TitleBar window to reactivate it.");
                ActivateTitleBarWindow(window);
                WaitForVisualStates(window, activatedStates);

                // A disabled back button keeps its regular look while the rest of the TitleBar deactivates.
                GetCheckBox(window, "IsBackButtonEnabledCheckBox").Uncheck();
                Wait.ForIdle();
                ActivateMainTestWindow();
                WaitForVisualStates(window, new (string Group, string State)[]
                {
                    ("InputActivation", "Deactivated"),
                    ("TitleTextGroup", "TitleTextDeactivated"),
                    ("BackButtonGroup", "BackButtonVisible"),
                });
            });
        }

        // RunInTitleBarTestWindow minimizes the main test window; restore it and click an empty spot of its system
        // caption so it takes the foreground from the TitleBar window.
        private static void ActivateMainTestWindow()
        {
            var mainWindow = new Window(TestEnvironment.Application.CoreWindow);
            mainWindow.SetWindowVisualState(WindowVisualState.Maximized);
            Wait.ForIdle();
            InputHelper.LeftClick(mainWindow, mainWindow.BoundingRectangle.Width / 2, 16);
        }

        // Minimize the main test window again and click a non-interactive part of the TitleBar window's client area
        // (the right side of the options panel, where all controls are left-aligned).
        private static void ActivateTitleBarWindow(Window window)
        {
            var mainWindow = new Window(TestEnvironment.Application.CoreWindow);
            if (mainWindow.WindowVisualState != WindowVisualState.Minimized)
            {
                mainWindow.SetWindowVisualState(WindowVisualState.Minimized);
                Wait.ForIdle();
            }

            // Skip Wait.ForIdle: it invokes a helper button in the main test window through UI Automation, which
            // activates the main window again.
            InputHelper.LeftClick(window, window.BoundingRectangle.Width - 40, window.BoundingRectangle.Height / 2, skipWait: true);
        }

        private static void WaitForVisualStates(Window window, (string Group, string State)[] expectedStates)
        {
            // TitleBarPageWindow keeps VisualStatesTextBlock up to date, so only read it here. Do not use Wait.ForIdle or
            // invoke anything: UI Automation invokes activate the window that hosts the invoked element. The timeout is
            // only an outer safety bound.
            string mismatch = null;
            var deadline = DateTime.Now.AddSeconds(5);
            do
            {
                Wait.ForMilliseconds(100);
                string readout = FindIn(window, "VisualStatesTextBlock").Name;
                var actualStates = readout.Split(';')
                    .Select(entry => entry.Split('='))
                    .Where(parts => parts.Length == 2)
                    .ToDictionary(parts => parts[0], parts => parts[1]);

                mismatch = null;
                foreach (var (group, state) in expectedStates)
                {
                    actualStates.TryGetValue(group, out string actual);
                    if (actual != state)
                    {
                        mismatch = $"{group}: expected '{state}' but was '{actual}' (readout: {readout})";
                        break;
                    }
                }
            }
            while (mismatch != null && DateTime.Now < deadline);

            Verify.IsNull(mismatch, mismatch ?? "All expected visual states reached");
        }

        private static void RunInTitleBarTestWindow(Action<Window> test)
        {
            using (var windowOpenedWaiter = new WindowOpenedWaiter(TitleBarWindowCondition))
            using (var setup = new TestSetupHelper(new[] { "TitleBar Tests", "New Window with Custom TitleBar" }))
            {
                windowOpenedWaiter.Wait();
                Window window = GetTitleBarTestWindow();

                // The test infrastructure maximizes the main test window and keeps it in the foreground, so it covers
                // the new TitleBar window and would receive every mouse click. Minimize it for the duration of the test.
                var mainWindow = new Window(TestEnvironment.Application.CoreWindow);
                var mainWindowState = mainWindow.WindowVisualState;
                mainWindow.SetWindowVisualState(WindowVisualState.Minimized);
                Wait.ForIdle();

                try
                {
                    test(window);
                }
                finally
                {
                    // The secondary window keeps the test app process alive, so always close it.
                    using (var closedWaiter = new WindowClosedWaiter(window))
                    {
                        window.Close();
                        closedWaiter.Wait();
                    }

                    mainWindow.SetWindowVisualState(mainWindowState);
                    Wait.ForIdle();
                }
            }
        }

        // The TitleBar test page also hosts an in-page TitleBar with the same Title, which renames the main window,
        // so pick the top-level window that contains the TitleBarPageWindow controls.
        private static Window GetTitleBarTestWindow()
        {
            var windows = UIObject.Root.Children.FindMultiple(TitleBarWindowCondition)
                .Where(o => o.Descendants.TryFind(UICondition.CreateFromId("IsBackButtonVisibleCheckBox"), out UIObject _))
                .Select(o => new Window(o))
                .ToList();
            Verify.AreEqual(1, windows.Count, "Exactly one TitleBarPageWindow should be open");
            return windows[0];
        }

        private static void Click(UIObject element)
        {
            Log.Comment($"Clicking '{element.AutomationId}' at {element.BoundingRectangle}");
            InputHelper.LeftClick(element);
            Wait.ForIdle();
        }

        private static void DoubleClick(UIObject element)
        {
            Log.Comment($"Double-clicking '{element.AutomationId}' at {element.BoundingRectangle}");
            InputHelper.LeftDoubleClick(element);
            Wait.ForIdle();
        }

        private static UIObject FindIn(Window window, string automationId)
        {
            Verify.IsTrue(
                window.Descendants.TryFind(UICondition.CreateFromId(automationId), out UIObject element),
                $"Element '{automationId}' should exist in the TitleBar window");
            return element;
        }

        private static CheckBox GetCheckBox(Window window, string automationId)
        {
            return new CheckBox(FindIn(window, automationId));
        }
    }
}
