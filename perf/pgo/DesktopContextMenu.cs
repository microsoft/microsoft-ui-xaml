using System;
using System.Runtime.InteropServices;

/// <summary>
/// Sends a right-click at the center of the screen using the SendInput API.
/// Used by PGO training to launch the desktop context menu.
/// </summary>
public class DesktopContextMenu
{
    [DllImport("user32.dll", SetLastError = true)]
    static extern uint SendInput(uint nInputs, INPUT[] pInputs, int cbSize);

    [DllImport("user32.dll")]
    static extern int GetSystemMetrics(int nIndex);

    const int SM_CXSCREEN = 0;
    const int SM_CYSCREEN = 1;
    const int INPUT_MOUSE = 0;
    const uint MOUSEEVENTF_MOVE      = 0x0001;
    const uint MOUSEEVENTF_RIGHTDOWN = 0x0008;
    const uint MOUSEEVENTF_RIGHTUP   = 0x0010;
    const uint MOUSEEVENTF_ABSOLUTE  = 0x8000;

    [StructLayout(LayoutKind.Sequential)]
    struct INPUT { public int type; public MOUSEINPUT mi; }

    [StructLayout(LayoutKind.Sequential)]
    struct MOUSEINPUT
    {
        public int dx, dy;
        public uint mouseData, dwFlags, time;
        public IntPtr dwExtraInfo;
    }

    /// <summary>
    /// Moves the cursor to the center of the primary screen and performs a right-click.
    /// </summary>
    public static void RightClickScreenCenter()
    {
        int w = GetSystemMetrics(SM_CXSCREEN);
        int h = GetSystemMetrics(SM_CYSCREEN);

        // SendInput uses absolute coordinates in the 0-65535 range.
        int absX = (w / 2) * 65536 / w;
        int absY = (h / 2) * 65536 / h;
        int size = Marshal.SizeOf(typeof(INPUT));

        var move = new INPUT();
        move.type = INPUT_MOUSE;
        move.mi.dx = absX;
        move.mi.dy = absY;
        move.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
        SendInput(1, new INPUT[] { move }, size);
        System.Threading.Thread.Sleep(300);

        var down = new INPUT();
        down.type = INPUT_MOUSE;
        down.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, new INPUT[] { down }, size);
        System.Threading.Thread.Sleep(100);

        var up = new INPUT();
        up.type = INPUT_MOUSE;
        up.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, new INPUT[] { up }, size);
    }
}
