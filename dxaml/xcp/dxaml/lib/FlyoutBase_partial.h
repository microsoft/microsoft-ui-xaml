// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      FlyoutBase - base class for Flyout provides the following functionality:
//        * Showing/hiding.
//        * Enforcing requirement that only one FlyoutBase is open at the time.
//        * Placement logic.
//        * Raising events.
//        * Entrance/exit transitions.
//        * Expose Attached property as storage from Flyouts in XAML.

#pragma once

#include "FlyoutBase.g.h"
#include <FlyoutBase.h>
#include "ComPtr.h"

#include "TransitionCollection.g.h"
#include "FlyoutMetadata.h"

namespace DirectUI
{
    class Popup;

    PARTIAL_CLASS(FlyoutBase)
    {
        friend class DxamlCoreTestHooks;
    public:

    enum class PreferredJustification
    {
        Center,
        Top,
        Bottom,
        Left,
        Right
    };

    // This type should always be kept in sync with CFlyoutBase::MajorPlacementMode
    using MajorPlacementMode = CFlyoutBase::MajorPlacementMode;

    public:
        FlyoutBase();

        ~FlyoutBase() override;

        // Helper for getting FlyoutMetadata and closing open flyout.
        static _Check_return_ HRESULT CloseOpenFlyout(_In_opt_ CFlyoutBase* parentFlyoutCore = nullptr);

        static _Check_return_ HRESULT HideFlyout(_In_ CFlyoutBase* flyout);

        // React to the IHM (touch keyboard) showing or hiding.
        // Flyout should always be shown above the IHM.
        _Check_return_ HRESULT NotifyInputPaneStateChange(
            InputPaneState inputPaneState,
            XRECTF inputPaneBounds);

        bool IsWindowedPopup();

        static _Check_return_ HRESULT IsOpen(_In_ CFlyoutBase* flyoutBase, _Out_ bool& isOpen);
        _Check_return_ HRESULT get_IsOpenImpl(_Out_ BOOLEAN* pValue);

        _Check_return_ HRESULT get_UsePickerFlyoutThemeImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_UsePickerFlyoutThemeImpl(BOOLEAN value);

        // Forward the call to create presenter.
        virtual _Check_return_ HRESULT CreatePresenterImpl(
            _Outptr_ xaml_controls::IControl** ppReturnValue);

        // Implementation of ShowAt method.
        _Check_return_ HRESULT ShowAtImpl(
            _In_ xaml::IFrameworkElement* pPlacementTarget);

        // Implementation of Show method.
        _Check_return_ HRESULT ShowAtWithOptionsImpl(
            _In_opt_ xaml::IDependencyObject* pPlacementTarget,
            _In_opt_ xaml_primitives::IFlyoutShowOptions* pShowOptions);

        // Implementation of Hide method.
        virtual _Check_return_ HRESULT HideImpl();

        _Check_return_ HRESULT PlaceFlyoutForDateTimePickerImpl(wf::Point point);

        _Check_return_ HRESULT SetIsWindowedPopup();

        void DisablePresenterResizing() { m_allowPresenterResizing = false; }

        _Check_return_ HRESULT get_IsLightDismissOverlayEnabledImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsLightDismissOverlayEnabledImpl(BOOLEAN value);

        _Check_return_ HRESULT get_IsConstrainedToRootBoundsImpl(_Out_ BOOLEAN* pValue);

        // Callback for ShowAt() from core layer
        static _Check_return_ HRESULT ShowAtStatic(
            _In_ CFlyoutBase* pCoreFlyoutBase,
            _In_ CFrameworkElement* pCoreTarget);

        // Callback for ShowAt() from core layer
        static _Check_return_ HRESULT ShowAtStatic(
            _In_ CFlyoutBase* pFlyoutBase,
            _In_ CFrameworkElement* pTarget,
            wf::Point point,
            wf::Rect exclusionRect,
            xaml_primitives::FlyoutShowMode flyoutShowMode);

        static _Check_return_ HRESULT OnClosingStatic(_In_ CFlyoutBase* object, _Out_ bool* cancel);

        static const FLOAT FlyoutMargin;

        _Check_return_ HRESULT TryInvokeKeyboardAcceleratorImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs);

