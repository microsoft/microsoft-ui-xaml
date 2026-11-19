// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>

// Direct2D immediate painter shared by D2D-HW and D2D-SW; the device decides hardware versus WARP.
// Owned by the immediate-rendering-d2d-sw branch.
namespace RenderingBackend
{
    HRESULT CreateD2DPainter(Mode, ID3D11Device*, const PainterOptions&, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        painter.reset();
        return E_NOTIMPL;
    }
}
