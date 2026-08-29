// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OfferableMemory.h"
#include "OfferableSoftwareBitmap.h"
#include "OfferableSoftwareBitmapUnitTests.h"
#include "PixelFormat.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

void OfferableSoftwareBitmapUnitTests::AllocateWrite()
{
    uint32_t widths[] =
    {
        1,
        10,
        100,
        1000,
    };

    uint32_t heights[] =
    {
        1,
        10,
        100,
        1000,
    };

    for (uint32_t widthIndex = 0; widthIndex < ARRAYSIZE(widths); widthIndex++)
    {
        for (uint32_t heightIndex = 0; heightIndex < ARRAYSIZE(heights); heightIndex++)
        {
            OfferableSoftwareBitmap softwareBitmap(
                pixelColor32bpp_A8R8G8B8,
                widths[widthIndex],
                heights[heightIndex]);

            // Write to full buffer size
            memset(softwareBitmap.GetBuffer(), 1, softwareBitmap.GetBufferSize());

            // Verify the implied buffer size is the same as the returned buffer size.
            uint32_t impliedBufferSize = softwareBitmap.GetWidth() * softwareBitmap.GetHeight() * 4;
            VERIFY_ARE_EQUAL(impliedBufferSize, softwareBitmap.GetBufferSize());
        }
    }
}

} } } } } }