        virtual _Check_return_ HRESULT OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs);

        _Check_return_ HRESULT EnsureAssociatedXamlRoot(_In_opt_ xaml::IDependencyObject* placementTarget);

        _Check_return_ xaml_controls::IControl* GetPresenter() const
        {
            return m_tpPresenter.Get();
        }

        _Check_return_ HRESULT get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue);
        _Check_return_ HRESULT put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue);

        // Calculates size of rectangle where Flyout can be placed
        static _Check_return_ HRESULT CalculateAvailableWindowRect(
            _In_ Popup* popup,
            _In_opt_ FrameworkElement* placementTarget,
            bool hasTargetPosition,
            wf::Point targetPosition,
            bool isFull,
            _Out_ wf::Rect* pAvailableRect);

        ctl::ComPtr<ABI::Microsoft::UI::Xaml::Media::ISystemBackdrop> GetSystemBackdrop();

    protected:
        _Check_return_ HRESULT OnPropertyChanged2(const PropertyChangedParams& args) override;

        // Convenience method which sets presenter style or clears the property if passed pStyle is null.
        static _Check_return_ HRESULT SetPresenterStyle(
            _In_ xaml_controls::IControl* pPresenter,
            _In_opt_ xaml::IStyle* pStyle);

        virtual _Check_return_ HRESULT ShowAtCore(
            _In_ xaml::IFrameworkElement* pPlacementTarget,
            _Out_ bool& openDelayed);

        virtual _Check_return_ HRESULT OnOpening();

        virtual _Check_return_ HRESULT OnClosing(bool* cancel);

        virtual _Check_return_ HRESULT OnClosed();

        // Sets required popup properties and hooks up transitions.
        virtual _Check_return_ HRESULT PreparePopupTheme(
            _In_ Popup* pPopup,
            MajorPlacementMode placementMode,
            _In_ xaml::IFrameworkElement* pPlacementTarget);

        virtual _Check_return_ HRESULT UpdatePresenterVisualState(
            FlyoutBase::MajorPlacementMode placementHint)
        {
            return S_OK;
        }

        // In the case where Placement has not been set explicitly, gives the control a chance
        // to change the Placement value based on other conditions.
        virtual _Check_return_ HRESULT AutoAdjustPlacement(
            _Inout_ FlyoutBase::MajorPlacementMode* pPlacement)
        {
            RRETURN(S_OK);
        }

        void SetTargetPosition(
            wf::Point point);

        _Check_return_ HRESULT ForwardPopupFlowDirection();

        _Check_return_ HRESULT EnsurePopupAndPresenter();

        bool m_isPositionedAtPoint;

        bool m_openingWindowedInProgress{ false };

        TrackerPtr<xaml_primitives::IPopup> m_tpPopup;

    private:
        _Check_return_ HRESULT ApplyTargetPosition();

        // Calculate bounding rectangle of the placement target taking into consideration whether target is on AppBar.
        // Placement mode is adjusted if target is on AppBar.
        static _Check_return_ HRESULT CalculatePlacementTargetBoundsPrivate(
            _Inout_ MajorPlacementMode* pPlacement,
            _In_ xaml::IFrameworkElement* pPlacementTarget,
            _Out_ wf::Rect* pPlacementTargetBounds,
            _Out_ BOOLEAN* pAllowFallbacks,
            DOUBLE bottomAppBarVerticalCorrection = 0.0);

        // Place control with list of fallbacks for a given placement mode.
        // Placement argument is adjusted to be the resulting placement mode.
        static _Check_return_ HRESULT CalculatePlacementPrivate(
            _Inout_ MajorPlacementMode* pMajorPlacement,
            PreferredJustification preferredJustification,
            BOOLEAN allowFallbacks,
            bool allowPresenterResizing,
            const wf::Rect& placementTargetBounds,
            const wf::Size& controlSize,
            const wf::Size& minControlSize,
            const wf::Size& maxControlSize,
            const wf::Rect& containerRect,
            bool isWindowed,
            _Out_ wf::Rect* pControlRect);


        // Helper method to set/clear m_tpPlacementTarget and maintain associated state and event handlers.
        _Check_return_ HRESULT SetPlacementTarget(_In_opt_ xaml::IFrameworkElement* pPlacementTarget);
    public:
       static _Check_return_ HRESULT GetPlacementTargetNoRef(
           _In_ CFlyoutBase* flyout,
           _Outptr_ CFrameworkElement** placementTarget);

    private:
        // Initializes popup and presenter on the first call, synchronizes properties, raises opening
        // event and opens the FlyoutBase's popup.
        _Check_return_ HRESULT Open();

        _Check_return_ HRESULT SetPopupLightDismissBehavior();

        // Validate and cache parameter for the duration of FlyoutBase showing.
        _Check_return_ HRESULT ValidateAndSetParameters(
            _In_ xaml::IFrameworkElement* pPlacementTarget,
            _Out_ bool *shouldOpen);

        // Check whether FlyoutBase can be opened.  If not, close currently open and queue for opening.
        _Check_return_ HRESULT CheckAndHandleOpenFlyout(_Out_ bool* shouldOpen);

        // Position popup accordingly.  Presenter size must be known when calling this method.
        _Check_return_ HRESULT PerformPlacement(
            wf::Size presenterSize,
            DOUBLE bottomAppBarVerticalCorrection = 0.0,
            bool preparePopupTheme = true);

        // Returns the presenter size.
        _Check_return_ HRESULT GetPresenterSize(
            _Out_ wf::Size* value);

        // Sets common properties on presenter control, applies template and attaches event handlers.
        _Check_return_ HRESULT PreparePresenter(
            _In_ xaml_controls::IControl* pPresenter);

        // Forwards select property values set on the flyout to the presenter.
        _Check_return_ HRESULT ForwardTargetPropertiesToPresenter();

        _Check_return_ HRESULT ForwardSystemBackdropToPopup();

        // Sets offsets on entrance / exit transition to match placement mode.
        _Check_return_ HRESULT SetTransitionParameters(
            _In_ xaml_animation::ITransition* pTransition,
            MajorPlacementMode placementMode);

        _Check_return_ HRESULT OnPresenterSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT OnPresenterLoaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPresenterUnloaded(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnPopupLostFocus(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnRootVisualPointerMoved(
            _In_ IInspectable* pSender,
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

        _Check_return_ HRESULT AddRootVisualPointerMovedHandler();
        _Check_return_ HRESULT RemoveRootVisualPointerMovedHandler();

        // Raising events.
        _Check_return_ HRESULT RaiseOpeningEvent();
        _Check_return_ HRESULT RaiseOpenedEvent();
        _Check_return_ HRESULT RaiseClosedEvent();

        // Private interface implementation
        _Check_return_ HRESULT UpdateTargetPosition(wf::Rect availableWindowRect, wf::Size presenterSize, _Out_ wf::Rect* presenterRect);

        // Return the appropriate metadata depending on whether this flyout is a child
        // flyout or not.  For child flyouts, it returns the metadata object for its
        // parent.  For root flyouts, it returns the global metadata from DXamlCore.
        _Check_return_ HRESULT GetFlyoutMetadata(_Out_ FlyoutMetadata** metadata);

        static bool GetIsAncestorOfTraverseThroughPopups(_In_opt_ CUIElement* pParentElem, _In_opt_ CUIElement* pMaybeDescendant);

        _Check_return_ HRESULT ConfigurePopupOverlay();

        static _Check_return_ HRESULT FindParentFlyoutFromElement(_In_ xaml::IFrameworkElement* element, _Out_ FlyoutBase** parentFlyout);

        _Check_return_ HRESULT UpdateStateToShowMode(xaml_primitives::FlyoutShowMode showMode);

        _Check_return_ HRESULT GetEffectivePlacement(_Out_ xaml_primitives::FlyoutPlacementMode* effectivePlacement);

    private:
        static const xaml_primitives::FlyoutPlacementMode g_defaultPlacementMode;
        static const DOUBLE g_entranceThemeOffset;

        static const WCHAR c_visualStateLandscape[];
        static const WCHAR c_visualStatePortrait[];

        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epPresenterSizeChangedHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epPresenterLoadedHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epPresenterUnloadedHandler;
        ctl::EventPtr<UIElementLostFocusEventCallback> m_epPopupLostFocusHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_epPlacementTargetUnloadedHandler;

        // Cached bounds of placement target. Used in cases where the placement target leaves the visual
        // tree before the flyout is displayed, for example, if the placement target was inside another flyout.
        wf::Rect m_placementTargetBounds;

        // Cached result of a call to CalculatePlacementTargetBoundsPrivate indicating whether placement fallback
        // should be allowed.
        BOOLEAN m_allowPlacementFallbacks;

        // Indicates if presenter has been resized (not due to IHM showing) to fit and resize event should be ignored.
        bool m_presenterResized;

        // Allows us to remember the height of the InputPane even after it has dismissed.
        DOUBLE m_cachedInputPaneHeight;

        // Indicates if the flyout should use the PickerFlyoutThemeTransition.
        BOOLEAN m_usePickerFlyoutTheme;

        // Placement target and placement mode saved while the flyout is open
        TrackerPtr<xaml::IFrameworkElement> m_tpPlacementTarget;
        MajorPlacementMode m_majorPlacementMode;
        PreferredJustification m_preferredJustification = PreferredJustification::Center;

        TrackerPtr<xaml_controls::IControl> m_tpPresenter;
        TrackerPtr<xaml_animation::ITransition> m_tpThemeTransition;

        TrackerPtr<xaml_primitives::IFlyoutBase> m_tpParentFlyout;
        ctl::ComPtr<FlyoutMetadata> m_childFlyoutMetadata;

        // Set the target position to show the flyout at the specified position
        bool m_isTargetPositionSet;

        // when there is not enough space to place the presenter, we'll resize it. default is true.
        bool m_allowPresenterResizing;

        // Indicates if we've overridden the value of the FlyoutPresenter's requested theme.
        bool m_isFlyoutPresenterRequestedThemeOverridden;

        wf::Point m_targetPoint;
        wf::Rect m_exclusionRect;
        DirectUI::InputDeviceType m_inputDeviceTypeUsedToOpen;

        // Old values of the target position properties to tell whether we should no-op
        // upon being told to re-show a flyout.
        bool m_wasTargetPositionSet{ false };
        wf::Point m_lastTargetPoint{};

        BOOLEAN m_isLightDismissOverlayEnabled;

        bool m_shouldTakeFocus{ true };
        bool m_shouldHideIfPointerMovesAway{ false };
        bool m_shouldOverlayPassThroughAllInput{ false };
        bool m_ownsOverlayInputPassThroughElement{ false };
        bool m_isPositionedForDateTimePicker{ false };

        bool m_hasPlacementOverride{ false };
        xaml_primitives::FlyoutPlacementMode m_placementOverride{ xaml_primitives::FlyoutPlacementMode_Top };

        EventRegistrationToken m_rootVisualPointerMovedToken = {};

        ctl::WeakRefPtr m_wrRootVisual;

        bool m_openingCanceled{ false };
    };
}
