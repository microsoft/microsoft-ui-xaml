#pragma once
// Note: Intended to be included by User32Utils.h.

//
// PlacementEx
//
// This stores a top-level window's position and monitor info.
//
// GetPlacement stores a window's position into the provided PlacementEx, and
// SetPlacement moves a window to this stored position, adjusting it as needed
// to fit the best monitor.
//
// This also has helpers that do other operations related to storing window
// positions, such as FullScreen, Cascading, StartupInfo, etc.
//
// See the readme.md in the same directory as this file more information.
//

// Internal flags used to track window state between launches
enum class PlacementFlags
{
    None                    = 0x0000,

    RestoreToMaximized      = 0x0001, // WPF_RESTORETOMAXIMIZED
                                      // Valid only if minimized (SW_MINIMIZE).
                                      // When restored from minimized, the
                                      // window will be maximized (restoring
                                      // it again will move it to the normal
                                      // position).

    Arranged                = 0x0002, // Window is arranged (snapped).
                                      // Set by GetPlacement if IsWindowArranged.
                                      //
                                      // This is used by SetPlacement if not
                                      // maximizing or minimizing, and if the
                                      // user setting for snapping is enabled.
                                      //
                                      // Like maximized/minimized windows,
                                      // arranged windows have separate normal
                                      // positions and 'real' position. Calling
                                      // ShowWindow SW_RESTORE will move the
                                      // window back to its normal position.
                                      //
                                      // The arrange position is stored in
                                      // the arrangeRect field. This is the
                                      // visible bounds of the window,
                                      // GetWindowRect, reduced by the size of
                                      // the window's invisible resize borders.

    AllowPartiallyOffScreen = 0x0004, // Skips forcing the normal position
                                      // within the bounds of the work area,
                                      // which SetPlacement does by default.

    AllowSizing             = 0x0008, // Set (by GetPlacement) if window has
                                      // WS_THICKFRAME.
                                      // In SetPlacement, if the normal rect
                                      // becomes larger than the work area, for
                                      // example if a wide window is moved to a
                                      // small monitor.
                                      // When this happens, the size of the
                                      // window is picked using the difference
                                      // in monitor sizes, instead of respecting
                                      // the logical size of the window.

    KeepHidden              = 0x0010, // Any show command other than SW_HIDE
                                      // will show the window. The KeepHidden
                                      // flag will cause the window to stay
                                      // hidden if it is not already visible.

    RestoreToArranged       = 0x0020, // Like RestoreToMaximized, but restores
                                      // arranged.
                                      // Like Arranged flag, this flag indicates
                                      // the arrangeRect field is set to the
                                      // visible bounds (the arrange rect,
                                      // aligned with the work area).
                                      // Valid only if minimized (SW_MINIMIZE).

    FullScreen              = 0x0040, // A FullScreen window fills the monitor
                                      // rect, and has no caption (WS_CAPTION)
                                      // or resize borders (WS_THICKFRAME).
                                      // To Enter/Exit FullScreen, the app must
                                      // remember the previous position (unlike
                                      // Maximize/Minimize, where the system
                                      // remembers this position). PlacementEx
                                      // handles this scenario.

    VirtualDesktopId        = 0x0080, // The virtualDesktopId field is set.
                                      // This is a GUID used with the virtual
                                      // desktop APIs.
                                      // Apps should only restore windows to a
                                      // past virtual desktop if restarting,
                                      // either because of a session reboot or
                                      // app re-launch.
                                      // (Default app launch should not launch
                                      // to a background desktop; similar to
                                      // launching minimized.)
                                      // Note: Virtual desktops require defining
                                      // USE_VIRTUAL_DESKTOP_APIS prior to including
                                      // User32Utils.h. See VirtualDesktopIds.h.

    NoActivate              = 0x0100, // Don't activate the window.
                                      // This is implied if VirtualDesktopId.

    NoApplyWindowAction     = 0x0200, // (Testing purposes only.) Always use the
                                      // fallback implementation for SetPlacement.
                                      // This skips the implementation that uses
                                      // ApplyWindowAction, which is an API that
                                      // was recently added and not available on
                                      // older releases.

};
DEFINE_ENUM_FLAG_OPERATORS(PlacementFlags);

// Flags to adjust how PlacementEx uses STARTUPINFO
enum class StartupInfoFlags
{
    ShowCommand             = 0x0001, // Heed the show command set by the command
                                      // line:
                                      // cmd: 'start /max <path>'
                                      // pwsh: 'start -WindowStyle Maximized'
                                      // This allows the user to launch an app
                                      // Maximized or Minimized.

    MonitorHint             = 0x0002, // Heed the monitor hint set by the taskbar,
                                      // start menu, desktop icons, file explorer,
                                      // etc. It indicates which monitor the app
                                      // should launch on.

    None                    = 0,

    // [Default] Heed both ShowCommand and MonitorHint. This matches the
    // default behavior of CreateWindow.
    All                     = ShowCommand | MonitorHint,
};
DEFINE_ENUM_FLAG_OPERATORS(StartupInfoFlags);

class PlacementEx
{
public:
    // Stores a window's position/monitor data in a PlacementEx.
    static bool GetPlacement(
        HWND hwnd,
        _Out_ PlacementEx* placement);

    // Moves a window to a position stored in a PlacementEx.
    static bool SetPlacement(
        HWND hwnd,
        _Inout_ PlacementEx* placement);

    // Stores a placement in the registry.
    void StoreInRegistry(
        PCWSTR registryPath,
        PCWSTR registryKeyName);

    static void StorePlacementInRegistry(
        HWND hwnd,
        PCWSTR registryPath,
        PCWSTR registryKeyName);

    // FullScreen
    //
    // A FullScreen window is one that fills its monitor, and has some styles
    // removed (no WS_CAPTION or WS_THICKFRAME). When entering FullScreen, the
    // previous window position is stored. When exiting FullScreen, this position
    // is moved to the window's monitor (which may have changed) and used to
    // move the window to it's previous position.
    bool EnterFullScreen(
        HWND hwnd,
        LONG_PTR stylesToRemove = WS_CAPTION | WS_THICKFRAME);

    bool ExitFullScreen(
        HWND hwnd,
        LONG_PTR stylesToAdd = WS_CAPTION | WS_THICKFRAME);

    bool IsFullScreen() const
    {
        return HasFlag(PlacementFlags::FullScreen);
    }

    bool ToggleFullScreen(
        HWND hwnd,
        LONG_PTR styles = WS_CAPTION | WS_THICKFRAME)
    {
        if (IsFullScreen())
        {
            return ExitFullScreen(hwnd, styles);
        }

        return EnterFullScreen(hwnd, styles);
    }

    // Sets the size.
    // The provided size is logical, and scaled up by the DPI stored in the
    // placement. The final position is adjusted to stay entirely within the
    // work area.
    void SetLogicalSize(SIZE size);

    // Fits the placement to a different monitor.
    // This changes the normal rect, work area, and dpi fields.
    // By default (unless AllowPartiallyOffScreen), the final normal rect will
    // be entirely within the bounds of the provided monitor's work area.
    void MoveToMonitor(const MonitorData& targetMonitor);

    void MoveToWindowMonitor(HWND hwnd)
    {
        MonitorData monitor;
        if (MonitorData::FromWindow(hwnd, &monitor))
        {
            MoveToMonitor(monitor);
        }
    }

    // Finds the monitor that best matches the position stored in the placement.
    // This uses the device name (a string) to match monitors, and falls back to
    // closest monitor (MonitorFromRect).
    bool FindClosestMonitor(_Out_ MonitorData* monitor) const;

