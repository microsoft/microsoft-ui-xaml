// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using System;
using System.Collections.Generic;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Shapes;
using System.Collections.ObjectModel;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using NavigationViewDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationViewBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class NavigationViewTests
    {
        private NavigationView SetupNavigationView(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Undo", Icon = new SymbolIcon(Symbol.Undo) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Cut", Icon = new SymbolIcon(Symbol.Cut) });

                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.IsSettingsVisible = true;
                navView.PaneDisplayMode = paneDisplayMode;
                navView.OpenPaneLength = 120.0;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 800.0;
                navView.Content = "This is a simple test";
                MUXControlsTestApp.App.TestContentRoot = navView;
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        [TestMethod]
        public void VerifyPaneDisplayModeAndDisplayModeMapping()
        {
            var navView = SetupNavigationView();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Expanded);
                Verify.IsTrue(navView.IsPaneOpen, "Pane opened");
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Minimal, "Top Minimal");
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
            });
            IdleSynchronizer.Wait();
            
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Expanded, "Left Expanded");
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftCompact;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Compact, "LeftCompact Compact");
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal;
            });
            IdleSynchronizer.Wait();           

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Minimal, "LeftMinimal Minimal");
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Auto;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Expanded, "Auto Expanded");
                navView.Width = navView.ExpandedModeThresholdWidth - 10.0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Compact, "Auto Compact");
                navView.Width = navView.CompactModeThresholdWidth - 10.0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(navView.DisplayMode, NavigationViewDisplayMode.Minimal, "Auto Minimal");
            });
        }

        [TestMethod]
        public void VerifyDefaultsAndBasicSetting()
        {
            NavigationView navView = null;
            Rectangle footer = null;
            TextBlock header = null;
            Setter styleSetter = null;
            Style hamburgerStyle = null;


            RunOnUIThread.Execute(() =>
            {
                footer = new Rectangle();
                footer.Height = 40;

                header = new TextBlock();
                header.Text = "Header";

                styleSetter = new Setter();
                styleSetter.Property = FrameworkElement.MinHeightProperty;
                styleSetter.Value = "80";
                hamburgerStyle = new Style();

                navView = new NavigationView();

                // Verify Defaults
                Verify.IsTrue(navView.IsPaneOpen);
                Verify.AreEqual(641, navView.CompactModeThresholdWidth);
                Verify.AreEqual(1008, navView.ExpandedModeThresholdWidth);
                Verify.IsNull(navView.PaneFooter);
                Verify.IsNull(navView.Header);
                Verify.IsTrue(navView.IsSettingsVisible);
                Verify.IsTrue(navView.IsPaneToggleButtonVisible);
                Verify.IsTrue(navView.AlwaysShowHeader);
                Verify.AreEqual(48, navView.CompactPaneLength);
                Verify.AreEqual(320, navView.OpenPaneLength);
                Verify.IsNull(navView.PaneToggleButtonStyle);
                Verify.AreEqual(0, navView.MenuItems.Count);
                Verify.AreEqual(NavigationViewDisplayMode.Minimal, navView.DisplayMode);
                Verify.AreEqual("", navView.PaneTitle);
                Verify.IsFalse(navView.IsBackEnabled);
                Verify.AreEqual(NavigationViewBackButtonVisible.Auto, navView.IsBackButtonVisible);

                // Verify basic setters
                navView.IsPaneOpen = true;
                navView.CompactModeThresholdWidth = 500;
                navView.ExpandedModeThresholdWidth = 1000;
                navView.PaneFooter = footer;
                navView.Header = header;
                navView.IsSettingsVisible = false;
                navView.IsPaneToggleButtonVisible = false;
                navView.AlwaysShowHeader = false;
                navView.CompactPaneLength = 40;
                navView.OpenPaneLength = 300;
                navView.PaneToggleButtonStyle = hamburgerStyle;
                navView.PaneTitle = "ChangedTitle";
                navView.IsBackEnabled = true;
                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                // TODO(test adding a MenuItem programmatically)
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(navView.IsPaneOpen);
                Verify.AreEqual(500, navView.CompactModeThresholdWidth);
                Verify.AreEqual(1000, navView.ExpandedModeThresholdWidth);
                Verify.AreEqual(footer, navView.PaneFooter);
                Verify.AreEqual(header, navView.Header);
                Verify.IsFalse(navView.IsSettingsVisible);
                Verify.IsFalse(navView.IsPaneToggleButtonVisible);
                Verify.IsFalse(navView.AlwaysShowHeader);
                Verify.AreEqual(40, navView.CompactPaneLength);
                Verify.AreEqual(300, navView.OpenPaneLength);
                Verify.AreEqual(hamburgerStyle, navView.PaneToggleButtonStyle);
                Verify.AreEqual("ChangedTitle", navView.PaneTitle);
                Verify.IsTrue(navView.IsBackEnabled);
                Verify.AreEqual(NavigationViewBackButtonVisible.Visible, navView.IsBackButtonVisible);

                // Verify nullable values
                navView.PaneFooter = null;
                navView.Header = null;
                navView.PaneToggleButtonStyle = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(navView.PaneFooter);
                Verify.IsNull(navView.Header);
                Verify.IsNull(navView.PaneToggleButtonStyle);
            });
        }

        [TestMethod]
        public void VerifyValuesCoercion()
        {
            RunOnUIThread.Execute(() =>
            {
                NavigationView navView = new NavigationView();

                navView.CompactModeThresholdWidth = -1;
                navView.ExpandedModeThresholdWidth = -1;
                navView.CompactPaneLength = -1;
                navView.OpenPaneLength = -1;

                Verify.AreEqual(0, navView.CompactModeThresholdWidth, "Should coerce negative CompactModeThresholdWidth values to 0");
                Verify.AreEqual(0, navView.ExpandedModeThresholdWidth, "Should coerce negative ExpandedModeThresholdWidth values to 0");
                Verify.AreEqual(0, navView.CompactPaneLength, "Should coerce negative CompactPaneLength values to 0");
                Verify.AreEqual(0, navView.OpenPaneLength, "Should coerce negative OpenPaneLength values to 0");
            });
        }

        [TestMethod]
        public void VerifyPaneProperties()
        {
            RunOnUIThread.Execute(() =>
            {
                NavigationView navView = new NavigationView();

                // These properties are template-bound to SplitView properties, so testing getters/setters should be sufficient
                navView.IsPaneOpen = false;
                navView.CompactPaneLength = 100.0;
                navView.OpenPaneLength = 200.0;

                Verify.AreEqual(false, navView.IsPaneOpen);
                Verify.AreEqual(100.0, navView.CompactPaneLength);
                Verify.AreEqual(200.0, navView.OpenPaneLength);

                navView.IsPaneOpen = true;
                Verify.AreEqual(true, navView.IsPaneOpen);
            });
        }

        [TestMethod]
        public void VerifySingleSelection()
        {
            NavigationViewItem menuItem1 = null;
            NavigationViewItem menuItem2 = null;
            NavigationView navView = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                MUXControlsTestApp.App.TestContentRoot = navView;

                menuItem1 = new NavigationViewItem();
                menuItem2 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menuItem1.IsSelected);
                Verify.IsFalse(menuItem2.IsSelected);
                Verify.AreEqual(navView.SelectedItem, null);

                menuItem1.IsSelected = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem1.IsSelected);
                Verify.IsFalse(menuItem2.IsSelected);
                Verify.AreEqual(navView.SelectedItem, menuItem1);

                menuItem2.IsSelected = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem2.IsSelected);
                Verify.IsFalse(menuItem1.IsSelected, "MenuItem1 should have been deselected when MenuItem2 was selected");
                Verify.AreEqual(navView.SelectedItem, menuItem2);
            });
        }

        [TestMethod]
        public void VerifySettingsItemToolTip()
        {
            NavigationView navView = null;
            NavigationViewItem settingsItem = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();

                navView.IsSettingsVisible = true;
                navView.IsPaneOpen = true;
                MUXControlsTestApp.App.TestContentRoot = navView;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                settingsItem = (NavigationViewItem)navView.SettingsItem;

                var toolTip = ToolTipService.GetToolTip(settingsItem);
                Verify.IsNull(toolTip, "Verify tooltip is disabled when pane is open");

                navView.IsPaneOpen = false;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var toolTip = ToolTipService.GetToolTip(settingsItem);
                Verify.IsNotNull(toolTip, "Verify tooltip is enabled when pane is closed");

                MUXControlsTestApp.App.TestContentRoot = null;
            });
        }

