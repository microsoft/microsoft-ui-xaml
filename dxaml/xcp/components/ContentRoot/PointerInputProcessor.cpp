// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputManager.h"
#include "PointerInputProcessor.h"

#include "corep.h"
#include "inputservices.h"

#include "RightTappedEventArgs.h"

#include "MUX-ETWEvents.h"

#include "FxCallbacks.h"

#include "PointerAnimationUsingKeyFrames.h"
#include "RuntimeEnabledFeatures.h"

#include "RootVisual.h"
#include "InputPaneHandler.h"

#include "Pointer.h"

#include "ScrollViewer.h"
#include "DMViewport.h"

#include "eventmgr.h"
#include "request.h"

#include "host.h"
#include "timer.h"

#include "XamlOneCoreTransforms.h"

#include "FocusSelection.h"

#include "DOPointerCast.h"

#include "JupiterWindow.h"
#include <DXamlServices.h>
#include "Button.h"

#include "PointerInputTelemetry.h"

using namespace ContentRootInput;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get DirectManipulation debug outputs, and 0 otherwise
#define DMIM_DBG 0

static const XUINT32 sc_Mouse_Pointer_Id = 1;

PointerInputProcessor::PointerInputProcessor(_In_ CInputManager& inputManager)
    : m_inputManager(inputManager),
      m_iPrimaryPointerId(-1)
{
}

bool PointerInputProcessor::HasPrimaryPointerLastPositionOverride() const
{
    return m_hasPrimaryPointerLastPositionOverride;
}

bool PointerInputProcessor::IsBarrelButtonPressed() const
{
    return m_fBarrelButtonPressed;
}

void PointerInputProcessor::SetBarrelButtonPressed(_In_ bool value)
{
    m_fBarrelButtonPressed = value;
}

void PointerInputProcessor::SetPrimaryPointerLastPositionOverride(XPOINTF value)
{
    m_hasPrimaryPointerLastPositionOverride = true;
    m_primaryPointerLastPositionOverride = value;
}

void PointerInputProcessor::ClearPrimaryPointerLastPositionOverride()
{
    m_hasPrimaryPointerLastPositionOverride = false;
}

