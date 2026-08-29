// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices;
using System.Windows.Interop;
using Microsoft.Toolkit.Win32.UI.XamlHost;
// CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
// I had to scope the namespace with variables names due to collisions
using WUX = Microsoft.UI.Xaml;
using VKEYS = Windows.System;
using WinRT;
// CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION
namespace Microsoft.Toolkit.Wpf.UI.XamlHost
{
    /// <summary>
    /// WindowsXamlHost control hosts UWP XAML content inside the Windows Presentation Foundation
    /// </summary>
    abstract partial class WindowsXamlHostBase : HwndHost
    {
        /// <summary>
        /// UWP XAML Application instance and root UWP XamlMetadataProvider.  Custom implementation required to
        /// probe at runtime for custom UWP XAML type information.  This must be created before
        /// creating any DesktopWindowXamlSource instances if custom UWP XAML types are required.
        /// </summary>
        private readonly WUX.Application _application;

        /// <summary>
        /// UWP XAML DesktopWindowXamlSource instance that hosts XAML content in a win32 application
        /// </summary>
        private readonly WUX.Hosting.DesktopWindowXamlSource _xamlSource;

        /// <summary>
        /// Private field that backs ChildInternal property.
        /// </summary>
        private WUX.UIElement _childInternal;

        /// <summary>
        ///     Fired when WindowsXamlHost root UWP XAML content has been updated
        /// </summary>
        public event EventHandler ChildChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsXamlHostBase"/> class.
        /// </summary>
        /// <remarks>
        /// Default constructor is required for use in WPF markup. When the default constructor is called,
        /// object properties have not been set. Put WPF logic in OnInitialized.
        /// </remarks>
        public WindowsXamlHostBase()
        {
            // Microsoft.UI.Xaml.Application object is required for loading custom control metadata.  If a custom
            // Application object is not provided by the application, the host control will create one (XamlApplication).
            // Instantiation of the application object must occur before creating the DesktopWindowXamlSource instance.
            // If no Application object is created before DesktopWindowXamlSource is created, DestkopWindowXamlSource
            // will create a generic Application object unable to load custom UWP XAML metadata.
            XamlApplication.GetOrCreateXamlApplicationInstance(ref _application);

            // Create DesktopWindowXamlSource, host for UWP XAML content
            _xamlSource = new WUX.Hosting.DesktopWindowXamlSource();

            // Hook DesktopWindowXamlSource OnTakeFocus event for Focus processing
            _xamlSource.TakeFocusRequested += OnTakeFocusRequested;
        }

        /// <summary>
        /// Gets or sets the root UWP XAML element displayed in the WPF control instance.
        /// </summary>
        /// <value>The <see cref="Microsoft.UI.Xaml.UIElement"/> child.</value>
        /// <remarks>This UWP XAML element is the root element of the wrapped <see cref="Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource" />.</remarks>
        protected WUX.UIElement ChildInternal
        {
            get
            {
                return _childInternal;
            }

            set
            {
                if (value == ChildInternal)
                {
                    return;
                }

                var currentRoot = (WUX.FrameworkElement)ChildInternal;
                if (currentRoot != null)
                {
                    currentRoot.SizeChanged -= XamlContentSizeChanged;
                }

                _childInternal = value;
                SetContent();

                var frameworkElement = ChildInternal as WUX.FrameworkElement;
                if (frameworkElement != null)
                {
                    // If XAML content has changed, check XAML size
                    // to determine if WindowsXamlHost needs to re-run layout.
                    frameworkElement.SizeChanged += XamlContentSizeChanged;

                    // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
                    // This was causing many tests to fail in catgates due to output file missmatch.
                    // WindowsXamlHost DataContext should flow through to UWP XAML content
                    // frameworkElement.DataContext = DataContext;
                    // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION
                }

                // Fire updated event
                ChildChanged?.Invoke(this, new EventArgs());
            }
        }

        /// <summary>
        /// Exposes ChildInternal without exposing its actual Type.
        /// </summary>
        /// <returns>the underlying UWP child object</returns>
        public object GetUwpInternalObject()
        {
            return ChildInternal;
        }

        /// <summary>
        /// Gets or sets a value indicating whether this wrapper control instance been disposed
        /// </summary>
        ///
        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
        // Changed to public and setter to private
        public bool IsDisposed { get; private set; }

        [ThreadStatic]
        static private int tls_subscriptionsToThreadFilterMessage = 0;
        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