    // Moves the position down/right by roughly the height of the title bar.
    // This is done when launching two windows to the same place. Rather than
    // have both windows in the exact same place (one covering the other),
    // an app can choose to cascade the windows, ensuring the user can see both
    // window's title bars. Note that this function doesn't check that scenario:
    // it always moves the window.
    void Cascade();

    // Cascades one window over another.
    static bool CascadeOver(HWND hwndMove, HWND hwndOver);

    // Updates a placement to account for flags set in the STARTUPINFO.
    // If no startup info is provided, this uses GetStartupInfo.
    // These are flags set when launching an application. For example, they
    // can request the app launch Maximized/Minimized, or on a certain monitor.
    void AdjustForStartupInfo(
        _In_opt_ STARTUPINFO* psi = nullptr,
        StartupInfoFlags siFlags = StartupInfoFlags::All);

    // AdjustForStartupInfo and also modify the placement for use as a 'main
    // window' (normal launch). When launching the main window normally, it
    // should not be minimized or on a background virtual desktop.
    void AdjustForMainWindow(
        _In_opt_ STARTUPINFO* psi = nullptr,
        StartupInfoFlags siFlags = StartupInfoFlags::All);

    // Sets the show command (SW_ value) for this placement.
    // There are a few special cases, including 'restore to maximize' (if
    // changing the placement from maximize to minimize, this sets the
    // RestoreToMaximized placement flag).
    void SetShowCommand(UINT cmdFromStartupIfo);

    void RestoreIfMinimized()
    {
        if (IsMinimizeShowCmd(showCmd))
        {
            SetShowCommand(SW_NORMAL);
        }
    }

    // Returns true if the provided show command is minimize/restore.
    // (There are several SW_ values for each of these, for example
    // SW_SHOWMINNOACTIVE, SW_SHOWMINIMIZED, SW_MINIMIZE.)
    static bool IsMinimizeShowCmd(UINT cmd);
    static bool IsRestoreShowCmd(UINT cmd);

    // Helper to move a RECT as needed to be fully within a work area.
    static RECT KeepRectOnMonitor(
        const RECT& rcPrev,
        const RECT & workArea);

    // Helper to move a normal position from one monitor to another.
    void AdjustNormalRect(
        const RECT& rcWorkNew,
        UINT dpiNew);

    // Given an arrangement rect stored from one monitor (work area), adjusts
    // the rect to stay the same relative position (like top-left corner) on
    // a different monitor's work area.
    void AdjustArrangeRect(const RECT rcWorkNew);

    // Adds (or Removes) some window styles.
    static void AddRemoveWindowStyles(
        HWND hwnd,
        bool add,
        LONG_PTR stylesToChange);

    // If the API ApplyWindowAction is available, SetPlacement is implemented
    // in this separate function, defined in WindowActions.h.
    static bool SetPlacementWithApplyWindowAction(
        HWND hwnd,
        const MonitorData& targetMonitor,
        _Inout_ PlacementEx* placement);

    // ToString() and FromString() allow you to store a placement in a string,
    // for example to write it to the registry.
    std::wstring ToString() const;

    static std::optional<PlacementEx> FromString(
        const std::wstring& placementString);

    static bool FromString(
        const std::wstring& placementString,
        _Out_ PlacementEx* placement)
    {
        std::optional<PlacementEx> plex = FromString(placementString);
        if (plex)
        {
            *placement = plex.value();
            return true;
        }
        return false;
    }

    static bool FromRegistryKey(
        PCWSTR registryPath,
        PCWSTR registryKeyName,
        _Out_ PlacementEx* placement);

    // Checks if a PlacementEx is valid (filled in by GetPlacement).
    bool IsValid() const;

    // Zeros out a PlacementEx, making it invalid.
    void Clear();

    bool HasFlag(PlacementFlags pf) const
    {
        return (flags & pf) == pf;
    }

    // Position (in screen coordinates) of the window.
    // For Maximized/Minimized windows this rect is the restore position.
    RECT normalRect = {};

    // The work area of the monitor (in screen coordinates).
    RECT workArea = {};

    // The (potentially virtualized) DPI of the window. If the window is DPI-
    // aware, returns the actual DPI of the window. Otherwise returns the
    // virtualized DPI that the app sees before the system stretches it to the
    // monitor's actual DPI.
    //
    // For DPI-aware windows, scale factor can be calculated by dividing by 96
    // (e.g. 120 DPI would be a scale factor of 120/96 = 125%). These windows
    // must manually scale their content by this scale factor, or content will
    // be rendered too small on high-DPI screens.
    UINT dpi = 0;

    // The 'Show Command' is a value accepted by ShowWindow().
    // This defines the window's state (SW_MAXIMIZE, SW_MINIMIZE, SW_NORMAL).
    UINT showCmd = 0;

    // The arrange rect is used only when the flag Arranged is set.
    //
    // This rect is the 'frame bounds' (visible bounds) of the window when it
    // is snapped (arranged). This rect does not include the invisible resize
    // borders (it is the rect that is expected to be aligned with the monitor
    // edges).
    //
    // Like maximized/minimized windows, arranged windows have two positions,
    // their normal rect (GetWindowPlacement) and their current position. For
    // maximized/minimized, this positions are always re-evaluated for the
    // monitor automatically, so we don't need to store the min/max positions.
    // But we DO need to store the arranged positions, for example to remember
    // that a window was snapped to the left half of its monitor (or top-left
    // corner, etc).
    RECT arrangeRect = {};

    // Additional flags.
    PlacementFlags flags = PlacementFlags::None;

    // The device name is a string representing the monitor (also 'device ID').
    // In cases like changing the primary monitor, the handle, rect, etc, all
    // may change but the device name would remain stable.
    WCHAR deviceName[CCHDEVICENAME] = {};

#ifdef USE_VIRTUAL_DESKTOP_APIS
    GUID virtualDesktopId = {};
#endif
};

// A PlacementEx is created zero initialized, which is invalid.
//
// A valid placement has:
// - a non-zero size normal position
// - a non-zero size work area
// - a normal position that intersects with the work area
// - a DPI that is >=96 (100%, the minimum supported DPI)
inline bool
PlacementEx::IsValid() const
{
    RECT rcI;
    return (dpi >= 96) &&
            !IsRectEmpty(&normalRect) &&
            !IsRectEmpty(&workArea) &&
            IntersectRect(&rcI, &normalRect, &workArea);
}

inline void
PlacementEx::Clear()
{
    dpi = 0;
    showCmd = 0;
    flags = PlacementFlags::None;
    normalRect = {};
    arrangeRect = {};
    workArea = {};
    deviceName[0] = 0;
}

