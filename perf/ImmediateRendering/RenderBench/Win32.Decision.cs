using System.Runtime.InteropServices;

namespace RenderBench;

// Helpers for the backend decision scenes S24-S32.
static unsafe partial class Win32
{
    const uint WM_GETMINMAXINFO = 0x0024;
    static readonly HashSet<nint> s_unclamped = [];

    // Windows clamps a captioned window to about the virtual screen (WM_GETMINMAXINFO ptMaxTrackSize). S24 and S30 need
    // client areas larger than the 1024x768 VM desktop, so the window is subclassed to raise that limit.
    public static void AllowLargerThanScreen(nint hwnd)
    {
        if (s_unclamped.Add(hwnd) && !SetWindowSubclass(hwnd, &MaxTrackProc, 0x5224, 0))
        {
            s_unclamped.Remove(hwnd);
            throw new InvalidOperationException($"SetWindowSubclass failed ({Marshal.GetLastPInvokeError()})");
        }
    }

    [UnmanagedCallersOnly]
    static nint MaxTrackProc(nint hwnd, uint message, nint wParam, nint lParam, nuint id, nuint data)
    {
        nint result = DefSubclassProc(hwnd, message, wParam, lParam);
        if (message == WM_GETMINMAXINFO)
        {
            var info = (MINMAXINFO*)lParam;
            info->ptMaxTrackSize = new POINT { X = 8192, Y = 8192 };
        }
        return result;
    }

    public static (int Width, int Height) ClientSize(nint hwnd)
    {
        GetClientRect(hwnd, out var rect);
        return (rect.Right - rect.Left, rect.Bottom - rect.Top);
    }

    // A topmost, opaque, non-activating popup covering hwnd's window rectangle (S27 occlusion). Destroy with DestroyOccluder.
    public static nint CreateOccluder(nint hwnd)
    {
        GetWindowRect(hwnd, out var rect);
        const uint WS_POPUP = 0x80000000, WS_VISIBLE = 0x10000000;
        const uint WS_EX_TOPMOST = 0x00000008, WS_EX_TOOLWINDOW = 0x00000080, WS_EX_NOACTIVATE = 0x08000000;
        nint occluder = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, "STATIC", "RenderBench occluder",
            WS_POPUP | WS_VISIBLE, rect.Left - 8, rect.Top - 8, rect.Right - rect.Left + 16, rect.Bottom - rect.Top + 16,
            0, 0, 0, 0);
        if (occluder == 0)
        {
            throw new InvalidOperationException($"CreateWindowEx(STATIC) failed ({Marshal.GetLastPInvokeError()})");
        }
        return occluder;
    }

    public static void DestroyOccluder(nint occluder)
    {
        if (occluder != 0)
        {
            DestroyWindow(occluder);
        }
    }

    public static bool IsIconicWindow(nint hwnd) => IsIconic(hwnd);

    // Run-RenderBench shows and closes a WinForms window before each decision run so that DWM releases the previous run's
    // surfaces; here, wait until dwm.exe private bytes are stable (two samples within 0.5 MB, at most 3 s) before the baseline.
    public static void SettleDwm(Func<long> dwmPrivateBytes)
    {
        DwmFlush();
        long previous = dwmPrivateBytes();
        var clock = System.Diagnostics.Stopwatch.StartNew();
        while (clock.ElapsedMilliseconds < 3000)
        {
            Thread.Sleep(250);
            long current = dwmPrivateBytes();
            if (Math.Abs(current - previous) < 512 * 1024 && clock.ElapsedMilliseconds >= 750)
            {
                return;
            }
            previous = current;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    struct MINMAXINFO { public POINT ptReserved, ptMaxSize, ptMaxPosition, ptMinTrackSize, ptMaxTrackSize; }

    [DllImport("comctl32.dll", SetLastError = true)]
    static extern bool SetWindowSubclass(nint hwnd, delegate* unmanaged<nint, uint, nint, nint, nuint, nuint, nint> proc, nuint id, nuint data);
    [DllImport("comctl32.dll")] static extern nint DefSubclassProc(nint hwnd, uint message, nint wParam, nint lParam);
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    static extern nint CreateWindowExW(uint exStyle, string className, string windowName, uint style, int x, int y, int width,
        int height, nint parent, nint menu, nint instance, nint param);
    [DllImport("user32.dll")] static extern bool DestroyWindow(nint hwnd);
    [DllImport("user32.dll")] static extern bool IsIconic(nint hwnd);
    [DllImport("dwmapi.dll")] static extern int DwmFlush();
}
