// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    /// <summary>
    /// Class for testing the maintain viewport behavior when performing scrolling in ListView/GridView
    /// </summary>
    /// 
    [TestClass]
    public class MoCoViewportScrollTests : MoCoViewportTestBase
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
        /// Performs some setup for the test case, such as loading the xaml and finding the list controls
        /// </summary>
        /// 
        private void InternalSetup()
        {
            // Load Xaml
            //
            string fileName = Path.Combine(TestDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/BasicViewportTest.xaml");
            var TargetPanel = LoadXamlTestPage(fileName);
            Verify.IsNotNull(TargetPanel, "TargetPanel not found");

            UIExecutor.Execute(() =>
            {
                // Find the list controls
                //
                lv = TargetPanel.FindNameInSubtree("lv") as ListView;
                Verify.IsNotNull(lv, "ListView not found");
            });
        }

        /// <summary>
        /// Tests data manipulations on a flat data collection and scrolls to the manipulated item
        /// Verifies the data manipulation does not change the items in the viewport
        /// Verifies the manipulated item is scrolled to the view
        /// 
        /// Variations:
        /// 1. List controls {ListView, GridView}
        /// 2. Panel orientations {Horizontal, Vertical}
        /// 3. Data manipulations {Insert, Remove, Resize}
        /// 
        /// Steps:
        /// 1. Load the XAML
        /// 2. Set the Panel orientation and populate the list
        /// 3. Scroll the list as appropriate
        /// 4. Perform the data manipulation off the viewport
        /// 5. Scroll to the manipulated item
        /// 6. Verify the current position of the manipulated item
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        public void TestFlatDataManipulationsAndScroll()
        {
            ListViewBase list = null; // List control under tests

            int initialScrollIndex = 5;
            int newItemIndex = 4;
            MoCoFlatDataModel manipulatedItem = null;
            ObservableCollection<MoCoFlatDataModel> dataCollection = null;

            // List of data manipulations to test
            //
            List<DataManipulationType> dataManipulations = new List<DataManipulationType>()
            {
                DataManipulationType.INSERT,
                DataManipulationType.REMOVE,
                DataManipulationType.EXPAND,
                DataManipulationType.COLLAPSE
            };

            // Load Xaml
            //
            InternalSetup();


            UIExecutor.Execute(() =>
            {
                list = lv;
                // Set this flag to avoid black placeholders while scrolling which may cause image comparision
                //
                list.ShowsScrollingPlaceholders = false;
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

                    // Populated the list
                    dataCollection = GetFlatDataCollection(20);
                    Verify.IsNotNull(dataCollection, "DataCollection is null");
                    list.ItemsSource = dataCollection;
                });

                TestServices.WindowHelper.WaitForIdle();

                // Perform all different pre-defined data manipulations (eg., Insert, Remove, Expand and Collapse)
                //
                foreach (DataManipulationType dmType in dataManipulations)
                {
                    // Scroll the list to middle
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Scrolling to middle...");
                        list.ScrollIntoView(list.Items[initialScrollIndex], ScrollIntoViewAlignment.Leading);
                    });

                    TestServices.WindowHelper.WaitForIdle();

                    // Manipulate an item off the viewport and verify the visual and viewport changes
                    //
                    TestFlatDataManipulations(list, dmType, newItemIndex, orientation, ScrollPosition.MIDDLE);

                    // Verify offset before scrolling to the manipulated item
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Verifying before scrolling to the manipulated item...");

                        // Find the index of the manipulated item
                        //
                        int manipulatedItemIndex = dmType == DataManipulationType.REMOVE ? newItemIndex - 1 : newItemIndex;
                        manipulatedItem = dataCollection[manipulatedItemIndex];

                        FrameworkElement lvi = list.ContainerFromItem(manipulatedItem) as FrameworkElement;
                        if (lvi == null)
                        {
                            Log.Warning("New item is not yet virtualized");
                        }
                        else
                        {
                            Point offset = lvi.TransformToVisual(list).TransformPoint(new Point());
                            Log.Warning(string.Format("Before scroll: X={0} Y={1} ", offset.X, offset.Y));

                            if (orientation == Orientation.Horizontal)
                            {
                                Verify.IsTrue(offset.X < 0, "Manipulated item is not off the viewport");
                                Verify.AreEqual(0, offset.Y, "Unexpected vertical offset in the manipulated item");
                            }
                            else
                            {
                                Verify.IsTrue(offset.Y < 0, "Manipulated item is not off the viewport");
                                Verify.AreEqual(0, offset.X, "Unexpected horizontal offset in the manipulated item");
                            }
                        }
                    });


                    // Scroll to the manipulated item
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Scrolling to the manipulated item...");
                        list.ScrollIntoView(manipulatedItem, ScrollIntoViewAlignment.Leading);
                    });

                    TestServices.WindowHelper.WaitForIdle();

                    // Verify the manipulated item is in the viewport
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Verifying after scrolling to the manipulated item...");
                        FrameworkElement lvi = list.ContainerFromItem(manipulatedItem) as FrameworkElement;
                        Verify.IsNotNull(lvi, "Unable to find the manipulated item");

                        Point offset = lvi.TransformToVisual(list).TransformPoint(new Point());
                        Log.Comment(string.Format("After scroll: X={0} Y={1} ", offset.X, offset.Y));

                        Verify.IsTrue(offset.X == 0 && offset.Y == 0, "Manipulate items is not scrolled to view");
                    });
                }
            }
        }

        /// <summary>
        /// Tests data manipulations on a grouped data collection and scrolls to the manipulated group
        /// Verifies the data manipulation does not change the items in the viewport
        /// Verifies the manipulated group is scrolled to the view
        /// 
        /// Variations:
        /// 1. List controls {ListView, GridView}
        /// 2. Panel orientations {Horizontal, Vertical}
        /// 3. Data manipulations {Insert, Remove, Resize}
        /// 
        /// Steps:
        /// 1. Load the XAML
        /// 2. Set the Panel orientation and populate the list
        /// 3. Scroll the list as appropriate
        /// 4. Perform the data manipulation off the viewport
        /// 5. Scroll to the manipulated group
        /// 6. Verify the current position of the manipulated group items
        /// </summary>
        /// 
        [TestMethod]
        [Priority(1)]
        [TestProperty("Hosting:Mode", "UAP")]
        public void TestGroupedDataManipulationsAndScroll()
        {
            ListViewBase list = null; // List control under tests

            int initialScrollIndex = 5;
            int newGroupIndex = 4;
            ObservableCollection<MoCoGroupedDataModel> dataCollection = null;
            MoCoGroupedDataModel manipulatedGroup = null;
            
            // List of data manipulations to test
            //
            List<DataManipulationType> dataManipulations = new List<DataManipulationType>()
            {
                DataManipulationType.INSERT_GROUP,
                DataManipulationType.REMOVE_GROUP,
                DataManipulationType.EXPAND_GROUP,
                DataManipulationType.COLLAPSE_GROUP
            };

            // Load Xaml
            //
            InternalSetup();

                Orientation orientation = Orientation.Vertical;
                UIExecutor.Execute(() =>
                {
                    list = lv;
                    // Set this flag to avoid black placeholders while scrolling which may cause image comparision
                    //
                    list.ShowsScrollingPlaceholders = false;
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
                    // Populate the list
                    //
                    this.cvs = new CollectionViewSource();
                    dataCollection = GetGroupedDataCollection(20);
                    this.cvs.Source = dataCollection;
                    this.cvs.IsSourceGrouped = true;
                    this.cvs.ItemsPath = new PropertyPath("Items");
                    list.ItemsSource = this.cvs.View;
                });

                TestServices.WindowHelper.WaitForIdle();

                foreach (DataManipulationType dmType in dataManipulations)
                {
                    // Scroll the list to middle
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Scrolling to middle group...");
                        ScrollToGroup(initialScrollIndex, list, ScrollIntoViewAlignment.Leading);
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    // Manipulate an item off the viewport and verify the visual and viewport changes
                    //
                    UIExecutor.Execute(() =>
                    {
                        PerformGroupedDataManipulation(list, dmType, newGroupIndex);
                    });
                    TestServices.WindowHelper.WaitForIdle();

                    // Verify offset before scrolling to the manipulated item
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Verifying before scrolling to the manipulated item...");

                        // Find the manipulated group to scroll and verify
                        //
                        if (dmType == DataManipulationType.REMOVE_GROUP)
                        {
                            manipulatedGroup = dataCollection[newGroupIndex - 1];
                        }
                        else
                        {
                            manipulatedGroup = dataCollection[newGroupIndex];
                        }

                        // Find and verify offset of all the items under the manupulated group
                        //
                        foreach (var item in manipulatedGroup.Items)
                        {
                            var lvi = list.ContainerFromItem(item) as FrameworkElement;
                            if (lvi == null)
                            {
                                Log.Comment(item.Name + " not yet virtualized");
                            }
                            else
                            {
                                Point offset = lvi.TransformToVisual(list).TransformPoint(new Point());
                                Log.Warning(string.Format("{2} - Before scroll: X={0} Y={1} ", offset.X, offset.Y, item.Name));

                                // Verify the offset of the group items before scroll
                                //
                                Verify.IsTrue(offset.Y < 0, "Manipulated item is not off the viewport");
                                Verify.AreEqual(0, offset.X, "Unexpected horizontal offset in the manipulated item");
                            }
                        }
                    });

                    TestServices.WindowHelper.WaitForIdle();

                    // Scroll to the manipulated item
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Scrolling to the manipulated group...");
                        list.ScrollIntoView(manipulatedGroup, ScrollIntoViewAlignment.Leading);

                        list.InvalidateMeasure();
                        list.UpdateLayout();
                        list.ScrollIntoView(manipulatedGroup, ScrollIntoViewAlignment.Leading);
                    });
                    TestServices.WindowHelper.WaitForIdle();


                    // Verify the manipulated item is in the viewport
                    //
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Verifying after scrolling to the manipulated group...");

                        // Find offset of all the items under the manupulated group
                        //
                        foreach (var item in manipulatedGroup.Items)
                        {
                            var lvi = list.ContainerFromItem(item) as FrameworkElement;
                            Verify.IsNotNull(lvi, "Unable to find the manipulated item");

                            Point offset = lvi.TransformToVisual(list).TransformPoint(new Point());
                            Log.Warning(string.Format("{2} - After scroll: X={0} Y={1} ", offset.X, offset.Y, item.Name));

                            // Verify the offset of the group items before scroll
                            //
                            Verify.IsTrue(offset.Y >= 0, "Manipulated item is not scrolled to the viewport");
                            Verify.AreEqual(0, offset.X, "Unexpected horizontal offset in the manipulated item");

                        }
                    });
                }
        }

        #region Data
        ListView lv = null;
        #endregion
    }
}
