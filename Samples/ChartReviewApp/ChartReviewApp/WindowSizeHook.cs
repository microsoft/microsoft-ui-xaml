// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System;
using System.Linq;
using System.Runtime.InteropServices;

namespace ChartsSample
{
    // Test hook (not for reviewers): --window-size=<width>x<height> sizes the window in physical pixels,
    // beyond the screen if needed, so layouts for larger displays can be captured on a smaller one.
    // Windows caps a window at the screen size through WM_GETMINMAXINFO; this lifts the cap.
    public sealed partial class MainWindow
    {
        private const uint WM_GETMINMAXINFO = 0x0024;
        private SubclassProc _sizeHookProc;
        private int _hookWidth;
        private int _hookHeight;

        [StructLayout(LayoutKind.Sequential)]
        private struct POINT { public int X, Y; }

        [StructLayout(LayoutKind.Sequential)]
        private struct MINMAXINFO { public POINT Reserved, MaxSize, MaxPosition, MinTrackSize, MaxTrackSize; }

        private delegate IntPtr SubclassProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam, UIntPtr id, UIntPtr data);

        [DllImport("comctl32.dll")]
        private static extern bool SetWindowSubclass(IntPtr hWnd, SubclassProc proc, UIntPtr id, UIntPtr data);

        [DllImport("comctl32.dll")]
        private static extern IntPtr DefSubclassProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam);

        [DllImport("user32.dll")]
        private static extern bool SetWindowPos(IntPtr hWnd, IntPtr after, int x, int y, int cx, int cy, uint flags);

        private bool ApplyWindowSizeIfRequested()
        {
            string spec = Environment.GetCommandLineArgs()
                .FirstOrDefault(a => a.StartsWith("--window-size=", StringComparison.OrdinalIgnoreCase))?
                .Substring("--window-size=".Length);
            string[] parts = spec?.Split('x', 'X');
            if (parts == null || parts.Length != 2 ||
                !int.TryParse(parts[0], out _hookWidth) || !int.TryParse(parts[1], out _hookHeight))
            {
                return false;
            }

            IntPtr hwnd = WinRT.Interop.WindowNative.GetWindowHandle(this);
            _sizeHookProc = (h, msg, wParam, lParam, id, data) =>
            {
                IntPtr result = DefSubclassProc(h, msg, wParam, lParam);
                if (msg == WM_GETMINMAXINFO)
                {
                    var info = Marshal.PtrToStructure<MINMAXINFO>(lParam);
                    info.MaxTrackSize.X = Math.Max(info.MaxTrackSize.X, _hookWidth);
                    info.MaxTrackSize.Y = Math.Max(info.MaxTrackSize.Y, _hookHeight);
                    info.MaxSize = info.MaxTrackSize;
                    Marshal.StructureToPtr(info, lParam, false);
                }
                return result;
            };
            SetWindowSubclass(hwnd, _sizeHookProc, (UIntPtr)1, UIntPtr.Zero);
            // SWP_NOZORDER | SWP_NOACTIVATE
            SetWindowPos(hwnd, IntPtr.Zero, 0, 0, _hookWidth, _hookHeight, 0x0004 | 0x0010);
            return true;
        }
    }
}