        /// <summary>
        /// Creates <see cref="Microsoft.UI.Xaml.Application" /> object, wrapped <see cref="Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource" /> instance; creates and
        /// sets root UWP XAML element on <see cref="Microsoft.UI.Xaml.Hosting.DesktopWindowXamlSource" />.
        /// </summary>
        /// <param name="hwndParent">Parent window handle</param>
        /// <returns>Handle to XAML window</returns>
        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
            // This should NOT be copied back to the GitHub copy of this file
            // Ensure that exactly once per thread we subscribe to ThreadFilterMessage so that each thread's main
            // message pump is calling ContentPreTranslateMessage exactly once for each window message.  Calling it more
            // than once will result in double messages.
            if (tls_subscriptionsToThreadFilterMessage == 0)
            {
                ComponentDispatcher.ThreadFilterMessage += this.OnThreadFilterMessage;
            }                 
            ++tls_subscriptionsToThreadFilterMessage;  
            // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

            // 'EnableMouseInPointer' is called by the WindowsXamlManager during initialization. No need
            // to call it directly here.

            _xamlSource.Initialize(Private.Infrastructure.Hosting.WPF.Interop.GetWindowIdFromWindow(hwndParent.Handle));
            var windowHandle = Private.Infrastructure.Hosting.WPF.Interop.GetWindowFromWindowId(_xamlSource.SiteBridge.WindowId);

            // Overridden function must return window handle of new target window (DesktopWindowXamlSource's Window)
            return new HandleRef(this, windowHandle);
        }

        /// <summary>
        /// The default implementation of SetContent applies ChildInternal to desktopWindowXamSource.Content.
        /// Override this method if that shouldn't be the case.
        /// For example, override if your control should be a child of another WindowsXamlHostBase-based control.
        /// </summary>
        protected virtual void SetContent()
        {
            if (_xamlSource != null)
            {
                _xamlSource.Content = _childInternal;
            }
        }

        /// <summary>
        /// WPF framework request to destroy control window.  Cleans up the HwndIslandSite created by DesktopWindowXamlSource
        /// </summary>
        /// <param name="hwnd">Handle of window to be destroyed</param>
        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            Dispose(true);
        }

        public void SetFocus()
        {
            IntPtr windowHandle = Private.Infrastructure.Hosting.WPF.Interop.GetWindowFromWindowId(_xamlSource.SiteBridge.WindowId);
            Private.Infrastructure.Hosting.WPF.Interop.SetFocus(windowHandle);
        }

        /// <summary>
        /// WindowsXamlHost Dispose
        /// </summary>
        /// <param name="disposing">Is disposing?</param>
        ///
        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
        protected override void Dispose(bool disposing)
        {
            if (disposing && !this.IsDisposed)
            {
                // Free any other managed objects here.
                //

                // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
                // This should NOT be copied back to the GitHub copy of this file
                --tls_subscriptionsToThreadFilterMessage;
                if (tls_subscriptionsToThreadFilterMessage == 0)
                {
                    ComponentDispatcher.ThreadFilterMessage -= this.OnThreadFilterMessage;
                }
                // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

                ChildInternal = null;
                // Required by CA2213: _xamlSource?.Dispose() is insufficient.
                if (_xamlSource != null)
                {
                    _xamlSource.TakeFocusRequested -= OnTakeFocusRequested;
                }
            }

            // Free any unmanaged objects here.
            //
            if (_xamlSource != null && !this.IsDisposed)
            {
                _xamlSource.Dispose();
            }

            // TODO: CoreInputSink cleanup is failing when explicitly disposing
            // WindowsXamlManager.  Add dispose call back when that bug is fixed.
            this.IsDisposed = true;

            // Call base class implementation.
            base.Dispose(disposing);
        }
        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION

        // CHANGES BEGIN : CODE IS DIFFERENT FROM GITHUB VERSION
        // Moved this method here from WindowsXamlHost.
        protected override global::System.IntPtr WndProc(global::System.IntPtr hwnd, int msg, global::System.IntPtr wParam, global::System.IntPtr lParam, ref bool handled)
        {
            const int WM_GETOBJECT = 0x003D;
            switch (msg)
            {
                // We don't want HwndHost to handle the WM_GETOBJECT.
                // Instead we want to let the HwndIslandSite's WndProc get it
                // So return handled = false and don't let the base class do
                // anything on that message.
                case WM_GETOBJECT:
                    handled = false;
                    return global::System.IntPtr.Zero;
            }

            return base.WndProc(hwnd, msg, wParam, lParam, ref handled);
        }
        // CHANGES END : CODE IS DIFFERENT FROM GITHUB VERSION
    }
}