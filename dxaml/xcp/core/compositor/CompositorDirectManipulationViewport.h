// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CCompositorDirectManipulationViewport class declaration.
//    Each DirectManipulation viewport is associated with a CCompositorDirectManipulationViewport
//    instance by CCoreServices and handed off to the compositor thread for
//    handling DM notifications.

#pragma once

// Uncomment to get DirectManipulation debug traces
// #define CDMV_DBG

class CCompositorDirectManipulationViewport : public CXcpObjectBase<IObject>
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CCompositorDirectManipulationViewport **ppCDMViewport, 
        _In_ IObject* pViewport,
        _In_ IObject* pPrimaryContent,
        _In_ IPALDirectManipulationCompositorService* pDirectManipulationCompositorService);

    IObject* GetCompositorViewportNoRef() const
    {
        return m_pViewport;
    }
    
    IObject* GetCompositorPrimaryContentNoRef() const
    {
        return m_pPrimaryContent;
    }

    IPALDirectManipulationCompositorService* GetDMCompositorServiceNoRef() const
    {
        return m_pDirectManipulationCompositorService;
    }

    void UpdateTransform();

    void SetTransform(
        _In_ XFLOAT translationX,
        _In_ XFLOAT translationY,
        _In_ XFLOAT zoomFactor)
    {
        m_fIsTransformSet = TRUE;
        m_translationX = translationX;
        m_translationY = translationY;
        m_zoomFactor = zoomFactor;
    }

    void ResetTransform()
    {
        m_fIsTransformSet = FALSE;
    }

    bool IsThisTransformSet(
        _In_ XFLOAT translationX,
        _In_ XFLOAT translationY,
        _In_ XFLOAT zoomFactor) const
    {
        return m_fIsTransformSet &&
            translationX == m_translationX &&
            translationY == m_translationY &&
            zoomFactor == m_zoomFactor;
    }

    // The following two routines are used by the compositor to manage the saved 
    // status field to detect when the underlying viewport transitions between 
    // states and fire appropriate telemetry/tracing events.
    void UpdateSavedStatus(_In_ XDMViewportStatus status)
    {
        m_savedStatus = status;
    }

    XDMViewportStatus GetSavedStatus() const
    {
        return m_savedStatus;
    }

    // Queries the underlying DM viewport for the latest status.
    _Check_return_ HRESULT QueryDMStatus(_Out_ XDMViewportStatus& status);

    // Returns the unique key to identify the underlying DM viewport. It allows the
    // compositor to detect when two different compositor viewports point to the same
    // underlying DM viewport.
    XHANDLE GetDMViewportKey() const
    {
        return m_dmViewportKey;
    }

private:
    // Constructor
    CCompositorDirectManipulationViewport(
        _In_ IObject* pViewport,
        _In_ IObject* pPrimaryContent,
        _In_ IPALDirectManipulationCompositorService* pDirectManipulationCompositorService)
        : m_pViewport(pViewport)
        , m_pPrimaryContent(pPrimaryContent)
        , m_pDirectManipulationCompositorService(pDirectManipulationCompositorService)
        , m_fIsTransformSet(FALSE)
        , m_translationX(0.0f)
        , m_translationY(0.0f)
        , m_zoomFactor(1.0f)
        , m_dmViewportKey(0)
        , m_savedStatus(XcpDMViewportBuilding)
    {
        ASSERT(pDirectManipulationCompositorService);
        pDirectManipulationCompositorService->AddRef();

        ASSERT(pViewport);
        pViewport->AddRef();

        ASSERT(pPrimaryContent);
        pPrimaryContent->AddRef();
    }

    // Destructor
    ~CCompositorDirectManipulationViewport() override
    {
#ifdef CDMV_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(L"CDMV: CCompositorDirectManipulationViewport - destructor.\r\n"));
#endif // CDMV_DBG
        ReleaseInterface(m_pDirectManipulationCompositorService);
        ReleaseInterface(m_pPrimaryContent);
        ReleaseInterface(m_pViewport);
    }

private:
    IObject* m_pViewport;        // IDirectManipulationViewport wrapped into an IObject to be PAL-friendly
    IObject* m_pPrimaryContent;  // IDirectManipulationContent wrapped into an IObject to be PAL-friendly
    IPALDirectManipulationCompositorService* m_pDirectManipulationCompositorService;
    
    XHANDLE m_dmViewportKey;            // Key to identify underlying DM viewport.
    XDMViewportStatus m_savedStatus;    // Last viewport status queried/saved by the compositor.
                                        // Compositor manages this field to detect status changes
                                        // and log the various telemetry/tracing events.

    // Fields used to cache the lastest inertial DManip transform. This is done as a workaround for a DManip bug
    // where the retrieved transform is a duplicate because a viewport characteristic has changed.
    bool  m_fIsTransformSet;
    XFLOAT m_translationX;
    XFLOAT m_translationY;
    XFLOAT m_zoomFactor;
};
