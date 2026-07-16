// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Markup;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using InkPresenter = global::Microsoft.UI.Xaml.Controls.InkPresenter;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // The InkCanvas public surface intentionally mirrors the classic
    // Windows.UI.Xaml.Controls.InkCanvas exactly: a default constructor plus a
    // single read-only InkPresenter property. All ink configuration flows through
    // the InkPresenter, so these tests only validate construction and that the
    // control participates in the visual tree.
    [TestClass]
    public class InkCanvasTests : ApiTestBase
    {
        [TestMethod]
        public void InkCanvasConstructionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();
                Verify.IsNotNull(inkCanvas, "InkCanvas should construct without throwing.");
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
        public void InkCanvasInkPresenterAccessorTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var inkCanvas = new InkCanvas();

                // The InkPresenter is created on a dedicated ink thread and may not be
                // available synchronously right after construction. The accessor must
                // simply be callable without throwing.
                InkPresenter presenter = inkCanvas.InkPresenter;
                Log.Comment("InkPresenter getter is accessible (value may be null before the ink thread initializes).");
            });
        }

        [TestMethod]
        public void InkCanvasMultipleInstancesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var canvas1 = new InkCanvas();
                var canvas2 = new InkCanvas();

                Verify.IsNotNull(canvas1, "Canvas1 should construct.");
                Verify.IsNotNull(canvas2, "Canvas2 should construct.");
                Verify.AreNotSame(canvas1, canvas2, "Instances should be independent.");
            });
        }
    }
}
