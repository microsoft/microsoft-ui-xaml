using System;
using System.Diagnostics;
using System.Text;

namespace OSFrameworkLens.Detection;

internal static class HwndInspector
{
    // GetClassName: MSDN max class-name length is 256.
    private const int ClassNameBufferSize = 256;
    // Safety cap on WM_GETTEXTLENGTH — 4 KB chars covers every real title and bounds per-tick allocation.
    private const int MaxTitleChars = 4096;
    // Drill depth cap for ChildWindowFromPointEx loops — well above the deepest
    // UI hierarchy seen in the wild (~10), bounds runaway loops if a target
    // returns its own child.
    private const int MaxHwndDrillDepth = 32;
    // SendMessageTimeout bound — must stay well below one 30 Hz tick so a hung
    // target UI thread can't stall the inspector.
    private const uint MessageTimeoutMs = 50;

    public static IntPtr DeepestHwndAt(Native.POINT screenPoint)
    {
        var hwnd = Native.WindowFromPoint(screenPoint);
        if (hwnd == IntPtr.Zero) return IntPtr.Zero;

        var cursor = hwnd;
        for (int depth = 0; depth < MaxHwndDrillDepth; depth++)
        {
            var pt = screenPoint;
            if (!Native.ScreenToClient(cursor, ref pt)) break;
            var child = Native.ChildWindowFromPointEx(cursor, pt, Native.CWP_SKIPINVISIBLE);
            if (child == IntPtr.Zero || child == cursor) break;
            cursor = child;
        }
        return cursor;
    }

    public static string GetClassName(IntPtr hwnd)
    {
        var sb = new StringBuilder(ClassNameBufferSize);
        var n = Native.GetClassNameW(hwnd, sb, sb.Capacity);
        return n > 0 ? sb.ToString() : string.Empty;
    }

    /// <summary>Two-step title read (WM_GETTEXTLENGTH → WM_GETTEXT); both ABORTIFHUNG so a stuck target UI thread can't stall the 30 Hz tick. Returns available=false on timeout.</summary>
    public static (string text, bool available) GetWindowTitleSafe(IntPtr hwnd)
    {
        if (Native.SendMessageTimeoutW(hwnd, Native.WM_GETTEXTLENGTH, IntPtr.Zero, IntPtr.Zero,
                Native.SMTO_ABORTIFHUNG, MessageTimeoutMs, out var lenResult) == IntPtr.Zero)
            return (string.Empty, false);

        var length = Math.Min(lenResult.ToInt32(), MaxTitleChars);
        if (length <= 0) return (string.Empty, true);

        var sb = new StringBuilder(length + 1); // +1 for trailing NUL
        return Native.SendMessageTimeoutW(hwnd, Native.WM_GETTEXT, (IntPtr)sb.Capacity, sb,
                Native.SMTO_ABORTIFHUNG, MessageTimeoutMs, out _) != IntPtr.Zero
            ? (sb.ToString(), true)
            : (string.Empty, false);
    }

    /// <summary>Per-message timeout (ms) — exposed so user-facing strings can quote the exact value.</summary>
    public static uint TitleTimeoutMs => MessageTimeoutMs;

    public static (uint pid, uint tid) GetThreadProcess(IntPtr hwnd)
    {
        var tid = Native.GetWindowThreadProcessId(hwnd, out var pid);
        return (pid, tid);
    }

    public static string GetProcessNameSafe(uint pid)
    {
        try { using var p = Process.GetProcessById((int)pid); return p.ProcessName + ".exe"; }
        // Process exited between GetWindowThreadProcessId and here.
        catch (ArgumentException)        { return $"pid:{pid}"; }
        // Process object exists but the underlying handle is already gone.
        catch (InvalidOperationException) { return $"pid:{pid}"; }
    }

    public static Native.RECT GetBounds(IntPtr hwnd)
    {
        Native.GetWindowRect(hwnd, out var r);
        return r;
    }

    public static IntPtr GetRoot(IntPtr hwnd) => Native.GetAncestor(hwnd, Native.GA_ROOT);
}
