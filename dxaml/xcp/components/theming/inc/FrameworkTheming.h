// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xstring_ptr.h>
#include <NamespaceAliases.h>
#include <fwd/Microsoft.UI.Xaml.h>

struct IThemingInterop;

namespace Theming {
    enum class Theme : uint8_t;
}

struct ColorAndBrushResourceInfo
{
    xstring_ptr         ColorKey;
    xstring_ptr         BrushKey;
    unsigned int        rgbValue;
    bool                OverrideAlpha;
};

class FrameworkTheming
{
public:
    typedef std::function<HRESULT()> NotifyThemeChangedFunc;

    FrameworkTheming(
        const std::shared_ptr<IThemingInterop>& spThemingInterop,
        NotifyThemeChangedFunc notifyThemeChangedFunc
        );
    FrameworkTheming(const FrameworkTheming&) = delete;

    _Check_return_ HRESULT OnThemeChanged(bool forceUpdate = false);

    _Check_return_ HRESULT SetRequestedTheme(xaml::ApplicationTheme requestedTheme, bool doNotifyThemeChange = true);
    _Check_return_ HRESULT SetRequestedTheme(Theming::Theme requestedTheme, bool doNotifyThemeChange = true);
    bool HasRequestedTheme() const;
    void UnsetRequestedTheme();

    bool HasHighContrastTheme() const;

    // Represents the theme that the framework is using.  It is a combination
    // of the requested theme (if not set, then the system theme) and the
    // high-contrast theme.
    Theming::Theme GetTheme() const;

    // Helpers to get the base and high-constrast components to the theme.
    Theming::Theme GetBaseTheme() const;
    Theming::Theme GetHighContrastTheme() const;

    const std::vector<ColorAndBrushResourceInfo>& GetColorAndBrushResourceInfoList() const
    {
        return m_colorAndBrushResourceInfoList;
    }

    void DisableHighContrastUpdateOnThemeChange()
    {
        m_disableHighContrastUpdateOnThemeChange = true;
    }

    UINT32 GetRootVisualBackground() const;
    UINT32 GetHwndBackground(Theming::Theme) const;

    bool IsBaseThemeChanging() const { return m_isSystemThemeChanging || m_isAppThemeChanging; }

    bool IsHighContrastChanging() const { return m_isHighContrastChanging; }

private:
    void RebuildColorAndBrushResources();

    std::shared_ptr<IThemingInterop> m_spThemingInterop;
    NotifyThemeChangedFunc           m_notifyThemeChangedFunc;

    Theming::Theme m_requestedTheme;
    Theming::Theme m_highContrastTheme;
    Theming::Theme m_systemTheme;

    unsigned int m_accentColor;

    // High contrast isn't updated in the OnThemeChanged handler when we're in a background task.
    bool m_disableHighContrastUpdateOnThemeChange : 1;

    bool m_isAppThemeChanging : 1;
    bool m_isSystemThemeChanging : 1;

    bool m_isHighContrastChanging : 1;

    std::vector<ColorAndBrushResourceInfo> m_colorAndBrushResourceInfoList;

}; // class FrameworkTheming
