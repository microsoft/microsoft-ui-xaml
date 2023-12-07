// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CDMViewport class declaration. Each DirectManipulation viewport is associated
//    with a CDMViewport instance.
//  Note:
//    This class is internal and used only by the InputManager. It corresponds to
//    IObject pViewport passed to IDirectManipulationService in the PAL.

#pragma once

#include "DirectManipulationContainer.h"

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMV_DBG 0
//#define DM_DEBUG

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation verbose debug outputs, and 0 otherwise
#define DMVv_DBG 0

//------------------------------------------------------------------------
//
//  CDMCViewport class used to communicate new or existing DM viewports
//  from an InputManager to a CCoreServices, via the
//  CInputServices::GetDirectManipulationViewports method.
//
//------------------------------------------------------------------------
class CDMCViewport : public CXcpObjectBase<IObject>
{
public:
    // ------------------------------------------------------------------------
    // CDMCViewport Public Methods
    // ------------------------------------------------------------------------
    static _Check_return_ HRESULT Create(
        _Outptr_ CDMCViewport** ppDMCViewport,
        _In_ IPALDirectManipulationCompositorService* pDMCompositorService,
        _In_ IObject* pCompositorViewport,
        _In_ IObject* pCompositorPrimaryContent);

    IPALDirectManipulationCompositorService* GetDMCompositorServiceNoRef() const
    {
        return m_pDMCompositorService;
    }

    IObject* GetCompositorViewportNoRef() const
    {
        return m_pCompositorViewport;
    }

    IObject* GetCompositorPrimaryContentNoRef() const
    {
        return m_pCompositorPrimaryContent;
    }

    // ------------------------------------------------------------------------
    // CDMCViewport Protected Constructor/Destructor
    // ------------------------------------------------------------------------
protected:
    CDMCViewport(
        _In_ IPALDirectManipulationCompositorService* pDMCompositorService,
        _In_ IObject* pCompositorViewport,
        _In_ IObject* pCompositorPrimaryContent)
        : m_pDMCompositorService(pDMCompositorService)
        , m_pCompositorViewport(pCompositorViewport)
        , m_pCompositorPrimaryContent(pCompositorPrimaryContent)
    {
        ASSERT(pDMCompositorService);
        AddRefInterface(pDMCompositorService);

        ASSERT(pCompositorViewport);
        AddRefInterface(pCompositorViewport);

        ASSERT(pCompositorPrimaryContent);
        AddRefInterface(pCompositorPrimaryContent);

#ifdef DM_DEBUG
        if (DMVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | XCP_TRACE_VERBOSE)))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | XCP_TRACE_VERBOSE | DMVv_DBG) /*traceType*/, L"DMVv[0x%p]:  CDMCViewport constructor.", this));
        }
#endif // DM_DEBUG
    }

    ~CDMCViewport() override
    {
#ifdef DM_DEBUG
        if (DMVv_DBG || gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | XCP_TRACE_VERBOSE)))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | XCP_TRACE_VERBOSE | DMVv_DBG) /*traceType*/, L"DMVv[0x%p]:  ~CDMCViewport destructor.", this));
        }
#endif // DM_DEBUG
        ReleaseInterface(m_pDMCompositorService);
        ReleaseInterface(m_pCompositorViewport);
        ReleaseInterface(m_pCompositorPrimaryContent);
    }

    // ------------------------------------------------------------------------
    // CDMCViewport Private Fields
    // ------------------------------------------------------------------------
private:
    IPALDirectManipulationCompositorService* m_pDMCompositorService;
    IObject* m_pCompositorViewport;
    IObject* m_pCompositorPrimaryContent;
};

//------------------------------------------------------------------------
//
//  CDMViewportBase base class for CDMViewport and CDMCrossSlideViewport.
//  Includes all the common fields and object model.
//
//------------------------------------------------------------------------
class CDMViewportBase : public CXcpObjectBase<IObject>
{
    // ------------------------------------------------------------------------
    // CDMViewportBase Public Methods
    // ------------------------------------------------------------------------
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDMViewportBase** ppDMViewport,
        _In_ CUIElement* pDMContainer);

    CUIElement* GetDMContainerNoRef() const
    {
        return m_pDMContainer;
    }

    bool GetIsCrossSlideViewport() const
    {
        return m_fIsCrossSlideViewport;
    }

    // Returns the number of contact Ids (pointerId) associated with this viewport
    _Check_return_ HRESULT GetContactIdCount(_Out_ XUINT32* pcContactIds);

    // Associates the provided contact Id (pointer Id) with this viewport
    _Check_return_ HRESULT AddContactId(_In_ XUINT32 contactId);

    // Gets the existing contact Id at the provided 0-based index
    _Check_return_ HRESULT GetContactId(_In_ XUINT32 index, _Out_ XUINT32* pContactId);

    // Returns True if the provided contact Id is associated with this viewport
    _Check_return_ HRESULT ContainsContactId(_In_ XUINT32 contactId, _Out_ bool* pfContainsContactId);

    // Detach the provided contact Id from this viewport
    _Check_return_ HRESULT RemoveContactId(_In_ XUINT32 contactId);

    // ------------------------------------------------------------------------
    // CDMViewportBase Protected Methods
    // ------------------------------------------------------------------------
protected:
    void SetIsCrossSlideViewport(_In_ bool fIsCrossSlideViewport)
    {
        m_fIsCrossSlideViewport = fIsCrossSlideViewport;
    }

    // ------------------------------------------------------------------------
    // CDMViewportBase Protected Constructor/Destructor
    // ------------------------------------------------------------------------
protected:
    CDMViewportBase(_In_ CUIElement* pDMContainer)
        : m_pDMContainer(pDMContainer)
    {
        ASSERT(pDMContainer);
        AddRefInterface(pDMContainer);

        SetIsCrossSlideViewport(FALSE);

#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   CDMViewportBase constructor.", this));
        }
#endif // DM_DEBUG
    }

    ~CDMViewportBase() override
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   ~CDMViewportBase destructor.", this));
        }
#endif // DM_DEBUG
        m_contactIds.clear();

        ReleaseInterface(m_pDMContainer);
    }

    // ------------------------------------------------------------------------
    // CDMViewportBase Private Methods
    // ------------------------------------------------------------------------


    // ------------------------------------------------------------------------
    // CDMViewportBase Private Fields
    // ------------------------------------------------------------------------
private:
    // Associated contact Ids (pointer Ids)
    xvector<XUINT32> m_contactIds;

    // Element associated with this viewport
    CUIElement* m_pDMContainer;

    bool m_fIsCrossSlideViewport;
};


//------------------------------------------------------------------------
//
//  CDMViewport class.
//
//------------------------------------------------------------------------
class CDMViewport final : public CDMViewportBase
{
    // ------------------------------------------------------------------------
    // CDMViewport Public Methods
    // ------------------------------------------------------------------------
public:
    static _Check_return_ HRESULT Create(
        _Outptr_result_nullonfailure_ CDMViewport** ppDMViewport,
        _In_ CUIElement* pDMContainer,
        _In_ CUIElement* pManipulatedElement);

