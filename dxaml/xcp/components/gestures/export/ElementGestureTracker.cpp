// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ElementGestureTracker.h"
#include "GestureOutputMapper.h"

#include <VisualTree.h>
#include <corep.h>
#include "RootScale.h"
#include <interactioncontext.h>
#include <windowscollections.h>
#include <ReentrancyGuard.h>

using namespace DirectUI;

ElementGestureTracker::ElementGestureTracker(
    _In_ CUIElement *pElement,
    _In_ bool bTapEnabled,
    _In_ bool bDoubleTapEnabled,
    _In_ bool bRightTapEnabled,
    _In_ bool bHoldEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode)
    : m_pElement(pElement)
    , m_bTapEnabled(bTapEnabled)
    , m_bDoubleTapEnabled(bDoubleTapEnabled)
    , m_bRightTapEnabled(bRightTapEnabled)
    , m_bHoldEnabled(bHoldEnabled)
    , m_manipulationMode(manipulationMode)
{
    ASSERT(!(static_cast<unsigned int>(m_manipulationMode) & SystemManipulationModeMask));

    VERIFYHR(InitializeInteractionEngine());
};

ElementGestureTracker::~ElementGestureTracker() {}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::ProcessPointerMessage
//
//  Synopsis:   Process the pointer data to ICM
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
ElementGestureTracker::ProcessPointerMessage(_In_ const InputMessage& msg)
{
    switch (msg.m_msgID)
    {
    case XCP_POINTERDOWN:
    case XCP_POINTERUP:
    case XCP_POINTERUPDATE:
    {
        // Process the pointer messages to feed pointer data into Touch Interaction Engine
        IFC_RETURN(ProcessPointerInformation(msg));
        break;
    }
    default:
        break;
    }

    return S_OK;
}

// The m_bPegged flag allows clients to check if this interaction engine can be destroyed.
void
ElementGestureTracker::UpdatePeg(bool peg)
{
    m_bPegged = peg;

    if (m_bPegged == false && m_bDelayDestroyInteractionEngine)
    {
        Destroy();
    }
}

