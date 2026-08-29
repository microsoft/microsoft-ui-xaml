// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Windows.Threading;
using WUX = Microsoft.UI.Xaml;
using Microsoft.Toolkit.Wpf.UI.XamlHost;


namespace Private.Infrastructure.Hosting.WPF
{
    internal class XamlSourceLayout
    {
        internal Double? Width = null;
        internal Double? Height = null;
        internal System.Windows.HorizontalAlignment HAlignment;
        internal System.Windows.VerticalAlignment VAlignment;

        internal XamlSourceLayout() { }
        internal XamlSourceLayout(
            Double? width,
            Double? height,
            System.Windows.HorizontalAlignment hAlignment,
            System.Windows.VerticalAlignment vAlignment
        )
        {
            Width = width;
            Height = height;
            HAlignment = hAlignment;
            VAlignment = vAlignment;
        }
    }

    internal class WPFTestWindow : Window
    {
        internal bool close;
        internal ulong winRT_MainWindowHandle;
        private XamlSourceLayout originalXamlSourceLayout;
        private XamlSourceLayout updatedXamlLayout;

        internal WPFTestWindow()
        {
            Title = "TAEF Tailored Application Host Process"; // Must be this value so that infra\server\WindowHelper.cpp:FindTopLevelForegroundWindow is able to find the window
            Background = System.Windows.Media.Brushes.DarkSlateBlue;
            WindowStartupLocation = WindowStartupLocation.CenterScreen;
            WindowState = WindowState.Maximized;
            WindowStyle = WindowStyle.SingleBorderWindow;
            close = false;
        }

        internal IntPtr CurrentMainWindowHandle
        {
            get
            {
                var wpfInterop = new WindowInteropHelper(this);
                var mainWndHandle = wpfInterop.Handle;
                if (mainWndHandle == IntPtr.Zero)
                {
                    throw new InvalidOperationException();
                }
                return mainWndHandle;
            }
        }

        internal void SetContent(WUX.UIElement content)
        {
            if (content != null)
            {
                var xamlSource = this.Content as WindowsXamlHost;
                if (xamlSource == null)
                {
                    xamlSource = new WindowsXamlHost();
                    this.Content = xamlSource;
                    SetXamlLayout(null);
                }
                Dispatcher.CurrentDispatcher.DoEvents();
                var xamlHostLoaded = xamlSource.AwaitLoaded();

                // Set focus to the DesktopWindowXamlSource here to ensure keyboard input goes to XAML.
                xamlSource.SetFocus();
                xamlHostLoaded.WaitWhileDoingEvents();

                xamlSource.Child = content;
            }
            else
            {
                var xamlSource = this.Content as WindowsXamlHost;
                if (xamlSource != null)
                {
                    var xamlHostUnloaded = xamlSource.AwaitUnloaded();
                    try
                    {
                        xamlSource.Child = null;
                    }
                    catch
                    {
                    }
                    RestoreXamlLayout();                    
                    
                    // The order of these calls is important. Calling xamlSource.Dispose here will close the
                    // DesktopWindowXamlSource, and setting "this.Content = null" will destroy the ContentIsland's
                    // parent HWND. It's important to Dispose the DesktopWindowXamlSource first, because the stack isn't
                    // yet resilient to the HWNDs suddenly being closed.
                    // TODO: Close DesktopWindowXamlSource automatically when the ContentIsland's HWND closes
                    xamlSource.Dispose();
                    this.Content = null;

                    Dispatcher.CurrentDispatcher.DoEvents();

                    xamlHostUnloaded.WaitWhileDoingEvents();
                }
            }
        }

        internal void SetWindowSizeOverride(Double width, Double height)
         {
            if (width == 0 && height == 0)
            {
                RestoreXamlLayout();
                return;
            }

            // Remember original window layout
            StoreOriginalXamlLayout();

            // Set new window dimensions
            SetXamlLayout(new XamlSourceLayout(
                width,
                height,
                System.Windows.HorizontalAlignment.Left,
                System.Windows.VerticalAlignment.Top));
        }

        private void StoreOriginalXamlLayout()
        {
            if (originalXamlSourceLayout == null)
            {
                var xamlSource = this.Content as WindowsXamlHost;
                if (xamlSource != null)
                {
                    originalXamlSourceLayout = new XamlSourceLayout(
                        xamlSource.ActualWidth,
                        xamlSource.ActualHeight,
                        xamlSource.HorizontalAlignment,
                        xamlSource.VerticalAlignment
                    );
                }
            }
        }

        private void SetXamlLayout(XamlSourceLayout newXamlLayout)
        {
            if (this.Content == null)
            {
                this.updatedXamlLayout = newXamlLayout;
            }
            else
            {
                if(newXamlLayout == null)
                {
                    newXamlLayout = this.updatedXamlLayout;
                }
                SetXamlLayoutInternal(newXamlLayout);
            }
        }

        private void SetXamlLayoutInternal(XamlSourceLayout newXamlLayout)
        {
            var xamlSource = this.Content as WindowsXamlHost;
            if (newXamlLayout != null && xamlSource != null)
            {
                xamlSource.Width = (Double)newXamlLayout.Width;
                xamlSource.Height = (Double)newXamlLayout.Height;
                xamlSource.HorizontalAlignment = newXamlLayout.HAlignment;
                xamlSource.VerticalAlignment = newXamlLayout.VAlignment;
            }
        }

        private void RestoreXamlLayout()
        {
            SetXamlLayoutInternal(this.originalXamlSourceLayout);
        }
    }
}