    CUIElement* GetManipulatedElementNoRef() const
    {
        return m_pManipulatedElement;
    }

#ifdef DBG
    void DebugStatuses()
    {
        XDMViewportStatus status = XcpDMViewportBuilding;
        XUINT64 statuses = 0;

        for (XUINT32 index = 0; index < m_statuses.size(); index++)
        {
            IGNOREHR(m_statuses.get_item(index, status));
            statuses *= 10;
            statuses += static_cast<XUINT64>(status);
        }

#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   DebugStatuses: %I64u.",
                this, statuses));
        }
#endif // DM_DEBUG

    }
#endif // DBG

    XUINT32 GetStatusesCount() const
    {
        return m_statuses.size();
    }

    IObject* GetCompositorViewportNoRef() const
    {
        return m_pCompositorViewport;
    }

    void SetCompositorViewport(_In_opt_ IObject* pCompositorViewport)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetCompositorViewport - old=0x%p, new=0x%p.",
                this, m_pCompositorViewport, pCompositorViewport));
        }
#endif // DM_DEBUG
        ReplaceInterface(m_pCompositorViewport, pCompositorViewport);
    }

    IObject* GetCompositorPrimaryContentNoRef() const
    {
        return m_pCompositorPrimaryContent;
    }

    void SetCompositorPrimaryContent(_In_opt_ IObject* pCompositorPrimaryContent)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetCompositorPrimaryContent - old=0x%p, new=0x%p.",
                this, m_pCompositorPrimaryContent, pCompositorPrimaryContent));
        }
#endif // DM_DEBUG
        ReplaceInterface(m_pCompositorPrimaryContent, pCompositorPrimaryContent);
    }

    bool GetIsPrimaryContentLayoutRefreshed() const
    {
        // DManipOnDComp_Staging:  This is now dead code
        ASSERT(FALSE);
        return m_fIsPrimaryContentLayoutRefreshed;
    }

    void SetIsPrimaryContentLayoutRefreshed(_In_ bool fIsPrimaryContentLayoutRefreshed)
    {
        m_fIsPrimaryContentLayoutRefreshed = fIsPrimaryContentLayoutRefreshed;
    }

    bool GetIsPrimaryContentLayoutRefreshedAfterStart() const
    {
        // DManipOnDComp_Staging:  This is now dead code
        ASSERT(FALSE);
        return m_fIsPrimaryContentLayoutRefreshedAfterStart;
    }

    void SetIsPrimaryContentLayoutRefreshedAfterStart(_In_ bool fIsPrimaryContentLayoutRefreshedAfterStart)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsPrimaryContentLayoutRefreshedAfterStart - old=%d, new=%d.",
                this, m_fIsPrimaryContentLayoutRefreshedAfterStart, fIsPrimaryContentLayoutRefreshedAfterStart));
        }
#endif // DM_DEBUG
        m_fIsPrimaryContentLayoutRefreshedAfterStart = fIsPrimaryContentLayoutRefreshedAfterStart;
    }

    bool GetIsPrimaryContentLayoutRefreshedAfterCompletion() const
    {
        // DManipOnDComp_Staging:  This is now dead code
        ASSERT(FALSE);
        return m_fIsPrimaryContentLayoutRefreshedAfterCompletion;
    }

    void SetIsPrimaryContentLayoutRefreshedAfterCompletion(_In_ bool fIsPrimaryContentLayoutRefreshedAfterCompletion)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsPrimaryContentLayoutRefreshedAfterCompletion - old=%d, new=%d.",
                this, m_fIsPrimaryContentLayoutRefreshedAfterCompletion, fIsPrimaryContentLayoutRefreshedAfterCompletion));
        }
#endif // DM_DEBUG
        m_fIsPrimaryContentLayoutRefreshedAfterCompletion = fIsPrimaryContentLayoutRefreshedAfterCompletion;
    }

    bool GetIsTouchConfigurationActivated() const
    {
        return m_fIsTouchConfigurationActivated;
    }

    void SetIsTouchConfigurationActivated(_In_ bool fIsTouchConfigurationActivated)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsTouchConfigurationActivated - old=%d, new=%d.",
                this, m_fIsTouchConfigurationActivated, fIsTouchConfigurationActivated));
        }
#endif // DM_DEBUG
        m_fIsTouchConfigurationActivated = fIsTouchConfigurationActivated;
    }

    bool GetIsNonTouchConfigurationActivated() const
    {
        return m_fIsNonTouchConfigurationActivated;
    }

    void SetIsNonTouchConfigurationActivated(_In_ bool fIsNonTouchConfigurationActivated)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsNonTouchConfigurationActivated - old=%d, new=%d.",
                this, m_fIsNonTouchConfigurationActivated, fIsNonTouchConfigurationActivated));
        }
#endif // DM_DEBUG
        m_fIsNonTouchConfigurationActivated = fIsNonTouchConfigurationActivated;
    }

    bool GetIsBringIntoViewportConfigurationActivated() const
    {
        return !m_fIsTouchConfigurationActivated && !m_fIsNonTouchConfigurationActivated;
    }

    bool GetIsTouchInteractionEndExpected() const
    {
        return m_fIsTouchInteractionEndExpected;
    }

    void SetIsTouchInteractionEndExpected(_In_ bool fIsTouchInteractionEndExpected)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsTouchInteractionEndExpected - old=%d, new=%d.",
                this, m_fIsTouchInteractionEndExpected, fIsTouchInteractionEndExpected));
        }
#endif // DM_DEBUG
        m_fIsTouchInteractionEndExpected = fIsTouchInteractionEndExpected;
    }

    bool GetIsTouchInteractionStartProcessed() const
    {
        return m_fIsTouchInteractionStartProcessed;
    }

    void SetIsTouchInteractionStartProcessed(_In_ bool fIsTouchInteractionStartProcessed)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsTouchInteractionStartProcessed - old=%d, new=%d.",
                this, m_fIsTouchInteractionStartProcessed, fIsTouchInteractionStartProcessed));
        }
#endif // DM_DEBUG
        m_fIsTouchInteractionStartProcessed = fIsTouchInteractionStartProcessed;
    }

    bool GetIsProcessingMakeVisibleInertia() const
    {
        return m_fIsProcessingMakeVisibleInertia;
    }

    void SetIsProcessingMakeVisibleInertia(_In_ bool fIsProcessingMakeVisibleInertia)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsProcessingMakeVisibleInertia - old=%d, new=%d.",
                this, m_fIsProcessingMakeVisibleInertia, fIsProcessingMakeVisibleInertia));
        }
