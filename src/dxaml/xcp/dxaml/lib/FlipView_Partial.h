// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FlipView.g.h"
#include "DirectManipulationStateChangeHandler.h"

// Minimum required time between mouse wheel inputs for triggering successive flips
#define FLIP_VIEW_DISTINCT_SCROLL_WHEEL_DELAY_MS 200

// How long the FlipView's navigation buttons show before fading out.
#define FLIP_VIEW_BUTTONS_SHOW_DURATION_MS 3000

namespace DirectUI
{
    class IsEnabledChangedEventArgs;
    class ButtonBase;

    // Represents a control that supports flipping among multiple items.
    PARTIAL_CLASS(FlipView)
        , private DirectManipulationStateChangeHandler
    {

    private:
        // Minimum required time between mouse wheel inputs for triggering successive flips. Set to 200ms by default. Can be overridden
        // by tests with IXamlTestHooks::SetFlipViewScrollWheelDelay and then restored with IXamlTestHooks::RestoreDefaultFlipViewScrollWheelDelay.
        static int s_scrollWheelDelayMS;

        // Dispatcher timer to set correct offset values after size changed
        TrackerPtr<xaml::IDispatcherTimer> m_tpFixOffsetTimer;

        // Stores a reference to the ButtonLayer part.
        //Panel* m_pButtonLayerPart;

        // Stores a reference to the  PreviousButtonHorizontalpart.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpPreviousButtonHorizontalPart;

        // Stores a reference to the NextButtonHorizontal part.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpNextButtonHorizontalPart;

        // Stores a reference to the PreviousButtonVertical part.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpPreviousButtonVerticalPart;

        // Stores a reference to the NextButtonVertical part.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpNextButtonVerticalPart;

        // Stores the ScrollingHostPart part SizeChanged token.
        EventRegistrationToken m_ScrollingHostPartSizeChangedEventToken;

        // Stores the PreviousButtonHorizontal part Click token.
        EventRegistrationToken m_PreviousButtonHorizontalPartClickEventToken;

        // Stores the NextButtonHorizontal part Click token.
        EventRegistrationToken m_NextButtonHorizontalPartClickEventToken;

        // Stores the PreviousButtonVertical part Click token.
        EventRegistrationToken m_PreviousButtonVerticalPartClickEventToken;

        // Stores the NextButtonVertical part Click token.
        EventRegistrationToken m_NextButtonVerticalPartClickEventToken;

        // Stores a value indicating whether we should show next/prev navigation buttons.
        BOOLEAN m_showNavigationButtons;

        // Stores a value to whether a FocusRect should be shown.
        BOOLEAN m_ShouldShowFocusRect;

        // Dispatcher timer causing the buttons to fade out after FLIP_VIEW_BUTTONS_SHOW_DURATION_MS.
        TrackerPtr<xaml::IDispatcherTimer> m_tpButtonsFadeOutTimer;

        // TRUE if new SelectedIndex is being animated into view.
        BOOLEAN m_animateNewIndex;
        // TRUE if selection change is due to a touch manipulation.
        BOOLEAN m_skipAnimationOnce;

        bool m_itemsAreSized;

        // True if we are in a Measure/Arrange pass. We need this to make sure that we don't change the SelectedIndex due to
        // the scroll position changing during a resize.
        bool m_inMeasure = false;
        bool m_inArrange = false;

        // Saved SnapPointsTypes. These are saved in the beginning of an animation and restored when the animation is completed.
        xaml_controls::SnapPointsType m_verticalSnapPointsType;
        xaml_controls::SnapPointsType m_horizontalSnapPointsType;

        // A value indicating the last time a scroll wheel event occurred.
        LONGLONG m_lastScrollWheelTime;
        // A value indicating the last wheel delta a scroll wheel event contained.
        INT m_lastScrollWheelDelta;

        bool m_keepNavigationButtonsVisible;

        bool m_moveFocusToSelectedItem = false;

        // Event pointers for the PointerEntered
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epPreviousButtonHorizontalPartPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epNextButtonHorizontalPartPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epPreviousButtonVerticalPartPointerEnteredHandler;
        ctl::EventPtr<UIElementPointerEnteredEventCallback> m_epNextButtonVerticalPartPointerEnteredHandler;

        // Event pointers for the PointerExited
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epPreviousButtonHorizontalPartPointerExitedHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epNextButtonHorizontalPartPointerExitedHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epPreviousButtonVerticalPartPointerExitedHandler;
        ctl::EventPtr<UIElementPointerExitedEventCallback> m_epNextButtonVerticalPartPointerExitedHandler;

        ctl::EventPtr<ViewChangedEventCallback> m_epScrollViewerViewChangedHandler;

    public:

        // Initializes a new instance.
        FlipView();

        // Destroys an instance.
        ~FlipView() override;

        // DirectManipulationStateChangeHandler implementation
        _Check_return_ HRESULT NotifyStateChange(
            _In_ DMManipulationState state,
            _In_ FLOAT xCumulativeTranslation,
            _In_ FLOAT yCumulativeTranslation,
            _In_ FLOAT zCumulativeFactor,
            _In_ FLOAT xCenter,
            _In_ FLOAT yCenter,
            _In_ BOOLEAN isInertial,
            _In_ BOOLEAN isTouchConfigurationActivated,
            _In_ BOOLEAN isBringIntoViewportConfigurationActivated) override;

        // Returns the applicable Previous and Next Buttons respectively. This is called from UIA to make these buttons accessible.
        _Check_return_ HRESULT GetPreviousAndNextButtons( _Outptr_ ButtonBase** ppPreviousButton, _Outptr_ ButtonBase** ppNextButton);

        // Can be invoked by tests with IXamlTestHooks::RestoreDefaultFlipViewScrollWheelDelay and IXamlTestHooks::SetFlipViewScrollWheelDelay
        // to override the default idle time required between mouse wheel deltas to trigger successive flips.
        static void RestoreDefaultScrollWheelDelay() { s_scrollWheelDelayMS = FLIP_VIEW_DISTINCT_SCROLL_WHEEL_DELAY_MS; }
        static void SetScrollWheelDelay(int scrollWheelDelayMS) { s_scrollWheelDelayMS = scrollWheelDelayMS; }

    protected:

        IFACEMETHOD(MeasureOverride)(
            _In_ wf::Size availableSize,
            _Out_ wf::Size* returnValue) override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* returnValue) override;

