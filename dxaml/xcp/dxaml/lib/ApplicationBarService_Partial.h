// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.ui.input.h>
#include "ApplicationBarService.g.h"
#include "JoltClasses.h"
#include "IApplicationBarService.h"
#include "TransitionCollection.g.h"

namespace DirectUI
{
    class AppBar;
    class Page;
    class Popup;
    class Grid;
    class Border;
    class EdgeUIThemeTransition;
    class SolidColorBrush;

    // Indicates which AppBar is priority for focus
    enum AppBarTabPriority
    {
       AppBarTabPriority_Top,
       AppBarTabPriority_Bottom
    };

    // Service class that provides the system implementation for managing
    // many app bars and the edgy event.
    PARTIAL_CLASS(ApplicationBarService),
        public IApplicationBarService
    {
        BEGIN_INTERFACE_MAP(ApplicationBarService, ApplicationBarServiceGenerated)
            INTERFACE_ENTRY(ApplicationBarService, IApplicationBarService)
        END_INTERFACE_MAP(ApplicationBarService, ApplicationBarServiceGenerated)

        protected:
            HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
            {
                if (InlineIsEqualGUID(iid, __uuidof(IApplicationBarService)))
                {
                    *ppObject = static_cast<IApplicationBarService*>(this);
                }
                else
                {
                    RRETURN(ApplicationBarServiceGenerated::QueryInterfaceImpl(iid, ppObject));
                }

                AddRefOuter();
                RRETURN(S_OK);
            }

            _Check_return_ HRESULT Initialize()  noexcept override;

        private:
            IWeakReference* m_pRootVisualWeakReference;
            std::list<IWeakReference*> m_ApplicationBars;

            // Edge Gesture Completed registration token
            EventRegistrationToken m_EdgeGestureCompletedEventToken;

            // Pointer pressed event tokens
            EventRegistrationToken m_DismissPressedEventToken;
            EventRegistrationToken m_DismissPointerReleasedEventToken;
            EventRegistrationToken m_DismissLayerRightTapToken;

            // Keyboard event token
            EventRegistrationToken m_KeyPressedEventToken;

            // activation token
            EventRegistrationToken m_activationToken;

            // Reference to current EdgeGesture object
            wui::IEdgeGesture* m_pEdgeGesture;

        public:
            //
            // Implementation
            //
            ApplicationBarService();
            ~ApplicationBarService() override;

            _Check_return_ HRESULT IsAppBarRegistered(_In_ AppBar* pAppBar, _Out_ BOOLEAN* pIsRegistered);

            //
            // IApplicationBarService
            //
            _Check_return_ HRESULT ClearCaches() override;

            _Check_return_ HRESULT RegisterApplicationBar(_In_ AppBar* pApplicationBar, _In_ AppBarMode mode) override;

            _Check_return_ HRESULT UnregisterApplicationBar(_In_ AppBar* pApplicationBar) override;

            _Check_return_ HRESULT OnBoundsChanged(_In_ BOOLEAN inputPaneChange = FALSE) override;

            _Check_return_ HRESULT OpenApplicationBar(_In_ AppBar* pAppBar, _In_ AppBarMode mode) override;

            _Check_return_ HRESULT CloseApplicationBar(_In_ AppBar* pAppBar, _In_ AppBarMode mode) override;

            _Check_return_ HRESULT HandleApplicationBarClosedDisplayModeChange(_In_ AppBar* pAppBar, _In_ AppBarMode mode) override;

            _Check_return_ HRESULT CloseAllNonStickyAppBars() override;
            _Check_return_ HRESULT CloseAllNonStickyAppBars(_Out_ bool* isAnyAppBarClosed) override;

            _Check_return_ HRESULT UpdateDismissLayer() override;

            _Check_return_ HRESULT ToggleApplicationBars() override;

            _Check_return_ HRESULT SaveCurrentFocusedElement(_In_ AppBar* pAppBar) override;

            _Check_return_ HRESULT FocusSavedElement(_In_ AppBar* pApplicationBar) override;

            _Check_return_ HRESULT ProcessTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateTabStopElement,
                _In_ BOOLEAN isBackward,
                _Outptr_result_maybenull_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden) override;

