// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ToolTip.g.h"
#include "DXamlTypes.h"

#define TOOLTIP_TOLERANCE                   2.0     // Used in PlacementMode.Mouse positioning to avoid screen edges.
#define DEFAULT_KEYBOARD_OFFSET             12      // Default offset for automatic tooltips opened by keyboard.
#define DEFAULT_MOUSE_OFFSET                20      // Default offset for automatic tooltips opened by mouse.

#define DEFAULT_TOUCH_OFFSET                44      // Default offset for automatic tooltips opened by touch.
#define CONTEXT_MENU_HINT_VERTICAL_OFFSET   -5      // The hint is slightly above center in the vertical axis.

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    // Represents the ToolTip control.
    PARTIAL_CLASS(ToolTip)
    {
        friend class DxamlCoreTestHooks;

        private:
            ctl::WeakRefPtr m_wrOwner;
            ctl::WeakRefPtr m_wrContainer;
            ctl::WeakRefPtr m_wrTargetOverride;
            ctl::WeakRefPtr m_wrPopup;
            xaml_primitives::PlacementMode* m_pToolTipServicePlacementModeOverride;
            BOOLEAN m_bIsPopupPositioned;
            BOOLEAN m_bClosing;
            BOOLEAN m_bIsOpenAsAutomaticToolTip;
            BOOLEAN m_bCallPerformPlacementAtNextPopupOpen;

            //
            // Bug 19931042: Hyperlink hover shows white box
            //
            // Opening a ToolTip is an asynchronous process, and we start the fade in transition via VSM after getting
            // the Popup.Opened event on some later frame. The OnPopupOpened event handler depends on other state flags to
            // make the VSM state transitions. Specifically, it needs m_bCallPerformPlacementAtNextPopupOpen == true to
            // call PerformPlacement which does the state transition, and PerformPlacement requires !m_bIsPopupPositioned
            // to do the state transition. Rapidly opening and closing the ToolTip can delay the Popup.Opened event enough
            // that these flags get overwritten by the time the first Popup.Opened arrives, at which point we've already
            // set m_bIsPopupPositioned and skip doing the state transition to "Opened". Subsequent Popup.Opened events
            // get ignored because we already cleared the m_bCallPerformPlacementAtNextPopupOpen flag when responding to
            // that first Popup.Opened event. We're then left in the "Closed" state and the ToolTip content never fades in.
            //
            // This counter tracks the number of times we've called CPopup::Open without receiving its Popup.Opened event.
            // If this counter is ever greater than 1, it means we've since closed the ToolTip and reopened it, and we
            // don't do anything until we receive that final Popup.Opened event. At that point we'll be in a consistent
            // state and we'll be able to make the transition to the "Opened" state.
            //
            int m_pendingPopupOpenEventCount = 0;

            static constexpr UINT m_mousePlacementVerticalOffset = 11;  // Mouse placement vertical offset

        public:
            EventRegistrationToken m_ownerPointerEnteredToken{};
            EventRegistrationToken m_ownerPointerExitedToken{};
            EventRegistrationToken m_ownerPointerCaptureLostToken{};
            EventRegistrationToken m_ownerPointerCanceledToken{};
            EventRegistrationToken m_ownerGotFocusToken{};
            EventRegistrationToken m_ownerLostFocusToken{};
            BOOLEAN m_bInputEventsHookedUp{};
            AutomaticToolTipInputMode m_inputMode{};
            BOOLEAN m_isSliderThumbToolTip{};

            // Top - Default PlacementMode for ToolTips in Jupiter.
            static const xaml_primitives::PlacementMode DefaultPlacementMode =
                xaml_primitives::PlacementMode_Top;

            _Check_return_ HRESULT SetOwner(
                _In_opt_ xaml::IDependencyObject* pNewOwner);

            _Check_return_ HRESULT GetContainer(
                _Outptr_ xaml::IFrameworkElement** pContainer);

            _Check_return_ HRESULT SetContainer(
                _In_opt_ xaml::IFrameworkElement* pNewContainer);

            _Check_return_ HRESULT OnRootVisualSizeChanged();

            // Sets the location of the ToolTip's Popup.
            //
            // Slider "Disambiguation UI" ToolTips need special handling since they need to remain centered
            // over the sliding Thumb, which has not yet rendered in its new position.  Therefore, we pass
            // the new target rect to handle this case.
            _Check_return_ HRESULT PerformPlacement(
                _In_opt_ RECT* pTargetRect = nullptr);

            _Check_return_ HRESULT SetPlacementOverrides(
                _In_ IFrameworkElement* pInputTargetOverride);

            // Removes the "automatic" flag and clears associated fields.
            //
            // For Slider, the Thumb ToolTip may be opened as an automatic ToolTip by pointer hover.  However, if
            // we click on the Thumb and start to drag, we don't want the ToolTip to disappear after several seconds.
            // Thus, we remove the automatic flag and keep the ToolTip open for Slider to handle.
            _Check_return_ HRESULT RemoveAutomaticStatusFromOpenToolTip();

            _Check_return_ HRESULT RepositionPopup();

            bool IsOpenAsAutomaticToolTip() const { return !!m_bIsOpenAsAutomaticToolTip; }

            _Check_return_ HRESULT HandlePointInSafeZone(
                _In_ const POINT& position);

            _Check_return_ HRESULT HandlePointInSafeZone(
                _In_ const wf::Point& point);

        protected:
            // Initializes a new instance of the ToolTip class.
            ToolTip();
            ~ToolTip() override;

            // Prepares object's state
            _Check_return_ HRESULT Initialize() override;

            // Handle the custom property changed event and call the OnPropertyChanged2 functions.
            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            _Check_return_ HRESULT PrepareState() override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(
                _Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

            // Apply a template to the ToolTip.
            IFACEMETHOD(OnApplyTemplate)() override;

            // IsEnabled property changed handler.
            _Check_return_ HRESULT OnIsEnabledChanged(
                _In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

            // Change to the correct visual state for the ToolTip.
            _Check_return_ HRESULT ChangeVisualState(
                // true to use transitions when updating the visual state, false
                // to snap directly to the new visual state.
                _In_ bool bUseTransitions) override;

        private:
            _Check_return_ HRESULT HookupParentPopup(
                _Outptr_ xaml_primitives::IPopup** ppPopup);

            _Check_return_ HRESULT OnIsOpenChanged(
                _In_ BOOLEAN bIsOpen);

            _Check_return_ HRESULT OnPlacementCriteriaChanged();

            _Check_return_ HRESULT OpenPopup();

            _Check_return_ HRESULT Close();

            // Handler for when the Popup is opened.
            _Check_return_ HRESULT OnPopupOpened(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            // Handler for when the Popup is closed.
            _Check_return_ HRESULT OnPopupClosed(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            // Handle the SizeChanged event.
            _Check_return_ HRESULT OnToolTipSizeChanged(
                _In_ IInspectable* pSender,
                _In_ xaml::ISizeChangedEventArgs* pArgs);

            _Check_return_ HRESULT OnOpened();

            _Check_return_ HRESULT OnClosed();

            _Check_return_ HRESULT PerformClipping(
                _In_ wf::Size size);

            // If we are in the process of animating to the Closed state, then closes the ToolTip's Popup.
            // Else, does nothing.
            _Check_return_ HRESULT ForceFinishClosing(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            // help method to get the target from placement override or placement target.
            _Check_return_ HRESULT GetTarget(_Outptr_result_maybenull_ IFrameworkElement** ppTarget);

            // Sets the location of the ToolTip's Popup within the Xaml window.
            _Check_return_ HRESULT PerformPlacementWithPopup(
                _In_opt_ RECT* pTargetRect = nullptr) noexcept;

            // Gets cached PointerPoint from XamlIslandRoot or CoreWindow
            wrl::ComPtr<ixp::IPointerPoint> GetCurrentPointFromRootOrCoreWindow(
                _In_ const ctl::ComPtr<IFrameworkElement>& spTarget);

            // PerformPlacementWithPopup Helpers
            _Check_return_ HRESULT PerformMousePlacementWithPopup(
                _In_opt_ RECT* dimentions, _In_ xaml_primitives::PlacementMode placement) noexcept;

            _Check_return_ HRESULT PerformNonMousePlacementWithPopup(
                _In_opt_ RECT* pTargetRect, _In_opt_ RECT* dimentions, _In_ xaml_primitives::PlacementMode placement) noexcept;

            _Check_return_ HRESULT CalculateTooltipClip(
                _In_opt_ RECT* toolTipRect, DOUBLE maxX, DOUBLE maxY) noexcept;

            // Sets the location of the ToolTip's Popup out of the Xaml window by using the windowed Popup.
            _Check_return_ HRESULT PerformPlacementWithWindowedPopup(
                _In_opt_ RECT* pTargetRect = nullptr);

            // Sets the mouse placement with the Windowed Popup
            _Check_return_ HRESULT PerformMousePlacementWithWindowedPopup(
                _In_ bool bIsRTL,
                _In_ wf::Rect availableMonitorRect,
                _In_ wf::Point currentPoint,
                _In_ DOUBLE tooltipActualWidth,
                _In_ DOUBLE tooltipActualHeight,
                _In_ DOUBLE horizontalOffset,
                _In_ DOUBLE verticalOffset,
                _In_ xaml_primitives::PlacementMode placement);

            // Sets the directional placement with the Windowed Popup
            _Check_return_ HRESULT PerformDirectionalPlacementWithWindowedPopup(
                _In_ xaml::IFrameworkElement* pTarget,
                _In_ bool bIsRTL,
                _In_ wf::Rect availableMonitorRect,
                _In_ wf::Point currentPoint,
                _In_ DOUBLE tooltipActualWidth,
                _In_ DOUBLE tooltipActualHeight,
                _In_ DOUBLE horizontalOffset,
                _In_ DOUBLE verticalOffset,
                _In_ xaml_primitives::PlacementMode placement,
                _In_opt_ RECT* pTargetRect = nullptr);

            // Convert the LTR point to RTL client point
            void ConvertToRTLPoint(
                _Inout_ wf::Point* pDipPoint,
                _In_ FLOAT ScreenWidth);

            _Check_return_ HRESULT MovePointToPointerToolTipShowPosition(
                _Inout_ wf::Point& point,
                xaml_primitives::PlacementMode placement);

            _Check_return_ HRESULT MovePointToPointerToolTipShowPosition(
                _Inout_ DOUBLE& left,
                _Inout_ DOUBLE& top,
                xaml_primitives::PlacementMode placement);

            _Check_return_ HRESULT GetPlacementRectInWindowCoordinates(
                _Out_ wf::Rect* placementRect);

            _Check_return_ HRESULT PerformPlacementInternal();
            
            static bool IsControlKeyOnly(
                _In_ wsy::VirtualKey key);

            _Check_return_ HRESULT HookupXamlIslandRoot();
            _Check_return_ HRESULT UnhookFromXamlIslandRoot();

            _Check_return_ HRESULT HookupOwnerLayoutChangedEvent();
            _Check_return_ HRESULT UnhookOwnerLayoutChangedEvent();
            bool IsOwnerPositionChanged();

            // Xaml Island, event is attached to XamlIslandRoot
            _Check_return_ HRESULT AddXamlIslandRootHandler(UIElement *rootElement);
            _Check_return_ HRESULT RemoveXamlIslandRootHandler();

            _Check_return_ HRESULT GetXamlIslandRootElement(
                _Outptr_ UIElement** contentRoot);

            _Check_return_ HRESULT UpdateOwnersBoundary();

            _Check_return_ HRESULT ForwardOwnerThemePropertyToToolTip();

            ctl::ComPtr<xaml_input::IPointerEventHandler> m_xamlIslandRootPointerMovedHandler{};
            ctl::ComPtr<xaml_input::IKeyEventHandler> m_xamlIslandRootKeyDownHandler{};
            ctl::ComPtr<xaml_input::IKeyEventHandler> m_xamlIslandRootKeyUpHandler{};

            EventRegistrationToken m_ownerLayoutUpdatedToken {};

            // only Ctrl Down, then Ctrl Up dismiss the ToolTip
            bool m_lastKeyDownIsControlOnly{ false };

            // When ToolTip owner position changed, we should dismiss the tooltip.
            XRECTF_RB m_ownerBounds {};

            bool m_isToolTipRequestedThemeOverridden{ false };
    };
}
