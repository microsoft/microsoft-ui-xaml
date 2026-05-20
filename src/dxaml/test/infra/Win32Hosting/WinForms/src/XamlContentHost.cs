// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Linq;

using MS.Win32;
using Xaml = Microsoft.UI.Xaml;
using WindowsUI = Windows.UI;
using Microsoft.Toolkit.Win32.UI.XamlHost;


namespace Microsoft.Windows.Interop
{

    /// <summary>
    ///     A Windows Forms control that hosts XAML content
    /// </summary>
    [global::System.ComponentModel.DesignerCategory("code")]
    partial class XamlContentHost : Control
    {
        /// <summary>
        ///    HwndIslandSite window handle associated with this Control instance
        /// </summary>
        private IntPtr hwndIslandSite;

        /// <summary>
        ///    Last preferredSize returned by XAML during WinForms layout pass
        /// </summary>
        private Size lastXamlContentPreferredSize = new Size();

        /// <summary>
        ///    XamlIslandRoot instance
        /// </summary>
        private readonly Xaml.Hosting.DesktopWindowXamlSource xamlBridge;

        /// <summary>
        ///     Initializes a new instance of the XamlContentHost class.
        /// </summary>
        [PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
        public XamlContentHost()
            : base()
        {
            // Set trace logging levels
            traceSource.Switch.Level = SourceLevels.Error;
#if DEBUG
            traceSource.Switch.Level = SourceLevels.Verbose;
#endif
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "XamlContentHost constructor start.");

            this.xamlBridge = new Xaml.Hosting.DesktopWindowXamlSource();
            this.xamlBridge.TakeFocusRequested += XamlBridge_TakeFocusRequested;
            this.SetStyle(ControlStyles.ContainerControl, true);
            this.SetStyle(ControlStyles.Selectable, true);

            // Respond to size changes on this Control
            this.SizeChanged += this.XamlContentHost_SizeChanged;

            traceSource.TraceEvent(TraceEventType.Verbose, 0, "XamlContentHost constructor end.");
        }

        /// <summary>
        ///     Fired when XAML content has been updated and finishes loading
        /// </summary>
        public event EventHandler<XamlContentUpdatedEventArgs> XamlContentUpdated;

        /// <summary>
        /// AutoSize determines whether the Control dynamically sizes to its content
        /// </summary>
        [ReadOnly(false)]
        [Browsable(true)]
        [DefaultValue(false)]
        [Category("Layout")]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
        public new bool AutoSize
        {
            get
            {
                return base.AutoSize;
            }

            set
            {
                base.AutoSize = value;
            }
        }

        /// <summary>
        /// AutoSizeMode determines whether the Control dynamically sizes to its content
        /// </summary>
        [ReadOnly(false)]
        [Browsable(true)]
        [Category("Layout")]
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible)]
        public AutoSizeMode AutoSizeMode
        {
            get
            {
                return GetAutoSizeMode();
            }

            set
            {
                SetAutoSizeMode(value);
            }
        }

        /// <summary>
        ///    Gets or sets XAML content for XamlContentHost
        /// </summary>
        [Category("XAML")]
        [Browsable(false)]
        public Xaml.UIElement Content
        {
            get
            {
                return this.xamlBridge.Content;
            }

            set
            {
                traceSource.TraceEvent(TraceEventType.Verbose, 0, "Setting Content...");

                if (!DesignMode && value != null)
                {
                    this.xamlBridge.Content = value;

                    var frameworkElement = this.xamlBridge.Content as Xaml.FrameworkElement;
                    Debug.Assert(frameworkElement != null, "XamlContentHost.Content: Root Xaml element is NULL. Content set failed.");

                    // If XAML content has changed, check XAML size and XamlContentHost.AutoSize
                    // setting to determine if XamlContentHost needs to re-run layout.
                    frameworkElement.LayoutUpdated += this.FrameworkElement_LayoutUpdated;

                    this.PerformLayout();
                }
            }
        }

        /// <summary>
        /// Raises the HandleCreated event.  XAML content creation should be delayed
        /// until after host handle creation occurs.  (Use loaded event on the host control.)
        /// </summary>
        /// <param name="e">EventArgs</param>
        protected override void OnHandleCreated(EventArgs e)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "OnHandleCreated");

            if (!DesignMode)
            {
                var parentHandle = this.Handle;
                _xamlSource.Initialize(Private.Infrastructure.Hosting.WPF.Interop.GetWindowIdFromWindow(parentHandle));
                this.hwndIslandSite = Private.Infrastructure.Hosting.WPF.Interop.GetWindowFromWindowId(_xamlSource.SiteBridge.WindowId);

                const int GWL_EXSTYLE = -20;
                const long WS_EX_CONTROLPARENT = 0x00010000L;

                var st = UnsafeNativeMethods.GetWindowLongPtr(this.hwndIslandSite, GWL_EXSTYLE);
                long style = st.ToInt64();
                if ((style & WS_EX_CONTROLPARENT) == 0)
                {
                    style |= WS_EX_CONTROLPARENT;
                    UnsafeNativeMethods.SetWindowLongPtr(this.hwndIslandSite, GWL_EXSTYLE, new IntPtr(style));
                }

                SafeNativeMethods.SetWindowPos(
                    (IntPtr)hwndIslandSite,
                    NativeMethods.HWND_TOP,
                    0, 0,
                    this.Width, this.Height, NativeMethods.SetWindowPosFlags.SHOWWINDOW);
            }

            base.OnHandleCreated(e);
        }

        /// <summary>
        /// Passes a msg sent to the HwndSource into the XamlContentHost for preprocessing.
        /// </summary>
        /// <param name="msg">Window Message</param>
        /// <param name="wParam">Window Message wParam</param>
        /// <param name="lParam">Window Message lParam</param>
        private void OnHwndSourceMsgNotifyXamlContentHost(int msg, IntPtr wParam, IntPtr lParam)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "OnHwndSourceMsgNotifyXamlContentHost");

            UnsafeNativeMethods.SendMessage(new HandleRef(this, this.Handle), msg, wParam, lParam);
        }

        /// <summary>
        /// Cleanup hosted XAML content
        /// </summary>
        /// <param name="disposing">IsDisposing?</param>
        protected override void Dispose(bool disposing)
        {
            traceSource.TraceEvent(TraceEventType.Verbose, 0, "Dispose");

            if (disposing)
            {
                this.xamlBridge?.Dispose();
            }

            base.Dispose(disposing);
        }
    }
}