#endif // DM_DEBUG
        m_fIsProcessingMakeVisibleInertia = fIsProcessingMakeVisibleInertia;
    }

    bool GetRequestReplayPointerUpdateWhenInertiaCompletes() const
    {
        return m_requestReplayPointerUpdateWhenInertiaCompletes;
    }

    void SetRequestReplayPointerUpdateWhenInertiaCompletes(bool value)
    {
        m_requestReplayPointerUpdateWhenInertiaCompletes = value;
    }

    bool GetHasNewManipulation() const
    {
        return m_fHasNewManipulation;
    }

    void SetHasNewManipulation(_In_ bool fHasNewManipulation)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetHasNewManipulation - old=%d, new=%d (HasOldManipulation=%d, IsCompositorAware=%d).",
                this, m_fHasNewManipulation, fHasNewManipulation, m_fHasOldManipulation, m_fIsCompositorAware));
        }
#endif // DM_DEBUG
        m_fHasNewManipulation = fHasNewManipulation;
    }

    bool GetHasOldManipulation() const
    {
        return m_fHasOldManipulation;
    }

    void SetHasOldManipulation(_In_ bool fHasOldManipulation)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetHasOldManipulation - old=%d, new=%d.",
                this, m_fHasOldManipulation, fHasOldManipulation));
        }
#endif // DM_DEBUG
        m_fHasOldManipulation = fHasOldManipulation;
    }

    bool GetIsCompositorAware() const
    {
        return m_fIsCompositorAware;
    }

    void SetIsCompositorAware(_In_ bool fIsCompositorAware)
    {
#ifdef DM_DEBUG
        if (m_fIsCompositorAware != fIsCompositorAware && (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT)))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsCompositorAware - old=%d, new=%d.",
                this, m_fIsCompositorAware, fIsCompositorAware));
        }
#endif // DM_DEBUG
        m_fIsCompositorAware = fIsCompositorAware;
    }

    bool GetHasDelayedStatusChangeProcessing() const
    {
        return m_fHasDelayedStatusChangeProcessing;
    }

    void SetHasDelayedStatusChangeProcessing(_In_ bool fHasDelayedStatusChangeProcessing)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetHasDelayedStatusChangeProcessing - old=%d, new=%d.",
                this, m_fHasDelayedStatusChangeProcessing, fHasDelayedStatusChangeProcessing));
        }
#endif // DM_DEBUG
        m_fHasDelayedStatusChangeProcessing = fHasDelayedStatusChangeProcessing;
    }

    bool GetHasReceivedContactIdInInertia() const
    {
        return m_fHasReceivedContactIdInInertia;
    }

    void SetHasReceivedContactIdInInertia(_In_ bool fHasReceivedContactIdInInertia)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetHasReceivedContactIdInInertia - old=%d, new=%d.",
                this, m_fHasReceivedContactIdInInertia, fHasReceivedContactIdInInertia));
        }
#endif // DM_DEBUG
        m_fHasReceivedContactIdInInertia = fHasReceivedContactIdInInertia;
    }

    bool GetNeedsUnregistration() const
    {
        return m_fNeedsUnregistration;
    }

    void SetNeedsUnregistration(_In_ bool fNeedsUnregistration)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetNeedsUnregistration - old=%d, new=%d.",
                this, m_fNeedsUnregistration, fNeedsUnregistration));
        }
#endif // DM_DEBUG
        m_fNeedsUnregistration = fNeedsUnregistration;
    }

    XUINT32 GetRemovedRunningStatuses() const
    {
        return m_cRemovedRunningStatuses;
    }

    void ResetRemovedRunningStatuses()
    {
        m_cRemovedRunningStatuses = 0;
    }

    void IncrementRemovedRunningStatuses()
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   IncrementRemovedRunningStatuses - old=%d, new=%d.",
                this, m_cRemovedRunningStatuses, m_cRemovedRunningStatuses + 1));
        }
#endif // DM_DEBUG
        m_cRemovedRunningStatuses++;
    }

    void DecrementRemovedRunningStatuses()
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   DecrementRemovedRunningStatuses - old=%d, new=%d.",
                this, m_cRemovedRunningStatuses, m_cRemovedRunningStatuses - 1));
        }
#endif // DM_DEBUG
        ASSERT(m_cRemovedRunningStatuses > 0);
        m_cRemovedRunningStatuses--;
    }

    bool GetUnregistered() const
    {
        return m_fUnregistered;
    }

    void SetUnregistered()
    {
        m_fUnregistered = TRUE;
    }

    bool GetIsCompletedStateSkipped() const
    {
        return m_fIsCompletedStateSkipped;
    }

    void SetIsCompletedStateSkipped(_In_ bool fIsCompletedStateSkipped)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetIsCompletedStateSkipped - old=%d, new=%d.",
                this, m_fIsCompletedStateSkipped, fIsCompletedStateSkipped));
        }
#endif // DM_DEBUG
        m_fIsCompletedStateSkipped = fIsCompletedStateSkipped;
    }

    XDMConfigurations GetActivatedConfiguration() const
    {
        if (GetIsTouchConfigurationActivated())
        {
            return GetTouchConfiguration();
        }
        else if (GetIsNonTouchConfigurationActivated())
        {
            return GetNonTouchConfiguration();
        }
        else
        {
            return GetBringIntoViewportConfiguration();
        }
    }

    XDMConfigurations GetTouchConfiguration() const
    {
        return m_touchConfiguration;
    }

    void SetTouchConfiguration(XDMConfigurations touchConfiguration)
    {
        m_touchConfiguration = touchConfiguration;
    }

    XDMConfigurations GetNonTouchConfiguration() const
    {
        return m_nonTouchConfiguration;
    }

    void SetNonTouchConfiguration(XDMConfigurations nonTouchConfiguration)
    {
        m_nonTouchConfiguration = nonTouchConfiguration;
    }

    XDMConfigurations GetBringIntoViewportConfiguration() const
    {
        return m_bringIntoViewportConfiguration;
    }

    void SetBringIntoViewportConfiguration(XDMConfigurations bringIntoViewportConfiguration)
    {
        m_bringIntoViewportConfiguration = bringIntoViewportConfiguration;
    }

    void GetInitialTransformationValues(
        _Out_ XFLOAT& initialTranslationX,
        _Out_ XFLOAT& initialTranslationY,
        _Out_ XFLOAT& initialUncompressedZoomFactor,
        _Out_ XFLOAT& initialZoomFactorX,
        _Out_ XFLOAT& initialZoomFactorY) const
    {
        initialTranslationX = m_initialTranslationX;
        initialTranslationY = m_initialTranslationY;
        initialUncompressedZoomFactor = m_initialUncompressedZoomFactor;
        initialZoomFactorX = m_initialZoomFactorX;
        initialZoomFactorY = m_initialZoomFactorY;
    }

    void SetInitialTransformationValues(
        _In_ XFLOAT initialTranslationX,
        _In_ XFLOAT initialTranslationY,
        _In_ XFLOAT initialUncompressedZoomFactor,
        _In_ XFLOAT initialZoomFactorX,
        _In_ XFLOAT initialZoomFactorY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/,
                L"DMV[0x%p]:   SetInitialTransformationValues - initialTranslationX=%4.6lf, initialTranslationY=%4.6lf, initialUncompressedZoomFactor=%4.8lf, initialZoomFactorX=%4.8lf, initialZoomFactorY=%4.8lf.",
                this, initialTranslationX, initialTranslationY, initialUncompressedZoomFactor, initialZoomFactorX, initialZoomFactorY));
        }
