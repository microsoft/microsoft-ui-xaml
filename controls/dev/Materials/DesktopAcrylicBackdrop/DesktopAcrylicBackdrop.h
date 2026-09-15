// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <winrt/windows.system.h>

#include "DesktopAcrylicBackdrop.g.h"

using namespace winrt::Windows::UI::Composition;

class DesktopAcrylicBackdrop :
    public ReferenceTracker<DesktopAcrylicBackdrop, winrt::implementation::DesktopAcrylicBackdropT>
{
public:
    DesktopAcrylicBackdrop() = default;
    virtual ~DesktopAcrylicBackdrop() = default;

    void OnTargetConnected(ICompositionSupportsSystemBackdrop connectedTarget, winrt::Microsoft::UI::Xaml::XamlRoot xamlRoot);
    void OnTargetDisconnected(ICompositionSupportsSystemBackdrop disconnectedTarget);

private:
};