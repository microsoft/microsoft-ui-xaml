// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <FrameworkUdk/BackButtonIntegration.h>

#include "Popup.g.h"
#include <FrameworkUdk/CoreWindowIntegrationInterface.h>

namespace DirectUI
{
    // Represents the Popup control
    PARTIAL_CLASS(Popup), public ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener
    {
        BEGIN_INTERFACE_MAP(DesktopWindowXamlSource, PopupGenerated)
            INTERFACE_ENTRY(DesktopWindowXamlSource, ABI::Microsoft::Internal::FrameworkUdk::ICoreWindowPositionChangedListener)
        END_INTERFACE_MAP(DesktopWindowXamlSource, PopupGenerated)

        public:

            enum class MajorPlacementMode
            {
                Auto,
                Top,
                Bottom,
                Left,
                Right,
            };

            enum class PreferredJustification
            {
                Auto,
                HorizontalCenter,
                VerticalCenter,
                Top,
                Bottom,
                Left,
                Right
            };

            enum DismissalTriggerFlags : UINT32
            {
                None                    = 0x0,
                CoreLightDismiss        = 0x1,
                WindowSizeChange        = 0x2,
                WindowDeactivated       = 0x4,
                BackPress               = 0x8,
                All                     = 0xFFFFFFFF    // Note: This flag encompases all present and future flags, hence
                                                        // it is not equivilant to the OR of the current set of flags.
            };

        public:
            // Initializes a new instance of the Popup class.
            Popup();
            ~Popup() override;

            _Check_return_ HRESULT Initialize() override;

            _Check_return_ HRESULT GetChildrenCount(
                _Out_ INT* pnCount) override;

            _Check_return_ HRESULT GetChild(
                _In_ INT nChildIndex,
                _Outptr_ xaml::IDependencyObject** ppDO) override;

            _Check_return_ HRESULT GetSavedFocusState(
                _Out_ xaml::FocusState* pFocusState);

            // FocusManager GetNext/PreviousTabStopOverride
            _Check_return_ HRESULT GetNextTabStopOverride(
                _Outptr_ DependencyObject** ppNextTabStop) override;

            _Check_return_ HRESULT GetPreviousTabStopOverride(
                _Outptr_ DependencyObject** ppPreviousTabStop) override;

            // Programmatic call to light-dismiss.
            // If it was opened as a light-dismiss Popup, the Popup will close and restore
            // focus to the control that had focus when the Popup was opened using focusStateAfterClosing.
            // If it was opened as a normal Popup, the Popup will simply close.
            _Check_return_ HRESULT LightDismiss(
                _In_ xaml::FocusState focusStateAfterClosing);

            // Set exactly what triggers will cause this popup to be dismissed
            _Check_return_ HRESULT SetDismissalTriggerFlags(_In_ UINT32 flags);

            // callback function that is called when the core popup class "Close" function is called
            static _Check_return_ HRESULT OnClosed(_In_ CDependencyObject* nativePopup);

            _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* pReturnValue);

            // Clear the caches on the current window
            void ClearWindowCaches();

            _Check_return_ HRESULT SetDefaultAutomationName(_In_ HSTRING automationName);

            _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

            // Truncates the automation name down to 20 words if it's longer than that, or if
            // it has a newline character before that, in order to avoid giving a really long string
            // to AutomationProperties.Name which would then be read out in its entirety.
            static _Check_return_ HRESULT TruncateAutomationName(
                _Inout_ HSTRING *stringToTruncate);

            _Check_return_ HRESULT GetShouldUIAPeerExposeWindowPattern(_Out_ bool *shouldExposeWindowPattern);

            _Check_return_ HRESULT HookupWindowPositionChangedHandlerImpl();

            static _Check_return_ HRESULT HookupWindowPositionChangedHandler(_In_ CDependencyObject* nativePopup);

            static void OnHostWindowPositionChanged(_In_ CDependencyObject* nativePopup);

            static void OnIslandLostFocus(_In_ CDependencyObject* nativePopup);

            _Check_return_ HRESULT SetOwner(_In_opt_ xaml::IDependencyObject* const owner);

            bool IsWindowed();

            _Check_return_ HRESULT get_IsConstrainedToRootBoundsImpl(_Out_ BOOLEAN* pValue);

            // Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop implementation
            IFACEMETHOD(get_SystemBackdrop)(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush) override;
            IFACEMETHOD(put_SystemBackdrop)(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush) override;

        protected:
            void NotifyOfDataContextChange(_In_ const DataContextChangedParams& args) override;