/* static */
inline bool
PlacementEx::GetPlacement(
    HWND hwnd,
    _Out_ PlacementEx* placement)
{
    placement->Clear();

    // PlacementEx is for top-level windows only. To position child windows you
    // should call SetWindowPos directly. (Child windows don't consider monitors,
    // maximize/minimize, etc, so PlacementEx isn't needed.)
    if (!IsTopLevel(hwnd))
    {
        return false;
    }

    // Call GetWindowPlacement to get the window's show state (max/min/normal)
    // and the normal position.
    // Note: If the window is arranged (snapped) this returns 'normal' as the
    // state but the normal position is the restore position (not where the
    // window is, but where it will go if restored from arrange).
    WINDOWPLACEMENT wp = { sizeof(wp) };
    if (!GetWindowPlacement(hwnd, &wp))
    {
        return false;
    }

    // Get the monitor info for the window's current monitor.
    MonitorData monitorData;
    if (!MonitorData::FromWindow(hwnd, &monitorData))
    {
        return false;
    }

    PlacementFlags flags = PlacementFlags::None;
    RECT rcWork = monitorData.workArea;
    RECT rcMonitor = monitorData.monitorRect;
    RECT rcNormal = wp.rcNormalPosition;
    RECT rcArrange{};

    // Offset the normal rect by the work area offset from the monitor rect,
    // aka 'Workspace Coordinates'. (PlacementEx doesn't use workspace
    // coordinates, but Get/SetWindowPlacement do).
    OffsetRect(&rcNormal,
        rcWork.left - rcMonitor.left, rcWork.top - rcMonitor.top);

    // Set RestoreToMaximize flag if window is minimized and GetWindowPlacement
    // set the WPF_RESTORETOMAXIMIZED flag.
    // Note: ASYNC flag is never expected here, and the min position is ignored.
    if (IsMinimizeShowCmd(wp.showCmd) &&
        (WI_IsFlagSet(wp.flags, WPF_RESTORETOMAXIMIZED)))
    {
        WI_SetFlag(flags, PlacementFlags::RestoreToMaximized);
    }

    // The DPI in the PlacementEx is the window's DPI, GetDpiForWindow.
    // But, if the current thread is virtualized for DPI, we instead use the
    // thread's DPI. This could be different from the DPI that the window
    // is running at (which is what GetDpiForwindow returns). The other APIs
    // we're calling, querying the window and monitor info, are using the
    // thread DPI.
    const UINT threadDpi = GetThreadVirtualizedDpi();
    const UINT dpi = (threadDpi == 0) ? GetDpiForWindow(hwnd) : threadDpi;

    // Set the arranged flag and arranged ret if the window is arranged.
    if (IsWindowArrangedWrapper(hwnd))
    {
        // The arranged rect is the visible bounds of the window.
        // This does not include the invisible resize borders (if the window
        // has any).
        // Note: This helper is in MiscUser32.h, and calls DwmGetWindowAttribute
        // (DWMWA_EXTENDED_FRAME_BOUNDS) with some adjustments (to make it safe
        // to call from DPI virtualized apps).
        if (!DwmGetExtendedFrameBounds(hwnd, &rcArrange))
        {
            return false;
        }

        WI_SetFlag(flags, PlacementFlags::Arranged);
    }

    LONG_PTR styles = GetWindowLongPtr(hwnd, GWL_STYLE);

    // Set the AllowSizing flag if the window has the WS_THICKFRAME style.
    if (WI_IsFlagSet(styles, WS_THICKFRAME))
    {
        WI_SetFlag(flags, PlacementFlags::AllowSizing);
    }

    // If window has no caption/resize border styles and is positioned
    // perfectly fitting the monitor rect, set FullScreen flag.
    RECT rcWindow;
    if (WI_AreAllFlagsClear(styles, WS_CAPTION | WS_THICKFRAME) &&
        GetWindowRect(hwnd, &rcWindow) &&
        EqualRect(&rcWindow, &rcMonitor))
    {
        WI_SetFlag(flags, PlacementFlags::FullScreen);
    }

#ifdef USE_VIRTUAL_DESKTOP_APIS
    if (GetVirtualDesktopId(hwnd, &placement->virtualDesktopId))
    {
        WI_SetFlag(flags, PlacementFlags::VirtualDesktopId);
    }
#endif

    // Fill in the provided PlacementEx.
    placement->normalRect = rcNormal;
    placement->arrangeRect = rcArrange;
    placement->dpi = dpi;
    placement->showCmd = wp.showCmd;
    placement->workArea = rcWork;
    placement->flags = flags;

    if (FAILED(StringCchCopy(
            placement->deviceName, ARRAYSIZE(placement->deviceName), monitorData.deviceName)))
    {
        return false;
    }

    return true;
}

// Called by PlacementEx::SetPlacement, if IsApplyWindowActionSupported.
// This translates the PlacementEx into a WINDOW_ACTION and calls ApplyWindowAction.
/* static */
inline bool
PlacementEx::SetPlacementWithApplyWindowAction(
    HWND hwnd,
    const MonitorData& targetMonitor,
    _Inout_ PlacementEx* placement)
{
    // This API is not available on older releases.
    if (!IsApplyWindowActionSupported())
    {
        return false;
    }

    const bool fVirtDesktop = placement->HasFlag(PlacementFlags::VirtualDesktopId);
    const bool fFullScreen = placement->HasFlag(PlacementFlags::FullScreen);

    // Hide window temporarily if moving multiple times.
    TempCloakWindowIf hideIf(hwnd, fVirtDesktop || fFullScreen);

    CWindowAction action;

    // Always specify the monitor the window should be on. This is needed for
    // a few reasons:
    // - ApplyWindowAction always uses nearest monitor (using the previous
    //      monitor's work area). But PlacementEx also stores the monitor's
    //      device name, and uses that if we find a match. This ensures the
    //      window stays on the same monitor even if the primary monitor changes
    //      (which changes the position of each monitor but not the device names).
    // - If setting the arrange position, we need to specify the monitor for
    //      the arrange position to be adjusted if needed by ApplyWindowAction.
    //      For example, if setting an arranged position from the past and the
    //      monitor has been removed or changed.
    action.SetMoveToMonitor(targetMonitor);

    // Activate by default, unless NoActivate flag.
    // If setting a virtual desktop this implies NoActivate.
    if (!placement->HasFlag(PlacementFlags::NoActivate) && !fVirtDesktop)
    {
        action.SetActivate();
    }

    // Show window by default, unless KeepHidden and previously invisible.
    if (!IsWindowVisible(hwnd) &&
        !placement->HasFlag(PlacementFlags::KeepHidden))
    {
        action.SetVisible();
    }

    // Set Fit to Monitor by default, unless AllowPartiallyOffScreen.
    if (!placement->HasFlag(PlacementFlags::AllowPartiallyOffScreen))
    {
        action.SetFitToMonitor();
    }

    // Set the state (Min/Max/Arrange/Restore).
    //
    // This is determined by the show command (SW_MAXIMIZE, SW_MINIMIZE, etc),
    // and by the Arranged PlacementEx flag.
    //
    // If Minimize, the two PlacementEx flags RestoreToMaximized/Arranged set
    // the restore state (when window restores from Min).
    //
    // If Arrange, or Min restore to Arrange, the PlacementEx has an arrange
    // position. This is a rect within the work area in the placement, normally
    // aligned with 2 or 3 edges of the work area. (Position does not include
    // invisible frame bounds, WAM_FRAME_BOUNDS.)
    if (placement->showCmd == SW_MAXIMIZE)
    {
        action.SetMaximized();
    }
    else if (IsMinimizeShowCmd(placement->showCmd))
    {
        if (placement->HasFlag(PlacementFlags::RestoreToMaximized))
        {
            action.SetMinRestoreToMaximized();
        }
        else if (placement->HasFlag(PlacementFlags::RestoreToArranged))
        {
            action.SetMinRestoreToArranged(placement->arrangeRect);
        }
        else
        {
            action.SetMinimized();
        }
    }
    else if (placement->HasFlag(PlacementFlags::Arranged))
    {
        action.SetArranged(placement->arrangeRect);
    }
    else
    {
        action.SetRestored();
    }

    // Set the normal rect. This is the restore position if Min/Max/Arrange,
    // or the window position if restored.
    action.SetNormalRect(placement->normalRect);

    // Set the work area and DPI. These are from the window/monitor when the
    // placement was created, and are used to update the position as needed if
    // the monitors have changed.
    action.SetPreviousWorkArea(placement->workArea);
    action.SetPreviousDpi(placement->dpi);

    // Call ApplyWindowAction to apply the changes.
    if (!action.Apply(hwnd))
    {
        return false;
    }

    // Enter FullScreen, if needed.
    if (fFullScreen)
    {
        placement->EnterFullScreen(hwnd);
    }

#ifdef USE_VIRTUAL_DESKTOP_APIS
    // Set virtual desktop ID, if needed.
    if (fVirtDesktop)
    {
        MoveToVirtualDesktop(hwnd, placement->virtualDesktopId);
    }
#endif

    return true;
}

