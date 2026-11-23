using System.Runtime.InteropServices;

namespace RenderBench;

// ---- coverage ----
// Full-window capture for the XAML coverage harness, identical in every backend mode. Options evaluated (test plan,
// "XAML coverage: harness and results"):
//  - RenderTargetBitmap of the content root: excludes popups (flyouts, dialogs, tooltips) and windowed popups, and
//    renders through a separate path rather than what the backend presented.
//  - Windows.Graphics.Capture / screen BitBlt / DWM thumbnails: need DWM to compose an on-screen desktop; in a
//    disconnected or minimized RDP session they return black (S22), and a thumbnail can't be read back at all.
//  - PrintWindow(PW_RENDERFULLCONTENT): DWM renders the window's own redirected content (DComp visual trees and the
//    immediate swap chain alike) into a DC, headless. Windowed popups (ShouldConstrainToRootBounds=false) are separate
//    top-level windows, so every visible, uncloaked top-level window of the process is captured the same way and
//    composited over the main client area in z-order.
// Chosen: PrintWindow of the window set. In immediate modes the harness also keeps WINUI_IMMEDIATE_CAPTURE and reports
// its difference from this capture (CaptureCrossCheckDiff).
static unsafe partial class Win32
{
    public const string WindowSetCaptureMethod = "PrintWindow(PW_RENDERFULLCONTENT) window set";

    public static (BgraImage Image, int PopupWindows) CaptureWindowSet(nint mainHwnd)
    {
        GetClientRect(mainHwnd, out RECT client);
        var origin = new POINT();
        ClientToScreen(mainHwnd, ref origin);
        int width = client.Right - client.Left, height = client.Bottom - client.Top;
        var canvas = PrintToBgra(mainHwnd, width, height, clientOnly: true);

        int popups = 0;
        foreach (nint popup in ProcessTopLevelWindows(mainHwnd).AsEnumerable().Reverse()) // bottom of the z-order first
        {
            GetWindowRect(popup, out RECT r);
            int w = r.Right - r.Left, h = r.Bottom - r.Top;
            if (w <= 0 || h <= 0)
            {
                continue;
            }
            var pixels = PrintToBgra(popup, w, h, clientOnly: false);
            popups++;
            BlendInto(canvas, width, height, pixels, w, h, r.Left - origin.X, r.Top - origin.Y);
        }
        for (int i = 3; i < canvas.Length; i += 4)
        {
            canvas[i] = 255;
        }
        return (new BgraImage(width, height, canvas), popups);
    }

    // Moves the cursor off the window so no control is in a pointer-over state.
    public static void ParkCursor(nint hwnd)
    {
        GetWindowRect(hwnd, out RECT r);
        SetCursorPos(r.Right + 50, r.Bottom + 50);
    }

    // Visible, uncloaked top-level windows of this process other than `mainHwnd`, top of the z-order first.
    static List<nint> ProcessTopLevelWindows(nint mainHwnd)
    {
        var state = new EnumState { Main = mainHwnd, ProcessId = (uint)Environment.ProcessId };
        var handle = GCHandle.Alloc(state);
        try
        {
            EnumWindows(&CollectWindow, GCHandle.ToIntPtr(handle));
        }
        finally
        {
            handle.Free();
        }
        return state.Windows;
    }

    sealed class EnumState
    {
        public nint Main;
        public uint ProcessId;
        public List<nint> Windows = [];
    }

    [UnmanagedCallersOnly]
    static int CollectWindow(nint hwnd, nint data)
    {
        var state = (EnumState)GCHandle.FromIntPtr(data).Target!;
        GetWindowThreadProcessId(hwnd, out uint pid);
        if (hwnd != state.Main && pid == state.ProcessId && IsWindowVisible(hwnd))
        {
            int cloaked = 0;
            DwmGetWindowAttribute(hwnd, 14 /* DWMWA_CLOAKED */, &cloaked, sizeof(int));
            if (cloaked == 0)
            {
                state.Windows.Add(hwnd);
            }
        }
        return 1;
    }

    // PrintWindow into a top-down 32bpp DIB section so per-pixel alpha (transparent popup margins) survives.
    static byte[] PrintToBgra(nint hwnd, int width, int height, bool clientOnly)
    {
        var header = new BITMAPINFOHEADER { biSize = (uint)sizeof(BITMAPINFOHEADER), biWidth = width, biHeight = -height,
            biPlanes = 1, biBitCount = 32 };
        nint screen = GetDC(0);
        nint memory = CreateCompatibleDC(screen);
        byte* bits;
        nint bitmap = CreateDIBSection(screen, &header, 0, &bits, 0, 0);
        nint previous = SelectObject(memory, bitmap);
        var pixels = new byte[width * height * 4];
        PrintWindow(hwnd, memory, (clientOnly ? 0x1u : 0u) | 0x2u /* PW_CLIENTONLY | PW_RENDERFULLCONTENT */);
        GdiFlush();
        new ReadOnlySpan<byte>(bits, pixels.Length).CopyTo(pixels);
        SelectObject(memory, previous);
        DeleteObject(bitmap);
        DeleteDC(memory);
        ReleaseDC(0, screen);
        return pixels;
    }

    // Source-over with premultiplied alpha when the popup's alpha channel carries coverage; opaque copy when the whole
    // alpha channel is zero (PrintWindow left it unset). PrintWindow renders the transparent margin a windowed popup
    // keeps for its shadow as pure black (RGB 0) in every mode, so pure-black popup pixels count as transparent: the
    // capture shows the popup body over the main window, without the DWM-composed shadow.
    static void BlendInto(byte[] canvas, int cw, int ch, byte[] src, int sw, int sh, int dx, int dy)
    {
        bool hasAlpha = false;
        for (int i = 3; i < src.Length && !hasAlpha; i += 4)
        {
            hasAlpha = src[i] != 0;
        }
        for (int y = Math.Max(0, -dy); y < sh && y + dy < ch; y++)
        {
            for (int x = Math.Max(0, -dx); x < sw && x + dx < cw; x++)
            {
                int s = (y * sw + x) * 4, d = ((y + dy) * cw + x + dx) * 4;
                if (src[s] == 0 && src[s + 1] == 0 && src[s + 2] == 0)
                {
                    continue;
                }
                int a = hasAlpha ? src[s + 3] : 255;
                for (int c = 0; c < 3; c++)
                {
                    canvas[d + c] = (byte)Math.Min(255, src[s + c] + canvas[d + c] * (255 - a) / 255);
                }
            }
        }
    }

    [DllImport("user32.dll")] static extern bool EnumWindows(delegate* unmanaged<nint, nint, int> callback, nint data);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(nint hwnd, out uint processId);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(nint hwnd);
    [DllImport("user32.dll")] static extern bool GetWindowRect(nint hwnd, out RECT rect);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(nint hwnd, uint attribute, void* value, int size);
    [DllImport("gdi32.dll")] static extern nint CreateDIBSection(nint dc, BITMAPINFOHEADER* info, uint usage, byte** bits, nint section, uint offset);
    [DllImport("gdi32.dll")] static extern bool GdiFlush();
}
