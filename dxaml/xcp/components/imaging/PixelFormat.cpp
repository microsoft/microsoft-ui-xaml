// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <PixelFormat.h>

//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function that returns a single pixel's plane stride in bits.
//
//      For instance, the YV12 format has an 8-bit Y plane followed by two
//      8-bit 2x2 sub-sampled U and V planes. Even though the average amount
//      of information per pixel is 12 bits (8*W*H+8*(W/2)*(H/2)*2 = 12*W*H),
//      a single pixel's stride in each of the planes is still one byte.
//
//      See GetPlaneStride below.
//
//------------------------------------------------------------------------------
uint32_t GetPixelPlaneBitStride(PixelFormat pixelFormat)
{
    uint32_t pixelPlaneBitStride = 0;

    switch (pixelFormat)
    {
    case pixelGray1bpp:
        pixelPlaneBitStride = 1;
        break;

    case pixelGray8bpp:
        pixelPlaneBitStride = 8;
        break;

    case pixelColor32bpp_A8R8G8B8:
        pixelPlaneBitStride = 32;
        break;

    case pixelColor64bpp_R16G16B16A16_Float:
        pixelPlaneBitStride = 64;
        break;
    }

    ASSERT(pixelPlaneBitStride != 0);

    return pixelPlaneBitStride;
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function that returns a plane's stride in bytes, assuming
//      that each row starts on a byte boundary.
//
//      For instance, the YV12 format has an 8-bit Y plane followed by two
//      8-bit 2x2 sub-sampled U and V planes. Stride of the sub-sampled
//      planes is half the stride of the Y plane.
//
//      See GetPixelPlaneBitStride above.
//
//------------------------------------------------------------------------------
uint32_t
GetPlaneStride(
    PixelFormat pixelFormat,
    uint32_t plane,
    uint32_t width
    )
{
    uint32_t planeStride =
        (GetPixelPlaneBitStride(pixelFormat) * width + 7) >> 3;

    switch (pixelFormat)
    {
    case pixelGray1bpp:
    case pixelGray8bpp:
    case pixelColor32bpp_A8R8G8B8:
    case pixelColor64bpp_R16G16B16A16_Float:
        ASSERT(plane == 0);
        break;

    default:
        // Unhandled format
        ASSERT(FALSE);
        break;
    }

    return planeStride;
}

uint32_t GetPixelStride(PixelFormat pixelFormat)
{
    return GetPixelPlaneBitStride(pixelFormat) / 8;
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      A helper function to verify a buffer's size sufficiency to store
//      a bitmap of the specified pixel format and dimensions.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
ValidateSurfaceBufferSize(
    PixelFormat pixelFormat,
    uint32_t width,
    uint32_t height,
    int32_t stride,
    uint32_t bufferSize
    )
{
    uint32_t requiredBufferSize = 0;
    uint32_t unsignedStride = 0;

    const uint32_t planeZeroStride = GetPlaneStride(pixelFormat, 0 /* plane */, width);

    IFC_RETURN(Int32ToUInt32(stride, &unsignedStride));

    if (unsignedStride < planeZeroStride)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(UInt32Mult(
        unsignedStride,
        height,
        &requiredBufferSize));

    if (bufferSize < requiredBufferSize)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}