inline bool
PlacementEx::FindClosestMonitor(_Out_ MonitorData* monitor) const
{
    // Use the device name first to try to match the monitor. If two monitors
    // change places (user switches primary monitor), the position and other
    // monitor fields will change, but the device names should be most stable.
    //
    // Fall back to finding the monitor closest to the normal rect.

    return MonitorData::FromDeviceName(deviceName, monitor) ||
           MonitorData::FromRect(normalRect, monitor);
}

/* static */
inline bool
PlacementEx::SetPlacement(HWND hwnd, _Inout_ PlacementEx* placement)
{
    // Determine which monitor is best/closest.
    MonitorData targetMonitor;
    if (!placement->FindClosestMonitor(&targetMonitor))
    {
        return false;
    }

    // On newer OS releases, SetPlacement uses the API ApplyWindowAction.
    // On older releaes (if the API is not available), or if the call to
    // ApplyWindowAction fails, we fall back to APIs that are available on
    // older releases.
    if (!placement->HasFlag(PlacementFlags::NoApplyWindowAction) &&
        SetPlacementWithApplyWindowAction(hwnd, targetMonitor, placement))
    {
        return true;
    }

    // Adjust the position to fit the monitor picked above.
    placement->MoveToMonitor(targetMonitor);

    // The RestoreToArranged flag is read only if Minimizing the window and
    // if snapping (the global user setting) is enabled.
    // If this is set, we'll arrange the window first, then minimize it.
    const bool isMinFromArranged =
        IsMinimizeShowCmd(placement->showCmd) &&
        placement->HasFlag(PlacementFlags::RestoreToArranged) &&
        IsSnappingEnabled();

    // The Arranged flag is read only when not Maximizing/Minimizing, and
    // if snapping (the global user setting) is enabled.
    // If Arranged (or RestoreToArranged), the arrangeRect field is set to
    // the visible bounds of the arranged window, fit to the work area.
    // If arranging the window, after moving it to the normal position, we'll
    // arrange (snap) the window and move it to the arrange rect, updated as
    // needed to fit the new monitor's work area.
    const bool isArranged =
        isMinFromArranged ||
        (placement->HasFlag(PlacementFlags::Arranged) &&
            !IsMinimizeShowCmd(placement->showCmd) &&
            (placement->showCmd != SW_MAXIMIZE) &&
            IsSnappingEnabled());

    // Determine if the window is changing monitors.
    // Note: This is the monitor the window is on now (not the monitor this
    // placement is from).
    // When moving a window between monitors, we always need to move it twice
    // (or else moving between monitors with different DPIs could result in
    // the wrong final window position).
    MonitorData previousMonitor;
    const bool isChangingMonitor =
        MonitorData::FromWindow(hwnd, &previousMonitor) &&
        !previousMonitor.Equals(targetMonitor);

    // Determine if the window was previously Maximized/Minimized/Arranged.
    const bool wasMinMaxArranged =
        IsZoomed(hwnd) || IsIconic(hwnd) || IsWindowArrangedWrapper(hwnd);

    // The KeepHidden flag is only read if the window is currently hidden.
    // Most show commands (other than SW_HIDE) will show the window. If this
    // flag is set (and window is not already visible) we'll hide the window
    // at the end.
    const bool keepHidden =
        !IsWindowVisible(hwnd) &&
        placement->HasFlag(PlacementFlags::KeepHidden);

    // The FullScreen flag causes the window to be sized to the monitor.
    const bool fullScreen = placement->HasFlag(PlacementFlags::FullScreen);

    // The VirtualDesktopId flag will set the window's virtual desktop ID,
    // potentially moving it to a background virtual desktop.
    const bool setVirtualDesktop =
        placement->HasFlag(PlacementFlags::VirtualDesktopId);

#ifdef USE_VIRTUAL_DESKTOP_APIS
    // If we're going to set the virtual desktop ID, remember the last
    // foreground window. This is used before moving the window to a background
    // virtual desktop, to avoid changing the active virtual desktop.
    const HWND prevFg = setVirtualDesktop ? GetForegroundWindow() : nullptr;
#endif

    // If moving the window multiple times, cloak the window during the operation.
    // This avoids the window flashing or animating from an unexpected location.
    TempCloakWindowIf hideIf(hwnd, isChangingMonitor ||
                                   isArranged ||
                                   wasMinMaxArranged ||
                                   keepHidden ||
                                   fullScreen ||
                                   setVirtualDesktop);

    // Restore the window if it is maximized, minimized, or arranged.
    // The normal position is the last non-Max/Min/Arrange position the window
    // had. This means that we need the window to be 'normal' temporarily.
    if (wasMinMaxArranged)
    {
        // SW_SHOWNOACTIVATE is like SW_RESTORE, but doesn't activate.
        // Note: SW_SHOWNA shows but doesn't restore from Min/Max/Arrange.
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    }

    // If the window is changing monitors, we need to move it onto the monitor
    // before moving it into the final position.
    //
    // This is needed to handle DPI changes. When moving a window over a DPI
    // change, it's final size can be unpredictable. To ensure the window ends
    // up at the position we picked above (MoveToMonitor) we need to move it
    // to that position after it is already on the monitor it is moving to.
    if (isChangingMonitor)
    {
        UINT toDpi = GetDpiForWindow(hwnd);
        UINT fromDpi = targetMonitor.dpi;
        int cx = MulDiv(RECTWIDTH(placement->normalRect), toDpi, fromDpi);
        int cy = MulDiv(RECTHEIGHT(placement->normalRect), toDpi, fromDpi);

        SetWindowPos(hwnd,
            nullptr,
            placement->normalRect.left,
            placement->normalRect.top,
            cx,
            cy,
            SWP_NOACTIVATE | SWP_NOZORDER);
    }

    // Transform coordinates to Workspace Coordinates.
    // PlacementEx stores coordinates in Screen Coordinates, but
    // Get/SetWindowPlacement use workspace: offset by difference in
    // work/monitor rect origin.
    OffsetRect(&placement->normalRect,
        targetMonitor.monitorRect.left - targetMonitor.workArea.left,
        targetMonitor.monitorRect.top - targetMonitor.workArea.top);

    WINDOWPLACEMENT wp = { sizeof(wp) };
    wp.rcNormalPosition = placement->normalRect;

    // Use SW_NORMAL if minimizing from arrange.
    // After setting the normal position, we'll arrange the window, and then
    // we'll minimize the window (setting the caller's minimize show command).
    wp.showCmd = isMinFromArranged ? SW_NORMAL : placement->showCmd;

    // If RestoreToMaximized, set WPF_RESTORETOMAXIMIZED.
    if (placement->HasFlag(PlacementFlags::RestoreToMaximized))
    {
        WI_SetFlag(wp.flags, WPF_RESTORETOMAXIMIZED);
    }

    // Call SetWindowPlacement.
    // This sets the normal position set above, and Min/Maximize state
    // depending on the caller's show command (and decisions above).
    if (!SetWindowPlacement(hwnd, &wp))
    {
        return false;
    }

    // If Arranging the window (and no failure from call above), make the
    // window arranged and move it into the adjusted arrange rect (fit to
    // the new monitor and expanded by the invisible resize area for this
    // window on its current monitor).
    if (isArranged)
    {
        // It is possible that we ended up Minimized/Maximized in the call
        // above, even if we requested SW_NORMAL. This can happen if this is
        // the first window shown by this process and the process was launched
        // Minimized (and the caller set the RestoreToArranged flag, indicating
        // the window should be Arranged and then Minimized).
        if (IsIconic(hwnd) || IsZoomed(hwnd))
        {
            ShowWindow(hwnd, SW_RESTORE);
        }

        // 'Double click' on the top resize border.
        // This causes the window to become arranged, snapping the top and
        // bottom edges of the window to it's monitor's work area.
        DefWindowProc(hwnd, WM_NCLBUTTONDBLCLK, HTTOP, 0);

        // The arrange rect is stored as frame bounds (without the invisible
        // resize borders, aka window margins). We need to add these invisible
        // resize borders back in, at the window's current DPI (now that it is
        // on the final monitor).
        RECT rcArrange = placement->arrangeRect;
        ExtendByMargins(&rcArrange, GetWindowMargins(hwnd));

        // Move the window to the adjusted arranged window position.
        SetWindowPos(
            hwnd,
            nullptr,
            rcArrange.left,
            rcArrange.top,
            RECTWIDTH(rcArrange),
            RECTHEIGHT(rcArrange),
            SWP_NOACTIVATE | SWP_NOZORDER);
    }

    // If minimizing but RestoreToArrange flag was set, we still need to
    // minimize the window (above we arranged it).
    if (isMinFromArranged)
    {
        ShowWindow(hwnd, placement->showCmd);
    }

    // FullScreen causes the window to be sized to the monitor, and have some
    // window styles removed.
    if (fullScreen)
    {
        placement->EnterFullScreen(hwnd);
    }

#ifdef USE_VIRTUAL_DESKTOP_APIS
    // If provided a virtual desktop ID, move the window to that virtual desktop.
    //
    // Note: This is done after showing/activating the window (on the current
    // virtual desktop). If we were to move the window to a background virtual
    // desktop prior to activating it, that would switch the current virtual
    // desktop.
    //
    // This assumes that the caller is restarting immediately after a reboot.
    // For normal app launch, where app does not want to ever launch in a
    // background virtual desktop, the VirtualDesktopId flag should not be set.
    if (setVirtualDesktop)
    {
        // TODO: Activation/Virtual desktop switch problems...
        //
        // SetWindowPlacement above has activated the window. In most cases
        // (if window ends up visible and on the current virtual desktop) this
        // is what we want. The window should activate and come to foreground
        // if possible.
        //
        //      Note: We could tweak the SW_ commands, but there is no way to
        //      Maximize without activating :(.
        //
        // If an app launches several windows, and some are on a background
        // virtual desktop, we MUST make sure to not cause a change in the
        // current/active virtual desktop. This is destructive, but unfortunately
        // happens if we attempt to move the foreground window to a background
        // virtual desktop.
        //
        // Another problem: IVirtualDesktopManager can check if a window is on
        // a background virtual desktop, (IsWindowOnCurrentVirtualDesktop), but
        // we cannot tell from the GUID alone if it will move the window to a
        // background virtual desktop. (So, by the time we can check if we're
        // moving to a background desktop it is too late to avoid doing so with
        // the foreground window.)
        //
        // Ideally, if an app launches several windows and ANY are on the active
        // virtual desktop, those windows would be activated (and foreground if
        // the app has foreground rights). But if the first window created ends
        // up on a background virtual desktop, the app has no other eligible
        // windows to make foreground when it must give it up on the one that
        // was just created (to move it to a background desktop).
        //
        // So, we call GetForegroundWindow prior to activating this window,
        // and here (before setting the virtual desktop ID), SetForegroundWindow
        // on that window, 'giving up' foreground rights (this app will launch
        // all windows without activating them). This NORMALLY works, but can
        // still show flashing or cause a virtual desktop switch (but it avoids
        // it in most cases).
        //
        // Ideas:
        // - Should the VD APIs handle this (enforcing that calling the APIs
        //      never changes the active virtual desktop?)
        //
        // - Should/can we ask if a GUID is a background desktop, and use it to
        //      avoid giving up foreground here if launching first window on the
        //      current desktop?
        //
        // - Should there be an Activate/NoActivate placement flag, which handles
        //      showing without activating separately from virtual desktops?
        if (prevFg)
        {
            SetForegroundWindow(prevFg);
        }

        MoveToVirtualDesktop(hwnd, placement->virtualDesktopId);
        // Note: Return value is true if window moved to a background desktop.
    }
#endif

    // Hide the window if KeepHidden flag and window started invisible.
    if (keepHidden)
    {
        ShowWindow(hwnd, SW_HIDE);
    }

    return true;
}

