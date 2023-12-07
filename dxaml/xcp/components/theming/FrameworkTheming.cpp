// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkTheming.h"
#include "Theme.h"
#include "ThemingInterop.h"
#include <xamlbehaviormode.h>
#include <MUX-ETWEvents.h>

using namespace Theming;

FrameworkTheming::FrameworkTheming(
    const std::shared_ptr<IThemingInterop>& spThemingInterop,
    NotifyThemeChangedFunc notifyThemeChangedFunc
)
    : m_spThemingInterop(spThemingInterop)
    , m_notifyThemeChangedFunc(notifyThemeChangedFunc)
    , m_requestedTheme(Theming::Theme::None)
    , m_highContrastTheme(Theming::Theme::HighContrastNone)
    , m_systemTheme(Theming::Theme::Dark)
    , m_accentColor(0x00000000)
    , m_disableHighContrastUpdateOnThemeChange(false)
    , m_isSystemThemeChanging(false)
    , m_isHighContrastChanging(false)
    , m_isAppThemeChanging(false)
{
    m_systemTheme = m_spThemingInterop->GetSystemTheme();
}

_Check_return_ HRESULT
FrameworkTheming::OnThemeChanged(bool forceUpdate)
{
    TraceThemeChangedStart();

    auto oldTheme = GetTheme();
    auto oldThemeSystem = m_systemTheme;
    auto oldThemeHighContrast = m_highContrastTheme;
    auto oldAccentColor = m_accentColor;

    m_systemTheme = m_spThemingInterop->GetSystemTheme();

    if (!m_disableHighContrastUpdateOnThemeChange)
    {
        m_highContrastTheme = m_spThemingInterop->GetSystemHighContrastTheme();
    }

    m_accentColor = m_spThemingInterop->GetSystemAccentColor();

    // Only do an update if something has actually changed, or if forced to.
    if (forceUpdate || oldTheme != GetTheme() || oldAccentColor != m_accentColor)
    {
        m_isSystemThemeChanging = oldThemeSystem != m_systemTheme;
        m_isHighContrastChanging = (oldThemeHighContrast != m_highContrastTheme);
        auto scopeGuard = wil::scope_exit([&]
        {
            m_isSystemThemeChanging = false;
            m_isHighContrastChanging = false;
        });
        RebuildColorAndBrushResources();
        IFC_RETURN(m_notifyThemeChangedFunc());
    }

    TraceThemeChangedStop();

    return S_OK;
}

_Check_return_ HRESULT
FrameworkTheming::SetRequestedTheme(xaml::ApplicationTheme requestedTheme, bool doNotifyThemeChange)
{
    return SetRequestedTheme(requestedTheme == xaml::ApplicationTheme_Light ? Theme::Light : Theme::Dark, doNotifyThemeChange);
}

_Check_return_ HRESULT
FrameworkTheming::SetRequestedTheme(Theming::Theme requestedTheme, bool doNotifyThemeChange)
{
    // The input theme should be a base theme.
    ASSERT((requestedTheme & Theme::HighContrastMask) == Theme::HighContrastNone);

    if (requestedTheme != m_requestedTheme)
    {
        auto oldDefaultTheme = GetBaseTheme();

        m_requestedTheme = requestedTheme;

        // Don't notify about theme switches when we're in high contrast.
        if (doNotifyThemeChange && !HasHighContrastTheme())
        {
            // Force a theme update to get around the perf optimization
            // that tests whether the theme actually changes after refreshing
            // the system theme if we're setting a requested theme, otherwise
            // the theme will never change in response to changes to the
            // system theme (because requested theme takes precedence) and
            // we'll end up not notifying the core.
            bool forceUpdate = (m_requestedTheme != Theme::None);

            m_isAppThemeChanging = oldDefaultTheme != m_requestedTheme;
            auto scopeGuard = wil::scope_exit([&] { m_isAppThemeChanging = false; });
            IFC_RETURN(OnThemeChanged(forceUpdate));
        }
    }

    return S_OK;
}

