// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomaticDragHelper.h"
#include "UIElement.g.h"

// The standard Windows mouse drag box size is defined by SM_CXDRAG and SM_CYDRAG.
// UIElement uses the standard box size with dimensions multiplied by this constant.
// This arrangement is in place as accidentally triggering a drag was deemed too easy while
// selecting several items with the mouse in quick succession.
#define UIELEMENT_MOUSE_DRAG_THRESHOLD_MULTIPLIER 2.0

using namespace DirectUI;

AutomaticDragHelper::AutomaticDragHelper(_In_ DirectUI::UIElement* pUIElement, _In_ bool shouldAddInputHandlers)
    : m_pOwnerNoRef(pUIElement)
    , m_shouldAddInputHandlers(shouldAddInputHandlers)
{
}

// Begin tracking the mouse cursor in order to fire a drag start if the pointer
// moves a certain distance away from m_lastMouseLeftButtonDownPosition.
_Check_return_ HRESULT
AutomaticDragHelper::BeginCheckingForMouseDrag(
_In_ xaml_input::IPointer* pPointer)
{
    BOOLEAN captured;

    ASSERT(pPointer);
    IFC_RETURN(m_pOwnerNoRef->CapturePointer(pPointer, &captured));

    m_isCheckingForMouseDrag = !!captured;

    return S_OK;
}


