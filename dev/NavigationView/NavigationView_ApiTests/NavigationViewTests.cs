﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Common;
using System;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using NavigationViewDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationViewItemHeader = Microsoft.UI.Xaml.Controls.NavigationViewItemHeader;
using NavigationViewItemSeparator = Microsoft.UI.Xaml.Controls.NavigationViewItemSeparator;
using NavigationViewBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible;
using System.Collections.ObjectModel;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Composition;
using Windows.UI.Xaml.Markup;
using System.Collections.Generic;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class NavigationViewTests : ApiTestBase
    {
        private NavigationView SetupNavigationView(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Undo", Icon = new SymbolIcon(Symbol.Undo) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Cut", Icon = new SymbolIcon(Symbol.Cut) });

                navView.PaneTitle = "Title";
                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.IsSettingsVisible = true;
                navView.PaneDisplayMode = paneDisplayMode;
                navView.OpenPaneLength = 120.0;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 800.0;
                navView.Height = 600.0;
                navView.Content = "This is a simple test";
                Content = navView;
                Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        private NavigationView SetupNavigationViewScrolling(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #1", Icon = new SymbolIcon(Symbol.Undo) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #2", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItemHeader() { Content = "Item #3" });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #4", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #5", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItemSeparator());
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #7", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #8", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #9", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #10", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItemHeader() { Content = "Item #11" });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #12", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #13", Icon = new SymbolIcon(Symbol.Cut) });
                navView.MenuItems.Add(new NavigationViewItemSeparator());
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item #15", Icon = new SymbolIcon(Symbol.Cut) });

                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.IsSettingsVisible = true;
                navView.PaneDisplayMode = paneDisplayMode;
                navView.OpenPaneLength = 120.0;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 800.0;
                navView.Height = 200.0;
                navView.Content = "This test should have enough NavigationViewItems to scroll.";
                Content = navView;
                Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        private NavigationView SetupNavigationViewPaneContent(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Undo", Icon = new SymbolIcon(Symbol.Undo) });
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Cut", Icon = new SymbolIcon(Symbol.Cut) });

                // Navigation View Pane Elements
                Button headerButton = new Button();
                headerButton.Content = "Header Button";

                Button footerButton = new Button();
                footerButton.Content = "Footer Button";

                // NavigationView Content Elements
                Button contentButtonOne = new Button();
                contentButtonOne.Content = "Content Button One";

                Button contentButtonTwo = new Button();
                contentButtonTwo.Content = "Content Button Two";
                contentButtonTwo.Margin = new Thickness(50, 0, 0, 0);

                StackPanel contentStackPanel = new StackPanel();
                contentStackPanel.Children.Add(contentButtonOne);
                contentStackPanel.Children.Add(contentButtonTwo);

                // Set NavigationView Properties

                navView.PaneHeader = headerButton;
                navView.PaneFooter = footerButton;
                navView.Header = "NavigationView Header";
                navView.AutoSuggestBox = new AutoSuggestBox();
                navView.Content = contentStackPanel;
                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.IsSettingsVisible = true;
                navView.PaneDisplayMode = paneDisplayMode;
                navView.OpenPaneLength = 300.0;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 800.0;
                navView.Height = 600.0;
                Content = navView;
                Windows.UI.ViewManagement.ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        [TestMethod]
        public void VerifyVisualTree()
        {
            using(VisualTreeVerifier visualTreeVerifier = new VisualTreeVerifier())
            {
                // Generate a basic NavigationView verification file for all the pane display modes
                foreach (var paneDisplayMode in Enum.GetValues(typeof(NavigationViewPaneDisplayMode)))
                {
                    var filePrefix = "NavigationView" + paneDisplayMode;
                    NavigationViewPaneDisplayMode displayMode = (NavigationViewPaneDisplayMode)paneDisplayMode;

                    // We can skip generating a verification file for Left mode since Auto is achieving the same result.
                    if (displayMode == NavigationViewPaneDisplayMode.Left)
                    {
                        continue;
                    }

                    Log.Comment($"Verify visual tree for NavigationViewPaneDisplayMode: {paneDisplayMode}");
                    var navigationView = SetupNavigationView(displayMode);
                    visualTreeVerifier.VerifyVisualTreeNoException(root: navigationView, verificationFileNamePrefix: filePrefix);
                }

                Log.Comment($"Verify visual tree for NavigationViewScrolling");
                var leftNavViewScrolling = SetupNavigationViewScrolling(NavigationViewPaneDisplayMode.Left);
                visualTreeVerifier.VerifyVisualTreeNoException(root: leftNavViewScrolling, verificationFileNamePrefix: "NavigationViewScrolling");
                
                Log.Comment($"Verify visual tree for NavigationViewLeftPaneContent");
                var leftNavViewPaneContent = SetupNavigationViewPaneContent(NavigationViewPaneDisplayMode.Left);
                visualTreeVerifier.VerifyVisualTreeNoException(root: leftNavViewPaneContent, verificationFileNamePrefix: "NavigationViewLeftPaneContent");

                Log.Comment($"Verify visual tree for NavigationViewTopPaneContent");
                var topNavViewPaneContent = SetupNavigationViewPaneContent(NavigationViewPaneDisplayMode.Top);
                visualTreeVerifier.VerifyVisualTreeNoException(root: topNavViewPaneContent, verificationFileNamePrefix: "NavigationViewTopPaneContent");
            }
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
        public void VerifyPaneDisplayModeChangingPaneAccordingly()
        {
            var navView = SetupNavigationView();

            foreach(var paneDisplayMode in Enum.GetValues(typeof(NavigationViewPaneDisplayMode)))
            {
                RunOnUIThread.Execute(() =>
                {
                    navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal;
                    navView.IsPaneOpen = false;
                    // Set it below threshold width for auto mode
                    navView.Width = navView.CompactModeThresholdWidth - 20;

                    Content.UpdateLayout();

                    navView.PaneDisplayMode = (NavigationViewPaneDisplayMode)paneDisplayMode;

                    Content.UpdateLayout();

                    // We only want to open the pane when explicitly set, otherwise it should be closed
                    // since we set the width below the threshold width
                    Verify.AreEqual((NavigationViewPaneDisplayMode)paneDisplayMode == NavigationViewPaneDisplayMode.Left, navView.IsPaneOpen);
                });
            }
        }

        [TestMethod]
        public void VerifyPaneDisplayModeAndIsPaneOpenInterplayOnNavViewLaunch()
        {
            Log.Comment("--- PaneDisplayMode: LeftMinimal ---");
            VerifyForPaneDisplayModeLeftMinimal();

            Log.Comment("--- PaneDisplayMode: LeftCompact ---");
            VerifyForPaneDisplayModeLeftCompact();

            Log.Comment("--- PaneDisplayMode: Left ---");
            VerifyForPaneDisplayModeLeft();

            Log.Comment("--- PaneDisplayMode: Auto ---");
            VerifyForPaneDisplayModeAuto();

            void VerifyForPaneDisplayModeLeftMinimal()
            {
                Log.Comment("Verify pane is closed when IsPaneOpen=true");
                var navView = SetupNavView(NavigationViewPaneDisplayMode.LeftMinimal, true);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });

                Log.Comment("Verify pane is closed when IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.LeftMinimal, false);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });
            }

            void VerifyForPaneDisplayModeLeftCompact()
            {
                Log.Comment("Verify pane is closed when IsPaneOpen=true");
                var navView = SetupNavView(NavigationViewPaneDisplayMode.LeftCompact, true);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });

                Log.Comment("Verify pane is closed when IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.LeftCompact, false);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });
            }

            void VerifyForPaneDisplayModeLeft()
            {
                Log.Comment("Verify pane is open when IsPaneOpen=true");
                var navView = SetupNavView(NavigationViewPaneDisplayMode.Left, true);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(navView.IsPaneOpen, "NavigationView pane should have been open");
                });

                Log.Comment("Verify pane is closed when IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Left, false);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });
            }

            void VerifyForPaneDisplayModeAuto()
            {
                Log.Comment("Verify pane is closed when launched in minimal state and IsPaneOpen=true");
                var navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, true, NavigationViewDisplayMode.Minimal);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });
                
                Log.Comment("Verify pane is closed when launched in compact state and IsPaneOpen=true");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, true, NavigationViewDisplayMode.Compact);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });

                Log.Comment("Verify pane is open when launched in expanded state and IsPaneOpen=true");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, true, NavigationViewDisplayMode.Expanded);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(navView.IsPaneOpen, "NavigationView pane should have been open");
                });

                Log.Comment("Verify pane is closed when launched in minimal state and IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, false, NavigationViewDisplayMode.Minimal);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });

                Log.Comment("Verify pane is closed when launched in compact state and IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, false, NavigationViewDisplayMode.Compact);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });

                Log.Comment("Verify pane is closed when launched in expanded state and IsPaneOpen=false");
                navView = SetupNavView(NavigationViewPaneDisplayMode.Auto, false, NavigationViewDisplayMode.Expanded);

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsFalse(navView.IsPaneOpen, "NavigationView pane should have been closed");
                });
            }

            NavigationView SetupNavView(NavigationViewPaneDisplayMode paneDisplayMode, bool isPaneOpen, NavigationViewDisplayMode displayMode = NavigationViewDisplayMode.Expanded)
            {
                NavigationView navView = null;
                RunOnUIThread.Execute(() =>
                {
                    navView = new NavigationView();
                    navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });

                    navView.PaneDisplayMode = paneDisplayMode;
                    navView.IsPaneOpen = isPaneOpen;
                    navView.ExpandedModeThresholdWidth = 600.0;
                    navView.CompactModeThresholdWidth = 400.0;

                    if (paneDisplayMode == NavigationViewPaneDisplayMode.Auto)
                    {
                        switch (displayMode)
                        {
                            case NavigationViewDisplayMode.Minimal:
                                navView.Width = navView.CompactModeThresholdWidth - 10.0;
                                break;
                            case NavigationViewDisplayMode.Compact:
                                navView.Width = navView.ExpandedModeThresholdWidth - 10.0;
                                break;
                            case NavigationViewDisplayMode.Expanded:
                                navView.Width = navView.ExpandedModeThresholdWidth + 10.0;
                                break;
                        }
                    }

                    Content = navView;
                });

                IdleSynchronizer.Wait();
                return navView;
            }
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
                Verify.IsTrue(navView.IsTitleBarAutoPaddingEnabled);
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
                navView.IsTitleBarAutoPaddingEnabled = false;
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
                Verify.IsFalse(navView.IsTitleBarAutoPaddingEnabled);
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
            string navItemPresenter1CurrentState = string.Empty;
            string navItemPresenter2CurrentState = string.Empty;
            NavigationView navView = null;
            NavigationViewItem menuItem1 = null;
            NavigationViewItem menuItem2 = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                Content = navView;

                menuItem1 = new NavigationViewItem();
                menuItem2 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
                Content.UpdateLayout();

                var menuItemLayoutRoot = VisualTreeHelper.GetChild(menuItem1, 0) as FrameworkElement;
                var navItemPresenter = VisualTreeHelper.GetChild(menuItemLayoutRoot, 0) as FrameworkElement;
                var navItemPresenterLayoutRoot = VisualTreeHelper.GetChild(navItemPresenter, 0) as FrameworkElement;
                var statesGroups = VisualStateManager.GetVisualStateGroups(navItemPresenterLayoutRoot);

                foreach (var visualStateGroup in statesGroups)
                {
                    Log.Comment($"VisualStateGroup1: Name={visualStateGroup.Name}, CurrentState={visualStateGroup.CurrentState.Name}");

                    visualStateGroup.CurrentStateChanged += (object sender, VisualStateChangedEventArgs e) =>
                    {
                        Log.Comment($"VisualStateChangedEventArgs1: Name={e.Control.Name}, OldState={e.OldState.Name}, NewState={e.NewState.Name}");
                        navItemPresenter1CurrentState = e.NewState.Name;
                    };
                }

                menuItemLayoutRoot = VisualTreeHelper.GetChild(menuItem2, 0) as FrameworkElement;
                navItemPresenter = VisualTreeHelper.GetChild(menuItemLayoutRoot, 0) as FrameworkElement;
                navItemPresenterLayoutRoot = VisualTreeHelper.GetChild(navItemPresenter, 0) as FrameworkElement;
                statesGroups = VisualStateManager.GetVisualStateGroups(navItemPresenterLayoutRoot);

                foreach (var visualStateGroup in statesGroups)
                {
                    Log.Comment($"VisualStateGroup2: Name={visualStateGroup.Name}, CurrentState={visualStateGroup.CurrentState.Name}");

                    visualStateGroup.CurrentStateChanged += (object sender, VisualStateChangedEventArgs e) =>
                    {
                        Log.Comment($"VisualStateChangedEventArgs2: Name={e.Control.Name}, OldState={e.OldState.Name}, NewState={e.NewState.Name}");
                        navItemPresenter2CurrentState = e.NewState.Name;
                    };
                }

                Verify.IsFalse(menuItem1.IsSelected);
                Verify.IsFalse(menuItem2.IsSelected);
                Verify.AreEqual(null, navView.SelectedItem);

                menuItem1.IsSelected = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Selected", navItemPresenter1CurrentState);
                Verify.AreEqual(string.Empty, navItemPresenter2CurrentState);

                Verify.IsTrue(menuItem1.IsSelected);
                Verify.IsFalse(menuItem2.IsSelected);
                Verify.AreEqual(menuItem1, navView.SelectedItem);

                menuItem2.IsSelected = true;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Normal", navItemPresenter1CurrentState);
                Verify.AreEqual("Selected", navItemPresenter2CurrentState);

                Verify.IsTrue(menuItem2.IsSelected);
                Verify.IsFalse(menuItem1.IsSelected, "MenuItem1 should have been deselected when MenuItem2 was selected");
                Verify.AreEqual(menuItem2, navView.SelectedItem);
            });
        }

        [TestMethod]
        public void VerifyClosedCompactVisualState()
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                Content = navView;

                var template = (DataTemplate)XamlReader.Load(@"
                    <DataTemplate
                        xmlns ='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:NavigationViewItem
                            Content='Item'>
                            <controls:NavigationViewItem.Icon>
                                <SymbolIcon Symbol='Home' />
                            </controls:NavigationViewItem.Icon>
                         </controls:NavigationViewItem>
                      </DataTemplate>");
                navView.MenuItemTemplate = template;
                navView.IsPaneOpen = false;
                navView.IsSettingsVisible = false;

                RoutedEventHandler loaded = null;
                loaded = (object sender, RoutedEventArgs args) =>
                {
                    navView.Loaded -= loaded;

                    var items = new List<object>() { new object() }; ;
                    navView.MenuItemsSource = items;
                };
                navView.Loaded += loaded;

                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var footerRepeater = VisualTreeUtils.FindVisualChildByName(navView, "MenuItemsHost") as FrameworkElement;
                var menuItem = VisualTreeHelper.GetChild(footerRepeater, 0) as FrameworkElement;
                var menuItemLayoutRoot = VisualTreeHelper.GetChild(menuItem, 0) as FrameworkElement;
                var navItemPresenter = VisualTreeHelper.GetChild(menuItemLayoutRoot, 0) as FrameworkElement;
                var navItemPresenterLayoutRoot = VisualTreeHelper.GetChild(navItemPresenter, 0) as FrameworkElement;
                var statesGroups = VisualStateManager.GetVisualStateGroups(navItemPresenterLayoutRoot);

                string navItemPresenter1CurrentState = null;
                foreach (var visualStateGroup in statesGroups)
                {
                    if (visualStateGroup.Name == "PaneAndTopLevelItemStates")
                    {
                        navItemPresenter1CurrentState = visualStateGroup.CurrentState.Name;
                    }
                }

                Verify.AreEqual("ClosedCompactAndTopLevelItem", navItemPresenter1CurrentState);
            });
        }


        [TestMethod]
        public void VerifyNavigationItemUIAType()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                Content = navView;

                var menuItem1 = new NavigationViewItem();
                var menuItem2 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
                Content.UpdateLayout();

                Verify.AreEqual(
                    AutomationControlType.ListItem,
                    NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem1).GetAutomationControlType());
                Verify.IsNull(NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem1).GetPattern(PatternInterface.Invoke));

                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;
                Content.UpdateLayout();
                Verify.AreEqual(
                    AutomationControlType.TabItem,
                    NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem1).GetAutomationControlType());
                // Tabs should only provide SelectionItem pattern but not Invoke pattern
                Verify.IsNull(NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem1).GetPattern(PatternInterface.Invoke));
            });
        }

        [TestMethod]
        public void VerifyAutomationPeerExpandCollapsePatternBehavior()
        {
            RunOnUIThread.Execute(() =>
            {

                var menuItem1 = new NavigationViewItem();
                var menuItem2 = new NavigationViewItem();
                var menuItem3 = new NavigationViewItem();
                var menuItem4 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";
                menuItem3.Content = "Item 3";
                menuItem4.Content = "Item 4";

                menuItem2.MenuItems.Add(menuItem3);
                menuItem4.HasUnrealizedChildren = true;

                var expandPeer = NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem1).GetPattern(PatternInterface.ExpandCollapse);

                Verify.IsNull(expandPeer,"Verify NavigationViewItem with no children has no ExpandCollapse pattern");

                expandPeer = NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem2).GetPattern(PatternInterface.ExpandCollapse);
                Verify.IsNotNull(expandPeer,"Verify NavigationViewItem with children has an ExpandCollapse pattern provided");

                expandPeer = NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem4).GetPattern(PatternInterface.ExpandCollapse);
                Verify.IsNotNull(expandPeer,"Verify NavigationViewItem without children but with UnrealizedChildren set to true has an ExpandCollapse pattern provided");
            });
        }

        [TestMethod]
        public void VerifySettingsItemToolTip()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                navView.IsSettingsVisible = true;
                navView.IsPaneOpen = true;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                Content = navView;
                Content.UpdateLayout();
                var settingsItem = (NavigationViewItem)navView.SettingsItem;

                var toolTip = ToolTipService.GetToolTip(settingsItem);
                Verify.IsNull(toolTip, "Verify tooltip is disabled when pane is open");

                navView.IsPaneOpen = false;
                Content.UpdateLayout();

                toolTip = ToolTipService.GetToolTip(settingsItem);
                Verify.IsNotNull(toolTip, "Verify tooltip is enabled when pane is closed");
            });
        }

        [TestMethod]
        public void VerifySettingsItemTag()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                navView.IsSettingsVisible = true;
                navView.IsPaneOpen = true;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                Content = navView;
                Content.UpdateLayout();
                var settingsItem = (NavigationViewItem)navView.SettingsItem;
                Verify.AreEqual(settingsItem.Tag, "Settings");
            });
        }

        // Disabled per GitHub Issue #211
        [TestMethod]
        [TestProperty("Ignore", "True")]
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

        [TestMethod]
        public void VerifyHeaderContentMarginOnTopNav()
        {
            VerifyHeaderContentMargin(NavigationViewPaneDisplayMode.Top, "VerifyVerifyHeaderContentMarginOnTopNav");
        }

        [TestMethod]
        public void VerifyHeaderContentMarginOnMinimalNav()
        {
            VerifyHeaderContentMargin(NavigationViewPaneDisplayMode.LeftMinimal, "VerifyVerifyHeaderContentMarginOnMinimalNav");
        }

        private void VerifyHeaderContentMargin(NavigationViewPaneDisplayMode paneDisplayMode, string verificationFileNamePrefix)
        {
            UIElement headerContent = null;

            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView() { Header = "HEADER", PaneDisplayMode = paneDisplayMode, Width = 400.0 };
                Content = navView;
                Content.UpdateLayout();
                Grid rootGrid = VisualTreeHelper.GetChild(navView, 0) as Grid;
                if (rootGrid != null)
                {
                    headerContent = rootGrid.FindName("HeaderContent") as UIElement;
                }
            });

            VisualTreeTestHelper.VerifyVisualTree(
                root: headerContent,
                verificationFileNamePrefix: verificationFileNamePrefix);
        }

        [TestMethod]
        public void VerifyMenuItemAndContainerMappingMenuItemsSource()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                MUXControlsTestApp.App.TestContentRoot = navView;

                navView.MenuItemsSource = new ObservableCollection<string> { "Item 1", "Item 2" };
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders

                MUXControlsTestApp.App.TestContentRoot.UpdateLayout();

                var menuItem = "Item 2";
                // Get container for item
                var itemContainer = navView.ContainerFromMenuItem(menuItem) as NavigationViewItem;
                bool correctContainerReturned = itemContainer != null && (itemContainer.Content as string) == menuItem;
                Verify.IsTrue(correctContainerReturned, "Correct container should be returned for passed in menu item.");

                // Get item for container
                var returnedItem = navView.MenuItemFromContainer(itemContainer) as string;
                bool correctItemReturned = returnedItem != null && returnedItem == menuItem;
                Verify.IsTrue(correctItemReturned, "Correct item should be returned for passed in container.");

                MUXControlsTestApp.App.TestContentRoot = null;
            });
        }

        [TestMethod]
        public void VerifyMenuItemAndContainerMappingMenuItems()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                MUXControlsTestApp.App.TestContentRoot = navView;

                var menuItem1 = new NavigationViewItem();
                var menuItem2 = new NavigationViewItem();
                menuItem1.Content = "Item 1";
                menuItem2.Content = "Item 2";

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders

                MUXControlsTestApp.App.TestContentRoot.UpdateLayout();

                // Get container for item
                var itemContainer = navView.ContainerFromMenuItem(menuItem2) as NavigationViewItem;
                bool correctContainerReturned = itemContainer != null && itemContainer == menuItem2;
                Verify.IsTrue(correctContainerReturned, "Correct container should be returned for passed in menu item.");

                // Get item for container
                var returnedItem = navView.MenuItemFromContainer(menuItem2) as NavigationViewItem;
                bool correctItemReturned = returnedItem != null && returnedItem == menuItem2;
                Verify.IsTrue(correctItemReturned, "Correct item should be returned for passed in container.");

                // Try to get an item that is not in the NavigationView
                NavigationViewItem menuItem3 = new NavigationViewItem();
                menuItem3.Content = "Item 3";
                var returnedItemForNonExistentContainer = navView.MenuItemFromContainer(menuItem3);
                Verify.IsTrue(returnedItemForNonExistentContainer == null, "Returned item should be null.");

                MUXControlsTestApp.App.TestContentRoot = null;
            });
        }

        [TestMethod]
        public void VerifySelectedItemIsNullWhenNoItemIsSelected()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                Content = navView;

                var menuItem1 = new NavigationViewItem();
                menuItem1.Content = "Item 1";

                navView.MenuItems.Add(menuItem1);
                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
                Content.UpdateLayout();

                Verify.IsFalse(menuItem1.IsSelected);
                Verify.AreEqual(null, navView.SelectedItem);

                menuItem1.IsSelected = true;
                Content.UpdateLayout();

                Verify.IsTrue(menuItem1.IsSelected);
                Verify.AreEqual(menuItem1, navView.SelectedItem);

                menuItem1.IsSelected = false;
                Content.UpdateLayout();

                Verify.IsFalse(menuItem1.IsSelected);
                Verify.AreEqual(null, navView.SelectedItem, "SelectedItem should have been [null] as no item is selected");
            });
        }

        [TestMethod]
        public void VerifyNavigationViewItemInFooterDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                Content = navView;

                var navViewItem = new NavigationViewItem() { Content = "Footer item" };

                navView.PaneFooter = navViewItem;

                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
                Content.UpdateLayout();

                // If we don't get here, app has crashed. This verify is just making sure code got run
                Verify.IsTrue(true);
            });
        }

        [TestMethod]
        public void VerifyExpandCollapseChevronVisibility()
        {
            NavigationView navView = null;
            NavigationViewItem parentItem = null;
            ObservableCollection<string> children = null;

            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                Content = navView;

                children = new ObservableCollection<string>();
                parentItem = new NavigationViewItem() { Content = "ParentItem", MenuItemsSource = children };

                navView.MenuItems.Add(parentItem);

                navView.Width = 1008; // forces the control into Expanded mode so that the menu renders
                Content.UpdateLayout();

                UIElement chevronUIElement = (UIElement)VisualTreeUtils.FindVisualChildByName(parentItem, "ExpandCollapseChevron");
                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Collapsed, "chevron should have been collapsed as NavViewItem has no children");

                // Add a child to parentItem through the MenuItemsSource API. This should make the chevron visible.
                children.Add("Child 1");
                Content.UpdateLayout();

                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Visible, "chevron should have been visible as NavViewItem now has children");

                // Remove all children of parentItem. This should collapse the chevron
                children.Clear();
                Content.UpdateLayout();

                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Collapsed, "chevron should have been collapsed as NavViewItem no longer has children");

                // Add a child to parentItem and set the MenuItemsSource as null. This should collapse the chevron
                children.Add("Child 2");
                Content.UpdateLayout();

                // we are doing this so that when we set MenuItemsSource as null, we can check if the chevron's visibility really changes
                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Visible, "chevron should have been visible as NavViewItem now has children");

                parentItem.MenuItemsSource = null;
                Content.UpdateLayout();

                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Collapsed, "chevron should have been collapsed as NavViewItem no longer has children");

                // Add a child to parentItem through the MenuItems API. This should make the chevron visible.
                parentItem.MenuItems.Add(new NavigationViewItem() { Content = "Child 3" });
                Content.UpdateLayout();

                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Visible, "chevron should have been visible as NavViewItem now has children");

                // Remove all children of parentItem. This should collapse the chevron
                parentItem.MenuItems.Clear();
                Content.UpdateLayout();

                Verify.IsTrue(chevronUIElement.Visibility == Visibility.Collapsed, "chevron should have been collapsed as NavViewItem no longer has children");
            });
        }

        [TestMethod]
        public void VerifyOverflowButtonToolTip()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;

                Content = navView;
                Content.UpdateLayout();

                var overflowButton = VisualTreeUtils.FindVisualChildByName(navView, "TopNavOverflowButton") as Button;
                var toolTipObject = ToolTipService.GetToolTip(overflowButton);

                bool testCondition = toolTipObject is ToolTip toolTip && toolTip.Content.Equals("More");
                Verify.IsTrue(testCondition, "ToolTip text should have been \"More\".");
            });
        }

        [TestMethod]
        public void VerifyClearingItemsCollectionDoesNotCrashWhenItemSelectedOnTopNav()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;

                var navViewItem1 = new NavigationViewItem() { Content = "MenuItem 1", };
                var navViewItem2 = new NavigationViewItem() { Content = "MenuItem 2" };

                Log.Comment("Set up the MenuItems collection");
                navView.MenuItems.Add(navViewItem1);
                navView.MenuItems.Add(navViewItem2);

                Content = navView;
                Content.UpdateLayout();

                Log.Comment("Set MenuItem 1 as selected");
                navView.SelectedItem = navViewItem1;
                Verify.AreEqual(navViewItem1, navView.SelectedItem, "MenuItem 1 should have been selected");

                // Clearing the MenuItems collection should not crash the app
                Log.Comment("Clear the MenuItems collection");
                navView.MenuItems.Clear();

                Log.Comment("Set up the MenuItemsSource collection");
                var itemsSource = new ObservableCollection<NavigationViewItem>() { navViewItem1, navViewItem2 };
                navView.MenuItemsSource = itemsSource;

                Content.UpdateLayout();

                Log.Comment("Set MenuItem 1 as selected");
                navView.SelectedItem = navViewItem1;
                Verify.AreEqual(navViewItem1, navView.SelectedItem, "MenuItem 1 should have been selected");

                // Clearing the MenuItemsSource collection should not crash the app
                Log.Comment("Clear the MenuItemsSource collection");
                itemsSource.Clear();
            });
        }

        [TestMethod]
        public void VerifyHierarchicalNavigationTopModeMenuItemsSourceDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                Content = navView;

                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;

                var childItem = new NavigationViewItem() { Content = "Item 1.1" };
                var parentItem = new NavigationViewItem() { Content = "Item 1", MenuItemsSource = new ObservableCollection<NavigationViewItem>() { childItem } };
                navView.MenuItemsSource = new ObservableCollection<NavigationViewItem>() { parentItem };             

                Content.UpdateLayout();
            });
        }

        [TestMethod]
        public void VerifyNavigationViewItemToolTipCreation()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("On RS4 and earlier the test needs to be modified slightly.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                // Item with null content
                var menuItem1 = new NavigationViewItem();

                // Item with empty string as content
                var menuItem2 = new NavigationViewItem();
                menuItem2.Content = "";

                // Item with null content and custom tooltip
                var menuItem3 = new NavigationViewItem();
                ToolTipService.SetToolTip(menuItem3, "Custom tooltip");

                // Item with non-empty string content
                var menuItem4 = new NavigationViewItem();
                menuItem4.Content = "Item 4";

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);
                navView.MenuItems.Add(menuItem3);
                navView.MenuItems.Add(menuItem4);

                // Use a pane configuration where tooltips are shown.
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                navView.IsPaneOpen = false;

                Content = navView;
                navView.UpdateLayout();

                Verify.AreEqual(null, ToolTipService.GetToolTip(menuItem1), "Item 1's tooltip should have been [null].");
                Verify.AreEqual(null, ToolTipService.GetToolTip(menuItem2), "Item 2's tooltip should have been [null].");
                Verify.AreEqual("Custom tooltip", ToolTipService.GetToolTip(menuItem3), "Item 3's tooltip should have been \"Custom tooltip\".");
                Verify.AreEqual("Item 4", ToolTipService.GetToolTip(menuItem4), "Item 4's tooltip should have been  \"Item 4\".");
            });
        }

        [TestMethod]
        public void VerifyNavigationViewItemToolTipPaneDisplayMode()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("On RS4 and earlier the test needs to be modified slightly.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();

                // Item with non-empty string content using in-built tooltip
                var menuItem1 = new NavigationViewItem();
                menuItem1.Content = "Item 1";

                // Item with custom tooltip
                var menuItem2 = new NavigationViewItem();
                menuItem2.Content = "Item 2";
                ToolTipService.SetToolTip(menuItem2, "Custom tooltip");

                navView.MenuItems.Add(menuItem1);
                navView.MenuItems.Add(menuItem2);

                Content = navView;

                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Left, false, "Item 1", "Custom tooltip");
                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Left, true, null, "Custom tooltip");

                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.LeftCompact, false, "Item 1", "Custom tooltip");
                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.LeftCompact, true, null, "Custom tooltip");

                // Show tooltips again
                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Left, false, "Item 1", "Custom tooltip");

                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.LeftMinimal, true, null, "Custom tooltip");

                // Show tooltips again
                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Left, false, "Item 1", "Custom tooltip");

                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Top, false, null, "Custom tooltip");

                // Show tooltips again
                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Left, false, "Item 1", "Custom tooltip");

                SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode.Top, true, null, "Custom tooltip");

                void SetPaneConfigAndVerifyToolTips(NavigationViewPaneDisplayMode paneDisplayMode, bool isPaneOpen, string expectedDefaultToolTip, string expectedCustomToolTip)
                {
                    Log.Comment($"Verifying tooltips with PaneDisplayMode=[{paneDisplayMode}] and IsPaneOpen=[{isPaneOpen}]");
                    navView.PaneDisplayMode = paneDisplayMode;
                    navView.IsPaneOpen = isPaneOpen;
                    Content.UpdateLayout();

                    Verify.AreEqual(expectedDefaultToolTip, ToolTipService.GetToolTip(menuItem1), $"Item 1's tooltip should have been \"{expectedDefaultToolTip ?? "null"}\".");
                    Verify.AreEqual(expectedCustomToolTip, ToolTipService.GetToolTip(menuItem2), $"Item 2's tooltip should have been {expectedCustomToolTip}.");
                }
            });
        }
    }
}