            // Override OnCreateAutomationPeer()
            IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_result_maybenull_ xaml_automation_peers::IAutomationPeer** returnValue);
            IFACEMETHOD(OnDisconnectVisualChildren)() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        private:
            bool IsAssociatedWithXamlIsland() const;

            _Check_return_ HRESULT OnWindowActivated(
                _In_ IInspectable* pSender,
                _In_ xaml::IWindowActivatedEventArgs* pArgs);

            _Check_return_ HRESULT OnXamlRootChanged(
                _In_ xaml::IXamlRoot* pSender,
                _In_ xaml::IXamlRootChangedEventArgs* pArgs);

            STDMETHOD (OnCoreWindowPositionChanged)() override; //ICoreWindowPositionChangedListener

            void OnHostWindowPositionChangedImpl();

            void OnIslandLostFocusImpl();
            _Check_return_ HRESULT OnXamlLostFocus();

            _Check_return_ HRESULT GetTabStop(
                _In_ BOOLEAN isForward,
                _Outptr_ DependencyObject** ppPreviousTabStop);

            _Check_return_ HRESULT Close();

            _Check_return_ HRESULT CacheContentPositionInfo();

            _Check_return_ HRESULT ShouldDismiss(_In_ DismissalTriggerFlags reason, _Out_ BOOLEAN* returnValue);

            _Check_return_ HRESULT SetPositionFromPlacement(float xPlacementTargetAdjustment = 0.0f, float yPlacementTargetAdjustment = 0.0f);

            static void SetPositionFromMajorPlacementAndJustification(
                _Inout_ wf::Point* position,
                wf::Size const& childSize,
                wf::Rect const& targetBounds,
                xaml::FlowDirection flowDirection,
                Popup::MajorPlacementMode majorPlacement,
                Popup::PreferredJustification justification);

            static void FlipMajorPlacementAndJustificationIfOutOfBounds(
                _Inout_ Popup::MajorPlacementMode* majorPlacement,
                _Inout_ Popup::PreferredJustification* justification,
                wf::Point const& position,
                wf::Size const& childSize,
                wf::Rect const& availableRect,
                _Out_ bool* valueChanged);

            static Popup::MajorPlacementMode GetMajorPlacementFromPlacement(
                xaml_primitives::PopupPlacementMode placement,
                xaml::FlowDirection flowDirection);
            static Popup::PreferredJustification GetJustificationFromPlacement(
                xaml_primitives::PopupPlacementMode placement,
                xaml::FlowDirection flowDirection);
            static xaml_primitives::PopupPlacementMode GetPlacementFromMajorPlacementAndJustification(
                Popup::MajorPlacementMode majorPlacementMode,
                Popup::PreferredJustification justification,
                xaml::FlowDirection flowDirection);

            _Check_return_ HRESULT OnPlacementTargetLayoutUpdated(
                _In_ IInspectable* sender,
                _In_ IInspectable* args);
            _Check_return_ HRESULT OnChildSizeChanged(
                _In_ IInspectable* sender,
                _In_ xaml::ISizeChangedEventArgs* args);
            _Check_return_ HRESULT OnChildLoaded(
                _In_ IInspectable* sender,
                _In_ xaml::IRoutedEventArgs* args);

            _Check_return_ HRESULT RegisterForXamlRootChangedEvents();
            _Check_return_ HRESULT UnregisterForXamlRootChangedEvents();
            _Check_return_ HRESULT RegisterForWindowActivatedEvents();
            _Check_return_ HRESULT UnregisterForWindowActivatedEvents();

            void GetLayoutCycleWarningContext(
                _In_ CPopup* corePopup,
                _In_opt_ CPopup* placementTargetPopupCore,
                _In_ const std::wstring& prefix,
                _Inout_ std::vector<std::wstring>& warningInfo) const;

            static void GetLayoutCycleWarningContext(
                _In_ CPopup* corePopup,
                _In_ const std::wstring& prefix,
                _Inout_ std::vector<std::wstring>& warningInfo);

            // event handlers used for light-dismiss on screen orientation change.
            ctl::EventPtr<WindowActivatedEventCallback> m_windowActivatedHandler;
            ctl::EventPtr<XamlRootChangedEventCallback> m_xamlRootChangedEventHandler;

            ctl::WeakRefPtr m_wrOwner;

            // Previous size of the xaml root, used to detect orientation/size changes for light dismiss.
            FLOAT m_previousXamlRootWidth;
            FLOAT m_previousXamlRootHeight;

            // Previous position of the window, used to detect when we move.
            FLOAT m_previousWindowX;
            FLOAT m_previousWindowY;

            // Store the flags that determine what triggers dismissal of this popup
            UINT32 m_dismissalTriggerFlags;

            HSTRING m_defaultAutomationName;

            EventRegistrationToken m_placementTargetLayoutUpdatedToken{};
            EventRegistrationToken m_childSizeChangedToken{};
            EventRegistrationToken m_childLoadedToken{};
            ctl::WeakRefPtr m_wrOldChild;

            bool m_placementAndJustificationCalculated{ false };
            MajorPlacementMode m_calculatedMajorPlacement;
            PreferredJustification m_calculatedJustification;
    };
}
