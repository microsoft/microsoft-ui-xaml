// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImageViewBase.h"

class CWindowRenderTarget;
class CD3D11Device;
struct ImageMetadata;

class ImageMetadataView : public ImageViewBase
{
public:
    // ImageMetadata may be null during download
    virtual const ImageMetadata* GetImageMetadata() const = 0;
    virtual float GetDownloadProgress() const = 0;
    virtual void SetGraphicsDevice(_In_opt_ CD3D11Device* graphicsDevice) = 0;
    virtual wf::Size GetMaxRootSize() = 0;
    virtual void SetMaxRootSize(wf::Size maxRootSize) = 0;
};
