using System;
using System.Runtime.InteropServices;
using OSFrameworkLens.Detection;

namespace OSFrameworkLens.UI;

/// <summary>
/// Click-through highlight rectangle drawn around the element under the cursor.
/// Implemented as a raw Win32 layered window because WinUI 3's composition
/// pipeline doesn't cooperate with WS_EX_LAYERED on its own Window surfaces.
/// LWA_COLORKEY (magenta) provides pixel-perfect transparency outside the border.
/// </summary>
internal sealed class HighlightOverlay
{
    private const string ClassName = "OSFrameworkLensOverlayClass";
    private const uint TransparentKeyColorRef = 0x00FF00FF; // magenta — no framework paints in this
    private const int StrokeThickness = 3;
    private const uint DefaultStrokeColorRef = 0x000000FF; // red COLORREF (0x00BBGGRR)
    private const int InitialWindowSize = 100;

    private static readonly Native.WNDPROC s_wndProcDelegate = WndProc;
    private static ushort s_classAtom;
    private static IntPtr s_keyBrush;
    private static readonly object s_classLock = new();

    private IntPtr _hwnd;
    private bool _visible;

    public IntPtr Hwnd => _hwnd;

    public void Show()
    {
        EnsureClassRegistered();

        // Layered + transparent + tool + noactivate = click-through, never focused,
        // never in Alt+Tab.
        const uint exStyle = Native.WS_EX_LAYERED | Native.WS_EX_TRANSPARENT
                           | Native.WS_EX_TOOLWINDOW | Native.WS_EX_TOPMOST
                           | Native.WS_EX_NOACTIVATE;

        _hwnd = Native.CreateWindowExW(
            exStyle, ClassName, null,
            Native.WS_POPUP,
            0, 0, InitialWindowSize, InitialWindowSize,
            IntPtr.Zero, IntPtr.Zero,
            Native.GetModuleHandleW(null), IntPtr.Zero);

        if (_hwnd == IntPtr.Zero)
            throw new InvalidOperationException(
                $"CreateWindowExW failed: {Marshal.GetLastWin32Error()}");

        Native.SetLayeredWindowAttributes(_hwnd, TransparentKeyColorRef, 0, Native.LWA_COLORKEY);
    }

    public void ShowRect(int left, int top, int right, int bottom, string colorHex)
    {
        if (_hwnd == IntPtr.Zero) return;
        s_currentColor = HexColor.ParseColorRef(colorHex, DefaultStrokeColorRef);

        Native.SetWindowPos(_hwnd, Native.HWND_TOPMOST,
            left, top, right - left, bottom - top,
            Native.SWP_NOACTIVATE | (_visible ? 0 : Native.SWP_SHOWWINDOW));
        _visible = true;
        Native.InvalidateRect(_hwnd, IntPtr.Zero, true);
    }

    public void HideRect()
    {
        if (_hwnd == IntPtr.Zero || !_visible) return;
        Native.ShowWindow(_hwnd, Native.SW_HIDE);
        _visible = false;
    }

    public void Close()
    {
        if (_hwnd != IntPtr.Zero)
        {
            Native.DestroyWindow(_hwnd);
            _hwnd = IntPtr.Zero;
        }
    }

    private static void EnsureClassRegistered()
    {
        lock (s_classLock)
        {
            if (s_classAtom != 0) return;
            // Background brush MUST be the color-key — otherwise uninitialised pixels
            // stay black and LWA_COLORKEY produces a solid black overlay.
            s_keyBrush = Native.CreateSolidBrush(TransparentKeyColorRef);
            var wc = new Native.WNDCLASSEX
            {
                cbSize = (uint)Marshal.SizeOf<Native.WNDCLASSEX>(),
                lpfnWndProc = Marshal.GetFunctionPointerForDelegate(s_wndProcDelegate),
                hInstance = Native.GetModuleHandleW(null),
                lpszClassName = ClassName,
                hbrBackground = s_keyBrush,
            };
            s_classAtom = Native.RegisterClassExW(ref wc);
            if (s_classAtom == 0)
                throw new InvalidOperationException(
                    $"RegisterClassExW failed: {Marshal.GetLastWin32Error()}");
        }
    }

    private static IntPtr WndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam)
    {
        if (msg == Native.WM_NCHITTEST) return new IntPtr(Native.HTTRANSPARENT);
        if (msg == Native.WM_PAINT) { PaintBorder(hWnd); return IntPtr.Zero; }
        return Native.DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    private static void PaintBorder(IntPtr hWnd)
    {
        var hdc = Native.BeginPaint(hWnd, out var ps);
        try
        {
            var pen = Native.CreatePen(Native.PS_SOLID, StrokeThickness, s_currentColor);
            var oldPen = Native.SelectObject(hdc, pen);
            var oldBrush = Native.SelectObject(hdc, Native.GetStockObject(Native.NULL_BRUSH));
            try
            {
                var w = ps.rcPaint.Right - ps.rcPaint.Left;
                var h = ps.rcPaint.Bottom - ps.rcPaint.Top;
                // Inset by half the pen width so the stroke stays inside the window.
                var inset = StrokeThickness / 2;
                Native.Rectangle(hdc, inset, inset, w - inset, h - inset);
            }
            finally
            {
                Native.SelectObject(hdc, oldPen);
                Native.SelectObject(hdc, oldBrush);
                Native.DeleteObject(pen);
            }
        }
        finally
        {
            Native.EndPaint(hWnd, ref ps);
        }
    }

    // Static because WndProc requires it; only one overlay is ever live.
    private static uint s_currentColor;
}
