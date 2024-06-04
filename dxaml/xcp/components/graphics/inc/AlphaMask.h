// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/Windows.UI.Composition.h>

class CD2DFactory;

// This is the AlphaMask used for drop shadows (and potentially other composition based effects)
class AlphaMask final
{
public:
    AlphaMask();
    ~AlphaMask();

    // Ensure will allocate any necessary surfaces including the composition
    // brush and rasterize the content.
    _Check_return_ HRESULT Ensure(_In_ CUIElement* pUIElement);

    _Check_return_ HRESULT UpdateIfAvailable(_In_ CUIElement* pUIElement);

    void Hide();

    bool IsPresent() const;

    wrl::ComPtr<WUComp::ICompositionBrush> GetCompositionBrush();

    static _Check_return_ HRESULT RasterizeFill(_In_ CUIElement* pUIElement, const CMILMatrix& realizationScale, const XPOINTF* shapeMaskOffset, const bool renderCollapsedMask, _In_ DCompSurface* surface);
    static _Check_return_ HRESULT RasterizeStroke(_In_ CUIElement* pUIElement, const CMILMatrix& realizationScale, const XPOINTF* shapeMaskOffset, const bool renderCollapsedMask, _In_ DCompSurface* surface);

private:

    class Impl;

    std::unique_ptr<Impl> m_pImpl;
};