bool
FrameworkTheming::HasRequestedTheme() const
{
    return m_requestedTheme != Theme::None;
}

bool
FrameworkTheming::HasHighContrastTheme() const
{
    return m_highContrastTheme != Theme::HighContrastNone;
}

Theme
FrameworkTheming::GetTheme() const
{
    ASSERT(m_systemTheme != Theme::None);
    return (m_requestedTheme != Theme::None ? m_requestedTheme : m_systemTheme) | m_highContrastTheme;
}

Theme
FrameworkTheming::GetBaseTheme() const
{
    return GetTheme() & Theme::BaseMask;
}

Theme
FrameworkTheming::GetHighContrastTheme() const
{
    return GetTheme() & Theme::HighContrastMask;
}

void FrameworkTheming::RebuildColorAndBrushResources()
{
    const bool overrideAlpha = true;

    static const xstring_ptr_storage colorKeyStorage[] =
    {
        // System color resources
        XSTRING_PTR_STORAGE(L"SystemColorActiveCaptionColor"),
        XSTRING_PTR_STORAGE(L"SystemColorBackgroundColor"),
        XSTRING_PTR_STORAGE(L"SystemColorButtonFaceColor"),
        XSTRING_PTR_STORAGE(L"SystemColorButtonTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorCaptionTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorGrayTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorHighlightColor"),
        XSTRING_PTR_STORAGE(L"SystemColorHighlightTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorHotlightColor"),
        XSTRING_PTR_STORAGE(L"SystemColorInactiveCaptionColor"),
        XSTRING_PTR_STORAGE(L"SystemColorInactiveCaptionTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorWindowColor"),
        XSTRING_PTR_STORAGE(L"SystemColorWindowTextColor"),
        XSTRING_PTR_STORAGE(L"SystemColorDisabledTextColor"),

        // Accent color resources
        XSTRING_PTR_STORAGE(L"SystemColorControlAccentColor"),
        XSTRING_PTR_STORAGE(L"SystemAccentColor"),

        // Variant accent color resources
        XSTRING_PTR_STORAGE(L"SystemAccentColorDark1"),
        XSTRING_PTR_STORAGE(L"SystemAccentColorDark2"),
        XSTRING_PTR_STORAGE(L"SystemAccentColorDark3"),
        XSTRING_PTR_STORAGE(L"SystemAccentColorLight1"),
        XSTRING_PTR_STORAGE(L"SystemAccentColorLight2"),
        XSTRING_PTR_STORAGE(L"SystemAccentColorLight3"),

        // List selection color resources.
        XSTRING_PTR_STORAGE(L"SystemListAccentLowColor"),
        XSTRING_PTR_STORAGE(L"SystemListAccentMediumColor"),
        XSTRING_PTR_STORAGE(L"SystemListAccentHighColor")
    };

    static const xstring_ptr_storage brushKeyStorage[] =
    {
        // System color resources
        XSTRING_PTR_STORAGE(L"SystemColorActiveCaptionBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorBackgroundBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorButtonFaceBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorButtonTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorCaptionTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorGrayTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorHighlightBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorHighlightTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorHotlightBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorInactiveCaptionBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorInactiveCaptionTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorWindowBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorWindowTextBrush"),
        XSTRING_PTR_STORAGE(L"SystemColorDisabledTextBrush"),

        // Accent color resources
        XSTRING_PTR_STORAGE(L"SystemColorControlAccentBrush"),
        xstring_ptr_storage::NullString(),

        // Variant accent color resources
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),

        // List selection color resources.
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString(),
        xstring_ptr_storage::NullString()
    };
    static_assert(ARRAY_SIZE(brushKeyStorage) == ARRAY_SIZE(colorKeyStorage), "Array does not match number of color resources.");

    // Dynamic values.
    unsigned int colorValues[] =
    {
        // SystemColor resources
        m_spThemingInterop->GetSystemColor(COLOR_ACTIVECAPTION),
        m_spThemingInterop->GetSystemColor(COLOR_BACKGROUND),
        m_spThemingInterop->GetSystemColor(COLOR_BTNFACE),
        m_spThemingInterop->GetSystemColor(COLOR_BTNTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_CAPTIONTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_GRAYTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_HIGHLIGHT),
        m_spThemingInterop->GetSystemColor(COLOR_HIGHLIGHTTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_HOTLIGHT),
        m_spThemingInterop->GetSystemColor(COLOR_INACTIVECAPTION),
        m_spThemingInterop->GetSystemColor(COLOR_INACTIVECAPTIONTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_WINDOW),
        m_spThemingInterop->GetSystemColor(COLOR_WINDOWTEXT),
        m_spThemingInterop->GetSystemColor(COLOR_GRAYTEXT),

        // Accent color resources
        m_accentColor,
        m_accentColor,

        // Variant accent color resources
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark1),
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark2),
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorDark3),
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight1),
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight2),
        m_spThemingInterop->GetSystemVariantAccentColor(IThemingInterop::VariantAccentColors::SystemAccentColorLight3),

        // List selection color resources (Accent color RGB values, different Alpha-channel values).
        (0x66FFFFFF & m_accentColor),
        (0x99FFFFFF & m_accentColor),
        (0xB2FFFFFF & m_accentColor)
    };
    static_assert(ARRAY_SIZE(colorValues) == ARRAY_SIZE(colorKeyStorage), "Array does not match number of color resources.");

    bool overrideAlphaValues[] =
    {
        // SystemColor resources
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,
        overrideAlpha,

        // Accent color resources
        false,
        false,

        // Variant accent color resources
        false,
        false,
        false,
        false,
        false,
        false,

        // List selection color resources (Accent color RGB values, different Alpha-channel values).
        false,
        false,
        false
    };
    static_assert(ARRAY_SIZE(overrideAlphaValues) == ARRAY_SIZE(colorKeyStorage), "Array does not match number of color resources.");

    m_colorAndBrushResourceInfoList.clear();
    for (size_t i = 0; i < ARRAY_SIZE(colorKeyStorage); ++i)
    {
        ColorAndBrushResourceInfo info =
        {
            XSTRING_PTR_FROM_STORAGE(colorKeyStorage[i]),
            XSTRING_PTR_FROM_STORAGE(brushKeyStorage[i]),
            colorValues[i],
            overrideAlphaValues[i]
        };

        m_colorAndBrushResourceInfoList.push_back(info);
    }
}

