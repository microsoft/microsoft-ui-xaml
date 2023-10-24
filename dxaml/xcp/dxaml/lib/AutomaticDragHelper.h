// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
namespace DirectUI
{
    class AutomaticDragHelper
    {
    public:
        AutomaticDragHelper(_In_ DirectUI::UIElement* pUIElement, _In_ bool shouldAddInputHandlers);

        _Check_return_ HRESULT StartDetectingDrag();

        _Check_return_ HRESULT StopDetectingDrag();

        _Check_return_ HRESULT HandlePointerPressedEventArgs(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT HandlePointerMovedEventArgs(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT HandlePointerReleasedEventArgs(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT HandlePointerCaptureLostEventArgs(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT HandleHoldingEventArgs(
            _In_ xaml_input::IHoldingRoutedEventArgs* pArgs);

        _Check_return_ HRESULT HandleDirectManipulationDraggingStarted();

    private:
        // This is called to register PointerMoved, PointerReleased, and PointerCaptureLost
        // events for determining whether or not the gesture constitutes drag after first
        // PointerPressed.
        _Check_return_ HRESULT RegisterDragPointerEvents();

        _Check_return_ HRESULT UnregisterEvents();

        //
        // This group of members implements the mouse drag and drop gesture (click and drag
        // outside of a rectangle).
        //

        // Begin tracking the mouse cursor in order to fire a drag start if the pointer
        // moves a certain distance away from m_lastMouseLeftButtonDownPosition.
        _Check_return_ HRESULT BeginCheckingForMouseDrag(
            _In_ xaml_input::IPointer* pPointer);

        // Stop tracking the mouse cursor.
        _Check_return_ HRESULT StopCheckingForMouseDrag(
            _In_ xaml_input::IPointer* pPointer);

        // Return true if we're tracking the mouse and newMousePosition is outside the drag
        // rectangle centered at m_lastMouseLeftButtonDownPosition (see IsOutsideDragRectangle).
        bool ShouldStartMouseDrag(
            _In_ wf::Point newMousePosition) const;

        // Returns true if testPoint is outside of the rectangle
        // defined by the SM_CXDRAG and SM_CYDRAG system metrics and
        // dragRectangleCenter.
        bool IsOutsideDragRectangle(
            _In_ wf::Point testPoint,
            _In_ wf::Point dragRectangleCenter) const;

        bool m_isLeftButtonPressed = false;

        // If this is true, start a drag when the mouse moves far enough away from
        // m_lastMouseLeftButtonDownPosition (while the left button is held down).
        // "Far enough away" is defined via system metrics SM_CXDRAG and SM_CYDRAG.
        bool m_isCheckingForMouseDrag = false;

        bool m_isHoldingCompleted = false;

        bool m_shouldAddInputHandlers = false;

        ctl::ComPtr<ixp::IPointerPoint> m_spPointerPoint;
        ctl::ComPtr<xaml_input::IPointer> m_spPointer;

        // Latest position the mouse left button was pushed. Used to initiate a mouse drag and drop.
        wf::Point m_lastMouseLeftButtonDownPosition{};

        EventRegistrationToken m_dragDropPointerPressedToken{};
        EventRegistrationToken m_dragDropPointerMovedToken{};
        EventRegistrationToken m_dragDropPointerReleasedToken{};
        EventRegistrationToken m_dragDropPointerCaptureLostToken{};
        EventRegistrationToken m_dragDropHoldingToken{};

        UIElement* m_pOwnerNoRef;
    };
}