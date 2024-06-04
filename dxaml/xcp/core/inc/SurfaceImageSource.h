// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <guiddef.h>

class DCompSurfaceFactory;
class OfferTracker;

class CSurfaceImageSource : public CImageSource
{
protected:
    // If both dimensions of a surface image source are larger than this
    // then it can be allocated stand-alone (and thus support flip-style updates)
    static const XINT32 sc_MinStandAloneSize = 512;

    CSurfaceImageSource(_In_ CCoreServices *pCore);

public:
    ~CSurfaceImageSource() override;

    DECLARE_CREATE(CSurfaceImageSource);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSurfaceImageSource>::Index;
    }

    _Check_return_ HRESULT Initialize(
        _In_ XINT32 width,
        _In_ XINT32 height,
        _In_ bool isOpaque
        );

    _Check_return_ HRESULT SetDevice(
        _In_ IUnknown *pDevice
        );

    _Check_return_ HRESULT BeginDraw(
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppSurface,
        _Out_ XPOINT *pSurfaceOffset
        );

    _Check_return_ HRESULT EndDraw();

    _Check_return_ HRESULT SetDeviceWithD2D(
        _In_ IUnknown *pDevice
        );

    _Check_return_ HRESULT BeginDrawWithD2D(
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _In_ bool calledFromUIThread,
        _Outptr_ IUnknown **ppUpdateObject,
        _Out_ XPOINT *pSurfaceOffset
        );

    _Check_return_ HRESULT EndDrawWithD2D();

    _Check_return_ HRESULT SuspendDraw(_In_ bool calledFromUIThread);

    _Check_return_ HRESULT ResumeDraw(_In_ bool calledFromUIThread);

    _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ SurfaceCache *pSurfaceCache) final
    {
        RRETURN(S_OK);
    }

    void CleanUpAfterRenderThreadFailure();
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) final;
    bool CheckForLostHardwareResources() override;

    XUINT32 GetWidth() const final { return m_width; }
    XUINT32 GetHeight() const final { return m_height; }
    bool IsOpaque() final { return m_isOpaque; }
    bool HasDimensions() final { return true; }

    _Check_return_ HRESULT GetDeviceWithGPUWork(
        _Inout_ xvector<IUnknown *> *pDevices
        );

protected:
    _Check_return_ HRESULT BeginDrawCommon(
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppUpdateObject,
        _Out_ XPOINT *pSurfaceOffset
        );

    _Check_return_ HRESULT EndDrawCommon();

    _Check_return_ HRESULT EnsureMultiThreadedDevice() const;

    _Check_return_ HRESULT EnsureSurfaceNotOffered() const;

    bool IsDrawing() const { return m_isDrawing; }

    virtual _Check_return_ HRESULT PreUpdateVirtual(
        _In_ const xvector<XRECT> *pUpdatedRects
        );

    virtual bool IsVirtual() { return false; }

    void SetSize(XUINT32 width, XUINT32 height) { m_width = width; m_height = height; }

    void RegisterForCleanupOnLeave() final
    {
        // SIS is already on another global list and gets cleaned
        // up through it. Hence to avoid duplicate tracking do nothing.
    }

    void UnregisterForCleanupOnEnter() final
    {
        // SIS is already on another global list and gets cleaned
        // up through it. Hence to avoid duplicate tracking do nothing.
    }

    wil::cs_leave_scope_exit GetMultithreadedLock() const { return m_pLock.lock(); }

private:
    enum InterfaceUsed
    {
        None = 0,
        Native,
        NativeWithD2D
    };

    _Check_return_ HRESULT EnsureSurfaceFactory();
    _Check_return_ HRESULT ObtainSurfaceFactory(_In_ IUnknown *pDevice, _Outptr_ DCompSurfaceFactory **ppSurfaceFactory);
    _Check_return_ HRESULT EnforceInterfaceUsage(InterfaceUsed interfaceUsed);
    _Check_return_ HRESULT ValidateUpdateRect(_In_ const XRECT *pUpdateRect);

    mutable wil::critical_section m_pLock;
    IUnknown* m_pDevice                     = nullptr;
    DCompSurfaceFactory *m_pSurfaceFactory  = nullptr;

    xref_ptr<OfferTracker> m_offerTracker;
    xvector<XRECT> m_updatedRects;

    InterfaceUsed m_interfaceUsed           = InterfaceUsed::None;

    XUINT32 m_width     = 0;
    XUINT32 m_height    = 0;
    bool m_isOpaque                    : 1;
    bool m_isDrawing                   : 1;
    bool m_onGlobalSISList             : 1;
    bool m_needReleaseHWSurfaceOnDraw  : 1;
    bool m_hasGPUWork                  : 1; // Used to determine whether to block on GPU work for animation synchronization

    // When EndDraw is called, we need to request a commit for the DComp device, but we should _not_ need to
    // propagate a dirty flag up the tree. DComp has enough dirtiness information to redraw only the updated
    // region of the V/SIS. If we propagate a dirty flag up the Xaml tree, then we'll regenerate a new sprite
    // visual for the element using the V/SIS image brush, which will cause us to redraw more than needed. This
    // can cause lots of overdraw for apps with large VSIS canvases like OneNote.
    bool m_shouldPropagateDirtyFlag    : 1;
};