#endif // DM_DEBUG
        ASSERT(initialUncompressedZoomFactor != 0.0f);
        ASSERT(initialZoomFactorX != 0.0f);
        ASSERT(initialZoomFactorY != 0.0f);
        m_initialTranslationX = initialTranslationX;
        m_initialTranslationY = initialTranslationY;
        m_initialUncompressedZoomFactor = initialUncompressedZoomFactor;
        m_initialZoomFactorX = initialZoomFactorX;
        m_initialZoomFactorY = initialZoomFactorY;
    }

    void GetCurrentTransformationValues(
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& uncompressedZoomFactor,
        _Out_ XFLOAT& zoomFactorX,
        _Out_ XFLOAT& zoomFactorY) const
    {
        translationX = m_currentTranslationX;
        translationY = m_currentTranslationY;
        uncompressedZoomFactor = m_currentUncompressedZoomFactor;
        zoomFactorX = m_currentZoomFactorX;
        zoomFactorY = m_currentZoomFactorY;
    }

    void SetCurrentTransformationValues(
        _In_ XFLOAT translationX,
        _In_ XFLOAT translationY,
        _In_ XFLOAT uncompressedZoomFactor,
        _In_ XFLOAT zoomFactorX,
        _In_ XFLOAT zoomFactorY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/,
                L"DMV[0x%p]:   SetCurrentTransformationValues - translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.",
                this, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
        }
#endif // DM_DEBUG
        ASSERT(uncompressedZoomFactor != 0.0f);
        ASSERT(zoomFactorX != 0.0f);
        ASSERT(zoomFactorY != 0.0f);
        m_currentTranslationX = translationX;
        m_currentTranslationY = translationY;
        m_currentUncompressedZoomFactor = uncompressedZoomFactor;
        m_currentZoomFactorX = zoomFactorX;
        m_currentZoomFactorY = zoomFactorY;
    }

    void GetCurrentAutoScrollVelocities(
        _Out_ XFLOAT* pAutoScrollXVelocity,
        _Out_ XFLOAT* pAutoScrollYVelocity) const
    {
        *pAutoScrollXVelocity = m_currentAutoScrollXVelocity;
        *pAutoScrollYVelocity = m_currentAutoScrollYVelocity;
    }

    void SetCurrentAutoScrollVelocities(
        _In_ XFLOAT autoScrollXVelocity,
        _In_ XFLOAT autoScrollYVelocity)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/,
                L"DMV[0x%p]:   SetCurrentAutoScrollVelocities - autoScrollXVelocity=%4.6lf, autoScrollYVelocity=%4.6lf.",
                this, autoScrollXVelocity, autoScrollYVelocity));
        }
#endif // DM_DEBUG

        ASSERT(autoScrollXVelocity == 0.0f || autoScrollYVelocity == 0.0f);
        m_currentAutoScrollXVelocity = autoScrollXVelocity;
        m_currentAutoScrollYVelocity = autoScrollYVelocity;
    }

    void GetTranslationAdjustments(
        _Out_ XFLOAT& translationAdjustmentX,
        _Out_ XFLOAT& translationAdjustmentY) const
    {
        translationAdjustmentX = m_translationAdjustmentX;
        translationAdjustmentY = m_translationAdjustmentY;
    }

    void SetTranslationAdjustments(
        _In_ XFLOAT translationAdjustmentX,
        _In_ XFLOAT translationAdjustmentY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetTranslationAdjustments - translationAdjustmentX=%4.6lf, translationAdjustmentY=%4.6lf.",
                this, translationAdjustmentX, translationAdjustmentY));
        }
#endif // DM_DEBUG
        m_translationAdjustmentX = translationAdjustmentX;
        m_translationAdjustmentY = translationAdjustmentY;
    }

    void GetMargins(
        _Out_ XFLOAT& marginX,
        _Out_ XFLOAT& marginY) const
    {
        marginX = m_marginX;
        marginY = m_marginY;
    }

    void SetMargins(
        _In_ XFLOAT marginX,
        _In_ XFLOAT marginY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetMargins - marginX=%4.6lf, marginY=%4.6lf.",
                this, marginX, marginY));
        }
#endif // DM_DEBUG
        m_marginX = marginX;
        m_marginY = marginY;
    }

    void GetInitialContentOffsets(
        _Out_ XFLOAT& initialContentOffsetX,
        _Out_ XFLOAT& initialContentOffsetY) const
    {
        initialContentOffsetX = m_initialContentOffsetX;
        initialContentOffsetY = m_initialContentOffsetY;
    }

    void SetInitialContentOffsets(
        _In_ XFLOAT initialContentOffsetX,
        _In_ XFLOAT initialContentOffsetY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetInitialContentOffsets - initialContentOffsetX=%4.6lf, initialContentOffsetY=%4.6lf.",
                this, initialContentOffsetX, initialContentOffsetY));
        }
#endif // DM_DEBUG
        m_initialContentOffsetX = initialContentOffsetX;
        m_initialContentOffsetY = initialContentOffsetY;
    }

    void GetContentOffsets(
        _Out_ XFLOAT& contentOffsetX,
        _Out_ XFLOAT& contentOffsetY) const
    {
        contentOffsetX = m_contentOffsetX;
        contentOffsetY = m_contentOffsetY;
    }

    void SetContentOffsets(
        _In_ XFLOAT contentOffsetX,
        _In_ XFLOAT contentOffsetY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetContentOffsets - contentOffsetX=%4.6lf, contentOffsetY=%4.6lf.",
                this, contentOffsetX, contentOffsetY));
        }
#endif // DM_DEBUG
        m_contentOffsetX = contentOffsetX;
        m_contentOffsetY = contentOffsetY;
    }

    void SetContentOffsets(
        _In_ XFLOAT contentOffsetX,
        _In_ XFLOAT contentOffsetY,
        _Out_ bool* pfIsModified)
    {
        *pfIsModified = (m_contentOffsetX != contentOffsetX) || (m_contentOffsetY != contentOffsetY);
        SetContentOffsets(contentOffsetX, contentOffsetY);
    }

    bool GetTargetTranslation(
        _Out_ float& targetTranslationX,
        _Out_ float& targetTranslationY) const
    {
        targetTranslationX = m_targetTranslationX;
        targetTranslationY = m_targetTranslationY;
        return m_areTargetTranslationsValid;
    }

    void SetTargetTranslation(
        _In_ float targetTranslationX,
        _In_ float targetTranslationY)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetTargetTranslations - targetTranslationX=%4.6lf, targetTranslationY=%4.6lf.",
                this, targetTranslationX, targetTranslationY));
        }
