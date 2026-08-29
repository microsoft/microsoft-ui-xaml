// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomWicBitmap.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

CustomWicBitmap::CustomWicBitmap(
    bool supportsTransparency,
    unsigned int width,
    unsigned int height,
    uint32_t colorValue1,
    uint32_t colorValue2
    )
    : m_refCounter(0)
    , m_supportsTransparency(supportsTransparency)
    , m_width(width)
    , m_height(height)
    , m_dpiX(72.0)
    , m_dpiY(72.0)
    , m_pBuffer(nullptr)
    , m_locked(false)
    , m_pendingLock(false)
{
    m_pBuffer = new uint32_t[width * height];

    InitializeWithGradientPattern(m_pBuffer, width, height, colorValue1, colorValue2);

    // Note: Only pre-multiplied alpha is supported in Jupiter, therefore must use GUID_WICPixelFormat32bppPBGRA
    //       instead of GUID_WICPixelFormat32bppBGRA.
    m_wicPixelFormat = m_supportsTransparency ? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGR;
}

void CustomWicBitmap::InitializeWithGradientPattern(
    uint32_t* pPixel,
    unsigned int width,
    unsigned int height,
    uint32_t colorValue1,
    uint32_t colorValue2
    )
{
    uint32_t a1 = GetAlpha(colorValue1);
    uint32_t r1 = GetRed(colorValue1);
    uint32_t g1 = GetGreen(colorValue1);
    uint32_t b1 = GetBlue(colorValue1);

    uint32_t a2 = GetAlpha(colorValue2);
    uint32_t r2 = GetRed(colorValue2);
    uint32_t g2 = GetGreen(colorValue2);
    uint32_t b2 = GetBlue(colorValue2);

    double maxT = 1.5 * static_cast<double>(width) + static_cast<double>(height);

    for (unsigned int y = 0; y < height; y++)
    {
        for (unsigned int x = 0; x < width; x++, pPixel++)
        {
            // Have a slight slope to the gradient so it doesn't mirror with swapped x/y
            double t = (1.5 * static_cast<double>(x) + static_cast<double>(y)) / maxT;
            double t_complement = (1 - t);

            *pPixel = BuildColor(
                static_cast<uint32_t>(a1 * t_complement + a2 * t),
                static_cast<uint32_t>(r1 * t_complement + r2 * t),
                static_cast<uint32_t>(g1 * t_complement + g2 * t),
                static_cast<uint32_t>(b1 * t_complement + b2 * t)
                );
        }
    }
}

} } } } } } }
