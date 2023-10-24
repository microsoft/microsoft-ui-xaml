// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Represents a control that lets the user select from a range of values
//      by moving a Thumb control along a track.

#pragma once

#include "Slider.g.h"
#include "SliderKeyProcess.h"

#define SLIDER_DEFAULT_MAXIMUM              100
#define SLIDER_DEFAULT_SMALL_CHANGE         1
#define SLIDER_DEFAULT_LARGE_CHANGE         10
#define SLIDER_TOOLTIP_DEFAULT_FONT_SIZE    15
#define SLIDER_TOOLTIP_PADDING_LEFT         8.0f
#define SLIDER_TOOLTIP_PADDING_TOP          3.0f
#define SLIDER_TOOLTIP_PADDING_RIGHT        8.0f
#define SLIDER_TOOLTIP_PADDING_BOTTOM       5.0f

namespace DirectUI
{
    class Grid;
    class IsEnabledChangedEventArgs;
    class Thumb;
    class ToolTip;

    // Represents a control that lets the user select from a range of values by
    // moving a Thumb control along a track.
    PARTIAL_CLASS(Slider)
    {
        //// Grant friend access to the  KeyPress::Slider class so it can access
        // the Step method.
        friend class KeyPress::Slider;
    private:
        // Header template part
        TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;

        // Horizontal template root
        TrackerPtr<xaml::IFrameworkElement> m_tpElementHorizontalTemplate;

        // Top TickBar for Horizontal Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementTopTickBar;

        // Inline TickBar for Horizontal Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementHorizontalInlineTickBar;

        // Bottom TickBar for Horizontal Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementBottomTickBar;

        // Left TickBar for Vertical Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementLeftTickBar;

        // Inline TickBar for Vertical Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementVerticalInlineTickBar;

        // Right TickBar for Vertical Slider
        TrackerPtr<xaml_primitives::ITickBar> m_tpElementRightTickBar;

        // Horizontal decrease Rectangle
        TrackerPtr<xaml_shapes::IRectangle> m_tpElementHorizontalDecreaseRect;

        // Thumb for dragging track
        TrackerPtr<xaml_primitives::IThumb> m_tpElementHorizontalThumb;

        // Vertical template root
        TrackerPtr<xaml::IFrameworkElement> m_tpElementVerticalTemplate;

        // Vertical decrease Rectangle
        TrackerPtr<xaml_shapes::IRectangle> m_tpElementVerticalDecreaseRect;

        // Thumb for dragging track
        TrackerPtr<xaml_primitives::IThumb> m_tpElementVerticalThumb;

        // Container for the horizontal and vertical slider template parts
        TrackerPtr<xaml::IFrameworkElement> m_tpSliderContainer;

        // Event registration tokens for events attached to template parts
        EventRegistrationToken m_ElementHorizontalThumbDragStartedToken = {};
        EventRegistrationToken m_ElementHorizontalThumbDragDeltaToken = {};
        EventRegistrationToken m_ElementHorizontalThumbDragCompletedToken = {};
        EventRegistrationToken m_ElementHorizontalThumbSizeChangedToken = {};
        EventRegistrationToken m_ElementVerticalThumbDragStartedToken = {};
        EventRegistrationToken m_ElementVerticalThumbDragDeltaToken = {};
        EventRegistrationToken m_ElementVerticalThumbDragCompletedToken = {};
        EventRegistrationToken m_ElementVerticalThumbSizeChangedToken = {};
        EventRegistrationToken m_focusEngagedToken = {};
        EventRegistrationToken m_focusDisengagedToken = {};

        ctl::EventPtr<UIElementPointerPressedEventCallback> m_epSliderContainerPointerPressedHandler;
        ctl::EventPtr<UIElementPointerReleasedEventCallback> m_epSliderContainerPointerReleasedHandler;
        ctl::EventPtr<UIElementPointerMovedEventCallback> m_epSliderContainerPointerMovedHandler;
        ctl::EventPtr<UIElementPointerCaptureLostEventCallback> m_epSliderContainerPointerCaptureLostHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epSliderContainerSizeChangedHandler;

        // Whether the pointer is currently over the control
        BOOLEAN m_IsPointerOver;

        // A flag indicating whether we are in the midst of processing an input event.  Input events on Slider
        // like DragDelta and MouseLeftButtonDown cause IntermediateValue to change, and sometimes also causes
        // Value to change.  When value changes, it normally updates IntermediateValue as well.  This flag prevents
        // IntermediateValue from being stomped on when Value is being updated while processing a DragDelta event.
        BOOLEAN m_bProcessingInputEvent;

        // Accumulates drag offsets in case the pointer drags off the end of
        // the track.
        DOUBLE m_DragValue;

        // Flag indicating whether we are currently pressed.
        //
        // Note that Thumb handles PointerPressed and takes care of the case where the user drags the
        // thumb. However, Slider is responsible for handling the case where the user clicks and drags on some
        // part of the track other than the Thumb.  In this case, the Thumb should jump to the click location
        // and follow the pointer until the user releases it.
        //
        // Also note that Thumb handles drag a bit differently than the Slider track. The Thumb remembers the click
        // offset and keeps itself oriented with this offset from the mouse position while dragging.  E.G. if you
        // click the left side of the Thumb and start dragging, the point of click on the left side of the Thumb
        // will always remain directly under the mouse position.
        // For the Slider track, on the other hand, this behavior is undesirable - we want the Thumb to always line
        // up centered beneath the current pointer position while dragging.
        BOOLEAN m_isPressed;

        // Flag indicating whether we have created and are using a default Disambiguation UI ToolTip for the Horizontal Thumb.
        BOOLEAN m_bUsingDefaultToolTipForHorizontalThumb;

        // Flag indicating whether we have created and are using a default Disambiguation UI ToolTip for the Vertical Thumb.
        BOOLEAN m_bUsingDefaultToolTipForVerticalThumb;

        // Tracks the PointerId we have currently captured.  A value of 0 means "none".  We only capture one PointerId at a time;
        // we do not support multitouch.
        UINT m_capturedPointerId;

        // Holds the value of the slider at the moment of engagement, used to handle cancel-disengagements where we reset the value.
        double m_preEngagementValue{ 0.0 };

        BOOLEAN m_disengagedWithA{ false };

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

        // Returns the distance across the thumb in the direction of orientation.
        //  e.g. returns the thumb width for horizontal orientation.
        _Check_return_ HRESULT GetThumbLength(
            _Out_ DOUBLE* pThumbLength);

        _Check_return_ HRESULT UpdateThumbToolTipVisibility(
            _In_ BOOLEAN bIsVisible,
            _In_ AutomaticToolTipInputMode mode = AutomaticToolTipInputMode::None);

        // Returns a reference to the slider thumb
        _Check_return_ HRESULT get_Thumb(
            _Out_ Thumb** pThumb);

        _Check_return_ HRESULT get_ElementHorizontalTemplate(
            _Outptr_ xaml::IUIElement** ppElement);

        _Check_return_ HRESULT get_ElementVerticalTemplate(
            _Outptr_ xaml::IUIElement** ppElement);

        // Sets the m_isPressed flag, and updates visual state if the flag has changed.
        _Check_return_ HRESULT SetIsPressed(
            _In_ BOOLEAN isPressed);

        // Handles the case where right tapped is raised by unhandled.
        _Check_return_ HRESULT OnRightTappedUnhandled(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;

        // PreviewKeyDown event handler.
        IFACEMETHOD(OnPreviewKeyDownImpl)(
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        // PreviewKeyUp event handler.
        IFACEMETHOD(OnPreviewKeyUpImpl)(
            _In_ xaml_input::IKeyRoutedEventArgs* args);

    protected:
        // Initializes a new instance of the Slider class.
        Slider();

        // Destroys an instance of the Slider class.
        ~Slider() override;

        _Check_return_ HRESULT PrepareState() override;

        _Check_return_ HRESULT GetDefaultValue2(
            _In_ const CDependencyProperty* pDP,
            _Out_ CValue* pValue) override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

        _Check_return_ HRESULT OnIntermediateValuePropertyChanged(
            _In_ IInspectable *pOldValue,
            _In_ IInspectable *pNewValue);

        // Apply a template to the Slider.
        IFACEMETHOD(OnApplyTemplate)() override;

        // OnFocusEngaged event handler
        _Check_return_ HRESULT OnFocusEngaged(
            _In_ xaml_controls::IControl* sender,
            _In_ xaml_controls::IFocusEngagedEventArgs* args);

        _Check_return_ HRESULT OnFocusDisengaged(
            _In_ xaml_controls::IControl* sender,
            _In_ xaml_controls::IFocusDisengagedEventArgs* args);

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(
            _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // GotFocus event handler.
        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // LostFocus event handler.
        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        // KeyDown event handler.
        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // PointerEnter event handler.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerExited event handler.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerPressed event handler.
        _Check_return_ HRESULT OnPointerPressed(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // PointerReleased event handler.
        _Check_return_ HRESULT OnPointerReleased(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // PointerCaptureLost event handler.
        _Check_return_ HRESULT OnPointerCaptureLost(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // PointerMoved event handler.
        _Check_return_ HRESULT OnPointerMoved(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        // Change to the correct visual state for the button.
        _Check_return_ HRESULT ChangeVisualState(
            // true to use transitions when updating the visual state, false
            // to snap directly to the new visual state.
            _In_ bool bUseTransitions) override;

        // Change the template being used to display this control when the orientation changes.
        _Check_return_ HRESULT OnOrientationChanged();

        // Updates the track layout using ActualWidth and ActualHeight of the control.
        _Check_return_ HRESULT UpdateTrackLayout();

        // This method will take the current min, max, and value to calculate and layout the current control measurements.
        _Check_return_ HRESULT UpdateTrackLayout(
            _In_ DOUBLE actualWidth,
            _In_ DOUBLE actualHeight) noexcept;

    private:
        DOUBLE LayoutRoundedDimension(DOUBLE value);
        float RoundingStep() const;

        // Called whenever the Thumb drag operation is started.
        _Check_return_ HRESULT OnThumbDragStarted(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IDragStartedEventArgs* pArgs);

        // Whenever the thumb gets dragged, we handle the event through this
        // function to update IntermediateValue depending upon the thumb
        // drag delta.
        _Check_return_ HRESULT OnThumbDragDelta(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IDragDeltaEventArgs* pArgs);

        // Whenever the thumb drag completes, we handle the event through this
        // function to update IntermediateValue to snap to the closest step or tick mark.
        _Check_return_ HRESULT OnThumbDragCompleted(
            _In_ IInspectable* pSender,
            _In_ xaml_primitives::IDragCompletedEventArgs* pArgs);

        // Whenever the thumb size changes, we need to recalculate track layout.
        _Check_return_ HRESULT OnThumbSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        // Handle the SizeChanged event.
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        // Causes the TickBars to rearrange.
        _Check_return_ HRESULT InvalidateTickBarsArrange();

        // Set Value to the next step position in the specified direction.  Uses SmallChange
        // as the step interval if bUseSmallChange is TRUE; uses LargeChange otherwise.
        _Check_return_ HRESULT Step(
            _In_ BOOLEAN bUseSmallChange,
            _In_ BOOLEAN bForward);

        // Find the closest step position from fromValue.  stepValue lets you specify the step interval.
        _Check_return_ HRESULT GetClosestStep(
            _In_ DOUBLE stepDelta,
            _In_ DOUBLE fromValue,
            _Out_ DOUBLE* pValue);

        // Helper function to translate the thumb based on an input point.
        // Calculates what percentage of the track the point represents, and sets the Slider's
        // IntermediateValue and Value accordingly.
        _Check_return_ HRESULT MoveThumbToPoint(
            _In_ wf::Point point);

        // Helper function to set a default ToolTip on the Slider Thumb.
        //
        // Slider has a "Disambiguation UI" feature that displays the Value of the Slider in a ToolTip centered
        // on the Thumb.  Currently, this ToolTip is created in code if the Thumb template part's ToolTip is null.
        _Check_return_ HRESULT SetDefaultThumbToolTip(
            _In_ xaml_controls::Orientation orientation);

        // Called when IsThumbToolTipEnabled changes.
        _Check_return_ HRESULT OnIsThumbToolTipEnabledChanged();

        // Called when ThumbToolTipValueConverter changes.
        _Check_return_ HRESULT OnThumbToolTipValueConverterChanged();

        // Gets the Horizontal or Vertical root Grid, depending on the current Orientation.
        _Check_return_ HRESULT GetRootGrid(
            _Outptr_ Grid** ppRootGrid);

        // Get the disambiguation UI ToolTip for the current Thumb, depending on the orientation.
        _Check_return_ HRESULT GetCurrentThumbToolTip(
            _Outptr_ ToolTip** ppThumbToolTip);

        // Update the visibility of TickBars in the template when the TickPlacement property changes.
        _Check_return_ HRESULT OnTickPlacementChanged();

        // Contains the logic to be employed if we decide to handle pointer released.
        _Check_return_ HRESULT PerformPointerUpAction();

        // Updates the visibility of the Header ContentPresenter
        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();

        _Check_return_ HRESULT OnIsFocusEngagedChanged();
    };


    class DefaultDisambiguationUIConverter :
        public xaml_data::IValueConverter,
        public ctl::ComBase
    {
    public:
        //  Convert from source to target
        //
        //  Converts the Slider's DOUBLE Value to an HSTRING we can display in the
        //  disambiguation UI tooltip's TextBlock.Text property.
        //  Uses a weak reference to the Slider as a ConverterParameter.  We get the
        //  Slider's StepFrequency and determine the number of significant digits in its
        //  mantissa.  We will display the same number of significant digits in the mantissa
        //  of the disambiguation UI's value.  We round to the final significant digit.
        //
        //  We choose to display a maximum of 4 significant digits in our formatted string.
        //
        //  E.G. If StepFrequency==0.1 and Value==0.57, the disambiguation UI shows 0.6
        IFACEMETHOD(Convert)(
            _In_ IInspectable *value,
            _In_ wxaml_interop::TypeName targetType,
            _In_opt_ IInspectable *parameter,
            _In_ HSTRING language,
            _Outptr_ IInspectable **returnValue);

        //  Convert from target to source
        IFACEMETHODIMP ConvertBack(
            _In_ IInspectable *value,
            _In_ wxaml_interop::TypeName targetType,
            _In_opt_ IInspectable *parameter,
            _In_ HSTRING language,
            _Outptr_ IInspectable **returnValue) override
        {
            return E_NOTIMPL;
        }

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(xaml_data::IValueConverter)))
            {
                *ppObject = static_cast<xaml_data::IValueConverter*>(this);
            }
            else
            {
                RRETURN(ctl::ComBase::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }
    };
}
