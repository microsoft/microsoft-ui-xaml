// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using System.Threading;
using Common;
using System.Diagnostics;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    // This is marked as a test class to make sure our AssemblyInitialize and
    // AssemblyCleanup fixtures get executed.  It won't actually host any tests.
    [TestClass]
    public class ApiTestAssemblyHandling
    {
        // Set when Program.Main returns, i.e. once the XAML app's message loop has ended.
        private static readonly ManualResetEvent s_appExited = new ManualResetEvent(false);

        // How long AssemblyCleanup waits for the XAML app to exit before ending the process itself.
        private static readonly TimeSpan s_appExitTimeout = TimeSpan.FromSeconds(30);

        [AssemblyInitialize]
        [TestProperty("CoreClrProfile", "localDotNet")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")] //24049610
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS4)]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        [TestProperty("IsolationLevel", "Class")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Host", "PackagedCWA")]
        public static void AssemblyInitialize(TestContext testContext)
        {
            if (testContext.Properties.Contains("WaitForDebugger") || testContext.Properties.Contains("WaitForAppDebugger"))
            {
                var processId = Windows.System.Diagnostics.ProcessDiagnosticInfo.GetForCurrentProcess().ProcessId;
                var waitEvent = new AutoResetEvent(false);

                while (!IsDebuggerPresent())
                {
                    Log.Comment(string.Format("Waiting for a debugger to attach (processId = {0})...", processId));
                    Windows.System.Threading.ThreadPoolTimer.CreateTimer((timer) => { waitEvent.Set(); }, TimeSpan.FromSeconds(1));
                    waitEvent.WaitOne();
                }

                DebugBreak();
            }

            ApiTestBase.EnableAllXamlOptionalChanges();

            // This is the entry point for API tests rather than Program.Main, so we'll call that on another thread
            // in order to initialize the XAML application for API testing.  It needs to be on its own thread because
            // it doesn't return - it contains the application loop.
#nullable enable
            _ = ThreadPool.QueueUserWorkItem((object? param) =>
            {
                try
                {
                    Program.Main(Array.Empty<string>());
                }
                finally
                {
                    s_appExited.Set();
                }
            });
#nullable restore

            App.AppLaunchedEvent.WaitOne();
        }

        [DllImport("kernel32.dll")]
        private static extern bool IsDebuggerPresent();

        [DllImport("kernel32.dll")]
        private static extern void DebugBreak();

        [AssemblyCleanup]
        public static void AssemblyCleanup()
        {
            // Exit the XAML app and wait for its message loop to end, then leave ending the host process to TAEF.
            // This used to kill the process from a thread-pool thread instead (TODO 27390753), which raced TAEF's own
            // shutdown of the host: the host died with exit code 0xFFFFFFFF partway through it, and on Windows 10 RS5
            // te.exe often never recovered, holding the pipeline job until its 300-minute timeout.
            var dispatcherQueue = IdleSynchronizer.DispatcherQueue;
            if (dispatcherQueue != null
                && dispatcherQueue.TryEnqueue(() => Microsoft.UI.Xaml.Application.Current.Exit())
                && s_appExited.WaitOne(s_appExitTimeout))
            {
                Log.Comment("The XAML app exited.");
                return;
            }

            // The app didn't exit in time, so fall back to ending the process ourselves.
            Log.Warning(string.Format("The XAML app didn't exit within {0} seconds, so ending the test host process.", s_appExitTimeout.TotalSeconds));
#nullable enable
            _ = ThreadPool.QueueUserWorkItem((object? param) => Process.GetCurrentProcess().Kill());
#nullable restore
        }
    }
}
