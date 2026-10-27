// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;

using Common;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.Foundation.Metadata;
using Windows.UI.ViewManagement;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Shapes;
using System.Runtime.InteropServices;
using Windows.UI.Core;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using System.Threading;
using System.Security.Cryptography.X509Certificates;
using MUXControlsTestApp.Samples;
using Windows.UI.ViewManagement;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    public class Category
    {
        public String Content { get; set; }
        public String Name { get; set; }
        public String Icon { get; set; }
        public ObservableCollection<Category> Children { get; set; }
        public bool SelectsOnInvoked { get; set; }

        public Category(String content, String name, String icon, ObservableCollection<Category> children, bool selectsOnInvoked)
        {
            this.Content = content;
            this.Name = name;
            this.Icon = icon;
            this.Children = children;
            this.SelectsOnInvoked = selectsOnInvoked;
        }
    }

    [global::Microsoft.UI.Xaml.Data.Bindable]
    public class NavigationViewChildrenData : global::System.ComponentModel.INotifyPropertyChanged
    {
        private ObservableCollection<string> children = new ObservableCollection<string>();
        private global::System.ComponentModel.PropertyChangedEventHandler propertyChanged;
        private string content;

        public string Content
        {
            get => content;
            set
            {
                content = value;
                propertyChanged?.Invoke(this, new global::System.ComponentModel.PropertyChangedEventArgs(nameof(Content)));
            }
        }
        public ObservableCollection<string> OtherChildren { get; } = new ObservableCollection<string>();
        public int SubscriberCount => propertyChanged?.GetInvocationList().Length ?? 0;
        public ObservableCollection<string> Children
        {
            get => children;
            set
            {
                children = value;
                propertyChanged?.Invoke(this, new global::System.ComponentModel.PropertyChangedEventArgs(nameof(Children)));
            }
        }
        public event global::System.ComponentModel.PropertyChangedEventHandler PropertyChanged
        {
            add => propertyChanged += value;
            remove => propertyChanged -= value;
        }
    }

    [TestClass]
    public class NavigationViewTests : ApiTestBase
    {
        private NavigationView SetupNavigationView(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                var undoNVI = new NavigationViewItem() { Content = "Undo", Icon = new SymbolIcon(Symbol.Undo) };
                undoNVI.Resources.Add("NavigationViewItemIconBackground", new SolidColorBrush(Colors.Red));
                navView.MenuItems.Add(undoNVI);
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

                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
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

                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
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

                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        private NavigationView SetupNavigationViewHierarchy(NavigationViewPaneDisplayMode paneDisplayMode = NavigationViewPaneDisplayMode.Auto)
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();

                NavigationViewItem navViewItem1 = new NavigationViewItem() { Content = "NVI1" };
                navViewItem1.MenuItems.Add(new NavigationViewItem() { Content = "NVI1" });

                NavigationViewItem navViewItem2 = new NavigationViewItem() { Content = "NVI2" };

                navView.MenuItems.Add(navViewItem1);
                navView.MenuItems.Add(navViewItem2);

                navView.PaneTitle = "Hierarchical NavView";
                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.IsSettingsVisible = true;
                navView.PaneDisplayMode = paneDisplayMode;
                navView.OpenPaneLength = 120.0;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 800.0;
                navView.Height = 600.0;
                navView.Content = "This is a simple Hierarchical Navigation View test";
                Content = navView;

                try
                {
                    ApplicationView.GetForCurrentView().TryEnterFullScreenMode();
                }
                catch (COMException)
                {
                    // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                }
            });

            IdleSynchronizer.Wait();
            return navView;
        }

        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
        public void VerifyVisualTree_OptimizedStyles()
        {
            VerifyVisualTreeHelper("_OptimizedStyles");
        }

        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
        [TestProperty("Data:XamlOptionalChanges", "{DefaultStyleOptimizations:false}")]
        public void VerifyVisualTree_OldStyles()
        {
            VerifyVisualTreeHelper("_OldStyles");
        }

        public void VerifyVisualTreeHelper(string defaultStyleOptimizationsString)
        {
            using (VisualTreeVerifier visualTreeVerifier = new VisualTreeVerifier())
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
                visualTreeVerifier.VerifyVisualTreeNoException(root: leftNavViewScrolling, verificationFileNamePrefix: "NavigationViewScrolling" + defaultStyleOptimizationsString);

                Log.Comment($"Verify visual tree for NavigationViewLeftPaneContent");
                var leftNavViewPaneContent = SetupNavigationViewPaneContent(NavigationViewPaneDisplayMode.Left);
                visualTreeVerifier.VerifyVisualTreeNoException(root: leftNavViewPaneContent, verificationFileNamePrefix: "NavigationViewLeftPaneContent");

                Log.Comment($"Verify visual tree for NavigationViewTopPaneContent");
                var topNavViewPaneContent = SetupNavigationViewPaneContent(NavigationViewPaneDisplayMode.Top);
                visualTreeVerifier.VerifyVisualTreeNoException(root: topNavViewPaneContent, verificationFileNamePrefix: "NavigationViewTopPaneContent");

                Log.Comment($"Verify visual tree for NavigationViewHierarchy");
                var hierarchyNavViewPaneContent = SetupNavigationViewHierarchy(NavigationViewPaneDisplayMode.Left);
                visualTreeVerifier.VerifyVisualTreeNoException(root: hierarchyNavViewPaneContent, verificationFileNamePrefix: "NavigationViewHierarchy");
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

            foreach (var paneDisplayMode in Enum.GetValues(typeof(NavigationViewPaneDisplayMode)))
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
        [TestProperty("Description", "Verifies IsPaneOpen=false set before PaneDisplayMode=Left is respected on launch (GitHub #7609).")]
        public void VerifyIsPaneOpenFalseRespectedWhenSetBeforePaneDisplayModeLeft()
        {
            Log.Comment("Code: IsPaneOpen=false, then PaneDisplayMode=Left");
            VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                return navView;
            }, expectedIsPaneOpen: false);

            Log.Comment("XAML: IsPaneOpen='False' PaneDisplayMode='Left'");
            VerifyLaunchState(() =>
            {
                return (NavigationView)XamlReader.Load(@"
                    <controls:NavigationView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        IsPaneOpen='False'
                        PaneDisplayMode='Left'>
                        <controls:NavigationView.MenuItems>
                            <controls:NavigationViewItem Content='MenuItem' />
                        </controls:NavigationView.MenuItems>
                    </controls:NavigationView>");
            }, expectedIsPaneOpen: false);

            Log.Comment("Code: IsPaneOpen=false, PaneDisplayMode=LeftCompact, then PaneDisplayMode=Left");
            VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftCompact;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                return navView;
            }, expectedIsPaneOpen: false);

            Log.Comment("Code: IsPaneOpen not set, PaneDisplayMode=Left (default open pane is unchanged)");
            VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                return navView;
            }, expectedIsPaneOpen: true);

            Log.Comment("Code: IsPaneOpen=false, PaneDisplayMode=Left, then IsPaneOpen=true before load (open wins)");
            VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                navView.IsPaneOpen = true;
                return navView;
            }, expectedIsPaneOpen: true);

            Log.Comment("Code: IsPaneOpen=false, PaneDisplayMode=LeftCompact, then Auto launching in Expanded");
            VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftCompact;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Auto;
                navView.ExpandedModeThresholdWidth = 600.0;
                navView.CompactModeThresholdWidth = 400.0;
                navView.Width = 610.0;
                return navView;
            }, expectedIsPaneOpen: false);

            Log.Comment("Runtime: after launching closed, LeftCompact -> Left opens the pane (post-template PaneDisplayMode change still clears the force-close flag)");
            var launchedNavViewCompact = VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                return navView;
            }, expectedIsPaneOpen: false);

            RunOnUIThread.Execute(() => launchedNavViewCompact.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftCompact);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => launchedNavViewCompact.PaneDisplayMode = NavigationViewPaneDisplayMode.Left);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(launchedNavViewCompact.IsPaneOpen, "NavigationView.IsPaneOpen after runtime LeftCompact -> Left");
                var splitView = VisualTreeUtils.FindVisualChildByName(launchedNavViewCompact, "RootSplitView") as SplitView;
                Verify.IsTrue(splitView.IsPaneOpen, "RootSplitView.IsPaneOpen after runtime LeftCompact -> Left");
            });

            Log.Comment("Runtime: after launching closed, LeftMinimal -> Left opens the pane (by-design explicit OpenPane in UpdatePaneDisplayMode)");
            var launchedNavView = VerifyLaunchState(() =>
            {
                var navView = new NavigationView();
                navView.MenuItems.Add(new NavigationViewItem() { Content = "MenuItem" });
                navView.IsPaneOpen = false;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                return navView;
            }, expectedIsPaneOpen: false);

            RunOnUIThread.Execute(() => launchedNavView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => launchedNavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(launchedNavView.IsPaneOpen, "Pane should open on a runtime LeftMinimal -> Left switch");
            });

            NavigationView VerifyLaunchState(Func<NavigationView> createNavView, bool expectedIsPaneOpen)
            {
                NavigationView navView = null;
                RunOnUIThread.Execute(() =>
                {
                    navView = createNavView();
                    Content = navView;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(NavigationViewDisplayMode.Expanded, navView.DisplayMode);
                    Verify.AreEqual(expectedIsPaneOpen, navView.IsPaneOpen, "NavigationView.IsPaneOpen");

                    var splitView = VisualTreeUtils.FindVisualChildByName(navView, "RootSplitView") as SplitView;
                    Verify.IsNotNull(splitView);
                    Verify.AreEqual(expectedIsPaneOpen, splitView.IsPaneOpen, "RootSplitView.IsPaneOpen");
                });

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
        [TestProperty("Description", "Verifies the pane toggle button has its full width on first render and follows CompactPaneLength (#5854, #9034).")]
        public void VerifyPaneToggleButtonWidthOnFirstRender()
        {
            // PaneDisplayMode == null means "leave the default (Auto) untouched": setting PaneDisplayMode,
            // even to Auto, initializes the pane button widths early and hides the bug.
            var testCases = new[] {
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)null, Width = 1200.0 }, // Expanded, pane open
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)null, Width = 800.0 },  // Compact, pane closed
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)null, Width = 500.0 },  // Minimal, pane closed
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)NavigationViewPaneDisplayMode.Left, Width = 1200.0 },
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)NavigationViewPaneDisplayMode.LeftCompact, Width = 1200.0 },
                new { PaneDisplayMode = (NavigationViewPaneDisplayMode?)NavigationViewPaneDisplayMode.LeftMinimal, Width = 1200.0 },
            };

            foreach (var testCase in testCases)
            {
                Log.Comment("Verifying PaneDisplayMode: " + (testCase.PaneDisplayMode?.ToString() ?? "default") + ", Width: " + testCase.Width);

                NavigationView navView = null;
                RunOnUIThread.Execute(() =>
                {
                    // No PaneTitle: with a PaneTitle the toggle button is stretched to OpenPaneLength instead.
                    navView = new NavigationView();
                    if (testCase.PaneDisplayMode.HasValue)
                    {
                        navView.PaneDisplayMode = testCase.PaneDisplayMode.Value;
                    }
                    navView.MenuItems.Add(new NavigationViewItem() { Content = "Item", Icon = new SymbolIcon(Symbol.Home) });
                    navView.Width = testCase.Width;
                    navView.Height = 600.0;
                    Content = navView;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                    Verify.IsNotNull(toggleButton);
                    Log.Comment("DisplayMode: " + navView.DisplayMode + ", IsPaneOpen: " + navView.IsPaneOpen);
                    Verify.AreEqual(navView.CompactPaneLength, toggleButton.ActualWidth, "TogglePaneButton should be CompactPaneLength wide on first render");

                    navView.CompactPaneLength = 60.0;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                    Verify.AreEqual(60.0, toggleButton.ActualWidth, "TogglePaneButton should follow CompactPaneLength changes");
                    Content = null;
                });
                IdleSynchronizer.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with a PaneTitle the pane toggle button follows runtime OpenPaneLength and NavigationView width changes.")]
        public void VerifyPaneToggleButtonWidthFollowsOpenPaneLengthWithPaneTitle()
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                // With a PaneTitle and the pane open, the toggle button is stretched to the effective OpenPaneLength.
                navView = new NavigationView();
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                navView.PaneTitle = "Title";
                navView.OpenPaneLength = 250.0;
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item", Icon = new SymbolIcon(Symbol.Home) });
                navView.Width = 1200.0;
                navView.Height = 600.0;
                Content = navView;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                Verify.IsNotNull(toggleButton);
                Verify.IsTrue(navView.IsPaneOpen);
                Verify.AreEqual(250.0, toggleButton.Width, "TogglePaneButton should be OpenPaneLength wide on first render");

                navView.OpenPaneLength = 280.0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                Verify.AreEqual(280.0, toggleButton.Width, "TogglePaneButton should follow OpenPaneLength changes");

                // A NavigationView narrower than OpenPaneLength clamps the effective open pane length to its width.
                navView.Width = 200.0;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                Verify.AreEqual(200.0, toggleButton.Width, "TogglePaneButton should follow the clamped open pane length");
                Content = null;
            });
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that an OpenPaneLength smaller than the close button does not give the pane toggle button a negative width.")]
        public void VerifyPaneToggleButtonWidthIsNotNegativeWithSmallOpenPaneLength()
        {
            NavigationView navView = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView();
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal;
                navView.PaneTitle = "Title";
                navView.CompactPaneLength = 24.0;
                navView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
                navView.MenuItems.Add(new NavigationViewItem() { Content = "Item", Icon = new SymbolIcon(Symbol.Home) });
                navView.Width = 1200.0;
                navView.Height = 600.0;
                Content = navView;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                navView.IsPaneOpen = true;
            });
            IdleSynchronizer.Wait();

            // The open overlay (LeftMinimal) pane shows the 40px close button, so the toggle button width is
            // OpenPaneLength - 40, which is negative here. Both steps re-compute it while the pane stays open.
            var steps = new (string Name, Action Change, double ExpectedWidth)[] {
                ("OpenPaneLength = 32", () => navView.OpenPaneLength = 32.0, 0.0),
                ("CompactPaneLength = 28", () => navView.CompactPaneLength = 28.0, 0.0),
            };

            foreach (var step in steps)
            {
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment(step.Name);
                    try
                    {
                        step.Change();
                    }
                    catch (Exception e)
                    {
                        Verify.Fail(step.Name + " threw: " + e.Message);
                    }
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var toggleButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TogglePaneButton");
                    Verify.IsNotNull(toggleButton);
                    Log.Comment("IsPaneOpen: " + navView.IsPaneOpen + ", TogglePaneButton.Width: " + toggleButton.Width);
                    Verify.IsTrue(navView.IsPaneOpen);
                    Verify.AreEqual(step.ExpectedWidth, toggleButton.Width, "TogglePaneButton.Width should be OpenPaneLength - 40 clamped to 0");
                });
                IdleSynchronizer.Wait();
            }

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });
            IdleSynchronizer.Wait();
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
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
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
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
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
        public void VerifySelectionWithOpacityTransition()
        {
            if (!new UISettings().AnimationsEnabled)
            {
                Log.Result(TestResult.Skipped, "Selection indicator animations must be enabled to exercise this regression.");
                return;
            }

            foreach (var paneDisplayMode in new[] { NavigationViewPaneDisplayMode.Left, NavigationViewPaneDisplayMode.Top })
            {
                var navView = SetupNavigationView(paneDisplayMode);
                NavigationViewItem first = null;
                NavigationViewItem second = null;
                ScalarTransition transition = null;

                RunOnUIThread.Execute(() =>
                {
                    first = (NavigationViewItem)navView.MenuItems[0];
                    second = (NavigationViewItem)navView.MenuItems[1];
                    transition = new ScalarTransition();
                    navView.OpacityTransition = transition;
                    Verify.Throws<UnauthorizedAccessException>(() =>
                    {
                        Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual(navView);
                    });
                    navView.SelectedItem = first;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(first.IsSelected);
                    Verify.IsNotNull(VisualTreeUtils.FindVisualChildByName(first, "SelectionIndicator"));
                    Verify.IsNotNull(VisualTreeUtils.FindVisualChildByName(second, "SelectionIndicator"));
                    navView.SelectedItem = second;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(second, navView.SelectedItem);
                    Verify.IsTrue(second.IsSelected);
                    Verify.IsFalse(first.IsSelected);
                    Verify.AreEqual(transition, navView.OpacityTransition);
                    navView.Opacity = 0.5;
                    navView.SelectedItem = first;
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(first, navView.SelectedItem);
                    Verify.IsTrue(first.IsSelected);
                    Verify.IsFalse(second.IsSelected);
                    Verify.AreEqual(0.5, navView.Opacity);
                    navView.Opacity = 1.0;
                });
            }
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
        public void VerifyAutomationPeerInvokesNonSelectingItemOnce()
        {
            VerifyAutomationPeerSelectionInvocation(selectsOnInvoked: false);
        }

        [TestMethod]
        public void VerifyAutomationPeerInvokesSelectingItemOnce()
        {
            VerifyAutomationPeerSelectionInvocation(selectsOnInvoked: true);
        }

        private void VerifyAutomationPeerSelectionInvocation(bool selectsOnInvoked)
        {
            foreach (var paneDisplayMode in new[] { NavigationViewPaneDisplayMode.Left, NavigationViewPaneDisplayMode.Top })
            {
                foreach (var hasPreviousSelection in new[] { false, true })
                {
                    foreach (var addToSelection in new[] { false, true })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            Log.Comment($"PaneDisplayMode={paneDisplayMode}, SelectsOnInvoked={selectsOnInvoked}, HasPreviousSelection={hasPreviousSelection}, AddToSelection={addToSelection}");
                            var previousItem = new NavigationViewItem { Content = "Previous" };
                            var targetItem = new NavigationViewItem { Content = "Target", SelectsOnInvoked = selectsOnInvoked };
                            var navView = new NavigationView { PaneDisplayMode = paneDisplayMode, Width = 1008 };
                            navView.MenuItems.Add(previousItem);
                            navView.MenuItems.Add(targetItem);
                            Content = navView;
                            Content.UpdateLayout();
                            navView.SelectedItem = hasPreviousSelection ? previousItem : null;

                            var invokedItems = new List<NavigationViewItemBase>();
                            navView.ItemInvoked += (sender, args) => invokedItems.Add(args.InvokedItemContainer);
                            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(targetItem);
                            var provider = peer.GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                            Verify.IsNotNull(provider);

                            for (int invocation = 0; invocation < 3; invocation++)
                            {
                                invokedItems.Clear();
                                // Keyboard accelerators use the SelectionItem pattern when Invoke is unavailable.
                                if (addToSelection)
                                {
                                    provider.AddToSelection();
                                }
                                else
                                {
                                    provider.Select();
                                }

                                Verify.AreEqual(1, invokedItems.Count, "Each automation action should invoke exactly once.");
                                Verify.AreEqual(targetItem, invokedItems[0], "Only the target item should be invoked.");
                                Verify.AreEqual(selectsOnInvoked ? targetItem : (hasPreviousSelection ? previousItem : null), navView.SelectedItem);
                                Verify.AreEqual(selectsOnInvoked, targetItem.IsSelected);
                                Verify.AreEqual(selectsOnInvoked, provider.IsSelected);
                                Verify.AreEqual(!selectsOnInvoked && hasPreviousSelection, previousItem.IsSelected);
                            }
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyNonSelectingInvocationDoesNotAffectSubsequentSelectionChanges()
        {
            VerifyInvocationDoesNotAffectSubsequentSelectionChanges(selectsOnInvoked: false);
        }

        [TestMethod]
        public void VerifySelectedItemInvocationDoesNotAffectSubsequentSelectionChanges()
        {
            VerifyInvocationDoesNotAffectSubsequentSelectionChanges(selectsOnInvoked: true);
        }

        private void VerifyInvocationDoesNotAffectSubsequentSelectionChanges(bool selectsOnInvoked)
        {
            foreach (var paneDisplayMode in new[] { NavigationViewPaneDisplayMode.Left, NavigationViewPaneDisplayMode.Top })
            {
                foreach (var addToSelection in new[] { false, true })
                {
                    foreach (var removeSelection in new[] { false, true })
                    {
                        RunOnUIThread.Execute(() =>
                        {
                            Log.Comment($"PaneDisplayMode={paneDisplayMode}, SelectsOnInvoked={selectsOnInvoked}, AddToSelection={addToSelection}, RemoveSelection={removeSelection}");
                            var previousItem = new NavigationViewItem { Content = "Previous" };
                            var targetItem = new NavigationViewItem { Content = "Target", SelectsOnInvoked = selectsOnInvoked };
                            var nextItem = new NavigationViewItem { Content = "Next" };
                            var navView = new NavigationView { PaneDisplayMode = paneDisplayMode, Width = 1008 };
                            navView.MenuItems.Add(previousItem);
                            navView.MenuItems.Add(targetItem);
                            navView.MenuItems.Add(nextItem);
                            Content = navView;
                            Content.UpdateLayout();
                            var selectedItem = selectsOnInvoked ? targetItem : previousItem;
                            navView.SelectedItem = selectedItem;

                            var invokedItems = new List<NavigationViewItemBase>();
                            navView.ItemInvoked += (sender, args) => invokedItems.Add(args.InvokedItemContainer);
                            var provider = FrameworkElementAutomationPeer.CreatePeerForElement(targetItem)
                                .GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                            Verify.IsNotNull(provider);
                            if (addToSelection)
                            {
                                provider.AddToSelection();
                            }
                            else
                            {
                                provider.Select();
                            }

                            Verify.AreEqual(1, invokedItems.Count);
                            Verify.AreEqual(targetItem, invokedItems[0]);
                            Verify.AreEqual(selectedItem, navView.SelectedItem);

                            if (removeSelection)
                            {
                                var selectedProvider = FrameworkElementAutomationPeer.CreatePeerForElement(selectedItem)
                                    .GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                                Verify.IsNotNull(selectedProvider);
                                selectedProvider.RemoveFromSelection();
                            }
                            else
                            {
                                navView.SelectedItem = nextItem;
                            }

                            Verify.AreEqual(1, invokedItems.Count, "A later selection change must not invoke another item.");
                            Verify.AreEqual(removeSelection ? null : nextItem, navView.SelectedItem);
                            Verify.IsFalse(selectedItem.IsSelected);
                            Verify.AreEqual(!removeSelection, nextItem.IsSelected);
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyItemInvokedHandlerCanChangeSelectionWithoutInvokingAgain()
        {
            foreach (var paneDisplayMode in new[] { NavigationViewPaneDisplayMode.Left, NavigationViewPaneDisplayMode.Top })
            {
                foreach (var selectsOnInvoked in new[] { false, true })
                {
                    foreach (var clearSelection in new[] { false, true })
                    {
                        NavigationView navView = null;
                        NavigationViewItem previousItem = null;
                        NavigationViewItem targetItem = null;
                        NavigationViewItem nextItem = null;
                        var events = new List<string>();
                        using var selectionChangedEvent = new ManualResetEvent(false);
                        RunOnUIThread.Execute(() =>
                        {
                            Log.Comment($"PaneDisplayMode={paneDisplayMode}, SelectsOnInvoked={selectsOnInvoked}, ClearSelection={clearSelection}");
                            previousItem = new NavigationViewItem { Content = "Previous" };
                            targetItem = new NavigationViewItem { Content = "Target", SelectsOnInvoked = selectsOnInvoked };
                            nextItem = new NavigationViewItem { Content = "Next" };
                            navView = new NavigationView { PaneDisplayMode = paneDisplayMode, Width = 1008 };
                            navView.MenuItems.Add(previousItem);
                            navView.MenuItems.Add(targetItem);
                            navView.MenuItems.Add(nextItem);
                            Content = navView;
                            Content.UpdateLayout();
                            navView.SelectedItem = selectsOnInvoked ? targetItem : previousItem;
                        });

                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            var invokedItems = new List<NavigationViewItemBase>();
                            navView.SelectionChanged += (sender, args) =>
                            {
                                events.Add("SelectionChanged");
                                selectionChangedEvent.Set();
                            };
                            navView.ItemInvoked += (sender, args) =>
                            {
                                invokedItems.Add(args.InvokedItemContainer);
                                events.Add("ItemInvoked");
                                if (invokedItems.Count == 1)
                                {
                                    navView.SelectedItem = clearSelection ? null : nextItem;
                                }
                            };

                            // The peer implements Invoke even though it does not expose the UIA Invoke pattern.
                            // Call it directly to exercise the shared invocation path without reselecting the item.
                            var provider = FrameworkElementAutomationPeer.CreatePeerForElement(targetItem) as IInvokeProvider;
                            Verify.IsNotNull(provider);
                            provider.Invoke();

                            Verify.AreEqual(1, invokedItems.Count, "Selection changes inside ItemInvoked must not invoke another item.");
                            Verify.AreEqual(targetItem, invokedItems[0]);
                            Verify.AreEqual(clearSelection ? null : nextItem, navView.SelectedItem);
                            Verify.IsFalse(previousItem.IsSelected);
                            Verify.IsFalse(targetItem.IsSelected);
                            Verify.AreEqual(!clearSelection, nextItem.IsSelected);
                        });

                        Verify.IsTrue(selectionChangedEvent.WaitOne(TimeSpan.FromSeconds(5)), "SelectionChanged should be raised for the handler's selection change.");
                        IdleSynchronizer.Wait();
                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual("ItemInvoked,SelectionChanged", string.Join(",", events));
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyAutomationPeerCanRemoveNonSelectingItemFromSelection()
        {
            RunOnUIThread.Execute(() =>
            {
                var targetItem = new NavigationViewItem { Content = "Target" };
                var navView = new NavigationView { PaneDisplayMode = NavigationViewPaneDisplayMode.Left, Width = 1008 };
                navView.MenuItems.Add(targetItem);
                Content = navView;
                Content.UpdateLayout();
                navView.SelectedItem = targetItem;
                targetItem.SelectsOnInvoked = false;
                Verify.IsTrue(targetItem.IsSelected);

                int invocationCount = 0;
                navView.ItemInvoked += (sender, args) => invocationCount++;
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(targetItem);
                var provider = peer.GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                Verify.IsNotNull(provider);
                provider.RemoveFromSelection();

                Verify.IsFalse(targetItem.IsSelected);
                Verify.IsFalse(provider.IsSelected);
                Verify.IsNull(navView.SelectedItem);
                Verify.AreEqual(0, invocationCount, "Removing selection should not invoke the item.");
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

                Verify.IsNull(expandPeer, "Verify NavigationViewItem with no children has no ExpandCollapse pattern");

                expandPeer = NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem2).GetPattern(PatternInterface.ExpandCollapse);
                Verify.IsNotNull(expandPeer, "Verify NavigationViewItem with children has an ExpandCollapse pattern provided");

                expandPeer = NavigationViewItemAutomationPeer.CreatePeerForElement(menuItem4).GetPattern(PatternInterface.ExpandCollapse);
                Verify.IsNotNull(expandPeer, "Verify NavigationViewItem without children but with UnrealizedChildren set to true has an ExpandCollapse pattern provided");
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

        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
        public void VerifyHeaderContentMarginOnTopNav()
        {
            VerifyHeaderContentMargin(NavigationViewPaneDisplayMode.Top, "VerifyVerifyHeaderContentMarginOnTopNav");
        }

        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", "22621")] // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
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
        public void VerifyHeaderIsNotClippedInLeftMinimalWhenBackAndPaneToggleButtonsAreHidden()
        {
            // Regression test for https://github.com/microsoft/microsoft-ui-xaml/issues/9789
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView()
                {
                    Header = "HEADER",
                    PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal,
                    IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                    IsPaneToggleButtonVisible = true,
                    IsPaneOpen = false,
                    Width = 400.0
                };
                Content = navView;
                Content.UpdateLayout();

                var headerContent = VisualTreeUtils.FindVisualChildByName(navView, "HeaderContent") as FrameworkElement;
                Verify.IsNotNull(headerContent);
                Verify.AreEqual(Visibility.Visible, headerContent.Visibility);

                double headerLeftWithToggleButton = headerContent.TransformToVisual(navView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X;
                Log.Comment($"Header left offset with only the pane toggle button visible: {headerLeftWithToggleButton}");

                navView.IsPaneToggleButtonVisible = false;
                Content.UpdateLayout();

                Verify.AreEqual(Visibility.Visible, headerContent.Visibility);
                double headerLeftWithNoButtons = headerContent.TransformToVisual(navView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X;
                Log.Comment($"Header left offset with neither back nor pane toggle button visible: {headerLeftWithNoButtons}");

                Verify.IsGreaterThanOrEqual(headerLeftWithNoButtons, 0.0, "Header should not be pushed past the left edge of the NavigationView");
                Verify.AreEqual(headerLeftWithToggleButton, headerLeftWithNoButtons, "Header should keep the same left inset as when only one button is visible");
            });
        }

        [TestMethod]
        public void VerifyHeaderPaddingUpdatesWhenSwitchingBetweenLeftMinimalAndTop()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (bool isPaneToggleButtonVisible in new[] { true, false })
                {
                    Log.Comment($"Verify mode transitions with IsPaneToggleButtonVisible={isPaneToggleButtonVisible}");

                    var topNavView = new NavigationView()
                    {
                        Header = "HEADER",
                        PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                        IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                        IsPaneToggleButtonVisible = isPaneToggleButtonVisible,
                        IsPaneOpen = false,
                        Width = 400.0,
                        Height = 280.0
                    };
                    Content = topNavView;
                    Content.UpdateLayout();

                    var topHeader = VisualTreeUtils.FindVisualChildByName(topNavView, "HeaderContent") as FrameworkElement;
                    var topPadding = VisualTreeUtils.FindVisualChildByName(topNavView, "ContentLeftPadding") as FrameworkElement;
                    Verify.IsNotNull(topHeader);
                    Verify.IsNotNull(topPadding);
                    Verify.AreEqual(0.0, topPadding.ActualWidth);
                    double topHeaderLeft = topHeader.TransformToVisual(topNavView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X;
                    double topHeaderWidth = topHeader.ActualWidth;

                    var navView = new NavigationView()
                    {
                        Header = "HEADER",
                        PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal,
                        IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                        IsPaneToggleButtonVisible = isPaneToggleButtonVisible,
                        IsPaneOpen = false,
                        Width = 400.0,
                        Height = 280.0
                    };
                    Content = navView;
                    Content.UpdateLayout();

                    var header = VisualTreeUtils.FindVisualChildByName(navView, "HeaderContent") as FrameworkElement;
                    var padding = VisualTreeUtils.FindVisualChildByName(navView, "ContentLeftPadding") as FrameworkElement;
                    var splitView = VisualTreeUtils.FindVisualChildByName(navView, "RootSplitView") as SplitView;
                    Verify.IsNotNull(header);
                    Verify.IsNotNull(padding);
                    Verify.IsNotNull(splitView);
                    Verify.AreEqual(SplitViewDisplayMode.Overlay, splitView.DisplayMode);
                    double minimalPadding = padding.ActualWidth;
                    double minimalHeaderLeft = header.TransformToVisual(navView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X;
                    double minimalHeaderWidth = header.ActualWidth;
                    Verify.IsGreaterThan(minimalPadding, 0.0, "LeftMinimal should reserve the header button slot");
                    Verify.IsGreaterThanOrEqual(minimalHeaderLeft, 0.0, "The LeftMinimal header should not be clipped");

                    for (int transition = 0; transition < 2; transition++)
                    {
                        navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;
                        Content.UpdateLayout();

                        Verify.AreEqual(SplitViewDisplayMode.Overlay, splitView.DisplayMode);
                        Verify.IsFalse(navView.IsPaneOpen);
                        Verify.AreEqual(0.0, padding.ActualWidth, "Top mode should clear the LeftMinimal header button slot");
                        Verify.AreEqual(topHeaderLeft, header.TransformToVisual(navView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X,
                            "Switching to Top should match the header position of a fresh Top NavigationView");
                        Verify.AreEqual(topHeaderWidth, header.ActualWidth, "Switching to Top should restore the full header width");

                        navView.PaneDisplayMode = NavigationViewPaneDisplayMode.LeftMinimal;
                        Content.UpdateLayout();

                        Verify.AreEqual(SplitViewDisplayMode.Overlay, splitView.DisplayMode);
                        Verify.IsFalse(navView.IsPaneOpen);
                        Verify.AreEqual(minimalPadding, padding.ActualWidth, "Returning to LeftMinimal should restore the header button slot");
                        Verify.AreEqual(minimalHeaderLeft, header.TransformToVisual(navView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).X);
                        Verify.AreEqual(minimalHeaderWidth, header.ActualWidth);
                    }
                }
            });
        }

        [TestMethod]
        public void VerifyHeaderPaddingUpdatesWhenCompactModeThresholdWidthChangesDisplayMode()
        {
            RunOnUIThread.Execute(() =>
            {
                const double minimalThreshold = 641.0;
                const double compactThreshold = 300.0;

                foreach (bool isPaneToggleButtonVisible in new[] { true, false })
                {
                    Log.Comment($"Verify CompactModeThresholdWidth changes with IsPaneToggleButtonVisible={isPaneToggleButtonVisible}");

                    Func<double, NavigationView> createNavView = (threshold) => new NavigationView()
                    {
                        Header = "HEADER",
                        PaneDisplayMode = NavigationViewPaneDisplayMode.Auto,
                        CompactModeThresholdWidth = threshold,
                        IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                        IsPaneToggleButtonVisible = isPaneToggleButtonVisible,
                        IsPaneOpen = false,
                        Width = 400.0,
                        Height = 280.0
                    };

                    Func<NavigationView, NavigationViewDisplayMode, double> getFreshPadding = (freshNavView, expectedDisplayMode) =>
                    {
                        Content = freshNavView;
                        Content.UpdateLayout();
                        Verify.AreEqual(expectedDisplayMode, freshNavView.DisplayMode);
                        var freshPadding = VisualTreeUtils.FindVisualChildByName(freshNavView, "ContentLeftPadding") as FrameworkElement;
                        Verify.IsNotNull(freshPadding);
                        return freshPadding.ActualWidth;
                    };

                    double freshMinimalPadding = getFreshPadding(createNavView(minimalThreshold), NavigationViewDisplayMode.Minimal);
                    double freshCompactPadding = getFreshPadding(createNavView(compactThreshold), NavigationViewDisplayMode.Compact);
                    Verify.IsGreaterThan(freshMinimalPadding, 0.0, "Minimal should reserve the header button slot");

                    var navView = createNavView(minimalThreshold);
                    Content = navView;
                    Content.UpdateLayout();
                    var padding = VisualTreeUtils.FindVisualChildByName(navView, "ContentLeftPadding") as FrameworkElement;
                    Verify.IsNotNull(padding);
                    Verify.AreEqual(NavigationViewDisplayMode.Minimal, navView.DisplayMode);
                    Verify.AreEqual(freshMinimalPadding, padding.ActualWidth);

                    for (int transition = 0; transition < 2; transition++)
                    {
                        navView.CompactModeThresholdWidth = compactThreshold;
                        Content.UpdateLayout();
                        Verify.AreEqual(NavigationViewDisplayMode.Compact, navView.DisplayMode);
                        Verify.AreEqual(freshCompactPadding, padding.ActualWidth, "Compact should match a fresh Compact NavigationView");

                        navView.CompactModeThresholdWidth = minimalThreshold;
                        Content.UpdateLayout();
                        Verify.AreEqual(NavigationViewDisplayMode.Minimal, navView.DisplayMode);
                        Verify.AreEqual(freshMinimalPadding, padding.ActualWidth, "Minimal should match a fresh Minimal NavigationView");
                    }
                }
            });
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
        public void VerifyTopNavigationViewIconOnlyChevronFits()
        {
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                    IsSettingsVisible = false,
                    Width = 1008
                };
                var item = new NavigationViewItem { Icon = new SymbolIcon(Symbol.Home) };
                navView.MenuItems.Add(item);
                Content = navView;
                Content.UpdateLayout();

                var chevron = (FrameworkElement)VisualTreeUtils.FindVisualChildByName(item, "ExpandCollapseChevron");
                var presenter = (FrameworkElement)VisualTreeUtils.FindVisualChildByName(item, "NavigationViewItemPresenter");
                var layoutRoot = (FrameworkElement)VisualTreeHelper.GetChild(presenter, 0);
                var leafWidth = layoutRoot.ActualWidth;
                Verify.AreEqual(Visibility.Collapsed, chevron.Visibility);

                item.MenuItems.Add(new NavigationViewItem { Content = "Child" });
                Content.UpdateLayout();
                Verify.AreEqual(Visibility.Visible, chevron.Visibility);
                Verify.IsTrue(chevron.ActualWidth > 0 && chevron.ActualHeight > 0);
                var bounds = chevron.TransformToVisual(layoutRoot).TransformBounds(
                    new global::Windows.Foundation.Rect(0, 0, chevron.ActualWidth, chevron.ActualHeight));
                Verify.IsTrue(bounds.Left >= -0.5 && bounds.Right <= layoutRoot.ActualWidth + 0.5,
                    "The visible chevron must fit inside the icon-only item's layout root.");

                item.MenuItems.Clear();
                Content.UpdateLayout();
                Verify.AreEqual(Visibility.Collapsed, chevron.Visibility);
                Verify.AreEqual(leafWidth, layoutRoot.ActualWidth, "Removing children should restore the icon-only leaf width.");
            });
        }

        [TestMethod]
        public void VerifyTopNavigationViewRecoversAfterChildrenAreRemoved()
        {
            foreach (bool useMenuItemsSource in new[] { false, true })
            {
                foreach (string childrenKind in new[] { "MenuItems", "SourceCollection", "SourceReplacement", "Unrealized" })
                {
                    foreach (bool openOverflow in new[] { false, true })
                    {
                        Log.Comment($"MenuItemsSource={useMenuItemsSource}, children={childrenKind}, open overflow={openOverflow}");
                        NavigationView navView = null;
                        NavigationViewItem parent = null;
                        ItemsRepeater primaryRepeater = null;
                        Button overflowButton = null;
                        var children = new ObservableCollection<NavigationViewItem>();

                        RunOnUIThread.Execute(() =>
                        {
                            navView = new NavigationView
                            {
                                PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                                IsSettingsVisible = false,
                                IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                                Width = 800,
                                Height = 300
                            };
                            var first = new NavigationViewItem { Content = "First", Width = 100 };
                            parent = new NavigationViewItem { Icon = new SymbolIcon(Symbol.Home) };
                            if (childrenKind == "SourceCollection")
                            {
                                parent.MenuItemsSource = children;
                            }
                            if (useMenuItemsSource)
                            {
                                navView.MenuItemsSource = new ObservableCollection<NavigationViewItem> { first, parent };
                            }
                            else
                            {
                                navView.MenuItems.Add(first);
                                navView.MenuItems.Add(parent);
                            }
                            navView.SelectedItem = first;
                            Content = navView;
                            Content.UpdateLayout();

                            primaryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(navView, "TopNavMenuItemsHost");
                            Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count);
                            var topGrid = (Grid)VisualTreeUtils.FindVisualChildByName(navView, "TopNavGrid");
                            var infiniteSize = new global::Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity);
                            topGrid.Measure(infiniteSize);
                            var leafWidth = topGrid.DesiredSize.Width;

                            var child = new NavigationViewItem { Content = "Child" };
                            switch (childrenKind)
                            {
                                case "MenuItems": parent.MenuItems.Add(child); break;
                                case "SourceCollection": children.Add(child); break;
                                case "SourceReplacement": parent.MenuItemsSource = new[] { child }; break;
                                case "Unrealized": parent.HasUnrealizedChildren = true; break;
                            }
                            Content.UpdateLayout();
                            topGrid.Measure(infiniteSize);
                            var parentWidth = topGrid.DesiredSize.Width;
                            Verify.IsTrue(parentWidth > leafWidth);
                            navView.Width = (leafWidth + parentWidth) / 2;
                            Content.UpdateLayout();
                        });
                        IdleSynchronizer.Wait();

                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                            overflowButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TopNavOverflowButton");
                            Verify.AreEqual(Visibility.Visible, overflowButton.Visibility);
                            if (openOverflow)
                            {
                                var provider = FrameworkElementAutomationPeer.CreatePeerForElement(overflowButton)
                                    .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                                Verify.IsNotNull(provider);
                                provider.Invoke();
                            }
                        });
                        IdleSynchronizer.Wait();

                        RunOnUIThread.Execute(() =>
                        {
                            switch (childrenKind)
                            {
                                case "MenuItems": parent.MenuItems.Clear(); break;
                                case "SourceCollection": children.Clear(); break;
                                case "SourceReplacement": parent.MenuItemsSource = null; break;
                                case "Unrealized": parent.HasUnrealizedChildren = false; break;
                            }
                            navView.InvalidateMeasure();
                            Content.UpdateLayout();
                        });
                        IdleSynchronizer.Wait();

                        RunOnUIThread.Execute(() =>
                        {
                            Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count,
                                "The smaller leaf must return from overflow without resizing NavigationView.");
                            Verify.AreEqual(Visibility.Collapsed, navView.TemplateSettings.OverflowButtonVisibility);
                            overflowButton.Flyout.Hide();
                        });
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyTopNavigationViewRecoversWithDataTemplatedItems()
        {
            VerifyTopNavigationViewRecoversWithDataTemplatedItems(false);
            VerifyTopNavigationViewRecoversWithDataTemplatedItems(true);
        }

        private void VerifyTopNavigationViewRecoversWithDataTemplatedItems(bool openOverflow)
        {
            NavigationView navView = null;
            ItemsRepeater primaryRepeater = null;
            Button overflowButton = null;
            var parent = new Category(null, "Parent", null, new ObservableCollection<Category>(), true);

            RunOnUIThread.Execute(() =>
            {
                var first = new Category("First navigation item", "First", null, new ObservableCollection<Category>(), true);
                navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                    IsSettingsVisible = false,
                    IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                    Width = 800,
                    Height = 300,
                    MenuItemsSource = new ObservableCollection<Category> { first, parent },
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <NavigationViewItem Icon='Home' Content='{Binding Content}' MenuItemsSource='{Binding Children}'/>
                          </DataTemplate>")
                };
                navView.SelectedItem = first;
                Content = navView;
                Content.UpdateLayout();
                primaryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(navView, "TopNavMenuItemsHost");
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count);
                var topGrid = (Grid)VisualTreeUtils.FindVisualChildByName(navView, "TopNavGrid");
                var infiniteSize = new global::Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity);
                topGrid.Measure(infiniteSize);
                var leafWidth = topGrid.DesiredSize.Width;

                parent.Children.Add(new Category("Child", "Child", null, new ObservableCollection<Category>(), true));
                Content.UpdateLayout();
                topGrid.Measure(infiniteSize);
                var parentWidth = topGrid.DesiredSize.Width;
                Verify.IsTrue(parentWidth > leafWidth);
                navView.Width = (leafWidth + parentWidth) / 2;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                overflowButton = (Button)VisualTreeUtils.FindVisualChildByName(navView, "TopNavOverflowButton");
                if (openOverflow)
                {
                    var provider = FrameworkElementAutomationPeer.CreatePeerForElement(overflowButton)
                        .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                    Verify.IsNotNull(provider);
                    provider.Invoke();
                }
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                parent.Children.Clear();
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count,
                    "Recycling data-templated containers must not keep invalidating the width cache.");
                Verify.AreEqual(Visibility.Collapsed, navView.TemplateSettings.OverflowButtonVisibility);
                overflowButton.Flyout.Hide();
            });
        }

        [TestMethod]
        public void VerifyTopNavigationViewTracksReplacedChildSources()
        {
            NavigationView navView = null;
            ItemsRepeater primaryRepeater = null;
            ObservableCollection<NavigationViewChildrenData> items = null;
            var first = new NavigationViewChildrenData { Content = "First navigation item" };
            var parent = new NavigationViewChildrenData { Content = "Parent" };
            var oldChildren = parent.Children;
            double availableWidth = 0;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<NavigationViewChildrenData> { first, parent };
                navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                    IsSettingsVisible = false,
                    IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                    Width = 800,
                    Height = 300,
                    MenuItemsSource = items,
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <NavigationViewItem Icon='Home' Content='{Binding Content}' MenuItemsSource='{Binding Children}'/>
                          </DataTemplate>")
                };
                navView.SelectedItem = first;
                Content = navView;
                Content.UpdateLayout();
                primaryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(navView, "TopNavMenuItemsHost");
                var topGrid = (Grid)VisualTreeUtils.FindVisualChildByName(navView, "TopNavGrid");
                var infiniteSize = new global::Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity);
                topGrid.Measure(infiniteSize);
                var leafWidth = topGrid.DesiredSize.Width;
                parent.Children.Add("Child");
                Content.UpdateLayout();
                topGrid.Measure(infiniteSize);
                Verify.IsTrue(topGrid.DesiredSize.Width > leafWidth);
                availableWidth = (leafWidth + topGrid.DesiredSize.Width) / 2;
                navView.Width = availableWidth;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                parent.Children = new ObservableCollection<string>();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(availableWidth, navView.Width);
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count,
                    "A recycled item must recover when its model replaces the child collection.");
                Verify.AreEqual(first, navView.SelectedItem);
                oldChildren.Add("Stale child");
                Content.UpdateLayout();
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count);
                parent.Children.Add("Replacement child");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                parent.Children.Clear();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count);
                parent.Children = new ObservableCollection<string>();
                items.Remove(parent);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, parent.SubscriberCount, "Removing the item must revoke its model-property subscription.");
                parent.Children = new ObservableCollection<string> { "Removed data" };
                Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                Verify.AreEqual(first, navView.SelectedItem);
            });
        }

        [TestMethod]
        public void VerifyTopNavigationViewObservesPhasedChildSource()
        {
            foreach (var templateKey in new[] { "UnphasedItem", "PhasedItem", "LatePhasedItem" })
            {
                NavigationView navView = null;
                ItemsRepeater primaryRepeater = null;
                var first = new NavigationViewChildrenData { Content = "First navigation item" };
                var parent = new NavigationViewChildrenData { Content = "Parent" };
                double availableWidth = 0;

                RunOnUIThread.Execute(() =>
                {
                    var templates = new NavigationViewPhasedTemplatePage();
                    navView = new NavigationView
                    {
                        PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                        IsSettingsVisible = false,
                        IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                        Width = 800,
                        Height = 300,
                        MenuItemsSource = new ObservableCollection<NavigationViewChildrenData> { first, parent },
                        MenuItemTemplate = (DataTemplate)templates.Resources[templateKey],
                        SelectedItem = first
                    };
                    Content = navView;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(parent.Children,
                        ((NavigationViewItem)navView.ContainerFromMenuItem(parent)).MenuItemsSource);
                    primaryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(navView, "TopNavMenuItemsHost");
                    var topGrid = (Grid)VisualTreeUtils.FindVisualChildByName(navView, "TopNavGrid");
                    var infiniteSize = new global::Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity);
                    topGrid.Measure(infiniteSize);
                    var leafWidth = topGrid.DesiredSize.Width;
                    parent.Children.Add("Child");
                    Content.UpdateLayout();
                    topGrid.Measure(infiniteSize);
                    Verify.IsTrue(topGrid.DesiredSize.Width > leafWidth);
                    availableWidth = (leafWidth + topGrid.DesiredSize.Width) / 2;
                    navView.Width = availableWidth;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                    Verify.IsNull(navView.ContainerFromMenuItem(parent));
                    parent.Children = new ObservableCollection<string> { "Replacement child" };
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                    Verify.IsNull(navView.ContainerFromMenuItem(parent));
                    parent.Children.Clear();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(availableWidth, navView.Width);
                    Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count,
                        templateKey + ": a recycled phased item must observe the replacement child collection.");
                    Verify.AreEqual(Visibility.Collapsed, navView.TemplateSettings.OverflowButtonVisibility);
                    Verify.AreEqual(first, navView.SelectedItem);
                });
            }
        }

        [TestMethod]
        public void VerifyTopNavigationViewObservesInitiallyNullChildSource()
        {
            NavigationView navView = null;
            ItemsRepeater primaryRepeater = null;
            var first = new NavigationViewChildrenData { Content = "First" };
            var parent = new NavigationViewChildrenData { Content = "Parent with a very long navigation label", Children = null };
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                    IsSettingsVisible = false,
                    IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                    Width = 800,
                    Height = 300,
                    MenuItemsSource = new ObservableCollection<NavigationViewChildrenData> { first, parent },
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <NavigationViewItem Icon='Home' Content='{Binding Content}' MenuItemsSource='{Binding Children}'/>
                          </DataTemplate>")
                };
                navView.SelectedItem = first;
                Content = navView;
                Content.UpdateLayout();
                primaryRepeater = (ItemsRepeater)VisualTreeUtils.FindVisualChildByName(navView, "TopNavMenuItemsHost");
                var topGrid = (Grid)VisualTreeUtils.FindVisualChildByName(navView, "TopNavGrid");
                topGrid.Measure(new global::Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity));
                navView.Width = topGrid.DesiredSize.Width - 10;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, primaryRepeater.ItemsSourceView.Count);
                parent.Children = new ObservableCollection<string> { "Child" };
                parent.Content = "P";
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, primaryRepeater.ItemsSourceView.Count);
                Verify.AreEqual(first, navView.SelectedItem);
            });
        }

        [TestMethod]
        public void VerifyNavigationViewRecyclesToTheOriginTemplate()
        {
            NavigationView navView = null;
            var item = new NavigationViewChildrenData { Content = "Parent" };
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                    IsSettingsVisible = false,
                    Width = 800,
                    Height = 300,
                    MenuItemsSource = new ObservableCollection<NavigationViewChildrenData> { item },
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                            <NavigationViewItem Content='{Binding Content}' MenuItemsSource='{Binding Children}'/>
                          </DataTemplate>")
                };
                Content = navView;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(item.Children, ((NavigationViewItem)navView.ContainerFromMenuItem(item)).MenuItemsSource);
                navView.MenuItemTemplate = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                        <NavigationViewItem Content='{Binding Content}' MenuItemsSource='{Binding OtherChildren}'/>
                      </DataTemplate>");
                navView.MenuItemsSource = new ObservableCollection<NavigationViewChildrenData> { item };
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(item.OtherChildren, ((NavigationViewItem)navView.ContainerFromMenuItem(item)).MenuItemsSource,
                    "The new template must not reuse an element created by the old template.");
            });
        }

        [TestMethod]
        public void VerifyOverflowButtonRemainsVisibleAfterSelectingOnlyOverflowItem()
        {
            foreach (bool useMenuItemsSource in new[] { false, true })
            {
                NavigationView navView = null;
                Button overflowButton = null;
                ItemsRepeater overflowRepeater = null;
                NavigationViewItem selectedItem = null;
                RunOnUIThread.Execute(() =>
                {
                    navView = new NavigationView
                    {
                        PaneDisplayMode = NavigationViewPaneDisplayMode.Top,
                        IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed,
                        IsSettingsVisible = false,
                        Width = 240,
                        Height = 300
                    };
                    var items = new ObservableCollection<NavigationViewItem>
                    {
                        new NavigationViewItem { Content = "AA" },
                        new NavigationViewItem { Content = "BBBBBBB" },
                        new NavigationViewItem { Content = "CCCCCCCCCCCCCCC" }
                    };
                    if (useMenuItemsSource)
                    {
                        navView.MenuItemsSource = items;
                    }
                    else
                    {
                        foreach (var item in items)
                        {
                            navView.MenuItems.Add(item);
                        }
                    }
                    selectedItem = items[2];
                    Content = navView;
                    Content.UpdateLayout();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    overflowButton = VisualTreeUtils.FindVisualChildByName(navView, "TopNavOverflowButton") as Button;
                    Verify.IsNotNull(overflowButton);
                    Verify.AreEqual(Visibility.Visible, overflowButton.Visibility);
                    var provider = FrameworkElementAutomationPeer.CreatePeerForElement(overflowButton)
                        .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                    Verify.IsNotNull(provider);
                    provider.Invoke();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    var flyout = overflowButton.Flyout as Flyout;
                    Verify.IsNotNull(flyout);
                    overflowRepeater = VisualTreeUtils.FindVisualChildByName(
                        flyout.Content, "TopNavMenuItemsOverflowHost") as ItemsRepeater;
                    Verify.IsNotNull(overflowRepeater);
                    Verify.AreEqual(1, overflowRepeater.ItemsSourceView.Count);
                    Verify.AreEqual(selectedItem, overflowRepeater.ItemsSourceView.GetAt(0));
                    var provider = FrameworkElementAutomationPeer.CreatePeerForElement(selectedItem) as IInvokeProvider;
                    Verify.IsNotNull(provider);
                    provider.Invoke();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(selectedItem, navView.SelectedItem);
                    Verify.AreEqual(2, overflowRepeater.ItemsSourceView.Count, "Both displaced items should remain reachable.");
                    Verify.AreEqual(Visibility.Visible, navView.TemplateSettings.OverflowButtonVisibility);
                    Verify.AreEqual(Visibility.Visible, overflowButton.Visibility);
                    (FrameworkElementAutomationPeer.CreatePeerForElement(overflowButton)
                        .GetPattern(PatternInterface.Invoke) as IInvokeProvider).Invoke();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(overflowButton.Flyout.IsOpen);
                    selectedItem = overflowRepeater.TryGetElement(0) as NavigationViewItem;
                    Verify.IsNotNull(selectedItem);
                    (FrameworkElementAutomationPeer.CreatePeerForElement(selectedItem) as IInvokeProvider).Invoke();
                });
                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(selectedItem, navView.SelectedItem);
                    Verify.IsTrue(overflowRepeater.ItemsSourceView.Count > 0);
                    Verify.AreEqual(Visibility.Visible, navView.TemplateSettings.OverflowButtonVisibility);
                });
            }
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

        [TestMethod]
        public void VerifyRemovedNavigationViewItemDoesNotReceivePaneChanges()
        {
            VerifyRemovedNavigationViewItemDoesNotReceiveSplitViewChanges(navView =>
            {
                navView.IsPaneOpen = false;
                navView.IsPaneOpen = true;
                Verify.IsTrue(navView.IsPaneOpen);
            });
        }

        [TestMethod]
        public void VerifyRemovedNavigationViewItemDoesNotReceiveDisplayModeChanges()
        {
            VerifyRemovedNavigationViewItemDoesNotReceiveSplitViewChanges(navView =>
            {
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top;
                navView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left;
                Verify.AreEqual(NavigationViewPaneDisplayMode.Left, navView.PaneDisplayMode);
            });
        }

        [TestMethod]
        public void VerifyRemovedNavigationViewItemDoesNotReceiveCompactPaneLengthChanges()
        {
            VerifyRemovedNavigationViewItemDoesNotReceiveSplitViewChanges(navView =>
            {
                navView.CompactPaneLength = 64;
                Verify.AreEqual(64.0, ((NavigationViewItem)navView.MenuItems[0]).CompactPaneLength);
            });
        }

        private void VerifyRemovedNavigationViewItemDoesNotReceiveSplitViewChanges(Action<NavigationView> updatePane)
        {
            NavigationView navView = null;
            NavigationViewItem parent = null;
            RunOnUIThread.Execute(() =>
            {
                navView = new NavigationView
                {
                    PaneDisplayMode = NavigationViewPaneDisplayMode.Left,
                    IsPaneOpen = true,
                    Width = 800,
                    Height = 600
                };
                parent = new NavigationViewItem { Content = "Parent", IsExpanded = true };
                for (int index = 0; index < 6; index++)
                {
                    parent.MenuItems.Add(new NavigationViewItem
                    {
                        Content = new TextBlock { Text = $"Child {index}" }
                    });
                }
                navView.MenuItems.Add(parent);
                Content = navView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                parent.MenuItems.Clear();
                parent.MenuItems.Add(new NavigationViewItem { Content = "Replacement" });
                // Keep the callback in the gap between GC disconnection and deferred native deletion.
                GC.Collect();
                updatePane(navView);
                parent.IsExpanded = true;
                Content.UpdateLayout();
                Verify.IsTrue(parent.IsExpanded);
                Verify.AreEqual(1, parent.MenuItems.Count);
                Verify.IsGreaterThan(parent.ActualHeight, 0.0);
            });
        }

        [TestMethod]
        public void VerifyNVIOutlivingNVDoesNotCrash()
        {
            NavigationViewItem menuItem1 = null;
            RunOnUIThread.Execute(() =>
            {
                var navView = new NavigationView();
                menuItem1 = new NavigationViewItem();

                navView.MenuItems.Add(menuItem1);
                Content = navView;
                Content.UpdateLayout();

                navView.MenuItems.Clear();
                Content = menuItem1;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                GC.Collect();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // NavigationView has a handler on NVI's IsSelected DependencyPropertyChangedEvent.
                menuItem1.IsSelected = !menuItem1.IsSelected;
            });
        }

        [TestMethod]
        public void VerifySelectingACollapsedItemShowsSelectionIndicator()
        {
            NavigationView navView = null;

            ObservableCollection<Category> subCategories = null;
            Category firstItem = null;
            Category secondItem = null;
            Category subItem = null;

            ManualResetEvent loadedEvent = new(false);
            ManualResetEvent selectionChangedEvent = new(false);

            RunOnUIThread.Execute(() =>
            {
                subItem = new("Menu Item 3", "MI3", "Icon", null, true);

                subCategories = new ObservableCollection<Category> {
                    subItem
                };

                firstItem = new("Menu Item 1", "MI1", "Icon", null, true);
                secondItem = new("Menu Item 2", "MI2", "Icon", subCategories, false);

                var categories = new ObservableCollection<Category> {
                    firstItem,
                    secondItem
                };

                navView = new NavigationView {
                    MenuItemsSource = categories,
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(@"
<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
  <NavigationViewItem Content='{Binding Content}' Name='{Binding Name}' MenuItemsSource='{Binding Children}' SelectsOnInvoked='{Binding SelectsOnInvoked}'/>
</DataTemplate>")
                };

                navView.Loaded += (sender, args) => loadedEvent.Set();
                navView.SelectionChanged += (sender, args) => selectionChangedEvent.Set();
                Content = navView;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            SelectItem(navView, subItem, selectionChangedEvent);
            Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 2"));
            SelectItem(navView, firstItem, selectionChangedEvent);
            Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 1"));

            Category newItem = null;

            RunOnUIThread.Execute(() =>
            {
                newItem = new("Menu Item 4", "MI4", "Icon", null, true);
                subCategories.Add(newItem);
            });

            SelectItem(navView, newItem, selectionChangedEvent);
            Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 2"));
            SelectItem(navView, firstItem, selectionChangedEvent);
            Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 1"));

            RunOnUIThread.Execute(() =>
            {
                var secondItemNvi = (NavigationViewItem)navView.ContainerFromMenuItem(secondItem);
                secondItemNvi.IsExpanded = true;
            });

            IdleSynchronizer.Wait();

            selectionChangedEvent.Reset();

            RunOnUIThread.Execute(() =>
            {
                Category newItem2 = new("Menu Item 5", "MI5", "Icon", null, true);
                subCategories.Add(newItem2);
                // This doesn't use SelectItem. I'm guessing we're testing for adding an item and immediately selecting
                // it, rather than waiting for it to render first.
                navView.SelectedItem = newItem2;
            });

            selectionChangedEvent.WaitOne();
            IdleSynchronizer.Wait();

            // Disabled - see Bug 50123187: NavigationViewTests.VerifySelectingACollapsedItemShowsSelectionIndicator is unstable
            //Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 5"));
        }

        [TestMethod]
        public void VerifyContainerIsNonNullWhenSelectingANewlyAddedItem()
        {
            NavigationView navView = null;

            ObservableCollection<Category> subCategories = null;
            Category firstItem = null;
            Category secondItem = null;
            Category subItem = null;

            ManualResetEvent loadedEvent = new(false);
            ManualResetEvent selectionChangedEvent = new(false);

            Action<NavigationViewSelectionChangedEventArgs> selectionChangedVerification = null;

            RunOnUIThread.Execute(() =>
            {
                subItem = new("Menu Item 3", "MI3", "Icon", null, true);

                subCategories = new ObservableCollection<Category> {
                    subItem
                };

                firstItem = new("Menu Item 1", "MI1", "Icon", null, true);
                secondItem = new("Menu Item 2", "MI2", "Icon", subCategories, false);

                var categories = new ObservableCollection<Category> {
                    firstItem,
                    secondItem
                };

                navView = new NavigationView {
                    MenuItemsSource = categories,
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(@"
<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
  <NavigationViewItem Content='{Binding Content}' Name='{Binding Name}' MenuItemsSource='{Binding Children}' SelectsOnInvoked='{Binding SelectsOnInvoked}'/>
</DataTemplate>")
                };

                navView.Loaded += (sender, args) => loadedEvent.Set();

                navView.SelectionChanged += (sender, args) =>
                {
                    selectionChangedVerification?.Invoke(args);
                    selectionChangedEvent.Set();
                };

                Content = navView;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            selectionChangedVerification = (args) =>
            {
                Verify.IsNotNull(args.SelectedItemContainer);
                Verify.AreEqual(subItem.Content, args.SelectedItemContainer.Content);
            };

            SelectItem(navView, subItem, selectionChangedEvent);

            Category newItem = null;

            RunOnUIThread.Execute(() =>
            {
                newItem = new("Menu Item 4", "MI4", "Icon", null, true);
                subCategories.Add(newItem);
            });

            selectionChangedVerification = (args) =>
            {
                Verify.IsNotNull(args.SelectedItemContainer);
                Verify.AreEqual(newItem.Content, args.SelectedItemContainer.Content);
            };

            SelectItem(navView, newItem, selectionChangedEvent);

            ManualResetEvent expandedEvent = new(false);

            RunOnUIThread.Execute(() =>
            {
                var secondItemNvi = (NavigationViewItem)navView.ContainerFromMenuItem(secondItem);

                var itemRepeater = GetRepeater(secondItemNvi);
                itemRepeater.ElementPrepared += (sender, args) =>
                {
                    if (((NavigationViewItem)args.Element).Content == "Menu Item 4")
                    {
                        expandedEvent.Set();
                    }
                };

                secondItemNvi.IsExpanded = true;
            });

            expandedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Category newItem2 = new("Menu Item 5", "MI5", "Icon", null, true);
                subCategories.Add(newItem2);

                selectionChangedVerification = (args) =>
                {
                    Verify.IsNotNull(args.SelectedItemContainer);
                    Verify.AreEqual(newItem2.Content, args.SelectedItemContainer.Content);
                };

                selectionChangedEvent.Reset();
                navView.SelectedItem = newItem2;
            });

            selectionChangedEvent.WaitOne();
            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void VerifySelectingANewItemAfterExpandingShowsSelectionIndicatorInTheCorrectLocation()
        {
            NavigationView navView = null;

            ObservableCollection<Category> subCategories = null;
            Category firstItem = null;
            Category secondItem = null;

            ManualResetEvent loadedEvent = new(false);
            ManualResetEvent selectionChangedEvent = new(false);

            RunOnUIThread.Execute(() =>
            {
                ObservableCollection<Category> subSubCategories = new ObservableCollection<Category> {
                    new("Menu Item 6", "MI6", "Icon", null, true)
                };

                subCategories = new ObservableCollection<Category> {
                    new("Menu Item 3", "MI3", "Icon", null, true),
                    new("Menu Item 4", "MI4", "Icon", subSubCategories, false),
                    new("Menu Item 5", "MI5", "Icon", null, true),
                };

                firstItem = new("Menu Item 1", "MI1", "Icon", null, true);
                secondItem = new("Menu Item 2", "MI2", "Icon", subCategories, false);

                var categories = new ObservableCollection<Category> {
                    firstItem,
                    secondItem
                };

                navView = new NavigationView {
                    MenuItemsSource = categories,
                    MenuItemTemplate = (DataTemplate)XamlReader.Load(@"
<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
  <NavigationViewItem Content='{Binding Content}' Name='{Binding Name}' MenuItemsSource='{Binding Children}' SelectsOnInvoked='{Binding SelectsOnInvoked}'/>
</DataTemplate>")
                };

                navView.Loaded += (sender, args) => loadedEvent.Set();
                navView.SelectionChanged += (sender, args) => selectionChangedEvent.Set();
                Content = navView;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            Category newItem = null;

            RunOnUIThread.Execute(() =>
            {
                newItem = new("Menu Item 7", "MI7", "Icon", null, true);
                subCategories.Add(newItem);
                navView.SelectedItem = newItem;

                var secondItemNvi = (NavigationViewItem)navView.ContainerFromMenuItem(secondItem);
                secondItemNvi.IsExpanded = true;
            });

            selectionChangedEvent.WaitOne();
            IdleSynchronizer.Wait();

            Verify.IsTrue(ItemHasSelectionIndicator(navView, "Menu Item 7"));
        }

        static ItemsRepeater GetRepeater(DependencyObject parent)
        {
            static ItemsRepeater GetNavigationViewRepeaterHelper(DependencyObject root)
            {
                if (root is ItemsRepeater itemsRepeater &&
                    (itemsRepeater.Name == "MenuItemsHost" ||
                    itemsRepeater.Name == "NavigationViewItemMenuItemsHost"))
                {
                    return itemsRepeater;
                }

                for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
                {
                    var repeater = GetNavigationViewRepeaterHelper(VisualTreeHelper.GetChild(root, i));

                    if (repeater != null)
                    {
                        return repeater;
                    }
                }

                return null;
            }

            return GetNavigationViewRepeaterHelper(parent);
        }

        static bool ItemHasSelectionIndicator(NavigationView navView, string expectedContent)
        {
            static bool ItemHasSelectionIndicatorHelper(DependencyObject root, string expectedContent, NavigationViewItemBase lastSeenItem)
            {
                if (root is Rectangle possibleSelectionIndicator &&
                    possibleSelectionIndicator.Name == "SelectionIndicator" &&
                    possibleSelectionIndicator.Opacity > 0 &&
                    lastSeenItem.Content == expectedContent)
                {
                    return true;
                }
                else if (root is NavigationViewItemBase item)
                {
                    lastSeenItem = item;
                }

                for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
                {
                    var hasSelectionIndicator = ItemHasSelectionIndicatorHelper(VisualTreeHelper.GetChild(root, i), expectedContent, lastSeenItem);

                    if (hasSelectionIndicator)
                    {
                        return true;
                    }
                }

                return false;
            }

            bool hasSelectionIndicator = false;

            RunOnUIThread.Execute(() =>
            {
                hasSelectionIndicator = ItemHasSelectionIndicatorHelper(navView, expectedContent, null);
            });

            return hasSelectionIndicator;
        }

        static void SelectItem(NavigationView navView, object item, ManualResetEvent selectionChangedEvent = null)
        {
            IdleSynchronizer.Wait();

            if (selectionChangedEvent != null)
            {
                selectionChangedEvent.Reset();
            }

            RunOnUIThread.Execute(() =>
            {
                navView.SelectedItem = item;
            });

            if (selectionChangedEvent != null)
            {
                selectionChangedEvent.WaitOne();
            }

            IdleSynchronizer.Wait();
        }
    }
}
