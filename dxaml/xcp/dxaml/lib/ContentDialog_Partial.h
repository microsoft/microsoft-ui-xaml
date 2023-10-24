// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DeferralManager.h"
#include "ContentDialog.g.h"

namespace DirectUI
{
    class ContentDialogButtonClickDeferral;
    class ContentDialogClosingDeferral;

    extern __declspec(selectany) const WCHAR ContentDialogShowAsyncOperationName[] = L"Windows.Foundation.IAsyncOperation`1<Microsoft.UI.Xaml.Controls.ContentDialogResult> Microsoft.UI.Xaml.Controls.ContentDialog.ShowAsync";

    class ContentDialogShowAsyncOperation :
        public DXamlAsyncBaseImpl<
        wf::IAsyncOperationCompletedHandler<xaml_controls::ContentDialogResult>,
        wf::IAsyncOperation<xaml_controls::ContentDialogResult>,
        Microsoft::WRL::AsyncCausalityOptions<ContentDialogShowAsyncOperationName >>
    {
        InspectableClass(wf::IAsyncOperation<xaml_controls::ContentDialogResult>::z_get_rc_name_impl(), BaseTrust);

    public:
        ContentDialogShowAsyncOperation()
            : m_result(xaml_controls::ContentDialogResult::ContentDialogResult_None)
        {}

        IFACEMETHOD(put_Completed)(_In_ wf::IAsyncOperationCompletedHandler<xaml_controls::ContentDialogResult> *pCompletedHandler) override
        {
            return __super::PutOnComplete(pCompletedHandler);
        }

        IFACEMETHOD(get_Completed)(_Outptr_result_maybenull_ wf::IAsyncOperationCompletedHandler<xaml_controls::ContentDialogResult> **ppCompletedHandler) override
        {
            return __super::GetOnComplete(ppCompletedHandler);
        }

        IFACEMETHOD(Cancel)() override
        {
            HRESULT hr = S_OK;
            IFC(__super::Cancel());

            if (m_spOwner)
            {
                IFC(m_spOwner->Hide());
            }

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT StartOperation(_In_ xaml_controls::IContentDialog* pOwner)
        {
            HRESULT hr = S_OK;

            ASSERT(!m_spOwner, "StartOperation should never be called on an operation that already has an owner.");
            m_spOwner = pOwner;
            IFC(DXamlAsyncBaseImpl::StartOperation());

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT
            SetResults(_In_ xaml_controls::ContentDialogResult result)
        {
                m_result = result;
                RRETURN(S_OK);
            }

        IFACEMETHOD(GetResults)(_Out_ xaml_controls::ContentDialogResult* pResult)
        {
            *pResult = m_result;
            RRETURN(S_OK);
        }

        void CoreFireCompletion()
        {
            m_spOwner.Reset();
            CoreFireCompletionImpl();
        }

    private:
        xaml_controls::ContentDialogResult m_result;
        ctl::ComPtr<xaml_controls::IContentDialog> m_spOwner;
    };

    PARTIAL_CLASS(ContentDialog)
    {
        friend class ContentDialogMetadata;

    public:
        ContentDialog() = default;

        ~ContentDialog() override;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size arrangeSize,
            _Out_ wf::Size* returnValue) override;

        _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions) override;

        _Check_return_ HRESULT GetDialogHeight(
            _Out_ DOUBLE* value)
        {
            HRESULT hr = S_OK;
            DOUBLE backgroundHeight = 0.0;

            if (m_tpBackgroundElementPart)
            {
                ctl::ComPtr<IFrameworkElement> spBackgroundAsFE;
                IFC(m_tpBackgroundElementPart.As(&spBackgroundAsFE));
                IFC(spBackgroundAsFE->get_ActualHeight(&backgroundHeight));
            }

            *value = backgroundHeight;

        Cleanup:
            RRETURN(hr);
        }

        _Check_return_ HRESULT HideImpl();

        _Check_return_ HRESULT ShowAsyncImpl(_Outptr_ wf::IAsyncOperation<xaml_controls::ContentDialogResult>** returnValue);

        _Check_return_ HRESULT ShowAsyncWithPlacementImpl(
            xaml_controls::ContentDialogPlacement placement,
            _Outptr_ wf::IAsyncOperation<xaml_controls::ContentDialogResult>** returnValue);

        _Check_return_ HRESULT OnBackButtonPressedImpl(_Out_ BOOLEAN* handled);

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        // React to the IHM (touch keyboard) showing or hiding by repositioning
        // the ContentDialog or shifting its command buttons.
        _Check_return_ HRESULT NotifyInputPaneStateChange(
            _In_ InputPaneState inputPaneState,
            _In_ XRECTF inputPaneBounds);

        // For testing purposes only. Invoked by IXamlTestHooks::SimulateRegionsForContentDialog implementation.
        void SimulateRegionsForContentDialog();

        _Check_return_ HRESULT put_XamlRootImpl(_In_ xaml::IXamlRoot* pValue) override;

    private:
        typedef CEventSource<
            wf::ITypedEventHandler<xaml_controls::ContentDialog*, xaml_controls::ContentDialogButtonClickEventArgs*>,
            xaml_controls::IContentDialog, xaml_controls::IContentDialogButtonClickEventArgs
        > CommandClickEventSourceType;

        // When ShowAsync is called we determine if the control is
        // currently in the visual tree. This causes the layout to
        // occur differently.
        enum class PlacementMode
        {
            Undetermined,
            EntireControlInPopup,
            TransplantedRootInPopup,
            InPlace,
        };

        enum class FullMode
        {
            Undetermined,
            Partial,
            Full
        };

        enum class TemplateVersion
        {
            Unsupported = 0,
            PhoneBlue = 1,
            Threshold = 2,
            Redstone2 = 3,
            Redstone3 = 4
        };

        _Check_return_ HRESULT OnPopupOpened(
            _In_ IInspectable* pSender,
            _In_ IInspectable* pArgs);

        _Check_return_ HRESULT OnCommandButtonClicked(
            CommandClickEventSourceType* const clickEventSource,
            const ctl::ComPtr<xaml_input::ICommand>& command,
            const ctl::ComPtr<IInspectable>& commandParameter,
            xaml_controls::ContentDialogResult result
            );

        _Check_return_ HRESULT OnWindowActivated(
            _In_ IInspectable* sender,
            _In_ xaml::IWindowActivatedEventArgs* args);

        _Check_return_ HRESULT OnFinishedClosing();

        // User callable methods
        _Check_return_ HRESULT HideInternal(_In_ xaml_controls::ContentDialogResult result);

        _Check_return_ HRESULT HideAfterDeferralWorker(bool isCanceled, xaml_controls::ContentDialogResult result);

        // Primary control methods
        _Check_return_ HRESULT HostDialogWithinPopup(bool wasSmokeLayerFoundAsTemplatePart);
        _Check_return_ HRESULT PreparePopup(bool wasSmokeLayerFoundAsTemplatePart);
        _Check_return_ HRESULT PrepareSmokeLayer();
        _Check_return_ HRESULT CreateSmokeLayerBrush(_Outptr_ xaml_media::IBrush** brush);
        _Check_return_ HRESULT DeferredOpenPopup();

        _Check_return_ HRESULT PrepareContent();
        _Check_return_ HRESULT ResetContentProperties();

        // Helpers
        _Check_return_ HRESULT GetButtonHelper(xaml_controls::ContentDialogButton buttonType, _Outptr_ xaml_primitives::IButtonBase** button);
        _Check_return_ HRESULT GetDefaultButtonHelper(_Outptr_ xaml_primitives::IButtonBase** button);
        _Check_return_ HRESULT BuildAndConfigureButtons() noexcept;
        _Check_return_ HRESULT CreateButton(_In_ HSTRING text, _Outptr_ xaml_primitives::IButtonBase** ppButton) const;
        _Check_return_ HRESULT PopulateButtonContainer(
            _In_ const ctl::ComPtr<xaml_primitives::IButtonBase>& primaryButton,
            _In_ const ctl::ComPtr<xaml_primitives::IButtonBase>& secondaryButton
            );

        _Check_return_ HRESULT AttachButtonEvents();
        _Check_return_ HRESULT DetachButtonEvents();

        _Check_return_ HRESULT SizeAndPositionContentInPopup();

        _Check_return_ HRESULT UpdateTitleSpaceVisibility();
        _Check_return_ HRESULT EnsureDeferralManagers();
        _Check_return_ HRESULT ResetAndPrepareContent();
        _Check_return_ HRESULT DetachEventHandlers();
        _Check_return_ HRESULT AttachEventHandlersForOpenDialog();
        _Check_return_ HRESULT DetachEventHandlersForOpenDialog();

        _Check_return_ HRESULT OnPopupChildUnloaded(
            _In_ IInspectable* sender,
            _In_ xaml::IRoutedEventArgs* args);

        _Check_return_ HRESULT OnLayoutRootLoaded(
            _In_ IInspectable* sender,
            _In_ xaml::IRoutedEventArgs* args);

        _Check_return_ HRESULT OnLayoutRootKeyDown(
            _In_ IInspectable* sender,
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        _Check_return_ HRESULT OnLayoutRootKeyUp(
            _In_ IInspectable* sender,
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        _Check_return_ HRESULT OnLayoutRootProcessKeyboardAccelerators(
            _In_ IUIElement* pSender,
            _In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs);

        _Check_return_ HRESULT OnXamlRootChanged(
            _In_ xaml::IXamlRoot* sender,
            _In_ xaml::IXamlRootChangedEventArgs* args);

        _Check_return_ HRESULT OnDialogSizeChanged(
            _In_ IInspectable* sender,
            _In_ xaml::ISizeChangedEventArgs* args);

        _Check_return_ HRESULT OnDialogShowingStateChanged(
            _In_ IInspectable* sender,
            _In_ xaml::IVisualStateChangedEventArgs* args);

        _Check_return_ HRESULT ProcessLayoutRootKey(
            bool isKeyDown,
            _In_ xaml_input::IKeyRoutedEventArgs* args);

        _Check_return_ HRESULT ShouldFireClosing(_Out_ bool* doFire);

        _Check_return_ HRESULT SetPopupAutomationProperties();

        _Check_return_ HRESULT SetInitialFocusElement();

        _Check_return_ HRESULT ExecuteCloseAction();

        _Check_return_ HRESULT AdjustVisualStateForInputPane();

        _Check_return_ HRESULT CreateStoryboardForLayoutAdjustmentsForInputPane(
            xaml::Thickness layoutRootPadding,
            xaml_controls::ScrollBarVisibility contentVerticalScrollBarVisiblity,
            bool setDialogVerticalAlignment,
            xaml::VerticalAlignment dialogVerticalAlignment,
            _Out_ xaml_animation::IStoryboard** storyboard);

        void DetermineTemplateVersion();

        _Check_return_ HRESULT RaiseOpenedEvent();
        _Check_return_ HRESULT RaiseClosedEvent(xaml_controls::ContentDialogResult result);

        _Check_return_ HRESULT GetDialogInnerMargin(
            _In_ wf::Rect adjustedLayoutBounds,
            _Out_ xaml::Thickness* innerMargin);
        _Check_return_ HRESULT GetFocusedElementPosition(
            _Out_ wf::Point* focusedPosition);

        _Check_return_ HRESULT SetButtonPropertiesFromCommand(xaml_controls::ContentDialogButton buttonType, _In_opt_ xaml_input::ICommand* oldCommand = nullptr);
        _Check_return_ HRESULT UpdateCanDragStatusWindowChrome(bool dragEnabled);

        _Check_return_ HRESULT DiscardPopup();
        
        TrackerPtr<wf::IAsyncOperation<xaml_controls::ContentDialogResult>> m_tpCurrentAsyncOperation;
        TrackerPtr<xaml_primitives::IPopup> m_tpPopup;
        TrackerPtr<xaml_primitives::IPopup> m_tpSmokeLayerPopup;
        TrackerPtr<xaml::IFrameworkElement> m_tpSmokeLayer;
        TrackerPtr<xaml::IVisualStateGroup> m_tpDialogShowingStates;

        ctl::ComPtr<DeferralManager<ContentDialogClosingDeferral>> m_spClosingDeferralManager;
        ctl::ComPtr<DeferralManager<ContentDialogButtonClickDeferral>> m_spButtonClickDeferralManager;

        ctl::EventPtr<ButtonBaseClickEventCallback> m_epPrimaryButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epSecondaryButtonClickHandler;
        ctl::EventPtr<ButtonBaseClickEventCallback> m_epCloseButtonClickHandler;
        ctl::EventPtr<WindowActivatedEventCallback> m_windowActivatedHandler;
        ctl::EventPtr<UIElementPointerReleasedEventCallback> m_epLayoutRootPointerReleasedHandler;
        ctl::EventPtr<FrameworkElementLoadedEventCallback> m_epLayoutRootLoadedHandler;
        ctl::EventPtr<UIElementKeyDownEventCallback> m_epLayoutRootKeyDownHandler;
        ctl::EventPtr<UIElementKeyUpEventCallback> m_epLayoutRootKeyUpHandler;
        ctl::EventPtr<UIElementGotFocusEventCallback> m_epLayoutRootGotFocusHandler;
        ctl::EventPtr<UIElementProcessKeyboardAcceleratorsEventCallback> m_epLayoutRootProcessKeyboardAcceleratorsHandler;
        ctl::EventPtr<DisplayInformationOrientationChangedCallback> m_epOrientationChangedHandler;
        ctl::EventPtr<XamlRootChangedEventCallback> m_xamlRootChangedEventHandler;
        ctl::EventPtr<PopupOpenedEventCallback> m_popupOpenedHandler;
        ctl::EventPtr<FrameworkElementUnloadedEventCallback> m_popupChildUnloadedEventHandler;
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_dialogSizeChangedHandler;
        ctl::EventPtr<VisualStateGroupCurrentStateChangedEventCallback> m_dialogShowingStateChangedEventHandler;

        ctl::WeakRefPtr m_spFocusedElementBeforeContentDialogShows;

        TemplateVersion m_templateVersion{ TemplateVersion::Unsupported };

        PlacementMode m_placementMode{ PlacementMode::Undetermined };
        bool m_isLayoutRootTransplanted{ false };
        bool m_hideInProgress{ false };
        bool m_hasPreparedContent{ false };
        bool m_prepareSmokeLayerAndPopup{ false };
        double m_preShowStatusBarOpacity{ 0.0 };

        // Set once we've received the Popup.Opened event. If the async operation is
        // canceled before this point, we won't fire our Opening, Closing, and ClosedEvents.
        bool m_isShowing{ false };

        bool m_isProcessingKeyboardAccelerators = false;

        static ULONG z_ulUniqueAsyncActionId;
        // Flag to indicate that we shouldn't fine the closing event when hiding because we
        // want to hide without the ability to cancel it.
        bool m_skipClosingEventOnHide{ false };

        // Apply our layout adjustments using a storyboard so that we don't stomp over template or user
        // provided values.  When we stop the storyboard, it will restore the previous values.
        TrackerPtr<xaml_animation::IStoryboard> m_layoutAdjustmentsForInputPaneStoryboard;

        double m_dialogMinHeight = 0;

        // This only gets set from the SimulateRegionsForContentDialog() test hook.
        bool m_simulateRegions{ false };

        // Flag to indicate if the content dialog should be opened in windowed mode.
        bool m_isWindowed{ false };

        // True when m_tpSmokeLayer is a control template part (a FrameworkElement). False when it is a Rectangle created in code-behind.
        bool m_isSmokeLayerATemplatePart{ false };
    };
}
