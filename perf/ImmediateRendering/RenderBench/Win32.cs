using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Windows.Foundation;

namespace RenderBench;

// Real OS input (SendInput) so every backend sees the same hit-test/pointer path, and screen capture for the
// visual-parity check (C1).
static unsafe partial class Win32
{
    public static (int X, int Y) ElementCenterOnScreen(FrameworkElement element, nint hwnd) => ElementPointOnScreen(element, hwnd, 0.5, 0.5);

    // Point at fractions (fx, fy) of the element's bounds, in screen pixels.
    public static (int X, int Y) ElementPointOnScreen(FrameworkElement element, nint hwnd, double fx, double fy)
    {
        var point = element.TransformToVisual(null).TransformPoint(new Point(element.ActualWidth * fx, element.ActualHeight * fy));
        double scale = element.XamlRoot.RasterizationScale;
        var client = new POINT { X = (int)Math.Round(point.X * scale), Y = (int)Math.Round(point.Y * scale) };
        ClientToScreen(hwnd, ref client);
        return (client.X, client.Y);
    }

    public static void Click(int x, int y)
    {
        SetCursorPos(x, y);
        var inputs = stackalloc INPUT[2];
        inputs[0] = new INPUT { type = 0, mi = new MOUSEINPUT { dwFlags = 0x0002 } }; // LEFTDOWN
        inputs[1] = new INPUT { type = 0, mi = new MOUSEINPUT { dwFlags = 0x0004 } }; // LEFTUP
        SendInput(2, inputs, sizeof(INPUT));
    }

    public static void ForceForeground(nint hwnd) => SetForegroundWindow(hwnd);

    // ---- Extended scenes S3-S10 ----

    // Wheel notches at (x, y); positive delta scrolls up/left. Horizontal uses MOUSEEVENTF_HWHEEL.
    public static uint Wheel(int x, int y, int delta, bool horizontal = false)
    {
        SetCursorPos(x, y);
        var input = new INPUT { type = 0, mi = new MOUSEINPUT { mouseData = unchecked((uint)delta), dwFlags = horizontal ? 0x1000u : 0x0800u } };
        return SendInput(1, &input, sizeof(INPUT));
    }

    public static uint MouseMove(int x, int y)
    {
        var input = AbsoluteMove(x, y);
        return SendInput(1, &input, sizeof(INPUT));
    }

    // Press at (x1, y1), move in `steps` increments, release at (x2, y2), queued as one SendInput batch so the positions
    // travel with the events.
    public static uint Drag(int x1, int y1, int x2, int y2, int steps = 8)
    {
        var inputs = new INPUT[steps + 3];
        inputs[0] = AbsoluteMove(x1, y1);
        inputs[1] = new INPUT { type = 0, mi = new MOUSEINPUT { dwFlags = 0x0002 } };
        for (int i = 1; i <= steps; i++)
        {
            inputs[i + 1] = AbsoluteMove(x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps);
        }
        inputs[^1] = new INPUT { type = 0, mi = new MOUSEINPUT { dwFlags = 0x0004 } };
        fixed (INPUT* p = inputs)
        {
            return SendInput((uint)inputs.Length, p, sizeof(INPUT));
        }
    }

    // One character as KEYEVENTF_UNICODE down/up; layout-independent typing.
    public static uint TypeChar(char ch)
    {
        var inputs = stackalloc INPUT[2];
        inputs[0] = new INPUT { type = 1, ki = new KEYBDINPUT { wScan = ch, dwFlags = 0x0004 } };
        inputs[1] = new INPUT { type = 1, ki = new KEYBDINPUT { wScan = ch, dwFlags = 0x0004 | 0x0002 } };
        return SendInput(2, inputs, sizeof(INPUT));
    }

    public const ushort VK_TAB = 0x09, VK_ESCAPE = 0x1B;

    public static uint Key(ushort vk)
    {
        var inputs = stackalloc INPUT[2];
        inputs[0] = new INPUT { type = 1, ki = new KEYBDINPUT { wVk = vk } };
        inputs[1] = new INPUT { type = 1, ki = new KEYBDINPUT { wVk = vk, dwFlags = 0x0002 } };
        return SendInput(2, inputs, sizeof(INPUT));
    }

    static INPUT AbsoluteMove(int x, int y)
    {
        int left = GetSystemMetrics(76), top = GetSystemMetrics(77);
        int width = Math.Max(1, GetSystemMetrics(78) - 1), height = Math.Max(1, GetSystemMetrics(79) - 1);
        return new INPUT
        {
            type = 0,
            mi = new MOUSEINPUT
            {
                dx = (int)Math.Round((x - left) * 65535.0 / width),
                dy = (int)Math.Round((y - top) * 65535.0 / height),
                dwFlags = 0x0001 | 0x8000 | 0x4000, // MOVE | ABSOLUTE | VIRTUALDESK
            },
        };
    }