_Check_return_ HRESULT PointerInputProcessor::TryGetPrimaryPointerLastPosition(_Out_ XPOINTF *pLastPosition, _Out_ bool *pSucceeded)
{
    if (m_hasPrimaryPointerLastPositionOverride)
    {
        *pLastPosition = m_primaryPointerLastPositionOverride;
        *pSucceeded = true;
    }
    else
    {
        std::shared_ptr<CPointerState> pointerState;
        *pSucceeded = false;

        pLastPosition->x = 0;
        pLastPosition->y = 0;

        if (m_iPrimaryPointerId >= 0)
        {
            auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

            ASSERT(mapPointerState.ContainsKey(m_iPrimaryPointerId));
            IFC_RETURN(mapPointerState.Get(m_iPrimaryPointerId, pointerState));

            *pLastPosition = pointerState->GetLastPosition();
            *pSucceeded = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::RaiseRightTappedEventFromContextMenu(_Out_ bool* pHandled)
{
    XPOINTF pointFocusedElementCenter;
    const auto contentRoot = m_inputManager.GetContentRoot();

    *pHandled = false;

    // Set the event source from the current focused element.
    CDependencyObject* pSource = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
    if (pSource == nullptr)
    {
        pSource = static_cast<CDependencyObject*>(contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual());
    }

    xref_ptr<CRightTappedEventArgs> rightTappedEventArgs;
    rightTappedEventArgs.init(new CRightTappedEventArgs(&m_inputManager.m_coreServices));

    // Set the original source element
    IFC_RETURN(rightTappedEventArgs->put_Source(pSource));

    CUIElement* pSourceAsUIE = do_pointer_cast<CUIElement>(pSource);
    if (pSourceAsUIE)
    {
        // Set the center position of the current focused element
        pointFocusedElementCenter.x = pSourceAsUIE->GetActualWidth() / 2;
        pointFocusedElementCenter.y = pSourceAsUIE->GetActualHeight() / 2;

        IFC_RETURN(CInputServices::ConvertTransformPointToGlobal(pSourceAsUIE, &pointFocusedElementCenter));

        static_cast<CInputPointEventArgs*>(rightTappedEventArgs)->SetGlobalPoint(pointFocusedElementCenter);
    }
    else
    {
        // Set the default position
        static_cast<CInputPointEventArgs*>(rightTappedEventArgs)->SetGlobalPoint(XPOINTF{});
    }

    // Set the device type as mouse
    rightTappedEventArgs->m_pointerDeviceType = DirectUI::PointerDeviceType::Mouse;

    // Raise RightTapped event
    m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_RightTapped),
        static_cast<CDependencyObject *>(pSource),
        rightTappedEventArgs,
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    // WM_CONTEXTMENU message won't pass to CoreWindow if RightTapped event is handled.
    *pHandled = rightTappedEventArgs->m_bHandled == TRUE;

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::ProcessPointerInput(
    _In_ InputMessage *pMsg,
    _Out_ XINT32 *handled)
{
    HRESULT hr = S_OK;
    xref_ptr<CDependencyObject> spDOContact;
    xref_ptr<CPointerEventArgs> pPointerArgs;
    XPOINTF xpContact;
    CUIElement* pContactElement = nullptr;
    XUINT32 pointerId = 0;
    std::shared_ptr<CPointerState> pointerState;
    xref_ptr<CDependencyObject> pPointerEnterDO;
    xref_ptr<CDependencyObject> pPointerCaptureDO;

    bool raiseCaptureLostOnCancel = false;

    TraceProcessPointerInputBegin(pMsg->m_pointerInfo.m_pointerId, static_cast<XUINT32>(pMsg->m_msgID), pMsg->m_pointerInfo.m_pointerLocation.x, pMsg->m_pointerInfo.m_pointerLocation.y);

    // Here's a possible reentrancy sceanrio:
    // 1 Xaml (fires sync event) -> 2 App -> 3 MessagePump -> 4 Xaml 
    // When we're at (4), we set previousMessageBeingProcessed to be the InputMessage from (1).  This allows us
    // to flag that previous InputMesage, so we remember that we've already processed later messages.
    InputMessage* previousMessageBeingProcessed = m_messageBeingProcessed;
    if (previousMessageBeingProcessed)
    {
        previousMessageBeingProcessed->m_supersededByLaterMessage = true;
        PointerInputTelemetry::PointerInputReentrancyDetected(previousMessageBeingProcessed->m_msgID, pMsg->m_msgID);
        const bool reentrancyChecksEnabled = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->
            IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableReentrancyChecks);
        if (reentrancyChecksEnabled)
        {
            XCP_FAULT_ON_FAILURE(0);
        }
    }
    m_messageBeingProcessed = pMsg;

    const auto contentRoot = m_inputManager.GetContentRoot();

    IFC(m_inputManager.m_coreServices.SetPointerInputEvent());

    pointerId = pMsg->m_pointerInfo.m_pointerId;
    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();
    if (mapPointerState.ContainsKey(pointerId))
    {
        IFC(mapPointerState.Get(pointerId, pointerState));
    }

    // Set the last input device type.  If this is a replay message, don't set the last input
    // device type since that might tromp on the value for keyboard or gamepad.
    if (!pMsg->IsReplayedMessage())
    {
        // It is possible (and probable) that the system will generate intermittent pointer update
        // messages that don't move the cursor.  The reason for this is probably steeped in the
        // history of windows before frameworks, but it can cause the appearance of user input 
        // when ther wasn't any.  This affects some of our processing when we want to know if the
        // user is using UIA or not.  So, if the pointer is of the same type and hasn't move, then
        // we will assume tht it is a geneated input and tell SetLastInputDeviceType that it should
        // be ignored pertaining to UIA.
        bool keepUIAFocusState = pointerState && pMsg->m_msgID == XCP_POINTERUPDATE &&
                                 pointerState->GetPointerInputType() == pMsg->m_pointerInfo.m_pointerInputType &&
                                 pointerState->GetLastPosition().x == pMsg->m_pointerInfo.m_pointerLocation.x &&
                                 pointerState->GetLastPosition().y == pMsg->m_pointerInfo.m_pointerLocation.y;

        if (pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypeMouse)
        {
            m_inputManager.SetLastInputDeviceType(DirectUI::InputDeviceType::Mouse, keepUIAFocusState);
        }
        else if (pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypeTouch)
        {
            m_inputManager.SetLastInputDeviceType(DirectUI::InputDeviceType::Touch, keepUIAFocusState);
        }
        else if (pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypePen)
        {
            m_inputManager.SetLastInputDeviceType(DirectUI::InputDeviceType::Pen, keepUIAFocusState);
        }
        else
        {
            ASSERT(FALSE);
        }

        m_inputManager.m_coreServices.GetInputServices()->SetLastInputDeviceType(pMsg->m_pointerInfo.m_pointerInputType);
    }

    m_fBarrelButtonPressed = pMsg->m_pointerInfo.m_bBarrelButtonPressed;

    // New pointer, so create a new state data for it.
    if (!pointerState)
    {
        pointerState = std::make_shared<CPointerState>(pMsg->m_hWindow, pointerId, pMsg->m_pointerInfo.m_pointerInputType);
        IFC(mapPointerState.Add(pointerId, pointerState));

        if (pMsg->m_msgID == XCP_POINTERUPDATE && (pMsg->m_pointerInfo).m_bInContact && !pointerState->IsPointerDown())
        {
            // Allow capturing the pointer after a DManip was cancelled.
            pointerState->SetPointerDown(true);
        }
    }

    pPointerEnterDO = pointerState->GetEnterDO();
    // Get the mouse entered DO if current message is PointerEnter and current entered DO
    // is nullptr. If current entered DO isn't nullptr, it is the middle of processing pointer messages.
    if (pPointerEnterDO == nullptr && pMsg->m_msgID == XCP_POINTERENTER)
    {
        std::shared_ptr<CPointerState> mousePointerState;
        IFC(mapPointerState.Get(sc_Mouse_Pointer_Id, mousePointerState));
        if (mousePointerState)
        {
            pPointerEnterDO = mousePointerState->GetEnterDO();
        }
    }

    pPointerCaptureDO = pointerState->GetCaptureDO();

    // create a point representing the hittest area
    xpContact.x = pMsg->m_pointerInfo.m_pointerLocation.x;
    xpContact.y = pMsg->m_pointerInfo.m_pointerLocation.y;
    pointerState->SetLastPosition(xpContact);

    {
        CDependencyObject* hitTestRoot = contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef();

        IFC(HitTestWithLightDismissAwareness(
            spDOContact,
            xpContact,
            pMsg->m_msgID,
            &pMsg->m_pointerInfo,
            hitTestRoot));
    }

    // Set the current contact with the public root if the current contact is non-hittestable element.
    // We need to support Pointer/Gesture events on the visual root. For example, ApplicationBar's toggling.
    if (!spDOContact)
    {
        // If we don't have a contact AND we are a pointer wheel changed then it is because the the user has
        // disabled the option to scroll inactive windows in favor of scrolling the active one.  For now, we
        // will therefore re-route this message to our focused elements.  Future enhancements may make this
        // smarter (like routing it to the last control with a pointer pressed or released).
        if (pMsg->m_msgID == XCP_POINTERWHEELCHANGED)
        {
            spDOContact = static_cast<CDependencyObject*>(contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef());
        }
        else
        {
            spDOContact = contentRoot->GetVisualTreeNoRef()->GetPublicRootVisual();
        }
    }

    if (!spDOContact)
    {
        goto Cleanup;
    }

    if ((pMsg->m_pPointerPointNoRef == nullptr)
        && (pMsg->m_msgID != XCP_POINTERLEAVE)
        && VisualTree::GetXamlIslandRootForElement(spDOContact.get())
        && !VisualTree::IsElementInWindowedPopup(do_pointer_cast<CUIElement>(spDOContact.get())))
    {
        // Bail out here if this message is win32-based but we hit-tested something in an island -- in islands,
        // we use winRT/PointerPoint-based input.  This situation can happen when a user clicks on a tooltip,
        // which is a windowed popup, but not hit test visible.  In this case we get win32-stype input from
        // the popup's HWND, now we've hit-tested through it and hit something in a XamlIslandRoot.  If we proceed
        // like this, the ElementGestureTracker won't work because we've configured it to use GestureRecognizer,
        // which only works for WinRT-based input.
        // Let's clean this up for good: http://osgvsowi/18004791 Converge windowed Popups
        goto Cleanup;
    }

    // Set the source with the contact element
    if (pMsg->m_msgID == XCP_POINTERDOWN)
    {
        // Always update the m_iPrimaryPointerId to be the most-recent pointer down.
        // NOTE: Even when we catch another pointer during a PointerDownThemeAnimation,
        // this will not cause undesired behavior (ie. changing the pivot to the new, secondary pointer).
        // Since PointerAnimationUsingKeyFrames caches the pointer in ComputeLocalProgressAndTime(),
        // it will not update its pivot point mid-animation.
        m_iPrimaryPointerId = static_cast<XINT32>(pointerId);
        ASSERT(m_iPrimaryPointerId != -1);

        // Removing the previous pointerId's interaction element if it still exists.
        // WinSimulator doesn't generate PointerEnter/Leave with touch simulation,
        // so the previous interaction element exists.
        IFC(m_inputManager.m_coreServices.GetInputServices()->RemovePointerIdFromInteractionElement(pointerId));
        IFC(m_inputManager.m_coreServices.GetInputServices()->AddPointerIdToInteractionElement(pointerId, do_pointer_cast<CUIElement>(spDOContact)));
        IFC(m_inputManager.m_coreServices.GetInputServices()->RemoveEntryFromPointerDownTrackerMap(pointerId));

        if (IsInputTypeTreatedLikeTouch(pMsg->m_pointerInfo.m_pointerInputType))
        {
            IFC(m_inputManager.m_coreServices.GetInputServices()->AddEntryToPointerDownTrackerMap(pointerId, do_pointer_cast<CUIElement>(spDOContact)));
        }

        if (IXcpInputPaneHandler* inputPaneHandler = m_inputManager.GetInputPaneHandler())
        {
            static_cast<CInputPaneHandler*>(inputPaneHandler)->SetPointerPosition(xpContact);
        }
    }

    // Create PointerEventArgs for PointerDown/Move/Up events
    pPointerArgs.attach(new CPointerEventArgs(&m_inputManager.m_coreServices));

    pPointerArgs->SetGlobalPoint(xpContact);
    pPointerArgs->m_pPointerPoint = pMsg->m_pPointerPointNoRef;
    pPointerArgs->m_pPointerEventArgs = pMsg->m_pPointerEventArgsNoRef;
    pPointerArgs->m_isGenerated = pMsg->IsReplayedMessage();

    // Set the original source element
    IFC(pPointerArgs->put_Source(spDOContact));

    // Set the Pointer object
    IFC(SetPointerFromPointerMessage(pMsg, pPointerArgs));

    // Set the key modifiers
    IFC(SetPointerKeyModifiers(pMsg->m_modifierKeys, pPointerArgs));

    // It is possible that prior to this input, there may have been an entered element that was deleted from
    // the tree.  If so, it would have had an entery placed in the pointer exited state map.  We want to process
    // this list now, before we raise any events, because it is possible that those events might also modify
    // the tree puting the element back in.  If we only processed this list at the end, the pending exit event might
    // occur after the enter event.
    VERIFYHR(ProcessPointerExitedState(pointerId, pPointerArgs));
    // Don't let handling of exited events affect any other events we might generate.
    pPointerArgs->m_bHandled = FALSE;

    m_fSawMouseLeave = (pMsg->m_pointerInfo.m_pointerInputType == XcpPointerInputTypeMouse == 1 && pMsg->m_msgID == XCP_POINTERLEAVE);

    // PointerCanceled event will be fired only the first time of having pMsg->m_pointerInfo.m_bCanceled flag.
    // PointerXXX(Moved or Released) event that set Canceled flag will be also fired after firing PointerCanceled event.
    if (pMsg->m_pointerInfo.m_bCanceled && !(pointerState->IsPointerCanceled()))
    {
        // Set PointerCanceled and firing PointerCanceled event immediately
        pointerState->SetPointerCanceled(TRUE);
        raiseCaptureLostOnCancel = pMsg->m_msgID != XCP_POINTERCAPTURECHANGED && pMsg->m_msgID != XCP_POINTERSUSPENDED;

#ifdef DM_DEBUG
        if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessPointerInput - Raising PointerCanceled for 0x%p.",
                this, pPointerCaptureDO ? pPointerCaptureDO : static_cast<CDependencyObject *>(spDOContact)));
        }
#endif // DM_DEBUG

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerCanceled),
            pPointerCaptureDO ? pPointerCaptureDO : static_cast<CDependencyObject *>(spDOContact),
            pPointerArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
    }

    if (raiseCaptureLostOnCancel || pMsg->m_msgID == XCP_POINTERCAPTURECHANGED || pMsg->m_msgID == XCP_POINTERSUSPENDED)
    {
    #ifdef DM_DEBUG
        if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled() && !raiseCaptureLostOnCancel)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessPointerInput - Handling %s.", this, pMsg->m_msgID == XCP_POINTERCAPTURECHANGED ? L"XCP_POINTERCAPTURECHANGED" : L"XCP_POINTERSUSPENDED"));
        }
    #endif // DM_DEBUG

        xref_ptr<CDispatcherTimer>& contextMenuTimer = m_inputManager.GetContextMenuProcessor().GetContextMenuTimer();

        // User moved the pointer, so context menu timer should be stopped if it is running
        if (contextMenuTimer && contextMenuTimer->IsInActiveState())
        {
            IFC(contextMenuTimer->Stop());
        }

        IFC(ReleasePointerCaptureById(pointerId));

        // Fire PointerCaptureLost if there is no captured before.
        if (pPointerCaptureDO == NULL)
        {
    #ifdef DM_DEBUG
            if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled())
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessPointerInput - Raising PointerCaptureLost for 0x%p and pointerId=%d.",
                    this, spDOContact.get(), pointerId));
            }
    #endif // DM_DEBUG

            // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
            // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
            // objects are alive and state is re-validated after return.
            // Temporarily disable capturing the pointer while raising the PointerCaptureLost event.
            pointerState->SetPointerCaptureDenied(true);

            {
                m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
                    EventHandle(KnownEventIndex::UIElement_PointerCaptureLost),
                    pPointerEnterDO ? pPointerEnterDO : static_cast<CDependencyObject *>(spDOContact),
                    pPointerArgs,
                    TRUE /* bIgnoreVisibility */,
                    TRUE /* fRaiseSync */,
                    TRUE /* fInputEvent */);
            }

            pointerState->SetPointerCaptureDenied(false);

            if (IsInputTypeTreatedLikeTouch(pMsg->m_pointerInfo.m_pointerInputType) && m_inputManager.GetIsContextMenuOnHolding())
            {
                // For context menu, touch press & hold followed by drag should dismiss flyout
                // The touch drag could be into the smoke layer, or the flyout layer
                // If the drag was into the flyout layer, we want to walk up the tree to get to the PopupRoot
                auto curr = spDOContact;
                while (curr != nullptr)
                {
                    if (curr->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
                    {
                        spDOContact = curr;
                        break;
                    }
                    curr = curr->GetParent();
                }

                if (auto popupRoot = do_pointer_cast<CPopupRoot>(spDOContact))
                {
                    // For touch input, if the contact element is a PopupRoot & context flyout was shown, the user performed a drag/pan after we showed a flyout
                    // Dismiss the flyout in this case, and do a hit test to the element under the pointer
                    IFC(popupRoot->CloseTopmostPopup(DirectUI::FocusState::Pointer, CPopupRoot::PopupFilter::LightDismissOnly));
                    IFC(HitTestHelper(xpContact, NULL, spDOContact.ReleaseAndGetAddressOf()));
                }
                else
                {
                    // If there is no flyout, app code may have done some processing during ContextRequested
                    // So, we fire ContextCanceled so that app code can undo whatever processing was done during ContextRequested
                    xref_ptr<CRoutedEventArgs> contextCancelArgs;
                    IFC(contextCancelArgs.init(new CRoutedEventArgs()));
                    IFC(contextCancelArgs->put_Source(spDOContact.get()));
                    m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
                        EventHandle(KnownEventIndex::UIElement_ContextCanceled),
                        spDOContact.get(),
                        contextCancelArgs.get(),
                        TRUE /* bIgnoreVisibility */);
                }

                m_inputManager.SetIsContextMenuOnHolding(false);
            }
        }
        else
        {
            m_inputManager.SetIsContextMenuOnHolding(false);
        }
    }
    switch (pMsg->m_msgID)
    {
    case XCP_POINTERENTER:
        IFC(ProcessPointerEnterLeave(spDOContact, pPointerEnterDO, pointerId, pPointerArgs, FALSE /* bSkipLeave */, TRUE /* bForceRaisePointerEntered*/));
        break;
    case XCP_POINTERDOWN:
    {
        IFC(ProcessPointerEnterLeave(
            spDOContact,
            pPointerEnterDO,
            pointerId,
            pPointerArgs,
            FALSE /* bSkipLeave */,
            FALSE /* bForceRaisePointerEntered*/));

        // Allow Mnemonics mode to exit on the pointer down
        IFC(contentRoot->GetAKExport().ProcessPointerInput(pMsg));

        pointerState->SetPointerDown(TRUE/*bPointerDown*/);
#ifdef DM_DEBUG
        if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessPointerInput - Raising PointerPressed for 0x%p.", this, spDOContact.get()));
        }
#endif // DM_DEBUG

        xref_ptr<CDispatcherTimer>& contextMenuTimer = m_inputManager.GetContextMenuProcessor().GetContextMenuTimer();

        // If timer is running on pointer down, it means a second touch has happened. So stop the timer to not show a context menu in this case.
        if (contextMenuTimer && contextMenuTimer->IsInActiveState())
        {
            IFC(contextMenuTimer->Stop());
        }

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerPressed),
            static_cast<CDependencyObject *>(spDOContact),
            pPointerArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        break;
    }
    case XCP_POINTERUPDATE:
        if (pPointerCaptureDO)
        {
            IFC(ProcessPointerCaptureEnterLeave(spDOContact, pPointerEnterDO, pPointerCaptureDO, pointerId, pPointerArgs));
        }
        else
        {
            bool enterLeaveFound = false;
            IFC(ProcessPointerEnterLeave(
                spDOContact,
                pPointerEnterDO,
                pointerId,
                pPointerArgs,
                FALSE /* bSkipLeave */,
                FALSE /* bForceRaisePointerEntered*/,
                FALSE /* bIgnoreHitTestVisibleForPointerExited */,
                FALSE /* bAsyncEvent */,
                FALSE /* bAddEventRequest */,
                &enterLeaveFound));
            // If this is a replay and we didn't actually change our entered elements, then don't raise the update event.
            if (!enterLeaveFound && pMsg->IsReplayedMessage())
            {
                break;
            }
        }
        pPointerArgs->m_bHandled = FALSE;

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.
        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerMoved),
            pPointerCaptureDO ? pPointerCaptureDO : static_cast<CDependencyObject *>(spDOContact),
            pPointerArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        break;
    case XCP_POINTERUP:
    {
        // Allow Mnemonics mode to exit on the pointer up
        IFC(contentRoot->GetAKExport().ProcessPointerInput(pMsg));

        pointerState->SetPointerDown(FALSE/*bPointerDown*/);
        // Save the QPC for this input, to be used for telemetry/tracing.

        if (!m_inputManager.m_coreServices.GetInputServices()->GetFirstPointerUpQPCSinceLastFrame())
        {
            m_inputManager.m_coreServices.GetInputServices()->SetFirstPointerUpSinceLastFrame(pMsg->m_pointerInfo.m_qpcInput);
        }
#ifdef DM_DEBUG
        if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ProcessPointerInput - Raising PointerReleased for 0x%p.",
                this, pPointerCaptureDO ? pPointerCaptureDO : spDOContact.get()));
        }
