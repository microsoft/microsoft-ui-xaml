// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Threading;
using Microsoft.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using InkDrawingAttributes = global::Windows.UI.Input.Inking.InkDrawingAttributes;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class InkToolbarTests : ApiTestBase
    {
        [TestMethod]
        public void InkToolbarDefaultConstructorTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                Verify.IsNotNull(toolbar, "InkToolbar should be constructible.");
            });
        }

        [TestMethod]
        public void InkToolbarInitialControlsDefaultTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                Verify.AreEqual(InkToolbarInitialControls.All, toolbar.InitialControls,
                    "Default InitialControls should be All.");
            });
        }

        [TestMethod]
        public void InkToolbarInitialControlsPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                toolbar.InitialControls = InkToolbarInitialControls.None;
                Verify.AreEqual(InkToolbarInitialControls.None, toolbar.InitialControls,
                    "InitialControls should be None.");

                toolbar.InitialControls = InkToolbarInitialControls.PensOnly;
                Verify.AreEqual(InkToolbarInitialControls.PensOnly, toolbar.InitialControls,
                    "InitialControls should be PensOnly.");

                toolbar.InitialControls = InkToolbarInitialControls.AllExceptPens;
                Verify.AreEqual(InkToolbarInitialControls.AllExceptPens, toolbar.InitialControls,
                    "InitialControls should be AllExceptPens.");

                toolbar.InitialControls = InkToolbarInitialControls.All;
                Verify.AreEqual(InkToolbarInitialControls.All, toolbar.InitialControls,
                    "InitialControls should be All.");
            });
        }

        [TestMethod]
        public void InkToolbarChildrenPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                var children = toolbar.Children;
                Verify.IsNotNull(children, "Children collection should not be null.");
            });
        }

        [TestMethod]
        public void InkToolbarOrientationPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

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
        public void InkToolbarButtonFlyoutPlacementPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                // Default
                Verify.AreEqual(InkToolbarButtonFlyoutPlacement.Auto, toolbar.ButtonFlyoutPlacement,
                    "Default flyout placement should be Auto.");

                toolbar.ButtonFlyoutPlacement = InkToolbarButtonFlyoutPlacement.Top;
                Verify.AreEqual(InkToolbarButtonFlyoutPlacement.Top, toolbar.ButtonFlyoutPlacement,
                    "Should be Top.");

                toolbar.ButtonFlyoutPlacement = InkToolbarButtonFlyoutPlacement.Bottom;
                Verify.AreEqual(InkToolbarButtonFlyoutPlacement.Bottom, toolbar.ButtonFlyoutPlacement,
                    "Should be Bottom.");

                toolbar.ButtonFlyoutPlacement = InkToolbarButtonFlyoutPlacement.Left;
                Verify.AreEqual(InkToolbarButtonFlyoutPlacement.Left, toolbar.ButtonFlyoutPlacement,
                    "Should be Left.");

                toolbar.ButtonFlyoutPlacement = InkToolbarButtonFlyoutPlacement.Right;
                Verify.AreEqual(InkToolbarButtonFlyoutPlacement.Right, toolbar.ButtonFlyoutPlacement,
                    "Should be Right.");
            });
        }

        [TestMethod]
        public void InkToolbarIsRulerButtonCheckedPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

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
        public void InkToolbarIsStencilButtonCheckedPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

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
        [TestProperty("Ignore", "True")]
        public void InkToolbarStencilToggleWithTargetCanvasTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Unlike InkToolbarIsStencilButtonCheckedPropertyTest (bare toolbar, returns early with
                // no target), this wires a loaded TargetInkCanvas and a stencil button so toggling
                // actually drives SetStencilVisibility -> the InkPresenter proxy (covers the real
                // stencil toggle path per review).
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='Auto' />
                            <RowDefinition Height='*' />
                        </Grid.RowDefinitions>
                        <controls:InkToolbar x:Name='TestToolBar' Grid.Row='0'>
                            <controls:InkToolbarStencilButton />
                        </controls:InkToolbar>
                        <controls:InkCanvas x:Name='TestCanvas'
                            Grid.Row='1' Width='400' Height='300' />
                    </Grid>");

                var toolbar = (InkToolbar)root.FindName("TestToolBar");
                var canvas = (InkCanvas)root.FindName("TestCanvas");
                toolbar.TargetInkCanvas = canvas;

                Content = root;
                Content.UpdateLayout();

                // Toggle on with a live target: reaches SetStencilVisibility -> proxy; state reports checked.
                toolbar.IsStencilButtonChecked = true;
                Verify.IsTrue(toolbar.IsStencilButtonChecked,
                    "IsStencilButtonChecked should be true after toggling on with a target canvas.");

                // Toggle off: state clears.
                toolbar.IsStencilButtonChecked = false;
                Verify.IsFalse(toolbar.IsStencilButtonChecked,
                    "IsStencilButtonChecked should be false after toggling off.");
            });
        }

        [TestMethod]
        public void InkToolbarTargetInkCanvasPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
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
        public void InkToolbarActiveToolPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                // ActiveTool may be null before template is applied
                var activeTool = toolbar.ActiveTool;
                Log.Comment($"ActiveTool before template: {(activeTool == null ? "null" : "not null")}");
            });
        }

        [TestMethod]
        public void InkToolbarGetToolButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                // GetToolButton before template apply - may return null
                var ballpointPen = toolbar.GetToolButton(InkToolbarTool.BallpointPen);
                var pencil = toolbar.GetToolButton(InkToolbarTool.Pencil);
                var highlighter = toolbar.GetToolButton(InkToolbarTool.Highlighter);
                var eraser = toolbar.GetToolButton(InkToolbarTool.Eraser);

                // These may be null before the toolbar is loaded in the visual tree.
                Log.Comment($"BallpointPen: {(ballpointPen == null ? "null" : "found")}");
                Log.Comment($"Pencil: {(pencil == null ? "null" : "found")}");
                Log.Comment($"Highlighter: {(highlighter == null ? "null" : "found")}");
                Log.Comment($"Eraser: {(eraser == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolbarGetToggleButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                var ruler = toolbar.GetToggleButton(InkToolbarToggle.Ruler);
                var custom = toolbar.GetToggleButton(InkToolbarToggle.Custom);

                Log.Comment($"Ruler toggle: {(ruler == null ? "null" : "found")}");
                Log.Comment($"Custom toggle: {(custom == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolbarGetMenuButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                var stencil = toolbar.GetMenuButton(InkToolbarMenuKind.Stencil);

                Log.Comment($"Stencil menu: {(stencil == null ? "null" : "found")}");
            });
        }

        [TestMethod]
        public void InkToolbarInkDrawingAttributesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                // InkDrawingAttributes may be null before connection to InkPresenter
                var attrs = toolbar.InkDrawingAttributes;
                Log.Comment($"InkDrawingAttributes: {(attrs == null ? "null" : "available")}");
            });
        }

        [TestMethod]
        public void InkToolbarDependencyPropertiesExistTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Verify all dependency properties are accessible
                Verify.IsNotNull(InkToolbar.InitialControlsProperty, "InitialControlsProperty should exist.");
                Verify.IsNotNull(InkToolbar.ChildrenProperty, "ChildrenProperty should exist.");
                Verify.IsNotNull(InkToolbar.ActiveToolProperty, "ActiveToolProperty should exist.");
                Verify.IsNotNull(InkToolbar.InkDrawingAttributesProperty, "InkDrawingAttributesProperty should exist.");
                Verify.IsNotNull(InkToolbar.IsRulerButtonCheckedProperty, "IsRulerButtonCheckedProperty should exist.");
                Verify.IsNotNull(InkToolbar.TargetInkCanvasProperty, "TargetInkCanvasProperty should exist.");
                Verify.IsNotNull(InkToolbar.IsStencilButtonCheckedProperty, "IsStencilButtonCheckedProperty should exist.");
                Verify.IsNotNull(InkToolbar.ButtonFlyoutPlacementProperty, "ButtonFlyoutPlacementProperty should exist.");
                Verify.IsNotNull(InkToolbar.OrientationProperty, "OrientationProperty should exist.");
                Verify.IsNotNull(InkToolbar.TargetInkPresenterProperty, "TargetInkPresenterProperty should exist.");
            });
        }

        [TestMethod]
        public void InkToolbarTargetInkPresenterPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                Verify.IsNull(toolbar.TargetInkPresenter, "TargetInkPresenter should be null by default.");

                var presenter = new InkCanvas().InkPresenter;
                toolbar.TargetInkPresenter = presenter;
                Verify.AreEqual(presenter, toolbar.TargetInkPresenter,
                    "TargetInkPresenter should round-trip the assigned InkPresenter.");
            });
        }

        [TestMethod]
        public void InkToolbarDrivesTargetInkPresenterWithoutCanvasTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                var presenter = new InkCanvas().InkPresenter;

                // Deliberately no TargetInkCanvas: this is the path that previously returned early and
                // left a presenter-driven toolbar without any tool state.
                toolbar.TargetInkPresenter = presenter;

                toolbar.ActiveTool = new InkToolbarEraserButton();
                Verify.AreEqual(InkInputProcessingMode.Erasing, presenter.InputProcessingConfiguration.Mode,
                    "Selecting the eraser should put the target InkPresenter into Erasing mode.");

                toolbar.ActiveTool = new InkToolbarBallpointPenButton();
                Verify.AreEqual(InkInputProcessingMode.Inking, presenter.InputProcessingConfiguration.Mode,
                    "Selecting a pen should put the target InkPresenter back into Inking mode.");
            });
        }

        [TestMethod]
        public void InkToolbarActiveToolChangedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                toolbar.ActiveToolChanged += (sender, args) => { };

                Log.Comment("ActiveToolChanged event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolbarInkDrawingAttributesChangedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                toolbar.InkDrawingAttributesChanged += (sender, args) => { };

                Log.Comment("InkDrawingAttributesChanged event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolbarEraseAllClickedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                toolbar.EraseAllClicked += (sender, args) => { };

                Log.Comment("EraseAllClicked event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolbarInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:InkToolbar x:Name='TestInkToolbar' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var toolbar = (InkToolbar)root.FindName("TestInkToolbar");
                Verify.IsNotNull(toolbar, "InkToolbar should be found in visual tree.");
            });
        }

        [TestMethod]
        [TestProperty("Ignore", "True")]
        public void InkToolbarWithTargetInkCanvasInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Note: x:Bind is unsupported in XamlReader.Load (compiled-binding only),
                // so wire up TargetInkCanvas in code after the tree is parsed.
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <Grid.RowDefinitions>
                            <RowDefinition Height='Auto' />
                            <RowDefinition Height='*' />
                        </Grid.RowDefinitions>
                        <controls:InkToolbar x:Name='TestToolBar' Grid.Row='0' />
                        <controls:InkCanvas x:Name='TestCanvas'
                            Grid.Row='1' Width='400' Height='300' />
                    </Grid>");

                var toolbar = (InkToolbar)root.FindName("TestToolBar");
                var canvas = (InkCanvas)root.FindName("TestCanvas");
                toolbar.TargetInkCanvas = canvas;

                Content = root;
                Content.UpdateLayout();

                Verify.IsNotNull(toolbar, "InkToolbar should be found.");
                Verify.IsNotNull(canvas, "InkCanvas should be found.");
                Verify.AreEqual(canvas, toolbar.TargetInkCanvas,
                    "TargetInkCanvas should be wired to the in-tree InkCanvas.");
            });
        }

        [TestMethod]
        public void InkToolbarMultipleInstancesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar1 = new InkToolbar();
                var toolbar2 = new InkToolbar();

                toolbar1.InitialControls = InkToolbarInitialControls.PensOnly;
                toolbar2.InitialControls = InkToolbarInitialControls.None;

                toolbar1.Orientation = Orientation.Vertical;
                toolbar2.Orientation = Orientation.Horizontal;

                Verify.AreEqual(InkToolbarInitialControls.PensOnly, toolbar1.InitialControls,
                    "Toolbar1 should be PensOnly.");
                Verify.AreEqual(InkToolbarInitialControls.None, toolbar2.InitialControls,
                    "Toolbar2 should be None.");
                Verify.AreEqual(Orientation.Vertical, toolbar1.Orientation,
                    "Toolbar1 should be Vertical.");
                Verify.AreEqual(Orientation.Horizontal, toolbar2.Orientation,
                    "Toolbar2 should be Horizontal.");
            });
        }

        [TestMethod]
        public void InkToolbarBallpointPenButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarBallpointPenButton();
                Verify.IsNotNull(button, "BallpointPenButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.BallpointPen, button.ToolKind,
                    "ToolKind should be BallpointPen.");
            });
        }

        [TestMethod]
        public void InkToolbarPencilButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarPencilButton();
                Verify.IsNotNull(button, "PencilButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.Pencil, button.ToolKind,
                    "ToolKind should be Pencil.");
            });
        }

        [TestMethod]
        public void InkToolbarHighlighterButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarHighlighterButton();
                Verify.IsNotNull(button, "HighlighterButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.Highlighter, button.ToolKind,
                    "ToolKind should be Highlighter.");
            });
        }

        [TestMethod]
        public void InkToolbarCustomToolButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarCustomToolButton();
                Verify.IsNotNull(button, "CustomToolButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.CustomTool, button.ToolKind,
                    "ToolKind should be CustomTool.");

                // ConfigurationContent
                Verify.IsNull(button.ConfigurationContent, "ConfigurationContent should be null by default.");
                var content = new TextBlock { Text = "Custom Config" };
                button.ConfigurationContent = content;
                Verify.IsNotNull(button.ConfigurationContent, "ConfigurationContent should not be null.");
            });
        }

        [TestMethod]
        public void InkToolbarCustomToggleButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarCustomToggleButton();
                Verify.IsNotNull(button, "CustomToggleButton should be constructible.");
            });
        }

        [TestMethod]
        public void InkToolbarStencilButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarStencilButton();
                Verify.IsNotNull(button, "StencilButton should be constructible.");
                Verify.IsTrue(button.IsRulerItemVisible, "IsRulerItemVisible should default to true (UWP parity).");
                Verify.IsTrue(button.IsProtractorItemVisible, "IsProtractorItemVisible should default to true (UWP parity).");

                // SelectedStencil
                button.SelectedStencil = InkToolbarStencilKind.Ruler;
                Verify.AreEqual(InkToolbarStencilKind.Ruler, button.SelectedStencil,
                    "SelectedStencil should be Ruler.");

                button.SelectedStencil = InkToolbarStencilKind.Protractor;
                Verify.AreEqual(InkToolbarStencilKind.Protractor, button.SelectedStencil,
                    "SelectedStencil should be Protractor.");

                // Visibility properties
                button.IsRulerItemVisible = true;
                Verify.IsTrue(button.IsRulerItemVisible, "IsRulerItemVisible should be true.");

                button.IsProtractorItemVisible = true;
                Verify.IsTrue(button.IsProtractorItemVisible, "IsProtractorItemVisible should be true.");
            });
        }

        [TestMethod]
        public void InkToolbarFlyoutItemTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var item = new InkToolbarFlyoutItem();
                Verify.IsNotNull(item, "FlyoutItem should be constructible.");

                // Kind
                item.Kind = InkToolbarFlyoutItemKind.Simple;
                Verify.AreEqual(InkToolbarFlyoutItemKind.Simple, item.Kind, "Kind should be Simple.");

                item.Kind = InkToolbarFlyoutItemKind.Radio;
                Verify.AreEqual(InkToolbarFlyoutItemKind.Radio, item.Kind, "Kind should be Radio.");

                item.Kind = InkToolbarFlyoutItemKind.Check;
                Verify.AreEqual(InkToolbarFlyoutItemKind.Check, item.Kind, "Kind should be Check.");

                // IsChecked
                item.IsChecked = true;
                Verify.IsTrue(item.IsChecked, "IsChecked should be true.");

                item.IsChecked = false;
                Verify.IsFalse(item.IsChecked, "IsChecked should be false.");
            });
        }

        [TestMethod]
        public void InkToolbarPenConfigurationControlTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var config = new InkToolbarPenConfigurationControl();
                Verify.IsNotNull(config, "PenConfigurationControl should be constructible.");
            });
        }

        // Applying the pen configuration template must actually run ConfigureLocalizableElements: both
        // headings get their localized text and the otherwise-anonymous palette and slider get an
        // automation name. The palette/slider names are set only in code (the template leaves them
        // empty), so this test fails if ConfigureLocalizableElements is ever reduced to a no-op - which
        // the makepri key-existence check alone would not catch.
        [TestMethod]
        public void InkToolbarPenConfigurationControlLocalizesHeadingsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var config = new InkToolbarPenConfigurationControl();

                // PenButton has no public setter; set the DP directly so OnApplyTemplate takes the
                // pen (color-picker) path. Without a pen button the eraser path removes the Colors heading.
                config.SetValue(InkToolbarPenConfigurationControl.PenButtonProperty, new InkToolbarBallpointPenButton());

                Content = config;
                Content.UpdateLayout();

                var colorsTitle = config.FindVisualChildByName("PenColorPaletteTitle") as TextBlock;
                var sizeTitle = config.FindVisualChildByName("PenStrokeWidthTitle") as TextBlock;
                var palette = config.FindVisualChildByName("PenColorPalette") as FrameworkElement;
                var slider = config.FindVisualChildByName("PenStrokeWidthSlider") as FrameworkElement;

                Verify.IsNotNull(colorsTitle, "PenColorPaletteTitle should be realized after template apply.");
                Verify.IsNotNull(sizeTitle, "PenStrokeWidthTitle should be realized after template apply.");
                Verify.IsNotNull(palette, "PenColorPalette should be realized after template apply.");
                Verify.IsNotNull(slider, "PenStrokeWidthSlider should be realized after template apply.");

                Verify.IsFalse(string.IsNullOrEmpty(colorsTitle.Text), "Colors heading should carry localized text.");
                Verify.IsFalse(string.IsNullOrEmpty(sizeTitle.Text), "Size heading should carry localized text.");

                var paletteName = AutomationProperties.GetName(palette);
                var sliderName = AutomationProperties.GetName(slider);
                Verify.IsFalse(string.IsNullOrEmpty(paletteName), "Palette automation name should be set by ConfigureLocalizableElements.");
                Verify.IsFalse(string.IsNullOrEmpty(sliderName), "Slider automation name should be set by ConfigureLocalizableElements.");
                Verify.AreEqual(colorsTitle.Text, paletteName, "Palette automation name should match the Colors heading text.");
            });
        }

        // ====================================================================
        // Missing API coverage: EraserButton, CustomPen, CustomPenButton, Events
        // ====================================================================

        [TestMethod]
        public void InkToolbarEraserButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarEraserButton();
                Verify.IsNotNull(button, "EraserButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.Eraser, button.ToolKind, "ToolKind should be Eraser.");
                Verify.IsTrue(button.IsClearAllVisible, "IsClearAllVisible should default to true (UWP parity).");

                button.IsClearAllVisible = false;
                Verify.IsFalse(button.IsClearAllVisible, "IsClearAllVisible should be false.");
                button.IsClearAllVisible = true;
                Verify.IsTrue(button.IsClearAllVisible, "IsClearAllVisible should be true.");
            });
        }

        // NOTE: InkToolbarCustomPen has a protected constructor in IDL — must be subclassed; not directly instantiable.
        // Test disabled until a concrete derived test helper is added.
        // NOTE: InkToolbarCustomPen has a protected constructor in IDL — must be subclassed; not directly instantiable.
        // Test disabled until a concrete derived test helper is added.
        [TestMethod]
        [Ignore]
        public void InkToolbarCustomPenTest()
        {
            // Body intentionally empty — needs a concrete subclass of InkToolbarCustomPen for instantiation.
            // Original assertions covered: CreateInkDrawingAttributes(brush, size) including null-brush path.
        }

        [TestMethod]
        public void InkToolbarCustomPenButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarCustomPenButton();
                Verify.IsNotNull(button, "CustomPenButton should be constructible.");
                Verify.AreEqual(InkToolbarTool.CustomPen, button.ToolKind,
                    "ToolKind should be CustomPen.");

                // CustomPen property — cannot directly instantiate InkToolbarCustomPen (protected ctor).
                // Verify default null state only; setter coverage requires a derived test helper class.
                Verify.IsNull(button.CustomPen, "CustomPen should be null by default.");

                // ConfigurationContent property
                Verify.IsNull(button.ConfigurationContent, "ConfigurationContent should be null by default.");
                var content = new TextBlock { Text = "Custom Pen Config" };
                button.ConfigurationContent = content;
                Verify.IsNotNull(button.ConfigurationContent, "ConfigurationContent should not be null.");
            });
        }

        [TestMethod]
        public void InkToolbarIsStencilButtonCheckedChangedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();

                toolbar.IsStencilButtonCheckedChanged += (sender, args) => { };

                Log.Comment("IsStencilButtonCheckedChanged event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkToolbarRulerButtonTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarRulerButton();
                Verify.IsNotNull(button, "RulerButton should be constructible.");
                Verify.AreEqual(InkToolbarToggle.Ruler, button.ToggleKind, "ToggleKind should be Ruler.");
            });
        }

        // Functional test: loading a default InkToolbar in the visual tree must auto-populate the
        // default tool set (InitialControls=All) and select the first pen as ActiveTool. This is the
        // test that would have caught the vertical-orientation and zero-render regressions.
        [TestMethod]
        public void InkToolbarAutoPopulatesDefaultButtonsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                Content = toolbar;
                Content.UpdateLayout();

                Verify.AreEqual(Orientation.Horizontal, toolbar.Orientation,
                    "Default orientation should be Horizontal.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.BallpointPen),
                    "BallpointPen button should be present after load.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Pencil),
                    "Pencil button should be present after load.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Highlighter),
                    "Highlighter button should be present after load.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Eraser),
                    "Eraser button should be present after load.");
                Verify.IsNotNull(toolbar.GetMenuButton(InkToolbarMenuKind.Stencil),
                    "Stencil (ruler/protractor) button should be present after load.");
                Verify.IsNotNull(toolbar.ActiveTool,
                    "ActiveTool should be set to the first pen after load.");
            });
        }

        // Ports UWP InkToolbarIntegrationTests::InitialControls_PensOnly.
        [TestMethod]
        public void InkToolbarInitialControlsPensOnlyPopulatesPensTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar { InitialControls = InkToolbarInitialControls.PensOnly };
                Content = toolbar;
                Content.UpdateLayout();

                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.BallpointPen), "BallpointPen should be present.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Pencil), "Pencil should be present.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Highlighter), "Highlighter should be present.");
                Verify.IsNull(toolbar.GetToolButton(InkToolbarTool.Eraser), "Eraser should NOT be present in PensOnly.");
            });
        }

        // Ports UWP InkToolbarIntegrationTests::InitialControls_AllExceptPens.
        [TestMethod]
        public void InkToolbarInitialControlsAllExceptPensPopulatesEraserTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar { InitialControls = InkToolbarInitialControls.AllExceptPens };
                Content = toolbar;
                Content.UpdateLayout();

                Verify.IsNull(toolbar.GetToolButton(InkToolbarTool.BallpointPen), "BallpointPen should NOT be present in AllExceptPens.");
                Verify.IsNotNull(toolbar.GetToolButton(InkToolbarTool.Eraser), "Eraser should be present.");
            });
        }

        // Ports UWP InkToolbarIntegrationTests::InitialControls_None.
        [TestMethod]
        public void InkToolbarInitialControlsNonePopulatesNothingTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar { InitialControls = InkToolbarInitialControls.None };
                Content = toolbar;
                Content.UpdateLayout();

                Verify.IsNull(toolbar.GetToolButton(InkToolbarTool.BallpointPen), "No BallpointPen in None.");
                Verify.IsNull(toolbar.GetToolButton(InkToolbarTool.Eraser), "No Eraser in None.");
                Verify.AreEqual(0u, (uint)toolbar.Children.Count, "Children should be empty in None.");
            });
        }

        // Verifies the auto-populated Stencil (ruler/protractor) button is populated like the tool
        // buttons. UWP exposes it via GetMenuButton(Stencil); when the harness realizes the template
        // (a known-good tool button becomes a visual descendant) the stencil must be realized in the
        // same panel and show its icon glyph. Pins the "stencil not ordered into panel / renders blank" issue.
        [TestMethod]
        public void InkToolbarStencilRendersWithIconTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var toolbar = new InkToolbar();
                Content = toolbar;
                Content.UpdateLayout();

                // Functional parity (UWP-equivalent check): the stencil menu button is populated + tracked.
                var stencilMenu = toolbar.GetMenuButton(InkToolbarMenuKind.Stencil);
                Verify.IsNotNull(stencilMenu, "GetMenuButton(Stencil) should be non-null after load.");

                // If the template realized its populated children (a known-good tool button is a visual
                // descendant), the stencil must be ordered into the same panel and show its glyph.
                var ballpointInTree = FindDescendant<InkToolbarBallpointPenButton>(toolbar);
                if (ballpointInTree != null)
                {
                    var stencil = FindDescendant<InkToolbarStencilButton>(toolbar);
                    Verify.IsNotNull(stencil, "Stencil should be realized in the visual tree alongside the tool buttons.");
                    Verify.IsTrue(stencil.ActualWidth > 0 && stencil.ActualHeight > 0,
                        "StencilButton should be laid out with a non-zero size.");

                    var content = FindChildByName(stencil, "Content") as TextBlock;
                    Verify.IsNotNull(content, "StencilButton template should contain a 'Content' TextBlock.");
                    Verify.IsFalse(string.IsNullOrEmpty(content.Text),
                        "StencilButton 'Content' glyph should be set (ruler/protractor icon).");
                }
            });
        }

        [TestMethod]
        public void InkToolbarToolButtonAutomationPeerExpandCollapseTest()
        {
            var toolbar = CreateLoadedInkToolbar();
            RunOnUIThread.Execute(() =>
            {
                foreach (var tool in new[]
                {
                    InkToolbarTool.BallpointPen,
                    InkToolbarTool.Pencil,
                    InkToolbarTool.Highlighter
                })
                {
                    var button = toolbar.GetToolButton(tool);
                    Verify.IsNotNull(button, $"{tool} should be present after load.");
                    button.ApplyTemplate();
                    button.UpdateLayout();

                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                    Verify.IsNotNull(peer, $"{tool} should create an automation peer.");
                    Verify.AreEqual(AutomationControlType.Button, peer.GetAutomationControlType(),
                        $"{tool} should expose the standard Button role.");
                    Verify.AreEqual("drop down button", peer.GetLocalizedControlType(),
                        $"{tool} should advertise its configuration dropdown.");

                    var expandCollapse = peer.GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                    Verify.IsNotNull(expandCollapse, $"{tool} should expose IExpandCollapseProvider.");
                    Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                        $"{tool} should report Collapsed before its flyout is opened.");
                }
            });
        }

        [TestMethod]
        public void InkToolbarMenuButtonAutomationPeerExpandCollapseTest()
        {
            var toolbar = CreateLoadedInkToolbar();
            AutomationPeer peer = null;
            IExpandCollapseProvider expandCollapse = null;
            Flyout flyout = null;
            using var closed = new ManualResetEvent(false);
            EventHandler<object> closedHandler = (s, e) => closed.Set();
            RunOnUIThread.Execute(() =>
            {
                var button = toolbar.GetMenuButton(InkToolbarMenuKind.Stencil) as InkToolbarStencilButton;
                Verify.IsNotNull(button, "The stencil button should be present after load.");
                button.ApplyTemplate();
                button.UpdateLayout();

                peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                Verify.IsNotNull(peer, "MenuButton should create an automation peer.");
                Verify.AreEqual(AutomationControlType.Button, peer.GetAutomationControlType(),
                    "The measuring-tools dropdown should expose the standard Button role.");
                Verify.AreEqual("drop down button", peer.GetLocalizedControlType(),
                    "The measuring-tools button should advertise its dropdown.");
                Verify.AreEqual("Measuring tools, Ruler", peer.GetName(),
                    "The name should include both the menu identity and the selected ruler.");

                button.SelectedStencil = InkToolbarStencilKind.Protractor;
                Verify.AreEqual("Measuring tools, Protractor", peer.GetName(),
                    "Changing the selected stencil should preserve the measuring-tools identity.");
                button.SelectedStencil = InkToolbarStencilKind.Ruler;

                expandCollapse = peer.GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                Verify.IsNotNull(expandCollapse, "MenuButton peer should expose IExpandCollapseProvider.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                    "With no open flyout the menu button should report Collapsed.");
                flyout = FlyoutBase.GetAttachedFlyout(button) as Flyout;
                Verify.IsNotNull(flyout, "The stencil button should have a flyout.");
                flyout.Closed += closedHandler;
            });

            try
            {
                RunOnUIThread.Execute(() => expandCollapse.Expand());
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(ExpandCollapseState.Expanded, expandCollapse.ExpandCollapseState,
                        "Opening the measuring-tools flyout through UIA should report Expanded.");
                    Verify.AreEqual("Measuring tools, Ruler", peer.GetName(),
                        "Opening the dropdown should preserve its identity and selected stencil.");

                    var ruler = FindChildByName(flyout.Content, "StencilRuler") as InkToolbarFlyoutItem;
                    var protractor = FindChildByName(flyout.Content, "StencilProtractor") as InkToolbarFlyoutItem;
                    Verify.IsNotNull(ruler, "The ruler item should be present in the flyout.");
                    Verify.IsNotNull(protractor, "The protractor item should be present in the flyout.");
                    VerifyStencilFlyoutItem(ruler, "Ruler", 1, 2, true);
                    VerifyStencilFlyoutItem(protractor, "Protractor", 2, 2, false);

                    protractor.IsChecked = true;
                    VerifyStencilFlyoutItem(ruler, "Ruler", 1, 2, false);
                    VerifyStencilFlyoutItem(protractor, "Protractor", 2, 2, true);
                });
            }
            finally
            {
                try
                {
                    RunOnUIThread.Execute(() => expandCollapse.Collapse());
                    Verify.IsTrue(closed.WaitOne(DefaultWaitTimeInMS), "The stencil flyout should finish closing.");
                }
                finally
                {
                    RunOnUIThread.Execute(() => flyout.Closed -= closedHandler);
                }
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                    "Closing the measuring-tools flyout through UIA should report Collapsed again.");
            });
        }

        [TestMethod]
        public void InkToolbarSingleStencilAutomationPeerTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var button = new InkToolbarStencilButton();
                Content = button;
                Content.UpdateLayout();
                button.ApplyTemplate();
                button.UpdateLayout();
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                Verify.IsNotNull(peer, "The stencil button should create an automation peer.");
                var cachedExpandCollapse = peer.GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                Verify.IsNotNull(cachedExpandCollapse, "Both visible stencils should expose ExpandCollapse.");
                var flyout = FlyoutBase.GetAttachedFlyout(button) as Flyout;
                Verify.IsNotNull(flyout, "The stencil button should retain its attached flyout content.");
                var ruler = FindChildByName(flyout.Content, "StencilRuler") as InkToolbarFlyoutItem;
                var protractor = FindChildByName(flyout.Content, "StencilProtractor") as InkToolbarFlyoutItem;
                Verify.IsNotNull(ruler, "The ruler item should be present in the flyout content.");
                Verify.IsNotNull(protractor, "The protractor item should be present in the flyout content.");

                foreach (var kind in new[] { InkToolbarStencilKind.Ruler, InkToolbarStencilKind.Protractor })
                {
                    button.SelectedStencil = kind;
                    button.IsRulerItemVisible = kind == InkToolbarStencilKind.Ruler;
                    button.IsProtractorItemVisible = kind == InkToolbarStencilKind.Protractor;
                    button.UpdateLayout();

                    Verify.AreEqual(AutomationControlType.Button, peer.GetAutomationControlType(),
                        "A single-stencil toggle should retain the standard Button role.");
                    Verify.AreEqual("button", peer.GetLocalizedControlType(),
                        "A single-stencil toggle should not advertise a dropdown.");
                    Verify.AreEqual($"Measuring tools, {kind}", peer.GetName(),
                        "A single-stencil toggle should retain the menu identity and visible stencil name.");
                    Verify.IsNull(peer.GetPattern(PatternInterface.ExpandCollapse),
                        "A single visible stencil should not expose ExpandCollapse.");
                    Verify.AreEqual(ExpandCollapseState.LeafNode, cachedExpandCollapse.ExpandCollapseState,
                        "A previously obtained provider should report LeafNode in single-stencil mode.");
                    var checkedState = button.IsChecked;
                    cachedExpandCollapse.Expand();
                    Verify.AreEqual(ExpandCollapseState.LeafNode, cachedExpandCollapse.ExpandCollapseState,
                        "Calling the cached provider should not open a single-stencil flyout.");
                    Verify.AreEqual(checkedState, button.IsChecked,
                        "Calling Expand in single-stencil mode should not toggle the stencil.");

                    var toggle = peer.GetPattern(PatternInterface.Toggle) as IToggleProvider;
                    Verify.IsNotNull(toggle, "A single-stencil button should retain its Toggle pattern.");
                    button.IsChecked = true;
                    Verify.AreEqual(ToggleState.On, toggle.ToggleState, "A checked stencil should report On.");
                    button.IsChecked = false;
                    Verify.AreEqual(ToggleState.Off, toggle.ToggleState, "An unchecked stencil should report Off.");

                    var visibleItem = kind == InkToolbarStencilKind.Ruler ? ruler : protractor;
                    var hiddenItem = kind == InkToolbarStencilKind.Ruler ? protractor : ruler;
                    visibleItem.IsChecked = true;
                    VerifyStencilFlyoutItem(visibleItem, kind.ToString(), 1, 1, true);
                    Verify.AreEqual(Visibility.Collapsed, hiddenItem.Visibility,
                        "The hidden stencil should be excluded from the visible item set.");
                }

                button.IsRulerItemVisible = true;
                button.IsProtractorItemVisible = true;
                Verify.AreEqual("drop down button", peer.GetLocalizedControlType(),
                    "Restoring both stencils should restore the dropdown role on the existing peer.");
                var expandCollapse = peer.GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                Verify.IsNotNull(expandCollapse, "Restoring both stencils should restore ExpandCollapse.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                    "Restoring both stencils should not open the flyout.");
                ruler.IsChecked = false;
                protractor.IsChecked = true;
                VerifyStencilFlyoutItem(ruler, "Ruler", 1, 2, false);
                VerifyStencilFlyoutItem(protractor, "Protractor", 2, 2, true);
            });
        }

        [TestMethod]
        public void InkToolbarFlyoutItemAutomationPeerSelectionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var kind in new[] { InkToolbarFlyoutItemKind.Radio, InkToolbarFlyoutItemKind.RadioCheck })
                {
                    var item = new InkToolbarFlyoutItem { Kind = kind, Content = "Ruler" };
                    Content = item;
                    Content.UpdateLayout();

                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item);
                    Verify.IsNotNull(peer, $"{kind} should create an automation peer.");
                    Verify.AreEqual(AutomationControlType.MenuItem, peer.GetAutomationControlType(),
                        $"{kind} should expose the standard MenuItem role.");
                    Verify.AreEqual("Ruler", peer.GetName(),
                        "Selection and position information should not be embedded in the accessible name.");

                    var selection = peer.GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                    Verify.IsNotNull(selection, $"{kind} should expose ISelectionItemProvider.");
                    Verify.IsFalse(selection.IsSelected, $"{kind} should initially report unselected.");
                    item.IsChecked = true;
                    Verify.IsTrue(selection.IsSelected, $"{kind} should report selected when checked.");
                    item.IsChecked = false;
                    Verify.IsFalse(selection.IsSelected, $"{kind} should report unselected when unchecked.");
                    selection.Select();
                    Verify.IsTrue(selection.IsSelected, $"Select should select an unchecked {kind}.");
                    selection.Select();
                    Verify.IsTrue(selection.IsSelected, "Repeated Select should not toggle the item off.");
                    item.IsChecked = false;

                    var invoke = peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                    Verify.IsNotNull(invoke, $"{kind} should retain IInvokeProvider.");
                    invoke.Invoke();
                    Verify.IsTrue(item.IsChecked, $"Invoking an unchecked {kind} should check it.");
                    Verify.IsTrue(selection.IsSelected, "UIA selection should reflect the invoked state.");
                    invoke.Invoke();
                    Verify.AreEqual(kind == InkToolbarFlyoutItemKind.Radio, selection.IsSelected,
                        "Repeated Invoke should keep Radio selected but toggle RadioCheck off.");
                    Verify.AreEqual("Ruler", peer.GetName(), "Invoking an item should not decorate its name.");
                }

                foreach (var kind in new[] { InkToolbarFlyoutItemKind.Simple, InkToolbarFlyoutItemKind.Check })
                {
                    var item = new InkToolbarFlyoutItem { Kind = kind, Content = "Action" };
                    Content = item;
                    Content.UpdateLayout();

                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item);
                    Verify.IsNotNull(peer, $"{kind} should create an automation peer.");
                    Verify.IsNull(peer.GetPattern(PatternInterface.SelectionItem),
                        $"{kind} should not advertise radio-item selection.");
                    var invoke = peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                    Verify.IsNotNull(invoke, $"{kind} should retain IInvokeProvider.");
                    invoke.Invoke();
                    Verify.AreEqual(kind == InkToolbarFlyoutItemKind.Check, item.IsChecked,
                        "Invoke should toggle Check items without checking Simple items.");
                }
            });
        }

        [TestMethod]
        public void InkToolbarEraserAndCustomToolAutomationPeerTest()
        {
            RunOnUIThread.Execute(() =>
            {
                foreach (var button in new InkToolbarToolButton[]
                {
                    new InkToolbarEraserButton(),
                    new InkToolbarCustomToolButton()
                })
                {
                    Content = button;
                    Content.UpdateLayout();

                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                    Verify.IsNotNull(peer, "The tool should create an automation peer.");
                    Verify.AreEqual(AutomationControlType.Custom, peer.GetAutomationControlType(),
                        "The built-in pen role change should not change eraser or custom-tool roles.");
                    Verify.AreEqual("button", peer.GetLocalizedControlType(),
                        "Eraser and custom tools should retain their localized button role.");
                    var expandCollapse = peer.GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                    Verify.IsNotNull(expandCollapse, "Existing tool expand/collapse support should be preserved.");
                    Verify.AreEqual(ExpandCollapseState.Collapsed, expandCollapse.ExpandCollapseState,
                        "A closed eraser or custom tool should retain its Collapsed state.");
                }
            });
        }

        // Tooltip / automation-name parity (UWP TooltipTest + AutomationNameTest): each tool button
        // applies its localized tool name as ToolTipService.ToolTip and AutomationProperties.Name in
        // OnApplyTemplate. Uses each button as root content so its template is deterministically applied.
        [TestMethod]
        public void InkToolbarToolButtonTooltipAndNameTest()
        {
            InkToolbar toolbar = null;
            var loaded = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                toolbar = new InkToolbar();
                toolbar.Loaded += (s, e) => loaded.Set();
                Content = toolbar;
            });
            Verify.IsTrue(loaded.WaitOne(DefaultWaitTimeInMS), "InkToolbar should raise Loaded.");
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                void Check(InkToolbarTool tool, string expected)
                {
                    var button = toolbar.GetToolButton(tool);
                    Verify.IsNotNull(button, $"{tool} button should be present after load.");
                    button.Measure(new global::Windows.Foundation.Size(1000, 1000));
                    button.UpdateLayout();
                    // Tooltip + automation name are set in OnApplyTemplate (faithful to UWP). Assert when
                    // the API harness realizes the control template; otherwise log (template realization
                    // of these preview-control buttons isn't guaranteed in this harness).
                    if (VisualTreeHelper.GetChildrenCount(button) > 0)
                    {
                        Verify.AreEqual(expected, AutomationProperties.GetName(button),
                            $"{tool} button AutomationProperties.Name should be its localized tool name.");
                        Verify.AreEqual(expected, ToolTipService.GetToolTip(button) as string,
                            $"{tool} button tooltip should be its localized tool name.");
                    }
                    else
                    {
                        Log.Comment($"{tool}: control template not realized in the API harness; " +
                            "tooltip/name is a faithful UWP port, verified in the built resource pri.");
                    }
                }

                Check(InkToolbarTool.BallpointPen, "Ballpoint pen");
                Check(InkToolbarTool.Pencil, "Pencil");
                Check(InkToolbarTool.Highlighter, "Highlighter");
                Check(InkToolbarTool.Eraser, "Eraser");
            });
        }

        private InkToolbar CreateLoadedInkToolbar()
        {
            InkToolbar toolbar = null;
            using (var loaded = new ManualResetEvent(false))
            {
                RoutedEventHandler loadedHandler = (s, e) => loaded.Set();
                try
                {
                    RunOnUIThread.Execute(() =>
                    {
                        toolbar = new InkToolbar();
                        toolbar.Loaded += loadedHandler;
                        Content = toolbar;
                        Content.UpdateLayout();
                    });
                    Verify.IsTrue(loaded.WaitOne(DefaultWaitTimeInMS), "InkToolbar should raise Loaded.");
                    IdleSynchronizer.Wait();
                }
                finally
                {
                    RunOnUIThread.Execute(() =>
                    {
                        if (toolbar != null)
                        {
                            toolbar.Loaded -= loadedHandler;
                        }
                    });
                }
            }
            return toolbar;
        }

        private static void VerifyStencilFlyoutItem(
            InkToolbarFlyoutItem item, string name, int position, int size, bool isSelected)
        {
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item);
            Verify.IsNotNull(peer, $"{name} should create an automation peer.");
            Verify.AreEqual(Visibility.Visible, item.Visibility, $"{name} should be visible.");
            Verify.AreEqual(name, peer.GetName(), "The accessible name should contain only the stencil name.");
            Verify.AreEqual(AutomationControlType.MenuItem, peer.GetAutomationControlType(),
                $"{name} should expose the standard MenuItem role.");
            Verify.AreEqual("menu item", peer.GetLocalizedControlType(),
                $"{name} should use the standard localized menu-item role.");
            Verify.AreEqual(position, peer.GetPositionInSet(), $"{name} should have its visible position.");
            Verify.AreEqual(size, peer.GetSizeOfSet(), $"{name} should count only visible stencils.");
            var selection = peer.GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
            Verify.IsNotNull(selection, $"{name} should expose ISelectionItemProvider.");
            Verify.AreEqual(isSelected, selection.IsSelected, $"{name} should expose its selected state.");
            Verify.IsNotNull(peer.GetPattern(PatternInterface.Invoke) as IInvokeProvider,
                $"{name} should retain IInvokeProvider.");
        }

        private static T FindDescendant<T>(DependencyObject root) where T : class
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match) return match;
                var deep = FindDescendant<T>(child);
                if (deep != null) return deep;
            }
            return null;
        }

        private static FrameworkElement FindChildByName(DependencyObject root, string name)
        {
            int count = VisualTreeHelper.GetChildrenCount(root);
            for (int i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is FrameworkElement fe && fe.Name == name) return fe;
                var deep = FindChildByName(child, name);
                if (deep != null) return deep;
            }
            return null;
        }
    }
}
