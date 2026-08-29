// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Text;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;
using Microsoft.UI.Xaml.Tests.Common;
using System.Threading.Tasks;
using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml;
using System.Diagnostics;
using Microsoft.UI.Xaml.Controls.Primitives;
using System.Runtime.InteropServices.WindowsRuntime;
using Microsoft.UI.Dispatching;


namespace Microsoft.UI.Xaml.Tests.Controls.DispatcherQueueTests
{
    [TestClass]
    public class DispatcherQueueTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Verify for UI Element's DependencyObject that its DispatcherQueue property is correctly set")]
        public async Task VerifyDispatcherQueuePropertyForUIElement()
        {
            StackPanel root = null;
            Button button = null;
            int threadid = -1;

            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                StackPanel stackPanel = new StackPanel();
                button = new Button();
                button.Width = 100;
                button.Height = 100;

                stackPanel.Children.Add(button);
                root.Children.Add(stackPanel);
                TestServices.WindowHelper.WindowContent = root;
                threadid = Environment.CurrentManagedThreadId;
            });
            TestServices.WindowHelper.WaitForIdle();
            
            var waiterDQ = new TaskCompletionSource<object>();
            
            // UIElement's DQ property
            var dispatcherQueue = button.DispatcherQueue;
            Verify.IsNotNull(dispatcherQueue);
            
            Verify.AreNotEqual(threadid, Environment.CurrentManagedThreadId);
            
            dispatcherQueue.TryEnqueue( new DispatcherQueueHandler(() =>
            {
                Verify.AreEqual(threadid, Environment.CurrentManagedThreadId);
                waiterDQ.TrySetResult(null);
            }));
            
            await waiterDQ.Task;
            TestServices.WindowHelper.WaitForIdle();

        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Description", "Verify for Window that its DispatcherQueue property is correctly set")]
        public async Task VerifyDispatcherQueuePropertyForWindow()
        {
            StackPanel root = null;
            Button button = null;
            int threadid = -1;
            var waiterWindow = new TaskCompletionSource<object>();
            Window window = null;
            UIExecutor.Execute(() =>
            {
                root = new StackPanel();
                StackPanel stackPanel = new StackPanel();
                button = new Button();
                button.Width = 100;
                button.Height = 100;

                stackPanel.Children.Add(button);
                root.Children.Add(stackPanel);
                TestServices.WindowHelper.WindowContent = root;
                threadid = Environment.CurrentManagedThreadId;
                window = Window.Current;
            });
            TestServices.WindowHelper.WaitForIdle();

            var windowDispatcherQueue = window.DispatcherQueue;
            Verify.IsNotNull(windowDispatcherQueue);
            Verify.AreNotEqual(threadid, Environment.CurrentManagedThreadId);
            
            windowDispatcherQueue.TryEnqueue( new DispatcherQueueHandler(() =>
            {
                Verify.AreEqual(threadid, Environment.CurrentManagedThreadId);
                waiterWindow.TrySetResult(null);
            }));
            await waiterWindow.Task;
            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