#endif // DM_DEBUG

        xref_ptr<CDispatcherTimer>& contextMenuTimer = m_inputManager.GetContextMenuProcessor().GetContextMenuTimer();

        // If touch is released, context menu timer should be stopped if it is running.
        if (contextMenuTimer && contextMenuTimer->IsInActiveState())
        {
            IFC(contextMenuTimer->Stop());
        }

        m_pPendingPointerEventArgs = pPointerArgs;
        break;
    }
    case XCP_POINTERLEAVE:
    {
        if (pPointerCaptureDO)
        {
            IFC(ProcessPointerLeave(pPointerCaptureDO, pointerId, pPointerArgs));
        }
        else
        {
            // Pointer Exit should be fired on the PointerEntered place or current contact.
            IFC(ProcessPointerLeave(pPointerEnterDO ? pPointerEnterDO : spDOContact, pointerId, pPointerArgs));
        }

        if (mapPointerState.ContainsKey(pointerId) && pPointerCaptureDO)
        {
            IFC(ReleasePointerCapture(pPointerCaptureDO, pPointerArgs->m_pPointer));
        }
        break;
    }
    case XCP_POINTERWHEELCHANGED:
    {
        // Allow Mnemonics mode to exit on pointer wheel scroll
        IFC(contentRoot->GetAKExport().ProcessPointerInput(pMsg));

        // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
        // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
        // objects are alive and state is re-validated after return.

        // Used by the ProcessInputMessageWithDirectManipulation method potentially invoked by the DM container,
        // synchronously during the RaiseRoutedEvent call.
        m_pCurrentMsgForDirectManipulationProcessing = pMsg;

        auto guard = wil::scope_exit([&] {
            m_pCurrentMsgForDirectManipulationProcessing = nullptr;
        });

        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerWheelChanged),
            pPointerCaptureDO ? pPointerCaptureDO : static_cast<CDependencyObject *>(spDOContact),
            pPointerArgs,
            FALSE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
        break;
    }
    case XCP_POINTERCAPTURECHANGED:
    case XCP_POINTERSUSPENDED:
        break; // These were handled prior to the switch statement.
    default:
    {
        *handled = false;
        IFC(E_UNEXPECTED);
        break;
    }
    } // switch(pMsg->m_msgID)

    if (m_inputManager.m_coreServices.GetInputServices()->GetMapInteraction().ContainsKey(pointerId) && !pMsg->IsReplayedMessage())
    {
        // SYNC_CALL_TO_APP: This can call out to app code, it's possible that the app pumps messages during this call, which could
        // call CleanPointerProcessingState on pointerState, making it no longer valid.
        // Process the pointer messages for the specified interaction element that will feed the pointer input into the
        // interaction context manager for gesture and manipulation recognition.
        // Replayed messages are only meant to update hover state of Xaml elements.
        IFC(m_inputManager.m_coreServices.GetInputServices()->ProcessInteractionPointerMessages(pointerId, pMsg));
    }

    if (pMsg->m_msgID == XCP_POINTERENTER)
    {
        // Register InputPane handler to response the input pane state(show/hide) changes
        if (spDOContact && (!m_inputManager.GetInputPaneHandler() || !m_inputManager.GetInputPaneInteraction()))
        {
            TraceRegisterInputPaneHandlerBegin();
            m_inputManager.CreateInputPaneHandler();
            // Note: We don't check for the open pane because if it had been opened while the application
            // was running, we would have previously registered the handler when we were told the pane was
            // opening.  It might have been open since before the application started, but checking is a
            // very expensive operation (in terms of cocreates/memory) and the scenario is rare.
            IFC(m_inputManager.RegisterInputPaneHandler(pMsg->m_hCoreWindow));
            TraceRegisterInputPaneHandlerEnd();
        }
    }

    if (IsInputTypeTreatedLikeTouch(pMsg->m_pointerInfo.m_pointerInputType))
    {
        pContactElement = do_pointer_cast<CUIElement>(spDOContact);
        if (pContactElement)
        {
            if (pMsg->m_msgID == XCP_POINTERDOWN)
            {
                IFC(m_inputManager.m_coreServices.GetInputServices()->RegisterDMViewportsOnPointerDown(pointerId, FALSE /*fIsDMHitTest*/, pContactElement));
            }
            else if (pMsg->m_msgID == XCP_POINTERUP && !m_pPendingPointerEventArgs)
            {
                IFC(m_inputManager.m_coreServices.GetInputServices()->UnRegisterDMViewportsOnPointerUp(pointerId));
            }
            // Else CleanPointerProcessingState() will call
            //   - UnregisterContactId and UnregisterCrossSlideViewportContactId when a XCP_POINTERUP message is received,
            //   - UnregisterCrossSlideViewportContactId when a XCP_POINTERSUSPENDED message is received.
        }
    }

    CXamlIslandRoot* xamlIslandRoot = contentRoot->GetXamlIslandRootNoRef();
    const bool isPointerDownOnFocusableIsland = pMsg->m_msgID == XCP_POINTERDOWN && xamlIslandRoot;
    if (isPointerDownOnFocusableIsland)
    {
        // When running in DesktopWindowXamlSource, we don't get any notification that we've been "activated", since we're
        // not running in a top-level window.  (When we're running in a CoreWindow or AppWindow, we get an activated
        // notification and we understand that we should be active for TextBox input, for example).
        // Most of the time, when a user clicks on a DesktopWindowXamlSource, we'll go through CFocusManager::UpdateFocus
        // and set focus to our island when that focus change succeeds.  However, some elements (like TextBox) skip setting
        // focus on themselves when they notice they already have focus (see CTextBoxBase::OnPointerPressed).  So here,
        // we handle this case.  If we see that the hit-tested element is within the focused element and that the FocusManager
        // tells us the island is still not focused, go ahead and take focus.  Note that when the user clicks on non-focusable
        // elements, like a TextBlock, we don't want to steal focus in this case.

        const bool islandHasFocus = contentRoot->GetFocusManagerNoRef()->IsPluginFocused() && contentRoot->GetVisualTreeNoRef()->GetActiveRootVisual() == xamlIslandRoot->GetPublicRootVisual();
        if (!islandHasFocus && Focus::FocusSelection::GetAllowFocusOnInteraction(spDOContact.get()))
        {
            CDependencyObject* focusedElement = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();
            const bool focusedElementContainsHitElement = focusedElement && (spDOContact.get() == focusedElement || focusedElement->IsAncestorOf(spDOContact));
            if (focusedElementContainsHitElement)
            {
                boolean success = false;
                VERIFYHR(xamlIslandRoot->TrySetFocus(&success));
                ASSERT(success, L"Moving focus to xaml island root failed");
            }
        }
    }

    // Clean up the pointer processing state
    IFC(m_inputManager.m_coreServices.GetInputServices()->CleanPointerProcessingState(contentRoot, pMsg, pointerId, pointerState, spDOContact));

    // Always set to Handled not to promote into Mouse messages
    *handled = TRUE;

