// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SurfaceDecodeParams.h"
#include "ImageProviderInterfaces.h"
#include "PixelFormat.h"
#include "ImagingTelemetry.h"

// TODO: ImageDecodeParams should no longer inherit from CXcpObjectBase.
// TODO: Move PixelFormat to components.
class ImageDecodeParams
    : public CXcpObjectBase<IObject>
{
public:
    ImageDecodeParams(
        PixelFormat format,
        unsigned int decodeWidth,
        unsigned int decodeHeight,
        bool isLoadedImageSurface,
        _In_ std::shared_ptr<ImagingTelemetry::ImageDecodeActivity> decodeActivity,
        uint64_t imageId,
        xstring_ptr source
        )
        : m_format(format)
        , m_decodeWidth(decodeWidth)
        , m_decodeHeight(decodeHeight)
        , m_autoPlay(false)
        , m_isLoadedImageSurface(isLoadedImageSurface)
        , m_decodeActivity(decodeActivity)
        , m_imageId(imageId)
        , m_strSource(source)
    {
    }

    ImageDecodeParams(
        PixelFormat format,
        unsigned int decodeWidth,
        unsigned int decodeHeight,
        bool autoPlay,
        _In_ SurfaceUpdateList const& surfaceUpdateList,
        bool isLoadedImageSurface,
        _In_ std::shared_ptr<ImagingTelemetry::ImageDecodeActivity> decodeActivity,
        uint64_t imageId,
        xstring_ptr source
        )
        : m_format(format)
        , m_decodeWidth(decodeWidth)
        , m_decodeHeight(decodeHeight)
        , m_autoPlay(autoPlay)
        , m_surfaceUpdateList(std::move(surfaceUpdateList))
        , m_isLoadedImageSurface(isLoadedImageSurface)
        , m_decodeActivity(decodeActivity)
        , m_imageId(imageId)
        , m_strSource(source)
    {
    }

    ImageDecodeParams(
        PixelFormat format,
        unsigned int decodeWidth,
        unsigned int decodeHeight,
        bool autoPlay,
        bool isLoadedImageSurface,
        _In_ std::shared_ptr<ImagingTelemetry::ImageDecodeActivity> decodeActivity,
        uint64_t imageId,
        xstring_ptr source
        )
        : m_format(format)
        , m_decodeWidth(decodeWidth)
        , m_decodeHeight(decodeHeight)
        , m_autoPlay(autoPlay)
        , m_isLoadedImageSurface(isLoadedImageSurface)
        , m_decodeActivity(decodeActivity)
        , m_imageId(imageId)
        , m_strSource(source)
    {
    }

    PixelFormat GetFormat() const { return m_format; }
    unsigned int GetDecodeWidth() const { return m_decodeWidth; }
    unsigned int GetDecodeHeight() const { return m_decodeHeight; }
    bool IsAutoPlay() const { return m_autoPlay; }
    bool IsHardwareOutput() const { return !m_surfaceUpdateList.empty(); }
    bool IsLoadedImageSurface() const { return m_isLoadedImageSurface; }
    const std::shared_ptr<ImagingTelemetry::ImageDecodeActivity> GetDecodeActivity() const { return m_decodeActivity; }

    uint64_t GetImageId() const { return m_imageId; }
    xstring_ptr GetStrSource() const { return m_strSource; }

    const SurfaceUpdateList& GetSurfaceUpdateList() const
    {
        return m_surfaceUpdateList;
    }

    SurfaceUpdateList DetachSurfaceUpdateList()
    {
        return std::move(m_surfaceUpdateList);
    }

private:
    PixelFormat m_format;
    unsigned int m_decodeWidth;
    unsigned int m_decodeHeight;
    bool m_autoPlay;
    SurfaceUpdateList m_surfaceUpdateList;
    bool m_isLoadedImageSurface;

    // Used for ETW tracing
    std::shared_ptr<ImagingTelemetry::ImageDecodeActivity> m_decodeActivity;
    uint64_t m_imageId;
    xstring_ptr m_strSource;
};