bool
ElementGestureTracker::IsPegged()
{
    return m_bPegged;
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::Destroy
//
//  Synopsis:   Delete ElementGestureTracker object that will destroy ICM
//
//-------------------------------------------------------------------------
void
ElementGestureTracker::Destroy()
{
    if (m_bPegged)
    {
        // Don't destroy the ElementGestureTracker while it's pegged, but remember
        // to destroy it when it gets unpegged.
        m_bDelayDestroyInteractionEngine = TRUE;
    }
    else
    {
#ifdef POINTER_TRACING
        gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: Destroy\r\n", this);
#endif // POINTER_TRACING

        // Delay the destroying interaction engine if it is the middle of processing
        // interaction output message.

        const bool processingOutput = m_gestureOutputMapper->IsProcessingOutput();

        if (processingOutput)
        {
            m_bDelayDestroyInteractionEngine = TRUE;
        }
        else
        {
            delete this;
        }
    }
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::Reset
//
//  Synopsis:   Reset Interaction Context Manager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
ElementGestureTracker::ResetAndReinitialize(
    _In_ CUIElement *pElement,
    _In_ bool bTapEnabled,
    _In_ bool bDoubleTapEnabled,
    _In_ bool bRightTapEnabled,
    _In_ bool bHoldEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode)
{
#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: ResetAndReinitialize\r\n", this);
#endif // POINTER_TRACING

    // manipulationMode must be the custom manipulation mode.
    ASSERT(!(static_cast<unsigned int>(manipulationMode) & SystemManipulationModeMask));

    IFC_RETURN(Reset());

    m_pElement = xref::weakref_ptr<CUIElement>(pElement);
    m_bTapEnabled = bTapEnabled;
    m_bDoubleTapEnabled = bDoubleTapEnabled;
    m_bRightTapEnabled = bRightTapEnabled;
    m_bHoldEnabled = bHoldEnabled;
    m_manipulationMode = manipulationMode;

    m_gestureRecognizerAdapter.m_gestureRecognizer.Reset();

    // Need to re-initialize ICM settings(config, output callback and properties) after reset ICM
    IFC_RETURN(InitializeInteractionEngine());

    return S_OK;
}

_Check_return_ HRESULT
ElementGestureTracker::Reset()
{
#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: Reset\r\n", this);
#endif // POINTER_TRACING

    m_gestureRecognizerAdapter.Reset(this);

    m_pElement.reset();

    m_frameId = 0;
    m_cPointerInteractionContexts = 0;
    SetManipulationStarting(false);
    SetManipulationStarted(false);

    m_floatPivotRadius = 0.0f;
    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::SetConfiguration
//
//  Synopsis:   Set the configuration for Interaction Context Manager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
ElementGestureTracker::SetConfiguration(
    _In_ bool bTapEnabled,
    _In_ bool bDoubleTapEnabled,
    _In_ bool bRightTapEnabled,
    _In_ bool bHoldEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode,
    _In_ XPOINTF ptPivotCenter,
    _In_ float fPivotRadius)
{
    // manipulationMode must be the custom manipulation mode.
    ASSERT(!(static_cast<unsigned int>(manipulationMode) & SystemManipulationModeMask));

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: SetConfiguration: bTapEnabled=%d, bDoubleTapEnabled=%d, bRightTapEnabled=%d, bHoldEnabled=%d, manipulationMode=0x%x\r\n",
        this, bTapEnabled, bDoubleTapEnabled, bRightTapEnabled, bHoldEnabled, manipulationMode);
#endif // POINTER_TRACING

    SetInteractionStopping(false);
    SetInteractionStopped(false);

    // ICM configuration settings
    if (m_bForceUpdateSettings ||
        manipulationMode != m_manipulationMode ||
        bTapEnabled != m_bTapEnabled ||
        bDoubleTapEnabled != m_bDoubleTapEnabled ||
        bRightTapEnabled != m_bRightTapEnabled ||
        bHoldEnabled != m_bHoldEnabled)
    {
        // Reset ICM and reconfig the setting
        IFC_RETURN(ResetAndReinitialize(m_pElement.lock(), bTapEnabled, bDoubleTapEnabled, bRightTapEnabled, bHoldEnabled, manipulationMode));
    }

    if (!_isnan(ptPivotCenter.x) && !_isnan(ptPivotCenter.y) && !_isnan(fPivotRadius))
    {
        m_bPivot = true;
        m_floatPivotRadius = fPivotRadius;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::IsIdle
//
//  Synopsis:   TRUE if interaction context manager is idle state
//
//-------------------------------------------------------------------------
bool
ElementGestureTracker::IsIdle()
{
    boolean isActive;
    if (FAILED(m_gestureRecognizerAdapter.m_gestureRecognizer->get_IsActive(&isActive)))
    {
        return false;
    }

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: IsIdle: state=%d\r\n", this, !isActive);
#endif // POINTER_TRACING

    return !isActive;
}

// bCallbackForManipulationCompleted is set to True when the final feedback
// with the INTERACTION_FLAG_END flag must trigger a TouchInteractionCallback
// call in order to raise a ManipulationCompleted event.
void
ElementGestureTracker::Stop(
    _In_ bool bCallbackForManipulationCompleted)
{
#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: Stop\r\n", this);
#endif // POINTER_TRACING

    const auto interactionStopped = m_gestureOutputMapper->IsInteractionStopped();

    SetInteractionStopping(bCallbackForManipulationCompleted && !interactionStopped);
    SetInteractionStopped(!bCallbackForManipulationCompleted || interactionStopped);

    m_cPointerInteractionContexts = 0;
    SetManipulationStarting(false);

    IGNOREHR(m_gestureRecognizerAdapter.m_gestureRecognizer->CompleteGesture());
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::IsStopped
//
//  Synopsis:   TRUE if interaction context manager is stopped
//
//-------------------------------------------------------------------------
bool
ElementGestureTracker::IsStopped()
{
    return m_gestureOutputMapper->IsInteractionStopped();
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::IsInertial
//
//  Synopsis:   TRUE if interaction context manager is processing inertia
//
//-------------------------------------------------------------------------
bool
ElementGestureTracker::IsInertial()
{
    return m_gestureOutputMapper->IsInertial();
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::ProcessInertiaInteraction
//
//  Synopsis:   Notify the processing inertia to the interaction context
//              manager.
//
//-------------------------------------------------------------------------
void ElementGestureTracker::ProcessInertiaInteraction()
{
    if (IsInertial())
    {
        VERIFYHR(m_gestureRecognizerAdapter.m_gestureRecognizer->ProcessInertia());
    }
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::SetManipulationStarting
//
//  Synopsis:   Set when interaction context manager is starting for manipulation
//
//-------------------------------------------------------------------------
void
ElementGestureTracker::SetManipulationStarting(bool bManipulationStarting)
{
#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: SetManipulationStarting: new=%d\r\n", this, bManipulationStarting);
#endif // POINTER_TRACING

    m_gestureOutputMapper->SetManipulationStarting(bManipulationStarting);
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::IsManipulationStarting
//
//  Synopsis:   TRUE if interaction context manager is starting for manipulation
//
//-------------------------------------------------------------------------
bool
ElementGestureTracker::IsManipulationStarting()
{
    return m_gestureOutputMapper->IsManipulationStarting();
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::IsManipulationStarted
//
//  Synopsis:   TRUE if interaction context manager started a manipulation
//
//-------------------------------------------------------------------------
bool
ElementGestureTracker::IsManipulationStarted()
{
    return m_gestureOutputMapper->IsManipulationStarted();
}

// Private Implementation

_Check_return_ HRESULT
ElementGestureTracker::UpdateSettings(
    _In_ bool bTapEnabled,
    _In_ bool bDoubleTapEnabled,
    _In_ bool bRightTapEnabled,
    _In_ bool bHoldEnabled,
    _In_ DirectUI::ManipulationModes manipulationMode)
{
    // manipulationMode must be the custom manipulation mode.
    ASSERT(!(static_cast<unsigned int>(manipulationMode) & SystemManipulationModeMask));

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: UpdateSettings: bTapEnabled=%d, bDoubleTapEnabled=%d, bRightTapEnabled=%d, bHoldEnabled=%d, manipulationMode=0x%x\r\n",
        this, bTapEnabled, bDoubleTapEnabled, bRightTapEnabled, bHoldEnabled, manipulationMode);
#endif // POINTER_TRACING

    // Clear m_bIsInteractionStopped flag by calling UpdateSettings.
    SetInteractionStopping(false);
    SetInteractionStopped(false);

    // Manipulation settings
    if (m_bForceUpdateSettings ||
        manipulationMode != m_manipulationMode ||
        bTapEnabled != m_bTapEnabled ||
        bDoubleTapEnabled != m_bDoubleTapEnabled ||
        bRightTapEnabled != m_bRightTapEnabled ||
        bHoldEnabled != m_bHoldEnabled)
    {
        if (m_bForceUpdateSettings)
        {
            m_bForceUpdateSettings = false;
        }

        m_gestureRecognizerAdapter.ConfigurationBuilder(bTapEnabled, bDoubleTapEnabled, bRightTapEnabled, bHoldEnabled, manipulationMode);

        m_bTapEnabled = bTapEnabled;
        m_bDoubleTapEnabled = bDoubleTapEnabled;
        m_bRightTapEnabled = bRightTapEnabled;
        m_bHoldEnabled = bHoldEnabled;

        m_manipulationMode = manipulationMode;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ElementGestureTracker::InitializeInteractionEngine
//
//  Synopsis:   Initialize the interaction context manager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
ElementGestureTracker::InitializeInteractionEngine()
{
#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: InitializeInteractionEngine\r\n", this);
#endif // POINTER_TRACING

    m_bForceUpdateSettings = TRUE;
    m_gestureRecognizerAdapter.EnsureInitialized(this);
    m_gestureOutputMapper = &m_gestureRecognizerAdapter.m_gestureOutputMapper;

    // Update the interaction context manager configuration
    IFC_RETURN(UpdateSettings(
        m_bTapEnabled,
        m_bDoubleTapEnabled,
        m_bRightTapEnabled,
        m_bHoldEnabled,
        m_manipulationMode));

    //
    // INTERACTION_CONTEXT_PROPERTY_FILTER_POINTERS is On as the default.
    // We should call Add/RemovePointerInteractionContext() properly for the contact pointer.
    //

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   ProcessPointerInformation
//
//  Synopsis:   Process pointer frame information with ICM
//  WARNING:    May delete "this" pointer
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
ElementGestureTracker::ProcessPointerInformation(_In_ const InputMessage& msg)
{
    const UINT32 pointerId = msg.m_pointerInfo.m_pointerId;
    POINTER_INFO *pPointerInfo = nullptr;

#ifdef POINTER_TRACING
    gps->DebugOutputSzNoEndl(L"[InputPointer:ICM(0x%x)]: ProcessPointerInformation: MsgId=0x%x Id=%d pointerId=%d currentFrameId=%d previousFrameId=%d\r\n",
        this,
        msg.m_msgID,
        pointerId,
        msg.m_pointerInfo.m_frameId,
        m_frameId);
#endif // POINTER_TRACING

    // The GestureRecognizer will return error if it detects re-entrancy.  For now, we'll just let it return error,
    // but in the future we may do work to prevent this.
    CReentrancyGuard reentrancyGuard(&m_isProcessingInput);

    auto traceGuard = wil::scope_exit([&] {
        if (m_bDelayDestroyInteractionEngine)
        {
            // Destroy the interaction engine after process the message.
            // This function may delete the "this" pointer.
            IGNOREHR(Destroy());
        }

        // Release memory allocated for pointer frame history
        if (pPointerInfo != nullptr)
        {
            delete[] pPointerInfo;
            pPointerInfo = nullptr;
        }
    });

    // Update the current pointer input type
    SetPointerInputType(msg.m_pointerInfo.m_pointerInputType);

    if (msg.m_msgID == XCP_POINTERDOWN)
    {
        wf::Point pt;
        IFC_RETURN(msg.m_pPointerPointNoRef->get_Position(&pt));

        CUIElement* element = m_pElement.lock_noref();

        if (element != nullptr && element->IsPointerPositionOver({ pt.X, pt.Y }))
        {
            // SYNC_CALL_TO_APP
            IFC_RETURN(m_gestureRecognizerAdapter.m_gestureRecognizer->ProcessDownEvent(msg.m_pPointerPointNoRef));
        }
    }
    else if (msg.m_msgID == XCP_POINTERUP)
    {
        // SYNC_CALL_TO_APP
        IFC_RETURN(m_gestureRecognizerAdapter.m_gestureRecognizer->ProcessUpEvent(msg.m_pPointerPointNoRef));
    }
    else if (msg.m_msgID == XCP_POINTERUPDATE)
    {
        wrl::ComPtr<wfc::IVector<ixp::PointerPoint*>> pointerPoints;

        IFC_RETURN(msg.m_pPointerEventArgsNoRef->GetIntermediatePoints(&pointerPoints));

        // SYNC_CALL_TO_APP
        // Allow the out-of-order error for now.  Tracking this with http://osgvsowi/15831453
        IFC_RETURN_ALLOW(m_gestureRecognizerAdapter.m_gestureRecognizer->ProcessMoveEvents(pointerPoints.Get()), INPUT_E_OUT_OF_ORDER);
    }
 
    return S_OK;
}

void ElementGestureTracker::SetManipulationStarted(bool value)
{
    m_gestureOutputMapper->SetManipulationStarted(value);
}

void ElementGestureTracker::SetInteractionStopping(bool value)
{
    m_gestureOutputMapper->SetInteractionStopping(value);
}

void ElementGestureTracker::SetInteractionStopped(bool value)
{
    m_gestureOutputMapper->SetInteractionStopped(value);
}

void ElementGestureTracker::SetPointerInputType(XPointerInputType type)
{
    m_gestureOutputMapper->SetPointerInputType(type);
}


