// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RevealBrushTestApi.g.h"

class RevealBrushTestApi :
    public ReferenceTracker<RevealBrushTestApi, winrt::implementation::RevealBrushTestApiT, winrt::composable>
{
public:
    winrt::RevealBrush RevealBrush();
    void RevealBrush(winrt::RevealBrush const& value);

    bool IsInFallbackMode();
    bool IsAmbientLightSet();
    bool IsBorderLightSet();
    bool IsHoverLightSet();
    winrt::CompositionBrush CompositionBrush();
    winrt::CompositionBrush NoiseBrush();

private:
    winrt::RevealBrush m_revealBrush{ nullptr };
};
