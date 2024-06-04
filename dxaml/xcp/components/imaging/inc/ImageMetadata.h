// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wincodec.h>

struct ImageMetadata
{
    GUID containerFormat = {};

    // width/height fields with orientation already applied
    uint32_t width = 0;
    uint32_t height = 0;

    // The scale percentage associated with the resource that provided
    // the encoded image data. 0 if there is no associated scale.
    uint32_t scalePercentage = 0;

    uint32_t frameCount = 0;
    uint32_t loopCount = 0;

    WICBitmapTransformOptions orientation = WICBitmapTransformRotate0;
    
    bool supportsAlpha = true;
    
    bool isSvg = false;

    bool isHdr = false;

    bool AreDimensionsSwapped() const
    {
        // 90 and 270 rotation have bit 0 set to 1.
        return ((orientation & WICBitmapTransformRotate90) != 0);
    }

    bool IsAnimatedImage() const
    {
        return frameCount > 1;
    }
};
