// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using Windows.UI.Input.Inking;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class InkCanvasTests : ApiTestBase
    {
        [TestMethod]
        public void InkCanvasDefaultPropertyValuesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Verify default property values
                Verify.IsTrue(inkCanvas.IsEnabled, "InkCanvas should be enabled by default.");
                Verify.AreEqual(InkCanvasMode.Draw, inkCanvas.Mode, "Default mode should be Draw.");
                Verify.AreEqual(
                    InkInputType.Pen | InkInputType.Mouse,
                    inkCanvas.AllowedInputTypes,
                    "Default AllowedInputTypes should be Pen | Mouse.");
            });
        }

        [TestMethod]
        public void InkCanvasModePropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Default
                Verify.AreEqual(InkCanvasMode.Draw, inkCanvas.Mode, "Default mode should be Draw.");

                // Set to Erase
                inkCanvas.Mode = InkCanvasMode.Erase;
                Verify.AreEqual(InkCanvasMode.Erase, inkCanvas.Mode, "Mode should be Erase after setting.");

                // Set to Select
                inkCanvas.Mode = InkCanvasMode.Select;
                Verify.AreEqual(InkCanvasMode.Select, inkCanvas.Mode, "Mode should be Select after setting.");

                // Set back to Draw
                inkCanvas.Mode = InkCanvasMode.Draw;
                Verify.AreEqual(InkCanvasMode.Draw, inkCanvas.Mode, "Mode should be Draw after setting back.");
            });
        }

        [TestMethod]
        public void InkCanvasAllowedInputTypesPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Default
                Verify.AreEqual(
                    InkInputType.Pen | InkInputType.Mouse,
                    inkCanvas.AllowedInputTypes,
                    "Default should be Pen | Mouse.");

                // Set to Pen only
                inkCanvas.AllowedInputTypes = InkInputType.Pen;
                Verify.AreEqual(InkInputType.Pen, inkCanvas.AllowedInputTypes, "Should be Pen only.");

                // Set to Touch only
                inkCanvas.AllowedInputTypes = InkInputType.Touch;
                Verify.AreEqual(InkInputType.Touch, inkCanvas.AllowedInputTypes, "Should be Touch only.");

                // Set to all input types
                inkCanvas.AllowedInputTypes = InkInputType.Pen | InkInputType.Touch | InkInputType.Mouse;
                Verify.AreEqual(
                    InkInputType.Pen | InkInputType.Touch | InkInputType.Mouse,
                    inkCanvas.AllowedInputTypes,
                    "Should be Pen | Touch | Mouse.");

                // Set to None
                inkCanvas.AllowedInputTypes = InkInputType.None;
                Verify.AreEqual(InkInputType.None, inkCanvas.AllowedInputTypes, "Should be None.");
            });
        }

        [TestMethod]
        public void InkCanvasIsEnabledPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Default
                Verify.IsTrue(inkCanvas.IsEnabled, "InkCanvas should be enabled by default.");

                // Disable
                inkCanvas.IsEnabled = false;
                Verify.IsFalse(inkCanvas.IsEnabled, "InkCanvas should be disabled after setting to false.");

                // Re-enable
                inkCanvas.IsEnabled = true;
                Verify.IsTrue(inkCanvas.IsEnabled, "InkCanvas should be enabled after setting to true.");
            });
        }

        [TestMethod]
        public void InkCanvasDefaultDrawingAttributesPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Set DefaultDrawingAttributes
                var attrs = new InkDrawingAttributes();
                attrs.Color = Windows.UI.Colors.Red;
                attrs.Size = new Windows.Foundation.Size(5, 5);
                attrs.PenTip = PenTipShape.Circle;

                inkCanvas.DefaultDrawingAttributes = attrs;

                var retrieved = inkCanvas.DefaultDrawingAttributes;
                Verify.IsNotNull(retrieved, "DefaultDrawingAttributes should not be null after setting.");
                Verify.AreEqual(Windows.UI.Colors.Red, retrieved.Color, "Color should be Red.");
                Verify.AreEqual(5.0, retrieved.Size.Width, "Width should be 5.");
                Verify.AreEqual(5.0, retrieved.Size.Height, "Height should be 5.");
                Verify.AreEqual(PenTipShape.Circle, retrieved.PenTip, "PenTip should be Circle.");
            });
        }

        [TestMethod]
        public void InkCanvasDrawingAttributesPenTipTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Rectangle pen tip
                var attrs = new InkDrawingAttributes();
                attrs.PenTip = PenTipShape.Rectangle;
                inkCanvas.DefaultDrawingAttributes = attrs;
                Verify.AreEqual(PenTipShape.Rectangle, inkCanvas.DefaultDrawingAttributes.PenTip,
                    "PenTip should be Rectangle.");

                // Circle pen tip
                attrs.PenTip = PenTipShape.Circle;
                inkCanvas.DefaultDrawingAttributes = attrs;
                Verify.AreEqual(PenTipShape.Circle, inkCanvas.DefaultDrawingAttributes.PenTip,
                    "PenTip should be Circle.");
            });
        }

        [TestMethod]
        public void InkCanvasStrokeContainerAccessTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // StrokeContainer may be null before InkPresenter is initialized (not loaded in tree)
                // But the property should be accessible without throwing
                var container = inkCanvas.StrokeContainer;
                // After loading, StrokeContainer comes from InkPresenter, which is created on load.
                // Here we just verify the getter doesn't throw.
                Log.Comment("StrokeContainer getter is accessible (value may be null before load).");
            });
        }

        [TestMethod]
        public void InkCanvasClearStrokesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // ClearStrokes should not throw even when there are no strokes / not loaded
                inkCanvas.ClearStrokes();
                Log.Comment("ClearStrokes did not throw when called before load.");
            });
        }

        [TestMethod]
        public void InkCanvasInVisualTreeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:InkCanvas x:Name='TestInkCanvas'
                            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            Width='400' Height='300' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var inkCanvas = (InkCanvas)root.FindName("TestInkCanvas");
                Verify.IsNotNull(inkCanvas, "InkCanvas should be found in visual tree.");
                Verify.AreEqual(400.0, inkCanvas.Width, "Width should be 400.");
                Verify.AreEqual(300.0, inkCanvas.Height, "Height should be 300.");
            });
        }

        [TestMethod]
        public void InkCanvasModeDependencyPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Verify the dependency property is accessible
                var dp = InkCanvas.ModeProperty;
                Verify.IsNotNull(dp, "ModeProperty should not be null.");

                var inkCanvas = new InkCanvas();
                inkCanvas.SetValue(dp, InkCanvasMode.Erase);
                Verify.AreEqual(InkCanvasMode.Erase, (InkCanvasMode)inkCanvas.GetValue(dp),
                    "Mode should be Erase via DependencyProperty.");
            });
        }

        [TestMethod]
        public void InkCanvasAllowedInputTypesDependencyPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var dp = InkCanvas.AllowedInputTypesProperty;
                Verify.IsNotNull(dp, "AllowedInputTypesProperty should not be null.");

                var inkCanvas = new InkCanvas();
                inkCanvas.SetValue(dp, InkInputType.Touch);
                Verify.AreEqual(InkInputType.Touch, (InkInputType)inkCanvas.GetValue(dp),
                    "AllowedInputTypes should be Touch via DependencyProperty.");
            });
        }

        [TestMethod]
        public void InkCanvasDefaultDrawingAttributesDependencyPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var dp = InkCanvas.DefaultDrawingAttributesProperty;
                Verify.IsNotNull(dp, "DefaultDrawingAttributesProperty should not be null.");

                var attrs = new InkDrawingAttributes();
                attrs.Color = Windows.UI.Colors.Blue;

                var inkCanvas = new InkCanvas();
                inkCanvas.SetValue(dp, attrs);

                var retrieved = (InkDrawingAttributes)inkCanvas.GetValue(dp);
                Verify.IsNotNull(retrieved, "Retrieved attributes should not be null.");
                Verify.AreEqual(Windows.UI.Colors.Blue, retrieved.Color, "Color should be Blue.");
            });
        }

        [TestMethod]
        public void InkCanvasIsEnabledDependencyPropertyTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var dp = InkCanvas.IsEnabledProperty;
                Verify.IsNotNull(dp, "IsEnabledProperty should not be null.");

                var inkCanvas = new InkCanvas();
                Verify.IsTrue((bool)inkCanvas.GetValue(dp), "Default IsEnabled should be true.");

                inkCanvas.SetValue(dp, false);
                Verify.IsFalse((bool)inkCanvas.GetValue(dp), "IsEnabled should be false.");
            });
        }

        [TestMethod]
        public void InkCanvasModePropertyChangePersistsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // Cycle through all modes and verify each persists
                foreach (InkCanvasMode mode in new[] { InkCanvasMode.Draw, InkCanvasMode.Erase, InkCanvasMode.Select })
                {
                    inkCanvas.Mode = mode;
                    Verify.AreEqual(mode, inkCanvas.Mode, $"Mode should persist as {mode}.");
                }
            });
        }

        [TestMethod]
        public void InkCanvasMultipleInstancesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var canvas1 = new InkCanvas();
                var canvas2 = new InkCanvas();

                // Set different properties on each
                canvas1.Mode = InkCanvasMode.Erase;
                canvas2.Mode = InkCanvasMode.Select;

                canvas1.AllowedInputTypes = InkInputType.Pen;
                canvas2.AllowedInputTypes = InkInputType.Touch | InkInputType.Mouse;

                // Verify they are independent
                Verify.AreEqual(InkCanvasMode.Erase, canvas1.Mode, "Canvas1 mode should be Erase.");
                Verify.AreEqual(InkCanvasMode.Select, canvas2.Mode, "Canvas2 mode should be Select.");
                Verify.AreEqual(InkInputType.Pen, canvas1.AllowedInputTypes, "Canvas1 should be Pen.");
                Verify.AreEqual(InkInputType.Touch | InkInputType.Mouse, canvas2.AllowedInputTypes,
                    "Canvas2 should be Touch | Mouse.");
            });
        }

        [TestMethod]
        public void InkCanvasStrokeCollectedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();
                bool eventFired = false;

                // Subscribe to event - verify it doesn't throw
                inkCanvas.StrokeCollected += (sender, args) =>
                {
                    eventFired = true;
                };

                // Event won't fire without ink presenter activity, but subscription should work.
                Log.Comment("StrokeCollected event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkCanvasStrokesErasedEventTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();
                bool eventFired = false;

                // Subscribe to event - verify it doesn't throw
                inkCanvas.StrokesErased += (sender, args) =>
                {
                    eventFired = true;
                };

                Log.Comment("StrokesErased event subscription succeeded.");
            });
        }

        [TestMethod]
        public void InkCanvasInVisualTreeWithPropertiesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = (Grid)XamlReader.Load(
                    @"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                           xmlns:controls='using:Microsoft.UI.Xaml.Controls'>
                        <controls:InkCanvas x:Name='TestInkCanvas'
                            Width='500' Height='400'
                            Mode='Erase' />
                    </Grid>");

                Content = root;
                Content.UpdateLayout();

                var inkCanvas = (InkCanvas)root.FindName("TestInkCanvas");
                Verify.IsNotNull(inkCanvas, "InkCanvas should be found.");
                Verify.AreEqual(InkCanvasMode.Erase, inkCanvas.Mode, "Mode should be Erase from XAML.");
            });
        }
    }
}
