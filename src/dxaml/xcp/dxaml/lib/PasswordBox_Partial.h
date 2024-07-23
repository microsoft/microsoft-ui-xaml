// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PasswordBox.g.h"
#include "PasswordBoxPasswordChangingEventArgs.g.h"
#include "ContextMenuEventArgs.g.h"
#include <PasswordBox.h>

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    //---------------------------------------------------------------------------
    //
    //  The DirectUI password text control.
    //
    //---------------------------------------------------------------------------
    PARTIAL_CLASS(PasswordBox)
    {
    public:
        PasswordBox();

        _Check_return_ HRESULT get_InputScopeImpl(_Out_ xaml_input::IInputScope** ppValue);
        _Check_return_ HRESULT put_InputScopeImpl(_In_ xaml_input::IInputScope* pValue);

        // Event handler overrides.
        IFACEMETHOD(OnPointerEntered)(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        IFACEMETHOD(OnPointerPressed)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnPointerMoved)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnPointerReleased)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnPointerExited)(_In_ xaml_input::IPointerRoutedEventArgs* e) override;
        IFACEMETHOD(OnPointerCaptureLost)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnPointerCanceled)(_In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnDoubleTapped)(_In_ xaml_input::IDoubleTappedRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnHolding)(_In_ xaml_input::IHoldingRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnManipulationStarted)(_In_ xaml_input::IManipulationStartedRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnManipulationCompleted)(_In_ xaml_input::IManipulationCompletedRoutedEventArgs* e) override;
        IFACEMETHOD(OnKeyUp)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnKeyDown)(_In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnGotFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnLostFocus)(_In_ xaml::IRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnTapped)(_In_ xaml_input::ITappedRoutedEventArgs* pArgs) override;
        IFACEMETHOD(OnRightTapped)(_In_ xaml_input::IRightTappedRoutedEventArgs* pArgs) override;
        _Check_return_ IFACEMETHOD(OnInheritedPropertyChanged)(_In_ IInspectable* pArgs) override;

        _Check_return_ HRESULT OnContextRequestedImpl(_In_ xaml_input::IContextRequestedEventArgs * pArgs) override;

        _Check_return_ HRESULT OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs) final;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);
        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        bool IsRevealButtonPressed() const { return m_revealButtonPressed;}
        _Check_return_ HRESULT SetRevealButtonPressed( bool userPressed);

        bool IsRevealButtonCheckbox() const { return m_revealButtonCheckBox; }
        void SetRevealButtoneIsCheckBox() { m_revealButtonCheckBox = true; }

        _Check_return_ HRESULT PasteFromClipboardImpl();

        // Callback from core, adds an IsPressed property change listener on the template's RevealButton.
        static _Check_return_ HRESULT OnApplyTemplateHandler(_In_ CPasswordBox* pNativePasswordBox);
        static _Check_return_ HRESULT ShowPlaceholderTextHandler(_In_ CDependencyObject *pNativePasswordBox, _In_ bool isEnabled);

        static _Check_return_ HRESULT OnPasswordChangingHandler(_In_ CPasswordBox* const passwordBox, _In_ bool textChanged);
        static _Check_return_ HRESULT OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativePasswordBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);
        static _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativePasswordBox);

        ctl::ComPtr<PasswordBoxPasswordChangingEventArgs> m_passwordChangingEventArgs;

        _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

    protected:
        ~PasswordBox() override;

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

    private:

        class RevealButtonPropertyChangedHandler : public ctl::implements<IDPChangedEventHandler>
        {
        public:
            RevealButtonPropertyChangedHandler(_In_ PasswordBox *pPasswordBox);

            IFACEMETHOD(Invoke)(
                _In_ xaml::IDependencyObject* pSender,
                _In_ const CDependencyProperty* pDP) override;

        private:
            PasswordBox *m_pPasswordBoxNoRef;
        };

        void SetRevealButton(_In_opt_ xaml_primitives::IButtonBase* const pRevealButton)
        {
            SetPtrValue(m_tpRevealButton, pRevealButton);
        }

        void SetRevealButtonPropertyChangedHandler(_In_opt_ RevealButtonPropertyChangedHandler* const handler)
        {
            SetPtrValue(m_tpRevealButtonPropertyChangedHandler, handler);
        }

        // "Header" template part

        // Holds a reference to the Header Content Presenter so we can collapse it when not in use
        TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;
        TrackerPtr<xaml::IUIElement> m_requiredHeaderPresenter;

        TrackerPtr<xaml::IUIElement> m_tpPlaceholderTextPresenter;

        // We dont want OnPropertyChanged to trigger UpdateHeaderPresenterVisibility while the template is being loaded
        // so we use this as a flag while initializing the object
        BOOLEAN m_isInitializing;

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();
        _Check_return_ HRESULT UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled);

        void SetPlaceholderTextPresenter(_In_ xaml::IUIElement* const pValue)
        {
            SetPtrValueWithQIOrNull(m_tpPlaceholderTextPresenter, pValue);
        }

        // "RevealButton" template part.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpRevealButton;

        // "RevealButton" template part property change listener delegate.
        TrackerPtr<RevealButtonPropertyChangedHandler> m_tpRevealButtonPropertyChangedHandler;

        // SizeChanged handler
        ctl::EventPtr<FrameworkElementSizeChangedEventCallback> m_epSizeChangedHandler;
        _Check_return_ HRESULT OnRevealButtonSizeChanged(
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        // "RevealButton" Checked handler
        ctl::EventPtr<ToggleButtonCheckedEventCallback> m_epRevealButtonCheckedHandler;
        _Check_return_ HRESULT OnRevealButtonChecked();

        // "RevealButton" unchecked event listener
        ctl::EventPtr<ToggleButtonUncheckedEventCallback> m_epRevealButtonUncheckedHandler;
        _Check_return_ HRESULT OnRevealButtonUnchecked();

        // "RevealButton" PointerReleased handler
        ctl::EventPtr<UIElementPointerReleasedEventCallback> m_epButtonPointerReleasedHandler;

        _Check_return_ HRESULT ReleaseTemplateParts();

        _Check_return_ HRESULT ValidateEvent(_In_ xaml::IRoutedEventArgs* pEventArgs, _Out_ bool* pIsValidEvent);

        bool m_revealButtonPressed;
        bool m_revealButtonCheckBox;
    };
}
