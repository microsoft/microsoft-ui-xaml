using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace WinUISnoopApp
{
    internal class HwndHelpers
    {
        public static Process GetProcessAtCursorPos()
        {
            System.Drawing.Point cursorPos;
            if (GetCursorPos(out cursorPos))
            {
                var targetHwnd = WindowFromPoint(cursorPos);
                int targetPid = 0;
                GetWindowThreadProcessId(targetHwnd, out targetPid);
                var targetProcess = Process.GetProcessById(targetPid);
                return targetProcess;
            }
            return null;
        }

        public static IntPtr FindMainWindowHandle()
        {
            IntPtr appHwnd = IntPtr.Zero;
            EnumWindows(delegate (IntPtr hwnd, IntPtr param)
            {
                int pid = 0;
                GetWindowThreadProcessId(hwnd, out pid);
                if (pid == System.Diagnostics.Process.GetCurrentProcess().Id)
                {
                    appHwnd = hwnd;
                    return false;
                }

                return true; // keep looking
            }, IntPtr.Zero);
            return appHwnd;
        }


        #region pInvokes
        [DllImport("user32.dll", SetLastError = true)]
        static extern uint GetWindowThreadProcessId(IntPtr hWnd, out int lpdwProcessId);

        [DllImport("user32.dll")]
        static extern IntPtr WindowFromPoint(System.Drawing.Point p);
        [DllImport("user32.dll")]
        static extern bool GetCursorPos(out System.Drawing.Point lpPoint);

        [DllImport("user32.dll")]
        static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
        public delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);

        [DllImport("user32.dll")]
        public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
        public const int SW_HIDE = 0;
        public const int SW_SHOWNORMAL = 1;
        #endregion
    }
}
