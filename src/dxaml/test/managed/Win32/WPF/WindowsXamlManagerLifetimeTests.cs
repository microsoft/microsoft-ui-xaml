// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Text;
using System.Linq;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using Private.Infrastructure;

using WPFMarkup = System.Windows.Markup;

using Microsoft.UI.Xaml.Tests.Common;

using Microsoft.UI.Dispatching;
using XamlControls = Microsoft.UI.Xaml.Controls;
using XamlMedia = Microsoft.UI.Xaml.Media;
using XamlMarkup = Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Documents;
using Microsoft.UI.Xaml.Input;
using Windows.Foundation;
using System.Threading;
using System.Diagnostics;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls.Primitives;
using Xaml = Microsoft.UI.Xaml;

using WPFControls = System.Windows.Controls;
using WPFSystem = System.Windows;
using System.Runtime.InteropServices.WindowsRuntime;

using Private.Infrastructure.Hosting.WPF;

namespace Microsoft.UI.Xaml.Tests.Win32.Common
{
    internal class WindowContainer: IDisposable
    {
        private WPFSystem.Window window;
        private WindowsXamlHostWrapper xamlHost;

        public WindowContainer(WPFHost host, string text)
        {
            this.window = host.MainWindow;
            Verify.IsNotNull(window);
            var dispatcher = window.Dispatcher;

            dispatcher.Invoke(() =>
            {
               var container = new WPFControls.StackPanel();
               var children = container.Children;

               this.xamlHost = new WindowsXamlHostWrapper()
               {
                   Height = 50,
                   Width = 200,
                   Content = new Xaml.Controls.Button()
                   {
                        Height = 45,
                        Width = 150,
                        Name = "xamlButton1",
                        Content = new Xaml.Controls.TextBlock()
                        {
                            Text = text,
                        },
                    },
                };
                xamlHost.InsertInto(children);
                window.Title = text;
                window.Content = container;
            });

            TestServices.WindowHelper.WaitForIdle();
        }

        public void Dispose()
        {
            var dispatcher = this.window.Dispatcher;

            dispatcher.Invoke(() =>
            {
                this.window.Content = null;
                this.xamlHost.Dispose();
                this.xamlHost = null;
                this.window = null;
            });
        }
    }

    [TestClass]
    public class WindowsXamlManagerLifetimeTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("UAP:AppXManifest", AppxManifests.WINDOWS_VERSION_CURRENT_CENTENNIAL)]
        [TestProperty("UAP:Host", "PackagedCwa")]
        [TestProperty("ThreadingModel", "STA")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "WPF")]
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
        [TestProperty("Ignore", "TRUE")] // TODO: fix test, it's been disabled because it hits an intentional failfast in microsoft.ui.input
        public void CanRecreateXamlSourceAfterNewWindow()
        {
            DispatcherQueueController dqc = DispatcherQueueController.CreateOnCurrentThread();

            var host = TestServices.Win32Host as WPFHost;
            Verify.IsNotNull(host);

            using(var container1 = new WindowContainer(host, "Window 1"))
            {
                TestServices.WindowHelper.WaitForIdle();
            }

            // After doing there wont be any CompositionTarget any more and SynchronouslyTickUIThread will wait forever
            // Need to create new xaml content for the composition target to exist again
            //TestServices.WindowHelper.WaitForIdle();
            host.Reset();

            using(var container2 = new WindowContainer(host, "Window 2"))
            {
                TestServices.WindowHelper.WaitForIdle();
            }

            TestServices.WindowHelper.WaitForIdle();

            //Verify.AreNotEqual(window1, window2);

            dqc.ShutdownQueue();
        }
    }
}
