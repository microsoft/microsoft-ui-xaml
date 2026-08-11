// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "DwmSystemBackdrop.h"

// This needs to be here instead of in CppWinRTIncludes.h because it includes Microsoft.UI.h, which contains
// definitions of a few types like WindowId that conflict with their definitions in Microsoft.UI.Content.h.
#include <winrt\Microsoft.UI.Interop.h>

namespace
{
    // Reads the material DWM currently draws behind 'windowHandle'. Returns false before Windows 11 22H2, where
    // the attribute doesn't exist and there is no DWM-drawn backdrop to speak of.
    bool TryGetWindowBackdropType(HWND windowHandle, DWM_SYSTEMBACKDROP_TYPE& backdropType)
    {
        backdropType = DWMSBT_AUTO;
        return SUCCEEDED(DwmGetWindowAttribute(windowHandle, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType)));
    }

    bool TrySetWindowBackdropType(HWND windowHandle, DWM_SYSTEMBACKDROP_TYPE backdropType)
    {
        return SUCCEEDED(DwmSetWindowAttribute(windowHandle, DWMWA_SYSTEMBACKDROP_TYPE, &backdropType, sizeof(backdropType)));
    }

    // Returns the handle of the Xaml Window whose backdrop 'systemBackdrop' is, given that it was just connected
    // to 'target' inside 'xamlRoot'. Returns null for every other way a SystemBackdrop can be attached.
    //
    // The DWM attribute covers a whole top-level window, so it is only ever the right answer when Xaml owns that
    // window and the backdrop is meant to fill all of it. Three conditions together say exactly that:
    //
    //  - The target has to be a XamlIsland. Window hosts its content in a DesktopWindowXamlSource, whose island
    //    fills the window's client area. Windowed popups connect the Popup itself, and SystemBackdropElement and
    //    CommandBarFlyout connect a ContentExternalBackdropLink; none of those cover a window.
    //  - The Window hosting that island has to be one of the app's Xaml Windows and have this very SystemBackdrop
    //    set on it. That rules out islands the app hosts in a window of its own, which Xaml doesn't own and may
    //    not reconfigure.
    //  - That island has to be the Window's own. XamlRoot.Content unwraps the WindowChrome that Xaml puts at the
    //    root of a Window's island, so for the Window's island it is the same element as Window.Content, while an
    //    island the app nested inside the Window has content of its own. Without this, an island nested inside a
    //    Window would take the whole window over as soon as it shared the Window's backdrop object.
    HWND TryGetXamlWindowForTarget(
        const winrt::Microsoft::UI::Xaml::Media::SystemBackdrop& systemBackdrop,
        const winrt::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop& target,
        const winrt::Microsoft::UI::Xaml::XamlRoot& xamlRoot)
    {
        if (!xamlRoot || !target.try_as<winrt::Microsoft::UI::Xaml::XamlIsland>())
        {
            return nullptr;
        }

        // Absent for an island that is being torn down, or one hosted outside a top-level window.
        const auto islandEnvironment = xamlRoot.ContentIslandEnvironment();
        if (!islandEnvironment)
        {
            return nullptr;
        }

        // Null in a host that drives Xaml through WindowsXamlManager instead of an Application.
        const auto application = winrt::Application::Current();
        if (!application)
        {
            return nullptr;
        }

        const auto hostWindowId = islandEnvironment.AppWindowId();
        const auto islandContent = xamlRoot.Content();

        for (const auto& window : application.as<winrt::IFrameworkApplicationPrivate>().Windows())
        {
            const auto appWindow = window.AppWindow();
            if (appWindow &&
                appWindow.Id().Value == hostWindowId.Value &&
                window.SystemBackdrop() == systemBackdrop &&
                window.Content() == islandContent)
            {
                return winrt::Microsoft::UI::GetWindowFromWindowId(hostWindowId);
            }
        }

        return nullptr;
    }
}

/* static */ std::unique_ptr<DwmSystemBackdrop> DwmSystemBackdrop::TryApplyToXamlWindow(
    const winrt::Microsoft::UI::Xaml::Media::SystemBackdrop& systemBackdrop,
    const winrt::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop& target,
    const winrt::Microsoft::UI::Xaml::XamlRoot& xamlRoot,
    DWM_SYSTEMBACKDROP_TYPE backdropType)
{
    const HWND windowHandle = TryGetXamlWindowForTarget(systemBackdrop, target, xamlRoot);
    if (!windowHandle)
    {
        return nullptr;
    }

    // Remember the window's current material before overwriting it, so that an app that configured the window
    // itself gets its choice back once this backdrop is detached.
    DWM_SYSTEMBACKDROP_TYPE previousBackdropType = DWMSBT_AUTO;
    if (!TryGetWindowBackdropType(windowHandle, previousBackdropType) ||
        !TrySetWindowBackdropType(windowHandle, backdropType))
    {
        return nullptr;
    }

    return std::unique_ptr<DwmSystemBackdrop>(new DwmSystemBackdrop(windowHandle, previousBackdropType, backdropType));
}

DwmSystemBackdrop::DwmSystemBackdrop(HWND windowHandle, DWM_SYSTEMBACKDROP_TYPE previousBackdropType, DWM_SYSTEMBACKDROP_TYPE backdropType)
    : m_windowHandle(windowHandle)
    , m_previousBackdropType(previousBackdropType)
    , m_appliedBackdropType(backdropType)
{
}

DwmSystemBackdrop::~DwmSystemBackdrop()
{
    // Undo only our own configuration. If the app has written the attribute behind our back it is the last
    // writer and gets to keep what it asked for.
    DWM_SYSTEMBACKDROP_TYPE currentBackdropType = DWMSBT_AUTO;
    if (TryGetWindowBackdropType(m_windowHandle, currentBackdropType) && currentBackdropType == m_appliedBackdropType)
    {
        TrySetWindowBackdropType(m_windowHandle, m_previousBackdropType);
    }
}

void DwmSystemBackdrop::BackdropType(DWM_SYSTEMBACKDROP_TYPE backdropType)
{
    if (backdropType != m_appliedBackdropType && TrySetWindowBackdropType(m_windowHandle, backdropType))
    {
        m_appliedBackdropType = backdropType;
    }
}
