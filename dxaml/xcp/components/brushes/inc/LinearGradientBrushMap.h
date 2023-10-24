// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DataStructureFunctionSpecializations.h>
#include <XMap.h>
#include <fwd/windows.ui.composition.h>

// A Xaml LinearGradientBrush may need to create multiple WUC CompositionLinearGradientBrushes if absolute mapping modes or
// relative transforms are involved. Each WUC brush corresponds to a different sized element using the brush. This class sorts
// out the mapping between a size and a brush.
class LinearGradientBrushMap
{
public:
    wrl::ComPtr<WUComp::ICompositionBrush> GetBrush(
        _In_ const XRECTF& brushBounds,
        _In_ WUComp::ICompositor4* compositor);

    void ReleaseDCompResources();

    xchainedmap<XSIZEF, wrl::ComPtr<WUComp::ICompositionBrush>> m_map;
};
