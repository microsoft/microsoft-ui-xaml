// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImagingInterfaces.h>
#include <ImageViewListener.h>
#include "ImagingTelemetry.h"
#include "ImageDecodeBoundsFinder.h"

class EncodedImageData;
class CBitmapImageReportCallback;
class ImageCache;
class ICoreAsyncAction;
class ImageDecodeParams;
class ImageMetadataView;
struct IImageAvailableResponse;
struct IImageAvailableCallback;
struct ID2D1DeviceContext5;

class CImageSource
    : public CMultiParentShareableDependencyObject
    , private IImageViewListener
{
    friend class HWWalk;
    friend class CImageBrush;

protected:
    CImageSource(_In_ CCoreServices *pCore);

    ~CImageSource() override;

    _Check_return_ HRESULT SetupImageSource(
        bool mustKeepSoftwareSurface = false,
        _In_opt_ CREATEPARAMETERS* createParameters = nullptr);

    _Check_return_ HRESULT SetDirty();

    _Check_return_ HRESULT RequestMeasure();

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** retObject,
        _In_ CREATEPARAMETERS* createParameters
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CImageSource>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peers of derived classes have state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    void SetKeepSystemMemory()
    {
        m_pImageSurfaceWrapper->SetKeepSystemMemory();
    }

    // Returns the 'physical' size of the underlying image surface, for use when rendering.
    virtual XUINT32 GetWidth() const;
    virtual XUINT32 GetHeight() const;

    virtual bool IsOpaque() { return m_pImageSurfaceWrapper->IsOpaque(); }
    virtual bool HasRetainedSize() { return m_pImageSurfaceWrapper->HasRetainedSize(); }
    bool CheckForHardwareResources() { return m_pImageSurfaceWrapper->CheckForHardwareResources(); }

    // Helper method to see if hardware surfaces are set or not. Mostly used in debug asserts.
    bool HasHardwareSurfaces() { return m_pImageSurfaceWrapper->HasHardwareSurfaces(); }

    void ResetSurfaces(
        bool mustKeepSoftwareSurface = false,
        bool mustKeepHardwareSurfaces = false
        );

    _Ret_maybenull_ IPALSurface* GetSoftwareSurface() { return m_pImageSurfaceWrapper->GetSoftwareSurface(); }

    virtual _Check_return_ HRESULT GetSoftwareSurfaceForPrinting(
        _Out_ xref_ptr<IPALSurface>& spSoftwareSurface
        );

    virtual bool ShouldRequestDecode();

    _Check_return_ HRESULT GetCacheIdentifier(
        _In_ const ImageDecodeParams& imageDecodeParams,
        _Out_ xstring_ptr* pstrCacheIdentifier
        );

    _Check_return_ HRESULT GetCacheIdentifier(
        unsigned long width,
        unsigned long height,
        PixelFormat pixelFormat,
        _Out_ xstring_ptr* pCacheIdentifier);

    virtual _Check_return_ HRESULT QueueRequestDecode(
        XUINT32 width,
        XUINT32 height,
        bool retainPlaybackState
        );

    _Check_return_ HRESULT RequestDecode(
        XUINT32 width,
        XUINT32 height,
        bool retainPlaybackState);

    virtual _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ SurfaceCache *pSurfaceCache
        );

    virtual bool CheckForLostHardwareResources();

    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

    static HRESULT TraceImageSourceRelationEtw(
        _In_ CDependencyObject *pDO
        );

    static void TraceImageRequestDecodeEtw(
        _In_ CImageSource* imageSource,
        _In_ UINT32 requestWidth,
        _In_ UINT requestHeight
    );

    bool IsSourceSetAndEqualTo( _In_opt_ CImageSource * pOther );

    static _Check_return_ HRESULT PixelWidth(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT PixelHeight(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    _Check_return_ HRESULT SetSource(
        XUINT32 cInputSize,
        _In_reads_(cInputSize) XUINT8 *pInputMemory,
        _In_ ICoreAsyncAction *pAsyncAction
        );

    _Check_return_ HRESULT OnStreamImageAvailable(
        _In_ IImageAvailableResponse* pResponse
        );

    virtual _Check_return_ HRESULT GetTitle(_Outptr_ HSTRING *output) {*output = nullptr; return S_OK;}
    virtual _Check_return_ HRESULT GetDescription(_Outptr_ HSTRING *output) {*output = nullptr; return S_OK;}

    wf::Size GetMaxRootSize();

public:
    xstring_ptr m_strSource;

    // m_fSourceUpdated flag tells us the source string changed
    // and an Image element pointing to this ImageSource
    // needs to update its Source property
    // TODO:  This logic seems to be flawed in the case where
    // more than one Image shares ImageSources, as this flag is reset
    // in first image that needs to update its Source property
    // BUG 819366
    bool m_fSourceUpdated                       : 1;
    bool m_fSourceNeedsMeasure                  : 1;
    bool m_pendingDecodeForLostSoftwareSurface  : 1;

protected:
    bool m_fDecodeToRenderSize;
    bool m_fDecodeToRenderSizeForcedOff;
    uint32_t m_frameIndex = 0;

    _Check_return_ HRESULT OnImageAvailableCommon(
        _In_ IImageAvailableResponse* pResponse
        );

    void DisconnectImageOperation();

    virtual XUINT32 GetDecodeWidth(
        );

    virtual XUINT32 GetDecodeHeight(
        );

    virtual DirectUI::DecodePixelType GetDecodeType() const;

    // This only applies to BitmapSource/BitmapImage currently.  Other classes like WriteableBitmap and SoftwareBitmapSource aim to use the provided
    // surface size as-is without modification when rendering.  It is debatable whether the other two classes should support DecodeToRenderSize, the
    // current thinking is that SoftwareBitmapSource aims to provide a low latency solution for getting uncompressed images to screen specifically for
    // Shell start menu and future uses like Photos team thumbnails.  It is up to the caller to ensure the image is the appropriate size before loading
    // it.
    virtual bool ShouldDecodeToRenderSize() { return (GetDecodeWidth() == 0) && (GetDecodeHeight() == 0) && !m_fDecodeToRenderSizeForcedOff; }

    virtual _Check_return_ HRESULT ResetCallbackEvents(
        );

    virtual _Check_return_ HRESULT MarkHandledCallbackEvents(
        );

    virtual _Check_return_ HRESULT ResetForSourceChange(
        bool mustKeepSoftwareSurface,
        bool mustKeepHardwareSurfaces,
        bool keepEncodedData
        );

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;
    virtual void RegisterForCleanupOnLeave()
    {
        if (m_pImageSurfaceWrapper != NULL)
        {
            m_pImageSurfaceWrapper->RegisterForDeviceCleanup();
        }
    }
    virtual void UnregisterForCleanupOnEnter()
    {
        if (m_pImageSurfaceWrapper != NULL)
        {
            m_pImageSurfaceWrapper->UnregisterForDeviceCleanup();
        }
    }

    virtual HRESULT GetAutoPlay(
        _Out_ bool& isAutoPlay
        )
    {
        isAutoPlay = false;
        return S_OK;
    }

    bool ShouldContinueAsyncAction();

    void CompleteAsyncAction(HRESULT hr);

    ICoreAsyncAction *m_pAsyncAction;

    std::shared_ptr<ImageMetadataView> m_imageMetadataView;
    xref_ptr<IAbortableImageOperation> m_spAbortableImageOperation;
    _Notnull_ ImageSurfaceWrapper *m_pImageSurfaceWrapper;

    _Ret_maybenull_ HWTexture* GetHardwareSurface() { return m_pImageSurfaceWrapper->GetHardwareSurface(); }
    _Ret_maybenull_ IPALAcceleratedBitmap* GetD2DBitmap() { return m_pImageSurfaceWrapper->GetD2DBitmap(); }

    void SetD2DBitmap(_In_ IPALAcceleratedBitmap *pBitmap) { m_pImageSurfaceWrapper->SetD2DBitmap(pBitmap); }

    _Check_return_ HRESULT SetImageCache(xref_ptr<ImageCache> imageCache);

    _Check_return_ HRESULT DecodeStream(
        _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
        bool synchronousDecode,
        bool retainPlaybackState
        );

    _Check_return_ HRESULT DecodeStreamWithSize(
        _In_ const xref_ptr<IImageAvailableCallback>& spImageAvailableCallback,
        XUINT32 width,
        XUINT32 height,
        bool synchronousDecode,
        bool forceDecode,
        bool retainPlaybackState,
        bool decodeToRenderSize
        );

    virtual _Check_return_ HRESULT PrepareDecode(bool retainPlaybackState);

    virtual _Check_return_ HRESULT OnDownloadImageAvailableImpl(
        _In_ IImageAvailableResponse* pResponse
        );

    void GetDecodePixelWidthAndHeightInPhysicalPixels(
        _In_ const DirectUI::DecodePixelType decodePixelType,
        _In_ const XUINT32 uDecodePixelWidthInLogicalPixels,
        _In_ const XUINT32 uDecodePixelHeightInLogicalPixels,
        _Out_ XUINT32 * const pDecodePixelWidthInPhysicalPixels,
        _Out_ XUINT32 * const pDecodePixelHeightInPhysicalPixels
    );

    virtual bool ShouldKeepSoftwareSurfaces() { return true; }

    _Check_return_ HRESULT PrepareDecodeParams(
        unsigned int width,
        unsigned int height,
        PixelFormat pixelFormat,
        bool forceUiThreadCopy,
        bool retainPlaybackState,
        _Out_ xref_ptr<ImageDecodeParams>& spImageDecodeParams
        );

protected:
    enum BitmapImageState
    {
        //
        // The BitmapImage is newly created.
        //
        //  Transitions:
        //      Downloading - new URI set
        //      Decoding - new stream set
        //
        Initial,

        //
        // The BitmapImage was created from a URI and is downloading its bitmap.
        //
        //  On Enter:
        //      Begins async download and decode
        //
        //  Transitions:
        //      Decoding - new stream set
        //      Failed - download and decode failed
        //
        Downloading,

        //
        // The BitmapImage compressed bits are ready to be decoded.  This state
        // provides a deferral mechanism so that the image decode can be initiated
        // in the UI thread when the render size is known.  This allows for large
        // memory savings.
        //
        //  On Enter:
        //      Prepare for a decoding operation.
        //
        //  Transitions:
        //      Decoding - new stream set
        //
        DecodePending,

        //
        // The BitmapImage was created from a stream and is decoding asynchronously.
        //
        //  On Enter:
        //      Begins synchronous decode
        //
        //  Transitions:
        //      Downloading - new URI set
        //      Failed - decode failed
        //
        Decoding,

        //
        // The BitmapImage failed to load.
        //
        //  On Enter:
        //      Fires ImageFailed
        //
        //  Transitions:
        //      Downloading - new URI set
        //      Decoding - new stream set
        //
        Failed,

        //
        // The BitmapImage has a decoded surface available. It also has its encoded
        // bitmap.  The ImageSurfaceWrapper keeps track of exactly what surfaces.
        //
        //  Transitions:
        //      Downloading - new URI set
        //      Decoding - new stream set
        //
        Decoded,

        //
        // The BitmapImage has only its encoded bitmap. It's decoding asynchronously into a
        // software surface.
        //
        //  Transitions:
        //      Decoded - decode completes
        //      Downloading - new URI set
        //      Decoding - new stream set
        //
        HasEncodedImageOnly
    };

    xref_ptr<ImageDecodeParams> m_spPendingDecodeParams;

public:
    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) final;

    _Check_return_ HRESULT SuspendAnimation();
    _Check_return_ HRESULT ResumeAnimation();

    _Check_return_ HRESULT PlayAnimation();
    _Check_return_ HRESULT StopAnimation();

    void SetDecodeToRenderSizeForcedOff();

    // Drag and Drop Custom Visual need software surface
    // and reloads it
    void SetKeepSoftwareSurfaceOnReload(bool value)
    {
        m_keepSoftwareSurfaceOnReload = value;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Out_ CValue *pValue
        ) override;

    bool IsFiringEvents() override { return true; }

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult ,
        _In_ bool fHandledEventsToo = false
        ) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue
        ) override;

    _Check_return_ HRESULT AddCallback(
        _In_ CBitmapImageReportCallback* pImageErrorCallback
        );

    void RemoveCallback(
        _In_ CBitmapImageReportCallback* pImageErrorCallback
        );

    uint32_t GetPhysicalWidth() const;
    uint32_t GetPhysicalHeight() const;

    // Returns the 'logical' size of the underlying image surface, for use in layout.
    // By default, this matches the 'physical' size.
    virtual XUINT32 GetLayoutWidth();
    virtual XUINT32 GetLayoutHeight();

    virtual bool HasDimensions() { return ((GetSoftwareSurface() != NULL) || (m_pImageSurfaceWrapper->CheckForHardwareResources()) || IsMetadataAvailable()); }

    virtual XINT32 GetPixelWidth() const;
    virtual XINT32 GetPixelHeight() const;

    // uriChanged means we need to drop the existing image operation because the URI may have changed
    // and we need to start a fresh image operation.
    // retainPlaybackState means we need to transfer the IsPlaying state from the old operation to the new one.
    // When the URI is totally changed such as user set a different URI source we want to play based on AutoPlay only.
    // If the URI is changing due to plateau scale such as foo.scale-100.png changed to foo.scale-200.png then
    // we want to keep the stopped/playing state regardless of the AutoPlay setting
    virtual _Check_return_ HRESULT ReloadImage(bool uriChanged, bool retainPlaybackState);

    virtual _Check_return_ HRESULT ReloadReleasedSoftwareImage();

    _Check_return_ HRESULT CreationComplete() override;

    _Check_return_ HRESULT TryUseCachedHardwareResources(
        _In_ const ImageDecodeParams& imageDecodeParams,
        _In_ SurfaceCache* pSurfaceCache,
        _Out_ bool* pGetEncodedImageOnly
        );

    _Check_return_ HRESULT ReloadOnResourceInvalidation(ResourceInvalidationReason reason);

    // Returns true if the state of the image indicates that it has either been
    // loaded or it has failed and we cannot expect new events to be raised.
    // Drag and Drop needs it to know that software bits are available.
    // We'll update Drag and Drop to be able to get hardware bits in 1501,
    // i.e. as of TP2, Software bits covers the targeted scenarios.
    bool IsImageDecoded(_Out_ bool *pAvailable) const
    {
        bool available = (m_bitmapState == BitmapImageState::Decoded);
        *pAvailable = available;
        return available || (m_bitmapState == BitmapImageState::Failed);
    }

    static void TraceDecodeStreamForImageEtw(
        _In_ CImageSource* image,
        _In_ const std::shared_ptr<EncodedImageData>& encodedImageData,
        _In_ UINT32 decodeWidth,
        _In_ UINT32 decodeHeight,
        _In_ bool retrievedFromCache
    );

    virtual _Check_return_ HRESULT GetID2D1SvgDocument(
        _In_ ID2D1DeviceContext5 *pD2DDeviceContextNoRef,
        _Out_ wrl::ComPtr<ID2D1SvgDocument>& d2dSvgDocument,
        _Out_ uint32_t* width,
        _Out_ uint32_t* height
        )
    {
        *width = 0;
        *height = 0;
        d2dSvgDocument = nullptr;
        return S_OK;
    }

    void TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::FallbackReason reason);

    //
    // Public fields
    //
    XFLOAT m_downloadProgress;
    DirectUI::BitmapCreateOptions m_nCreateOptions;
    XINT32 m_decodePixelWidth;
    XINT32 m_decodePixelHeight;
    DirectUI::DecodePixelType m_decodePixelType;

