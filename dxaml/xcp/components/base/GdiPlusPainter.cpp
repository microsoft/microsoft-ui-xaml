// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>

// GDI+ immediate painter (CPU, antialiased, PARGB). Owned by the immediate-rendering-gdiplus branch.
namespace RenderingBackend
{
    HRESULT CreateGdiPlusPainter(const PainterOptions&, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        painter.reset();
        return E_NOTIMPL;
    }
}
