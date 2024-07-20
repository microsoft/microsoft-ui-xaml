// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a control that provides a scroll bar that has a sliding Thumb
//      whose position corresponds to a value.

#pragma once

#include "ScrollBar.g.h"

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents a control that provides a scroll bar that has a sliding Thumb
    // whose position corresponds to a value.
    PARTIAL_CLASS(ScrollBar)
    {
        private:
            // Flag indicating whether the ScrollBar must react to user input or not.
            BOOLEAN m_isIgnoringUserInput;

            // Flag indicating whether the mouse is over the ScrollBar.
            BOOLEAN m_isPointerOver;

            // Used to prevent GoToState(TRUE /*bUseTransitions*/) calls while applying the template.
            // We don't want to show the initial fade-out of the mouse/panning indicators.
            BOOLEAN m_suspendVisualStateUpdates;

            // Value indicating how far the ScrollBar has beeen dragged.
            DOUBLE m_dragValue;

            // Template parts for the horizontal and vertical templates
            // (each including a root, increase small/large repeat buttons, and
            // a thumb).
            TrackerPtr<xaml::IFrameworkElement> m_tpElementHorizontalTemplate;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementHorizontalLargeIncrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementHorizontalLargeDecrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementHorizontalSmallIncrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementHorizontalSmallDecrease;
            TrackerPtr<xaml_primitives::IThumb> m_tpElementHorizontalThumb;
            TrackerPtr<xaml::IFrameworkElement> m_tpElementVerticalTemplate;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementVerticalLargeIncrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementVerticalLargeDecrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementVerticalSmallIncrease;
            TrackerPtr<xaml_primitives::IRepeatButton> m_tpElementVerticalSmallDecrease;
            TrackerPtr<xaml_primitives::IThumb> m_tpElementVerticalThumb;
           
            TrackerPtr<xaml::IFrameworkElement> m_tpElementHorizontalPanningRoot;
            TrackerPtr<xaml::IFrameworkElement> m_tpElementHorizontalPanningThumb;
            TrackerPtr<xaml::IFrameworkElement> m_tpElementVerticalPanningRoot;
            TrackerPtr<xaml::IFrameworkElement> m_tpElementVerticalPanningThumb;

            // Event registration tokens for events attached to template parts
            // so the handlers can be removed if we apply a new template.
            EventRegistrationToken m_ElementHorizontalThumbDragStartedToken;
            EventRegistrationToken m_ElementHorizontalThumbDragDeltaToken;
            EventRegistrationToken m_ElementHorizontalThumbDragCompletedToken;
            EventRegistrationToken m_ElementHorizontalLargeDecreaseClickToken;
            EventRegistrationToken m_ElementHorizontalLargeIncreaseClickToken;
            EventRegistrationToken m_ElementHorizontalSmallDecreaseClickToken;
            EventRegistrationToken m_ElementHorizontalSmallIncreaseClickToken;
            EventRegistrationToken m_ElementVerticalThumbDragStartedToken;
            EventRegistrationToken m_ElementVerticalThumbDragDeltaToken;
            EventRegistrationToken m_ElementVerticalThumbDragCompletedToken;
            EventRegistrationToken m_ElementVerticalLargeDecreaseClickToken;
            EventRegistrationToken m_ElementVerticalLargeIncreaseClickToken;
            EventRegistrationToken m_ElementVerticalSmallDecreaseClickToken;
            EventRegistrationToken m_ElementVerticalSmallIncreaseClickToken;
            
            // value that indicates that we are currently blocking indicators from showing
            BOOLEAN m_blockIndicators;

            // Enters/Leaves the mode where the child's actual size is used
            // for the extent exposed through IScrollInfo
            bool m_isUsingActualSizeAsExtent;

            static bool IsConscious() noexcept
            {
                return DXamlCore::ShouldUseDynamicScrollbars();
            }

    protected:
            // Initializes a new instance of the ScrollBar class.
            ScrollBar();
            ~ScrollBar() override;
            
            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            // Update the visual states when the Visibility property is changed.
            _Check_return_ HRESULT OnVisibilityChanged() override;
            
            // Apply a template to the ScrollBar.
            IFACEMETHOD(OnApplyTemplate)() override;
            
            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(
                _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            // PointerPressed event handler.
            IFACEMETHOD(OnPointerPressed)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerReleased event handler.
            IFACEMETHOD(OnPointerReleased)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerEnter event handler.
            IFACEMETHOD(OnPointerEntered)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerExited event handler.
            IFACEMETHOD(OnPointerExited)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            // PointerCaptureLost event handler.
            IFACEMETHOD(OnPointerCaptureLost)(
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

            IFACEMETHOD(OnTapped)(_In_ xaml_input::ITappedRoutedEventArgs* pArgs) override;
            IFACEMETHOD(OnDoubleTapped)(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs) override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
            
            // Change to the correct visual state for the button.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;
            
            // Returns the actual length of the ScrollBar in the direction of its orientation.
            _Check_return_ HRESULT GetTrackLength(
                _Out_ DOUBLE* pLength);

            // Returns the combined actual length in the direction of its orientation of the ScrollBar's RepeatButtons.
            _Check_return_ HRESULT GetRepeatButtonsLength(
                _Out_ DOUBLE* pRepeatButtonsLength);
            
        public:
            // Called when the Value value changed.
            IFACEMETHOD(OnValueChanged)(
                _In_ DOUBLE oldValue,
                _In_ DOUBLE newValue) override;
            
            // Called when the Minimum value changed.
            IFACEMETHOD(OnMinimumChanged)(
                _In_ DOUBLE oldMinimum,
                _In_ DOUBLE newMinimum) override;
            
            // Called when the Maximum value changed.
            IFACEMETHOD(OnMaximumChanged)(
                _In_ DOUBLE oldMaximum,
                _In_ DOUBLE newMaximum) override;
            
            // Gets a value indicating whether the ScrollBar is currently
            // dragging.
            _Check_return_ HRESULT get_IsDragging(
                _Out_ BOOLEAN *pValue);

            // Gets a value indicating whether the ScrollBar reacts to user input or not.
            _Check_return_ HRESULT get_IsIgnoringUserInput(
                _Out_ BOOLEAN& value);

            // Sets a value indicating whether the ScrollBar reacts to user input or not.
            _Check_return_ HRESULT put_IsIgnoringUserInput(
                _In_ BOOLEAN value);

            _Check_return_ HRESULT get_ElementHorizontalTemplate(
                _Outptr_ xaml::IUIElement** ppElement);

            _Check_return_ HRESULT get_ElementVerticalTemplate(
                _Outptr_ xaml::IUIElement** ppElement);
                                
            // allows SeZo to remove the indicators from view temporarily during view change
            _Check_return_ HRESULT BlockIndicatorFromShowing();
            _Check_return_ HRESULT ResetBlockIndicatorFromShowing();

            // Adjust the drag value in case programmatic scroll changes come in during a drag
            _Check_return_ HRESULT AdjustDragValue(DOUBLE delta);

            // Enters the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StartUseOfActualSizeAsExtent()
            {
                m_isUsingActualSizeAsExtent = true;
            }

            // Leaves the mode where the child's actual size is used for
            // the extent exposed through IScrollInfo.
            void StopUseOfActualSizeAsExtent()
            {
                m_isUsingActualSizeAsExtent = false;
            }

        private:
            // Called whenever the Thumb drag operation is started.
            _Check_return_ HRESULT OnThumbDragStarted(
                _In_ IInspectable* pSender,
                _In_ xaml_primitives::IDragStartedEventArgs* pArgs);
            
            // Whenever the thumb gets dragged, we handle the event through this
            // function to update the current value depending upon the thumb
            // drag delta.
            _Check_return_ HRESULT OnThumbDragDelta(
                _In_ IInspectable* pSender,
                _In_ xaml_primitives::IDragDeltaEventArgs* pArgs);
            
            // Raise the Scroll event when the Thumb drag is completed.
            _Check_return_ HRESULT OnThumbDragCompleted(
                _In_ IInspectable* pSender,
                _In_ xaml_primitives::IDragCompletedEventArgs* pArgs);
            
            // Handle the SizeChanged event.
            _Check_return_ HRESULT OnSizeChanged(
                _In_ IInspectable* pSender,
                _In_ xaml::ISizeChangedEventArgs* pArgs);

            // Called whenever the SmallDecrement button is clicked.
            _Check_return_ HRESULT SmallDecrement(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Called whenever the SmallIncrement button is clicked.
            _Check_return_ HRESULT SmallIncrement(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Called whenever the LargeDecrement button is clicked.
            _Check_return_ HRESULT LargeDecrement(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Called whenever the LargeIncrement button is clicked.
            _Check_return_ HRESULT LargeIncrement(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // This raises the Scroll event, passing in the scrollEventType as a
            // parameter to let the handler know what triggered this event.
            _Check_return_ HRESULT RaiseScrollEvent(
                _In_ xaml_primitives::ScrollEventType scrollEventType);
            
            // Change the template being used to display this control when the
            // orientation changes.
            _Check_return_ HRESULT OnOrientationChanged();

            // Update track based on panning or mouse activity
            _Check_return_ HRESULT RefreshTrackLayout();

            //Update scrollbar visibility based on what input device is active and the orientation
            //of the ScrollBar.
            _Check_return_ HRESULT UpdateScrollBarVisibility();

            // This method will take the current min, max, and value to
            // calculate and layout the current control measurements.
            _Check_return_ HRESULT UpdateTrackLayout();

            // Based on the ViewportSize, the Track's length, and the Minimum
            // and Maximum values, we will calculate the length of the Thumb.
            _Check_return_ HRESULT ConvertViewportSizeToDisplayUnits(
                _In_ DOUBLE trackLength,
                _Out_ DOUBLE* pThumbSize);

            // This will resize the Thumb, based on calculations with the
            // ViewportSize, the Track's length, and the Minimum and Maximum
            // values.
            _Check_return_ HRESULT UpdateIndicatorLengths(
                _In_ DOUBLE trackLength,
                _Out_ DOUBLE* pMouseIndicatorLength,
                _Out_ DOUBLE* pTouchIndicatorLength);

            // Stores some ScrollBar internal sizes in a WarningContext when the layout iterations get close to the 250 limit
            // in order to ease layout cycles' debugging (that involve a ScrollBar).
            void StoreLayoutCycleWarningContext(
                double trackLength,
                double repeatButtonsLength,
                double minimum,
                double maximum,
                double mouseIndicatorSize,
                double touchIndicatorSize,
                double mouseMinSize,
                double touchMinSize,
                double actualSize);

            // Rounds the value using CUIElement::LayoutRound when get_UseLayoutRounding returns True.
            // Returns True when rounding was performed, and False otherwise.
            bool RoundWithLayoutRound(
                _Inout_ double* value);

            // Retrieves a reference to a child template object given its name
            template<class TInterface, class TRuntime>
            _Check_return_ HRESULT GetTemplateChildHelper(_In_reads_(cName) WCHAR* pName, _In_ size_t cName, _Outptr_ TRuntime** ppReference);
    };
}
