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
