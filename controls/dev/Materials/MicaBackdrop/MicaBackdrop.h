// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "winrt/Microsoft.UI.Composition.SystemBackdrops.h"
#include <winrt/windows.system.h>
#include <optional>

#include "DwmSystemBackdrop.h"
#include "MicaBackdrop.g.h"
#include "MicaBackdrop.properties.h"

using namespace winrt::Microsoft::UI::Composition;
using namespace winrt::Microsoft::UI::Composition::SystemBackdrops;

class MicaBackdrop :
    public ReferenceTracker<MicaBackdrop, winrt::implementation::MicaBackdropT>,
    public MicaBackdropProperties
{
public:
    MicaBackdrop() = default;
    virtual ~MicaBackdrop() = default;

    void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot);
    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop disconnectedTarget);
    void OnDefaultSystemBackdropConfigurationChanged(ICompositionSupportsSystemBackdrop target, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    // Whether the material should be drawn dark for 'configuration'. A DWM-drawn backdrop needs this told to it
    // explicitly, where a lifted MicaController reads the configuration itself.
    static bool IsDarkTheme(const SystemBackdropConfiguration& configuration);

    // Mica attached to a single target. The material is drawn either by a lifted MicaController, which renders
    // it into the target, or - on the system composition engine, where that controller has nothing to render
    // into - by DWM for the Xaml Window the target covers. Exactly one of the two is set.
    class TargetEntry
    {
    public:
        TargetEntry(ICompositionSupportsSystemBackdrop target, MicaController controller, SystemBackdropConfiguration configuration);
        TargetEntry(ICompositionSupportsSystemBackdrop target, std::unique_ptr<DwmSystemBackdrop> dwmBackdrop);
        ~TargetEntry();

        // Block copy and assignment. This class is meant to be constructed in-place in the list.
        TargetEntry(const TargetEntry& other) = delete;
        TargetEntry& operator=(const TargetEntry& other) = delete;

        const ICompositionSupportsSystemBackdrop& Target() const { return m_target; }

        // Moves this target onto the material for 'kind'. 'dwmBackdropType' is that kind's DWM equivalent, and
        // is empty for a kind that has none.
        void UpdateKind(MicaKind kind, std::optional<DWM_SYSTEMBACKDROP_TYPE> dwmBackdropType);

        // Moves this target onto the light or dark variant of the material. A no-op for a lifted controller,
        // which follows the SystemBackdropConfiguration it was given.
        void UpdateDarkMode(bool useDarkMode);

    private:
        ICompositionSupportsSystemBackdrop m_target;
        MicaController m_controller{ nullptr };
        std::unique_ptr<DwmSystemBackdrop> m_dwmBackdrop;
    };

    std::vector<std::unique_ptr<TargetEntry>> m_targets;
};