#pragma once

// Dynamically loads the API to allow running on older releases.
bool ApplyWindowActionWrapper(HWND hwnd, WINDOW_ACTION* action);

//
// CWindowAction
//
// Helper class to set fields in a WINDOW_ACTION and call ApplyWindowAction.
//
// ApplyWindowAction: https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/nf-winuser-applywindowaction
// WINDOW_ACTION: https://learn.microsoft.com/en-us/windows/win32/winmsg/winuser/ns-winuser-windowaction
//
// A Window Action describes changes to make to a top level window. Moving,
// sizing, activating, maximizing, etc.
//
// The 'kinds' field of the action is flags that describe the changes to make.
// Some have no additional payload, like WAK_ACTIVATE, and others like
// WAK_POSITION have a corresponding field containing the value.
//
// The 'modifiers' field is also flags, some of which have their own field. The
// modifiers change the behavior of one or more of the kinds in some way. For
// example, the modifier WAM_FRAME_BOUNDS changes how the provided rect
// (position/size) are interpretted.
//
class CWindowAction : public WINDOW_ACTION
{
public:
    CWindowAction()
    {
        RtlZeroMemory(this, sizeof(*this));
    }

    // Calls ApplyWindowAction to apply the changes in the action to the window.
    bool Apply(HWND hwnd)
    {
        return ApplyWindowActionWrapper(hwnd, this);
    }

    //
    // The functions below set fields in the action.
    //

    // Activates the window.
    // This makes the window the active window (GetActiveWindow), and the
    // foreground window (GetForegroundWindow) if the app is in foreground.
    void SetActivate()
    {
        WI_SetFlag(kinds, WAK_ACTIVATE);
    }

    // Shows (or hides) a window. This sets/clears WS_VISIBLE.
    void SetVisible(bool val = true)
    {
        WI_SetFlag(kinds, WAK_VISIBILITY);
        visible = val;
    }

    // The insert after window is the window that this window is behind, or
    // below (in z-order). This window can be special sentinel values like
    // HWND_TOP or HWND_TOPMOST, which have special meaning.
    void SetInsertAfter(HWND val)
    {
        WI_SetFlag(kinds, WAK_INSERT_AFTER);
        insertAfter = val;
    }

    // Set the position and size (the rect).
    // By default, this rect includes parts of the window that are invisible
    // resize borders. (As opposed to 'frame bounds', the visible bounds of
    // the window, with invisible resize borders removed.)
    void SetRect(RECT rc)
    {
        WI_SetAllFlags(kinds, WAK_POSITION | WAK_SIZE);
        position.x = rc.left;
        position.y = rc.top;
        size.cx = RECTWIDTH(rc);
        size.cy = RECTHEIGHT(rc);
    }

    // Changes the behavior of the provided rect, indicating the rect does not
    // include invisible resize borders (it is the desired visible bounds of
    // the window). This is most useful with Arranged positions, which normally
    // align the visible bounds of the window with the edges of the monitor.
    void SetFrameBounds()
    {
        WI_SetFlag(modifiers, WAM_FRAME_BOUNDS);
    }

    // Sets the state, Minimize, Maximize, Arrange, Restore.
    // When Min/Max/Arranged, the window has a 'normal' position that is
    // separate from it's current position. (Normal is the last non-special
    // position.) Restoring a window from Max/Min/Arrange moves it back to the
    // normal position.
    void SetState(WINDOW_PLACEMENT_STATE state)
    {
        WI_SetFlag(kinds, WAK_PLACEMENT_STATE);
        placementState = state;
    }

    // Sets the normal rect. This requires also setting the state.
    //
    // If Max/Min/Arranged, the normal rect overrides the default normal
    // position (which is the previous non-special position the window had).
    // If the state is Restored, the normal position has the same meaning as
    // the position and size (the rect).
    void SetNormalRect(RECT rc)
    {
        WI_SetFlag(kinds, WAK_NORMAL_RECT);
        normalRect = rc;
    }

    void SetMaximized()
    {
        SetState(WPS_MAXIMIZED);
    }

    void SetRestored()
    {
        SetState(WPS_NORMAL);
    }

    // Setting Arranged requires an arranged rect, which is in frame bounds
    // (visible bounds, no invisible resize borders). This is expected to be
    // aligned with edges of the work area (like left half, or corner, etc).
    void SetArranged(RECT arrangeRect)
    {
        SetState(WPS_ARRANGED);
        SetRect(arrangeRect);
        SetFrameBounds();
    }

    // Minimize normally remembers the previous state of the window (if
    // Maximized or Arranged). Restoring a Minimized windows returns the window
    // to that previous state. Optionally, an action can specify the window be
    // Minimized but restore to Maximized or Arranged.
    void SetMinimized()
    {
        SetState(WPS_MINIMIZED);
    }

