// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp.Utilities
{
    public class ApiTestBase
    {
        private Border _host;

        public const int DefaultWaitTimeInMS = 5000;

        // Set this content instead of using App.CurrentWindow.Content
        // because the latter requires you to tick the UI thread
        // before a layout pass can happen while you can directly call
        // UpdateLayout after the former, which is faster and less
        // sensitive to timing issues.
        public UIElement Content
        {
            get { return _host.Child; }
            set { _host.Child = value; }
        }

        [TestInitialize]
        public void Setup()
        {
            IdleSynchronizer.Wait();
            var hostLoaded = new ManualResetEvent(false);
            RunOnUIThread.Execute(() =>
            {
                _host = new Border();
                _host.Loaded += delegate { hostLoaded.Set(); };
                MUXControlsTestApp.App.TestContentRoot = _host;
            });
            Verify.IsTrue(hostLoaded.WaitOne(DefaultWaitTimeInMS), "Waiting for loaded event");
        }

        [TestCleanup]
        public void Cleanup()
        {
            TestUtilities.ClearVisualTreeRoot();
        }
    }
}