#endif // DM_DEBUG
        m_targetTranslationX = targetTranslationX;
        m_targetTranslationY = targetTranslationY;
        m_areTargetTranslationsValid = true;
    }


    void GetCompositorTransformationValues(
        _Out_ XFLOAT& translationX,
        _Out_ XFLOAT& translationY,
        _Out_ XFLOAT& zoomFactor) const
    {
        translationX = m_compositorTranslationX;
        translationY = m_compositorTranslationY;
        zoomFactor = m_compositorZoomFactor;
    }

    void SetCompositorTransformationValues(
        _In_ XFLOAT translationX,
        _In_ XFLOAT translationY,
        _In_ XFLOAT zoomFactor)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetCompositorTransformationValues - translationX=%4.6lf, translationY=%4.6lf, zoomFactor=%4.8lf.",
                this, translationX, translationY, zoomFactor));
        }
#endif // DM_DEBUG
        ASSERT(zoomFactor != 0.0f);
        m_compositorTranslationX = translationX;
        m_compositorTranslationY = translationY;
        m_compositorZoomFactor = zoomFactor;
    }

    XUINT32 GetUITicksForNotifications() const
    {
        return m_cUITicksForNotifications;
    }

    void SetUITicksForNotifications(XUINT32 count)
    {
        m_cUITicksForNotifications = count;
    }

    XUINT32 GetIgnoredRunningStatuses() const
    {
        return m_cIgnoredRunningStatuses;
    }

    void ResetIgnoredRunningStatuses()
    {
        m_cIgnoredRunningStatuses = 0;
    }

    void IncrementIgnoredRunningStatuses()
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   IncrementIgnoredRunningStatuses - old=%d, new=%d.",
                this, m_cIgnoredRunningStatuses, m_cIgnoredRunningStatuses + 1));
        }
#endif // DM_DEBUG
        m_cIgnoredRunningStatuses++;
    }

    void DecrementIgnoredRunningStatuses()
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   DecrementIgnoredRunningStatuses - old=%d, new=%d.",
                this, m_cIgnoredRunningStatuses, m_cIgnoredRunningStatuses - 1));
        }
#endif // DM_DEBUG
        ASSERT(m_cIgnoredRunningStatuses > 0);
        m_cIgnoredRunningStatuses--;
    }

    DirectManipulationState GetState() const
    {
        return m_state;
    }

    void SetState(DirectManipulationState state)
    {
        if (m_fIsPrimaryContentLayoutRefreshed && (state == ManipulationStarting || state == ConstantVelocityScrollStarted))
        {
            // Detect the first re-layout of panels that arrange themselves differently during a DManip.
            SetIsPrimaryContentLayoutRefreshedAfterStart(FALSE);
        }
        if (state == ManipulationCompleted)
        {
            // Once we have completed a manipulation, our target offset values will no longer
            // be valid until they are set again.
            m_areTargetTranslationsValid = false;
        }

        m_state = state;
    }

    bool GetIsPreConstantVelocityPanStateStarting() const
    {
        return m_fIsPreConstantVelocityPanStateStarting;
    }

    void SetIsPreConstantVelocityPanStateStarting(_In_ bool fIsPreConstantVelocityPanStateStarting)
    {
        m_fIsPreConstantVelocityPanStateStarting = fIsPreConstantVelocityPanStateStarting;
    }

    bool GetIsCompletedStateDelayedByConstantVelocityPan() const
    {
        return m_fIsCompletedStateDelayedByConstantVelocityPan;
    }

    void SetIsCompletedStateDelayedByConstantVelocityPan(_In_ bool fIsCompletedStateDelayedByConstantVelocityPan)
    {
        m_fIsCompletedStateDelayedByConstantVelocityPan = fIsCompletedStateDelayedByConstantVelocityPan;
    }

    // Returns True when the HasNewManipulation
    // was set to True
    bool DeclareNewViewportForCompositor()
    {
        if (GetHasOldManipulation())
        {
            SetHasOldManipulation(FALSE);
            ASSERT(GetIsCompositorAware());
        }
        else if (!GetIsCompositorAware())
        {
            SetHasNewManipulation(TRUE);
            return true;
        }
        return false;
    }

    void DeclareOldViewportForCompositor()
    {
        ASSERT(!GetHasReceivedContactIdInInertia());

        if (GetIsCompositorAware())
        {
            SetHasOldManipulation(TRUE);
        }
        else if (GetHasNewManipulation())
        {
            SetHasNewManipulation(FALSE);
        }
    }

    XUINT32 GetContentsCount() const
    {
        return m_pContents ? m_pContents->size() : 0;
    }

    XUINT32 GetClipContentsCount() const
    {
        return m_pClipContents ? m_pClipContents->size() : 0;
    }

    bool GetHasDMHitTestContactId() const
    {
        return m_fHasDMHitTestContactId;
    }

    void SetHasDMHitTestContactId(_In_ bool fHasDMHitTestContactId)
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetHasDMHitTestContactId - old=%d, new=%d.",
                this, m_fHasDMHitTestContactId, fHasDMHitTestContactId));
        }