protected:

    _Check_return_ HRESULT OnDownloadImageAvailable(
        _In_ IImageAvailableResponse* pResponse
        );

    HRESULT FireImageFailed(
        _In_ XUINT32 iErrorCode
        );

    void ResetCacheIdentifier();

    _Check_return_ HRESULT FirePublicDownloadEvent();
    _Check_return_ HRESULT FireInternalDownloadEvent();

    HRESULT FireImageOpened();


    _Check_return_ HRESULT TriggerReloadImage(
        bool uriChanged,
        bool retainPlaybackState,
        bool checkForParsing
        );

    void SetBitmapState(BitmapImageState newState);

    xref_ptr<ImageCache> m_imageCache;
    bool IsMetadataAvailable() const;

    uint32_t m_retainedNaturalWidth = 0;
    uint32_t m_retainedNaturalHeight = 0;

    BitmapImageState m_bitmapState;

    _Check_return_ HRESULT RedecodeEncodedImage(
        bool lostSurface
        );

private:
    // IImageViewListener
    _Check_return_ HRESULT OnImageViewUpdated(ImageViewBase& sender) override;

    _Check_return_ HRESULT DecodeToRenderSize(
        XUINT32 requestWidth,
        XUINT32 requestHeight,
        bool retainPlaybackState
    );

    _Check_return_ HRESULT UpdateDecodeSize();

     bool ShouldFirePublicBitmapImageEvents();
    _Check_return_ HRESULT EnsureAbsoluteUri();

    _Check_return_ HRESULT CheckCachedHardwareResources(
        unsigned long width,
        unsigned long height,
        PixelFormat pixelFormat,
        _Out_ bool *pCacheHit);

    _Check_return_ HRESULT UpdateHardwareCacheIfAvailable();

    _Check_return_ HRESULT OnHdrChanged();
    _Check_return_ HRESULT OnUriSourceChanged(bool retainPlaybackState);
    _Check_return_ HRESULT RegisterWithReloadManager();
    _Check_return_ HRESULT UnregisterWithReloadManager();
    _Check_return_ HRESULT ShouldReloadOnResourceInvalidation(ResourceInvalidationReason reason, _Out_ bool* shouldReload);

    XUINT32 GetLogicalSize(XUINT32 physicalSize) const;
    bool HasLogicalDecodeSize() const;

    bool IsAnimatedBitmap();

    std::uint64_t GetEstimatedSurfaceCommitSize() const;

    CXcpList<REQUEST>* m_pEventList;

    //
    // ImageBrushes attach handlers for download progress, opened, and failed. An ImageBrush
    // can be pointed to an existing BitmapImage, in which case it'll get the opened event
    // immediately upon entering the tree. If a second ImageBrush is attached to this same
    // BitmapImage later, we have to not fire the event for the first ImageBrush again. We
    // can't just remove the callback after firing an event because we'll need to fire again
    // if the source of this BitmapImage changes, so we just keep two lists.
    //
    xvector< xref_ptr<CBitmapImageReportCallback> > m_imageReportCallbacks;
    xvector< xref_ptr<CBitmapImageReportCallback> > m_imageReportAlreadyReportedCallbacks;

    xstring_ptr m_strCacheIdentifier;

