#pragma once
// Note: Intended to be included by User32Utils.h.

#include <vector>

//
// MonitorData
//
// A MonitorData has information about a monitor (HMONITOR). This is similar
// to the MONITORINFOEX struct, but it also includes the DPI.
//

class MonitorData
{
public:
    // These helpers use MonitorFrom(Point/Rect/Window) to pick the nearest
    // monitor, and returns a MonitorData with info about that monitor.
    // Note: HMONITORs can become invalid at any time. Callers are expected
    // to check the return value and handle failure.
    static bool FromHandle(HMONITOR monitorHandle, _Out_ MonitorData* monitorData) noexcept;
    static bool FromPoint(POINT pt, _Out_ MonitorData* monitorData) noexcept;
    static bool FromRect(RECT rc, _Out_ MonitorData* monitorData) noexcept;
    static bool FromWindow(HWND hwnd, _Out_ MonitorData* monitorData) noexcept;
    static bool FromDeviceName(PCWSTR deviceName, _Out_ MonitorData* monitorData) noexcept;

    // Enumerates every currently connected monitor and returns a MonitorData
    // for each one. Order matches EnumDisplayMonitors enumeration order, which
    // is OS-defined and does NOT necessarily match any other monitor index
    // ordering (e.g. softgpu target index).
    static std::vector<MonitorData> GetMonitorList() noexcept;

    // Compare two monitors to see if they are different in ANY way.
    // The MonitorData is information about a monitor at a point in time. If
    // the monitor changes (for example resolution or DPI changes, but handle
    // and name do not), this will detect that the monitors are NOT equal.
    bool operator==(const MonitorData& otherMonitor) const noexcept
    {
        return Equals(otherMonitor);
    }
    bool operator!=(const MonitorData& otherMonitor) const noexcept
    {
        return !Equals(otherMonitor);
    }
    bool Equals(const MonitorData& otherMonitor) const noexcept;

    // Compares only the device name (the string) for two monitors. This is
    // used when finding a monitor that is 'best' to use, given a monitor from
    // the past. (If the monitor from the past changed, it will likely have the
    // same device name, but other fields have changed.)
    bool MatchesDeviceName(PCWSTR otherDeviceName) const noexcept;

private:
    static BOOL MonitorEnumProc(HMONITOR, HDC, PRECT, LPARAM);

public:
    // The HMONITOR is the system's handle for this monitor, which can be used
    // by APIs that take a monitor, like GetMonitorInfo.
    // WARNING: This handle can become invalid at any time! It is best to read
    // all needed data about a monitor before making any changes, and not
    // assume that the handle is still valid later.
    HMONITOR handle = nullptr;

    // The monitor rect is the position/size of the monitor. These values are
    // in screen coordinates, and show the size of each monitor (resolution),
    // as well as the relative position of each monitor. There is always one
    // monitor, the primary monitor, whose origin (top left corner) is 0,0.
    //                              ┌─────────────────────────┐
    //                              │ Primary (0, 0)          │
    //                              │ [0, 0, 1920, 1080]      │
    //                              │                         │
    //   ┌────────────────────────┐ │                         │
    //   │ Secondary (1280, 960)  │ │                         │
    //   │ [-1280, 800, 0, 1760]  │ │                         │
    //   │                        │ └─────────────────────────┘
    //   │                        │
    //   │                        │
    //   │                        │
    //   └────────────────────────┘
    RECT monitorRect = {};

    // The work area is a subset of the monitor rect. This is the part of the
    // monitor that is not covered by the taskbar (or taskbar-like apps).
    // Non-topmost windows should generally stay within the work area of their
    // monitor.
    RECT workArea = {};

    // The DPI is used for calculating the monitor's scale factor (the density
    // of the pixels that the window's output is being displayed on). All UI
    // drawn by an app should be scaled by the DPI scale factor:
    //
    //  If the logical height of a button is 25, and the DPI is 192 (200%),
    //  the physical height of the button is 50: MulDiv(25, 192, 96).
    //
    // WARNING: While it is often the case that a window's DPI matches the DPI
    // of the monitor it is mostly on, this is NOT always true! Apps should use
    // GetDpiForWindow in most cases to know what scale factor a window should
    // be using, or listen for WM_DPICHANGED to know when the DPI changes after
    // the window is created. Apps should NOT assume that finding the DPI of the
    // monitor that the window position is currently on will be the same. (In
    // cases where the monitors change, or when a window is dragged between
    // monitors, there are times when these two DPIs disagree, and using the
    // monitor DPI can cause the window to get stuck at the wrong DPI!)
    UINT dpi = 0;

