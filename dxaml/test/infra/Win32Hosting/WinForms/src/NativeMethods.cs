// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MS.Win32
{
    using System;
    using System.Runtime.ConstrainedExecution;
    using System.Runtime.InteropServices;
    using System.Security;
    using System.Security.Permissions;
    using System.Runtime.Versioning;
    using System.ComponentModel;
    using Microsoft.Windows.Interop;

    internal static class NativeMethods
    {
        public static IntPtr HWND_TOP = IntPtr.Zero;
        public static IntPtr HWND_TOPMOST = IntPtr.Zero-1;

        public const int WM_MOVE = 0x0003;
        public const int WM_SIZE = 0x0005;
        public const int WM_ACTIVATE = 0x0006;
        public const int WM_SETFOCUS = 0x0007;
        public const int WM_PAINT = 0x000F;
        public const int WM_ERASEBKGND = 0x14; // 20
        public const int WM_WINDOWPOSCHANGING = 0x0046;
        public const int WM_WINDOWPOSCHANGED = 0x0047;
        public const int WM_PARENTNOTIFY = 0x0210;
        public const int WM_SETREDRAW = 0x000B;
        public const int WM_NCPAINT = 0x85; // 133
        public const int WM_POINTERDOWN = 0x0246;
        public const int WM_LBUTTONDOWN = 0x0201;
        public const int WM_MOUSEACTIVATE = 0x0021;
        public const int WM_USER = 0x0400;
        public const int WM_REFLECT = WM_USER + 0x1C00;

        public const int WM_KEYDOWN = 0x0100;
        public const int WM_KEYUP = 0x0101;
        public const int WM_CHAR = 0x0102;
        public const int WM_SYSKEYDOWN = 0x0104;
        public const int WM_SYSKEYUP = 0x0105;

        /// <summary>
        /// SetWindowPos Flags
        /// </summary>
        internal static class SetWindowPosFlags
        {
            public static readonly int
            NOSIZE = 0x0001,
            NOMOVE = 0x0002,
            NOZORDER = 0x0004,
            NOREDRAW = 0x0008,
            NOACTIVATE = 0x0010,
            DRAWFRAME = 0x0020,
            FRAMECHANGED = 0x0020,
            SHOWWINDOW = 0x0040,
            HIDEWINDOW = 0x0080,
            NOCOPYBITS = 0x0100,
            NOOWNERZORDER = 0x0200,
            NOREPOSITION = 0x0200,
            NOSENDCHANGING = 0x0400,
            DEFERERASE = 0x2000,
            ASYNCWINDOWPOS = 0x4000;
        }

        internal enum ActivateParams
        {
            WA_INACTIVE = 0,
            WA_ACTIVE = 1,
            WA_CLICKACTIVE = 2
        }

        class Utils
        {
            internal static int LOWORD(IntPtr ptr)
            {
                return (int)((long)ptr & 0xffff);
            }

            internal static int HIWORD(int n)
            {
                return (n >> 16) & 0xffff;
            }
        }
    }

    internal static class UnsafeNativeMethods
    {
        [DllImport("User32", ExactSpelling = true, CharSet = CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        public static extern IntPtr GetFocus();

        [DllImport("User32", CharSet = CharSet.Auto, SetLastError = true)]
        [ResourceExposure(ResourceScope.None)]
        internal static extern IntPtr SendMessage(HandleRef hWnd, int msg, IntPtr wParam, IntPtr lParam);

        /// <SecurityNote>
        ///    Critical: This code calls into unmanaged code which elevates
        /// </SecurityNote>
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [DllImport("user32.dll", EntryPoint = "EnableWindow", SetLastError = true, ExactSpelling = true, CharSet = System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IntEnableWindow(HandleRef hWnd, bool enable);

        /// <SecurityNote>
        ///    Critical: This code calls into unmanaged code which elevates
        /// </SecurityNote>
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [DllImport("user32.dll", EntryPoint = "EnableWindow", CharSet = System.Runtime.InteropServices.CharSet.Auto)]
        public static extern bool IntEnableWindowNoThrow(HandleRef hWnd, bool enable);

        /// <SecurityNote>
        ///     Critical: This code causes elevation to unmanaged code
        /// </SecurityNote>
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [DllImport("kernel32.dll", ExactSpelling = true, CharSet = CharSet.Auto)]
        internal static extern void SetLastError(int dwErrorCode);

        /// <SecurityNote>
        ///    Critical: This code calls into unmanaged code which elevates
        /// </SecurityNote>
        [SecurityCritical]
        public static bool EnableWindow(HandleRef hWnd, bool enable)
        {
            UnsafeNativeMethods.SetLastError(0);

            bool result = IntEnableWindow(hWnd, enable);
            if (!result)
            {
                int win32Err = Marshal.GetLastWin32Error();
                if (win32Err != 0)
                {
                    throw new Win32Exception(win32Err);
                }
            }

            return result;
        }

        /// <SecurityNote>
        /// Critical: It calls methods with SuppressUnmanagedCodeSecurity attribute, Win32 call
        /// </SecurityNote>
        [SecurityCritical, SuppressUnmanagedCodeSecurity]
        [DllImport("user32.dll", EntryPoint = "GetParent", ExactSpelling = true, CharSet = CharSet.Auto, SetLastError = true)]
        //public static extern IntPtr IntGetParent(HandleRef hWnd);
        public static extern IntPtr IntGetParent(IntPtr hWnd);

        /// <SecurityNote>
        ///  SecurityCritical: This code happens to return a critical resource and causes unmanaged code elevation
        /// </SecurityNote>
        [SecurityCritical]
        [SuppressUnmanagedCodeSecurity]
        [DllImport("user32.dll", EntryPoint = "SetFocus", ExactSpelling = true, CharSet = CharSet.Auto, SetLastError = true)]
        public static extern IntPtr IntSetFocus(IntPtr hWnd);

        public static IntPtr GetWindowLongPtr(IntPtr hWnd, int nIndex)
        {
            if (IntPtr.Size == 8)
                return GetWindowLongPtr64(hWnd, nIndex);
            else
                return new IntPtr(GetWindowLong32(hWnd, nIndex));
        }

        [DllImport("user32.dll", EntryPoint = "GetWindowLong")]
        private static extern int GetWindowLong32(IntPtr hWnd, int nIndex);

        [DllImport("user32.dll", EntryPoint = "GetWindowLongPtr")]
        private static extern IntPtr GetWindowLongPtr64(IntPtr hWnd, int nIndex);

        public static IntPtr SetWindowLongPtr(IntPtr hWnd, int nIndex, IntPtr dwNewLong)
        {
            if (IntPtr.Size == 8)
                return SetWindowLongPtr64(hWnd, nIndex, dwNewLong);
            else
                return new IntPtr(SetWindowLong32(hWnd, nIndex, dwNewLong.ToInt32()));
        }

        [DllImport("user32.dll", EntryPoint = "SetWindowLong")]
        private static extern int SetWindowLong32(IntPtr hWnd, int nIndex, int dwNewLong);

        [DllImport("user32.dll", EntryPoint = "SetWindowLongPtr")]
        private static extern IntPtr SetWindowLongPtr64(IntPtr hWnd, int nIndex, IntPtr dwNewLong);

    } // End Unsafe methods

    internal static class SafeNativeMethods
    {

        [DllImport("User32", ExactSpelling = true, CharSet = CharSet.Auto)]
        [ResourceExposure(ResourceScope.None)]
        internal static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter,
            int x, int y, int cx, int cy, int flags);
    }
}