#endif // DM_DEBUG
        m_fHasDMHitTestContactId = fHasDMHitTestContactId;
    }

    _Check_return_ HRESULT GetOldStatus(_Out_ XDMViewportStatus& oldStatus);
    _Check_return_ HRESULT SetOldStatus(_In_ XDMViewportStatus oldStatus);
    _Check_return_ HRESULT GetCurrentStatus(_Out_ XDMViewportStatus& currentStatus);
    _Check_return_ HRESULT SetCurrentStatus(_In_ XDMViewportStatus currentStatus);
    _Check_return_ HRESULT GetStatus(_In_ XUINT32 index, _Out_ XDMViewportStatus& status);

    // Adds the provided interaction type to the end of the interaction types queue.
    void PushInteractionType(_In_ XDMViewportInteractionType interactionType);
    // Returns the front of the interaction type queue.
    XDMViewportInteractionType GetFrontInteractionType();
    // Returns the back of the interaction type queue.
    XDMViewportInteractionType GetBackInteractionType();
    // Deletes the front of the interaction type queue.
    void PopInteractionType();
    bool HasQueuedInteractionType() { return !m_queuedInteractionTypes.empty(); }
    void ClearQueuedInteractionTypes() { while (!m_queuedInteractionTypes.empty()) m_queuedInteractionTypes.pop(); }

    XUINT32 GetConfigurationCount();
    _Check_return_ HRESULT GetConfiguration(_In_ XUINT32 index, _Out_ XDMConfigurations& configuration);
    _Check_return_ HRESULT HasConfiguration(_In_ XDMConfigurations configuration, _Out_ bool& fHasConfiguration);
    _Check_return_ HRESULT AddConfiguration(_In_ XDMConfigurations configuration);
    _Check_return_ HRESULT RemoveConfiguration(_In_ XDMConfigurations configuration);

    _Check_return_ HRESULT SetManipulatedElement(_In_ CUIElement* pManipulatedElement);
    _Check_return_ HRESULT AddContent(_In_ CUIElement* pContentElement, _In_ XDMContentType contentType, _Outptr_result_maybenull_ CDMContent** ppContent);
    _Check_return_ HRESULT RemoveContent(_In_ CUIElement* pContentElement, _Outptr_result_maybenull_ CDMContent** ppContent);
    _Check_return_ HRESULT ClearContents();
    _Check_return_ HRESULT GetContent(_In_ XUINT32 contentIndex, _Outptr_ CDMContent** ppContent);
    _Check_return_ HRESULT GetContentNoRef(_In_ CUIElement* pContentElement, _Out_opt_ CDMContent** ppContent, _Out_opt_ XUINT32* pContentIndex);
    _Check_return_ HRESULT GetContentElementNoRef(_In_ XUINT32 contentIndex, _Out_ CUIElement** ppContentElement);
    _Check_return_ HRESULT AddClipContent(_In_ CUIElement* pContentElement, _In_ XDMContentType contentType, _Outptr_result_maybenull_ CDMContent** ppContent);
    _Check_return_ HRESULT RemoveClipContent(_In_ CUIElement* pContentElement, _Outptr_result_maybenull_ CDMContent** ppContent);
    _Check_return_ HRESULT ClearClipContents();
    _Check_return_ HRESULT GetClipContent(_In_ XUINT32 contentIndex, _Outptr_ CDMContent** ppContent);
    _Check_return_ HRESULT GetClipContentNoRef(_In_ CUIElement* pContentElement, _Out_opt_ CDMContent** ppContent, _Out_opt_ XUINT32* pContentIndex);
    _Check_return_ HRESULT GetClipContentElementNoRef(_In_ XUINT32 contentIndex, _Out_ CUIElement** ppContentElement);

    _Check_return_ HRESULT HasActiveStatus(_Out_ bool& fHasActiveStatus);
    _Check_return_ HRESULT RemoveOldStatus();
    _Check_return_ HRESULT RemoveCurrentStatus();
    _Check_return_ HRESULT RemoveStatus(_In_ XUINT32 index);

    void SetHasValidBounds(bool hasValidBounds)
    {
        m_hasValidBounds = hasValidBounds;
    }

    bool HasValidBounds() const
    {
        return m_hasValidBounds;
    }

    void SetHasViewportInteraction(bool hasViewportInteraction)
    {
        m_hasViewportInteraction = hasViewportInteraction;
    }

    bool HasViewportInteraction() const
    {
        return m_hasViewportInteraction;
    }

    // ------------------------------------------------------------------------
    // CDMViewport Private Constructor/Destructor
    // ------------------------------------------------------------------------
private:
    CDMViewport(_In_ CUIElement* pDMContainer, _In_ CUIElement* pManipulatedElement)
        : CDMViewportBase(pDMContainer)
        , m_cUITicksForNotifications(0)
        , m_cRemovedRunningStatuses(0)
        , m_cIgnoredRunningStatuses(0)
        , m_pManipulatedElement(pManipulatedElement)
        , m_pContents(NULL)
        , m_pClipContents(NULL)
        , m_fHasNewManipulation(FALSE)
        , m_fHasOldManipulation(FALSE)
        , m_fNeedsUnregistration(FALSE)
        , m_fUnregistered(FALSE)
        , m_fIsCompletedStateSkipped(FALSE)
        , m_fIsPreConstantVelocityPanStateStarting(FALSE)
        , m_fIsCompletedStateDelayedByConstantVelocityPan(FALSE)
        , m_fIsCompositorAware(FALSE)
        , m_fHasDelayedStatusChangeProcessing(FALSE)
        , m_fHasReceivedContactIdInInertia(FALSE)
        , m_fIsPrimaryContentLayoutRefreshed(FALSE)
        , m_fIsPrimaryContentLayoutRefreshedAfterStart(FALSE)
        , m_fIsPrimaryContentLayoutRefreshedAfterCompletion(FALSE)
        , m_fIsTouchConfigurationActivated(FALSE)
        , m_fIsNonTouchConfigurationActivated(FALSE)
        , m_fIsTouchInteractionEndExpected(FALSE)
        , m_fIsTouchInteractionStartProcessed(FALSE)
        , m_fIsProcessingMakeVisibleInertia(FALSE)
        , m_requestReplayPointerUpdateWhenInertiaCompletes(FALSE)
        , m_pCompositorViewport(NULL)
        , m_pCompositorPrimaryContent(NULL)
        , m_touchConfiguration(XcpDMConfigurationNone)
        , m_nonTouchConfiguration(XcpDMConfigurationNone)
        , m_bringIntoViewportConfiguration(XcpDMConfigurationNone)
        , m_initialTranslationX(0.0f)
        , m_initialTranslationY(0.0f)
        , m_initialUncompressedZoomFactor(1.0f)
        , m_initialZoomFactorX(1.0f)
        , m_initialZoomFactorY(1.0f)
        , m_currentTranslationX(0.0f)
        , m_currentTranslationY(0.0f)
        , m_currentUncompressedZoomFactor(1.0f)
        , m_currentZoomFactorX(1.0f)
        , m_currentZoomFactorY(1.0f)
        , m_currentAutoScrollXVelocity(0.0f)
        , m_currentAutoScrollYVelocity(0.0f)
        , m_translationAdjustmentX(0.0f)
        , m_translationAdjustmentY(0.0f)
        , m_marginX(0.0f)
        , m_marginY(0.0f)
        , m_initialContentOffsetX(0.0f)
        , m_initialContentOffsetY(0.0f)
        , m_contentOffsetX(0.0f)
        , m_contentOffsetY(0.0f)
        , m_compositorTranslationX(0.0f)
        , m_compositorTranslationY(0.0f)
        , m_compositorZoomFactor(1.0f)
        , m_state(ManipulationNone)
        , m_fHasDMHitTestContactId(FALSE)
    {
        ASSERT(pManipulatedElement);
        AddRefInterface(pManipulatedElement);

#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   CDMViewport constructor.", this));
        }
#endif // DM_DEBUG
    }

    ~CDMViewport() override
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   ~CDMViewport destructor.", this));
        }
#endif // DM_DEBUG

        ReleaseInterface(m_pManipulatedElement);
        ReleaseInterface(m_pCompositorViewport);
        ReleaseInterface(m_pCompositorPrimaryContent);
        ClearContents();
        ClearClipContents();
    }


    // ------------------------------------------------------------------------
    // CDMViewport Private Methods
    // ------------------------------------------------------------------------
    _Check_return_ HRESULT GetConfigurationIndex(_In_ XDMConfigurations configuration, _Out_ XINT32& configurationIndex);


    // ------------------------------------------------------------------------
    // CDMViewport Private Fields
    // ------------------------------------------------------------------------