            _Check_return_ HRESULT FocusApplicationBar(
                _In_ AppBar* pAppBar,
                _In_ xaml::FocusState focusState) override;

            void SetFocusReturnState(_In_ xaml::FocusState focusState) override
                { m_focusReturnState = focusState; }

            void ResetFocusReturnState() override
                { m_focusReturnState = xaml::FocusState_Unfocused; }

            _Check_return_ HRESULT GetAppBarStatus(
                _Out_ bool* pIsTopOpen,
                _Out_ bool* pIsTopSticky,
                _Out_ XFLOAT* pTopWidth,
                _Out_ XFLOAT* pTopHeight,
                _Out_ bool* pIsBottomOpen,
                _Out_ bool* pIsBottomSticky,
                _Out_ XFLOAT* pBottomWidth,
                _Out_ XFLOAT* pBottomHeight) override;

            _Check_return_ HRESULT ProcessToggleApplicationBarsFromMouseRightTapped() override;

            _Check_return_ HRESULT GetTopAndBottomAppBars(
                _Outptr_ AppBar** ppTopAppBar,
                _Outptr_ AppBar** ppBottomAppBar) override;

            _Check_return_ HRESULT GetTopAndBottomOpenAppBars(
                _Outptr_ AppBar** ppTopAppBar,
                _Outptr_ AppBar** ppBottomAppBar,
                _Out_ BOOLEAN* pIsAnyLightDismiss) override;

            _Check_return_ HRESULT GetFirstFocusableElementFromAppBars(
                _In_opt_ AppBar* pTopAppBar,
                _In_opt_ AppBar* pBottomAppBar,
                _In_ AppBarTabPriority tabPriority,
                _In_ BOOLEAN startFromEnd,
                _Outptr_result_maybenull_ CDependencyObject **ppNewTabStop) override;

       private:
            _Check_return_ HRESULT CleanupOpenEventHooks();
            _Check_return_ HRESULT CleanupWindowActivatedEventHook();

            _Check_return_ HRESULT TryGetBounds(_Out_ BOOLEAN* boundsChanged);

            _Check_return_ HRESULT ShouldDismissNonStickyAppBars(_Out_ BOOLEAN* shouldDismiss);

            _Check_return_ HRESULT EvaluatePopupState();

            _Check_return_ HRESULT AddApplicationBarToVisualTree(_In_ AppBar* pAppBar, _In_ AppBarMode mode);
            _Check_return_ HRESULT RemoveApplicationBarFromVisualTree(_In_ AppBar* pAppBar, _In_ AppBarMode mode);

