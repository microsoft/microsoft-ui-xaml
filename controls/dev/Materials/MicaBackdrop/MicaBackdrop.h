// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <winrt/windows.system.h>

#include "MicaBackdrop.g.h"
#include "MicaBackdrop.properties.h"

using namespace winrt::Windows::UI::Composition;

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
};