private:
    // Number of UI thread ticks cumulated since the owning DM container
    // was told that its manipulation handler is interested in receiving
    // characteristic notifications. This count is reset to 0 each time
    // the current characteristics are used for a DM ProcessInput call.
    // m_cUITicksForNotifications == 0 --> no notifications will be sent.
    // m_cUITicksForNotifications > 0 --> notifications will be sent.
    XUINT32 m_cUITicksForNotifications;

    // How many incoming status transitions to Running to remove.
    // Count is incremented when enabling the viewport caused a transitional Running status that
    // needs to be ignored.
    XUINT32 m_cRemovedRunningStatuses;

    // How many incoming status transitions to Running to ignore.
    // For instance, synchronous processing of BringIntoViewport calls
    // cause m_cIgnoredRunningStatuses to be incremented.
    XUINT32 m_cIgnoredRunningStatuses;

    // Manipulated element associated with this viewport.
    // Corresponds to the primary content of the viewport.
    CUIElement* m_pManipulatedElement;

    // Array of secondary contents (like headers)
    xvector<CDMContent*>* m_pContents;

    // Array of secondary clip contents
    xvector<CDMContent*>* m_pClipContents;

    union
    {
        UINT32 m_bitFieldsAsDWORD;

        struct
        {
            // When True, the compositor thread has not been notified of a new manipulation yet
            bool m_fHasNewManipulation : 1;

            // When True, the compositor thread has not been notified of an old manipulation yet
            bool m_fHasOldManipulation : 1;

            // Set to True when the viewport needs to be discarded after the compositor was made
            // aware of its demise.
            bool m_fNeedsUnregistration : 1;

            // Set to True when the viewport is removed from the InputManager's viewport list and
            // becomes unusable.
            bool m_fUnregistered : 1;

            // Set to True when a Ready DM status needs to be ignored and the viewport must not
            // enter the ManipulationCompleted state. This occurs for example when BringIntoViewport
            // is invoked at the end of a manipulation.
            bool m_fIsCompletedStateSkipped : 1;

            // Set to True when the viewport entered a constant velocity pan while its state was ManipulationStarting.
            // The viewport will restore to ManipulationStarting after the pan, if a completion was not delayed.
            bool m_fIsPreConstantVelocityPanStateStarting : 1;

            // Set to True when the viewport delays its transition to the ManipulationCompleted state until an
            // ongoing constant velocity pan completes.
            bool m_fIsCompletedStateDelayedByConstantVelocityPan : 1;

            // When True, the compositor thread has an active CCompositorDirectManipulationViewport
            // for this viewport
            bool m_fIsCompositorAware : 1;

            // Used for workaround of bug 609050
            // Set to True when the viewport status change processing is delayed to the next UI tick
            bool m_fHasDelayedStatusChangeProcessing : 1;

            // When True, a contact Id was added to the m_contactIds vector while the viewport was in inertia phase.
            bool m_fHasReceivedContactIdInInertia : 1;

#pragma region DManipOnDComp_Staging
// These 3 flags can be removed when DManip-on-DComp is complete

            // When True, the primary content is re-arranged on the UI thread during a manipulation
            bool m_fIsPrimaryContentLayoutRefreshed : 1;

            // Set to False when state changes to ManipulationStarting for a viewport with m_fIsPrimaryContentLayoutRefreshed set to True.
            // Then set to True when a layout of the manipulated element occurred, marking the beginning DM's offset consumption by the UI thread.
            bool m_fIsPrimaryContentLayoutRefreshedAfterStart : 1;

            // Set to False when state changes to ManipulationCompleted. Then set to True when a layout
            // of the manipulated element occurred, marking the end of DM's offset consumption by the UI thread.
            bool m_fIsPrimaryContentLayoutRefreshedAfterCompletion : 1;

#pragma endregion DManipOnDComp_Staging

            // Set to True when the configuration dedicated to touch (m_touchConfiguration) was activated in DM.
            // Set to False when the keyboard / mouse-wheel configuration (m_nonTouchConfiguration) was activated instead.
            // Or when the bring-into-viewport configuration was activated.
            bool m_fIsTouchConfigurationActivated : 1;

            // Set to True when the keyboard / mouse-wheel configuration (m_nonTouchConfiguration) was activated in DM.
            // Set to False when the configuration dedicated to touch (m_touchConfiguration) was activated instead,
            // or when the bring-into-viewport configuration was activated.
            bool m_fIsNonTouchConfigurationActivated : 1;

            // Set to True when a XcpDMViewportInteractionBegin notification is received for a touch interaction.
            // A XcpDMViewportInteractionEnd notification is thus expected to mark the completion of the interaction.
            // This flag is used to determine whether a ScrollViewer.DirectManipulationCompleted event must be raised
            // or not when receiving a XcpDMViewportInteractionEnd notification.
            bool m_fIsTouchInteractionEndExpected : 1;

            // Set to True when a transition to the Running status has already triggered a ScrollViewer.DirectManipulationCompleted
            // event for the current touch interaction. Any subsequent transition to Running for the interaction must skip the event.
            bool m_fIsTouchInteractionStartProcessed : 1;

            // Set to True when a MakeVisible call triggered a bring-into-viewport animation.
            bool m_fIsProcessingMakeVisibleInertia : 1;

            // Set to True when a SetContact call was made for this viewport
            // due to a DM_POINTERHITTEST message.
            // A subsequent viewport interaction type of XcpDMViewportInteractionEnd
            // then means that all contact points were released.
            bool m_fHasDMHitTestContactId : 1;

            // When a mouse wheel scroll completes, we want to replay the most recent pointer update (mouse move) message to hit
            // test again. Mouse wheel scroll is handled by DM through inertia, so replay the pointer update when we transition
            // out of the inertia state and back into ready.
            // Flick is also handled with inertia, and in that case we don't want to replay the pointer update - the mouse move
            // could have happened a long time before the finger flicked, and replaying that pointer update could put hover on
            // an element in some part of the tree totally unrelated to the flick.
            bool m_requestReplayPointerUpdateWhenInertiaCompletes : 1;
        };
    };

    // Object provided to the compositor thread for interaction with the
    // IPALDirectManipulationCompositorService interface. It wraps
    // a IDirectManipulationViewport pointer.
    IObject* m_pCompositorViewport;

    // Object provided to the compositor thread for interaction with the
    // IPALDirectManipulationCompositorService interface. It wraps
    // a IDirectManipulationContent pointer for the primary content.
    IObject* m_pCompositorPrimaryContent;

    // Current active configuration
    XDMConfigurations m_touchConfiguration;

    // Current non-touch configuration
    XDMConfigurations m_nonTouchConfiguration;

    // Current bring-into-viewport configuration
    XDMConfigurations m_bringIntoViewportConfiguration;

    // Array of possible configurations
    xvector<XDMConfigurations> m_configurations;

    // DM statuses set in between successive UI ticks
    xvector<XDMViewportStatus> m_statuses;

    // DM interaction types queued in between successive UI ticks
    std::queue<XDMViewportInteractionType> m_queuedInteractionTypes;

    // Transformation values at the beginning of the current manipulation
    XFLOAT m_initialTranslationX;
    XFLOAT m_initialTranslationY;
    XFLOAT m_initialUncompressedZoomFactor;
    XFLOAT m_initialZoomFactorX;
    XFLOAT m_initialZoomFactorY;

    // Latest transformation values recorded at UI thread tick - used for
    // ManipulationDelta/Completed events
    XFLOAT m_currentTranslationX;
    XFLOAT m_currentTranslationY;
    XFLOAT m_currentUncompressedZoomFactor;
    XFLOAT m_currentZoomFactorX;
    XFLOAT m_currentZoomFactorY;

    // Targeted transformation values.  When a ZoomToRect is in progress, these
    // members will contain the ending values for the manipulation.  We don't need
    // to worry about the scale factors here, because this is to support
    // BringIntoView which only handles translations.
    XFLOAT m_targetTranslationX = 0;
    XFLOAT m_targetTranslationY= 0;
    bool m_areTargetTranslationsValid = false;

    // Latest auto-scrolling velocities set in CInputServices::SetConstantVelocities.
    XFLOAT m_currentAutoScrollXVelocity;
    XFLOAT m_currentAutoScrollYVelocity;

    // Translation adjustments based on zoom factor, content size and alignment
    // Margins are not taken into account
    XFLOAT m_translationAdjustmentX;
    XFLOAT m_translationAdjustmentY;

    // Manipulated element's margins at the beginning of a manipulation
    XFLOAT m_marginX;
    XFLOAT m_marginY;

    // Top/left content offsets at the start of a DM manipulation.
    XFLOAT m_initialContentOffsetX;
    XFLOAT m_initialContentOffsetY;

    // Top/left content offsets provided to DM's SetContentRect API.
    // Starts at (0, 0) for any manipulation and is updated in case
    // a non-DM driven offset change occurs.
    // These fields are not reset to (0, 0) at the end of a manipulation.
    XFLOAT m_contentOffsetX;
    XFLOAT m_contentOffsetY;

    // Transformation values applied on the compositor thread
    XFLOAT m_compositorTranslationX;
    XFLOAT m_compositorTranslationY;
    XFLOAT m_compositorZoomFactor;

    // Keeps track of which phase a manipulation is in, and which events were raised for it.
    DirectManipulationState m_state;

    // True if SetViewportBounds has been called for this viewport
    bool m_hasValidBounds = false;

    // True if CDirectManipulationService::CreateViewportInteraction has been called for this viewport
    bool m_hasViewportInteraction = false;
};


