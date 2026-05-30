// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Threading;
using Private.Infrastructure.Hosting;
using MUD = Microsoft.UI.Dispatching;
using WUX = Microsoft.UI.Xaml;
using WUC = Windows.UI.Core;
using Microsoft.Toolkit.Wpf.UI.XamlHost;

namespace Private.Infrastructure.Hosting.WPF
{
    internal class WPFApp : System.Windows.Application
    {
        public WPFApp()
        {
        }

        protected override void OnExit(System.Windows.ExitEventArgs e)
        {
            base.OnExit(e);
        }
    }

#if BUILD_WINDOWS
    [CLSCompliant(false)]
#endif
    public sealed partial class WPFHost : IWin32Host, IDisposable
    {
        private readonly TaskCompletionSource<Dispatcher> hostDispatcher;
        private TaskCompletionSource<WPFTestWindow> currentWindow;
        private readonly Application application;
        private TaskCompletionSource<Microsoft.UI.Dispatching.DispatcherQueue> dispatcherQueue;
        private readonly Thread uiThread;
        private bool noExit;
        private readonly bool initializeXamlManager;
        private readonly DpiAwarenessContext _dpiAwarenessContext;

        internal WPFHost(DpiAwarenessContext dpiAwarenessContext, bool initializeXamlManager)
        {
            this.noExit = true;
            this.initializeXamlManager = initializeXamlManager;
            this._dpiAwarenessContext = dpiAwarenessContext;
            if (Application.Current == null)
            {
                this.application = new WPFApp();
            }
            else
            {
                this.application = Application.Current as WPFApp;
            }
            this.hostDispatcher = new TaskCompletionSource<Dispatcher>();
            this.dispatcherQueue = new TaskCompletionSource<Microsoft.UI.Dispatching.DispatcherQueue>();
            this.currentWindow = new TaskCompletionSource<WPFTestWindow>();
            this.uiThread = new Thread(this.UIThreadProc);
            this.uiThread.SetApartmentState(ApartmentState.STA);
            this.uiThread.Start();
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern DpiAwarenessContext SetThreadDpiAwarenessContext(DpiAwarenessContext dpiContext);

        private void UIThreadProc()
        {
            Debug.WriteLine("WPFHost - DPI awareness context being set to " + this._dpiAwarenessContext);
            SetThreadDpiAwarenessContext(this._dpiAwarenessContext);
            var dispatcher = Dispatcher.CurrentDispatcher;
            dispatcher.UnhandledException += this.OnDispatcherUnhandledException;
            try
            {
                var dqc = MUD::DispatcherQueueController.CreateOnCurrentThread();

                this.hostDispatcher.SetResult(Dispatcher.CurrentDispatcher);
                while (noExit)
                {
                    dispatcher.DoEvents();

                    if (this.initializeXamlManager)
                    {
                        // Using XamlApplication as below (desired way) causing tests to fail with timeout error
                        // in TestCleanup at the very end of execution of tests.
                        // using (new XamlApplication())
                        using (WUX.Hosting.WindowsXamlManager.InitializeForCurrentThread())
                        {
                            this.WindowMessageLoop();
                        }
                    }
                    else
                    {
                        // Dont call InitializeForCurrentThread() and let the test own the xaml core
                        this.WindowMessageLoop();
                    }
                    dispatcher.DoEvents();
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    GC.Collect();
                    GC.WaitForPendingFinalizers();
                    dispatcher.DoEvents();
                }
                dqc.ShutdownQueue();
            }
            catch (Exception e)
            {
                AppDomainExceptionHandler.LogException(e);
                var dispatcherQ = this.dispatcherQueue;
                var curW = this.currentWindow;
                if (IsDisposed)
                {
                    dispatcherQ.TrySetResult(null);
                    curW.TrySetResult(null);
                }
                else
                {
                    dispatcherQ.TrySetException(e);
                    curW.TrySetException(e);
                }
            }
            finally
            {
                dispatcher.UnhandledException -= this.OnDispatcherUnhandledException;
                this.dispatcherQueue.TrySetResult(null);
                this.currentWindow.TrySetResult(null);
            }
        }

        private void WindowMessageLoop()
        {
            var dispatcher = Dispatcher.CurrentDispatcher;
            var dispatcherQ = this.dispatcherQueue;
            var curW = this.currentWindow;
            dispatcher.DoEvents();
            var window = new WPFTestWindow();

            RoutedEventHandler loaded = (o, a) =>
            {
                window.Focus();

                curW.TrySetResult(window);
                window.winRT_MainWindowHandle = (ulong)this.MainWindow.CurrentMainWindowHandle;
                Interop.SetForegroundWindow(this.MainWindow.CurrentMainWindowHandle);

                var dispatcherQueueValue = Microsoft.UI.Dispatching.DispatcherQueue.GetForCurrentThread();
                dispatcherQ.TrySetResult(dispatcherQueueValue);
            };
            window.Loaded += loaded;
            try
            {
                dispatcher.DoEvents();
                window.Show();
                try
                {
                    while (!window.close)
                    {
                        dispatcher.DoEvents();
                    }
                }
                finally
                {
                    window.winRT_MainWindowHandle = 0;
                    window.Close();
                    window.close = false;
                    dispatcher.DoEvents();
                }
            }
            catch (Exception e)
            {
                AppDomainExceptionHandler.LogException(e);
                dispatcherQ.TrySetException(e);
                curW.TrySetException(e);
            }
            finally
            {
                window.Loaded -= loaded;
                window.Content = null;
                window = null;
            }
        }

        private void OnDispatcherUnhandledException(object sender, System.Windows.Threading.DispatcherUnhandledExceptionEventArgs e)
        {
            AppDomainExceptionHandler.LogException(e.Exception);
            // We need to be resiliant to RCW disconnections.
            // This is bound to happen when IXamlTestHooks::Shutdown() is called.
            e.Handled = true;
        }

        internal async Task EnsureWindow()
        {
            await this.currentWindow.Task;
        }

        public ulong MainWindowHandle
        {
            get
            {
                if (this.MainWindow.winRT_MainWindowHandle == 0)
                {
                    throw new InvalidOperationException();
                }
                return this.MainWindow.winRT_MainWindowHandle;
            }
        }

        public void SetWindowSizeOverride(Double width, Double height)
        {
            var mainWindow = this.MainWindow;
            if (mainWindow == null)
            {
                throw new InvalidOperationException("Mainwindow does not exist");
            }

            mainWindow.SetWindowSizeOverride(width, height);
        }

        private WindowsXamlHost XamlHost
        {
            get
            {
                try
                {
                    var window = this.currentWindow.Task.Result;
                    var xamlHost = window.Content as WindowsXamlHost;
                    return xamlHost;
                }
                catch
                {
                    return null;
                }
            }
        }

        internal WUX.FrameworkElement XamlContent
        {
            get
            {
                try
                {
                    var xamlHost = this.XamlHost;
                    if (xamlHost != null)
                    {
                        var xamlContent = xamlHost.Child as WUX.FrameworkElement;
                        return xamlContent;
                    }
                    return null;
                }
                catch
                {
                    return null;
                }
            }
        }

        public object Content
        {
            get
            {
                return this.XamlContent;
            }
            set
            {
                var mainWindow = this.MainWindow;
                if (mainWindow == null)
                {
                    throw new InvalidOperationException("Mainwindow does not exist");
                }

                mainWindow.SetContent((Microsoft.UI.Xaml.UIElement)value);
            }
        }

        private bool ContentLoaded
        {
            get
            {
                var xamlContent = this.XamlContent;
                if (xamlContent != null)
                {
                    return xamlContent.IsLoaded;
                }
                return false;
            }
        }

        internal async Task ExecuteOnUIThreadAsync(Func<Task> action, DispatcherPriority priority = DispatcherPriority.Normal)
        {
            var dispatcher = await this.hostDispatcher.Task;
            await EnsureWindow();
            await dispatcher.InvokeAsync(action, priority);
        }

        internal Dispatcher HostDispatcher
        {
            get
            {
                if (!this.hostDispatcher.Task.IsCompleted)
                {
                    throw new InvalidOperationException("UI Thread has not completed setup");
                }
                var dispatcher = this.hostDispatcher.Task.Result;
                return dispatcher;
            }
        }

        internal WPFTestWindow MainWindow
        {
            get
            {
                if (!this.currentWindow.Task.IsCompleted)
                {
                    throw new InvalidOperationException("UI Thread has not completed setup");
                }
                var window = this.currentWindow.Task.Result;
                return window;
            }
        }

        internal void ExecuteOnUIThread(Action action)
        {
            this.HostDispatcher.Invoke(action);
        }

        internal async Task<object> GetDispatcherQueueInternal()
        {
            await this.hostDispatcher.Task;
            var result = await this.dispatcherQueue.Task;
            return result;
        }

        public IAsyncOperation<object> GetDispatcherQueue()
        {
            return GetDispatcherQueueInternal().AsAsyncOperation();
        }

        public void Reset()
        {
            var mainWindow = this.MainWindow;
            this.ExecuteOnUIThread(() =>
            {
                try
                {
                    mainWindow.SetContent(null);
                }
                catch { }
                mainWindow.close = true;
                this.dispatcherQueue = new TaskCompletionSource<Microsoft.UI.Dispatching.DispatcherQueue>();
                this.currentWindow = new TaskCompletionSource<WPFTestWindow>();
            });
            this.dispatcherQueue.Task.WaitWhileDoingEvents();
            this.currentWindow.Task.WaitWhileDoingEvents();
        }

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal bool IsDisposed { get; private set; }

        private void Dispose(bool disposing)
        {
            if (disposing && !this.IsDisposed)
            {
                this.IsDisposed = true;

                this.noExit = false;
                this.Reset();
                this.uiThread.Join();
            }
        }

        public void GCCollect()
        {
            GC.Collect(2, GCCollectionMode.Forced, true);
            GC.WaitForPendingFinalizers();
        }

        void IWin32Host.DoEvents()
        {
            var dispatcher = this.HostDispatcher;
            dispatcher.DoEvents();
        }
    }
}
