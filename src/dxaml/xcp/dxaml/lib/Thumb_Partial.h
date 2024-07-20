// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Thumb.g.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents the Thumb control.
    PARTIAL_CLASS(Thumb)
    {
        private:
            // Set to a value other than ManipulationModes_None at the beginning of a pen-driven drag within a ScrollBar.
            // That mode is then restored at the end of the drag operation, when the pointer is released or otherwise canceled.
            xaml_input::ManipulationModes m_prePenDragManipulationMode{ xaml_input::ManipulationModes_None };
                
            // Origin of the thumb's drag operation.
            wf::Point m_origin;

            // Last position of the thumb while during a drag operation.
            wf::Point m_previousPosition;

            // Transform to the original position of the thumb
            // Used so that we don't inadvertently change coordinate systems during a drag
            TrackerPtr<xaml_media::IGeneralTransform> m_tpTransformToOriginal;

            // True if we want to ignore all touch pointer inputs (set by mouse scrollbar)
            BOOLEAN m_ignoreTouchInput;

        public:
            // Initializes a new instance of the Thumb class.
            Thumb();
            ~Thumb() override;

            // Cancel a drag operation if it is currently in progress.
            _Check_return_ HRESULT CancelDragImpl();

            // Apply a template to the Thumb.
            IFACEMETHOD(OnApplyTemplate)() override;

            // Sets a value indicating whether the Thumb reacts to touch input or not.
            _Check_return_ HRESULT put_IgnoreTouchInput(_In_ BOOLEAN value);

        protected:
            // Handle the custom property changed event and call the
            // OnPropertyChanged methods. 
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // GotFocus event handler.
            IFACEMETHOD(OnGotFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

            // LostFocus event handler.
            IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

            // PointerPressed event handler.
            IFACEMETHOD(OnPointerPressed)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerEnter event handler.
            IFACEMETHOD(OnPointerEntered)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerMoved event handler.
            IFACEMETHOD(OnPointerMoved)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerReleased event handler.
            IFACEMETHOD(OnPointerReleased)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerExited event handler.
            IFACEMETHOD(OnPointerExited)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerCaptureLost event handler.
            IFACEMETHOD(OnPointerCaptureLost)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerCanceled event handler.
            IFACEMETHOD(OnPointerCanceled)(
                _In_ xaml_input::IPointerRoutedEventArgs* args) override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            // Update the current visual state of the thumb.
            _Check_return_ HRESULT UpdateVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions = true);
            
            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(
                _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            // Update the visual states when the Visibility property is changed.
            _Check_return_ HRESULT OnVisibilityChanged() override;

        private:
            // Caches and replaces the Thumb's ManipulationMode used prior to a pen-driven drag, if needed.
            _Check_return_ HRESULT CacheAndReplacePrePenDragManipulationMode(mui::PointerDeviceType nPointerDeviceType);

            // Restores the Thumb's ManipulationMode used prior to a pen-driven drag, if needed.
            _Check_return_ HRESULT RestorePrePenDragManipulationMode(_In_ xaml_input::IPointerRoutedEventArgs* args);

            // Update the visual states when the IsDragging property is changed.
            _Check_return_ HRESULT OnDraggingChanged();

            // Called when we got or lost focus
            _Check_return_ HRESULT FocusChanged(_In_ BOOLEAN hasFocus);

            // Raise DragStarted event
            _Check_return_ HRESULT RaiseDragStarted();

            // Raise DragDelta event
            _Check_return_ HRESULT RaiseDragDelta(_In_ DOUBLE dDeltaX, _In_ DOUBLE dDeltaY);

            // Raise DragCompleted event
            _Check_return_ HRESULT RaiseDragCompleted(_In_ BOOLEAN isCanceled);

            // Called to evaluate the current input should be ignored
            _Check_return_ HRESULT ShouldIgnoreInput(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs, 
                _Out_ BOOLEAN* pIgnoreInput);
    };
}