// this one only takes in account app requested theme, base system theme and highcontrast theme
UINT32 FrameworkTheming::GetRootVisualBackground() const
{
    return GetHwndBackground(Theming::Theme::None);
}

// HWND's background is decided this way :
// 1. if highcontrast theme is applied, it overrides everything all
// 2. else if top level element (XamlRoot.Content) has a requested theme property set, pick actualtheme of that (because requestedtheme can be overriden)
// 3. else use the base theme
UINT32 FrameworkTheming::GetHwndBackground(const Theming::Theme xamlRootTheme) const
{
    UINT32 backgroundColor;
    // In threshold, we set the background color to better match the app's base theme.
    // This minimizes jarring transitions such as when in light theme and doing a
    // page navigation transition, which used to show the black in the background
    // as the white page rotated out of the way.

    if (HasHighContrastTheme())
    {
        backgroundColor = m_spThemingInterop->GetSystemColor(COLOR_WINDOW);
    }
    else
    {
        Theming::Theme theme = Theming::Theme::None;
        theme = xamlRootTheme != Theming::Theme::None ? xamlRootTheme : GetBaseTheme();
        backgroundColor = (theme == Theme::Light  ? 0xffffffff : 0xff000000);
    }

    return backgroundColor;
}

void FrameworkTheming::UnsetRequestedTheme()
{
    m_requestedTheme = Theming::Theme::None;
}