public:
    // Public field placed here to pack better on x64
    // m_fReloadNeeded says the source changed and
    // signals that we now need to reload the image. It is a separate flag
    // from m_fSourceUpdated because they are reset at different times/places
    // (and the m_fSourceUpdated reset seems to be wrong)
    bool m_fReloadNeeded    : 1;
    bool m_keepSoftwareSurfaceOnReload : 1;

private:
    bool m_fRegisteredWithReloadManager : 1;
    bool m_forceDecodeRequest : 1;
    bool m_publicBitmapEventFired : 1;

    xref_ptr<IPALUri> m_absoluteUri; // Cached to avoid redoing expensive URI combine operations.
};

class CBitmapSource : public CImageSource
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** retObject,
        _In_ CREATEPARAMETERS* createParameters);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBitmapSource>::Index;
    }

protected:
    CBitmapSource(_In_ CCoreServices *pCore)
        : CImageSource(pCore)
    {}
};

class CBitmapImage
    : public CBitmapSource
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject** retObject,
        _In_ CREATEPARAMETERS* createParameters
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBitmapImage>::Index;
    }

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ SurfaceCache *pSurfaceCache
        ) override;

protected:
    CBitmapImage(_In_ CCoreServices *pCore)
        : CBitmapSource(pCore)
    {}

    _Check_return_ HRESULT PrepareDecode(bool retainPlaybackState) override;

    bool ShouldKeepSoftwareSurfaces() override { return false; }

    HRESULT GetAutoPlay(_Out_ bool& isAutoPlay) override;

    _Check_return_ HRESULT OnDownloadImageAvailableImpl(
        _In_ IImageAvailableResponse* pResponse
        ) override;
};