//
// PlacementParams
//
// This is used when creating a window, to pick the initial position of the
// window.
//
// Most apps will want to do a bit better than the default. This helper allows
// apps to opt into customized behavior:
//
// - Setting a custom fallback size (the size of the window the first time the
//      user ever launches the app).
//
// - Setting a registry key to store the last close position.
//
// - Adjusting behavior for 'restart' scenarios, where it is ok to launch
//      minimized or on a background virtual desktop.
//
// - Using another window as the position, cascading to keep the new position
//      from covering the previous window. This avoids launching two instances
//      of the app and ending up with two windows in the exact same place (one
//      covering the other).
//
class PlacementParams
{
    const SIZE _defaultSize;
    PlacementEx _placement = {};
    bool _isRestart = false;
    bool _allowOffscreen = false;
    StartupInfoFlags _startupInfoFlags = StartupInfoFlags::All;

public:
    // Apps should specify the default size, which is used only if no other
    // size is available (the default is 600 x 400).
    //
    // If the app saves its last close position in the registry using
    // PlacementEx::StorePlacementInRegistry, that registry key can be provided
    // here, to launch the window where it closed by default.
    PlacementParams(
        SIZE defSize = { 600, 400 },
        PCWSTR keyPath = nullptr,
        PCWSTR keyName = nullptr) : _defaultSize(defSize)
    {
        if (keyPath && keyName)
        {
            PlacementEx::FromRegistryKey(keyPath, keyName, &_placement);
        }
    }

    // Called if relaunching the app (as opposed to a 'normal' launch). This
    // allows the window to launch Minimized or on a background desktop (if
    // closed in that state).
    void SetIsRestart()
    {
        _isRestart = true;
    }

    // The Previous Window
    //
    // Apps that can launch many instances of itself should ideally not blindly
    // use the position in the registry. Else, launching many instances of the
    // app would 'pile up' (many windows would be in exactly the same place).
    //
    // Instead, if another instance is running, the new instance should 'cascade'
    // over the other window (down/right a bit, keeping both title bars visible).
    void FindPrevWindow(PCWSTR className)
    {
        HWND hwndPrev = FindWindow(className, nullptr);

        if (hwndPrev)
        {
            SetPrevWindow(hwndPrev);
        }
    }

    void SetPrevWindow(HWND hwndPrev)
    {
        // Ignore cloaked windows.
        if (!hwndPrev || IsCloaked(hwndPrev))
        {
            return;
        }

        PlacementEx placementT;
        if (PlacementEx::GetPlacement(hwndPrev, &placementT))
        {
            // Ignore FullScreen windows.
            // (There isn't a general way to replicate a FullScreen window's
            // restore position, unless we can ask that other instance for it.)
            if (!placementT.HasFlag(PlacementFlags::FullScreen))
            {
                placementT.Cascade();
                _placement = placementT;
            }
        }
    }

