// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;
using System.Runtime.InteropServices;
using Windows.Foundation;
using WUC = Windows.UI.Core;

namespace Private.Infrastructure.Hosting.WPF
{
    [ComImport, InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("45D64A29-A63E-4CB6-B498-5781D298CB4F")]
    internal interface ICoreWindowInterop
    {
        IntPtr WindowHandle { get; }
        void MessageHandled(bool value);
    }

    internal static class Interop
    {
        public static ICoreWindowInterop GetInterop(this WUC.CoreWindow @this)
        {
            var unkIntPtr = Marshal.GetIUnknownForObject(@this);
            try
            {
                var interopObj = Marshal.GetTypedObjectForIUnknown(unkIntPtr, typeof(ICoreWindowInterop)) as ICoreWindowInterop;
                return interopObj;
            }
            finally
            {
                 Marshal.Release(unkIntPtr);
                 unkIntPtr = IntPtr.Zero;
            }
        }

        public static IntPtr GetWindowFromWindowId(Microsoft.UI.WindowId windowId)
        {
            IntPtr hwnd = IntPtr.Zero;
            int hresult = Windowing_GetWindowFromWindowId(windowId, out hwnd);
            Exception ex = Marshal.GetExceptionForHR(hresult);
            if (ex != null)
            {
                throw ex;
            }
            return hwnd;
        }

        public static Microsoft.UI.WindowId GetWindowIdFromWindow(IntPtr hwnd)
        {
            Microsoft.UI.WindowId windowId;
            int hresult = Windowing_GetWindowIdFromWindow(hwnd, out windowId);
            Exception ex = Marshal.GetExceptionForHR(hresult);
            if (ex != null)
            {
                throw ex;
            }
            return windowId;
        }

        [DllImport("user32.dll")]
        public static extern bool SetForegroundWindow(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern bool SetFocus(IntPtr hWnd);

        [DllImport("user32.dll")]
        public static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

        [DllImport("user32.dll")]
        public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

        [DllImport("Microsoft.UI.Windowing.Core.dll")]
        public static extern bool ContentPreTranslateMessage(ref global::System.Windows.Interop.MSG message);

        // These pinvokes to FrameworkUdk mimic Microsoft.UI.Interop.h, the header c++ apps use to call GetWindowFromWindowId and GetWindowIdFromWindow.
        [DllImport("Microsoft.Internal.FrameworkUdk.dll")]
        public static extern int Windowing_GetWindowFromWindowId(Microsoft.UI.WindowId windowId, out IntPtr hWnd);

        [DllImport("Microsoft.Internal.FrameworkUdk.dll")]
        public static extern int Windowing_GetWindowIdFromWindow(IntPtr hWnd, out Microsoft.UI.WindowId windowId);
    }
}