//------------------------------------------------------------------------
//
//  CDMCrossSlideViewport class.
//  These cross-slide viewports are used to support cross-slide scenarios.
//  A CDMCrossSlideViewport is temporarily assigned to an element with
//  m_fIsDirectManipulationCrossSlideContainer set to True during a cross-slide
//  gesture.
//
//------------------------------------------------------------------------
class CDMCrossSlideViewport : public CDMViewportBase
{
    // ------------------------------------------------------------------------
    // CDMCrossSlideViewport Public Methods
    // ------------------------------------------------------------------------
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDMCrossSlideViewport** ppDMCrossSlideViewport,
        _In_ CUIElement* pDMContainer);

    bool HasParentViewportConfiguration()
    {
        return m_fHasParentViewportConfiguration;
    }

    void SetParentViewportConfiguration(
        _In_ XDMConfigurations parentViewportConfiguration)
    {
        ASSERT(!m_fHasParentViewportConfiguration);
        m_parentViewportConfiguration = parentViewportConfiguration;
        m_parentViewportsCombinedConfigurations = parentViewportConfiguration;
        m_fHasParentViewportConfiguration = TRUE;
    }

    XDMConfigurations GetParentViewportConfiguration() const
    {
        return m_parentViewportConfiguration;
    }

    void CombineParentViewportsConfigurations(
        _In_ XDMConfigurations parentViewportsConfigurations)
    {
        ASSERT(m_fHasParentViewportConfiguration);
        XUINT32 configs = static_cast<XUINT32>(m_parentViewportsCombinedConfigurations);
        configs |= static_cast<XUINT32>(parentViewportsConfigurations);
        m_parentViewportsCombinedConfigurations = static_cast<XDMConfigurations>(parentViewportsConfigurations);
    }

    XDMConfigurations GetParentViewportsCombinedConfigurations() const
    {
        return m_parentViewportsCombinedConfigurations;
    }

    bool GetNeedsStart() const
    {
        return m_fNeedsStart;
    }

    void SetNeedsStart(_In_ bool fNeedsStart)
    {
        m_fNeedsStart = fNeedsStart;
    }

    bool GetIsRejectionViewport() const
    {
        return m_fIsRejectionViewport;
    }

    void SetIsRejectionViewport(_In_ bool fIsRejectionViewport)
    {
        m_fIsRejectionViewport = fIsRejectionViewport;
    }

    // ------------------------------------------------------------------------
    // CDMCrossSlideViewport Private Constructor/Destructor
    // ------------------------------------------------------------------------
private:
    CDMCrossSlideViewport(_In_ CUIElement* pDMContainer)
        : CDMViewportBase(pDMContainer)
        , m_parentViewportConfiguration(XcpDMConfigurationNone)
        , m_parentViewportsCombinedConfigurations(XcpDMConfigurationNone)
        , m_fHasParentViewportConfiguration(FALSE)
        , m_fNeedsStart(TRUE)
        , m_fIsRejectionViewport(FALSE)
    {
        SetIsCrossSlideViewport(TRUE);

#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   CDMCrossSlideViewport constructor.", this));
        }
#endif // DM_DEBUG
    }

    ~CDMCrossSlideViewport() override
    {
#ifdef DM_DEBUG
        if (DMV_DBG || gps->IsDebugTraceTypeActive(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   ~CDMCrossSlideViewport destructor.", this));
        }
#endif // DM_DEBUG
    }

    // ------------------------------------------------------------------------
    // CDMCrossSlideViewport Private Fields
    // ------------------------------------------------------------------------
private:
    // Active configuration of the first ScrollViewer encountered in the parent
    // chain of the UIElement with m_fIsDirectManipulationCrossSlideContainer == True
    XDMConfigurations m_parentViewportConfiguration;

    // Combination of the active configuration of the all ScrollViewer controls
    // encountered in the parent chain of the UIElement with m_fIsDirectManipulationCrossSlideContainer == True
    XDMConfigurations m_parentViewportsCombinedConfigurations;

    // Set to True when the m_parentViewportConfiguration is set.
    bool m_fHasParentViewportConfiguration;

    // Set to True when the element with m_fIsDirectManipulationCrossSlideContainer == True
    // needs to be notified of the potential cross-slide gesture with a call to
    // OnDirectManipulationCrossSlideContainerStart
    bool m_fNeedsStart;

    // Set to True when this viewport is used to handle a ManipulationMode System|TranslateX/Y/RailsX/RailsY/Inertia,
    // System|Scale or System|Scale|ScaleInertia.
    bool m_fIsRejectionViewport;
};