    // By default, the position is always entirely within the bounds of the
    // work area. Allowing offscreen skips this adjustment, but only if the new
    // position is at least 50% within the work area. (SetPlacement does not
    // allow moving a window more than 50% off screen.)
    void SetAllowPartiallyOffscreen()
    {
        _allowOffscreen = true;
    }

    // StartupInfo flags are set by default, and can be cleared.
    void ClearStartupInfoFlag(StartupInfoFlags flag)
    {
        WI_ClearAllFlags(_startupInfoFlags, flag);
    }

    // Positions the window and show it (make it visible). This should be done
    // after all setup is complete, and when the thread is ready to start
    // receiving messages.
    PlacementEx PositionAndShow(HWND hwnd)
    {
        // If no position set, start with the window's current position and set
        // the size using the default size.
        if (!_placement.IsValid() &&
            PlacementEx::GetPlacement(hwnd, &_placement))
        {
            _placement.SetLogicalSize(_defaultSize);
        }

        if (_placement.IsValid())
        {
            // If not a restart, make sure the window is not minimized (or cloaked).
            // This also modifies the placement as needed if any of the StartupInfo
            // flags are set (for example, a request to launch this app Maximized/
            // Minimized or on a particular monitor).
            if (!_isRestart)
            {
                _placement.AdjustForMainWindow(nullptr, _startupInfoFlags);
            }

            if (_allowOffscreen)
            {
                WI_SetFlag(_placement.flags, PlacementFlags::AllowPartiallyOffScreen);
            }

            // Set the initial window position and show the window.
            if (PlacementEx::SetPlacement(hwnd, &_placement))
            {
                return _placement;
            }
        }

        // Something failed above. Fall back to showing the window at it's current
        // position.
        ShowWindow(hwnd, SW_SHOW);
        PlacementEx::GetPlacement(hwnd, &_placement);
        return _placement;
    }
};

/* static */
inline void
PlacementEx::StorePlacementInRegistry(
    HWND hwnd,
    PCWSTR registryPath,
    PCWSTR registryKeyName)
{
    PlacementEx placement;
    if (GetPlacement(hwnd, &placement))
    {
        placement.StoreInRegistry(registryPath, registryKeyName);
    }
}

inline void
PlacementEx::StoreInRegistry(
    PCWSTR registryPath,
    PCWSTR registryKeyName)
{
    WriteStringRegKey(registryPath, registryKeyName, ToString());
}

inline void
PlacementEx::SetLogicalSize(SIZE size)
{
    normalRect.right = normalRect.left + MulDiv(size.cx, dpi, 96);
    normalRect.bottom = normalRect.top + MulDiv(size.cy, dpi, 96);
    normalRect = KeepRectOnMonitor(normalRect, workArea);
}

/* static */
inline void
PlacementEx::AddRemoveWindowStyles(
    HWND hwnd,
    bool add,
    LONG_PTR stylesToChange)
{
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (add)
    {
        style = style | stylesToChange;
    }
    else
    {
        style = style & ~stylesToChange;
    }

    SetWindowLongPtr(hwnd, GWL_STYLE, style);

    // Recompute the client rect (which may change depending on styles).
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
}

inline bool
PlacementEx::EnterFullScreen(
    HWND hwnd,
    LONG_PTR stylesToRemove)
{
    // If the FullScreen flag is not already set, update this placement to
    // the window's current position.
    if (!IsFullScreen() && !GetPlacement(hwnd, this))
    {
        return false;
    }

    // Get the monitor info.
    MonitorData monitor;
    if (!MonitorData::FromRect(normalRect, &monitor))
    {
        return false;
    }
    RECT rcMonitor = monitor.monitorRect;

    // Set the FullScreen flag.
    WI_SetFlag(flags, PlacementFlags::FullScreen);

    // Remove styles for window border and caption.
    AddRemoveWindowStyles(hwnd, false /* remove styles */, stylesToRemove);

    // Move the window to fit the monitor rect.
    // Also show the window (if it is hidden) and activate it.
    SetWindowPos(hwnd,
                 nullptr,
                 rcMonitor.left,
                 rcMonitor.top,
                 rcMonitor.right - rcMonitor.left,
                 rcMonitor.bottom - rcMonitor.top,
                 SWP_SHOWWINDOW);

    return true;
}

inline bool
PlacementEx::ExitFullScreen(
    HWND hwnd,
    LONG_PTR stylesToAdd)
{
    if (!IsFullScreen())
    {
        return false;
    }

    // Make sure the position is on the monitor the window is currently on.
    MoveToWindowMonitor(hwnd);

    // Clear the FullScreen flag.
    WI_ClearFlag(flags, PlacementFlags::FullScreen);

    // Add styles for window border and caption.
    AddRemoveWindowStyles(hwnd, true /* add styles */, stylesToAdd);

    // Move the window to the restore position (where it was before entering
    // FullScreen).
    SetPlacement(hwnd, this);

    return true;
}

/* static */
inline RECT
PlacementEx::KeepRectOnMonitor(
    const RECT& rcPrev,
    const RECT & workArea)
{
    RECT rc = rcPrev;

    // Check right/bottom before left/top.
    // This keeps the title bar (top-left of the window) on-screen in cases
    // where the new size is larger than the new monitor's work area.

    if (rc.right > workArea.right)
    {
        OffsetRect(&rc, workArea.right - rc.right, 0);
    }
    if (rc.left < workArea.left)
    {
        OffsetRect(&rc, workArea.left - rc.left, 0);
    }
    if (rc.bottom > workArea.bottom)
    {
        OffsetRect(&rc, 0, workArea.bottom - rc.bottom);
    }
    if (rc.top < workArea.top)
    {
        OffsetRect(&rc, 0, workArea.top - rc.top);
    }

    return rc;
}

inline void
PlacementEx::AdjustNormalRect(
    const RECT& rcWorkNew,
    UINT dpiNew)
{
    const RECT rcNormalPrev = normalRect;
    const RECT rcWorkPrev = workArea;
    const UINT dpiPrev = dpi;
    const int cxWorkPrev = RECTWIDTH(rcWorkPrev);
    const int cxWorkNew = RECTWIDTH(rcWorkNew);
    const int cyWorkPrev = RECTHEIGHT(rcWorkPrev);
    const int cyWorkNew = RECTHEIGHT(rcWorkNew);

    // Scale the offset from the monitor's work area by the difference in work
    // area size. This ensures that the position is roughly in the same place
    // if moved to a monitor with a very different size work area and back
    // (after forcing the position onto the work area if necessary).
    normalRect.left = rcWorkNew.left +
        MulDiv(rcNormalPrev.left - rcWorkPrev.left, cxWorkNew, cxWorkPrev);
    normalRect.top = rcWorkNew.top +
        MulDiv(rcNormalPrev.top - rcWorkPrev.top, cyWorkNew, cyWorkPrev);

    // Scale the size (width/height) from the old monitor's DPI to the new
    // monitor's DPI. This retains the logical window size.
    normalRect.right = normalRect.left +
        MulDiv(RECTWIDTH(rcNormalPrev), dpiNew, dpiPrev);
    normalRect.bottom = normalRect.top +
        MulDiv(RECTHEIGHT(rcNormalPrev), dpiNew, dpiPrev);

    // The AllowPartiallyOffScreen skips remaining adjustments, but only if the
    // new position is at least 50% within the new monitor's work area.
    if (HasFlag(PlacementFlags::AllowPartiallyOffScreen))
    {
        RECT rcI{};
        IntersectRect(&rcI, &normalRect, &rcWorkNew);
        UINT onscreenArea = RECTWIDTH(rcI) * RECTHEIGHT(rcI);
        UINT totalArea = RECTWIDTH(normalRect) * RECTHEIGHT(normalRect);

        if (onscreenArea > (totalArea / 2))
        {
            return;
        }
    }

    // Nudge the window to stay within the bounds of the work area.
    normalRect = KeepRectOnMonitor(normalRect, rcWorkNew);

    // If the window is now the same size as the work area or larger, pick a
    // new size using the previous size and work area. (If previously half the
    // monitor width, in the center, choose something similar fit to the new
    // monitor's work area.
    if (HasFlag(PlacementFlags::AllowSizing))
    {
        if (RECTWIDTH(normalRect) > cxWorkNew)
        {
            normalRect.left = rcWorkNew.left +
                MulDiv(rcNormalPrev.left - rcWorkPrev.left, cxWorkNew, cxWorkPrev);

            normalRect.right = rcWorkNew.right -
                MulDiv(rcWorkPrev.right - rcNormalPrev.right, cxWorkNew, cxWorkPrev);
        }

        if (RECTHEIGHT(normalRect) > cyWorkNew)
        {
            normalRect.top = rcWorkNew.top +
                MulDiv(rcNormalPrev.top - rcWorkPrev.top, cyWorkNew, cyWorkPrev);

            normalRect.bottom = rcWorkNew.bottom -
                MulDiv(rcWorkPrev.bottom - rcNormalPrev.bottom, cyWorkNew, cyWorkPrev);
        }
    }
}