    // The device name is a string representing the monitor. In cases where a
    // monitor's position, handle, and other fields change (like changing
    // primary monitor), the device id remains stable.
    WCHAR deviceName[CCHDEVICENAME] = {};
};

/* static */
inline bool
MonitorData::FromHandle(HMONITOR monitorHandle, _Out_ MonitorData* monitorData) noexcept
{
    MONITORINFOEX mi = { sizeof(mi) };
    // Note: GetDpiForMonitor returns x/y DPI but these values are always equal.
    UINT _dpi, unused;

    if (!GetMonitorInfo(monitorHandle, &mi) ||
        (GetDpiForMonitor(monitorHandle, MDT_EFFECTIVE_DPI, &_dpi, &unused) != S_OK))
    {
        return false;
    }

    monitorData->handle = monitorHandle;
    monitorData->monitorRect = mi.rcMonitor;
    monitorData->workArea = mi.rcWork;
    monitorData->dpi = _dpi;

    if (FAILED(StringCchCopy(
            monitorData->deviceName, ARRAYSIZE(monitorData->deviceName), mi.szDevice)))
    {
        return false;
    }

    return true;
}

/* static */
inline bool
MonitorData::FromPoint(POINT pt, _Out_ MonitorData* monitorData) noexcept
{
    return FromHandle(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), monitorData);
}

/* static */
inline bool
MonitorData::FromRect(RECT rc, _Out_ MonitorData* monitorData) noexcept
{
    return FromHandle(MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST), monitorData);
}

/* static */
inline bool
MonitorData::FromWindow(HWND hwnd, _Out_ MonitorData* monitorData) noexcept
{
    return FromHandle(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), monitorData);
}

struct DeviceNameSearchData
{
    PCWSTR deviceName;
    MonitorData result;
    bool resultSet = false;
};

/* static */
inline BOOL
MonitorData::MonitorEnumProc(
    HMONITOR handle,
    HDC,
    PRECT,
    LPARAM lParam)
{
    DeviceNameSearchData* data = reinterpret_cast<DeviceNameSearchData*>(lParam);

    MonitorData monitor;
    if (FromHandle(handle, &monitor))
    {
        if (monitor.MatchesDeviceName(data->deviceName))
        {
            data->result = monitor;
            data->resultSet = true;
            return FALSE;
        }
    }

    return TRUE;
}

/* static */
inline bool
MonitorData::FromDeviceName(
    PCWSTR deviceName,
    _Out_ MonitorData* monitorData) noexcept
{
    DeviceNameSearchData data;
    data.deviceName = deviceName;

    EnumDisplayMonitors(nullptr, nullptr,
        MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

    if (data.resultSet)
    {
        *monitorData = data.result;
        return true;
    }

    return false;
}

/* static */
inline BOOL CALLBACK
GetMonitorListEnumProc(
    HMONITOR handle,
    HDC,
    PRECT,
    LPARAM lParam)
{
    auto* monitors = reinterpret_cast<std::vector<MonitorData>*>(lParam);

    MonitorData monitor;
    if (MonitorData::FromHandle(handle, &monitor))
    {
        monitors->push_back(monitor);
    }
    // Skip monitors whose handle became invalid between EnumDisplayMonitors
    // and FromHandle, but keep enumerating.
    return TRUE;
}

/* static */
inline std::vector<MonitorData>
MonitorData::GetMonitorList() noexcept
{
    std::vector<MonitorData> monitors;
    EnumDisplayMonitors(nullptr, nullptr,
        GetMonitorListEnumProc, reinterpret_cast<LPARAM>(&monitors));
    return monitors;
}

inline bool
MonitorData::MatchesDeviceName(PCWSTR otherDeviceName) const noexcept
{
    return (wcscmp(deviceName, otherDeviceName) == 0);
}

inline bool
MonitorData::Equals(const MonitorData& otherMonitor) const noexcept
{
    // If provided a reference to the same address as this monitor, we
    // know all of the fields are the same (we don't need to check).
    if (this == &otherMonitor)
    {
        return true;
    }

    if (dpi != otherMonitor.dpi)
    {
        return false;
    }

    if (!EqualRect(&monitorRect, &otherMonitor.monitorRect))
    {
        return false;
    }

    if (!EqualRect(&workArea, &otherMonitor.workArea))
    {
        return false;
    }

    if (!MatchesDeviceName(otherMonitor.deviceName))
    {
        return false;
    }

    // All fields match; the provided monitor data is equivalent to this one.
    return true;
}