Cleanup:
    // The right button is no longer flagged as pressed when handling a pointer-up message,
    // so we'll save off the pointer ID in the pointer-down message so we'll know that
    // a given pointer-up message corresponds to a right button.
    if (pMsg->m_msgID == XCP_POINTERDOWN && pMsg->m_pointerInfo.m_bRightButtonPressed)
    {
        m_iRightButtonPointerId = static_cast<XINT32>(pointerId);
    }
    else if (pMsg->m_msgID == XCP_POINTERUP && m_iRightButtonPointerId == pointerId)
    {
        m_iRightButtonPointerId = -1;
    }

    m_messageBeingProcessed = previousMessageBeingProcessed;

    if (pPointerArgs != nullptr)
    {
        IGNOREHR(ProcessPointerExitedState(pointerId, pPointerArgs));
    }

    if (FAILED(hr))
    {
        // Clean up the pointer processing state
        IGNOREHR(m_inputManager.m_coreServices.GetInputServices()->CleanPointerProcessingState(contentRoot, pMsg, pointerId, pointerState, spDOContact));
    }

    TraceProcessPointerInputEnd();

    return hr;
}

// NOTE: THIS CODE IS VERY SIMILAR TO THE CODE IN PROCESSENTERLEAVE BUT I DO NOT
// WANT TO CONSOLIDATE BECAUSE OTHERWISE THE DEBUGGING EXPERIENCE IS TRAUMATIC
// UNTIL WE HAVE ALL ISSUES IN THIS CODE CLEANED UP I WOULD LIKE TO KEEP THIS
_Check_return_ HRESULT PointerInputProcessor::ProcessPointerCaptureEnterLeave(
    _In_opt_ CDependencyObject *pContactElement,
    _In_opt_ CDependencyObject *pPointerEnterDO,
    _In_ CDependencyObject *pPointerCaptureDO,
    _In_ XUINT32 pointerId,
    _In_ CPointerEventArgs *pArgs)
{
    CDependencyObject* pVisual = nullptr;
    xref_ptr<CDependencyObject> pReleaseVisual;
    bool bInTree = false;
    const auto contentRoot = m_inputManager.GetContentRoot();

    // Source shouldn't be the hidden root since it doesn't have a managed peer.
    ASSERT(pArgs->m_pSource != contentRoot->GetVisualTreeNoRef()->GetRootVisual());

    // Pass 1: Set the leave and dirty bits on all elements in the parent chain
    pVisual = static_cast<CDependencyObject*>(pPointerEnterDO);
    while (pVisual)
    {
        m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, true);
        m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, false);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 2 set the enter on the elements until we encounter the first dirty bit
    pVisual = static_cast<CDependencyObject*>(pContactElement);
    while (pVisual)
    {
        m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, true);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 3 : now fire events for leave until we hit first element with enter set
    pVisual = static_cast<CDependencyObject*>(pPointerEnterDO);
    while (pVisual && !m_inputManager.m_coreServices.GetInputServices()->HasPointerEnter(pVisual, pointerId))
    {
        m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);
        if (pPointerCaptureDO == static_cast<CDependencyObject*>(pVisual))
        {
            bInTree = TRUE;
        }
        if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible() && (bInTree == TRUE))
        {
            // AddRef before, and Release after calling out to script so that no
            // tree changes will cause pVisual to be deleted.
            pReleaseVisual = pVisual;

            // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
            // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
            // objects are alive and state is re-validated after return.
            m_inputManager.m_coreServices.GetEventManager()->Raise(
                EventHandle(KnownEventIndex::UIElement_PointerExited),
                TRUE /* bReFire */,
                pVisual,
                pArgs,
                TRUE /* fRaiseSync */,
                TRUE /* fInputEvent */);
        }
        pVisual = pVisual->GetParentInternal();
    }

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

    // Set the new EnterDOHolder with the contact element
    if (mapPointerState.ContainsKey(pointerId))
    {
        std::shared_ptr<CPointerState> pointerState;
        IFC_RETURN(mapPointerState.Get(pointerId, pointerState));
        IFC_RETURN(pointerState->SetEnterDO(static_cast<CDependencyObject*>(pContactElement)));
    }

    bInTree = FALSE;

    pVisual = static_cast<CDependencyObject*>(pContactElement);

    // Pass 4: Now walk up the tree
    //      A) firing enter events until we hit the first guy that has dirty bit set
    //         and from there reset the dirty bit
    while (pVisual)
    {
        if (static_cast<CDependencyObject*>(pVisual) == pPointerCaptureDO)
        {
            bInTree = TRUE;
        }
        if (bInTree)
        {
            if (m_inputManager.m_coreServices.GetInputServices()->HasPointerEnter(pVisual, pointerId) &&
                !m_inputManager.m_coreServices.GetInputServices()->IsInputPointerNodeDirty(pVisual, pointerId))
            {
                if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible())
                {
                    pReleaseVisual = pVisual;

                    // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
                    // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
                    // objects are alive and state is re-validated after return.
                    m_inputManager.m_coreServices.GetEventManager()->Raise(
                        EventHandle(KnownEventIndex::UIElement_PointerEntered),
                        TRUE /* bReFire */,
                        pVisual,
                        pArgs,
                        TRUE /* fRaiseSync */,
                        TRUE /* fInputEvent */);
                }
            }
            else if (m_inputManager.m_coreServices.GetInputServices()->IsInputPointerNodeDirty(pVisual, pointerId))
            {
                m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);
            }
        }
        else
        {
            m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);
            m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, false);
        }
        pVisual = pVisual->GetParentInternal();
    }

    CDependencyObject* rootElement = m_inputManager.GetContentRoot()->GetVisualTreeNoRef()->GetRootElementNoRef();
    IFC_RETURN(m_inputManager.m_coreServices.GetInputServices()->UpdateCursor(rootElement, m_fSawMouseLeave));
    return S_OK;
}

bool PointerInputProcessor::ShouldEventAlwaysPassThroughPopupLightDismissLayer(_In_ MessageMap message, _In_opt_ PointerInfo *pointerInfo)
{
    switch (message)
    {
        case XCP_POINTERDOWN:
        case XCP_POINTERUPDATE:
        case XCP_POINTERUP:
            // A pointer-info object should always accompany a pointer-down event.
            ASSERT(pointerInfo != nullptr);

            return pointerInfo->m_bRightButtonPressed || m_iRightButtonPointerId == pointerInfo->m_pointerId;

        case XCP_DRAGENTER:
        case XCP_DRAGLEAVE:
        case XCP_DRAGOVER:
        case XCP_DROP:
            return true;

        default:
            return false;
    }
}

bool PointerInputProcessor::IsInputTypeTreatedLikeTouch(_In_ XPointerInputType pointerType)
{
    // With Pen Navigation feature enabled, Pen input is treated
    // the same as touch (as opposed to being treated as mouse) except when
    // the barrel button is pressed.
    return (pointerType == XcpPointerInputTypeTouch || (pointerType == XcpPointerInputTypePen && !m_fBarrelButtonPressed));
}

xref_ptr<CPointer> PointerInputProcessor::GetPointerForPointerInfo(_In_ const PointerInfo& pointerInfo)
{
    // Some apps handle the PointerMoved event and check the PointerRoutedEventArgs.Pointer object on event event.
    // During most runs of PointerMoved events the Pointer object is the same every time, and there's so way for
    // the app to modify it, so we can just re-use the object.  This saves extra allocations in the core, and for
    // the peer, and for the RCW when this is a managed app.
    if (m_cachedPointer && m_cachedPointer->MatchesPointerInfo(pointerInfo))
    {
        return m_cachedPointer;
    }

    xref_ptr<CPointer> pointer;
    CREATEPARAMETERS cp(&m_inputManager.m_coreServices);

    IFCFAILFAST(CPointer::Create((CDependencyObject**)&pointer, &cp));
    IFCFAILFAST(pointer->SetPointerFromPointerInfo(pointerInfo));

    m_cachedPointer = pointer;
    return m_cachedPointer;
}

_Check_return_ HRESULT PointerInputProcessor::SetPointerFromPointerMessage(_In_ InputMessage *pMsg, _In_ CPointerEventArgs *pPointerArgs)
{
    xref_ptr<CPointer> pointer = GetPointerForPointerInfo(pMsg->m_pointerInfo);

    // We should remove m_pointerDeviceType or m_pointerInfo.m_pointerInputType
    pPointerArgs->m_pointerDeviceType = pPointerArgs->GetPointerDeviceType(pMsg->m_pointerInfo.m_pointerInputType);

    pPointerArgs->m_pPointer = pointer.detach();

    return S_OK; //RRETURN_REMOVAL
}