#if BUILD_WINDOWS
        [TestMethod]
        [TestProperty("BUG", "RS3:12705080")]
        public void CanLoadSimpleNavigationView()
        {
            RunOnUIThread.Execute(() =>
            {
                XamlReader.Load(@"
                    <NavigationView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <NavigationView.MenuItems>
                            <NavigationViewItem Icon='Save' Content='Save' />
                        </NavigationView.MenuItems>
                        <TextBlock>Hello World</TextBlock>
                    </NavigationView>");
            });
        }
#endif

#if !BUILD_WINDOWS
        // Disabled per GitHub Issue #211
        //[TestMethod]
        public void VerifyCanNotAddWUXItems()
        {
            if (!ApiInformation.IsTypePresent("Windows.UI.Xaml.Controls.NavigationViewItem"))
            {
                Log.Warning("WUX version of NavigationViewItem only available starting in RS3.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                var muxItem = new Microsoft.UI.Xaml.Controls.NavigationViewItem { Content = "MUX Item" };
                navView.MenuItems.Add(muxItem);

                navView.MenuItems.Add(new Microsoft.UI.Xaml.Controls.NavigationViewItemSeparator());

                // No errors should occur here when we only use MUX items
                navView.UpdateLayout();

                var wuxItem = new Windows.UI.Xaml.Controls.NavigationViewItem { Content = "WUX Item" };
                navView.MenuItems.Add(wuxItem);

                // But adding a WUX item should generate an exception (as soon as the new item gets processed)
                Verify.Throws<Exception>(() => { navView.UpdateLayout(); });
            });
        }
#endif
        [TestMethod]
        public void VerifySelectionChangeWhenPreviouslySelectedItemIsHidden()
        {
            NavigationViewItem menuItem1 = null;
            NavigationViewItem menuItem2 = null;
            NavigationViewItem menuItem1_1 = null;
            NavigationView navView = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                MUXControlsTestApp.App.TestContentRoot = navView;

                menuItem1 = new NavigationViewItem();
                menuItem2 = new NavigationViewItem();
                menuItem1_1 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";
                menuItem1_1.Content = "Item 1_1";

                menuItem1.MenuItems.Add(menuItem1_1);
                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menuItem1.IsSelected);
                Verify.IsFalse(menuItem1.IsExpanded);
                Verify.IsFalse(menuItem2.IsSelected);
                Verify.AreEqual(navView.SelectedItem, null);

                menuItem1.IsExpanded = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem1.IsExpanded);
                Verify.AreEqual(navView.SelectedItem, null);

                menuItem1_1.IsSelected = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem1_1.IsSelected);
                Verify.AreEqual(navView.SelectedItem, menuItem1_1);

                menuItem1.IsExpanded = false;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menuItem1.IsExpanded);
                Verify.AreEqual(navView.SelectedItem, menuItem1_1);

                menuItem2.IsSelected = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem2.IsSelected);
                Verify.AreEqual(navView.SelectedItem, menuItem2);
            });
        }

        [TestMethod]
        public void VerifyCanExpandCollapseUsingAPI()
        {
            NavigationViewItem menuItem1 = null;
            NavigationViewItem menuItem2 = null;
            NavigationViewItem menuItem1_1 = null;
            NavigationView navView = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                MUXControlsTestApp.App.TestContentRoot = navView;

                menuItem1 = new NavigationViewItem();
                menuItem2 = new NavigationViewItem();
                menuItem1_1 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";
                menuItem1_1.Content = "Item 1_1";

                menuItem1.MenuItems.Add(menuItem1_1);
                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menuItem1.IsExpanded);

                navView.Expand(menuItem1);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(menuItem1.IsExpanded);

                navView.Collapse(menuItem1);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menuItem1.IsExpanded);
            });
        }
    }
}
