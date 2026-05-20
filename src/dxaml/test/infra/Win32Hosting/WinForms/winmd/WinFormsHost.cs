// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Windows;
using System.Threading;
using System.Threading.Tasks;
using System.Collections.Generic;
using Windows.Foundation;
using Microsoft.Windows.Interop;
using Private.Infrastructure.Hosting;

namespace Private.Infrastructure.Hosting.WinForms
{
#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    public sealed class WinFormsHost : IWin32Host, IDisposable
    {
        private readonly TaskCompletionSource<Window> hostWindow;
        private Application application;
        private readonly Thread uiThread;
        private bool noExit = true;
        private readonly TaskCompletionSource<XamlContentHost> xamlContentHost;

        internal WinFormsHost()
        {
            this.hostWindow = new TaskCompletionSource<Window>();
            this.xamlContentHost = new TaskCompletionSource<XamlContentHost>();

            this.uiThread = new Thread(this.UIThreadProc);
            this.uiThread.SetApartmentState(ApartmentState.STA);
            this.uiThread.Start();
        }

        internal void UIThreadProc()
        {
            this.application = new Application();

            while (this.noExit)
            {
                Window window = null;
                if (this.hostWindow.Task.IsCompleted)
                {
                    window = this.hostWindow.Task.Result;
                }
                else
                {
                    window = new Window();
                    this.hostWindow.SetResult(window);
                }
                this.application.Run(window);
                window.Content = null;
            }
        }

        public ulong MainWindowHandle
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        public void Reset()
        {
            throw new NotImplementedException();
        }

        private XamlContentHost XamlContentHost
        {
            get
            {
                return this.xamlContentHost.Task.Result;
            }
        }

        public object Content
        {
            get { return this.XamlContentHost.Content; }
            set { this.XamlContentHost.Content = value as Microsoft.UI.Xaml.FrameworkElement; }
        }

        internal async Task Init()
        {
            TaskCompletionSource<XamlContentHost> jupiterHostCompletionSource = new TaskCompletionSource<XamlContentHost>();
            await this.ExecuteOnUIThread(async () =>
            {
                await this.SetContent(new XamlContentHost());
            });
            XamlContentHost host = await jupiterHostCompletionSource.Task;
            this.xamlContentHost.SetResult(host);
        }

        internal async Task SetContent(object content)
        {
            await this.ExecuteOnUIThread(async () =>
            {
                var window = await this.hostWindow.Task;
                window.Content = content;
            });
        }

        internal async Task ExecuteOnUIThread(Func<Task> action)
        {
            var window = await this.hostWindow.Task;
            await window.Dispatcher.InvokeAsync(action);
        }

        internal Task<object> GetDispatcherQueueInternal()
        {
            throw new NotImplementedException();
        }

        public IAsyncOperation<object> GetDispatcherQueue()
        {
            return GetDispatcherQueueInternal().AsAsyncOperation();
        }

        public void Dispose()
        {
            noExit = false;
            Application.Current.Shutdown();
            this.uiThread.Join();
        }

        public void SetWindowSizeOverride(Double width, Double height)
        {
            throw new NotImplementedException();
        }

        public void GCCollect()
        {
            GC.Collect(2, GCCollectionMode.Forced, true);
            GC.WaitForPendingFinalizers();
        }

        void IWin32Host.DoEvents()
        {
            throw new NotImplementedException();
        }
    }
}
