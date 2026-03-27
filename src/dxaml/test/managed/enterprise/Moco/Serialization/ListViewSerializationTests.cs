// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.Serialization
{
    [TestClass]
    public class SerializationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperNonGroupedList()
        {
            VerifyListViewPersistenceHelper(false /* isGrouped */, false /* scrollInLoaded */);
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperGroupedList()
        {
            VerifyListViewPersistenceHelper(true /* isGrouped */, false /* scrollInLoaded */);
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperNonGroupedListInLoaded()
        {
            VerifyListViewPersistenceHelper(false /* isGrouped */, true /* scrollInLoaded */);
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperGroupedListInLoaded()
        {
            VerifyListViewPersistenceHelper(true /* isGrouped */, true /* scrollInLoaded */);
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperNavigateFooter()
        {
            CustomPage page = null;

            UIExecutor.Execute(() =>
            {
                page = new CustomPage(false, false /*scrollOnLoaded*/, false /*header*/, true /*footer*/);

                TestServices.WindowHelper.WindowContent = page;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var child = VisualTreeHelper.GetChild(page.InnerListView, 0) as FrameworkElement;
                var sv = child.FindName("ScrollViewer") as ScrollViewer;
                Log.Comment("ScrollViewer offset:" + sv.VerticalOffset.ToString());

                // verify that it has scrolled past the items into the footer range
                Verify.IsGreaterThan(sv.VerticalOffset, 600.0);
            });
        }

        [TestMethod]
        public void VerifyListViewPersistenceHelperNavigateHeader()
        {
            CustomPage page = null;

            UIExecutor.Execute(() =>
            {
                page = new CustomPage(false, false /*scrollOnLoaded*/, true /*header*/);

                TestServices.WindowHelper.WindowContent = page;
            });

            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var child = VisualTreeHelper.GetChild(page.InnerListView, 0) as FrameworkElement;
                var sv = child.FindName("ScrollViewer") as ScrollViewer;
                Log.Comment("ScrollViewer offset:" + sv.VerticalOffset.ToString());

                // verify that it has scrolled a bit into the header
                Verify.IsGreaterThan(sv.VerticalOffset, 200.0);
            });
        }

        [TestMethod]
        public void CanSerializeAndDeserialize()
        {
            ListView list = null;
            string scrollPosition = null;
            IList<string> data = null;
            var listLoaded = new AutoResetEvent(false);
            var testCompletedEvent = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                list = (ListView)XamlReader.Load(@" <ListView 
                                                            xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
                                                        <ListView.Resources>
                                                            <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                                                        </ListView.Resources>
                                                    </ListView>");
                list.ItemsSource = data = Enumerable.Range(0, 500).Select(i => "Item #" + i).ToList();
                list.Height = 500;
                TestServices.WindowHelper.WindowContent = list;

                list.Loaded += delegate
                {
                    Log.Comment("List loaded.");
                    listLoaded.Set();
                };
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(10)), "List loaded.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(async delegate
            {
                Log.Comment("Serializing position at index 100.");
                list.ScrollIntoView(data[100], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();
                Verify.AreEqual(100, ((ItemsStackPanel)list.ItemsPanelRoot).FirstVisibleIndex);
                scrollPosition = ListViewPersistenceHelper.GetRelativeScrollPosition(list, i => (string)i);
                Log.Comment("scrollPosition = {0}", scrollPosition);

                var panel = (ItemsStackPanel)list.ItemsPanelRoot;

                Log.Comment("Scrolling back to index 0.");
                list.ScrollIntoView(data[0], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();
                Verify.AreEqual(0, panel.FirstVisibleIndex);

                Log.Comment("Restoring scroll position at index 100 with ListViewPersistenceHelper.");
                await ListViewPersistenceHelper.SetRelativeScrollPositionAsync(list, scrollPosition, i => Task.Run(delegate
                {
                    Log.Comment(i);
                    return (object)i;
                }).AsAsyncOperation());
                list.UpdateLayout();
                Verify.AreEqual(100, panel.FirstVisibleIndex);

                Log.Comment("Serializing position at index 111 with temsUpdatingScrollMode to KeepLastItemInView.");
                panel.ItemsUpdatingScrollMode = ItemsUpdatingScrollMode.KeepLastItemInView;
                scrollPosition = ListViewPersistenceHelper.GetRelativeScrollPosition(list, i => (string)i);
                Log.Comment("scrollPosition = {0}", scrollPosition);

                Log.Comment("Scrolling back to index 0.");
                list.ScrollIntoView(data[0], ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();
                Verify.AreEqual(0, panel.FirstVisibleIndex);

                Log.Comment("Restoring scroll position at index 111 with ListViewPersistenceHelper.");
                await ListViewPersistenceHelper.SetRelativeScrollPositionAsync(list, scrollPosition, i => Task.Run(delegate
                {
                    Log.Comment(i);
                    return (object)i;
                }).AsAsyncOperation());
                list.UpdateLayout();
                Verify.AreEqual(111, panel.LastVisibleIndex);

                testCompletedEvent.Set();
            });

            Verify.IsTrue(testCompletedEvent.WaitOne(TimeSpan.FromSeconds(10)), "Test completed.");
        }

        [TestMethod]
        public void CanRefreshListViewBeforeDeserialization()
        {
            Log.Comment("Preparing ListView...");
            VerifyListViewPersistenceHelper(true /* isGrouped */, true /* scrollInLoaded */);

            UIExecutor.Execute(() =>
            {
                var page = (CustomPage)TestServices.WindowHelper.WindowContent;
                var list = page.InnerListView;
                list.GroupStyle[0].HidesIfEmpty = true;
                var serializationKey = ListViewPersistenceHelper.GetRelativeScrollPosition(list, (i) => "dummy");

                Log.Comment("Reset group style to trigger the refresh of ListView and the reset of the modern panels' cache manager.");
                var groupStyle = list.GroupStyle[0];
                list.GroupStyle.Clear();
                list.GroupStyle.Add(groupStyle);

                Log.Comment("Deserialization of the scroll position.");
                var group0 = (object)((ICollectionViewGroup)((ICollectionView)list.ItemsSource).CollectionGroups[0]).Group;
                ListViewPersistenceHelper.SetRelativeScrollPositionAsync(list, serializationKey, i => Task.Run(() => group0).AsAsyncOperation()).AsTask();
            });

            UIExecutor.Execute(() =>
            {
                Log.Comment("Running layout. Expected: we should not crash.");
                var page = (CustomPage)TestServices.WindowHelper.WindowContent;
                page.InnerListView.UpdateLayout();
            });
        }

        private void VerifyListViewPersistenceHelper(bool isGrouped, bool scrollInLoaded)
        {
            CustomPage page = null;

            UIExecutor.Execute(() =>
            {
                page = new CustomPage(isGrouped, scrollInLoaded);

                TestServices.WindowHelper.WindowContent = page;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                int correctFirstVisibleIndex = isGrouped ? 3 : 4;

                // list should have scrolled with ListViewPersistenceHelper
                Verify.IsGreaterThanOrEqual(page.FirstVisibleIndex, correctFirstVisibleIndex);
            });
            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