    public static uint CurrentSessionId() => ProcessIdToSessionId((uint)Environment.ProcessId, out uint id) ? id : uint.MaxValue;

    public static uint ActiveConsoleSessionId() => WTSGetActiveConsoleSessionId();

    // Does SendInput reach this desktop? Moves the cursor to (x, y) with a real input event and checks that the
    // session's last-input tick advanced. In a disconnected/remote session SendInput succeeds but nothing arrives.
    public static bool ProbeSendInput(int x, int y, out string reason)
    {
        var before = new LASTINPUTINFO { cbSize = (uint)sizeof(LASTINPUTINFO) };
        GetLastInputInfo(ref before);
        Thread.Sleep(30);
        uint sent = MouseMove(x, y);
        Thread.Sleep(50);
        var after = new LASTINPUTINFO { cbSize = (uint)sizeof(LASTINPUTINFO) };
        GetLastInputInfo(ref after);
        if (sent != 1)
        {
            reason = $"SendInput returned {sent} (error {Marshal.GetLastPInvokeError()})";
            return false;
        }
        if (after.dwTime == before.dwTime)
        {
            reason = "SendInput produced no input event (GetLastInputInfo unchanged)";
            return false;
        }
        reason = "SendInput probe delivered input";
        return true;
    }

    // Refresh rate of the monitor showing hwnd; null when the driver doesn't report one (0/1 = hardware default).
    public static double? RefreshRate(nint hwnd)
    {
        nint monitor = MonitorFromWindow(hwnd, 2 /* MONITOR_DEFAULTTONEAREST */);
        var info = new MONITORINFOEXW { cbSize = (uint)sizeof(MONITORINFOEXW) };
        if (!GetMonitorInfoW(monitor, ref info))
        {
            return null;
        }
        const int DevModeSize = 220, FrequencyOffset = 184, SizeOffset = 68;
        var devMode = stackalloc byte[DevModeSize];
        new Span<byte>(devMode, DevModeSize).Clear();
        *(ushort*)(devMode + SizeOffset) = DevModeSize;
        if (!EnumDisplaySettingsW(info.szDevice, -1 /* ENUM_CURRENT_SETTINGS */, devMode))
        {
            return null;
        }
        uint hz = *(uint*)(devMode + FrequencyOffset);
        return hz > 1 ? hz : null;
    }

    // Captures the window's client area to a 32bpp BMP. Screen: BitBlt from the screen DC (what DWM composed). In a
    // disconnected/minimized remote session the screen copy is black, so Auto falls back to
    // PrintWindow(PW_CLIENTONLY | PW_RENDERFULLCONTENT), which asks DWM for the window's own redirected content.
    public static (int Width, int Height, string Method, bool Uniform) SaveClientScreenshot(nint hwnd, string path, ScreenshotMethod method)
    {
        GetClientRect(hwnd, out RECT rect);
        int width = rect.Right - rect.Left, height = rect.Bottom - rect.Top;
        byte[] pixels;
        string used;
        if (method == ScreenshotMethod.PrintWindow)
        {
            pixels = Capture(hwnd, width, height, usePrintWindow: true);
            used = "PrintWindow";
        }
        else
        {
            pixels = Capture(hwnd, width, height, usePrintWindow: false);
            used = "Screen";
            if (method == ScreenshotMethod.Auto && IsUniform(pixels))
            {
                pixels = Capture(hwnd, width, height, usePrintWindow: true);
                used = "PrintWindow (screen copy was uniform)";
            }
        }
        WriteBmp(path, width, height, pixels);
        return (width, height, used, IsUniform(pixels));
    }

    // A screen copy of a whole scene that is one color (black, or white in some remote sessions) didn't capture it.
    static bool IsUniform(byte[] pixels)
    {
        for (int i = 4; i < pixels.Length; i += 4)
        {
            if (pixels[i] != pixels[0] || pixels[i + 1] != pixels[1] || pixels[i + 2] != pixels[2])
            {
                return false;
            }
        }
        return true;
    }

