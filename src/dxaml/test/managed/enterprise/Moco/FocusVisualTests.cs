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
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls.ListViewBase
{
    [TestClass]
    public class FocusVisualTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
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

        #region RS1 default behavior tests

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDefaultBehaviorWithNoSelection()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "None", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDefaultBehaviorWithSingleSelection()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Single", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDefaultBehaviorWithMultipleSelection1()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDefaultBehaviorWithMultipleSelection2()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVIEDefaultBehaviorWithMultipleSelection1()
        {
            RunTest(
                true, /* isGridView */
                false, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVIEDefaultBehaviorWithMultipleSelection2()
        {
            RunTest(
                true, /* isGridView */
                false, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDefaultBehaviorWithNoSelection()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "None", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDefaultBehaviorWithSingleSelection()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Single", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDefaultBehaviorWithMultipleSelection1()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDefaultBehaviorWithMultipleSelection2()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVIEDefaultBehaviorWithMultipleSelection1()
        {
            RunTest(
                false, /* isGridView */
                false, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVIEDefaultBehaviorWithMultipleSelection2()
        {
            RunTest(
                false, /* isGridView */
                false, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        #endregion

        #region RS1 DottedLine behavior tests

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDottedLineBehaviorWithMultipleSelection1()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                true /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICDottedLineBehaviorWithMultipleSelection2()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                true /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDottedLineBehaviorWithMultipleSelection1()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                true /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICDottedLineBehaviorWithMultipleSelection2()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                false, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                true /* useDottedLine (RS1 default behavior: false) */
                );
        }

        #endregion

        #region RS1 No SystemFocusVisuals tests

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICNoSystemFocusVisualsBehaviorWithMultipleSelection1()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                true, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void GVICNoSystemFocusVisualsBehaviorWithMultipleSelection2()
        {
            RunTest(
                true, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                true, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICNoSystemFocusVisualsBehaviorWithMultipleSelection1()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                false, /* shouldSelect */
                true, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("HasAssociatedMasterFile", "True")]
        public void LVICNoSystemFocusVisualsBehaviorWithMultipleSelection2()
        {
            RunTest(
                false, /* isGridView */
                true, /* isChrome */
                "Multiple", /* selectionMode */
                true, /* shouldSelect */
                true, /* doNotUseSystemFocusVisuals (RS1 default behavior: false) */
                false /* useDottedLine (RS1 default behavior: false) */
                );
        }

        #endregion

        #region Helpers

        private void RunTest(bool isGridView, bool isChrome, string selectionMode, bool shouldSelect, bool doNotUseSystemFocusVisuals, bool useDottedLine)
        {
            using (TestServices.Utilities.CreateRenderingScopeGuard(
                DCompRendering.WUCCompleteSynchronousCompTree,
                resizeWindow: false,
                injectMockDComp: true,
                resetDevice: true))
             {
                TestServices.WindowHelper.SetWindowSizeOverrideWithScale(new Size(400, 400), 1.0f);

                Grid root = null;
                Microsoft.UI.Xaml.Controls.ListViewBase lvBase = null;

                UIExecutor.Execute(() =>
                {
                    root = SetupTestListViewBase(isGridView, isChrome, selectionMode, doNotUseSystemFocusVisuals);
                    if (useDottedLine)
                    {
                        Application.Current.FocusVisualKind = FocusVisualKind.DottedLine;
                    }
                    else
                    {
                        Application.Current.FocusVisualKind = FocusVisualKind.HighVisibility;
                    }

                    lvBase = (Microsoft.UI.Xaml.Controls.ListViewBase)root.FindName("lvBase");
                    for (int i = 0; i < 9; ++i)
                    {
                        lvBase.Items.Add("Item " + i.ToString());
                    }

                    TestServices.WindowHelper.WindowContent = root;
                });
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var item0 = lvBase.ContainerFromIndex(0);
                    (item0 as Control).Focus(FocusState.Programmatic);
                });
                TestServices.WindowHelper.WaitForIdle();

                if (lvBase is GridView)
                {
                    TestServices.KeyboardHelper.Right();
                    TestServices.WindowHelper.WaitForIdle();
                }

                TestServices.KeyboardHelper.Down();
                TestServices.WindowHelper.WaitForIdle();

                if (shouldSelect && selectionMode.Equals("Multiple", StringComparison.OrdinalIgnoreCase))
                {
                    TestServices.KeyboardHelper.Enter();
                    TestServices.WindowHelper.WaitForIdle();
                }

                TestServices.Utilities.VerifyMockDCompOutput(MockDComp.SurfaceComparison.NoComparison);
            }
        }

        private Grid SetupTestListViewBase(bool isGridView, bool isChrome, string selectionMode, bool doNotUseSystemFocusVisuals)
        {
            string xamlFormat = @"
<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>
    <{0} x:Name='lvBase' {1} SelectionMode='{3}' {4}>
        {5}
        <{0}.ItemTemplate>
            <DataTemplate>
                <Grid {6}>
                    <TextBlock {2} Text='{{Binding}}' {7}/>
                </Grid>
            </DataTemplate>
        </{0}.ItemTemplate>
    </{0}>
</Grid>
";
            string expandedTemplateStyle = isGridView ? "ItemContainerStyle='{ThemeResource GridViewItemExpanded}'" : "ItemContainerStyle='{ThemeResource ListViewItemExpanded}'";
            string itemContainerStyleFormat = @"
<{0}.ItemContainerStyle>
    <Style TargetType='{0}Item'>
        <Setter Property='UseSystemFocusVisuals' Value='False'/>
    </Style>
</{0}.ItemContainerStyle>
";
            string itemContainerStyle = string.Format(
                itemContainerStyleFormat,
                isGridView ? "GridView" : "ListView");

            string xaml = string.Format(
                xamlFormat,
                isGridView ? "GridView" : "ListView",
                isGridView ? "Width='400' Height='400'" : "Width='100' Height='400'",
                isGridView ? "Width='100' Height='100'" : "Width='100' Height='40'",
                selectionMode,
                isChrome ? string.Empty : expandedTemplateStyle,
                doNotUseSystemFocusVisuals ? itemContainerStyle : string.Empty,
                isGridView ? "Background='White'" : string.Empty,
                isGridView ? "Foreground='Black'" : string.Empty);

            Log.Comment("Test ListView XAML:");
            Log.Comment(xaml);

            return (Grid)XamlReader.Load(xaml);
        }

        #endregion
    }
}