        // Invoked whenever application code or internal processes call ApplyTemplate.
        IFACEMETHOD(OnApplyTemplate)();

        // PointerWheelChanged event handler.
        IFACEMETHOD(OnPointerWheelChanged)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Invoked when the PointerEntered event is raised.
        IFACEMETHOD(OnPointerEntered)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Invoked when the PointerMoved event is raised.
        IFACEMETHOD(OnPointerMoved)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Determines if the specified item is (or is eligible to be) its own container.
        IFACEMETHOD(IsItemItsOwnContainerOverride)(_In_ IInspectable* item, _Out_ BOOLEAN* returnValue) override;

        // Creates or identifies the element that is used to display the given item.
        IFACEMETHOD(GetContainerForItemOverride)(_Outptr_ xaml::IDependencyObject** returnValue) override;

        // Prepares the specified element to display the specified item.
        IFACEMETHOD(PrepareContainerForItemOverride)(_In_ xaml::IDependencyObject* element, _In_ IInspectable* item) override;

        // Invoked when the Items property changes.
        _Check_return_ HRESULT NotifyOfSourceChanged(
            _In_ wfc::IObservableVector<IInspectable*>* pSender,
            _In_ wfc::IVectorChangedEventArgs* e) noexcept override;

        _Check_return_ HRESULT OnItemsChanged(
            _In_ wfc::IVectorChangedEventArgs* e) override;

        // Changes to the correct visual state.
        _Check_return_ HRESULT ChangeVisualState(_In_ bool bUseTransitions) override;

        // Override OnCreateAutomationPeer()
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        // GotFocus event handler.
        IFACEMETHOD(OnGotFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

        // LostFocus event handler.
        IFACEMETHOD(OnLostFocus)(
                _In_ xaml::IRoutedEventArgs* pArgs) override;

        // PointerCaptureLost event handler.
        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // PointerCanceled event handler.
        IFACEMETHOD(OnPointerCanceled)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Handle the custom property changed event and call the OnPropertyChanged methods.
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        // Called when the IsEnabled property changes.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        _Check_return_ HRESULT OnItemsSourceChanged(
            _In_ IInspectable* pNewValue) override;

        // Update the visual states when the Visibility property is changed.
        _Check_return_ HRESULT OnVisibilityChanged() override;

    private:

