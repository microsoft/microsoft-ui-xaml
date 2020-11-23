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
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Controls.Primitives;

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
        public void VerifyCompactTabWidthVisualStates_ItemsMode()
        {
            VerifyCompactTabWidthVisualStates();
        }

        [TestMethod]
        public void VerifyCompactTabWidthVisualStates_ItemsSourceMode()
        {
            VerifyCompactTabWidthVisualStates(isItemsSourceMode: true);           
        }

        private void VerifyCompactTabWidthVisualStates(bool isItemsSourceMode = false)
        {
            TabView tabView = null;
            RunOnUIThread.Execute(() =>
            {
                tabView = new TabView();
                SetupTabViewItems();

                Log.Comment("Set TabWidthMode to compact");
                tabView.TabWidthMode = TabViewWidthMode.Compact;

                Log.Comment("Select a tab. In TabWidthMode compact, a selected tab will not be in compact state. We verify that behavior in this test");
                tabView.SelectedIndex = 0;

                Content = tabView;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify a selected tab exists");
                VerifySelectedItem("Tab 0");

                Log.Comment("Verify the created TabView displays every tab in compact mode");
                VerifyTabWidthVisualStates(tabView, tabView.TabItems, true);

                Log.Comment("Verify that adding a new item creates a tab in compact mode");
                AddItem("Tab 3");
                Content.UpdateLayout();

                VerifyTabWidthVisualStates(tabView, new List<object>() { tabView.TabItems[tabView.TabItems.Count - 1] }, true);

                Log.Comment("Change the TabWidthMode to non compact and verify that every tab is no longer in compact mode");
                tabView.TabWidthMode = TabViewWidthMode.Equal;
                Content.UpdateLayout();

                VerifyTabWidthVisualStates(tabView, tabView.TabItems, false);

                Log.Comment("Change the TabWidthMode to compact and verify that every tab is now in compact mode");
                tabView.TabWidthMode = TabViewWidthMode.Compact;
                Content.UpdateLayout();

                VerifyTabWidthVisualStates(tabView, tabView.TabItems, true);
            });

            void SetupTabViewItems()
            {
                if (isItemsSourceMode)
                {
                    var tabItemsSource = new ObservableCollection<string>() { "Tab 0", "Tab 1", "Tab 2" };
                    tabView.TabItemsSource = tabItemsSource;
                }
                else
                {
                    tabView.TabItems.Add(CreateTabViewItem("Tab 0", Symbol.Add));
                    tabView.TabItems.Add(CreateTabViewItem("Tab 1", Symbol.AddFriend));
                    tabView.TabItems.Add(CreateTabViewItem("Tab 2"));
                }
            }

            void VerifySelectedItem(string expectedHeader)
            {
                object selectedItemHeader = isItemsSourceMode
                    ? tabView.SelectedItem
                    : (tabView.SelectedItem as TabViewItem).Header;

                Verify.AreEqual(expectedHeader, selectedItemHeader);
            }

            void AddItem(string header)
            {
                if (isItemsSourceMode)
                {
                    ((ObservableCollection<string>)tabView.TabItemsSource).Add(header);
                }
                else
                {
                    tabView.TabItems.Add(CreateTabViewItem(header));
                }
            }
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

            //RunOnUIThread.Execute(() =>
            //{
            //    var selectionItemProvider = GetProviderFromTVI(tvi0);
            //    Verify.IsTrue(selectionItemProvider.IsSelected, "Item should be selected");

            //    selectionItemProvider = GetProviderFromTVI(tvi1);
            //    Verify.IsFalse(selectionItemProvider.IsSelected, "Item should not be selected");

            //    Log.Comment("Change selection through automationpeer");
            //    selectionItemProvider.Select();
            //    Verify.IsTrue(selectionItemProvider.IsSelected, "Item should have been selected");

            //    selectionItemProvider = GetProviderFromTVI(tvi0);
            //    Verify.IsFalse(selectionItemProvider.IsSelected, "Item should not be selected anymore");

            //    Verify.IsNotNull(selectionItemProvider.SelectionContainer);
            //});

            //static ISelectionItemProvider GetProviderFromTVI(TabViewItem item)
            //{
            //    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item);
            //    var provider = peer.GetPattern(PatternInterface.SelectionItem)
            //                    as ISelectionItemProvider;
            //    Verify.IsNotNull(provider);
            //    return provider;
            //}
        }

        [TestMethod]
        public void VerifyTabViewWithoutTabsDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                TabView tabView = new TabView();
                Content = tabView;

                // Creating a TabView without tabs should not crash the app.
                Content.UpdateLayout();

                var tabItemsSource = new ObservableCollection<string>() { "Tab 1", "Tab 2" };
                tabView.TabItemsSource = tabItemsSource;

                // Clearing the ItemsSource should not crash the app.
                Log.Comment("Clear the specified tab items source");
                tabItemsSource.Clear();
            });       
        }

        [TestMethod]
        public void TabViewItemBackgroundTest()
        {
            TabView tabView = null;
            TabViewItem tabViewItem1 = null;
            TabViewItem tabViewItem2 = null;
            RunOnUIThread.Execute(() =>
            {
                tabView = new TabView();

                tabViewItem1 = CreateTabViewItem("Tab1", Symbol.Home);
                tabViewItem2 = CreateTabViewItem("Tab2", Symbol.Document);

                tabView.TabItems.Add(tabViewItem1);
                tabView.TabItems.Add(tabViewItem2);

                Content = tabView;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Verify that the TabViewItem we use for background API testing here is unselected
                Verify.IsFalse(tabViewItem2.IsSelected, "TabViewItem should have been unselected");

                bool testCondition = !IsBrushUsingSolidColor(tabViewItem2.Background, Colors.Blue);
                Verify.IsTrue(testCondition, "Default TabViewItem background color should have not been [blue]");

                var tabContainer = VisualTreeUtils.FindVisualChildByName(tabViewItem2, "TabContainer") as Grid;

                testCondition = !IsBrushUsingSolidColor(tabContainer.Background, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [TabContainer] background color should have not been [blue]");

                Log.Comment("Set the background color of the TabViewItem to [blue]");
                tabViewItem2.Background = new SolidColorBrush(Colors.Blue);

                testCondition = IsBrushUsingSolidColor(tabViewItem2.Background, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's background color should have been [blue]");

                testCondition = IsBrushUsingSolidColor(tabContainer.Background, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [TabContainer] background color should have been [blue]");

                bool IsBrushUsingSolidColor(Brush brush, Color color)
                {
                    return brush is SolidColorBrush scBrush && scBrush.Color == color;
                }
            });
        }

        [TestMethod]
        public void TabViewItemForegroundTest()
        {
            TabView tabView = null;
            TabViewItem tabViewItem1 = null;
            TabViewItem tabViewItem2 = null;
            RunOnUIThread.Execute(() =>
            {
                tabView = new TabView();

                tabViewItem1 = CreateTabViewItem("Tab1", Symbol.Home);
                tabViewItem2 = CreateTabViewItem("Tab2", Symbol.Document);

                tabView.TabItems.Add(tabViewItem1);
                tabView.TabItems.Add(tabViewItem2);

                Content = tabView;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Verify that the TabViewItem we use for foreground API testing here is unselected
                Verify.IsFalse(tabViewItem2.IsSelected, "TabViewItem should have been unselected");

                bool testCondition = !IsBrushUsingSolidColor(tabViewItem2.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "Default TabViewItem foreground color should have not been [blue]");

                var iconControl = VisualTreeUtils.FindVisualChildByName(tabViewItem2, "IconControl") as ContentControl;
                var headerPresenter = VisualTreeUtils.FindVisualChildByName(tabViewItem2, "ContentPresenter") as ContentPresenter;

                testCondition = !IsBrushUsingSolidColor(iconControl.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [IconControl] foreground color should have not been [blue]");

                testCondition = !IsBrushUsingSolidColor(headerPresenter.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [ContentPresenter] foreground color should have not been [blue]");

                Log.Comment("Set the foreground color of the TabViewItem to [blue]");
                tabViewItem2.Foreground = new SolidColorBrush(Colors.Blue);

                testCondition = IsBrushUsingSolidColor(tabViewItem2.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's foreground color should have been [blue]");

                testCondition = IsBrushUsingSolidColor(iconControl.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [IconControl] foreground color should have been [blue]");

                testCondition = IsBrushUsingSolidColor(headerPresenter.Foreground, Colors.Blue);
                Verify.IsTrue(testCondition, "TabViewItem's [ContentPresenter] foreground color should have been [blue]");

                bool IsBrushUsingSolidColor(Brush brush, Color color)
                {
                    return brush is SolidColorBrush scBrush && scBrush.Color == color;
                }
            });
        }

        private static void VerifyTabWidthVisualStates(TabView tabView, IList<object> items, bool isCompact)
        {
            var listView = VisualTreeUtils.FindVisualChildByName(tabView, "TabListView") as TabViewListView;

            foreach (var item in items)
            {
                var tabItem = item is TabViewItem
                    ? (TabViewItem)item
                    : listView.ContainerFromItem(item) as TabViewItem;

                var rootGrid = VisualTreeHelper.GetChild(tabItem, 0) as FrameworkElement;

                foreach (var group in VisualStateManager.GetVisualStateGroups(rootGrid))
                {
                    if (group.Name == "TabWidthModes")
                    {
                        if (tabItem.IsSelected || !isCompact)
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
