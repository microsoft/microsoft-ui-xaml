// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public static class TestHelpers
    {
        public static class NativeMethods
        {
            [DllImport("dwmapi.dll", SetLastError = true)]
            public static extern int DwmGetUnmetTabRequirements(IntPtr hwnd, out int requirements);
            
            [DllImport("user32.dll", SetLastError = true)]
            [return: MarshalAs(UnmanagedType.Bool)]
            public static extern bool GetWindowPlacement(IntPtr hWnd, ref Windowplacement lpwndpl);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern IntPtr PostMessage(IntPtr hWnd, int msg, int wParam, IntPtr lParam);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern IntPtr GetMenu (IntPtr hwnd);
            
            [DllImport("user32.dll", SetLastError = true)]
            public static extern IntPtr GetSystemMenu(IntPtr hWnd, bool bRevert);

            [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
            public static extern IntPtr FindWindowEx(IntPtr hwndParent, IntPtr hwndChild, string lpszClass, string lpszWindow);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern int GetMenuState(IntPtr hmenu, int uIDCheckItem, int uCheck);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern int GetWindowRect(IntPtr hWnd, ref RECT rectangle);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern int SetProcessDpiAwarenessContext([In] UIntPtr value);

            [DllImport("user32.dll", SetLastError = true)]
            public static extern bool GetCursorPos(out global::System.Drawing.Point lpPoint);

            [DllImport("api-ms-win-ntuser-ie-window-l1-1-0.dll", SetLastError = true)]
            public static extern IntPtr GetForegroundWindow();
            
            [DllImport("user32.dll", SetLastError = true)]
            public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, int uFlags);
           
            public struct Windowplacement
            {
                public int length;
                public int flags;
                public int showCmd;
                public global::System.Drawing.Point ptMinPosition;
                public global::System.Drawing.Point ptMaxPosition;
                public global::System.Drawing.Rectangle rcNormalPosition;
            }

            public enum ShowWindowCmd : int
            {
                SW_HIDE = 0,
                SW_SHOWNORMAL = 1,
                SW_SHOWMINIMIZED = 2,
                SW_SHOWMAXIMIZED = 3,
                SW_SHOWNOACTIVATE = 4,
                SW_SHOW = 5,
                SW_MINIMIZE = 6,
                SW_SHOWMINNOACTIVE = 7,
                SW_SHOWNA = 8,
                SW_RESTORE = 9,
                SW_SHOWDEFAULT = 10,
                SW_FORCEMINIMIZE = 11
            }
            
            [StructLayout(LayoutKind.Sequential)]
            public struct RECT
            {
                public int Left;        // x position of upper-left corner
                public int Top;         // y position of upper-left corner
                public int Right;       // x position of lower-right corner
                public int Bottom;      // y position of lower-right corner
            }

            public const int WM_SYSCOMMAND  = 0x0112;
            public const int SC_MINIMIZE            = 0xF020;
            public const int SC_MAXIMIZE            = 0xF030;
            public const int SC_CLOSE               = 0xF060;
            public const int SC_RESTORE             = 0xF120;

            public const int MF_BYCOMMAND = 0x0;
            public const int MF_BYPOSITION = 0x400;

            public const int MF_ENABLED = 0x0;
            public const int MF_DISABLED = 0x2;
            public const int MF_GRAYED = 0x1;

            public const string CLASSID_CONTEXTMENU = "#32768"; // #32678 is class for popup menus and system menu is one of them
            public const string SYSTEMENUITEM_RESTORE = "61728";
            public const string SYSTEMENUITEM_MOVE = "61456";
            public const string SYSTEMENUITEM_SIZE = "61440";
            public const string SYSTEMENUITEM_MINIMIZE = "61472";
            public const string SYSTEMENUITEM_MAXIMIZE = "61488";
            public const string SYSTEMENUITEM_CLOSE = "61536";

            public const int SWP_ASYNCWINDOWPOS = 0x4000;
            public const int SWP_DEFERERASE = 0x2000;
            public const int SWP_DRAWFRAME = 0x0020;
            public const int SWP_FRAMECHANGED = 0x0020;
            public const int SWP_HIDEWINDOW = 0x0080;
            public const int SWP_NOACTIVATE = 0x0010;
            public const int SWP_NOCOPYBITS = 0x0100;
            public const int SWP_NOMOVE = 0x0002;
            public const int SWP_NOOWNERZORDER = 0x0200;
            public const int SWP_NOREDRAW = 0x0008;
            public const int SWP_NOREPOSITION = 0x0200;
            public const int SWP_NOSENDCHANGING = 0x0400;
            public const int SWP_NOSIZE = 0x0001;
            public const int SWP_NOZORDER = 0x0004;
            public const int SWP_SHOWWINDOW = 0x0040;
        }

        
        // these are approximations, actual value will include dpi calculations too
        public const int CAPTION_BUTTON_WIDTH = 46;
        public const int CAPTION_BUTTON_HEIGHT = 32;

        public enum CaptionButtons : int
        {
            Minimize,
            Maximize,
            Close
        }
    
        public static bool SystemTabbedShellIsEnabled
        {
            get
            {
                if (!s_systemTabbedShellIsEnabledCached)
                {
                    int requirements = 0;

                    try
                    {
                        NativeMethods.DwmGetUnmetTabRequirements(IntPtr.Zero, out requirements);
                        if (requirements == 0) // 0 = DWMTWR_NONE
                        {
                            s_systemTabbedShellIsEnabled = true;
                        }
                    }
                    catch(MissingMethodException)
                    {
                        Log.Error("DwmGetUnmetTabRequirements not available on this platform");
                    }

                    s_systemTabbedShellIsEnabledCached = true;
                }

                return s_systemTabbedShellIsEnabled;
            }
        }

        static bool s_systemTabbedShellIsEnabledCached;
        static bool s_systemTabbedShellIsEnabled;

        public static bool IsWindowMinimized()
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.Windowplacement placement = new NativeMethods.Windowplacement();
            NativeMethods.GetWindowPlacement(hwnd, ref placement);
            return placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_SHOWMINIMIZED || placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_MINIMIZE;
        }

        public static bool IsWindowMaximized()
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.Windowplacement placement = new NativeMethods.Windowplacement();
            NativeMethods.GetWindowPlacement(hwnd, ref placement);
            return placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_SHOWMAXIMIZED;
        }

        // if this restore is expected to happen from a minimize from a window which was earlier maximized, 
        // after restore, the window will be maximized again
        public static bool IsWindowRestored(bool WindowStateMaximized=false)
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.Windowplacement placement = new NativeMethods.Windowplacement();
            NativeMethods.GetWindowPlacement(hwnd, ref placement);
            bool windowInMaximizedState = WindowStateMaximized && (placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_SHOWMAXIMIZED);
            return placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_SHOWNORMAL || placement.showCmd == (int)NativeMethods.ShowWindowCmd.SW_RESTORE || windowInMaximizedState;
        }

        public static bool IsWindowClosed(int processID)
        {
            bool closedProcess = false;
            try
            {
                var p = Process.GetProcessById(processID);
                closedProcess = p.HasExited;
            }
            catch (ArgumentException)
            {
                closedProcess = true;
            }
            return closedProcess;
        }

        public static void MinimizeWindow()
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.PostMessage(hwnd, NativeMethods.WM_SYSCOMMAND, NativeMethods.SC_MINIMIZE, IntPtr.Zero);
            Wait.ForIdle();
            Verify.IsTrue(IsWindowMinimized(), "Window minimized");
        }

        public static void TryCaptionButtonLeftClick(CaptionButtons type, bool skipWait = false) 
        {
            UIObject root = TestEnvironment.Application.ApplicationFrameWindow ?? TestEnvironment.Application.CoreWindow;
            var windowDimensionsRect = root.BoundingRectangle;
            double midx = 0.0f;
            // midpoint x coordinate of a caption button offsetted from right side of a window
            if (type == CaptionButtons.Minimize)
            {
                midx = windowDimensionsRect.Width - (CAPTION_BUTTON_WIDTH * 2.5 );
            }
            else if (type == CaptionButtons.Maximize)
            {
                midx = windowDimensionsRect.Width - (CAPTION_BUTTON_WIDTH * 1.5 );
            }
            else
            {   //close
                midx = windowDimensionsRect.Width - (CAPTION_BUTTON_WIDTH * 0.5 );
            }
            double midy = CAPTION_BUTTON_HEIGHT / 2;
            
            InputHelper.LeftClick(root, midx, midy, skipWait);
        }
        

        // if restored window is a maximized window, set WindowStateMaximized to true
        public static void RestoreWindow(bool WindowStateMaximized=false)
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.PostMessage(hwnd, NativeMethods.WM_SYSCOMMAND, NativeMethods.SC_RESTORE, IntPtr.Zero);
            Wait.ForIdle();
            Verify.IsTrue(IsWindowRestored(WindowStateMaximized), "Window restored");
        }

        public static void MaximizeWindow()
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.PostMessage(hwnd, NativeMethods.WM_SYSCOMMAND, NativeMethods.SC_MAXIMIZE, IntPtr.Zero);
            Wait.ForIdle();
            Verify.IsTrue(IsWindowMaximized(), "Window maximized");
        }

        public static void MoveWindow(int left, int top) 
        {
            IntPtr hwnd = TestEnvironment.Application.Hwnd;
            NativeMethods.SetWindowPos(hwnd, IntPtr.Zero, left, top, 0, 0, NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOZORDER | NativeMethods.SWP_NOACTIVATE | NativeMethods.SWP_NOOWNERZORDER);
        }
    }
}
