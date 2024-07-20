// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "winrt/Microsoft.UI.Composition.SystemBackdrops.h"
#include <winrt/windows.system.h>

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

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    class ControllerEntry
    {
    public:
        ControllerEntry(ICompositionSupportsSystemBackdrop target, MicaController controller, SystemBackdropConfiguration configuration);
        ~ControllerEntry();

        // Block copy and assignment. This class is meant to be constructed in-place in the list.
        ControllerEntry(const ControllerEntry& other) = delete;
        ControllerEntry& operator=(const ControllerEntry& other) = delete;

        ICompositionSupportsSystemBackdrop m_target;
        MicaController m_controller;
    };

    std::vector<std::unique_ptr<ControllerEntry>> m_controllers;
};