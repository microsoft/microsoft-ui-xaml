// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "winrt/Microsoft.UI.Composition.SystemBackdrops.h"
#include <winrt/windows.system.h>

#include "DesktopAcrylicBackdrop.g.h"

using namespace winrt::Microsoft::UI::Composition;
using namespace winrt::Microsoft::UI::Composition::SystemBackdrops;

class DesktopAcrylicBackdrop :
    public ReferenceTracker<DesktopAcrylicBackdrop, winrt::implementation::DesktopAcrylicBackdropT>
{
public:
    DesktopAcrylicBackdrop() = default;
    virtual ~DesktopAcrylicBackdrop() = default;

    void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot);
    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop disconnectedTarget);

private:
    class ControllerEntry
    {
    public:
        ControllerEntry(ICompositionSupportsSystemBackdrop target, DesktopAcrylicController controller, SystemBackdropConfiguration configuration);
        ~ControllerEntry();

        // Block copy and assignment. This class is meant to be constructed in-place in the list.
        ControllerEntry(const ControllerEntry& other) = delete;
        ControllerEntry& operator=(const ControllerEntry& other) = delete;

        ICompositionSupportsSystemBackdrop m_target;
        DesktopAcrylicController m_controller;
    };

    std::vector<std::unique_ptr<ControllerEntry>> m_controllers;
};