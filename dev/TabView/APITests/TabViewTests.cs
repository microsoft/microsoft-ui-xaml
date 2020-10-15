// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Threading;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;
using Common;
using Microsoft.UI.Xaml.Controls;
using System.Collections.Generic;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Provider;
using Windows.UI.Xaml.Controls;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Symbol = Windows.UI.Xaml.Controls.Symbol;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{

    [TestClass]
    public class TabViewTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyCompactTabWidthVisualStates()
        {
            TabView tabView = null;
            RunOnUIThread.Execute(() =>
            {
                tabView = new TabView();
                Content = tabView;

                tabView.TabItems.Add(CreateTabViewItem("Item 0", Symbol.Add));
                tabView.TabItems.Add(CreateTabViewItem("Item 1", Symbol.AddFriend));
                tabView.TabItems.Add(CreateTabViewItem("Item 2"));

                tabView.SelectedIndex = 0;
                tabView.SelectedItem = tabView.TabItems[0];
                (tabView.SelectedItem as TabViewItem).IsSelected = true;
                Verify.AreEqual("Item 0", (tabView.SelectedItem as TabViewItem).Header);
                Content.UpdateLayout();
            });
            // Waiting for layout
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Now set tab width mode
                tabView.TabWidthMode = TabViewWidthMode.Compact;
            });

            IdleSynchronizer.Wait();

            // Check if switching to compact updates all items correctly
            RunOnUIThread.Execute(() =>
            {
                VerifyTabWidthVisualStates(tabView.TabItems, true);
                tabView.TabItems.Add(CreateTabViewItem("Item 3"));
            });

            IdleSynchronizer.Wait();

            // Check if a newly added item has correct visual states
            RunOnUIThread.Execute(() =>
            {
                VerifyTabWidthVisualStates(tabView.TabItems, true);
                tabView.TabWidthMode = TabViewWidthMode.Equal;
            });

            IdleSynchronizer.Wait();

            // Switch back to non compact and check if every item has the correct visual state
            RunOnUIThread.Execute(() =>
            {
                VerifyTabWidthVisualStates(tabView.TabItems, false);
            });
        }

        [TestMethod]
        public void VerifyTabViewUIABehavior()
        {
            RunOnUIThread.Execute(() =>
            {
                TabView tabView = new TabView();
                Content = tabView;

                tabView.TabItems.Add(CreateTabViewItem("Item 0", Symbol.Add));
                tabView.TabItems.Add(CreateTabViewItem("Item 1", Symbol.AddFriend));
                tabView.TabItems.Add(CreateTabViewItem("Item 2"));

                Content.UpdateLayout();

                var tabViewPeer = FrameworkElementAutomationPeer.CreatePeerForElement(tabView);
                Verify.IsNotNull(tabViewPeer);
                var tabViewSelectionPattern = tabViewPeer.GetPattern(PatternInterface.Selection);
                Verify.IsNotNull(tabViewSelectionPattern);
                var selectionProvider = tabViewSelectionPattern as ISelectionProvider;
                // Tab controls must require selection
                Verify.IsTrue(selectionProvider.IsSelectionRequired);
            });
        }

        [TestMethod]
        public void VerifyTabViewItemUIABehavior()
        {
            TabView tabView = null;

            TabViewItem tvi0 = null;
            TabViewItem tvi1 = null;
            TabViewItem tvi2 = null;
            RunOnUIThread.Execute(() =>
            {
                tabView = new TabView();
                Content = tabView;

                tvi0 = CreateTabViewItem("Item 0", Symbol.Add);
                tvi1 = CreateTabViewItem("Item 1", Symbol.AddFriend);
                tvi2 = CreateTabViewItem("Item 2");

                tabView.TabItems.Add(tvi0);
                tabView.TabItems.Add(tvi1);
                tabView.TabItems.Add(tvi2);

                tabView.SelectedIndex = 0;
                tabView.SelectedItem = tvi0;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var selectionItemProvider = GetProviderFromTVI(tvi0);
                Verify.IsTrue(selectionItemProvider.IsSelected, "Item should be selected");

                selectionItemProvider = GetProviderFromTVI(tvi1);
                Verify.IsFalse(selectionItemProvider.IsSelected, "Item should not be selected");

                Log.Comment("Change selection through automationpeer");
                selectionItemProvider.Select();
                Verify.IsTrue(selectionItemProvider.IsSelected, "Item should have been selected");

                selectionItemProvider = GetProviderFromTVI(tvi0);
                Verify.IsFalse(selectionItemProvider.IsSelected, "Item should not be selected anymore");

                Verify.IsNotNull(selectionItemProvider.SelectionContainer);
            });

            static ISelectionItemProvider GetProviderFromTVI(TabViewItem item)
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item);
                var provider = peer.GetPattern(PatternInterface.SelectionItem)
                                as ISelectionItemProvider;
                Verify.IsNotNull(provider);
                return provider;
            }
        }

        [TestMethod]
        public void TabViewItemHeaderTest()
        {
            TabViewItem tvi0 = null;
            TabViewItem tvi1 = null;
            TabViewItem tvi2 = null;
            RunOnUIThread.Execute(() =>
            {
                var tabView = new TabView();

                tvi0 = CreateTabViewItem(null, "tab0Content");
                tvi1 = CreateTabViewItem("", "tab1Content");
                tvi2 = CreateTabViewItem("tab2", "tab2Content");

                tabView.TabItems.Add(tvi0);
                tabView.TabItems.Add(tvi1);
                tabView.TabItems.Add(tvi2);

                Content = tabView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Verify headers
                var headerContentPresenter1 = VisualTreeUtils.FindVisualChildByName(tvi0, "ContentPresenter") as ContentPresenter;
                var headerContentPresenter2 = VisualTreeUtils.FindVisualChildByName(tvi1, "ContentPresenter") as ContentPresenter;
                var headerContentPresenter3 = VisualTreeUtils.FindVisualChildByName(tvi2, "ContentPresenter") as ContentPresenter;

                Verify.AreEqual(null, headerContentPresenter1.Content, "tvi0's header should have been [null]");
                Verify.AreEqual("", headerContentPresenter2.Content, "tvi1's header should have been the empty string");
                Verify.AreEqual("tab2", headerContentPresenter3.Content, "tvi2's header should have been \"tab2\"");

                // Verify ToolTips
                var toolTip0 = ToolTipService.GetToolTip(tvi0) as ToolTip;
                var toolTip1 = ToolTipService.GetToolTip(tvi1) as ToolTip;
                var toolTip2 = ToolTipService.GetToolTip(tvi2) as ToolTip;

                bool testCondition = toolTip0.IsEnabled == false && toolTip0.Content == null;
                Verify.IsTrue(testCondition, "tvi0's ToolTip should have been disabled with [null] as content");

                testCondition = toolTip1.IsEnabled == false && toolTip1.Content == null;
                Verify.IsTrue(testCondition, "tvi1's ToolTip should have been disabled with [null] as content");

                testCondition = toolTip2.IsEnabled == true && toolTip2.Content is string s && s == "tab2";
                Verify.IsTrue(testCondition, "tvi2's ToolTip should have been enabled with \"tab2\" as content");
            });
        }

        private static void VerifyTabWidthVisualStates(IList<object> items, bool isCompact)
        {
            foreach (var item in items)
            {
                var tabItem = item as TabViewItem;
                var rootGrid = VisualTreeHelper.GetChild(tabItem, 0) as FrameworkElement;

                foreach (var group in VisualStateManager.GetVisualStateGroups(rootGrid))
                {
                    if (group.Name == "TabWidthModes")
                    {
                        if(tabItem.IsSelected || !isCompact)
                        {
                            Verify.AreEqual("StandardWidth", group.CurrentState.Name, "Verify that this tab item is rendering in standard width");
                        }
                        else
                        {
                            Verify.AreEqual("Compact", group.CurrentState.Name, "Verify that this tab item is rendering in compact width");
                        }
                    }
                }

            }
        }

        private static TabViewItem CreateTabViewItem(string name, Symbol icon, bool closable = true, bool enabled = true)
        {
            var tabViewItem = new TabViewItem();

            tabViewItem.Header = name;
            tabViewItem.IconSource = new Microsoft.UI.Xaml.Controls.SymbolIconSource() { Symbol = icon };
            tabViewItem.IsClosable = closable;
            tabViewItem.IsEnabled = enabled;

            return tabViewItem;
        }

        private static TabViewItem CreateTabViewItem(string name, object content, bool closable = true, bool enabled = true)
        {
            var tabViewItem = new TabViewItem();

            tabViewItem.Header = name;
            tabViewItem.Content = content;
            tabViewItem.IsClosable = closable;
            tabViewItem.IsEnabled = enabled;

            return tabViewItem;
        }

        private static TabViewItem CreateTabViewItem(string name, bool closable = true, bool enabled = true)
        {
            var tabViewItem = new TabViewItem();

            tabViewItem.Header = name;
            tabViewItem.IsClosable = closable;
            tabViewItem.IsEnabled = enabled;

            return tabViewItem;
        }
    }
}
