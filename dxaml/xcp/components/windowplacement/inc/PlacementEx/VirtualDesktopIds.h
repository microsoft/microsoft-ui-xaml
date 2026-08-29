#pragma once
// Note: Included by User32Utils.h, if USE_VIRTUAL_DESKTOP_APIS is defined.

//
// This file has wrapper functions used by PlacementEx.h to use virtual desktops.
// https://learn.microsoft.com/en-us/windows/win32/api/shobjidl_core/nn-shobjidl_core-ivirtualdesktopmanager
//
// This requires:
// - initializing COM, by running this on each thread:
//      CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
// - linking against:
//      $(ONECOREUAP_INTERNAL_SDK_LIB_PATH)\onecoreuapuuid.lib
//      $(ONECORE_INTERNAL_PRIV_SDK_LIB_PATH_L)\OneCore_Forwarder_ole32.lib
// - Not querying window state from a SendMessage call
//      (the virtual desktop APIs fail in this case...)
//
// These APIs require linking to additional binaries, and can also (because
// of COM usage) lead to deadlocks depending on how the APIs are called. See
// VirtualDesktopHelper in chromium, which moves these calls to a background
// thread for this reason:
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/views/frame/browser_desktop_window_tree_host_win.cc
//
// TODO: Maybe add a helper like VirtualDesktopHelper from chromium?
// - manage background thread that initializes COM, and allows calls from
//      within a SendMessage call.
// - dynamically link com dlls?
// - RegisterCloakedNotification/WM_CLOAKED_STATE_CHANGED?
//

// Uncomment the below if debugging virtual desktops issues:
// #define BREAK_ON_VIRTUAL_DESKTOP_FAILURE

// Returns the GUID for the virtual desktop that a window belongs to.
inline bool
GetVirtualDesktopId(
    HWND hwnd,
    _Out_opt_ GUID* desktopId)
{
    Microsoft::WRL::ComPtr<IVirtualDesktopManager> spVirtualDesktopManager;

    HRESULT hr = CoCreateInstance(__uuidof(VirtualDesktopManager),
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IVirtualDesktopManager,
        (void**)&spVirtualDesktopManager);

    if (hr != S_OK)
    {
        // Note: The API above fails if the current thread needs to call:
        // CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#ifdef BREAK_ON_VIRTUAL_DESKTOP_FAILURE
        __debugbreak();
#endif
        return false;
    }

    // Get the ID (GUID) of the window's virtual desktop.
    hr = spVirtualDesktopManager->GetWindowDesktopId(hwnd, desktopId);
    if (hr != S_OK)
    {
#ifdef BREAK_ON_VIRTUAL_DESKTOP_FAILURE
        // ELEMENTNOTFOUND is expected if newly created window queries its
        // desktop ID (if taskbar isn't aware of the window yet).
        if (hr != TYPE_E_ELEMENTNOTFOUND)
        {
            // Note: The call above fails if called while handling a SendMessage,
            // error code: RPC_E_CANTCALLOUT_ININPUTSYNCCALL
            __debugbreak();
        }
#endif
        return false;
    }

    return true;
}

// Moves a window to a Virtual Desktop, specified by the virtual desktop GUID.
// This returns true if the window was successfully moved AND if the window is
// now on a background (not active) virtual desktop.
inline bool
MoveToVirtualDesktop(
    HWND hwnd,
    const GUID& desktopId)
{
    Microsoft::WRL::ComPtr<IVirtualDesktopManager> spVirtualDesktopManager;

    HRESULT hr = CoCreateInstance(__uuidof(VirtualDesktopManager),
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IVirtualDesktopManager,
        (void**)&spVirtualDesktopManager);

    // Note: Virtual desktop IDs are best effort.
    // They may fail for legit reasons, like user deleted the virtual desktop.

    if (hr != S_OK)
    {
        return false;
    }

    hr = spVirtualDesktopManager->MoveWindowToDesktop(hwnd, desktopId);

    if (hr != S_OK)
    {
        return false;
    }

    BOOL isCurrentDesktop = TRUE;
    hr = spVirtualDesktopManager->IsWindowOnCurrentVirtualDesktop(hwnd, &isCurrentDesktop);

    if (hr != S_OK)
    {
        return false;
    }

    // Return true if the window is now on a background virtual desktop.
    return !isCurrentDesktop;
}
