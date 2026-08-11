// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <dwmapi.h>
#include <memory>

// Asks DWM to draw one of its system backdrop materials behind a Xaml Window, and puts the window back the way
// it was when this object goes away.
//
// Xaml normally draws Mica and desktop Acrylic with a lifted SystemBackdropController, which renders the
// material into the backdrop's target. Those controllers can only render through the lifted compositor, so in a
// process that opted into the system composition engine (CompositionEngine.TrySetProcessEngine) they have
// nothing to render into and the backdrop silently doesn't appear. DWM draws the same materials for a window
// directly - the recipe the system provides for every other window - which is what Xaml falls back to.
//
// Only Xaml's own Windows are configured this way, because the DWM attribute covers a whole top-level window:
// see TryApplyToXamlWindow.
class DwmSystemBackdrop
{
public:
    // Asks DWM to draw 'backdropType' behind the Xaml Window that 'systemBackdrop' is set on and that 'target'
    // covers. Returns null when the backdrop isn't attached to a Xaml Window that way, or when DWM doesn't
    // support the attribute - callers then fall back to the lifted SystemBackdropController.
    static std::unique_ptr<DwmSystemBackdrop> TryApplyToXamlWindow(
        const winrt::Microsoft::UI::Xaml::Media::SystemBackdrop& systemBackdrop,
        const winrt::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop& target,
        const winrt::Microsoft::UI::Xaml::XamlRoot& xamlRoot,
        DWM_SYSTEMBACKDROP_TYPE backdropType);

    ~DwmSystemBackdrop();

    // Block copy and assignment. This object owns a window's DWM configuration, which only one owner can undo.
    DwmSystemBackdrop(const DwmSystemBackdrop& other) = delete;
    DwmSystemBackdrop& operator=(const DwmSystemBackdrop& other) = delete;

    // Switches the window to a different material, for instance when MicaBackdrop.Kind changes. Leaves the
    // window alone once an app has taken the attribute over, just like the destructor does.
    void BackdropType(DWM_SYSTEMBACKDROP_TYPE backdropType);

private:
    DwmSystemBackdrop(HWND windowHandle, DWM_SYSTEMBACKDROP_TYPE previousBackdropType, DWM_SYSTEMBACKDROP_TYPE backdropType);

    // Whether the window is still configured with what we last applied to it.
    bool IsLastWriter() const;

    HWND m_windowHandle{ nullptr };

    // What the window was configured with before we touched it, restored on the way out so that an app that set
    // the attribute itself keeps its choice.
    DWM_SYSTEMBACKDROP_TYPE m_previousBackdropType{ DWMSBT_AUTO };

    // What we last configured the window with, so we can tell whether we are still the last writer.
    DWM_SYSTEMBACKDROP_TYPE m_appliedBackdropType{ DWMSBT_AUTO };
};
