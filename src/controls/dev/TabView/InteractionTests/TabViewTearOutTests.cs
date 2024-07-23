// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.IO;
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
using MUXTestInfra.Shared.Infra;
using System.Linq;
using System.Collections.Generic;
using System;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TabViewTearOutTests
    {
        private static TestApplicationInfo _tabViewTearOutApp;

        public static TestApplicationInfo TabViewTearOutApp
        {
            get
            {
                if (_tabViewTearOutApp == null)
                {
                    _tabViewTearOutApp = new TestApplicationInfo(
                        "TabViewTearOutApp_6f07fta6qpts2",
                        "Tab Tear-Out App",
                        Path.Combine("TabViewTearOutApp", "TabViewTearOutApp.exe"));
                }

                return _tabViewTearOutApp;
            }
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("IsolationLevel", "Method")] // These tests create and destroy windows, so we'll isolate each test to ensure we start with a known good state each time.
        public static void ClassInitialize(TestContext testContext)
        {
            // These tests involve multiple windows, so don't bother maximizing the window at the start of tests.
            testContext.Properties.Add("MaximizeWindowAtStart", false);

            TestEnvironment.Initialize(testContext, TabViewTearOutApp);
        }

        [ClassCleanup]
        public static void ClassCleanup()
        {
            TestEnvironment.AssemblyCleanupWorker(TabViewTearOutApp);
        }

        [TestCleanup]
        public static void TestCleanup()
        {
            var testAppProcess = TestEnvironment.Application.Process;

            if (testAppProcess != null && !testAppProcess.HasExited)
            {
                testAppProcess.Kill();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that CanTearOutTabs enables the tearing out of tabs into new windows, and the rejoining of those tabs into existing windows.")]
        public void CanTearOutAndRejoinTabs()
        {
            TabViewTearOutTestHelpers.CanTearOutAndRejoinTabs(GetTabViewTearOutAppWindows, GetTabViewFromWindow, GetTabsFromTabView);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that CanTearOutTabs does not prevent internally reordering tabs.")]
        [TestProperty("Ignore", "True")] // Task 50591398: Re-enable tests when we ingest the latest Microsoft.UI.Input.dll
        public void CanReorderTabs()
        {
            TabViewTearOutTestHelpers.CanReorderTabs(GetTabViewTearOutAppWindows, GetTabViewFromWindow, GetTabsFromTabView);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that CanTearOutTabs does not prevent selecting tabs.")]
        public void CanSelectTabs()
        {
            TabViewTearOutTestHelpers.CanSelectTabs(GetTabViewTearOutAppWindows, GetTabViewFromWindow, GetTabsFromTabView);
        }

        private static readonly UICondition _windowCondition =
            UICondition.CreateFromClassName("WinUIDesktopWin32WindowClass")
            .AndWith(UICondition.CreateFromName(TabViewTearOutApp.TestAppMainWindowTitle));

        private static readonly UICondition _tabViewCondition =
            UICondition.CreateFromName("TearOutTabsTabView");

        private static readonly UICondition _tabCondition =
            UICondition.CreateFromClassName("ListViewItem");

        private static List<Window> GetTabViewTearOutAppWindows()
        {
            return UIObject.Root.Children.FindMultiple(_windowCondition).Select(o => new Window(o)).ToList();
        }

        private static Tab GetTabViewFromWindow(Window window)
        {
            return new Tab(window.Descendants.Find(_tabViewCondition));
        }

        private static List<TabItem> GetTabsFromTabView(Tab tabView)
        {
            return tabView.Descendants.FindMultiple(_tabCondition).Select(o => new TabItem(o)).ToList();
        }
    }

    public static class TabViewTearOutTestHelpers
    {
        public static void CanTearOutAndRejoinTabs(
            Func<List<Window>> getTabViewTearOutAppWindowsFunc,
            Func<Window, Tab> getTabViewFromWindowFunc,
            Func<Tab, List<TabItem>> getTabsFromTabViewFunc)
        {
            try
            {
                Log.Comment("We should begin with only one window.");

                var windows = getTabViewTearOutAppWindowsFunc();

                Verify.AreEqual(1, windows.Count);

                var window = windows.First();

                var tabView = getTabViewFromWindowFunc(window);
                var tabs = getTabsFromTabViewFunc(tabView);

                Log.Comment("The window should have three tabs in the order 1, 2, 3.");

                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 1", tabs[0].Name);
                Verify.AreEqual("Item 2", tabs[1].Name);
                Verify.AreEqual("Item 3", tabs[2].Name);

                Log.Comment("Let's ensure the window is visible prior to dragging on it.");

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll tear out the first tab.");

                InputHelper.MouseDragDistance(tabs[0], distance: 300, Direction.South, duration: 1000);

                Log.Comment("We should now have two windows: the first with two tabs in the order 2, 3; and the second with one tab that is 1.");

                windows = getTabViewTearOutAppWindowsFunc();
                Verify.AreEqual(2, windows.Count);

                // The new window is the one that contains Item 1.
                var newWindow = windows.Where(w => w.Descendants.TryFind(UICondition.CreateFromName("Item 1"), out _)).First();
                var newWindowTabView = getTabViewFromWindowFunc(newWindow);

                tabs = getTabsFromTabViewFunc(tabView);
                var newTabs = getTabsFromTabViewFunc(newWindowTabView);

                Verify.AreEqual(2, tabs.Count);
                Verify.AreEqual("Item 2", tabs[0].Name);
                Verify.AreEqual("Item 3", tabs[1].Name);
                Verify.AreEqual(1, newTabs.Count);
                Verify.AreEqual("Item 1", newTabs[0].Name);

                Log.Comment("Let's ensure the second window is visible prior to dragging on it.");

                newWindow.SetWindowVisualState(WindowVisualState.Normal);

                using (var closingWaiter = newWindow.GetWindowClosedWaiter())
                {
                    Log.Comment("Now we'll drag the torn out tab to the end of the original tab view. This should merge the tab to the end the original tab view and close the second window.");

                    var addNewTabButton = tabView.Descendants.Find(UICondition.CreateFromName("Add New Tab"));

                    InputHelper.MouseDragToTarget(newTabs[0], addNewTabButton);
                    closingWaiter.Wait();
                }

                Log.Comment("We should now have one windows with three tabs in the order 2, 3, 1.");

                windows = getTabViewTearOutAppWindowsFunc();
                tabs = getTabsFromTabViewFunc(tabView);

                Verify.AreEqual(1, windows.Count);
                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 2", tabs[0].Name);
                Verify.AreEqual("Item 3", tabs[1].Name);
                Verify.AreEqual("Item 1", tabs[2].Name);
            }
            finally
            {
                foreach (Window window in getTabViewTearOutAppWindowsFunc())
                {
                    using (var closingWaiter = window.GetWindowClosedWaiter())
                    {
                        window.Close();
                        closingWaiter.Wait();
                    }
                }
            }
        }

        public static void CanReorderTabs(
            Func<List<Window>> getTabViewTearOutAppWindowsFunc,
            Func<Window, Tab> getTabViewFromWindowFunc,
            Func<Tab, List<TabItem>> getTabsFromTabViewFunc)
        {
            try
            {
                Log.Comment("We should begin with only one window.");

                var windows = getTabViewTearOutAppWindowsFunc();

                Verify.AreEqual(1, windows.Count);

                var window = windows.First();

                var tabView = getTabViewFromWindowFunc(window);
                var tabs = getTabsFromTabViewFunc(tabView);

                Log.Comment("The window should have three tabs in the order 1, 2, 3.");

                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 1", tabs[0].Name);
                Verify.AreEqual("Item 2", tabs[1].Name);
                Verify.AreEqual("Item 3", tabs[2].Name);

                Log.Comment("Let's ensure the window is visible prior to dragging on it.");

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll drag tab 1 to the right side of tab 2.");

                InputHelper.MouseDragToTarget(tabs[0], tabs[1], 50);

                Log.Comment("We should still have only one window.");

                windows = getTabViewTearOutAppWindowsFunc();

                Verify.AreEqual(1, windows.Count);

                tabs = getTabsFromTabViewFunc(tabView);

                Log.Comment("The window should now have three tabs in the order 2, 1, 3.");

                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 2", tabs[0].Name);
                Verify.AreEqual("Item 1", tabs[1].Name);
                Verify.AreEqual("Item 3", tabs[2].Name);

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll drag tab 3 to the left side of tab 2.");

                InputHelper.MouseDragToTarget(tabs[2], tabs[0], -50);

                Log.Comment("We should still have only one window.");

                windows = getTabViewTearOutAppWindowsFunc();

                Verify.AreEqual(1, windows.Count);

                tabs = getTabsFromTabViewFunc(tabView);

                Log.Comment("The window should now have three tabs in the order 3, 2, 1.");

                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 3", tabs[0].Name);
                Verify.AreEqual("Item 2", tabs[1].Name);
                Verify.AreEqual("Item 1", tabs[2].Name);
            }
            finally
            {
                foreach (Window window in getTabViewTearOutAppWindowsFunc())
                {
                    using (var closingWaiter = window.GetWindowClosedWaiter())
                    {
                        window.Close();
                        closingWaiter.Wait();
                    }
                }
            }
        }
        public static void CanSelectTabs(
            Func<List<Window>> getTabViewTearOutAppWindowsFunc,
            Func<Window, Tab> getTabViewFromWindowFunc,
            Func<Tab, List<TabItem>> getTabsFromTabViewFunc)
        {
            try
            {
                Log.Comment("We should begin with only one window.");

                var windows = getTabViewTearOutAppWindowsFunc();

                Verify.AreEqual(1, windows.Count);

                var window = windows.First();

                var tabView = getTabViewFromWindowFunc(windows[0]);
                var tabs = getTabsFromTabViewFunc(tabView);

                Log.Comment("The window should have three tabs in the order 1, 2, 3.");

                Verify.AreEqual(3, tabs.Count);
                Verify.AreEqual("Item 1", tabs[0].Name);
                Verify.AreEqual("Item 2", tabs[1].Name);
                Verify.AreEqual("Item 3", tabs[2].Name);

                Log.Comment("The first tab should be selected.");

                Verify.IsTrue(tabs[0].IsSelected);
                Verify.IsFalse(tabs[1].IsSelected);
                Verify.IsFalse(tabs[2].IsSelected);

                Log.Comment("Let's ensure the window is visible prior to clicking on it.");

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll click on tab 2. It should be selected.");

                InputHelper.LeftClick(tabs[1]);

                Verify.IsFalse(tabs[0].IsSelected);
                Verify.IsTrue(tabs[1].IsSelected);
                Verify.IsFalse(tabs[2].IsSelected);

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll click on tab 3. It should be selected.");

                InputHelper.LeftClick(tabs[2]);

                Verify.IsFalse(tabs[0].IsSelected);
                Verify.IsFalse(tabs[1].IsSelected);
                Verify.IsTrue(tabs[2].IsSelected);

                window.SetWindowVisualState(WindowVisualState.Normal);

                Log.Comment("Now we'll click on tab 1. It should be selected.");

                InputHelper.LeftClick(tabs[0]);

                Verify.IsTrue(tabs[0].IsSelected);
                Verify.IsFalse(tabs[1].IsSelected);
                Verify.IsFalse(tabs[2].IsSelected);
            }
            finally
            {
                foreach (Window window in getTabViewTearOutAppWindowsFunc())
                {
                    using (var closingWaiter = window.GetWindowClosedWaiter())
                    {
                        window.Close();
                        closingWaiter.Wait();
                    }
                }
            }
        }
    }
}
