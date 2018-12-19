// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.Foundation;
using System.Threading;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using RefreshPullDirection = Microsoft.UI.Xaml.Controls.RefreshPullDirection;
using ScrollViewerIRefreshInfoProviderAdapter = Microsoft.UI.Private.Controls.ScrollViewerIRefreshInfoProviderAdapter;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ScrollViewerAdapterTests
    {
        [TestMethod]
        public void CanInstantiate()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
            });
        }

        [TestMethod]
        public void AdaptOnNullThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(null, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        public void AdaptOnSVWithNonUIElementContentThrows()
        {
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Content = 1;
                MUXControlsTestApp.App.TestContentRoot = sv;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = (ScrollViewer)MUXControlsTestApp.App.TestContentRoot;
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(sv, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        public void AdaptOnSVWithoutContentThrows()
        {
            var resetEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Loaded += (object sender, RoutedEventArgs e) => { resetEvent.Set(); };
                MUXControlsTestApp.App.TestContentRoot = sv;
            });

            IdleSynchronizer.Wait();
            resetEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = (ScrollViewer)MUXControlsTestApp.App.TestContentRoot;
                Verify.IsNull(sv.Content);
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                Verify.IsNotNull(adapter);
                Verify.Throws<ArgumentException>(() => { adapter.Adapt(sv, new Size(1.0, 1.0)); });
            });
        }

        [TestMethod]
        public void AdaptOnSV()
        {
            var resetEvent = new AutoResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                sv.Content = new Button();
                sv.Loaded += (object sender, RoutedEventArgs e) => { resetEvent.Set(); };

                MUXControlsTestApp.App.TestContentRoot = sv;
            });

            IdleSynchronizer.Wait();
            resetEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = (ScrollViewer)MUXControlsTestApp.App.TestContentRoot;
                ScrollViewerIRefreshInfoProviderAdapter adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                adapter.Adapt(sv, new Size(1.0, 1.0));
            });
        }

        [TestMethod]
        public void AdaptOnSVWithoutWaitingForLoaded()
        {
            var objects = new Dictionary<string, WeakReference>();
            var resetEvent = new AutoResetEvent(false);
            ScrollViewerIRefreshInfoProviderAdapter adapter;
            RunOnUIThread.Execute(() =>
            {
                ScrollViewer sv = new ScrollViewer();
                objects["sv"] = new WeakReference(sv);
                sv.Content = new Button();
                sv.Loaded += (object sender, RoutedEventArgs e) => { resetEvent.Set(); };
                adapter = new ScrollViewerIRefreshInfoProviderAdapter(RefreshPullDirection.TopToBottom, null);
                objects["adapter"] = new WeakReference(adapter);
                adapter.Adapt(sv, new Size(1.0, 1.0));
                MUXControlsTestApp.App.TestContentRoot = sv;
            });
            
            IdleSynchronizer.Wait();
            resetEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                adapter = null;
                MUXControlsTestApp.App.TestContentRoot = null;
            });
            
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        void CheckLeaks(Dictionary<string, WeakReference> objects)
        {
            foreach (var pair in objects)
            {
                Log.Comment("Checking if object {0} has been collected", pair.Key);
                object target = pair.Value.Target;
                if (target != null)
                {
                    Verify.DisableVerifyFailureExceptions = true; // throwing exceptions makes running this in TDP harder
                    Verify.Fail(String.Format("Object {0} is still alive when it should not be.", target));
                    Verify.DisableVerifyFailureExceptions = false;
                }
            }
            objects.Clear();
        }
    }

}
