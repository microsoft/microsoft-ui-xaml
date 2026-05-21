// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco
{
    [TestClass]
    public class ScrollIntoViewTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        class ScrollingTestCase
        {
            public ScrollIntoViewAlignment Alignment = ScrollIntoViewAlignment.Default;
            public int Index = 0;
            public double ExpectedOffset = 0;

            public ScrollingTestCase(ScrollIntoViewAlignment alignment, int index, double expectedOffset)
            {
                Alignment = alignment;
                Index = index;
                ExpectedOffset = expectedOffset;
            }
        }

        [TestMethod]
        public void ScrollIntoViewWithAlignment_ListView()
        {
            ListView listView = null;
            List<int> data = Enumerable.Range(0, 50).ToList();
            int itemsCount = data.Count;

            UIExecutor.Execute(() =>
            {
                listView = (ListView)XamlReader.Load(@" <ListView 
                                                                xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                                                Height='200' 
                                                                Width='200'>
                                                            <ListView.Resources>
                                                              <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                                                            </ListView.Resources>
                                                        </ListView>");
                listView.ShowsScrollingPlaceholders = false;
                listView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = listView;
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                (listView.ItemsPanelRoot as ItemsStackPanel).CacheLength = 0;
            });

            List<ScrollingTestCase> cases = new List<ScrollingTestCase>();
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 1, 44));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 5, 156));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 6, 156));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 30, 156));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, data.Count - 1, 156));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 30, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 6, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));

            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 1, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 5, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 6, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 30, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, data.Count - 1, 156));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 30, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 6, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));

            foreach (var c in cases)
            {
                PerformScrollIntoViewAndVerify(listView, data, c.Alignment, c.Index, c.ExpectedOffset);
            }
        }

        [TestMethod]
        public void ScrollIntoViewWithAlignment_GridView()
        {
            GridView gridView = null;
            List<int> data = Enumerable.Range(0, 50).ToList();
            int itemsCount = data.Count;

            UIExecutor.Execute(() =>
            {
                gridView = new GridView() { Width = 200, Height = 200 };
                gridView.ShowsScrollingPlaceholders = false;
                gridView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = gridView;
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                (gridView.ItemsPanelRoot as ItemsWrapGrid).CacheLength = 0;
            });

            List<ScrollingTestCase> cases = new List<ScrollingTestCase>();
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 4, 48));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 10, 96));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 12, 144));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, itemsCount - 1, 152));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));

            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 10, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 12, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, itemsCount - 1, 142));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));

            foreach (var c in cases)
            {
                PerformScrollIntoViewAndVerify(gridView, data, c.Alignment, c.Index, c.ExpectedOffset);
            }
        }

        [TestMethod]
        public void ScrollIntoViewWithAlignment_GridViewWrapGrid()
        {
            GridView gridView = null;
            List<int> data = Enumerable.Range(0, 50).ToList();
            int itemsCount = data.Count;

            UIExecutor.Execute(() =>
            {
                gridView = (GridView)XamlReader.Load(@" <GridView 
                                                              xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                                              Height='200' 
                                                              Width='200'>
                                                            <GridView.ItemsPanel>
                                                                <ItemsPanelTemplate>
                                                                    <WrapGrid Orientation='Horizontal'/>
                                                                </ItemsPanelTemplate>
                                                            </GridView.ItemsPanel>
                                                        </GridView>");
                gridView.ShowsScrollingPlaceholders = false;
                gridView.ItemsSource = data;
                TestServices.WindowHelper.WindowContent = gridView;
            });

            TestServices.WindowHelper.WaitForIdle(false);

            List<ScrollingTestCase> cases = new List<ScrollingTestCase>();
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 4, 48));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 10, 96));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 12, 144));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, itemsCount - 1, 152));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Default, 0, 0));

            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 10, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 12, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, itemsCount - 1, 152));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 4, 0));
            cases.Add(new ScrollingTestCase(ScrollIntoViewAlignment.Leading, 0, 0));

            foreach (var c in cases)
            {
                PerformScrollIntoViewAndVerify(gridView, data, c.Alignment, c.Index, c.ExpectedOffset);
            }
        }

        void PerformScrollIntoViewAndVerify(ListViewBase listViewBase, List<int> data, ScrollIntoViewAlignment alignment, int itemIndex, double expectedOffset)
        {
            Log.Comment(string.Format("+ PerformScrollIntoViewAndVerify: index={0}, alignment={1}, expectedOffset={2}.", itemIndex, alignment, expectedOffset));

            FrameworkElement listViewItem = null;

            UIExecutor.Execute(() =>
            {
                listViewBase.ScrollIntoView(data[itemIndex], alignment);
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                listViewItem = listViewBase.ContainerFromIndex(itemIndex) as FrameworkElement;
                Verify.IsNotNull(listViewItem, "listViewItem returned from ItemContainerGenerator is null.");

                var gt = listViewItem.TransformToVisual(listViewBase);
                var actualOffset = gt.TransformPoint(new Point());

                Log.Comment("Actual Offset: " + actualOffset.Y);
                if (!(Math.Abs(expectedOffset - actualOffset.Y) <= 1))
                {
                    Verify.Fail("Expected offset does not match actual offset");
                }
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ScrollGroupIntoViewAndDeleteGroupBeforeLayoutRuns()
        {
            ListView lv = null;
            CollectionViewSource cvs = null;
            var groups = new ObservableCollection<ObservableCollection<int>>();

            UIExecutor.Execute(() =>
            {
                lv = new ListView();
                lv.ShowsScrollingPlaceholders = false;
                lv.Height = 500;

                for (int i = 0; i < 2; i++)
                {
                    var group = new ObservableCollection<int>();
                    group.Add(0);
                    
                    groups.Add(group);
                }

                cvs = new CollectionViewSource() { IsSourceGrouped = true };
                cvs.Source = groups;

                lv.GroupStyle.Add((GroupStyle)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <GroupStyle xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>        " +
                   "     <GroupStyle.HeaderTemplate>                                                       " +
                   "         <DataTemplate>                                                                " +
                   "             <Grid Background='Red' >                                                  " +
                   "                 <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>    " +
                   "             </Grid>                                                                   " +
                   "         </DataTemplate>                                                               " +
                   "     </GroupStyle.HeaderTemplate>                                                      " +
                   " </GroupStyle>                                                                         "));

                lv.ItemTemplate = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                   " <DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> " +
                   "     <TextBlock Text='{Binding}' />  " +
                   " </DataTemplate>");

                lv.ItemsSource = cvs.View;
                TestServices.WindowHelper.WindowContent = lv;
            });

            TestServices.WindowHelper.WaitForIdle();

            // Scroll second group into view and immediately delete it.
            // the scroll command is stashed away until next layout. and when 
            // the command is executed 
            UIExecutor.Execute(() =>
            {
                // verify that the first item is the first visible index
                var panel = lv.ItemsPanelRoot as ItemsStackPanel;
                Verify.IsNotNull(panel);
                Verify.AreEqual(0, panel.FirstVisibleIndex);

                lv.ScrollIntoView(groups[1]);
                groups.RemoveAt(1);
            });

            TestServices.WindowHelper.WaitForIdle();

            // Verify we didn't crash by checking the first visible index. It 
            // should be the same as not scrolling.
            UIExecutor.Execute(() =>
            {
                var panel = lv.ItemsPanelRoot as ItemsStackPanel;
                Verify.IsNotNull(panel);
                Verify.AreEqual(0, panel.FirstVisibleIndex);
            });

        }
    }
}