            _Check_return_ HRESULT OnDismissLayerPressed(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnDismissLayerPointerReleased(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnDismissLayerRightTapped(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IRightTappedRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnEdgeGestureCompleted(
                _In_ wui::IEdgeGesture *pSender,
                _In_ wui::IEdgeGestureEventArgs *pArgs);

            _Check_return_ HRESULT OnWindowActivated(
                _In_ IInspectable* pSender,
                _In_ xaml::IWindowActivatedEventArgs* pArgs);

            _Check_return_ HRESULT OnAppBarUnloaded(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            static _Check_return_ HRESULT GetKeyboardModifiers(
                _Out_ wsy::VirtualKeyModifiers* pModifierKeys);

            bool GetShouldEnterAppBar(
                  _In_ CDependencyObject *pFocusedElement,
                  _In_ BOOLEAN shiftPressed);

            _Check_return_ HRESULT GetShouldExitAppBar(
                 _In_ AppBar* pAppBar,
                 _In_ CDependencyObject* pFocusedElement,
                 _In_ BOOLEAN shiftPressed,
                 _Out_ BOOLEAN* pShouldExitAppBar);


            _Check_return_ HRESULT GetAppBarStatusInternal(
                _Out_ bool* pIsTopOpen,
                _Out_ bool* pIsTopSticky,
                _Out_ XFLOAT* pTopWidth,
                _Out_ XFLOAT* pTopHeight,
                _Out_ bool* pIsBottomOpen,
                _Out_ bool* pIsBottomSticky,
                _Out_ XFLOAT* pBottomWidth,
                _Out_ XFLOAT* pBottomHeight);

            _Check_return_ HRESULT SetAppBarOwnerPropertiesOnHost(_In_ Border* pAppBarHost);
            _Check_return_ HRESULT ClearAppBarOwnerPropertiesOnHost(_In_ Border* pAppBarHost);

            _Check_return_ HRESULT GetTopAndBottomAppBars(
                _In_ bool openAppBarsOnly,
                _Outptr_ AppBar** ppTopAppBar,
                _Outptr_ AppBar** ppBottomAppBar,
                _Out_opt_ BOOLEAN* pIsAnyLightDismiss);

            _Check_return_ HRESULT ReevaluateIsOverlayVisible();
            _Check_return_ HRESULT CreateOverlayAnimations();
            _Check_return_ HRESULT UpdateTargetForOverlayAnimations();
            _Check_return_ HRESULT PlayOverlayOpeningAnimation();
            _Check_return_ HRESULT PlayOverlayClosingAnimation();

         private:
            // parent XamlRoot
            ctl::WeakRefPtr m_weakXamlRoot;
            
            // the host of the dismiss layer and the wrappers
            TrackerPtr<Popup> m_tpPopupHost;
            TrackerPtr<Grid> m_tpDismissLayer;

            // AppBarLightDismiss element which implment Invoke accessibility pattern (a child of m_tpDismissLayer with the same size)
            TrackerPtr<Grid> m_tpAccDismissLayer;
            // wrappers that will host the appbars. We need wrappers in order to be able to apply transitions
            TrackerPtr<Border> m_tpTopBarHost;
            TrackerPtr<Border> m_tpBottomBarHost;
            TrackerPtr<EdgeUIThemeTransition> m_tpBottomHostTransition;

            // transparent brush, used to toggle on and off the hittesting behavior of the grid
            TrackerPtr<SolidColorBrush> m_tpTransparentBrush;
            // map of unloading appbars. Needed to be able to unhook the unloaded event after we received it.
            std::map<xaml::IFrameworkElement*, EventRegistrationToken> m_unloadingAppbars;
            // cache the previous bounds that we used as our visual viewport
            wf::Rect m_bounds;
            // Holds a weak reference to the most recent element focused on the main content before any appbar
            // got opened.
            IWeakReference* m_pPreviousFocusedElementWeakRef;
            // After dismissing an appbar or pressing ESC on a sticky appbar focus returns to the previously focused element before appbar is
            // opened. We determine the focus state when returning the focus to the previously focus element on manner dismissing and hold this state
            // at this variable.
            xaml::FocusState m_focusReturnState;
            // The only case where top app bar should not get focus as soon as it opens is the case where
            // bottom app bar will get opened at the same time. This BOOLEAN should be false in that case. Otherwise
            // it should always be true.
            BOOLEAN m_shouldTopGetFocus;
            // Light dismiss layer and the popup host preserve their current state when this flag is set.
            BOOLEAN m_suspendLightDismissLayerState;
            // Focus appbars after all appbars which are loading have loaded so we can determine which appbar should receive focus.
            INT32 m_appBarsLoading;

            static const INT64 s_OpeningDurationMs = 467;
            static const INT64 s_ClosingDurationMs = 167;
            bool m_isOverlayVisible;
            TrackerPtr<xaml_animation::IStoryboard> m_overlayOpeningStoryboard;
            TrackerPtr<xaml_animation::IStoryboard> m_overlayClosingStoryboard;
            ctl::EventPtr<TimelineCompletedEventCallback> m_overlayClosingCompletedHandler;
            ctl::EventPtr<WindowActivatedEventCallback> m_windowActivatedHandler;
    };
}
