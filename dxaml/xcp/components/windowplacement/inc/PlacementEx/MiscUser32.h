#pragma once
// Note: Intended to be included by User32Utils.h.

// Undefine RECTWIDTH/RECTHEIGHT if they were previously defined as macros
// (e.g. pcshell/shell/Taskbar/helpers/util.h). This header defines them as
// inline functions instead.
#ifdef RECTWIDTH
#undef RECTWIDTH
#endif
#ifdef RECTHEIGHT
#undef RECTHEIGHT
#endif

inline LONG RECTWIDTH(const RECT& rc)
{
    return (rc.right - rc.left);
}

inline LONG RECTHEIGHT(const RECT& rc)
{
    return (rc.bottom - rc.top);
}

constexpr UINT NUM_DPI_AWARENESS = 3;

// Returns true if the window is a top-level window (parented to the desktop window).
inline bool IsTopLevel(HWND hwnd)
{
    return (GetAncestor(hwnd, GA_PARENT) == GetDesktopWindow());
}

// Returns the DPI the thread is virtualized to, or 0 if the thread is not
// virtualized (Per-Monitor DPI Aware).
inline UINT GetThreadVirtualizedDpi()
{
    return GetDpiFromDpiAwarenessContext(GetThreadDpiAwarenessContext());
}

// Returns one of:
// - DPI_AWARENESS_UNAWARE
//      Virtualized to 100% DPI
// - DPI_AWARENESS_SYSTEM_AWARE
//      Virtualized to an arbitrary DPI, the DPI of the primary monitor
//      when the process started.
// - DPI_AWARENESS_PER_MONITOR_AWARE
//      Not virtualized. Expected to scale to the current DPI of the monitor.
inline DPI_AWARENESS GetThreadDpiAwareness()
{
    return GetAwarenessFromDpiAwarenessContext(GetThreadDpiAwarenessContext());
}

//
// Cloaking
//

inline bool IsCloaked(HWND hwnd)
{
    DWORD dwCloak = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &dwCloak, sizeof(dwCloak));
    return dwCloak != 0;
}

inline bool IsShellCloaked(HWND hwnd)
{
    DWORD dwCloak = 0;
    DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &dwCloak, sizeof(dwCloak));
    return WI_IsFlagSet(dwCloak, DWM_CLOAKED_SHELL);
}

inline void CloakWindow(HWND hwnd, BOOL cloak = TRUE)
{
    DwmSetWindowAttribute(hwnd, DWMWA_CLOAK, &cloak, sizeof(cloak));
}

inline void UnCloakWindow(HWND hwnd)
{
    CloakWindow(hwnd, false /* cloak */);
}

// Cloaking a window temporarily allows it to be moved multiple times,
// Maximized, etc without flashing or animating from unexpected locations.
class TempCloakWindowIf
{
    HWND _hwnd = nullptr;
public:
    TempCloakWindowIf(HWND hwnd, bool shouldHide = true)
    {
        if (shouldHide && !IsCloaked(hwnd))
        {
            CloakWindow(hwnd);
            _hwnd = hwnd;
        }
    }
    ~TempCloakWindowIf()
    {
        if (_hwnd)
        {
            UnCloakWindow(_hwnd);
        }
    }
};

// Note: 'Margins' below refers to invisible resize borders.
//
// This uses DWMWA_EXTENDED_FRAME_BOUNDS (the visible bounds) and GetWindowRect
// (the real/input bounds). The margins is the difference between these rects.
//
// DWMWA_EXTENDED_FRAME_BOUNDS returns PHYSICAL values, even if the caller is
// virtualized for DPI. GetWindowRect (and most other APIs) return logical
// coordinates. To allow using the margins from DPI virtualized apps, this
// switches explicitly to Per-Monitor DPI Aware (Physical) to call GetWindowRect
// and scales the values from the monitor DPI to the logical DPI, if needed.
inline RECT GetWindowMargins(HWND hwnd)
{
    DPI_AWARENESS_CONTEXT prev = SetThreadDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    RECT margins{};

    RECT rcWindow, rcFrame;
    if (GetWindowRect(hwnd, &rcWindow) &&
        SUCCEEDED(DwmGetWindowAttribute(
            hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rcFrame, sizeof(rcFrame))))
    {
        margins.left = rcFrame.left - rcWindow.left;
        margins.top = rcFrame.top - rcWindow.top;
        margins.right = rcWindow.right - rcFrame.right;
        margins.bottom = rcWindow.bottom - rcFrame.bottom;
    }

    MonitorData mon;
    if ((GetAwarenessFromDpiAwarenessContext(prev) != DPI_AWARENESS_PER_MONITOR_AWARE) &&
        (MonitorData::FromRect(rcWindow, &mon)))
    {
        UINT monitorDpi = mon.dpi;
        UINT threadDpi = GetDpiFromDpiAwarenessContext(prev);

        margins.left = MulDiv(margins.left, threadDpi, monitorDpi);
        margins.top = MulDiv(margins.top, threadDpi, monitorDpi);
        margins.right = MulDiv(margins.right, threadDpi, monitorDpi);
        margins.bottom = MulDiv(margins.bottom, threadDpi, monitorDpi);
    }

    SetThreadDpiAwarenessContext(prev);

    return margins;
}

inline void ExtendByMargins(RECT* rc, const RECT& margins)
{
    rc->left -= margins.left;
    rc->top -= margins.top;
    rc->right += margins.right;
    rc->bottom += margins.bottom;
}

inline void ReduceByMargins(RECT* rc, const RECT& margins)
{
    rc->left += margins.left;
    rc->top += margins.top;
    rc->right -= margins.right;
    rc->bottom -= margins.bottom;
}

// Gets a windows 'extended frame bounds'.
//
// This is the visible bounds of the window, not including the invisible resize
// area.
//
// Note: This doesn't call DWMWA_EXTENDED_FRAME_BOUNDS directly, because it
// does not work in DPI virtualized apps. (The result canot always be compared
// to GetWindowRect.) GetWindowMargins determines logical margins and this
// reduces the result of GetWindowRect by that amount, 'logical extended frame
// bounds'.
inline bool DwmGetExtendedFrameBounds(
    HWND hwnd,
    _Out_ PRECT prcFrame)
{
    if (!GetWindowRect(hwnd, prcFrame))
    {
        return false;
    }

    ReduceByMargins(prcFrame, GetWindowMargins(hwnd));

    return true;
}

// Returns true if the user setting for snapping is enabled.
// Settings, system, multitasking, 'snap windows'.
inline bool IsSnappingEnabled()
{
    BOOL snapEnabled = FALSE;
    return SystemParametersInfo(SPI_GETWINARRANGING, 0, &snapEnabled, 0) && snapEnabled;
}

// Wrapper that dynamically loads the API user32!IsWindowArranged.
//
// There is currently no header file for this function, so load it
// dynamically per the documentation.
// https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-iswindowarranged
inline bool IsWindowArrangedWrapper(HWND hwnd)
{
    using PFNISWINDOWARRANGED = BOOL(*)(HWND hwnd);
    static PFNISWINDOWARRANGED fnIsWindowArranged = nullptr;
    static bool doOnce = false;
    if (!doOnce)
    {
        doOnce = true;
        HMODULE hmodUser = LoadLibraryW(L"user32.dll");
        fnIsWindowArranged = reinterpret_cast<PFNISWINDOWARRANGED>(
            GetProcAddress(hmodUser, "IsWindowArranged"));
    }
    if (fnIsWindowArranged)
    {
        return !!fnIsWindowArranged(hwnd);
    }
    return false;
}
