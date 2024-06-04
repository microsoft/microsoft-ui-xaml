// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct REQUEST;
class  CImage;
class  CBitmapImage;
class  CImageSource;
class  CBitmapIcon;

class CBitmapImageReportCallback;
class CImageBrushReportCallback;
class CWindowRenderTarget;
class SurfaceCache;
class HWTexture;
class HWTextureManager;

//------------------------------------------------------------------------
//
//  Interface:  IImageReportCallback
//
//  Synopsis:
//      This callback is meant to allow host element of ImageBrush to
//      respond to DownloadProgress, ImageOpened, and Error events.
//
//      Usually Image failure, which is occurred on ImageBrush, will be
//      handled in ImageBrush object level. The error is reported to
//      ImageBursh's ImageFailed event handler or fall back to the global
//      OnError handler if ImageFailed handler is not registered.
//
//      But if the ImageBrush is created by Image ui element,  It is Image
//      element's responsibility to handle the error. ie, The error should
//      be first reported to Image element's ImageFailed event handler,
//      then global error handler.
//
//      With the error report callback, CImage element can set this callback
//      when it loads the ImageBrush. When ImageBrush reports error, it
//      will ask the callback to report it. otherwise, it will report to
//      its own error handler.
//
//------------------------------------------------------------------------
struct IImageReportCallback
{
public:
    virtual XUINT32 AddRef() = 0;
    virtual XUINT32 Release() = 0;
    virtual _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode) = 0;
    virtual _Check_return_ HRESULT FireDownloadProgress(_In_ XFLOAT iProgress) = 0;
    virtual _Check_return_ HRESULT FireImageOpened() = 0;
    virtual void Disable() = 0;
};

class CImageBrush : public CTileBrush
{
    friend class HWWalk;

protected:
    CImageBrush( _In_ CCoreServices *pCore )
        : CTileBrush(pCore)
        , m_pImageSource(NULL)
        , m_cLockCount(0)
        , m_pEventList(NULL)
        , m_pBitmapImageReportCallback(NULL)
        , m_pImageReportCallback(NULL)
        , m_decodeToRenderSizeForcedOff(false)
        , m_isDragDrop(false)
    {
    }

    ~CImageBrush() override;

public:
    DECLARE_CREATE(CImageBrush);

    // CDependencyObject overrides

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) final
    {
        return (m_pEventList != NULL);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CImageBrush>::Index;
    }

// For ImageFailed event support.

public:
    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult ,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) final;

protected:

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

public:

// CImageBrush methods
    _Check_return_ HRESULT ReloadSoftwareSurfaceIfReleased() final;

    _Check_return_ HRESULT Lock(
        _Out_ void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight
        ) final;

    XUINT32 UnLock() final;

    IPALSurface *GetSurface() final;

    bool IsOpaque() final;

    _Check_return_ HRESULT ReloadImage(bool uriChanged, _In_opt_ IImageReportCallback *pImageErrorCallback);

    bool ShouldRequestDecode();

    _Check_return_ HRESULT QueueRequestDecode(
        XUINT32 width,
        XUINT32 height,
        bool retainPlaybackState
        );

    bool HasDimensions();
    bool HasRetainedSize();
    _Ret_maybenull_ IPALSurface* GetSoftwareSurface() final;
    _Check_return_ HRESULT GetSoftwareSurfaceForPrinting(
        _Out_ xref_ptr<IPALSurface>& spSoftwareSurface
        );

    _Check_return_ HRESULT GetID2D1SvgDocument(
        _In_ ID2D1DeviceContext5 *pD2DDeviceContextNoRef,
        _Out_ wrl::ComPtr<ID2D1SvgDocument>& d2dSvgDocument,
        _Out_ uint32_t* width,
        _Out_ uint32_t* height
        );

    // This is to purpose the image brush for producing Drag Visuals
    // Drag and Drop needs BitmapImage to be decoded to its natural size,
    // if DecodedPixelHeight and DecodedPixelWidth are not set, when
    // ImageBrush is not in the live tree, so this flag (set before
    // SetSource is called) tells ImageBrush to reload the image regardless.
    void SetIsDragDrop() { m_isDragDrop = true; }

// CTileBrush methods
    void GetNaturalBounds(
        _Out_ XRECTF *pNaturalBounds ) override;

    void GetActualBounds(
        _Out_ XRECTF *pActualBounds ) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    void FireImageDownloadEvent(_In_ XFLOAT downloadProgress);

    HRESULT FireImageFailed(_In_ XUINT32 iErrorCode);

    HRESULT FireImageOpened();

    void RemoveCallback();

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ SurfaceCache *pSurfaceCache
        ) final;

    _Check_return_ HRESULT GetVisibleImageSourceBounds(
        _In_ const XRECTF_RB *pWindowBounds,
        _Out_ XRECTF *pBounds
        );

    _Ret_maybenull_ HWTexture* GetHardwareSurface() final;

    void UpdateDecodeToRenderSizeStatusOnSourceChange( _In_opt_ CImageSource * pPreviousSource );

public:
    // CImageBrush fields
    CImageSource *m_pImageSource;

private:
    XUINT32 m_cLockCount;
// The following are loaded lazily.  Always access them through the methods
// above, never directly.

    // Events
    CXcpList<REQUEST> *m_pEventList;

    CBitmapImageReportCallback *m_pBitmapImageReportCallback;
    IImageReportCallback *m_pImageReportCallback;
    bool m_decodeToRenderSizeForcedOff;
    bool m_isDragDrop;

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT UpdateAcceleratedBrush(
        _In_ const D2DRenderParams &renderParams
        ) final;


//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetPrintBrush(
        _In_ const D2DRenderParams &printParams,
        _Outptr_ IPALAcceleratedBrush **ppBrush
        ) final;
};

class CBitmapImageReportCallback final : public IImageReportCallback
{
public:
    static HRESULT Create(_In_ CImageBrush* pImageBrush, _Out_ CBitmapImageReportCallback** ppCallback);
    static HRESULT Create(_In_ CBitmapIcon* pBitmapIcon, _Out_ CBitmapImageReportCallback** ppCallback);

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    virtual XINT32 MakeSeekable()
    {
        // Do not make it seekable
        return 0;
    }

    _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode) override;
    _Check_return_ HRESULT FireDownloadProgress(_In_ XFLOAT iProgress) override;
    _Check_return_ HRESULT FireImageOpened() override;

    void Disable() override;

    ~CBitmapImageReportCallback();

private:
    CBitmapImageReportCallback();
private:
    // This can either be a callback for an image brush or an image icon, not both.
    CImageBrush    *m_pImageBrush;
    CBitmapIcon     *m_pBitmapIcon;

    XUINT32    m_cRef;
};
