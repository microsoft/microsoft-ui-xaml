// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Controls;

namespace Microsoft.UI.Xaml.Tests.Foundation.Win32.Common
{
    [TestClass]
    public class WindowsXamlManagerTestsCentennial : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("UAP:AppXManifest", AppxManifests.WINDOWS_VERSION_CURRENT_CENTENNIAL)]
        [TestProperty("UAP:Host", "PackagedCwa")]
        [TestProperty("ThreadingModel", "STA")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            DispatcherQueue.GetForCurrentThread().DoEvents();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestCleanup]
        public override void TestCleanup()
        {
            DispatcherQueue.GetForCurrentThread().DoEvents();
            // Don't call base
        }

        [TestMethod]
        public void CanNestWindowsXamlManager()
        {
            DispatcherQueueController dqc = DispatcherQueueController.CreateOnCurrentThread();

            using (WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                using (WindowsXamlManager.InitializeForCurrentThread())
                {
                    Log.Comment("Nest manager 2");
                    var bt2 = new Button() { Height = 10, Width = 10, };
                    Verify.IsNotNull(bt2);
                }
                var bt3 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt3);
            }
            dqc.DispatcherQueue.DoEvents();
            Log.Comment("Nesting completed");

            dqc.ShutdownQueue();
        }

        [TestMethod]
        public void CanCreateMultipleWindowsXamlManager()
        {
            DispatcherQueueController dqc = DispatcherQueueController.CreateOnCurrentThread();

            using (WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
            }

            // Note: On System Xaml, the next line throws an exception because the message pump hasn't been drained
            using (WindowsXamlManager.InitializeForCurrentThread()){};
            
            DispatcherQueue.GetForCurrentThread().DoEvents();

            using (WindowsXamlManager.InitializeForCurrentThread())
            {
                Log.Comment("Manager 2");
                var bt2 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt2);
            }
            DispatcherQueue.GetForCurrentThread().DoEvents();
            Log.Comment("Creation completed");

            dqc.ShutdownQueue();
        }

        [TestMethod]
        public void CanCreateMultipleDesktopWindowXamlSource()
        {
            DispatcherQueueController dqc = DispatcherQueueController.CreateOnCurrentThread();

            using (new DesktopWindowXamlSource())
            {
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                Log.Comment("XamlSource 1");
            }

            // Note: On System Xaml, the next line throws an exception because the message pump hasn't been drained

            using (new DesktopWindowXamlSource()){};

            DispatcherQueue.GetForCurrentThread().DoEvents();

            using (new DesktopWindowXamlSource())
            {
                var bt2 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt2);
                Log.Comment("XamlSource 2");
            }
            DispatcherQueue.GetForCurrentThread().DoEvents();

            dqc.ShutdownQueue();
        }

        [TestMethod]
        [TestProperty("Ignore", "TRUE")] // DCPP: lifted hwnd Xaml islands
        public void CanNestDesktopWindowXamlSource()
        {
            using (new DesktopWindowXamlSource())
            {
                Log.Comment("XamlSource 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                using (new DesktopWindowXamlSource())
                {
                    Log.Comment("Nest XamlSource 2");
                    var bt2 = new Button() { Height = 10, Width = 10, };
                    Verify.IsNotNull(bt2);
                }
                var bt3 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt3);
            }
            DispatcherQueue.GetForCurrentThread().DoEvents();
            Log.Comment("Nesting completed");
        }

        [TestMethod]
        public void CanCreateMultipleXamlIsland()
        {
            DispatcherQueueController dqc = DispatcherQueueController.CreateOnCurrentThread();

            using (new XamlIsland())
            {
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                Log.Comment("XamlIsland 1");
            }

            // Note: On System Xaml, the next line throws an exception because the message pump hasn't been drained

            using (new XamlIsland()){};

            DispatcherQueue.GetForCurrentThread().DoEvents();

            using (new XamlIsland())
            {
                var bt2 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt2);
                Log.Comment("XamlIsland 2");
            }
            DispatcherQueue.GetForCurrentThread().DoEvents();

            dqc.ShutdownQueue();
        }

        [TestMethod]
        [TestProperty("Ignore", "TRUE")]
        public void CanNestXamlIsland()
        {
            using (new XamlIsland())
            {
                Log.Comment("XamlIsland 1");
                var bt1 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt1);
                using (new XamlIsland())
                {
                    Log.Comment("Nest XamlIsland 2");
                    var bt2 = new Button() { Height = 10, Width = 10, };
                    Verify.IsNotNull(bt2);
                }
                var bt3 = new Button() { Height = 10, Width = 10, };
                Verify.IsNotNull(bt3);
            }
            DispatcherQueue.GetForCurrentThread().DoEvents();
            Log.Comment("Nesting completed");
        }    
    }
}