inline void
PlacementEx::AdjustArrangeRect(
    const RECT rcWorkNew)
{
    const RECT rcWorkPrev = workArea;
    const LONG cxPrev = RECTWIDTH(rcWorkPrev);
    const LONG cyPrev = RECTHEIGHT(rcWorkPrev);
    const LONG cxNew = RECTWIDTH(rcWorkNew);
    const LONG cyNew = RECTHEIGHT(rcWorkNew);

    // Adjust each side of the window, keeping the window at the same relative
    // position wrt each monitor edge. Don't allow the window to be outside the
    // new work area (even if it was previously partially outside the monitor).
    arrangeRect.left = rcWorkNew.left +
        (std::max)(0, MulDiv((arrangeRect.left - rcWorkPrev.left), cxNew, cxPrev));

    arrangeRect.right = rcWorkNew.right -
        (std::max)(0, MulDiv((rcWorkPrev.right - arrangeRect.right), cxNew, cxPrev));

    arrangeRect.top = rcWorkNew.top +
        (std::max)(0, MulDiv((arrangeRect.top - rcWorkPrev.top), cyNew, cyPrev));

    arrangeRect.bottom = rcWorkNew.bottom -
        (std::max)(0, MulDiv((rcWorkPrev.bottom - arrangeRect.bottom), cyNew, cyPrev));
}

inline void
PlacementEx::MoveToMonitor(const MonitorData& targetMonitor)
{
    // Move the normal rect to the target monitor.
    AdjustNormalRect(targetMonitor.workArea, targetMonitor.dpi);

    // If the Arranged or RestoreToArranged flag are set, transform the arrange
    // rect from the previous work area to the new work area. This retains the
    // alignment with the work area edges (for example, left snapped).
    if (HasFlag(PlacementFlags::Arranged) ||
        HasFlag(PlacementFlags::RestoreToArranged))
    {
        AdjustArrangeRect(targetMonitor.workArea);
    }

    // This changes the DPI and monitor info this placement stores.
    workArea = targetMonitor.workArea;
    dpi = targetMonitor.dpi;
    StringCchCopy(deviceName, ARRAYSIZE(deviceName), targetMonitor.deviceName);
}

inline void
PlacementEx::Cascade()
{
    // Compute the height of the caption bar, using the DPI in the PlacementEx.
    // Note: This adds 2x the size of the border because the caption bar's true
    // height doesn't include the top resize area. (When we cascade a window,
    // we want the full caption bar on the previous window to be visible, and
    // using only the reported height of the caption bar from system metrics
    // would cause the window to visibly overlap the other window's title bar.)
    const UINT captionHeight =
        GetSystemMetricsForDpi(SM_CYCAPTION, dpi) +
        (2 * GetSystemMetricsForDpi(SM_CXSIZEFRAME, dpi));

    // Move to the right/down by the height of the caption bar.
    OffsetRect(&normalRect, captionHeight, captionHeight);

    // If new position is now past the right side of the work area, move it
    // back to the left side of the work area.
    if (normalRect.right > workArea.right)
    {
        OffsetRect(&normalRect, workArea.left - normalRect.left, 0);
    }

    // If new position is now past the bottom of the work area, move it back
    // to the left side of the work area.
    if (normalRect.bottom > workArea.bottom)
    {
        OffsetRect(&normalRect, 0, workArea.top - normalRect.top);
    }
}

/* static */
inline bool
PlacementEx::CascadeOver(HWND hwndMove, HWND hwndOver)
{
    PlacementEx placementOver;
    if (GetPlacement(hwndOver, &placementOver))
    {
        placementOver.Cascade();
        return SetPlacement(hwndMove, &placementOver);
    }
    return false;
}

inline void
PlacementEx::AdjustForMainWindow(
    _In_opt_ STARTUPINFO* psi,
    StartupInfoFlags siFlags)
{
    // If Minimized, switch to restored (or Maximized).
    RestoreIfMinimized();

    // Do not launch on a background virtual desktop (cloaked).
    WI_ClearFlag(flags, PlacementFlags::VirtualDesktopId);

    // Apply startup info parameters (start Max/Min or Monitor Hint).
    AdjustForStartupInfo(psi, siFlags);
}

inline void
PlacementEx::AdjustForStartupInfo(
    _In_opt_ STARTUPINFO* psi,
    StartupInfoFlags siFlags)
{
    STARTUPINFO si{};

    // Read the startup info if one was not provided explicitly.
    if (!psi)
    {
        si.cb = sizeof(si);
        GetStartupInfo(&si);
        psi = &si;
    }

    // If the monitor hint is set, move the stored position to the hint monitor.
    MonitorData monitor;
    if (WI_IsFlagSet(siFlags, StartupInfoFlags::MonitorHint) &&
        MonitorData::FromHandle((HMONITOR)psi->hStdOutput, &monitor))
    {
        MoveToMonitor(monitor);
    }

    // Set the show command if the flag is set (both by the caller and the
    // startup info).
    // This notably handles cases where the startup info requests Maximized or
    // Minimized. The other show commands, SW_NORMAL, SW_SHOWDEFAULT, are NOT
    // used. This ensures a Maximized window that is closed and launched in a
    // default manner relaunches Maximized (not at the normal position).
    // Also check for show commands like SW_HIDE and SW_SHOWNA, though these
    // flags are not generally used with the startup info.
    if (WI_IsFlagSet(siFlags, StartupInfoFlags::ShowCommand) &&
        WI_IsFlagSet(psi->dwFlags, STARTF_USESHOWWINDOW))
    {
        switch (psi->wShowWindow)
        {
            case SW_HIDE:
                WI_SetFlag(flags, PlacementFlags::KeepHidden);
                break;

            case SW_SHOWNOACTIVATE:
            case SW_SHOWNA:
                WI_SetFlag(flags, PlacementFlags::NoActivate);
                break;

            case SW_NORMAL:
            case SW_SHOW:
            case SW_RESTORE:
            case SW_SHOWDEFAULT:
                // Keep the existing show command.
                break;

            default:
                SetShowCommand(psi->wShowWindow);
        }
    }
}

