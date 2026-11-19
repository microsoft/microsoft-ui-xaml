using System.Runtime.InteropServices;
using Microsoft.UI.Xaml;
using Windows.Foundation;

namespace RenderBench;

// Real OS input (SendInput) so every backend sees the same hit-test/pointer path, and screen capture for the
// visual-parity check (C1).
static unsafe class Win32
{
    public static (int X, int Y) ElementCenterOnScreen(FrameworkElement element, nint hwnd)
    {
        var point = element.TransformToVisual(null).TransformPoint(new Point(element.ActualWidth / 2, element.ActualHeight / 2));
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

    // Captures the window's client area from the screen (what DWM composed) to a 32bpp BMP.
    public static (int Width, int Height) SaveClientScreenshot(nint hwnd, string path)
    {
        GetClientRect(hwnd, out RECT rect);
        var origin = new POINT();
        ClientToScreen(hwnd, ref origin);
        int width = rect.Right - rect.Left, height = rect.Bottom - rect.Top;

        nint screen = GetDC(0);
        nint memory = CreateCompatibleDC(screen);
        nint bitmap = CreateCompatibleBitmap(screen, width, height);
        nint previous = SelectObject(memory, bitmap);
        BitBlt(memory, 0, 0, width, height, screen, origin.X, origin.Y, 0x00CC0020 /* SRCCOPY */);
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

        Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
        using var file = new BinaryWriter(File.Create(path));
        file.Write((ushort)0x4D42);
        file.Write(14 + sizeof(BITMAPINFOHEADER) + pixels.Length);
        file.Write(0);
        file.Write(14 + sizeof(BITMAPINFOHEADER));
        file.Write(new ReadOnlySpan<byte>(&header, sizeof(BITMAPINFOHEADER)));
        file.Write(pixels);
        return (width, height);
    }

    [StructLayout(LayoutKind.Sequential)] struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    struct MOUSEINPUT { public int dx, dy; public uint mouseData, dwFlags, time; public nuint dwExtraInfo; }

    [StructLayout(LayoutKind.Explicit)]
    struct INPUT
    {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public MOUSEINPUT mi;
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
    [DllImport("user32.dll")] static extern uint SendInput(uint count, INPUT* inputs, int size);
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
