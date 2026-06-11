// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Enumeration used to specify pixel format on surfaces
enum PixelFormat
{
    pixelUnknown = 0,
    pixelGray1bpp,
    pixelGray8bpp,              // Could be alpha video channel

    //
    // Types used for image decoding.
    //
    pixelColor32bpp_A8R8G8B8,
    pixelColor64bpp_R16G16B16A16_Float,

    PixelFormatDWORDALIGN = 0x7fffffff
};

uint32_t GetPixelPlaneBitStride(PixelFormat pixelFormat);

uint32_t GetPlaneStride(PixelFormat pixelFormat, uint32_t plane, uint32_t width);

uint32_t GetPixelStride(PixelFormat pixelFormat);

_Check_return_ HRESULT
ValidateSurfaceBufferSize(
    PixelFormat pixelFormat,
    uint32_t width,
    uint32_t height,
    int32_t stride,
    uint32_t bufferSize
    );
