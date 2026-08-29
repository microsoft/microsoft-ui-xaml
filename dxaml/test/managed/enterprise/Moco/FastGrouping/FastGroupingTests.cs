// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Threading;
using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using System;
using Microsoft.UI.Xaml.Input;
using System.Linq;

using Private.Infrastructure;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;
using Windows.ApplicationModel.DataTransfer;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.FastGrouping
{
    [TestClass]
    public class FastGroupingTests : FastGroupingTestBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
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

        #region Moco:GridView Panel:ItemsWrapGrid VirtualizationDirection:Horizontal

        void LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal()
        {
            LoadXaml("GridView_Template", "ItemsWrapGrid", "Vertical", "ScrollViewer.HorizontalScrollBarVisibility='Auto' ScrollViewer.VerticalScrollBarVisibility='Disabled' ScrollViewer.HorizontalScrollMode='Enabled' ScrollViewer.VerticalScrollMode='Disabled'");
        }

        [TestMethod]
        public void GV_IWG_HVirt_Basics()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 1);

            PerformScenarios(DataModel.OC_OC_IsA,
                () =>
                {
                    // Validate Layout - OnLoaded
                    ValidateVisibleIndeces(0, 19);
                },
                () =>
                {
                    // Validate GroupScrolledIntoView
                    ValidateVisibleIndeces(29, 50);
                },
                () =>
                {
                    // Validate ItemScrolledIntoView
                    ValidateVisibleIndeces(0, 19);
                });

            ValidateReset("G:0",
               () =>
               {
                   // Validate Reset
                   ValidateVisibleIndeces(0, 24);
               });
        }

        [TestMethod]
        public void GV_IWG_HVirt_Panning()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            // Validate indeces on loaded
            ValidateVisibleIndeces(0, 39);

            Log.Comment("1. Panning forward using touch");
            PanRightToEnd();

            // Wait for rubberband animations to settle down 
            TestServices.WindowHelper.WaitForIdle();
            ValidateVisibleIndeces(10, 49);

            Log.Comment("2. Panning backward using touch");
            PanLeftToBegining();
            ValidateVisibleIndeces(0, 39);
        }

        [TestMethod]
        public void GV_IWG_HVirt_HideIfEmpty()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 3);
            SetHidesIfEmpty(true);
            TestServices.WindowHelper.WaitForIdle();

            PerformScenarios(DataModel.OC_OC_IsA,
                () =>
                {
                    // Validate Layout - OnLoaded
                    ValidateVisibleIndeces(0, 39);
                },
                () =>
                {
                    // Validate Layout - GroupScolledIntoView
                    ValidateVisibleIndeces(6, 49);
                },
                () =>
                {
                    // Validate ItemScrolledIntoView
                    ValidateVisibleIndeces(0, 39);
                });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // TestIssue: Investigate why setting selection mode is not working
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void GV_IWG_HVirt_Selection()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            Log.Comment("Verify Single Select Programatically");
            // Programatic selection
            UIExecutor.Execute(() =>
            {
                ((controlUnderTest.ControlInstance) as ListViewBase).SelectionMode = ListViewSelectionMode.Single;
                //((controlUnderTest.ControlInstance) as ListViewBase).SelectedItems.Clear();
            });

            VerifyListViewSingleSelect(controlUnderTest, 5);

            Log.Comment("Verify MultiSelect using Touch");
            // Selections using Touch
            UIExecutor.Execute(() =>
            {
                ((controlUnderTest.ControlInstance) as ListViewBase).SelectionMode = ListViewSelectionMode.Multiple;
            });

            VerifyListViewMultipleSelect(controlUnderTest, 5, 5, PointerInputType.Touch);

            Log.Comment("Verify ExtendedSelect using Touch");
            UIExecutor.Execute(() =>
            {
                ((controlUnderTest.ControlInstance) as ListViewBase).SelectionMode = ListViewSelectionMode.Extended;
            });

            VerifyListViewMultipleSelect(controlUnderTest, 5, 1, PointerInputType.Touch);
        }

        [TestMethod]
        public void GV_IWG_HVirt_GroupHeaderPlacement()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            // Top is the default
            TestServices.WindowHelper.WaitForIdle();
            ValidateVisibleIndeces(0, 39);

            SetGroupHeaderPlacement(Microsoft.UI.Xaml.Controls.Primitives.GroupHeaderPlacement.Left);
            TestServices.WindowHelper.WaitForIdle();
            ValidateVisibleIndeces(0, 19);

            SetGroupHeaderPlacement(Microsoft.UI.Xaml.Controls.Primitives.GroupHeaderPlacement.Top);
            TestServices.WindowHelper.WaitForIdle();
            ValidateVisibleIndeces(0, 39);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_ForwardTabTest()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() { "tab", "tab", "tab", "tab", "tab" };
            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_BackwardTabTest()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() { "shifttab", "shifttab", "shifttab", "shifttab", "shifttab" };
            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(Button), "Before"),
                Tuple.Create(typeof(Button), "After"),
            };

            VerifyKeyNavOrder(false, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void GV_IWG_HVirt_KeyNav_TabArrowTest()
        {
            LoadXaml("GridView_Template", "ItemsWrapGrid", "Vertical", "ScrollViewer.HorizontalScrollBarVisibility='Auto' ScrollViewer.VerticalScrollBarVisibility='Disabled' ScrollViewer.HorizontalScrollMode='Enabled' ScrollViewer.VerticalScrollMode='Disabled'");
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() {
                // Initial tab should focus header
                "tab",
                // Next tab should focus the selected item/first item
                "tab",
                // In-order backwards from the first item should focus the group header
                "up",
                // In-order forwards from the group header should take you back to the first item
                "down",
                // Not in-order backwards from the first row should focus the group header
                "left",
                // In-order forwards from the group header should take you back to the previously focused item
                "right",
                // In-order navigation forward should take you to the first item in the next row
                "down", "down", "down", "down", "down", "down",
                // In-order navigation backward should take you to the last item in the previous row
                "up",
                // Not in-order backwards from the first row should focus the group header
                "left",
                // Not in-order forwards from the group header should take you back to the previously focused item
                "right",
                // Not in-order forwards should take you between columns and if the new column is shorter, it will take you to the last row in it
                "right",
                // Not in-order backwards should take you between columns while preserving the row.
                "left",
                // Not in-order backwards from the first row should focus the group header
                "left",
                // Not in-order forwards from the group header should take you back to the previously focused item
                "right",
                // Not in-order forwards should take you between columns while preserving the row.
                "right",
                // Not in-order forwards should focus group header if navigating between groups
                "right",
                // Not in-order forwards from a group header should focus an element in the same row as the previously focused item
                "right",
                // Not in-order backwards should focus group header if navigating between groups
                "left",
                // Not in-order backwards from a group header should focus an element in the same row as the previously focused item
                "left",
                // In-order navigation forward into a new group should focus the group header.
                "down",
                // In-order forwards from a group header should focus the first item in the group.
                "down",
                // Tabbing out to the header and back in should go back to the selected item.
                "shifttab", "tab",
                // Tabbing out to the footer and back in should go back to the selected item.
                "tab", "shifttab",
                // Tabbing forward should take you to the footer.
                "tab",
                // Tabbing out the listviewbase should focus the next control.
                "tab",
                // Tabbing past the last control should wrap focus back to the beginning.
                "tab"
            };

        List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:2"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:3"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:4"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:5"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:6"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:5"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:5"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:3"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:3"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:3"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        // [TestMethod] 24023441
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void GV_IWG_VVirt_KeyNav_TabArrowTest()
        {
            LoadXaml("GridView_Template", "ItemsWrapGrid", "Horizontal", "ScrollViewer.HorizontalScrollBarVisibility='Disabled' ScrollViewer.VerticalScrollBarVisibility='Auto' ScrollViewer.HorizontalScrollMode='Disabled' ScrollViewer.VerticalScrollMode='Enabled'");
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() {
                // Initial tab should focus header
                "tab",
                // Next tab should focus the selected item/first item
                "tab",
                // In-order backwards from the first item should focus the group header
                "left",
                // In-order forwards from the group header should take you back to the first item
                "right",
                // Not in-order backwards from the first row should focus the group header
                "up",
                // In-order forwards from the group header should take you back to the previously focused item
                "down",
                // In-order navigation forward should take you to the first item in the next row
                "right", "right", "right", "right", "right", "right", "right",
                // In-order navigation backward should take you to the last item in the previous row
                "left",
                // Not in-order backwards from the first row should focus the group header
                "up",
                // Not in-order forwards from the group header should take you back to the previously focused item
                "down",
                // Not in-order forwards should take you between columns and if the new column is shorter, it will take you to the last row in it
                "down",
                // Not in-order backwards should take you between columns while preserving the row.
                "up",
                // Not in-order backwards from the first row should focus the group header
                "up",
                // Not in-order forwards from the group header should take you back to the previously focused item
                "down",
                // Not in-order forwards should take you between columns while preserving the row.
                "down",
                // Not in-order forwards should focus group header if navigating between groups
                "down",
                // Not in-order forwards from a group header should focus an element in the same row as the previously focused item
                "down",
                // Not in-order backwards should focus group header if navigating between groups
                "up",
                // Not in-order backwards from a group header should focus an element in the same row as the previously focused item
                "up",
                // In-order navigation forward into a new group should focus the group header.
                "right",
                // In-order forwards from a group header should focus the first item in the group.
                "right",
                // Tabbing out to the header and back in should go back to the selected item.
                "shifttab", "tab",
                // Tabbing out to the footer and back in should go back to the selected item.
                "tab", "shifttab",
                // Tabbing forward should take you to the footer.
                "tab",
                // Tabbing out the listviewbase should focus the next control.
                "tab",
                // Tabbing past the last control should wrap focus back to the beginning.
                "tab"
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:2"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:3"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:4"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:5"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:6"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:7"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:6"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:6"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:2"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:2"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:2"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:9"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_PageDownPageUpHomeEndTest()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() { "tab", "tab", "pagedown", "pagedown", "pagedown", "pagedown", "pageup", "pageup", "pageup", "pageup", "end", "end", "home", "home", "up", "pagedown", "end", "pageup" };
            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>() 
            { 
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),

                Tuple.Create(typeof(GridViewItem), "G:2:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),

                Tuple.Create(typeof(GridViewItem), "G:2:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),

                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),

                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),

                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:9"),

                Tuple.Create(typeof(GridViewItem), "G:4:I:9"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:6"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_TabNavigationLocal()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 5, 2, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:3"),
                Tuple.Create(typeof(GridViewItem), "G:3:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:3:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:4"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_TabNavigationLocal_EmptyGroups()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 2, 1);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "shifttab", "shifttab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_TabNavigationCycle()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 5, 2, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Cycle;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab"
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>() 
            { 
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:3"),
                Tuple.Create(typeof(GridViewItem), "G:3:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:3:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:4"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:4:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "Header")
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_TabNavigationCycle_EmptyGroups()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 2, 1);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Cycle;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "shifttab", "shifttab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:0:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:1:I:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2E0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:0"),
                Tuple.Create(typeof(GridViewItem), "G:2:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "Header"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void GV_IWG_HVirt_KeyNav_TabNavigationLocal_OnlyEmptyGroups()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirHorizontal();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 0, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab",
                "shifttab", "shifttab", "shifttab", "shifttab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:2"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:1"),
                Tuple.Create(typeof(GridViewHeaderItem), "G:0"),
                Tuple.Create(typeof(Button), "Header"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        #endregion

        #region Moco:ListView Panel:ItemsStackPanel VirtualizationDirection:Vertical

        void LoadXaml_ListView_ItemsStackPanel_VdirVertical()
        {
            LoadXaml("ListView_Template", "ItemsStackPanel", "Vertical");
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "OneCore")]
        public void LV_ISP_VVirt_Basics()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 1);

            // Note that scroll intoview and data scenarios are not supported yet.
            PerformScenarios(DataModel.OC_OC_IsA,
                 () =>
                 {
                     // Validate Layout - OnLoaded
                     ValidateVisibleIndeces(0, 6);
                 },
                () =>
                {
                    // Validate GroupScrolledIntoView
                    ValidateVisibleIndeces(39, 46);
                },
                () =>
                {
                    // Validate ItemScrolledIntoView
                    ValidateVisibleIndeces(0, 6);
                });

            ValidateReset("G:0",
               () =>
               {
                   // Validate Reset
                   ValidateVisibleIndeces(0, 5);
               });
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_ForwardTabTest()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() { "tab", "tab", "tab", "tab", "tab" };
            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>() 
                    { 
                        Tuple.Create(typeof(Button), "Header"),
                        Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                        Tuple.Create(typeof(Button), "Footer"),
                        Tuple.Create(typeof(Button), "After"),
                        Tuple.Create(typeof(Button), "Before"),
                    };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_BackwardTabTest()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 0);

            List<string> inputKeys = new List<string>() { "shifttab", "shifttab", "shifttab", "shifttab", "shifttab" };
            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
                    {
                        Tuple.Create(typeof(Button), "Footer"),
                        Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                        Tuple.Create(typeof(Button), "Header"),
                        Tuple.Create(typeof(Button), "Before"),
                        Tuple.Create(typeof(Button), "After"),
                    };

            VerifyKeyNavOrder(false, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_TabNavigationLocal()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 5, 2, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:3"),
                Tuple.Create(typeof(ListViewItem), "G:3:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:3:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:4"),
                Tuple.Create(typeof(ListViewItem), "G:4:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:4:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_TabNavigationLocal_EmptyGroups()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 2, 1);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "shifttab", "shifttab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "After"),
                Tuple.Create(typeof(Button), "Before"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_TabNavigationCycle()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 5, 2, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Cycle;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab"
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:3"),
                Tuple.Create(typeof(ListViewItem), "G:3:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:3:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:4"),
                Tuple.Create(typeof(ListViewItem), "G:4:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:4:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "Header")
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_TabNavigationCycle_EmptyGroups()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 2, 1);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Cycle;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "shifttab", "shifttab", "tab", "tab", "tab", "tab", "tab", "tab",
                "tab", "tab", "tab", "tab", "tab", "tab", "tab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:0:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:1:I:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2E0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:0"),
                Tuple.Create(typeof(ListViewItem), "G:2:I:1"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(Button), "Header"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        [TestMethod]
        public void LV_ISP_VVirt_KeyNav_TabNavigationLocal_OnlyEmptyGroups()
        {
            LoadXaml_ListView_ItemsStackPanel_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, 3, 0, 0);

            UIExecutor.Execute(() =>
            {
                (controlUnderTest.ControlInstance as ListViewBase).TabNavigation = KeyboardNavigationMode.Local;
            });

            List<string> inputKeys = new List<string>()
            {
                "tab", "tab", "tab", "tab", "tab",
                "shifttab", "shifttab", "shifttab", "shifttab",
            };

            List<Tuple<Type, string>> expectedFocusOrder = new List<Tuple<Type, string>>()
            {
                Tuple.Create(typeof(Button), "Header"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(Button), "Footer"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:2"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:1"),
                Tuple.Create(typeof(ListViewHeaderItem), "G:0"),
                Tuple.Create(typeof(Button), "Header"),
            };

            VerifyKeyNavOrder(true, inputKeys, expectedFocusOrder);
        }

        #endregion

        #region Moco:GridView Panel:ItemsWrapGrid VirtualizationDirection:Vertical

        void LoadXaml_GridView_ItemsWrapGrid_VdirVertical()
        {
            LoadXaml("ListView_Template", "ItemsWrapGrid", "Horizontal");
        }

        [TestMethod]
        public void GV_IWG_VVdir_Basics()
        {
            LoadXaml_GridView_ItemsWrapGrid_VdirVertical();
            SetupMocoItemSource(DataModel.OC_OC_IsA, DefaultNumGroups, DefaultNumItemsPerGroup, 1);

            PerformScenarios(DataModel.OC_OC_IsA,
                  () =>
                  {
                      // Validate Layout - OnLoaded
                      ValidateVisibleIndeces(0, 17);
                  },
                () =>
                {
                    // Validate GroupScrolledIntoView
                    ValidateVisibleIndeces(33, 49);
                },
                () =>
                {
                    // Validate ItemScrolledIntoView
                    ValidateVisibleIndeces(0, 17);
                });

            ValidateReset("G:0",
               () =>
               {
                   // Validate Reset
                   ValidateVisibleIndeces(0, 14);
               });
        }

        #endregion
    }
}
