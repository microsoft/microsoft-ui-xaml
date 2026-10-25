// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerGraphicsHooks.h"

namespace WinUIProfiler::Hooks
{
    enum class GraphicsResourceKind : uint8_t { Texture2D = 1, CompositionSurface = 2, CompositionBrush = 3 };

    void GraphicsResourceChanged(const void* resource, const void* deviceOwner, GraphicsResourceKind kind,
        GraphicsSurfaceOperation operation, uint32_t width, uint32_t height, HRESULT result = S_OK,
        const void* xamlOwner = nullptr) noexcept;
}