// Stop tracking the mouse cursor.
_Check_return_ HRESULT
AutomaticDragHelper::StopCheckingForMouseDrag(
    _In_ xaml_input::IPointer* pPointer)
{
    ASSERT(pPointer);

    // Do not call ReleasePointerCapture() more times than we called CapturePointer()
    if (m_isCheckingForMouseDrag)
    {
        m_isCheckingForMouseDrag = false;

        bool hasCapture = false;
        IFC_RETURN(OwnerHasPointerCapture(m_mouseDragPointerId, &hasCapture));
        if (hasCapture)
        {
            // Releasing capture can delete this helper through a CanDrag change.
            IFC_RETURN(m_pOwnerNoRef->ReleasePointerCapture(pPointer));
        }
    }
    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::OwnerHasPointerCapture(
    _In_ UINT pointerId,
    _Out_ bool* hasCapture)
{
    *hasCapture = false;
    ctl::ComPtr<wfc::IVectorView<xaml_input::Pointer*>> captures;
    IFC_RETURN(m_pOwnerNoRef->get_PointerCaptures(&captures));
    if (captures)
    {
        UINT count = 0;
        IFC_RETURN(captures->get_Size(&count));
        for (UINT i = 0; i < count; ++i)
        {
            ctl::ComPtr<xaml_input::IPointer> capture;
            UINT capturedId = 0;
            IFC_RETURN(captures->GetAt(i, &capture));
            IFC_RETURN(capture->get_PointerId(&capturedId));
            if (capturedId == pointerId)
            {
                *hasCapture = true;
                break;
            }
        }
    }
    return S_OK;
}


// Return true if we're tracking the mouse and newMousePosition is outside the drag
// rectangle centered at m_lastMouseLeftButtonDownPosition (see IsOutsideDragRectangle).
bool AutomaticDragHelper::ShouldStartMouseDrag(
    _In_ wf::Point newMousePosition) const
{
    return m_isCheckingForMouseDrag && IsOutsideDragRectangle(newMousePosition, m_lastMouseLeftButtonDownPosition);
}


// Returns true if testPoint is outside of the rectangle
// defined by the SM_CXDRAG and SM_CYDRAG system metrics and
// dragRectangleCenter.
bool AutomaticDragHelper::IsOutsideDragRectangle(
    _In_ wf::Point testPoint,
    _In_ wf::Point dragRectangleCenter) const
{
    const double dx = abs(testPoint.X - dragRectangleCenter.X);
    const double dy = abs(testPoint.Y - dragRectangleCenter.Y);

    double maxDx = GetSystemMetrics(SM_CXDRAG);
    double maxDy = GetSystemMetrics(SM_CYDRAG);

    maxDx *= UIELEMENT_MOUSE_DRAG_THRESHOLD_MULTIPLIER;
    maxDy *= UIELEMENT_MOUSE_DRAG_THRESHOLD_MULTIPLIER;

    return (dx > maxDx || dy > maxDy);
}


_Check_return_ HRESULT
AutomaticDragHelper::StartDetectingDrag()
{
    if (m_shouldAddInputHandlers && m_dragDropPointerPressedToken.value == 0)
    {
        ctl::ComPtr<xaml_input::IPointerEventHandler> spDragDropPointerPressedHandler;

        spDragDropPointerPressedHandler.Attach(
            new ClassMemberEventHandler<
            UIElement,
            xaml::IUIElement,
            xaml_input::IPointerEventHandler,
            IInspectable,
            xaml_input::IPointerRoutedEventArgs>(m_pOwnerNoRef, &UIElement::OnPointerPressed));


        IFC_RETURN(m_pOwnerNoRef->add_PointerPressed(spDragDropPointerPressedHandler.Get(), &m_dragDropPointerPressedToken));
    }

    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::StopDetectingDrag()
{
    // Removing the helper must not leave callbacks that will target a future helper.
    IFC_RETURN(UnregisterEvents());
    m_isLeftButtonPressed = false;
    m_isCheckingForMouseDrag = false;
    if (m_dragDropPointerPressedToken.value != 0)
    {
        IFC_RETURN(m_pOwnerNoRef->remove_PointerPressed(m_dragDropPointerPressedToken));
        // zero out the token;
        m_dragDropPointerPressedToken.value = 0;
    }

    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::RegisterDragPointerEvents()
{
    if (m_shouldAddInputHandlers)
    {
        ctl::ComPtr<xaml_input::IPointerEventHandler> spDragDropPointerMovedHandler;
        ctl::ComPtr<xaml_input::IPointerEventHandler> spDragDropPointerReleasedHandler;
        ctl::ComPtr<xaml_input::IPointerEventHandler> spDragDropPointerCaptureLostHandler;

        // Hookup pointer events so we can catch and handle it for drag and drop.
        if (m_dragDropPointerMovedToken.value == 0)
        {
            spDragDropPointerMovedHandler.Attach(
                new ClassMemberEventHandler<
                UIElement,
                xaml::IUIElement,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(m_pOwnerNoRef, &UIElement::OnPointerMoved));


            IFC_RETURN(m_pOwnerNoRef->add_PointerMoved(spDragDropPointerMovedHandler.Get(), &m_dragDropPointerMovedToken));
        }

        if (m_dragDropPointerReleasedToken.value == 0)
        {
            spDragDropPointerReleasedHandler.Attach(
                new ClassMemberEventHandler<
                UIElement,
                xaml::IUIElement,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(m_pOwnerNoRef, &UIElement::OnPointerReleased));


            UIElement::PointerReleasedEventSourceType* eventSource = nullptr;
            IFC_RETURN(m_pOwnerNoRef->GetPointerReleasedEventSourceNoRef(&eventSource));
            IFC_RETURN(eventSource->AddHandler(spDragDropPointerReleasedHandler.Get(), true /* handledEventsToo */));
            m_dragDropPointerReleasedToken.value = reinterpret_cast<INT64>(spDragDropPointerReleasedHandler.Get());
        }

        if (m_dragDropPointerCaptureLostToken.value == 0)
        {
            spDragDropPointerCaptureLostHandler.Attach(
                new ClassMemberEventHandler<
                UIElement,
                xaml::IUIElement,
                xaml_input::IPointerEventHandler,
                IInspectable,
                xaml_input::IPointerRoutedEventArgs>(m_pOwnerNoRef, &UIElement::OnPointerCaptureLost));


            UIElement::PointerCaptureLostEventSourceType* eventSource = nullptr;
            IFC_RETURN(m_pOwnerNoRef->GetPointerCaptureLostEventSourceNoRef(&eventSource));
            IFC_RETURN(eventSource->AddHandler(spDragDropPointerCaptureLostHandler.Get(), true /* handledEventsToo */));
            m_dragDropPointerCaptureLostToken.value = reinterpret_cast<INT64>(spDragDropPointerCaptureLostHandler.Get());
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
AutomaticDragHelper::HandlePointerPressedEventArgs(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;
    ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
    UINT pointerId = 0;

    ASSERT(pArgs);

    IFC_RETURN(pArgs->get_Pointer(&spPointer));
    IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));
    IFC_RETURN(spPointer->get_PointerId(&pointerId));

    if (m_isLeftButtonPressed && pointerId != m_mouseDragPointerId)
    {
        bool hasCapture = false;
        IFC_RETURN(OwnerHasPointerCapture(m_mouseDragPointerId, &hasCapture));
        if (hasCapture)
        {
            return S_OK;
        }
        m_isLeftButtonPressed = false;
        m_isCheckingForMouseDrag = false;
        IFC_RETURN(UnregisterEvents());
    }

    // Preserve touch cleanup for non-primary presses as well.
    m_spPointerPoint = nullptr;
    m_spPointer = nullptr;
    m_isHoldingCompleted = false;

    IFC_RETURN(pArgs->GetCurrentPoint(m_pOwnerNoRef, &spPointerPoint));
    IFCPTR_RETURN(spPointerPoint);

    // Check if this is a mouse button down.
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        // Mouse button down.
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;
        BOOLEAN isLeftButtonPressed = FALSE;
        ixp::PointerUpdateKind updateKind = ixp::PointerUpdateKind_Other;

        IFC_RETURN(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR_RETURN(spPointerProperties);
        IFC_RETURN(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));
        IFC_RETURN(spPointerProperties->get_PointerUpdateKind(&updateKind));

        // A secondary-button chord must not replace the primary press origin.
        if (isLeftButtonPressed && updateKind == ixp::PointerUpdateKind_LeftButtonPressed)
        {
            // A fresh primary down also recovers from a release with no capture-loss notification.
            m_mouseDragPointerId = pointerId;
            m_isLeftButtonPressed = true;
            // Start listening for a mouse drag gesture
            IFC_RETURN(spPointerPoint->get_Position(&m_lastMouseLeftButtonDownPosition));
            IFC_RETURN(BeginCheckingForMouseDrag(spPointer.Get()));
            m_isLeftButtonPressed = m_isCheckingForMouseDrag;
            if (m_isCheckingForMouseDrag)
            {
                IFC_RETURN(RegisterDragPointerEvents());
            }
            else
            {
                IFC_RETURN(UnregisterEvents());
            }
        }
    }
    else
    {
        m_spPointerPoint = spPointerPoint;
        m_spPointer = spPointer;

        if (m_shouldAddInputHandlers && m_dragDropHoldingToken.value == 0)
        {
            // Touch input occurs, subscribe to holding
            ctl::ComPtr<xaml_input::IHoldingEventHandler> spDragDropHoldingEventHandler;

            spDragDropHoldingEventHandler.Attach(
                new ClassMemberEventHandler<
                UIElement,
                xaml::IUIElement,
                xaml_input::IHoldingEventHandler,
                IInspectable,
                xaml_input::IHoldingRoutedEventArgs>(m_pOwnerNoRef, &UIElement::OnHolding));


            IFC_RETURN(m_pOwnerNoRef->add_Holding(spDragDropHoldingEventHandler.Get(), &m_dragDropHoldingToken));
        }

        IFC_RETURN(RegisterDragPointerEvents());
    }

    return S_OK;
}

_Check_return_ HRESULT
AutomaticDragHelper::HandlePointerMovedEventArgs(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    ASSERT(pArgs);

    IFC_RETURN(pArgs->get_Pointer(&spPointer));
    IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));

    // Our behavior is different between mouse and touch.
    // It's up to us to detect mouse drag gestures - if we
    // detect one here, start a drag drop.
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        UINT pointerId = 0;
        IFC_RETURN(spPointer->get_PointerId(&pointerId));
        if (!m_isLeftButtonPressed || pointerId != m_mouseDragPointerId)
        {
            return S_OK;
        }

        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        wf::Point newMousePosition;

        IFC_RETURN(pArgs->GetCurrentPoint(m_pOwnerNoRef, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);

        IFC_RETURN(spPointerPoint->get_Position(&newMousePosition));
        if (ShouldStartMouseDrag(newMousePosition))
        {
            ctl::ComPtr<wf::IAsyncOperation<wadt::DataPackageOperation>> spAsyncOperation;
            bool ownerHadCapture = false;
            IFC_RETURN(OwnerHasPointerCapture(pointerId, &ownerHadCapture));
            ctl::ComPtr<UIElement> owner(m_pOwnerNoRef);

            m_isLeftButtonPressed = false;
            IFC_RETURN(UnregisterEvents());
            IFC_RETURN(StopCheckingForMouseDrag(spPointer.Get()));

            // No helper access after release: an app can delete it in PointerCaptureLost.
            BOOLEAN canDrag = FALSE;
            IFC_RETURN(owner->get_CanDrag(&canDrag));
            if (ownerHadCapture && canDrag)
            {
                IFC_RETURN(owner->StartDragAsync(spPointerPoint.Get(), &spAsyncOperation));
            }
        }
    }

    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::HandlePointerReleasedEventArgs(
_In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    ASSERT(pArgs);

    IFC_RETURN(pArgs->get_Pointer(&spPointer));
    UINT pointerId = 0;
    IFC_RETURN(spPointer->get_PointerId(&pointerId));
    if (m_isLeftButtonPressed && pointerId != m_mouseDragPointerId)
    {
        return S_OK;
    }
    IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));

    // Check if this is a mouse button up
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        BOOLEAN isLeftButtonPressed = FALSE;
        ctl::ComPtr<ixp::IPointerPoint> spPointerPoint;
        ctl::ComPtr<ixp::IPointerPointProperties> spPointerProperties;

        IFC_RETURN(pArgs->GetCurrentPoint(m_pOwnerNoRef, &spPointerPoint));
        IFCPTR_RETURN(spPointerPoint);
        IFC_RETURN(spPointerPoint->get_Properties(&spPointerProperties));
        IFCPTR_RETURN(spPointerProperties);
        IFC_RETURN(spPointerProperties->get_IsLeftButtonPressed(&isLeftButtonPressed));

        // if the mouse left button was the one released...
        if (m_isLeftButtonPressed && !isLeftButtonPressed)
        {
            m_isLeftButtonPressed = false;
            IFC_RETURN(UnregisterEvents());
            // Terminate any mouse drag gesture tracking.
            IFC_RETURN(StopCheckingForMouseDrag(spPointer.Get()));
        }
    }
    else
    {
        IFC_RETURN(UnregisterEvents());
    }

    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::HandlePointerCaptureLostEventArgs(
    _In_ xaml_input::IPointerRoutedEventArgs* pArgs)
{
    ctl::ComPtr<xaml_input::IPointer> spPointer;
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    ASSERT(pArgs);

    IFC_RETURN(pArgs->get_Pointer(&spPointer));
    UINT pointerId = 0;
    IFC_RETURN(spPointer->get_PointerId(&pointerId));
    if (m_isLeftButtonPressed && pointerId != m_mouseDragPointerId)
    {
        return S_OK;
    }

    IFC_RETURN(spPointer->get_PointerDeviceType(&pointerDeviceType));
    if (pointerDeviceType == mui::PointerDeviceType_Mouse || pointerDeviceType == mui::PointerDeviceType_Pen)
    {
        // We're not necessarily going to get a PointerReleased on capture lost, so reset this flag here.
        m_isLeftButtonPressed = false;
        m_isCheckingForMouseDrag = false;
    }

    IFC_RETURN(UnregisterEvents());

    return S_OK;
}