/* static */
inline bool
PlacementEx::IsMinimizeShowCmd(UINT cmd)
{
    switch (cmd)
    {
        case SW_SHOWMINNOACTIVE:
        case SW_SHOWMINIMIZED:
        case SW_MINIMIZE:
            return true;
    }
    return false;
}

/* static */
inline bool
PlacementEx::IsRestoreShowCmd(UINT cmd)
{
    switch (cmd)
    {
        case SW_NORMAL:
        case SW_RESTORE:
        case SW_SHOWDEFAULT:
            return true;
    }
    return false;
}

inline void
PlacementEx::SetShowCommand(UINT newShowCmd)
{
    // If Restoring a Minimized placement with 'restore to maximize' flag, swap
    // the new show command to SW_MAXIMIZE and clear the restore to maximize flag.
    if (IsRestoreShowCmd(newShowCmd) &&
        IsMinimizeShowCmd(showCmd) &&
        WI_IsFlagSet(flags, PlacementFlags::RestoreToMaximized))
    {
        newShowCmd = SW_MAXIMIZE;
        WI_ClearFlag(flags, PlacementFlags::RestoreToMaximized);
    }

    // If Maximized and now Minimizing, set the 'restore to maximize' flag.
    if (IsMinimizeShowCmd(newShowCmd) &&
        (showCmd == SW_MAXIMIZE))
    {
        WI_SetFlag(flags, PlacementFlags::RestoreToMaximized);
    }

    // If arranged and now Minimizing, set 'restore to arranged' flag instead
    // of 'arranged'.
    if (IsMinimizeShowCmd(newShowCmd) &&
        WI_IsFlagSet(flags, PlacementFlags::Arranged))
    {
        WI_ClearFlag(flags, PlacementFlags::Arranged);
        WI_SetFlag(flags, PlacementFlags::RestoreToArranged);
    }

    showCmd = newShowCmd;
}

/* static */
inline bool
PlacementEx::FromRegistryKey(
    PCWSTR registryPath,
    PCWSTR registryKeyName,
    _Out_ PlacementEx* placement)
{
    std::wstring placementStr = ReadStringRegKey(registryPath, registryKeyName);

    return !placementStr.empty() && FromString(placementStr, placement);
}

// TODO: Something better for To/FromString()...
// - maybe switch to PCWSTR?
//      StringCchPrintf
//      see %SDXROOT%\clientcore\windows\tools\uiext\util.cpp
//      _ParseDecimalUINT
//      limit numbers to 16 bit


constexpr PCWSTR GUID_FORMAT_STR =
    L"%08X-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX";

inline void StringToGuid(const std::wstring& guidStr, _Out_ GUID* guid)
{
  swscanf(guidStr.c_str(), GUID_FORMAT_STR,
    &guid->Data1, &guid->Data2, &guid->Data3,
    &guid->Data4[0], &guid->Data4[1], &guid->Data4[2], &guid->Data4[3],
    &guid->Data4[4], &guid->Data4[5], &guid->Data4[6], &guid->Data4[7]);
}

inline std::wstring GuidToString(const GUID& guid)
{
    return wil::str_printf<std::wstring>(
        GUID_FORMAT_STR,
        guid.Data1,
        guid.Data2,
        guid.Data3,
        guid.Data4[0],
        guid.Data4[1],
        guid.Data4[2],
        guid.Data4[3],
        guid.Data4[4],
        guid.Data4[5],
        guid.Data4[6],
        guid.Data4[7]
    );
}

// Serializes the placement information into a string.
inline std::wstring
PlacementEx::ToString() const
{
    // Serialize the data to a string.
    // 15 integers then a string, separated by commas:
    // - [left,top,right,bottom] normal rect
    // - [left,top,right,bottom] work area
    // - dpi
    // - show command
    // - placement flags
    // - [left,top,right,bottom] arrange rect
    // - virtual desktop GUID
    // - device name

#ifdef USE_VIRTUAL_DESKTOP_APIS
    std::wstring guidString = GuidToString(virtualDesktopId);

    return wil::str_printf<std::wstring>(
        L"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ws,%ws\n",
        normalRect.left,
        normalRect.top,
        normalRect.right,
        normalRect.bottom,
        workArea.left,
        workArea.top,
        workArea.right,
        workArea.bottom,
        dpi,
        showCmd,
        flags,
        arrangeRect.left,
        arrangeRect.top,
        arrangeRect.right,
        arrangeRect.bottom,
        guidString.c_str(),
        deviceName);
#else
    return wil::str_printf<std::wstring>(
        L"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%ws\n",
        normalRect.left,
        normalRect.top,
        normalRect.right,
        normalRect.bottom,
        workArea.left,
        workArea.top,
        workArea.right,
        workArea.bottom,
        dpi,
        showCmd,
        flags,
        arrangeRect.left,
        arrangeRect.top,
        arrangeRect.right,
        arrangeRect.bottom,
        deviceName);
#endif

}

// Converts a string (produced by PlacementEx::ToString()) into a PlacementEx.
/* static */
inline std::optional<PlacementEx>
PlacementEx::FromString(const std::wstring& placementString)
{
    if (placementString.empty())
    {
        return std::nullopt;
    }

    std::wstring delim = L",";

    #define NUM_INTEGERS_IN_PLACEMENT_STRING 15
    int parsedInts[NUM_INTEGERS_IN_PLACEMENT_STRING];

    PlacementEx pex;
    try
    {
        auto start = 0U;
        auto end = placementString.find(delim);
        int index = 0;
#ifdef USE_VIRTUAL_DESKTOP_APIS
        GUID virtualDesktopId = {};
#endif

        while (end != std::wstring::npos)
        {
            std::wstring token = placementString.substr(start, end - start);

            start = (UINT)(end + delim.length());
            end = placementString.find(delim, start);

            if (index == NUM_INTEGERS_IN_PLACEMENT_STRING)
            {
#ifdef USE_VIRTUAL_DESKTOP_APIS
                StringToGuid(token, &virtualDesktopId);
#endif
                break;
            }

            parsedInts[index] = stoi(token);

            index++;
        }

        if (index != NUM_INTEGERS_IN_PLACEMENT_STRING)
        {
            return std::nullopt;
        }

        // The remainder of the string is the deviceName.
        std::wstring deviceName = placementString.substr(
            start, placementString.length() - start - 1);
        // TODO: registry string has a newline, or a '.'?
        // its important this string matches what we read live:
        // - close, change primary monitor, relaunch to same position

        pex.normalRect.left = parsedInts[0];
        pex.normalRect.top = parsedInts[1];
        pex.normalRect.right = parsedInts[2];
        pex.normalRect.bottom = parsedInts[3];
        pex.workArea.left = parsedInts[4];
        pex.workArea.top = parsedInts[5];
        pex.workArea.right = parsedInts[6];
        pex.workArea.bottom = parsedInts[7];
        pex.dpi = parsedInts[8];
        pex.showCmd = parsedInts[9];
        pex.flags = static_cast<PlacementFlags>(parsedInts[10]);
        pex.arrangeRect.left = parsedInts[11];
        pex.arrangeRect.top = parsedInts[12];
        pex.arrangeRect.right = parsedInts[13];
        pex.arrangeRect.bottom = parsedInts[14];
#ifdef USE_VIRTUAL_DESKTOP_APIS
        pex.virtualDesktopId = virtualDesktopId;
#endif

        StringCchCopy(pex.deviceName,
            ARRAYSIZE(pex.deviceName), deviceName.c_str());
    }
    catch(std::invalid_argument const& /* ex */)
    {
        return std::nullopt;
    }

    return pex;
}
