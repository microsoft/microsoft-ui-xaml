// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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

        // The following tests exercise the InkCanvas weak-reference / callback lifetime behavior.
        //
        // In OnLoaded, InkCanvas wires up XamlRoot.Changed and SizeChanged handlers that capture a
        // weak reference to itself. Those weak references are created through the safe
        // make_weak(static_cast<winrt::InkCanvas>(*this)) pattern (rather than the projected get_weak())
        // to avoid the C++/WinRT over-release bug (cppwinrt #1431) for composed/aggregated objects.
        //
        // TestInkCanvas (a managed subclass, below) forces exactly that composed/aggregation scenario:
        // the CLR creates an outer object that aggregates the native InkCanvas across a module boundary,
        // which is where the projected get_weak() bug would manifest. These tests load and then destroy
        // such instances while the callbacks may still be pending and verify no crash / refcount imbalance.

        // Managed subclass so the native InkCanvas is composed/aggregated by an outer object in another
        // module - the exact scenario affected by the get_weak() over-release bug.
        public class TestInkCanvas : InkCanvas
        {
        }

        [TestMethod]
        public void InkCanvasLoadUnloadDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                Content = new TestInkCanvas();
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Unload the InkCanvas. This revokes the XamlRoot.Changed and SizeChanged handlers
                // that were established during load.
                Content = null;
                Log.Comment("InkCanvas unloaded without crashing.");
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void InkCanvasDestroyedBeforePendingCallbacksDoesNotCrash()
        {
            // Create, load, and then immediately drop and destroy several aggregated InkCanvas instances.
            // Destroying the instance while load-time callbacks may still be pending exercises the
            // weak-capture-fails-safely path. If the weak reference had been created with the buggy
            // projected get_weak(), destroying the aggregated outer object here could over-release it and
            // lead to a use-after-free / access violation.
            for (int i = 0; i < 5; i++)
            {
                RunOnUIThread.Execute(() =>
                {
                    Content = new TestInkCanvas();
                    Content.UpdateLayout();

                    // Immediately remove from the tree while callbacks may still be pending.
                    Content = null;
                });

                IdleSynchronizer.Wait();

                // Force the managed wrapper (and, transitively, the native InkCanvas) to be collected so
                // that any callback that fires afterwards must safely observe a dropped weak reference.
                RunOnUIThread.Execute(() =>
                {
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                });

                IdleSynchronizer.Wait();
            }

            Log.Comment("Repeated create/load/unload/collect cycles completed without crashing.");
        }
    }
}