_Check_return_ HRESULT
AutomaticDragHelper::UnregisterEvents()
{
    // Unregister events handlers
    if (m_dragDropPointerMovedToken.value != 0)
    {
        IFC_RETURN(m_pOwnerNoRef->remove_PointerMoved(m_dragDropPointerMovedToken));
        m_dragDropPointerMovedToken.value = 0;
    }

    if (m_dragDropPointerReleasedToken.value != 0)
    {
        IFC_RETURN(m_pOwnerNoRef->remove_PointerReleased(m_dragDropPointerReleasedToken));
        m_dragDropPointerReleasedToken.value = 0;
    }

    if (m_dragDropPointerCaptureLostToken.value != 0)
    {
        IFC_RETURN(m_pOwnerNoRef->remove_PointerCaptureLost(m_dragDropPointerCaptureLostToken));
        m_dragDropPointerCaptureLostToken.value = 0;
    }

    if (m_dragDropHoldingToken.value != 0)
    {
        IFC_RETURN(m_pOwnerNoRef->remove_Holding(m_dragDropHoldingToken));
        m_dragDropHoldingToken.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT
AutomaticDragHelper::HandleHoldingEventArgs(
    _In_ xaml_input::IHoldingRoutedEventArgs* pArgs)
{
    mui::PointerDeviceType pointerDeviceType = mui::PointerDeviceType_Touch;

    ASSERT(pArgs);

    IFC_RETURN(pArgs->get_PointerDeviceType(&pointerDeviceType));

    if (pointerDeviceType == mui::PointerDeviceType_Touch)
    {
        ixp::HoldingState holdingState = ixp::HoldingState_Started;
        IFC_RETURN(pArgs->get_HoldingState(&holdingState));

        if (holdingState == ixp::HoldingState::HoldingState_Started)
        {
            m_isHoldingCompleted = true;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
AutomaticDragHelper::HandleDirectManipulationDraggingStarted()
{
    // Pen cross-slide uses the mouse/pen path, which does not store a touch pointer.
    ASSERT(!m_isHoldingCompleted || (m_spPointerPoint && m_spPointer));

    // Release cross-slide viewport now
    IFC_RETURN(m_pOwnerNoRef->DirectManipulationCrossSlideContainerCompleted());
    if (m_isHoldingCompleted)
    {
        IFC_RETURN(m_pOwnerNoRef->OnTouchDragStarted(m_spPointerPoint.Get(), m_spPointer.Get()));
    }

    m_spPointerPoint = nullptr;
    m_spPointer = nullptr;

    return S_OK;
}