    void SetMinRestoreToMaximized()
    {
        SetState(WPS_MINIMIZED);
        WI_SetFlag(modifiers, WAM_RESTORE_TO_MAXIMIZED);
    }

    void SetMinRestoreToArranged(RECT arrangeRect)
    {
        SetState(WPS_MINIMIZED);
        WI_SetFlag(modifiers, WAM_RESTORE_TO_ARRANGED);
        SetRect(arrangeRect);
        SetFrameBounds();
    }

    // The fit to monitor flag causes the window's normal position to be moved
    // as needed to stay entirely within the bounds of the work area.
    void SetFitToMonitor()
    {
        WI_SetFlag(kinds, WAK_FIT_TO_MONITOR);
    }

    // The Move to Monitor flag specifies the monitor the window should be on,
    // using a point (this picks the nearest monitor to this point, in screen
    // coordinates).
    void SetMoveToMonitorPoint(POINT point)
    {
        WI_SetFlag(kinds, WAK_MOVE_TO_MONITOR);
        pointOnMonitor = point;
    }

    void SetMoveToMonitor(MonitorData monitor)
    {
        SetMoveToMonitorPoint({ monitor.workArea.left, monitor.workArea.top });
    }

    // The provided position and size are assumed to be picked for the current
    // monitors. If a position is from the past, the action can specify the
    // work area from the past. The provided position is adjusted if the work
    // area has changed, to ensure the window's position relative to the monitor
    // stays the same.
    void SetPreviousWorkArea(RECT prevWorkArea)
    {
        WI_SetFlag(modifiers, WAM_WORK_AREA);
        workArea = prevWorkArea;
    }

    // The provided size is assumed to be picked for the window's current DPI.
    // If the size is picked from a past DPI, or for the DPI of the monitor,
    // the DPI field in the action should be set to that DPI. This ensures that
    // if the window changes DPI, the final size matches the one provided.
    void SetPreviousDpi(UINT prevDpi)
    {
        WI_SetFlag(modifiers, WAM_DPI);
        dpi = prevDpi;
    }
};

// ApplyWindowAction is dynamically loaded the first time it is called.
typedef BOOL (__stdcall *fnApplyWindowAction)(HWND, WINDOW_ACTION*);

// Called once per process to load ApplyWindowAction (and check if supported).
inline fnApplyWindowAction LoadApplyWindowActionApi()
{
    static bool initOnce = false;
    static fnApplyWindowAction pfnApplyWindowAction = nullptr;
    if (initOnce)
    {
        return pfnApplyWindowAction;
    }
    initOnce = true;

    // Dynamically load the user32!ApplyWindowAction API.
    pfnApplyWindowAction =
        reinterpret_cast<fnApplyWindowAction>(
            GetProcAddress(LoadLibrary(L"user32.dll"), "ApplyWindowAction"));

    // There are some OS builds that have the API export prior to the API being
    // supported. To know if the API is supported (prior to attempting to use
    // it to move the window), we create a dummy window on the first call and
    // attempt to call ApplyWindowAction on it, to see if it returns false.
    if (pfnApplyWindowAction)
    {
        HINSTANCE hInstance = GetModuleHandle(NULL);
        PCWSTR className = L"ProbeApplyApiWindowClassName";

        WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
        wc.hInstance = hInstance;
        wc.lpfnWndProc = DefWindowProc;
        wc.lpszClassName = className;
        RegisterClassEx(&wc);

        HWND hwnd = CreateWindowEx(
            0, className, nullptr,
            0, 0, 0, 0, 0,
            nullptr, nullptr, hInstance, nullptr);

        WINDOW_ACTION action{};
        action.kinds = WAK_POSITION;

        if (!pfnApplyWindowAction(hwnd, &action))
        {
            // The API is present but disabled. Clear the pointer so that we
            // consider it not available.
            pfnApplyWindowAction = nullptr;
        }

        DestroyWindow(hwnd);
        UnregisterClass(className, hInstance);
    }

    return pfnApplyWindowAction;
}

// Returns true if the ApplyWindowAction API is supported on the current OS.
inline bool IsApplyWindowActionSupported()
{
    return (LoadApplyWindowActionApi() != nullptr);
}

// Calls ApplyWindowAction, if it is available and supported on the current OS.
inline bool ApplyWindowActionWrapper(HWND hwnd, WINDOW_ACTION* action)
{
    fnApplyWindowAction pfn = LoadApplyWindowActionApi();

    if (pfn)
    {
        if (pfn(hwnd, action))
        {
            return true;
        }

#ifdef BREAK_ON_WINDOW_ACTIONS_FAILURE
        __debugbreak();
        pfn(hwnd, action);
#endif
    }

    return false;
}