        _Check_return_ HRESULT OnScrollViewerViewChanged(_In_ IInspectable* pSender, _In_ xaml_controls::IScrollViewerViewChangedEventArgs* pArgs);

        _Check_return_ HRESULT SetFixOffsetTimer();

        _Check_return_ HRESULT FixOffset(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

        _Check_return_ HRESULT MoveNext(_Out_opt_ bool* pSuccessfullyMoved = nullptr);

        _Check_return_ HRESULT MovePrevious(_Out_opt_ bool* pSuccessfullyMoved = nullptr);

        // Undoes the application of a Template.
        _Check_return_ HRESULT UnhookTemplate();

        // Handles the SizeChanged event for the ScrollingHost part.
        _Check_return_ HRESULT OnScrollingHostPartSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);

        // Handles the Click event for the PreviousButton parts.
        _Check_return_ HRESULT OnPreviousButtonPartClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        // Handles the Click event for the NextButton parts.
        _Check_return_ HRESULT OnNextButtonPartClick(_In_ IInspectable* pSender, _In_ xaml::IRoutedEventArgs* pArgs);

        // Invoked when the PointerEntered for the navigation buttons event is raised.
        _Check_return_ HRESULT OnPointerEnteredNavigationButtons(_In_ IInspectable* pSender, _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Invoked when the PointerExited for the navigation buttons event is raised.
        _Check_return_ HRESULT OnPointerExitedNavigationButtons(_In_ IInspectable* pSender, _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnSelectedIndexChanged(_In_ IInspectable* pOldValue, _In_ IInspectable* pNewValue);

        // Invoked when ItemsHost is available
        _Check_return_ HRESULT OnItemsHostAvailable() override;

        _Check_return_ HRESULT CalculateBounds(_In_ INT index, _Out_ XRECTF* bounds);

        // Gets the desired width of the FlipViewItems.
        _Check_return_ HRESULT GetDesiredItemWidth(_Out_ DOUBLE* pWidth);

        // Gets the desired height of the FlipViewItems.
        _Check_return_ HRESULT GetDesiredItemHeight(_Out_ DOUBLE* pHeight);

        // Updates the selected item internally and updates the control's visual state
        _Check_return_ HRESULT UpdateSelectedIndex(UINT SelectedIndex);

        // Creates a button click event handler
        _Check_return_ HRESULT CreateButtonClickEventHandler(
            _In_reads_(strLen + 1) WCHAR *strButtonName,
            _In_ size_t strLen,
            _In_ HRESULT (DirectUI::FlipView::*pfnEventHandler)(IInspectable*, xaml::IRoutedEventArgs*),
            _Outptr_ xaml_primitives::IButtonBase **pButton,
            _Out_ EventRegistrationToken *pEventToken);

        // Creates a button pointer entered event handler
        _Check_return_ HRESULT CreateButtonPointerEnteredEventHandler(
            _In_ xaml_primitives::IButtonBase* pButton,
            _In_ ctl::EventPtr<UIElementPointerEnteredEventCallback>& eventPtr);

        // Creates a button pointer exited event handler
        _Check_return_ HRESULT CreateButtonPointerExitedEventHandler(
            _In_ xaml_primitives::IButtonBase* pButton,
            _In_ ctl::EventPtr<UIElementPointerExitedEventCallback>& eventPtr);

        // Initializes the ScrollViewer object
        _Check_return_ HRESULT InitializeScrollViewer();

        _Check_return_ HRESULT SetOffsetToSelectedIndex();

        _Check_return_ HRESULT HandlePointerLostOrCanceled(_In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        // Creates a DispatcherTimer for fading out the FlipView's buttons.
        _Check_return_ HRESULT EnsureButtonsFadeOutTimer();

        // Starts the fade out timer if not yet started.  Otherwise, resets it back to the original duration.
        _Check_return_ HRESULT ResetButtonsFadeOutTimer();

        // Hides the navigation buttons immediately.
        _Check_return_ HRESULT HideButtonsImmediately();

        // Handler for the Tick event on m_pButtonsFadeOutTimer.
        _Check_return_ HRESULT ButtonsFadeOutTimerTickHandler(
            _In_opt_ IInspectable* pUnused1,
            _In_opt_ IInspectable* pUnused2);

         _Check_return_ HRESULT SaveAndClearSnapPointsTypes();

         _Check_return_ HRESULT RestoreSnapPointsTypes();
     };
}
