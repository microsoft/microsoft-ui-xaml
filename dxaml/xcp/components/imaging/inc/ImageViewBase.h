// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <vector>
#include "ImagingTelemetry.h"

struct IImageViewListener;

// Image views allow accessing image data asynchronously.
// Once the view is created the data may or may not be available on the view immediately.
// All image views share the same event subscription and error handling pattern.
// ImageViewBase provides a common way to
//    - get error status if the view had failed to expose the requested data
//    - receive notifications when the view has new data or error status available

class ImageViewBase
{
public:
    void AddImageViewListener(IImageViewListener& imageViewListener);
    void RemoveImageViewListener(IImageViewListener& imageViewListener);

    // Depending on the view type this may signal errors from:
    // * URI parsing - formatting
    // * Download - access, network issues, corrupted media
    // * Decode - unsupported format, data corruption
    // * Upload - driver errors
    virtual HRESULT GetHR(uint64_t imageId) const = 0;

protected:
    std::vector<IImageViewListener*> m_imageViewListeners;
    void TriggerViewUpdated();
};
