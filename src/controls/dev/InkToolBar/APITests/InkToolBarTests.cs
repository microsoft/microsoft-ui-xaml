// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class InkToolBarTests : ApiTestBase
    {
        [TestMethod]
        public void InkToolBarDefaultConstructorTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                Verify.IsNotNull(toolbar, "InkToolBar should be constructible.");
            });
        }

        [TestMethod]
        public void InkToolBarInitialControlsDefaultTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                Verify.AreEqual(InkToolBarInitialControls.All, toolbar.InitialControls,
                    "Default InitialControls should be All.");
            });
        }

        [TestMethod]
        public void InkToolBarInitialControlsPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                toolbar.InitialControls = InkToolBarInitialControls.None;
                Verify.AreEqual(InkToolBarInitialControls.None, toolbar.InitialControls,
                    "InitialControls should be None.");

                toolbar.InitialControls = InkToolBarInitialControls.PensOnly;
                Verify.AreEqual(InkToolBarInitialControls.PensOnly, toolbar.InitialControls,
                    "InitialControls should be PensOnly.");

                toolbar.InitialControls = InkToolBarInitialControls.AllExceptPens;
                Verify.AreEqual(InkToolBarInitialControls.AllExceptPens, toolbar.InitialControls,
                    "InitialControls should be AllExceptPens.");

                toolbar.InitialControls = InkToolBarInitialControls.All;
                Verify.AreEqual(InkToolBarInitialControls.All, toolbar.InitialControls,
                    "InitialControls should be All.");
            });
        }

        [TestMethod]
        public void InkToolBarChildrenPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                var children = toolbar.Children;
                Verify.IsNotNull(children, "Children collection should not be null.");
            });
        }

        [TestMethod]
        public void InkToolBarOrientationPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                // Default
                Verify.AreEqual(Orientation.Horizontal, toolbar.Orientation,
                    "Default orientation should be Horizontal.");

                // Set to Vertical
                toolbar.Orientation = Orientation.Vertical;
                Verify.AreEqual(Orientation.Vertical, toolbar.Orientation,
                    "Orientation should be Vertical.");

                // Set back to Horizontal
                toolbar.Orientation = Orientation.Horizontal;
                Verify.AreEqual(Orientation.Horizontal, toolbar.Orientation,
                    "Orientation should be Horizontal again.");
            });
        }

        [TestMethod]
        public void InkToolBarButtonFlyoutPlacementPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                // Default
                Verify.AreEqual(InkToolBarButtonFlyoutPlacement.Auto, toolbar.ButtonFlyoutPlacement,
                    "Default flyout placement should be Auto.");

                toolbar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Top;
                Verify.AreEqual(InkToolBarButtonFlyoutPlacement.Top, toolbar.ButtonFlyoutPlacement,
                    "Should be Top.");

                toolbar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Bottom;
                Verify.AreEqual(InkToolBarButtonFlyoutPlacement.Bottom, toolbar.ButtonFlyoutPlacement,
                    "Should be Bottom.");

                toolbar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Left;
                Verify.AreEqual(InkToolBarButtonFlyoutPlacement.Left, toolbar.ButtonFlyoutPlacement,
                    "Should be Left.");

                toolbar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Right;
                Verify.AreEqual(InkToolBarButtonFlyoutPlacement.Right, toolbar.ButtonFlyoutPlacement,
                    "Should be Right.");
            });
        }

        [TestMethod]
        public void InkToolBarIsRulerButtonCheckedPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                Verify.IsFalse(toolbar.IsRulerButtonChecked,
                    "IsRulerButtonChecked should be false by default.");

                toolbar.IsRulerButtonChecked = true;
                Verify.IsTrue(toolbar.IsRulerButtonChecked,
                    "IsRulerButtonChecked should be true.");

                toolbar.IsRulerButtonChecked = false;
                Verify.IsFalse(toolbar.IsRulerButtonChecked,
                    "IsRulerButtonChecked should be false.");
            });
        }

        [TestMethod]
        public void InkToolBarIsStencilButtonCheckedPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                Verify.IsFalse(toolbar.IsStencilButtonChecked,
                    "IsStencilButtonChecked should be false by default.");

                toolbar.IsStencilButtonChecked = true;
                Verify.IsTrue(toolbar.IsStencilButtonChecked,
                    "IsStencilButtonChecked should be true.");

                toolbar.IsStencilButtonChecked = false;
                Verify.IsFalse(toolbar.IsStencilButtonChecked,
                    "IsStencilButtonChecked should be false.");
            });
        }

        [TestMethod]
        public void InkToolBarTargetInkCanvasPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                var inkCanvas = new InkCanvas();

                // Default should be null
                Verify.IsNull(toolbar.TargetInkCanvas, "TargetInkCanvas should be null by default.");

                // Set target
                toolbar.TargetInkCanvas = inkCanvas;
                Verify.IsNotNull(toolbar.TargetInkCanvas, "TargetInkCanvas should not be null after setting.");
                Verify.AreEqual(inkCanvas, toolbar.TargetInkCanvas, "TargetInkCanvas should match.");

                // Clear target
                toolbar.TargetInkCanvas = null;
                Verify.IsNull(toolbar.TargetInkCanvas, "TargetInkCanvas should be null after clearing.");
            });
        }

        [TestMethod]
        public void InkToolBarActiveToolPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                // ActiveTool may be null before template is applied
                var activeTool = toolbar.ActiveTool;
                Log.Comment($"ActiveTool before template: {(activeTool == null ? "null" : "not null")}");
            });
        }

        [TestMethod]
        public void InkToolBarGetToolButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                // GetToolButton before template apply - may return null
                var ballpointPen = toolbar.GetToolButton(InkToolBarTool.BallpointPen);
                var pencil = toolbar.GetToolButton(InkToolBarTool.Pencil);
                var highlighter = toolbar.GetToolButton(InkToolBarTool.Highlighter);
                var eraser = toolbar.GetToolButton(InkToolBarTool.Eraser);

                // These may be null before the toolbar is loaded in the visual tree.
                Log.Comment($"BallpointPen: {(ballpointPen == null ? "null" : "found")}");
                Log.Comment($"Pencil: {(pencil == null ? "null" : "found")}");
                Log.Comment($"Highlighter: {(highlighter == null ? "null" : "found")}");
                Log.Comment($"Eraser: {(eraser == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolBarGetToggleButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                var ruler = toolbar.GetToggleButton(InkToolBarToggle.Ruler);
                var custom = toolbar.GetToggleButton(InkToolBarToggle.Custom);

                Log.Comment($"Ruler toggle: {(ruler == null ? "null" : "found")}");
                Log.Comment($"Custom toggle: {(custom == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolBarGetMenuButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                var stencil = toolbar.GetMenuButton(InkToolBarMenuKind.Stencil);

                Log.Comment($"Stencil menu: {(stencil == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolBarInkDrawingAttributesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                // InkDrawingAttributes may be null before connection to InkPresenter
                var attrs = toolbar.InkDrawingAttributes;
                Log.Comment($"InkDrawingAttributes: {(attrs == null ? "null" : "available")}");
            });
        }

        [TestMethod]
        public void InkToolBarDependencyPropertiesExistTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Verify all dependency properties are accessible
                Verify.IsNotNull(InkToolBar.InitialControlsProperty, "InitialControlsProperty should exist.");
                Verify.IsNotNull(InkToolBar.ChildrenProperty, "ChildrenProperty should exist.");
                Verify.IsNotNull(InkToolBar.ActiveToolProperty, "ActiveToolProperty should exist.");
                Verify.IsNotNull(InkToolBar.InkDrawingAttributesProperty, "InkDrawingAttributesProperty should exist.");
                Verify.IsNotNull(InkToolBar.IsRulerButtonCheckedProperty, "IsRulerButtonCheckedProperty should exist.");
                Verify.IsNotNull(InkToolBar.TargetInkCanvasProperty, "TargetInkCanvasProperty should exist.");
                Verify.IsNotNull(InkToolBar.IsStencilButtonCheckedProperty, "IsStencilButtonCheckedProperty should exist.");
                Verify.IsNotNull(InkToolBar.ButtonFlyoutPlacementProperty, "ButtonFlyoutPlacementProperty should exist.");
                Verify.IsNotNull(InkToolBar.OrientationProperty, "OrientationProperty should exist.");
                Verify.IsNotNull(InkToolBar.TargetInkPresenterProperty, "TargetInkPresenterProperty should exist.");
            });
        }

        [TestMethod]
        public void InkToolBarTargetInkPresenterPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();

                Verify.IsNull(toolbar.TargetInkPresenter, "TargetInkPresenter should be null by default.");

                // Set an arbitrary object (in real use this would be an InkPresenter)
                var obj = new object();
                toolbar.TargetInkPresenter = obj;
                Verify.IsNotNull(toolbar.TargetInkPresenter, "TargetInkPresenter should not be null after setting.");
            });
        }

        [TestMethod]
        public void InkToolBarActiveToolChangedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                bool eventFired = false;

                toolbar.ActiveToolChanged += (sender, args) =>
                {
                    eventFired = true;
                };

                Log.Comment("ActiveToolChanged event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolBarInkDrawingAttributesChangedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                bool eventFired = false;

                toolbar.InkDrawingAttributesChanged += (sender, args) =>
                {
                    eventFired = true;
                };

                Log.Comment("InkDrawingAttributesChanged event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolBarEraseAllClickedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolBar();
                bool eventFired = false;

                toolbar.EraseAllClicked += (sender, args) =>
                {
                    eventFired = true;
                };

                Log.Comment("EraseAllClicked event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolBarInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:InkToolBar x:Name='TestInkToolBar' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var toolbar = (InkToolBar)root.FindName("TestInkToolBar");
                Verify.IsNotNull(toolbar, "InkToolBar should be found in visual tree.");
            });
        }

        [TestMethod]
        public void InkToolBarWithTargetInkCanvasInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='Auto' />
                            <RowDefinition Height='*' />
                        </Grid.RowDefinitions>
                        <controls:InkToolBar x:Name='TestToolBar'
                            TargetInkCanvas='{x:Bind TestCanvas}'
                            Grid.Row='0' />
                        <controls:InkCanvas x:Name='TestCanvas'
                            Grid.Row='1' Width='400' Height='300' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var toolbar = (InkToolBar)root.FindName("TestToolBar");
                var canvas = (InkCanvas)root.FindName("TestCanvas");

                Verify.IsNotNull(toolbar, "InkToolBar should be found.");
                Verify.IsNotNull(canvas, "InkCanvas should be found.");
            });
        }

        [TestMethod]
        public void InkToolBarMultipleInstancesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar1 = new InkToolBar();
                var toolbar2 = new InkToolBar();

                toolbar1.InitialControls = InkToolBarInitialControls.PensOnly;
                toolbar2.InitialControls = InkToolBarInitialControls.None;

                toolbar1.Orientation = Orientation.Vertical;
                toolbar2.Orientation = Orientation.Horizontal;

                Verify.AreEqual(InkToolBarInitialControls.PensOnly, toolbar1.InitialControls,
                    "Toolbar1 should be PensOnly.");
                Verify.AreEqual(InkToolBarInitialControls.None, toolbar2.InitialControls,
                    "Toolbar2 should be None.");
                Verify.AreEqual(Orientation.Vertical, toolbar1.Orientation,
                    "Toolbar1 should be Vertical.");
                Verify.AreEqual(Orientation.Horizontal, toolbar2.Orientation,
                    "Toolbar2 should be Horizontal.");
            });
        }

        [TestMethod]
        public void InkToolBarBallpointPenButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarBallpointPenButton();
                Verify.IsNotNull(button, "BallpointPenButton should be constructible.");
                Verify.AreEqual(InkToolBarTool.BallpointPen, button.ToolKind,
                    "ToolKind should be BallpointPen.");
            });
        }

        [TestMethod]
        public void InkToolBarPencilButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarPencilButton();
                Verify.IsNotNull(button, "PencilButton should be constructible.");
                Verify.AreEqual(InkToolBarTool.Pencil, button.ToolKind,
                    "ToolKind should be Pencil.");
            });
        }

        [TestMethod]
        public void InkToolBarHighlighterButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarHighlighterButton();
                Verify.IsNotNull(button, "HighlighterButton should be constructible.");
                Verify.AreEqual(InkToolBarTool.Highlighter, button.ToolKind,
                    "ToolKind should be Highlighter.");
            });
        }

        [TestMethod]
        public void InkToolBarCustomToolButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarCustomToolButton();
                Verify.IsNotNull(button, "CustomToolButton should be constructible.");
                Verify.AreEqual(InkToolBarTool.CustomTool, button.ToolKind,
                    "ToolKind should be CustomTool.");

                // ConfigurationContent
                Verify.IsNull(button.ConfigurationContent, "ConfigurationContent should be null by default.");
                var content = new TextBlock { Text = "Custom Config" };
                button.ConfigurationContent = content;
                Verify.IsNotNull(button.ConfigurationContent, "ConfigurationContent should not be null.");
            });
        }

        [TestMethod]
        public void InkToolBarCustomToggleButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarCustomToggleButton();
                Verify.IsNotNull(button, "CustomToggleButton should be constructible.");
            });
        }

        [TestMethod]
        public void InkToolBarStencilButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolBarStencilButton();
                Verify.IsNotNull(button, "StencilButton should be constructible.");

                // SelectedStencil
                button.SelectedStencil = InkToolBarStencilKind.Ruler;
                Verify.AreEqual(InkToolBarStencilKind.Ruler, button.SelectedStencil,
                    "SelectedStencil should be Ruler.");

                button.SelectedStencil = InkToolBarStencilKind.Protractor;
                Verify.AreEqual(InkToolBarStencilKind.Protractor, button.SelectedStencil,
                    "SelectedStencil should be Protractor.");

                // Visibility properties
                button.IsRulerItemVisible = true;
                Verify.IsTrue(button.IsRulerItemVisible, "IsRulerItemVisible should be true.");

                button.IsProtractorItemVisible = true;
                Verify.IsTrue(button.IsProtractorItemVisible, "IsProtractorItemVisible should be true.");
            });
        }

        [TestMethod]
        public void InkToolBarFlyoutItemTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var item = new InkToolBarFlyoutItem();
                Verify.IsNotNull(item, "FlyoutItem should be constructible.");

                // Kind
                item.Kind = InkToolBarFlyoutItemKind.Simple;
                Verify.AreEqual(InkToolBarFlyoutItemKind.Simple, item.Kind, "Kind should be Simple.");

                item.Kind = InkToolBarFlyoutItemKind.Radio;
                Verify.AreEqual(InkToolBarFlyoutItemKind.Radio, item.Kind, "Kind should be Radio.");

                item.Kind = InkToolBarFlyoutItemKind.Check;
                Verify.AreEqual(InkToolBarFlyoutItemKind.Check, item.Kind, "Kind should be Check.");

                // IsChecked
                item.IsChecked = true;
                Verify.IsTrue(item.IsChecked, "IsChecked should be true.");

                item.IsChecked = false;
                Verify.IsFalse(item.IsChecked, "IsChecked should be false.");
            });
        }

        [TestMethod]
        public void InkToolBarPenConfigurationControlTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var config = new InkToolBarPenConfigurationControl();
                Verify.IsNotNull(config, "PenConfigurationControl should be constructible.");
            });
        }
    }
}
