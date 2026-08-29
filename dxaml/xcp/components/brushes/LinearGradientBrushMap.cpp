// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LinearGradientBrushMap.h"

#include <microsoft.ui.composition.h>

wrl::ComPtr<WUComp::ICompositionBrush> LinearGradientBrushMap::GetBrush(
    _In_ const XRECTF& brushBounds,
    _In_ WUComp::ICompositor4* compositor)
{
    XSIZEF elementSize = { brushBounds.Width, brushBounds.Height };

    if (!m_map.ContainsKey(elementSize))
    {
        wrl::ComPtr<WUComp::ICompositionLinearGradientBrush> linearGradientBrush;
        IFCFAILFAST(compositor->CreateLinearGradientBrush(linearGradientBrush.ReleaseAndGetAddressOf()));

        wrl::ComPtr<WUComp::ICompositionBrush> gradientAsCBrush;
        IFCFAILFAST(linearGradientBrush.As(&gradientAsCBrush));
        IFCFAILFAST(m_map.Add(elementSize, gradientAsCBrush));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> brush;
    IFCFAILFAST(m_map.Get(elementSize, brush));

    return brush;
}

void LinearGradientBrushMap::ReleaseDCompResources()
{
    m_map.Clear();
}