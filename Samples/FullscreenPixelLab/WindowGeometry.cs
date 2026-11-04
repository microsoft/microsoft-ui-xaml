using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;

namespace FullscreenPixelLab;

internal sealed record WindowGeometry(
    int TopOffset, int ClientWidth, int ClientHeight, int BridgeWidth, int BridgeHeight,
    uint Dpi, uint Style, uint ExStyle, bool Maximized)
{
    internal static double GetScale(IntPtr hwnd) => ReadDpi(hwnd) / 96.0;

    private static uint ReadDpi(IntPtr hwnd)
    {
        var dpi = GetDpiForWindow(hwnd);
        if (dpi == 0)
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "GetDpiForWindow failed.");
        }
        return dpi;
    }

    internal static WindowGeometry Read(IntPtr hwnd)
    {
        IntPtr bridge = IntPtr.Zero;
        EnumChildWindows(hwnd, (child, _) =>
        {
            var name = new StringBuilder(128);
            if (GetClassName(child, name, name.Capacity) > 0
                && name.ToString() == "Microsoft.UI.Content.DesktopChildSiteBridge")
            {
                bridge = child;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        if (bridge == IntPtr.Zero)
        {
            throw new InvalidOperationException("The XAML DesktopChildSiteBridge HWND was not found.");
        }

        var origin = new Point();
        if (!ClientToScreen(hwnd, ref origin) || !GetClientRect(hwnd, out var client)
            || !GetWindowRect(bridge, out var rect))
        {
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Could not read the client and XAML bridge rectangles.");
        }

        return new WindowGeometry(rect.Top - origin.Y, client.Right - client.Left, client.Bottom - client.Top,
            rect.Right - rect.Left, rect.Bottom - rect.Top, ReadDpi(hwnd),
            ReadStyle(hwnd, -16), ReadStyle(hwnd, -20), IsZoomed(hwnd));
    }

    private static uint ReadStyle(IntPtr hwnd, int index)
    {
        Marshal.SetLastPInvokeError(0);
        var value = GetWindowLongPtr(hwnd, index);
        var error = Marshal.GetLastWin32Error();
        if (value == IntPtr.Zero && error != 0)
        {
            throw new Win32Exception(error, "GetWindowLongPtr failed.");
        }
        return unchecked((uint)value.ToInt64());
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Point
    {
        public int X, Y;
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct Rect
    {
        public int Left, Top, Right, Bottom;
    }

    private delegate bool EnumWindowCallback(IntPtr hwnd, IntPtr parameter);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool EnumChildWindows(IntPtr parent, EnumWindowCallback callback, IntPtr parameter);

    [DllImport("user32.dll", EntryPoint = "GetClassNameW", CharSet = CharSet.Unicode)]
    private static extern int GetClassName(IntPtr hwnd, StringBuilder className, int count);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool ClientToScreen(IntPtr hwnd, ref Point point);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetClientRect(IntPtr hwnd, out Rect rect);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetWindowRect(IntPtr hwnd, out Rect rect);

    [DllImport("user32.dll", EntryPoint = "GetWindowLongPtrW", SetLastError = true)]
    private static extern IntPtr GetWindowLongPtr(IntPtr hwnd, int index);

    [DllImport("user32.dll", SetLastError = true)]
    private static extern uint GetDpiForWindow(IntPtr hwnd);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool IsZoomed(IntPtr hwnd);
}