    static byte[] Capture(nint hwnd, int width, int height, bool usePrintWindow)
    {
        var origin = new POINT();
        ClientToScreen(hwnd, ref origin);
        nint screen = GetDC(0);
        nint memory = CreateCompatibleDC(screen);
        nint bitmap = CreateCompatibleBitmap(screen, width, height);
        nint previous = SelectObject(memory, bitmap);
        if (usePrintWindow)
        {
            PrintWindow(hwnd, memory, 0x1 | 0x2 /* PW_CLIENTONLY | PW_RENDERFULLCONTENT */);
        }
        else
        {
            BitBlt(memory, 0, 0, width, height, screen, origin.X, origin.Y, 0x00CC0020 /* SRCCOPY */);
        }
        SelectObject(memory, previous);

        var header = new BITMAPINFOHEADER { biSize = (uint)sizeof(BITMAPINFOHEADER), biWidth = width, biHeight = -height,
            biPlanes = 1, biBitCount = 32 };
        var pixels = new byte[width * height * 4];
        fixed (byte* data = pixels)
        {
            GetDIBits(memory, bitmap, 0, (uint)height, data, &header, 0);
        }
        DeleteObject(bitmap);
        DeleteDC(memory);
        ReleaseDC(0, screen);
        return pixels;
    }

    public static void WriteBmp(string path, int width, int height, byte[] pixels)
    {
        var header = new BITMAPINFOHEADER { biSize = (uint)sizeof(BITMAPINFOHEADER), biWidth = width, biHeight = -height,
            biPlanes = 1, biBitCount = 32 };
        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using var file = new BinaryWriter(File.Create(path));
        file.Write((ushort)0x4D42);
        file.Write(14 + sizeof(BITMAPINFOHEADER) + pixels.Length);
        file.Write(0);
        file.Write(14 + sizeof(BITMAPINFOHEADER));
        file.Write(new ReadOnlySpan<byte>(&header, sizeof(BITMAPINFOHEADER)));
        file.Write(pixels);
    }

    [StructLayout(LayoutKind.Sequential)] struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public nuint dwExtraInfo; }

    [StructLayout(LayoutKind.Sequential)]
    struct KEYBDINPUT { public ushort wVk, wScan; public uint dwFlags, time; public nuint dwExtraInfo; }

    [StructLayout(LayoutKind.Explicit)]
    struct INPUT
    {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public MOUSEINPUT mi;
        [FieldOffset(8)] public KEYBDINPUT ki;
    }

    [StructLayout(LayoutKind.Sequential)] struct LASTINPUTINFO { public uint cbSize, dwTime; }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    struct MONITORINFOEXW
    {
        public uint cbSize;
        public RECT rcMonitor, rcWork;
        public uint dwFlags;
        public fixed char szDeviceChars[32];
        public string szDevice { get { fixed (char* p = szDeviceChars) { return new string(p); } } }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct BITMAPINFOHEADER
    {
        public uint biSize; public int biWidth, biHeight; public ushort biPlanes, biBitCount;
        public uint biCompression, biSizeImage; public int biXPelsPerMeter, biYPelsPerMeter; public uint biClrUsed, biClrImportant;
    }

    [DllImport("user32.dll")] static extern bool ClientToScreen(nint hwnd, ref POINT point);
    [DllImport("user32.dll")] static extern bool GetClientRect(nint hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll", SetLastError = true)] static extern uint SendInput(uint count, INPUT* inputs, int size);
    [DllImport("user32.dll")] static extern bool GetLastInputInfo(ref LASTINPUTINFO info);
    [DllImport("user32.dll")] static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll")] static extern bool PrintWindow(nint hwnd, nint dc, uint flags);
    [DllImport("user32.dll")] static extern nint MonitorFromWindow(nint hwnd, uint flags);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern bool GetMonitorInfoW(nint monitor, ref MONITORINFOEXW info);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern bool EnumDisplaySettingsW(string device, int mode, byte* devMode);
    [DllImport("kernel32.dll")] static extern bool ProcessIdToSessionId(uint pid, out uint session);
    [DllImport("kernel32.dll")] static extern uint WTSGetActiveConsoleSessionId();
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(nint hwnd);
    [DllImport("user32.dll")] static extern nint GetDC(nint hwnd);
    [DllImport("user32.dll")] static extern int ReleaseDC(nint hwnd, nint dc);
    [DllImport("gdi32.dll")] static extern nint CreateCompatibleDC(nint dc);
    [DllImport("gdi32.dll")] static extern nint CreateCompatibleBitmap(nint dc, int width, int height);
    [DllImport("gdi32.dll")] static extern nint SelectObject(nint dc, nint obj);
    [DllImport("gdi32.dll")] static extern bool BitBlt(nint dest, int x, int y, int w, int h, nint src, int sx, int sy, uint rop);
    [DllImport("gdi32.dll")] static extern int GetDIBits(nint dc, nint bitmap, uint start, uint lines, byte* bits, BITMAPINFOHEADER* info, uint usage);
    [DllImport("gdi32.dll")] static extern bool DeleteObject(nint obj);
    [DllImport("gdi32.dll")] static extern bool DeleteDC(nint dc);
}
