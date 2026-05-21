// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Enterprise.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    /// <summary>
    /// Class for the basic tests for maintain viewport behavior in ListView/GridView
    /// </summary>
    /// 
    [TestClass]
    public class MoCoViewportBasicTests : MoCoViewportTestBase
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

        /// <summary>
        /// Tests basic data manipulations such as Insert, Remove, Resize using flat data collection
        /// Verifies the viewport offset changes or not
        /// 
        /// Variations: 
        ///   Panel orientations {Horizontal, Vertical}
        /// 
        /// Steps:
        /// 1. Loads the XAML
        /// 2. Set the current orientation to the ItemsPanel
        /// 3. Scroll the list as appropriate
        /// 4. Insert item in/out of the viewport
        /// 5. Remove item in/out of the viewport
        /// 6. Resize item in/out of the viewport
        /// 7. Verify the viewport changes as appropriate
        /// 8. Verify the visual changes as appropriate
        /// </summary>
        [TestMethod]
        [Priority(1)]
        public void TestBasicDataManipulations()
        {
            ListViewBase list = null; // List control under tests

            // Load Xaml
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var targetPanel = LoadXamlTestPage(fileName);

            // Populate list
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(targetPanel, "targetPanel not found");

                // Find the list view
                list = targetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(list, "ListView not found");
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                (list.ItemsPanelRoot as ItemsStackPanel).CacheLength = 1;
            });

            // Start testing with all variations
            foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Resetting ItemsSource...");
                    list.ItemsSource = null;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting " + orientation + " orientation...");
                    SetPanelOrientation(list, orientation);
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting ItemsSource...");
                    list.ItemsSource = GetFlatDataCollection(20);
                });

                TestServices.WindowHelper.WaitForIdle();

                const int scrollIndex = 5;

                // Scroll the list so index 5 is the first fully displayed item.
                // The Root Grid has a 32px Margin to account for the safe zone
                // applied to bring-into-view operations, so that index 4 is not
                // partially displayed. Thus index 5 ends up being the tracked
                // element.
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Scrolling to " + scrollIndex + "...");
                    ScrollTo(scrollIndex, list, ScrollIntoViewAlignment.Leading);
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Point offset = GetViewportOffset(list);
                    Log.Comment(string.Format("Original offset:{0}", offset));
                });

                TestServices.WindowHelper.WaitForIdle();

                // Insert item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.INSERT, scrollIndex - 1, orientation, ScrollPosition.MIDDLE);

                // Remove item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.REMOVE, scrollIndex - 1, orientation, ScrollPosition.MIDDLE);

                // Expand item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.EXPAND, scrollIndex - 1, orientation, ScrollPosition.MIDDLE);

                // Collapse item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.COLLAPSE, scrollIndex - 1, orientation, ScrollPosition.MIDDLE);

                // Expand tracked item. No scroll offset change expected.
                TestFlatDataManipulations(list, DataManipulationType.EXPAND, scrollIndex, orientation, ScrollPosition.MIDDLE, false /*scrollOffsetChangeExpected*/);

                // Collapse tracked item. No scroll offset change expected.
                TestFlatDataManipulations(list, DataManipulationType.COLLAPSE, scrollIndex, orientation, ScrollPosition.MIDDLE, false /*scrollOffsetChangeExpected*/);
            }
        }

        /// <summary>
        /// Tests the ScrollOnUpdate behavior on ListView/GridView when an item is partially visible in the viewport
        /// Verifies the viewport offset changes or not
        /// 
        /// Variations:
        ///   Panel orientations {Horizontal, Vertical}
        /// 
        /// Steps:
        /// 1. Load the Xaml
        /// 2. Populate the list
        /// 3. Set the current orientation to the ItemsPanel
        /// 4. Scroll the list so that "n"th item is partially visible in the viewport (intersecting the viewport)
        /// 5. Insert item at index n where nth item is intersecting the viewport
        /// 6. Remove the item intersecting the viewport
        /// 7. Resize the item intersecting the viewport
        /// 8. Verify the viewport changes or not
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        public void TestWithPartiallyVisibleItem()
        {
            ListViewBase list = null; // List control under tests
            const int intersectingItemIndex = 2;

            // Load Xaml
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var targetPanel = LoadXamlTestPage(fileName);

            // Populate list
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(targetPanel, "targetPanel not found");

                // Find the list view
                list = targetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(list, "ListView not found");
            });

            TestServices.WindowHelper.WaitForIdle(false);

            UIExecutor.Execute(() =>
            {
                (list.ItemsPanelRoot as ItemsStackPanel).CacheLength = 1;
            });

            // Iterate through all panel orientations
            foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Resetting ItemsSource...");
                    list.ItemsSource = null;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting " + orientation + " orientation...");
                    SetPanelOrientation(list, orientation);
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting ItemsSource...");
                    list.ItemsSource = GetFlatDataCollection(20);
                });

                TestServices.WindowHelper.WaitForIdle();

                // Scroll to an arbitrary item
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Scrolling to " + intersectingItemIndex + "...");
                    ScrollTo(intersectingItemIndex, list, ScrollIntoViewAlignment.Leading);
                });
                
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Point offset = GetViewportOffset(list);
                    Log.Comment(string.Format("Original offset:{0}", offset));
                });

                // Scroll the list so the item is intersecting the near edge
                TestServices.WindowHelper.WaitForIdle();
                Log.Comment("Scrolling the list slightly so the item is intersecting the near edge...");
                UIExecutor.Execute(() =>
                {
                    // Access the underlying ScrollViewer
                    ScrollViewer sv = list.FindElementOfTypeInSubtree<ScrollViewer>();
                    Verify.IsNotNull(sv, "Unable to find the underlying ScrollViewer");

                    switch (orientation)
                    {
                        case Orientation.Vertical:
                        {
                            bool result = sv.ChangeView(0, sv.VerticalOffset + 35, 1, true /*disableAnimation*/);
                            Verify.IsTrue(result, "Vertical offset adjustment not successful");
                            break;
                        }

                        case Orientation.Horizontal:
                        {
                            bool result = sv.ChangeView(sv.HorizontalOffset + 60, 0, 1, true /*disableAnimation*/);
                            Verify.IsTrue(result, "Horizontal offset adjustment not successful");
                            break;
                        }
                    }
                });

                // Index intersectingItemIndex == 2 ends up being the tracked item.
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Point offset = GetViewportOffset(list);
                    Log.Comment(string.Format("Adjusted original offset:{0}", offset));
                });

                TestServices.WindowHelper.WaitForIdle();

                // Insert item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.INSERT, intersectingItemIndex, orientation, ScrollPosition.INTERSECTING);

                // Remove item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.REMOVE, intersectingItemIndex, orientation, ScrollPosition.INTERSECTING);

                // Expand item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.EXPAND, intersectingItemIndex - 1, orientation, ScrollPosition.INTERSECTING);

                // Collapse item before tracked one.
                TestFlatDataManipulations(list, DataManipulationType.COLLAPSE, intersectingItemIndex - 1, orientation, ScrollPosition.INTERSECTING);

                // Expand tracked item. No scroll offset change expected.
                TestFlatDataManipulations(list, DataManipulationType.EXPAND, intersectingItemIndex, orientation, ScrollPosition.INTERSECTING, false /*scrollOffsetChangeExpected*/);

                // Collapse tracked item. No scroll offset change expected.
                TestFlatDataManipulations(list, DataManipulationType.COLLAPSE, intersectingItemIndex, orientation, ScrollPosition.INTERSECTING, false /*scrollOffsetChangeExpected*/);
            }
        }

        /// <summary>
        /// Tests the ListView/GridView by bulk number of inserting, removing, resizing items in a flat data collection
        /// Verifies the viewport offset changes and visual rendering of the items
        /// 
        /// Variations:
        /// 1. List controls {ListView, GridView}
        /// 2. Panel orientations {Horizontal, Vertical}
        /// 
        /// Steps:
        /// 1. Load the Xaml
        /// 2. Populate the list
        /// 3. Set the current orientation to the ItemsPanel
        /// 4. Scroll the list to an arbitrary item so that the subsequent data manipulation occurs off the viewport
        /// 5. Insert items multiple times out of the viewport
        /// 6. Remove items multiple times out of the viewport
        /// 7. Resize items multiple times out of the viewport
        /// 8. Verify the viewport changes
        /// 9. Verify visual changes
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        public void TestBulkDataManipulations()
        {
            ListViewBase list = null; // List control under tests

            // Load Xaml
            //
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var targetPanel = LoadXamlTestPage(fileName);

            // Populate list
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(targetPanel, "targetPanel not found");

                // Find the list view
                list = targetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(list, "ListView not found");

                // populate the lists
                list.ItemsSource = GetFlatDataCollection(20);
                list.Visibility = Visibility.Visible;
            });

            TestServices.WindowHelper.WaitForIdle(false);

            // Iterate through all the panel orientations
            foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
            {
                // Set panel orientation
                //
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting " + orientation + " orientation...");
                    SetPanelOrientation(list, orientation);
                    (list.ItemsPanelRoot as ItemsStackPanel).CacheLength = 0;
                });

                TestServices.WindowHelper.WaitForIdle();

                // Populate the list with >100 items and scroll the 100 th item so that bulk deletion happens above the viewport
                //
                SetupListForDataManipulation(list, 120, 100);

                // Expand item
                //
                TestFlatDataManipulations(list, DataManipulationType.BULK_EXPAND, 0, orientation, ScrollPosition.MIDDLE);

                // Collapse item
                //
                TestFlatDataManipulations(list, DataManipulationType.BULK_COLLAPSE, 0, orientation, ScrollPosition.MIDDLE);

                // Delete bulk items
                //
                TestFlatDataManipulations(list, DataManipulationType.BULK_REMOVE, 0, orientation, ScrollPosition.MIDDLE);
            }
        }

        /// <summary>
        /// Tests the ListView/GridView by inserting, removing, resizing items and groups in a grouped data collection
        /// Verifies the viewport offset changes and visual rendering of the items
        /// 
        /// Variations:
        /// 1. List Controls {ListView, GridView}
        /// 2. Panel orientations {Horizontal, Vertical}
        /// 
        /// Steps:
        /// 1. Load the Xaml
        /// 2. Populate the list with grouped data collection
        /// 3. Set the current orientation to the ItemsPanel
        /// 4. Scroll the list to an arbitrary item so that the subsequent data manipulation occurs off the viewport
        /// 5. Insert items and groups out of the viewport
        /// 6. Remove items and groups out of the viewport
        /// 7. Resize items and groups out of the viewport
        /// 8. Verify the viewport changes
        /// 9. Verify visual changes
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestGroupedDataManipulations()
        {
            ListViewBase list = null; // List control under tests

            // Load Xaml
            //
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var targetPanel = LoadXamlTestPage(fileName);

            // Find the lists
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(targetPanel, "targetPanel not found");

                // Find the list view
                list = targetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(list, "ListView not found");
            });

            TestServices.WindowHelper.WaitForIdle();

            // Testing both ListView and GridView
            //
            UIExecutor.Execute(() =>
            {
                SetPanelOrientation(list, Orientation.Vertical);

                // Populate the list
                this.cvs = new CollectionViewSource();
                this.cvs.Source = GetGroupedDataCollection(20);
                this.cvs.IsSourceGrouped = true;
                this.cvs.ItemsPath = new PropertyPath("Items");

                // populate the lists
                list.ItemsSource = this.cvs.View;
            });

            // Scroll the list
            //
            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to group# 3...");
                ScrollToGroup(3, list, ScrollIntoViewAlignment.Leading);
            });

            // Wait for few seconds to let the animation complete
            TestServices.WindowHelper.WaitForIdle();

            // Expand item
            //
            TestGroupedDataManipulations(list, DataManipulationType.EXPAND, 2, 3, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Collapse item
            //
            TestGroupedDataManipulations(list, DataManipulationType.COLLAPSE, 2, 3, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Expand group
            //
            TestGroupedDataManipulations(list, DataManipulationType.EXPAND_GROUP, 2, 0, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Collapse group
            //
            TestGroupedDataManipulations(list, DataManipulationType.COLLAPSE_GROUP, 2, 0, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Add item
            //
            TestGroupedDataManipulations(list, DataManipulationType.INSERT, 0, 1, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Add group
            //
            TestGroupedDataManipulations(list, DataManipulationType.INSERT_GROUP, 1, 0, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Delete item
            //
            TestGroupedDataManipulations(list, DataManipulationType.REMOVE, 1, 2, Orientation.Vertical, ScrollPosition.MIDDLE);

            // Delete group
            //
            TestGroupedDataManipulations(list, DataManipulationType.REMOVE_GROUP, 1, 0, Orientation.Vertical, ScrollPosition.MIDDLE);
        }

        /// <summary>
        /// Tests the MaintainOffset behavior on ListView and GridView when ScrollOnUpdate=MaintainOffset in its ItemsPanel
        /// 
        /// Variations: 
        /// 1. List Control {ListView, GridView}
        /// 2. Panel orientations {Horizontal, Vertical}
        /// 3. Scroll position {Not-Scrolled, Scrolled}
        /// 4. Data manipulations {Insert, Remove, Resize}
        /// 
        /// Steps:
        /// 1. Loads the XAML
        /// 2. Set the ItemsPanel which has ScrollOnUpdate property set to MaintainOffset
        /// 3. Set the current orientation to the ItemsPanel
        /// 4. Scroll the list to middle
        /// 5. Insert item out of the viewport
        /// 6. Remove item out of the viewport
        /// 7. Resize item out of the viewport
        /// 8. Verify the viewport changes
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        public void TestMaintainOffset()
        {
            ListViewBase list = null; // List control under tests
            ItemsPanelTemplate newPanelTemplate = null;
            int initScrollPos = 5;

            List<DataManipulationType> dataManipulations = new List<DataManipulationType>()
            {
                DataManipulationType.INSERT,
                DataManipulationType.REMOVE,
                DataManipulationType.EXPAND,
                DataManipulationType.COLLAPSE
            };

            // Load Xaml
            //
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var targetPanel = LoadXamlTestPage(fileName);

            // Find the Lists and ItemsPanel
            //
            UIExecutor.Execute(() =>
            {
                Verify.IsNotNull(targetPanel, "targetPanel not found");

                // Find the list view
                list = targetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(list, "ListView not found");

                // Find the ItemsPanel which maintains the offset
                //
                newPanelTemplate = targetPanel.Resources["MaintainOffsetPanel"] as ItemsPanelTemplate;
                Verify.IsNotNull(newPanelTemplate, "MaintainOffsetPanel is not found");
            });

            TestServices.WindowHelper.WaitForIdle();

            // Test both ListView and GridView
            //
            UIExecutor.Execute(() =>
            {
                // Set the new ItemsPanel
                //
                list.ItemsPanel = newPanelTemplate;

                // Populate the list
                //
                list.ItemsSource = GetFlatDataCollection(20);
            });

            TestServices.WindowHelper.WaitForIdle();

            // Start testing with all variations
            //
            foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Setting " + orientation + " orientation...");
                    SetPanelOrientation(list, orientation);
                });

                // Wait for few seconds to let the animation stop
                TestServices.WindowHelper.WaitForIdle();

                // Scroll the list
                //
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Scrolling to " + initScrollPos + "...");
                    ScrollTo(initScrollPos, list, ScrollIntoViewAlignment.Leading);
                });

                TestServices.WindowHelper.WaitForIdle();

                foreach (DataManipulationType dmType in dataManipulations)
                {
                    Point prevOffset = new Point();
                    Point currOffset = new Point();

                    UIExecutor.Execute(() =>
                    {
                        // Find the current viewport offset before data manipulation
                        //
                        prevOffset = GetViewportOffset(list);

                        // Perform data manipulation
                        //
                        PerformFlatDataManipulation(list, dmType, 2);
                    });

                    // Verify the viewport offset after the data manipulation
                    //
                    UIExecutor.Execute(() =>
                    {
                        currOffset = GetViewportOffset(list);

                        Verify.AreEqual(prevOffset.X, currOffset.X, "Viewport offset is not maintained horizontally");
                        Verify.AreEqual(prevOffset.Y, currOffset.Y, "Viewport offset is not maintained vertically");
                    });
                }
            }
        }

        [TestMethod]
        public void CanElectNewTrackedElement()
        {
            ListView list = null;
            ItemsStackPanel panel = null;
            ScrollViewer scrollViewer = null;
            var listLoaded = new AutoResetEvent(false);
            ObservableCollection<string> data = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsSource = data = new ObservableCollection<string>(Enumerable.Range(0, 500).Select(i => "Item #" + i));
                list.Height = 500;
                list.Loaded += delegate
                {
                    panel = (ItemsStackPanel)list.ItemsPanelRoot;
                    scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            foreach (bool trackFirstVisibleElement in new bool[] { true, false })
            {
                UIExecutor.Execute(() =>
                {
                    ItemsUpdatingScrollMode mode = trackFirstVisibleElement ? ItemsUpdatingScrollMode.KeepItemsInView : ItemsUpdatingScrollMode.KeepLastItemInView;
                    Log.Comment("Setting ItemsUpdatingScrollMode to " + mode);
                    panel.ItemsUpdatingScrollMode = mode;

                    Log.Comment("Scrolling to item 100");
                    list.ScrollIntoView(data[100], ScrollIntoViewAlignment.Leading);
                    list.UpdateLayout();
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    double expectedVerticalOffset =
                        trackFirstVisibleElement ?
                        scrollViewer.VerticalOffset - ((FrameworkElement)list.ContainerFromIndex(panel.FirstVisibleIndex)).ActualHeight :
                        scrollViewer.VerticalOffset;

                    Log.Comment("Removing the first/last visible element to validate that we can elect a new tracked element.");
                    data.RemoveAt(trackFirstVisibleElement ? panel.FirstVisibleIndex : panel.LastVisibleIndex);
                    list.UpdateLayout();
                    Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);

                    Log.Comment("Removing enough elements from the realized range to validate that we can gracefully fail to elect a new tracked element.");
                    int startIndex = trackFirstVisibleElement ? panel.FirstVisibleIndex : panel.LastVisibleIndex;
                    int visibleRangeSize = panel.LastVisibleIndex - panel.FirstVisibleIndex + 1;
                    for (int i = 0; i < visibleRangeSize; ++i)
                    {
                        data.RemoveAt(startIndex);
                        if (!trackFirstVisibleElement) --startIndex;
                    }

                    list.UpdateLayout();
                    // We bailed out on tracking, so the ScrollViewer offset should stay the same.
                    Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);
                });
            }
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanKeepLastItemInViewAfterBulkAddDelete()
        {
            ListView list = null;
            ItemsStackPanel panel = null;
            ScrollViewer scrollViewer = null;
            var listLoaded = new AutoResetEvent(false);
            ObservableCollection<string> data = null;

            var loadList = new Action<Orientation, bool>((o, includeHeadersAndFooters) =>
            {
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Loading list.");
                    list = new ListView();
                    list.ItemsPanel = (ItemsPanelTemplate)Markup.XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemsStackPanel ItemsUpdatingScrollMode='KeepLastItemInView'/></ItemsPanelTemplate>");

                    list.Loaded += delegate
                    {
                        panel = (ItemsStackPanel)list.ItemsPanelRoot;
                        scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                        listLoaded.Set();
                    };
                    TestServices.WindowHelper.WindowContent = list;
                });

                Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Preparing list.");
                    panel.Orientation = o;

                    list.Width = (o == Orientation.Horizontal) ? 500 : double.NaN;
                    list.Height = (o == Orientation.Vertical) ? 500 : double.NaN;
                    list.ItemsSource = data = new ObservableCollection<string>();
                    scrollViewer.HorizontalScrollMode = (o == Orientation.Horizontal) ? ScrollMode.Auto : ScrollMode.Disabled;
                    scrollViewer.VerticalScrollMode = (o == Orientation.Horizontal) ? ScrollMode.Disabled : ScrollMode.Auto;
                    scrollViewer.HorizontalScrollBarVisibility = scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;

                    list.Header = includeHeadersAndFooters ? new TextBlock { Text = "Header", Height = 60, Width = 70 } : null;
                    list.Footer = includeHeadersAndFooters ? new TextBlock { Text = "Footer", Height = 80, Width = 90 } : null;
                });
                TestServices.WindowHelper.WaitForIdle();
            });

            var validateViewportAtEnd = new Action<bool>((isHorizontal) =>
            {
                TestServices.WindowHelper.WaitForIdle();
                UIExecutor.Execute(() =>
                {
                    // Validate we are always at the very end.
                    Verify.IsTrue(
                        (isHorizontal && scrollViewer.ExtentWidth - scrollViewer.ViewportWidth - scrollViewer.HorizontalOffset < 1.0 ) ||
                        (!isHorizontal && scrollViewer.ExtentHeight - scrollViewer.ViewportHeight - scrollViewer.VerticalOffset < 1.0));
                });
            });

            foreach (bool includeHeaderAndFooter in new bool[] { false, true })
            {
                foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
                {
                    bool isHorizontal = (orientation == Orientation.Horizontal);
                    loadList(orientation, includeHeaderAndFooter);

                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Bulk add of initial 500 items");
                        for (int i = 0; i < 500; ++i)
                        {
                            data.Add(string.Format("Item #{0}", data.Count + 1));
                        }
                    });
                    validateViewportAtEnd(isHorizontal);


                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Bulk add of 500 items *before* the last element.");
                        for (int i = 0; i < 500; ++i)
                        {
                            data.Insert(data.Count - 1, string.Format("Item #{0}", data.Count + 1));
                        }
                    });
                    validateViewportAtEnd(isHorizontal);

                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Bulk add of 500 items *after* the last element.");
                        for (int i = 0; i < 500; ++i)
                        {
                            data.Add(string.Format("Item #{0}", data.Count + 1));
                        }
                    });
                    validateViewportAtEnd(isHorizontal);

                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Bulk add of 500 items *after* the last element (another one).");
                        for (int i = 0; i < 500; ++i)
                        {
                            data.Add(string.Format("Item #{0}", data.Count + 1));
                        }
                    });
                    validateViewportAtEnd(isHorizontal);

                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Bulk delete of items.");
                        while (data.Count > 500)
                        {
                            data.RemoveAt(0);
                        }
                    });
                    validateViewportAtEnd(isHorizontal);
                }
            }
        }

        [TestMethod]
        public void CanStartScrolledToTheBottomWithKeepLastItemInView()
        {
            // In this test, we have a ListView with ItemsStackPanel.ItemsUpdatingScrollMode==KeepLastItemInView.
            // We want to validate that the viewport always starts at the bottom.
            // There are three scenarios:
            //  - The data (i.e. ItemsSource value) is non-empty and set before the list is loaded.
            //  - The data is initially null and then set to a non-empty list after the list loaded.
            //  - The data is initially an empty list, then populated later.
            // In all of these scenarios, the list should start at the bottom.

            ListView list = null;
            ItemsStackPanel panel = null;
            var listLoaded = new AutoResetEvent(false);
            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsPanel = (ItemsPanelTemplate)Markup.XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemsStackPanel ItemsUpdatingScrollMode='KeepLastItemInView' CacheLength='0' /></ItemsPanelTemplate>");
                Log.Comment("Setting the ItemsSource **before** the list is loaded.");
                list.ItemsSource = Enumerable.Range(0, 1000).Select(i => "Item #" + i);
                list.Height = 420;
                list.Loaded += delegate
                {
                    panel = (ItemsStackPanel)list.ItemsPanelRoot;
                    listLoaded.Set();
                };
                list.ContainerContentChanging += (o, e) =>
                {
                    if (e.InRecycleQueue == false)
                    {
                        // Index #0 is the special container. It's always pinned
                        // by the modern panels if it exists.
                        Verify.IsTrue(e.ItemIndex == 0 || e.ItemIndex >= 989);
                    }
                };

                var root = new StackPanel();
                root.Children.Add(new Button() { Content = "Button" });
                root.Children.Add(list);

                TestServices.WindowHelper.WindowContent = root;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);

                Log.Comment("Setting the ItemsSource to null");
                list.ItemsSource = null;
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(-1, panel.FirstVisibleIndex);
                Verify.AreEqual(-1, panel.LastVisibleIndex);

                Log.Comment("Setting the ItemsSource **after** the list is loaded.");
                list.ItemsSource = Enumerable.Range(0, 1000).Select(i => "Item #" + i);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);

                Log.Comment("Setting the ItemsSource to an empty list");
                list.ItemsSource = new ObservableCollection<string>();
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(-1, panel.FirstVisibleIndex);
                Verify.AreEqual(-1, panel.LastVisibleIndex);

                Log.Comment("Populating an empty ItemsSource **after** the list is loaded.");
                var data = (ObservableCollection<string>)list.ItemsSource;
                for (int i = 0; i < 1000; ++i) data.Add("Item #" + i);
            });
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanStayScrolledToTheBottomAfterResetWithKeepLastItemInView()
        {
            ListView list = null;
            ItemsStackPanel panel = null;
            var data = new ResetCollection(Enumerable.Range(0, 1000).Select(i => "Item #" + i));
            var listLoaded = new AutoResetEvent(false);

            // In this test, we have a ListView with ItemsStackPanel.ItemsUpdatingScrollMode==KeepLastItemInView.
            // We want to validate that a 'soft' reset (with the same data as before) doesn't change the viewport.
            // And that a 'hard' reset (with completely different data) takes us at the bottom of the list.

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsPanel = (ItemsPanelTemplate)Markup.XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemsStackPanel ItemsUpdatingScrollMode='KeepLastItemInView' CacheLength='0' /></ItemsPanelTemplate>");
                list.ItemsSource = data;
                list.Height = 420;
                list.Loaded += delegate
                {
                    panel = (ItemsStackPanel)list.ItemsPanelRoot;
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);
                data.Reset();
                list.UpdateLayout();
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);

                list.ScrollIntoView(data[800]);
                list.UpdateLayout();
                Verify.AreEqual(800, panel.FirstVisibleIndex);
                Verify.AreEqual(810, panel.LastVisibleIndex);

                data.Reset();
                list.UpdateLayout();
                Verify.AreEqual(800, panel.FirstVisibleIndex);
                Verify.AreEqual(810, panel.LastVisibleIndex);

                data.ResetWith(Enumerable.Range(0, 1000).Select(i => "Item #" + -i));
                list.UpdateLayout();
                Verify.AreEqual(989, panel.FirstVisibleIndex);
                Verify.AreEqual(999, panel.LastVisibleIndex);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]   // DCPP: Tests are failing during shutdown on WPF due to outstanding animations
        public void CanTrackGroupHeaderInNonContiguousForwardSituation()
        {
            ListView list = null;
            var groups = GetGroupedDataCollection(groupCount: 20);
            var listLoaded = new AutoResetEvent(false);

            // In this test, we have a ListView with an ItemsStackPanel that has a CacheLength of zero.
            // The data source consists of 20 groups with 4 items each.
            // We scroll to the last element of group 15 (i.e. ItemsStackPanel.FirstVisibleIndex == 63).
            // In this situation, we have a gap in the realized range:
            // Header 15
            //  |- Item 63
            // Header 16
            //  |- Item 64
            //  |- Item 65
            //  ...
            // We then change the viewport so that header 15 is the first visible element.
            // After that, we insert an item before header 15 so that we start tracking it.
            // And then we run layout.
            // During layout, before we can use header 15 as the anchor, we need to clear what comes afterward
            // to honor the contiguousness invariant that's assumed in the modern panels logic.

            UIExecutor.Execute(() =>
            {
                Log.Comment("Preparing list.");
                list = new ListView();
                list.ItemsPanel = (ItemsPanelTemplate)Markup.XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemsStackPanel CacheLength='0' /></ItemsPanelTemplate>");
                list.GroupStyle.Add(new GroupStyle
                {
                    HeaderTemplate = (DataTemplate)Markup.XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding Header}' /></DataTemplate>")
                });
                var cvs = new CollectionViewSource();
                cvs.Source = groups;
                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Items");
                list.ItemsSource = cvs.View;

                list.SelectionMode = ListViewSelectionMode.None;
                list.Height = 500;
                list.Loaded += delegate { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling the last item of group 15 into view.");
                var targetGroupIndex = 15;
                var targetGroup = groups[targetGroupIndex];
                var targetItem = new MoCoFlatDataModel("Target item");
                targetGroup.Items.Add(targetItem);
                list.ScrollIntoView(targetItem, ScrollIntoViewAlignment.Leading);
                list.UpdateLayout();

                Log.Comment("Moving the viewport so that header 15 is the first visible element.");
                var isp = (ItemsStackPanel)list.ItemsPanelRoot;
                var targetItemContainer = (FrameworkElement)list.ContainerFromItem(targetItem);
                var changeViewOffset = targetItemContainer.TransformToVisual(isp).TransformPoint(new Point(0, 0)).Y;
                changeViewOffset -= targetGroup.Items.Count * targetItemContainer.ActualHeight;
                list.FindElementOfTypeInSubtree<ScrollViewer>().ChangeView(null, changeViewOffset, null, disableAnimation: true);

                Log.Comment("Tracking group header 15.");
                groups[targetGroupIndex - 1].Items.Add(new MoCoFlatDataModel("Trigger tracking of targetGroup"));
                list.UpdateLayout();    // We used to crash during this call.

                // +1 for the item we inserted to trigger tracking.
                Verify.AreEqual(targetGroupIndex * 4 + 1, isp.FirstVisibleIndex);
            });
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void CanTrackGroupHeaderInNonContiguousBackwardSituation()
        {
            ListView list = null;
            var groups = GetGroupedDataCollection(groupCount: 10, itemsPerGroup: 100);
            var listLoaded = new AutoResetEvent(false);

            // In this test, we have a ListView with an ItemsStackPanel.
            // The data source consists of 10 groups with 100 items each (enough for one group to fill the buffer).
            // We scroll to the last element of group 5 (i.e. ItemsStackPanel.FirstVisibleIndex == 599).
            // We end up with a situation like this:

            //  -------------- buffer starts
            // Header 5
            //  |- Item 597
            //  |- Item 598
            //  -------------- visible window start
            //  |- Item 599
            // Header 6
            //  |- Item 600
            //  |- Item 601
            //  |- ...
            //  |- Item 624
            //  -------------- visible window ends
            //  |- Item 625
            //  |- Item 626
            //  -------------- buffer ends

            // We then clear group 6 and replace (delete/add) item 599.
            // Consequentially, two things will happen:
            //  - we will start tracking header 6 and use it as an anchor during the next layout.
            //  - the items block is non-contiguous with regard to header 6 (header 5 ... Item 598, **Gap**, header 6).
            // During layout, before we can use header 6 as the anchor, we need to clear what comes before it
            // (i.e. header 5 ... Item 598) to honor the contiguousness invariant that's assumed in the modern panels logic.

            UIExecutor.Execute(() =>
            {
                Log.Comment("Preparing list.");
                list = new ListView();
                list.GroupStyle.Add(new GroupStyle
                {
                    HeaderTemplate = (DataTemplate)Markup.XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='{Binding Header}' /></DataTemplate>")
                });
                var cvs = new CollectionViewSource();
                cvs.Source = groups;
                cvs.IsSourceGrouped = true;
                cvs.ItemsPath = new PropertyPath("Items");
                list.ItemsSource = cvs.View;

                list.SelectionMode = ListViewSelectionMode.None;
                list.Height = 400;
                list.Loaded += delegate { listLoaded.Set(); };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling the last item of group 5 into view.");
                var targetGroupIndex = 5;
                var targetGroup = groups[targetGroupIndex];
                var targetItem = targetGroup.Items[targetGroup.Items.Count - 1];
                list.ScrollIntoView(targetItem, ScrollIntoViewAlignment.Leading);
            });
            // It's important to let the cache buffer grow a little.
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Replace (delete/add) the last item of group 5 and clear group 6");
                groups[5].Items.RemoveAt(groups[5].Items.Count - 1);
                groups[6].Items.Clear();
                groups[5].Items.Add(new MoCoFlatDataModel("Inserted item"));

                // Now we are tracking group header 6.
                // We used to crash during this call.
                list.UpdateLayout();

                Log.Comment("Validates the first visible item is 599");
                var isp = (ItemsStackPanel)list.ItemsPanelRoot;
                Verify.AreEqual(599, isp.FirstVisibleIndex);
            });
        }

        [TestMethod]
        public void CanShrinkListViewWithKeepLastItemInView()
        {
            CanShrinkListViewWithKeepLastItemInView(VerticalAlignment.Top /*itemsStackPanelVerticalAlignment*/, false /*useCustomItemTemplate*/);
            CanShrinkListViewWithKeepLastItemInView(VerticalAlignment.Bottom /*itemsStackPanelVerticalAlignment*/, true /*useCustomItemTemplate*/);
            CanShrinkListViewWithKeepLastItemInView(VerticalAlignment.Top /*itemsStackPanelVerticalAlignment*/, false /*useCustomItemTemplate*/);
            CanShrinkListViewWithKeepLastItemInView(VerticalAlignment.Bottom /*itemsStackPanelVerticalAlignment*/, true /*useCustomItemTemplate*/);
        }

        #region Private methods

        private void CanShrinkListViewWithKeepLastItemInView(VerticalAlignment itemsStackPanelVerticalAlignment, bool useCustomItemTemplate)
        {
            // In this test, we have a ListView with ItemsStackPanel.ItemsUpdatingScrollMode==KeepLastItemInView and VerticalAlignment==Top or Bottom.
            // It uses either the default null ItemTemplate or a custom one with a wrapping TextBlock.
            // The ListView is shrunk by 10px both horizontally and vertically several times and the test checks each time that the inner ScrollViewer's
            // VerticalOffset equals the ScrollableHeight (i.e. the last item is kept totally in view).

            Grid root = null;
            ListView listView = null;
            ScrollViewer scrollViewer = null;
            ItemsStackPanel itemsStackPanel = null;
            var listViewLoaded = new AutoResetEvent(false);

            UIExecutor.Execute(() =>
            {
                Log.Comment("Setting up ListView.");
                listView = new ListView();

                Log.Comment("Setting ItemsPanel.");
                listView.ItemsPanel = 
                    Markup.XamlReader.Load("<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><ItemsStackPanel ItemsUpdatingScrollMode='KeepLastItemInView'/></ItemsPanelTemplate>") as ItemsPanelTemplate;

                if (useCustomItemTemplate)
                {
                    Log.Comment("Setting ItemTemplate.");
                    listView.ItemTemplate =
                        Markup.XamlReader.Load("<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock HorizontalAlignment='Left' Text='{Binding}' FontSize='18' MaxLines='0' TextTrimming='CharacterEllipsis' TextWrapping='Wrap'/></DataTemplate>") as DataTemplate;
                }

                Log.Comment("Setting the ItemsSource before the ListView is loaded.");
                listView.ItemsSource = Enumerable.Range(0, 10).Select(i => "ListView Item #" + i);

                listView.Loaded += delegate
                {
                    itemsStackPanel = listView.ItemsPanelRoot as ItemsStackPanel;
                    Verify.IsNotNull(itemsStackPanel, "itemsStackPanel is null.");

                    listViewLoaded.Set();
                };

                Log.Comment("Setting up root Grid.");
                root = new Grid()
                {
                    Background = new SolidColorBrush(Colors.Gray),
                    MaxWidth = 200,
                    MaxHeight = 250
                };

                root.Children.Add(listView);

                TestServices.WindowHelper.WindowContent = root;
            });

            Verify.IsTrue(listViewLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load ListView.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                if (itemsStackPanel.VerticalAlignment != itemsStackPanelVerticalAlignment)
                {
                    Log.Comment("Setting ItemsStackPanel.VerticalAlignment.");
                    itemsStackPanel.VerticalAlignment = itemsStackPanelVerticalAlignment;
                }

                Log.Comment("Accessing ScrollViewer.");
                scrollViewer = listView.FindElementOfTypeInSubtree<ScrollViewer>();
                Verify.IsNotNull(scrollViewer, "scrollViewer is null.");
            });

            TestServices.WindowHelper.WaitForIdle();

            for (int iteration = 0; iteration < 12; iteration++)
            {
                UIExecutor.Execute(() =>
                {
                    double verticalOffset = scrollViewer.VerticalOffset;
                    double viewportHeight = scrollViewer.ViewportHeight;
                    double extentHeight = scrollViewer.ExtentHeight;
                    double scrollableHeight = scrollViewer.ScrollableHeight;

                    Log.Comment("root.ActualWidth: " + root.ActualWidth.ToString());
                    Log.Comment("root.ActualHeight: " + root.ActualHeight.ToString());
                    Log.Comment("scrollViewer.VerticalOffset: " + verticalOffset.ToString());
                    Log.Comment("scrollViewer.ViewportHeight: " + viewportHeight.ToString());
                    Log.Comment("scrollViewer.ExtentHeight: " + extentHeight.ToString());
                    Log.Comment("scrollViewer.ScrollableHeight: " + scrollableHeight.ToString());

                    Verify.IsTrue(Math.Abs(extentHeight - viewportHeight - scrollableHeight) < 1.0);
                    Verify.IsTrue(Math.Abs(verticalOffset - scrollableHeight) < 1.0);

                    Log.Comment("Setting root.Width & .Height.");
                    root.Width = root.ActualWidth - 10.0;
                    root.Height = root.ActualHeight - 10.0;
                });

                TestServices.WindowHelper.WaitForIdle();
            }
        }

        /// <summary>
        /// Helper method for setting up the list by populating the list and scrolling to the given item
        /// </summary>
        /// <param name="list">ListView/GridView</param>
        /// <param name="numOfItems">Number of list items to be populated</param>
        /// <param name="scrollToIndex">Index of the item to be scrolled</param>
        /// 
        private void SetupListForDataManipulation(ListViewBase list, int numOfItems, int scrollToIndex)
        {
            // Set panel orientation and Populate the list
            //
            UIExecutor.Execute(() =>
            {
                // Populate the list
                //
                Log.Comment("Populating list...");
                list.ItemsSource = GetFlatDataCollection(numOfItems);
            });

            // Scroll to middle
            //
            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to index " + scrollToIndex + "...");
                ScrollTo(scrollToIndex, list, ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        #endregion
    }
}
