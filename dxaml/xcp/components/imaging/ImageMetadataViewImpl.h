// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <ImageMetadataView.h>
#include <memory>

class EncodedImageData;
class CD3D11Device;

// ImageMetadataView contains methods for read-only access to the image metadata.
// This class provides the implementation that actually hosts the view data.
// ImageCache uses this private implementation API to expose the image metadata to the view consumers.
class ImageMetadataViewImpl final : public ImageMetadataView
{
public:
    void SetEncodedImageData(_In_opt_ std::shared_ptr<EncodedImageData> encodedImageData, HRESULT downloadStatus);
    void SetDownloadProgress(float downloadProgress);

    // ImageMetadataView
    const ImageMetadata* GetImageMetadata() const override;
    float GetDownloadProgress() const override;
    void SetGraphicsDevice(_In_opt_ CD3D11Device* graphicsDevice) override;
    wf::Size GetMaxRootSize() override;
    void SetMaxRootSize(wf::Size maxRootSize) override;

    // ImageViewBase
    HRESULT GetHR(uint64_t imageId) const override;

private:
    void ParseImageMetadata(uint64_t imageId) const;

    std::shared_ptr<EncodedImageData> m_encodedImageData;
    float m_downloadProgress = 0;
    xref::weakref_ptr<CD3D11Device> m_graphicsDevice;
    wf::Size m_maxRootSize{ 0, 0 };
    mutable HRESULT m_downloadParseResult = S_OK;
    mutable bool m_needParse = false;
};
