// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerProtocol.h"

namespace WinUIProfiler::Hooks
{
    enum class GraphicsSurfaceOperation : uint8_t
    {
        Create, Destroy, OwnershipRelease, Close, Rebind, Resize, Share, BaselinePresent
    };

    void GraphicsDeviceCreated(const void* deviceOwner) noexcept;
    void GraphicsDeviceReleased(const void* deviceOwner, bool lost) noexcept;
    // deviceOwner is a CD3D11DeviceInstance, not a borrowed COM identity.
    // Null means the device association/backing is unavailable.
    void GraphicsSurfaceChanged(const void* surface, const void* deviceOwner,
        GraphicsSurfaceOperation operation, uint32_t width, uint32_t height, HRESULT result = S_OK) noexcept;
}