_Check_return_ HRESULT PointerInputProcessor::SetPointerKeyModifiers(_In_ XUINT32 keyModifiers, _In_ CPointerEventArgs* pPointerArgs)
{
    if (keyModifiers & KEY_MODIFIER_CTRL)
    {
        pPointerArgs->m_keyModifiers |= DirectUI::VirtualKeyModifiers::Control;
    }
    // PlatformServices interpreted VK_MENU as KEY_MODIFIER_ALT
    if (keyModifiers & KEY_MODIFIER_ALT)
    {
        pPointerArgs->m_keyModifiers |= DirectUI::VirtualKeyModifiers::Menu;
    }
    if (keyModifiers & KEY_MODIFIER_SHIFT)
    {
        pPointerArgs->m_keyModifiers |= DirectUI::VirtualKeyModifiers::Shift;
    }
    if (keyModifiers & KEY_MODIFIER_WINDOWS)
    {
        pPointerArgs->m_keyModifiers |= DirectUI::VirtualKeyModifiers::Windows;
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::HitTestHelper(
    _In_ XPOINTF ptHit,
    _In_opt_ CDependencyObject *pHitTestRoot,
    _Outptr_result_maybenull_ CDependencyObject **ppVisualHit)
{
    // Do the hit-test.
    IFC_RETURN(m_inputManager.m_coreServices.HitTest(ptHit, pHitTestRoot, ppVisualHit));

    // In case of no root ScrollViewer under the hidden root(which is the Canvas as the public root),
    // Windows8, HitTest returns nullptr on the outside of root bound if there isn't a root ScrollViewer.
    // Windows8.1, HitTest returns the hidden root if  there isn't a root ScrollViewer.
    // To make the windows8 compatible, reset the hit result as nullptr if it is a hidden root.
    if (*ppVisualHit == m_inputManager.GetContentRoot()->GetVisualTreeNoRef()->GetRootVisual())
    {
        ReleaseInterface(*ppVisualHit);
    }

    return S_OK;
}

bool PointerInputProcessor::ShouldEventCloseFlyout(_In_ MessageMap message)
{
    // Pointer-down messages should close the topmost flyout.
    // The others can pass through the light-dismiss layer, but should leave it open.
    return message == XCP_POINTERDOWN;
}

// When the topmost light-dismissable popup is a flyout,
// we allow pointer events to pass through the light-dismiss layer
// in three different scenarios:
//
// 1. Right-clicks always pass through the light-dismiss layer;
//
// 2. Drag-and-drop events always pass through the light-dismiss layer;
//
// 3. All other pointer events pass through the light-dismiss layer
//    if they are over a UI element that the flyout has designated
//    as an element that pointer events can reach through the light-dismiss layer.
//
_Check_return_ HRESULT PointerInputProcessor::HitTestWithLightDismissAwareness(
    _Inout_ xref_ptr<CDependencyObject>& contactDO,
    _In_ XPOINTF contactPoint,
    _In_ MessageMap message,
    _In_opt_ PointerInfo *pointerInfo,
    _In_ CDependencyObject* hitTestRoot)
{
    const auto contentRoot = m_inputManager.GetContentRoot();

    IFC_RETURN(HitTestHelper(contactPoint, hitTestRoot, contactDO.ReleaseAndGetAddressOf()));

    if (contactDO)
    {
        CPopup* popup = nullptr;
        CPopupRoot* popupRoot = do_pointer_cast<CPopupRoot>(contactDO);
        CUIElement* elementToDisableHitTestingOn = nullptr;

        if (popupRoot)
        {
            popup = popupRoot->GetTopmostPopup(CPopupRoot::PopupFilter::LightDismissOnly);
            elementToDisableHitTestingOn = popupRoot;
        }
        else
        {
            popup = CPopup::GetClosestPopupAncestor(do_pointer_cast<CUIElement>(contactDO.get()));
            elementToDisableHitTestingOn = popup;
        }

        CDependencyObject* overlayInputPassThroughElementNoRef = nullptr;

        if (popup)
        {
            IFC_RETURN(popup->GetOverlayInputPassThroughElementNoRef(&overlayInputPassThroughElementNoRef));
        }

        // If this popup isn't light dismiss but has an overlay input pass-through element,
        // then we'll need to handle that separately, as the popup root being returned as a
        // hit-tested element only happens for light-dismiss popups.
        if (popupRoot ||
            (popup && overlayInputPassThroughElementNoRef != nullptr && !popup->IsFlyout() && !popup->m_fIsLightDismiss))
        {
            if (popup->IsFlyout() || overlayInputPassThroughElementNoRef != nullptr)
            {
                bool hitTestAgain = true;
                bool shouldEventPassThroughLightDismissLayer = ShouldEventAlwaysPassThroughPopupLightDismissLayer(message, pointerInfo);

                // If the event shouldn't always pass through the light-dismiss overlay,
                // then we need to check to see if the pointer is currently over the
                // pass-through element, if one exists.  If one does, and the pointer is over it,
                // then we'll pass the event through.  Otherwise, we won't.
                if (!shouldEventPassThroughLightDismissLayer)
                {
                    if (overlayInputPassThroughElementNoRef != nullptr)
                    {
                        // If the app specified that the overlay input pass-through element is the root visual,
                        // then we want to always allow the event to pass through the light-dismiss layer.
                        // This is needed because setting the root visual as the pass-through element implies that
                        // you want all input to go through the light-dismiss layer, but since popups don't have
                        // the root visual as their ancestor in the visual tree, the test for ancestry will fail.
                        if (overlayInputPassThroughElementNoRef->GetPublicRootVisual() == overlayInputPassThroughElementNoRef)
                        {
                            shouldEventPassThroughLightDismissLayer = true;
                        }
                        else
                        {
                            xref_ptr<CDependencyObject> subtreeHitTestDO;

                            CUIElementHitTestDisabler disableHitTesting(elementToDisableHitTestingOn);
                            IFC_RETURN(HitTestHelper(contactPoint, hitTestRoot, subtreeHitTestDO.ReleaseAndGetAddressOf()));

                            if (subtreeHitTestDO && overlayInputPassThroughElementNoRef->IsAncestorOf(subtreeHitTestDO))
                            {
                                // If the popup in question is a flyout with a button placement target that this hit-testing has hit,
                                // then we want to prevent that button's flyout from being opened by this input, since that will cause
                                // the flyout to close and then immediately be reopened.
                                CFrameworkElement* placementTargetNoRef = nullptr;

                                if (auto associatedFlyoutNoRef = popup->GetAssociatedFlyoutNoRef())
                                {
                                    IFC_RETURN(FxCallbacks::FlyoutBase_GetPlacementTargetNoRef(associatedFlyoutNoRef, &placementTargetNoRef));
                                }

                                if (placementTargetNoRef->IsAncestorOf(subtreeHitTestDO))
                                {
                                    if (auto placementTargetAsButtonNoRef = do_pointer_cast<CButton>(placementTargetNoRef))
                                    {
                                        IFC_RETURN(FxCallbacks::Button_SuppressFlyoutOpening(placementTargetAsButtonNoRef));
                                    }
                                }
                                contactDO = subtreeHitTestDO;
                                shouldEventPassThroughLightDismissLayer = true;
                                hitTestAgain = false;
                            }
                        }
                    }
                }

                if (shouldEventPassThroughLightDismissLayer)
                {
                    if (hitTestAgain)
                    {
                        CUIElementHitTestDisabler disableHitTesting(elementToDisableHitTestingOn);
                        IFC_RETURN(HitTestHelper(contactPoint, hitTestRoot, contactDO.ReleaseAndGetAddressOf()));
                    }

                    if (ShouldEventCloseFlyout(message))
                    {
                        if (popup->IsFlyout())
                        {
                            // If the input hit a flyout, then we'll want to close child popups of that flyout
                            // instead of closing all flyouts.  Otherwise, we'll close all flyouts.
                            // If we have a chain of open flyouts, then closing the root flyout will close
                            // all its nested children.
                            CFlyoutBase *associatedFlyout = nullptr;

                            if (auto contactPopup = CPopup::GetClosestPopupAncestor(do_pointer_cast<CUIElement>(contactDO.get())))
                            {
                                associatedFlyout = contactPopup->GetAssociatedFlyoutNoRef();
                            }

                            IFC_RETURN(FxCallbacks::FlyoutBase_CloseOpenFlyout(associatedFlyout));
                        }
                        else if (popupRoot)
                        {
                            IFC_RETURN(popupRoot->CloseTopmostPopup(DirectUI::FocusState::Pointer, CPopupRoot::PopupFilter::LightDismissOnly));
                        }
                    }
                }
             }
        }
        else if (message == XCP_DRAGENTER || message == XCP_DRAGLEAVE || message == XCP_DRAGOVER || message == XCP_DROP)
        {
            auto uiElement = do_pointer_cast<CUIElement>(contactDO);

            if (uiElement && uiElement->GetAllowsDragAndDropPassThrough())
            {
                CUIElementHitTestDisabler disableLightDismissLayerHitTesting(uiElement);

                IFC_RETURN(HitTestHelper(contactPoint, hitTestRoot, contactDO.ReleaseAndGetAddressOf()));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::ProcessPointerExitedState(_In_ XINT32 pointerId, _In_ CPointerEventArgs* pPointerEventArgs)
{
    XINT32 pointerIdFromPointerExitedState = 0;
    CPointerExitedState* pPointerExitedState = nullptr;
    CDependencyObject* pNewPointerEnteredDO = nullptr;
    CDependencyObject* pPointerExitedDO = nullptr;
    xvector<CPointerExitedState*>* pPointerExitedStates = nullptr;
    XUINT32 cPointerExited = 0;

    ASSERT(pointerId > 0);

    // Reset the handled state to fire PointerExited event.
    pPointerEventArgs->m_bHandled = FALSE;

    auto& mapPointerExitedState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerExitedState();

    auto guard = wil::scope_exit([&] {
        if (pPointerExitedStates)
        {
            cPointerExited = pPointerExitedStates->size();
            for (XUINT32 iPointerExited = 0; iPointerExited < cPointerExited; iPointerExited++)
            {
                IGNOREHR(pPointerExitedStates->get_item(iPointerExited, pPointerExitedState));
                ASSERT(pPointerExitedState);
                PointerExitedStateKey key = { pPointerExitedState->m_pointerId, pPointerExitedState->GetExitedDONoRef() };
                IGNOREHR(pPointerExitedState->SetEnteredDO(nullptr));
                IGNOREHR(pPointerExitedState->SetExitedDO(nullptr));
                IGNOREHR(mapPointerExitedState.Remove(key, pPointerExitedState));
                delete pPointerExitedState;
            }
            delete pPointerExitedStates;
        }
    });

    for (xchainedmap<PointerExitedStateKey, CPointerExitedState*>::const_iterator it = mapPointerExitedState.begin(); it != mapPointerExitedState.end(); ++it)
    {
        pPointerExitedState = (*it).second;
        if (pPointerExitedState)
        {
            pPointerExitedDO = pPointerExitedState->GetExitedDONoRef();
            pointerIdFromPointerExitedState = pPointerExitedState->GetPointerId();

            if (pPointerExitedDO && pointerId == pointerIdFromPointerExitedState)
            {
                pNewPointerEnteredDO = pPointerExitedState->GetEnteredDONoRef();

                if (pNewPointerEnteredDO && pPointerExitedDO->IsActive())
                {
                    // Ignore HR to ensure the clean up the pointer exited state.
                    IGNOREHR(ProcessPointerEnterLeave(
                        pNewPointerEnteredDO,
                        pPointerExitedDO,
                        pointerId,
                        pPointerEventArgs,
                        FALSE /* bSkipLeave */,
                        FALSE /* bForceRaisePointerEntered */,
                        TRUE /* bIgnoreHitTestVisibleForPointerExited */,
                        FALSE /* bAsyncEvent */,
                        TRUE /* bAddEventRequest */));
                }
                else
                {
                    // Ignore HR to ensure the clean up the pointer exited state.
                    // If pNewPointerEnteredDO is nullptr, PointerExited event will be fired once
                    // on the exited DO element instead of the firing PointerExited to the
                    // routing pNewPointerEnteredDO.
                    IGNOREHR(ProcessPointerLeave(
                        pPointerExitedDO,
                        pointerId,
                        pPointerEventArgs,
                        FALSE /* bAsyncEvent */,
                        TRUE /* bAddEventRequest */,
                        static_cast<CDependencyObject*>(pNewPointerEnteredDO),
                        pNewPointerEnteredDO ? FALSE : TRUE /* bRaiseOnce */));
                }

                if (pPointerExitedStates == nullptr)
                {
                    pPointerExitedStates = new xvector<CPointerExitedState*>();
                }

                IFC_RETURN(pPointerExitedStates->push_back(pPointerExitedState));
            }
        }
    }

    return S_OK;
}

// NOTE: THIS CODE IS VERY SIMILAR TO THE CODE IN PROCESSENTERLEAVE BUT I DO NOT
// WANT TO CONSOLIDATE BECAUSE OTHERWISE THE DEBUGGING EXPERIENCE IS TRAUMATIC
// UNTIL WE HAVE ALL ISSUES IN THIS CODE CLEANED UP I WOULD LIKE TO KEEP THIS
_Check_return_ HRESULT PointerInputProcessor::ProcessPointerEnterLeave(
    _In_opt_ CDependencyObject *pContactElement,
    _In_ CDependencyObject *pPointerEnterDO,
    _In_ XUINT32 pointerId,
    _In_ CPointerEventArgs *pArgs,
    _In_ XINT32 bSkipLeave,
    _In_ bool bForceRaisePointerEntered,
    _In_ bool bIgnoreHitTestVisibleForPointerExited,
    _In_ bool bAsyncEvent,
    _In_ bool bAddEventRequest,
    _Out_ bool* enterLeaveFound)
{
    CDependencyObject *pVisual = pPointerEnterDO;
    xref_ptr<CDependencyObject> pReleaseVisual;
    CUIElement* pPointerExitedAsUIENoRef = nullptr;
    REQUEST* pEventRequestPointerExited = nullptr;
    bool bEventPointerExitedAdded = false;
    bool enterLeaveEventFired = false;
    std::shared_ptr<CPointerState> pointerState;
    const auto contentRoot = m_inputManager.GetContentRoot();

    pArgs->m_bHandled = false;

    // Source shouldn't be the hidden root since it doesn't have a managed peer.
    ASSERT(pArgs->m_pSource != contentRoot->GetVisualTreeNoRef()->GetRootVisual());

    auto guard = wil::scope_exit([&] {
        if (enterLeaveFound != nullptr) *enterLeaveFound = enterLeaveEventFired;
    });

    // Pass 1: Set the leave and dirty bits on all elements in the parent chain

    while (pVisual)
    {
        m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, true);
        m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, false);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 2 set the enter on the elements up to the tree root.  At this same time
    // we will check each item that we pass to see if it is a pannable ScrollViewer or
    // Map Control and if so we will enable pan/zoom for "auto" mouse mode.
    pVisual = pContactElement;

    while (pVisual)
    {
        m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, true);
        pVisual = pVisual->GetParentInternal();
    }

    // Pass 3 : now fire events for leave until we hit first element with enter set

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

    if (mapPointerState.ContainsKey(pointerId))
    {
        IFC_RETURN(mapPointerState.Get(pointerId, pointerState));
    }

    pVisual = static_cast<CDependencyObject*>(pPointerEnterDO);

    while (pVisual && ! m_inputManager.m_coreServices.GetInputServices()->HasPointerEnter(pVisual, pointerId))
    {
        m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);

        // Don't fire the leave message if we're calling this from PointerUp on Capture mode
        if ((static_cast<CUIElement*>(pVisual)->IsHitTestVisible() || bIgnoreHitTestVisibleForPointerExited) &&
            (!bSkipLeave))
        {
            pReleaseVisual = pVisual;

            // Update our the enter DO in the pointer state as we walk up the element
            // chain.  Since elements can be removed from the tree as we are walking
            // it, we don't want to fire leave events for elements that we have
            // already fired on.  More importantly, we don't want to call back into
            // this function to process those leave events and stomp on our real
            // enter DO. The whole logic of ProcessPointerExitedState and the way
            // we handle entered items being removed from the tree should be looked
            // at.  ProcessPointerExitedState assumes the next enter DO will be be
            // the parent of the element being removed, which is not necessarily true
            // when we are walking the tree firing leave events.  In addition, if a leave
            // event handler removes an ancestor of the element, then we can end up
            // not calling leave on ancestors further up the tree.  To compensate for
            // this we, adjust the enter DO to the parent of the element that we are
            // currently leaving.  That means that if the leave event ends up removing
            // a descendent (or itself), we won't even go through fixup code for the
            // state change, because the element will no longer being the the DO chain
            // (From the enterDO to the root).
            // Note also, there is a scenario where we pass the mouse pointer's Enter DO
            // to a new pointer being created for touch.  So we only want to update the
            // DO if the current enter DO is the current visual.
            if (pointerState && pointerState->GetEnterDO() == pVisual)
            {
                IFC_RETURN(pointerState->SetEnterDO(pVisual->GetParentInternal()));
            }

            // To fire PointerExited event on the removed(leaving the tree) element, add the event request manually.
            if (bAddEventRequest && !pVisual->IsActive())
            {
                pPointerExitedAsUIENoRef = do_pointer_cast<CUIElement>(pVisual);
                if (pPointerExitedAsUIENoRef && pPointerExitedAsUIENoRef->m_pEventList)
                {
                    IFC_RETURN(GetRemovedPointerExitedEventRequest(EventHandle(KnownEventIndex::UIElement_PointerExited), pPointerExitedAsUIENoRef->m_pEventList, &pEventRequestPointerExited));
                    if (pEventRequestPointerExited)
                    {
                        IFC_RETURN(m_inputManager.m_coreServices.GetEventManager()->AddRequest(pPointerExitedAsUIENoRef, pEventRequestPointerExited));
                        bEventPointerExitedAdded = TRUE;
                    }
                }
            }

            // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
            // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
            // objects are alive and state is re-validated after return.
            m_inputManager.m_coreServices.GetEventManager()->Raise(
                EventHandle(KnownEventIndex::UIElement_PointerExited),
                TRUE /* bReFire */,
                pVisual,
                pArgs,
                bAsyncEvent ? FALSE : TRUE /* fRaiseSync */,
                TRUE /* fInputEvent */);
            enterLeaveEventFired = true;

            if (bEventPointerExitedAdded)
            {
                ASSERT(pEventRequestPointerExited);
                bEventPointerExitedAdded = FALSE;
                IFC_RETURN(m_inputManager.m_coreServices.GetEventManager()->RemoveRequest(pPointerExitedAsUIENoRef, pEventRequestPointerExited));
            }
        }
        pVisual = pVisual->GetParentInternal();
    }

    // Set the new EnterDOHolder with the contact element
    if (pointerState)
    {
        IFC_RETURN(pointerState->SetEnterDO(static_cast<CDependencyObject*>(pContactElement)));
    }

    pVisual = static_cast<CDependencyObject*>(pContactElement);
    pArgs->m_bHandled = FALSE;

    // Pass 4: Now walk up the tree
    //      A) firing enter events until we hit the first guy that has dirty bit set
    //         and from there reset the dirty bit
    //      B) no need to set the mouse cursor for pointer enter/leave
    while (pVisual)
    {
        if (bForceRaisePointerEntered ||
            (m_inputManager.m_coreServices.GetInputServices()->HasPointerEnter(pVisual, pointerId) &&
                !m_inputManager.m_coreServices.GetInputServices()->IsInputPointerNodeDirty(pVisual, pointerId)))
        {
            if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible())
            {
                pReleaseVisual = pVisual;

                // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
                // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
                // objects are alive and state is re-validated after return.
                m_inputManager.m_coreServices.GetEventManager()->Raise(
                    EventHandle(KnownEventIndex::UIElement_PointerEntered),
                    TRUE /* bReFire */,
                    pVisual,
                    pArgs,
                    bAsyncEvent ? FALSE : TRUE /* fRaiseSync */,
                    TRUE /* fInputEvent */);
                enterLeaveEventFired = true;
            }
        }
        else if (m_inputManager.m_coreServices.GetInputServices()->IsInputPointerNodeDirty(pVisual, pointerId))
        {
            m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);
        }

        pVisual = pVisual->GetParentInternal();
    }

    CDependencyObject* rootElement= m_inputManager.GetContentRoot()->GetVisualTreeNoRef()->GetRootElementNoRef();
    m_inputManager.m_coreServices.GetInputServices()->UpdateCursor(rootElement);

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::ProcessPointerLeave(
    _In_ CDependencyObject *pExitedElement,
    _In_ XUINT32 pointerId,
    _In_ CPointerEventArgs *pPointerArgs,
    _In_ bool bAsyncEvent,
    _In_ bool bAddEventRequest,
    _In_opt_ CDependencyObject *pNewEnteredElement,
    _In_ bool bRaiseOnce)
{
    CDependencyObject *pVisual = NULL;
    CUIElement* pPointerExitedAsUIENoRef = NULL;
    REQUEST* pEventRequestPointerExited = NULL;
    bool bEventPointerExitedAdded = false;
    const auto contentRoot = m_inputManager.GetContentRoot();

    // Source shouldn't be the hidden root since it doesn't have a managed peer.
    ASSERT(pPointerArgs->m_pSource != contentRoot->GetVisualTreeNoRef()->GetRootVisual());

    // Clean up all pointer entered and node dirty flags
    pVisual = static_cast<CDependencyObject*>(pExitedElement);
    while (pVisual)
    {
        m_inputManager.m_coreServices.GetInputServices()->RemoveInputPointerNodeDirty(pVisual, pointerId);
        m_inputManager.m_coreServices.GetInputServices()->RemovePointerEnter(pVisual, pointerId);
        pVisual = pVisual->GetParentInternal();
        if (bRaiseOnce || (pNewEnteredElement && pNewEnteredElement == pVisual))
        {
            break;
        }
    }

    // Raise the leave element from the contact element to the ancestor elements
    pVisual = static_cast<CDependencyObject*>(pExitedElement);
    while (pVisual && !m_inputManager.m_coreServices.GetInputServices()->HasPointerEnter(pVisual, pointerId))
    {
        if (static_cast<CUIElement*>(pVisual)->IsHitTestVisible())
        {
            // To fire PointerExited event on the removed(leaving the tree) element, add the event request manually.
            if (bAddEventRequest && !pVisual->IsActive())
            {
                pPointerExitedAsUIENoRef = do_pointer_cast<CUIElement>(pVisual);
                if (pPointerExitedAsUIENoRef && pPointerExitedAsUIENoRef->m_pEventList)
                {
                    IFC_RETURN(GetRemovedPointerExitedEventRequest(EventHandle(KnownEventIndex::UIElement_PointerExited), pPointerExitedAsUIENoRef->m_pEventList, &pEventRequestPointerExited));
                    if (pEventRequestPointerExited)
                    {
                        IFC_RETURN(m_inputManager.m_coreServices.GetEventManager()->AddRequest(pPointerExitedAsUIENoRef, pEventRequestPointerExited));
                        bEventPointerExitedAdded = TRUE;
                    }
                }
            }

            // SYNC_CALL_TO_APP - This is a synchronous callout to application code that allows the application to re-enter
            // XAML. The application could change state and release objects, so protect against reentrancy by ensuring that
            // objects are alive and state is re-validated after return.
            m_inputManager.m_coreServices.GetEventManager()->Raise(
                EventHandle(KnownEventIndex::UIElement_PointerExited),
                TRUE /* bReFire */,
                pVisual,
                pPointerArgs,
                bAsyncEvent ? FALSE : TRUE /* fRaiseSync */,
                TRUE /* fInputEvent */);

            if (bEventPointerExitedAdded)
            {
                ASSERT(pEventRequestPointerExited);
                bEventPointerExitedAdded = FALSE;
                IFC_RETURN(m_inputManager.m_coreServices.GetEventManager()->RemoveRequest(pPointerExitedAsUIENoRef, pEventRequestPointerExited));
            }
        }
        pVisual = pVisual->GetParentInternal();
        if (bRaiseOnce || (pNewEnteredElement && pNewEnteredElement == pVisual))
        {
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::GetRemovedPointerExitedEventRequest(
    _In_ EventHandle hEventPointerExited,
    _In_ CXcpList<REQUEST>* pEventList,
    _Out_ REQUEST** pEventRequestPointerExited)
{
    CXcpList<REQUEST>::XCPListNode *pEventTemp = NULL;
    REQUEST* pEventRequestTemp = NULL;
    *pEventRequestPointerExited = NULL;

    pEventTemp = pEventList->GetHead();

    // Find the removed PointerExited event request in the event list.
    while (pEventTemp)
    {
        pEventRequestTemp = static_cast<REQUEST*>(pEventTemp->m_pData);

        if (pEventRequestTemp &&
            pEventRequestTemp->m_hEvent == hEventPointerExited &&
            !pEventRequestTemp->m_bAdded)
        {
            *pEventRequestPointerExited = pEventRequestTemp;
            break;
        }

        pEventTemp = pEventTemp->m_pNext;
    }

    return S_OK; //RRETURN_REMOVAL
}

_Check_return_ HRESULT PointerInputProcessor::ReleasePointerCapture(_In_ CDependencyObject *pObject, _In_ CPointer* pPointer)
{
    XUINT32 pointerId = pPointer->GetPointerId();
    std::shared_ptr<CPointerState> pointerState;

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();
    if (!mapPointerState.ContainsKey(pointerId))
    {
        return S_OK;
    }

    IFC_RETURN(mapPointerState.Get(pointerId, pointerState));

    // Release mouse capture in case of mouse input device type
    if (pointerState->GetPointerInputType() == XcpPointerInputTypeMouse)
    {
        IXcpBrowserHost *pBrowserHost = m_inputManager.m_coreServices.GetBrowserHost();
        if (EnsureIslandMouseCaptureReleased())
        {
            // Mouse capture was in an island, and was released.
        }
        else
        {
            xref_ptr<CPopup> popup;
            xref_ptr<CUIElement> element;
            element = static_cast<CUIElement*>(pObject);
            IFCFAILFAST(CPopupRoot::GetOpenPopupForElement(element, popup.ReleaseAndGetAddressOf()));
            if (popup && popup->HasPointerCapture())
            {
                popup->ReleasePointerCapture();
            }
            else if (pBrowserHost)
            {
                FxCallbacks::JupiterWindow_ReleasePointerCapture();
            }
        }
    }

    if (!mapPointerState.ContainsKey(pointerId))
    {
        // The pointerId we looked up earlier in the function is no longer valid.  This means there was some kind of
        // re-entrancy that has changed our pointer state (try a breakpoint in CleanPointerProcessingState).  pointerState
        // is now probably pointing at invalid memory.
        // This is not expected; please prevent the re-entrancy.
        XAML_FAIL_FAST();
    }

    CDependencyObject* pPointerEnterDO = pointerState->GetEnterDO();
    CDependencyObject* pPointerCaptureDO = pointerState->GetCaptureDO();
    XPOINTF pointLast = pointerState->GetLastPosition();

    // we can only release capture if we had it in the first place
    // leaving an element means it has no capture
    if (pObject != pPointerCaptureDO || !pPointerCaptureDO)
    {
        return S_OK;
    }

    // The PointerCaptureLost event is raised at the location of the last known
    // input device state.
    xref_ptr<CPointerEventArgs> pArgs;
    pArgs.attach(new CPointerEventArgs(&m_inputManager.m_coreServices));

    pArgs->SetGlobalPoint(pointLast);

    // Set the source
    IFC_RETURN(pArgs->put_Source(pObject));

    // Set Pointer object
    pArgs->m_pPointer = pPointer;
    pPointer->AddRef();

    ixp::IPointerPoint* pointerPoint;
    CXamlIslandRoot* pIslandRoot = m_inputManager.GetContentRoot()->GetXamlIslandRootNoRef();
    if (pIslandRoot)
    {
        pointerPoint = (pIslandRoot->GetPreviousPointerPoint()).Get();
    }
    else
    {
        CJupiterWindow* jupiterWindow = DirectUI::DXamlServices::GetCurrentJupiterWindow();
        pointerPoint = (jupiterWindow->GetInputSiteAdapterPointerPoint()).Get();
    }
    pArgs->m_pPointerPoint = pointerPoint;

    // Reset the pointer entered and node dirty states
    if (pPointerEnterDO != pPointerCaptureDO)
    {
        CDependencyObject* pVisual = static_cast<CDependencyObject*>(pPointerCaptureDO);
        while (pVisual)
        {
            m_inputManager.m_coreServices.GetInputServices()->SetInputPointerNodeDirty(pVisual, pointerId, false);
            m_inputManager.m_coreServices.GetInputServices()->SetPointerEnter(pVisual, pointerId, false);
            pVisual = pVisual->GetParentInternal();
        }
    }

     // Clear the pointer capture DO and Pointer
    pointerState->SetCaptureDO(NULL);
    pointerState->SetCapturePointer(NULL);

#ifdef DM_DEBUG
    if (m_inputManager.m_coreServices.GetInputServices()->IsDMInfoTracingEnabled())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER | DMIM_DBG) /*traceType*/, L"DMIM[0x%p]:  ReleasePointerCapture - Raising PointerCaptureLost for 0x%p.", this, pPointerCaptureDO));
    }
#endif // DM_DEBUG

    if (pArgs->m_pPointerPoint)
    {
        m_inputManager.m_coreServices.GetEventManager()->RaiseRoutedEvent(
            EventHandle(KnownEventIndex::UIElement_PointerCaptureLost),
            pPointerCaptureDO,
            pArgs,
            TRUE /* bIgnoreVisibility */,
            TRUE /* fRaiseSync */,
            TRUE /* fInputEvent */);
    }

    // Remove the captured Pointer from the UIElement::PointerCaptures collection
    xref_ptr<CPointerCollection> pointerCaptures = static_cast<CUIElement*>(pObject)->GetPointerCaptures();
    if (pointerCaptures != nullptr)
    {
        XUINT32 countCaptures = pointerCaptures->GetCount();
        for (XUINT32 i = 0; i < countCaptures; i++)
        {
            xref_ptr<CPointer> pPointerFromCaptures;
            pPointerFromCaptures.attach(static_cast<CPointer*>(pointerCaptures->GetItemWithAddRef(i)));

            if (pPointerFromCaptures->m_uiPointerId == pPointer->m_uiPointerId)
            {
                // RemoveAt returns the object with a ref count still on it. We have to release that reference.
                xref_ptr<CDependencyObject> removed;
                removed.attach(static_cast<CDependencyObject*>(pointerCaptures->RemoveAt(i)));
                break;
            }
        }
    }

    if (pPointer->GetPointerDeviceType() == DirectUI::PointerDeviceType::Mouse)
    {
        CDependencyObject* rootElement = m_inputManager.GetContentRoot()->GetVisualTreeNoRef()->GetRootElementNoRef();
        IFC_RETURN(m_inputManager.m_coreServices.GetInputServices()->UpdateCursor(rootElement, m_fSawMouseLeave));
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::SetPointerCapture(
    _In_ CDependencyObject* pObject,
    _In_ CPointer* pPointer,
    _Out_ bool* pResult)
{
    std::shared_ptr<CPointerState> pointerState;
    xref_ptr<CDependencyObject> pointerCaptureDO;
    UINT32 pointerId = 0;
    xref_ptr<CUIElement> element;
    xref_ptr<CPointer> pointerCaptured;
    CREATEPARAMETERS cp(&m_inputManager.m_coreServices);

    IFCPTR_RETURN(pPointer);
    pointerId = pPointer->GetPointerId();

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

    if (mapPointerState.ContainsKey(pointerId))
    {
        IFC_RETURN(mapPointerState.Get(pointerId, pointerState));
        if (pointerState != nullptr)
        {
            pointerCaptureDO = pointerState->GetCaptureDO();
        }
    }

    // cannot call set pointer capture someone already has capture or no pointer input
    if (!pointerState || pointerCaptureDO || !pointerState->IsPointerDown() || pointerState->IsPointerCaptureDenied())
    {
        // if we're trying to capture the same DO, the result to script is still true, otherwise it's false
        *pResult = pointerCaptureDO && pointerCaptureDO == pObject;
    }
    else
    {
        // Now store the new entry for capture DO and Pointer
        pointerCaptureDO = pObject;
        IFC_RETURN(pointerState->SetCaptureDO(pointerCaptureDO.get()));

        // Create new Pointer and put on the PointerCaptures collection
        IFC_RETURN(CPointer::Create((CDependencyObject**)pointerCaptured.ReleaseAndGetAddressOf(), &cp));
        IFC_RETURN(pointerCaptured->SetPointer(pPointer));
        IFC_RETURN(pointerState->SetCapturePointer(pointerCaptured.get()));

        // Append the capture UIElement into the UIElement::PointerCaptures collection
        IFCPTR_RETURN(pObject);
        element = static_cast<CUIElement*>(pObject);
        auto pointerCaptures = element->GetPointerCaptures();
        if (pointerCaptures == nullptr)
        {
            IFC_RETURN(CPointerCollection::Create((CDependencyObject**)pointerCaptures.ReleaseAndGetAddressOf(), &cp));
            IFC_RETURN(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_PointerCaptures, pointerCaptures.get()));
        }
        IFC_RETURN(pointerCaptures->Append(pointerCaptured.get()));

        // Set mouse capture explicitly in case of mouse input device type
        if (pointerState->GetPointerInputType() == XcpPointerInputTypeMouse)
        {
            auto browserHost = m_inputManager.m_coreServices.GetBrowserHost();
            if (browserHost != nullptr)
            {
                CContentRoot* contentRoot = VisualTree::GetContentRootForElement(element);
                // When changing capture, always release old the mouse capture and clear
                // cached information.
                contentRoot->GetInputManager().GetPointerInputProcessor().EnsureIslandMouseCaptureReleased();

                // If we are inside a CompositionIsland, need to use the island's CoreComponentInput
                // to set focus rather than the top-level HWND.
                //
                // ISLANDTODO: Do we want to make this ancestor walk more efficient, or create a
                // method like CUIElement::GetElementIslandInputSite() that encapsulates the correct policy?
                // - Are there any other places we need to stop the ancestor walk, such as at
                //   KnownTypeIndex::PopupRoot or something?
                xref_ptr<CPopup> popup;
                IFCFAILFAST(CPopupRoot::GetOpenPopupForElement(element, popup.ReleaseAndGetAddressOf()));
                if (popup && popup->IsWindowed())
                {
                    popup->SetPointerCapture();
                }
                else if (CXamlIslandRoot* capturedIsland = contentRoot->GetXamlIslandRootNoRef())
                {
                    // Island case:
                    // - Use, and __remember__, the CoreInput to request mouse capture.
                    IFCFAILFAST(capturedIsland->SetPointerCapture());
                }
                else
                {
                    // Non-island / legacy HWND case:
                    // - If this element is inside a windowed popup, we'll need to pass the HWND for
                    //   that window to SetCapture.
                    FxCallbacks::JupiterWindow_SetPointerCapture();
                }
            }
        }

        // Set result of operation for script
        *pResult = TRUE;
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::ReleaseAllPointerCaptures(
    _In_opt_ CDependencyObject *pObject)
{
    std::shared_ptr<CPointerState> pointerState;
    CDependencyObject* pPointerCaptureDO = NULL;

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

    for (auto it = mapPointerState.begin();
        it != mapPointerState.end();
        ++it)
    {
        pointerState = (*it).second;
        if (pointerState)
        {
            pPointerCaptureDO = pointerState->GetCaptureDO();
            if (pPointerCaptureDO)
            {
                if (pObject == nullptr || pPointerCaptureDO == pObject)
                {
                    IFC_RETURN(ReleasePointerCapture(pPointerCaptureDO, pointerState->GetCapturePointer()));
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT PointerInputProcessor::ReleasePointerCaptureById(_In_ XINT32 releasePointerId)
{
    std::shared_ptr<CPointerState> pointerState;
    CDependencyObject* pPointerCaptureDO = NULL;

    auto& mapPointerState = m_inputManager.m_coreServices.GetInputServices()->GetMapPointerState();

    for (auto it = mapPointerState.begin();
        it != mapPointerState.end();
        ++it)
    {
        pointerState = (*it).second;
        if (pointerState && pointerState->GetPointerId() == releasePointerId)
        {
            pPointerCaptureDO = pointerState->GetCaptureDO();
            if (pPointerCaptureDO)
            {
                IFC_RETURN(ReleasePointerCapture(pPointerCaptureDO, pointerState->GetCapturePointer()));
            }

            if (pointerState->GetEnterDO() && IsInputTypeTreatedLikeTouch(pointerState->GetPointerInputType()) &&
                pointerState->IsPointerDown())
            {
                // A DManip-based manipulation is recognized.

                // Stop any ongoing gesture-based manipulation, allowing a final
                // callback for a ManipulationCompleted notification.
                m_inputManager.m_coreServices.GetInputServices()->GetInteractionManager().StopInteraction(
                    static_cast<CUIElement*>(pointerState->GetEnterDO()),
                    true /*bCallbackForManipulationCompleted*/);

                // Discard all potential rejection cross-slide viewports associated with this
                // element and its parent chain.
                IFC_RETURN(m_inputManager.m_coreServices.GetInputServices()->DiscardRejectionViewportsInParentChain(pointerState->GetEnterDO()));
            }
        }
    }

    return S_OK;
}

// ISLANDTODO: Rethink how we're doing mouse capture so that Xaml can't ever get into a bad state:
// - Should we allow mouse capture beyond a drag operation (i.e. after POINTERUP)?
// - Should the capture be automatically enabled on the CoreInput side during the POINTERDOWN
//   operation, and completely removed from CInputServices?
bool PointerInputProcessor::EnsureIslandMouseCaptureReleased()
{
    const auto contentRoot = m_inputManager.GetContentRoot();
    if (contentRoot)
    {
        if (CXamlIslandRoot* xamlIslandRoot = contentRoot->GetXamlIslandRootNoRef())
        {
            xamlIslandRoot->ReleasePointerCapture();
            return true;  // Had capture
        }
    }

    if (XamlOneCoreTransforms::IsEnabled())
    {
        if (FxCallbacks::JupiterWindow_HasPointerCapture())
        {
            FxCallbacks::JupiterWindow_ReleasePointerCapture();
            return true; // had capture
        }

    }

    return false;  // Didn't have capture
}

InputMessage* PointerInputProcessor::GetCurrentMsgForDirectManipulationProcessing() const
{
    return m_pCurrentMsgForDirectManipulationProcessing;
}

void PointerInputProcessor::SetCurrentMsgForDirectManipulationProcessing(_In_opt_ InputMessage* inputMessage)
{
    m_pCurrentMsgForDirectManipulationProcessing = inputMessage;
}

void PointerInputProcessor::ResetPendingPointerEventArgs()
{
    m_pPendingPointerEventArgs.reset();
}
