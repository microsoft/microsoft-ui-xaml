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
                tabView.TabWidthMode = TabViewWidthMode.Compact;
                Content.UpdateLayout();
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

        private static void VerifyTabWidthVisualStates(IList<object> items, bool isCompact)
        {
            foreach (var item in items)
            {
                var tabItem = item as TabViewItem;
                if (tabItem.IsSelected || !isCompact)
                {
                    VisualStateHelper.ContainsVisualState(tabItem, "StandardWidth");
                }
                else
                {
                    VisualStateHelper.ContainsVisualState(tabItem, "Compact");
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
