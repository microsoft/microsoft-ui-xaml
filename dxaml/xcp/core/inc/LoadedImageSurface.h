// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CDependencyObject.h"
#include "XcpList.h"
#include "DeviceListener.h"
#include "DisplayListener.h"
#include "IPLMListener.h"
#include "ImageViewListener.h"
#include "ImagingTelemetry.h"

class CLoadedImageSurfaceImageAvailableCallback;
class ImageCache;
class ImageMetadataView;
class IRawData;
struct ImageMetadata;
struct IAbortableImageOperation;

class CLoadedImageSurface
    : public CDependencyObject
    , private DeviceListener
    , private DisplayListener
    , private IPLMListener
    , private IImageViewListener
{
protected:
    CLoadedImageSurface(_In_ CCoreServices* core);
    ~CLoadedImageSurface() override;

public:
    DECLARE_CREATE(CLoadedImageSurface);

    void SetDesiredSize(float width, float height);
    _Check_return_ HRESULT InitFromUri(_In_ xstring_ptr uri);
    _Check_return_ HRESULT InitFromMemory(_In_ wistd::unique_ptr<IRawData> rawData);

    _Check_return_ HRESULT Close();

    _Check_return_ HRESULT GetCompositionSurface(_Outptr_ DCompSurface** dcompSurface);

    _Check_return_ HRESULT InitInstance() override;

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle event,
        _In_ CValue *value,
        _In_ XINT32 listenerType,
        _Out_opt_ CValue *result,
        _In_ bool handledEventsToo) override;

    _Check_return_ HRESULT RemoveEventListener(_In_ EventHandle event, _In_ CValue* value) override;

    bool AllowsHandlerWhenNotLive(XINT32 listenerType, KnownEventIndex eventIndex) const override;

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::LoadedImageSurface;
    }

    _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value) override;

private:
    _Check_return_ HRESULT GetImageDescription(ImageCache &imageCache);
    _Check_return_ HRESULT FireLoadCompleted(HRESULT result);
    _Check_return_ HRESULT StartDecodingHelper();
    _Check_return_ HRESULT StartFinalSizeDecoding();
    XSIZE GetDesiredMaxPixelSize(float plateauScale) const;
    XSIZE GetDecodePixelSize(_In_ const ImageMetadata& metadata, float plateauScale) const;
    bool IsDownloadComplete() const;
    bool IsVirtualPossible(XSIZE maxKnownPixelSize) const;
    void ReleaseSurfaceMemory();

    // DeviceListener
    _Check_return_ HRESULT OnDeviceRemoved(bool cleanupDComp) override;
    _Check_return_ HRESULT OnDeviceCreated() override;

    // DisplayListener
    _Check_return_ HRESULT OnScaleChanged() override;

    // IPLMListener
    _Check_return_ HRESULT OnSuspend(_In_ bool isTriggeredByResourceTimer) override;
    _Check_return_ HRESULT OnResume() override;
    void OnLowMemory() override;

    // IImageViewListener
    _Check_return_ HRESULT OnImageViewUpdated(ImageViewBase& sender) override;

    friend class CLoadedImageSurfaceImageAvailableCallback;
    CXcpList<REQUEST>* m_eventList = nullptr;
    xref_ptr<ImageSurfaceWrapper> m_imageSurfaceWrapper;
    xref_ptr<ImageCache> m_imageCache;
    xref_ptr<IAbortableImageOperation> m_abortableImageOperation;
    xref_ptr<CLoadedImageSurfaceImageAvailableCallback> m_imageAvailableCallback;
    std::shared_ptr<ImageMetadataView> m_imageMetadataView;
    float m_desiredMaxWidth = 0;
    float m_desiredMaxHeight = 0;

    bool m_closed : 1;
    bool m_isDeviceListener : 1;
    bool m_isDisplayListener : 1;
    bool m_isPLMListener : 1;
};
