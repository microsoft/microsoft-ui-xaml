// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBox.g.h"
#include "TextBoxTextChangingEventArgs.g.h"
#include "TextBoxBeforeTextChangingEventArgs.g.h"
#include "TextBoxSelectionChangingEventArgs.g.h"
#include "ContextMenuEventArgs.g.h"

#include <TextBox.h>

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    //---------------------------------------------------------------------------
    //
    //  The DirectUI editable plain text control.
    //
    //---------------------------------------------------------------------------
    PARTIAL_CLASS(TextBox)
    {
    public:
        TextBox();

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
        IFACEMETHOD(OnInheritedPropertyChanged)(_In_ IInspectable* pArgs) override;
        IFACEMETHOD(add_CandidateWindowBoundsChanged)(_In_ wf::ITypedEventHandler<xaml_controls::TextBox*, xaml_controls::CandidateWindowBoundsChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;

        _Check_return_ HRESULT OnContextRequestedImpl(_In_ xaml_input::IContextRequestedEventArgs * pArgs) override;

        _Check_return_ HRESULT OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs) final;

        // FrameworkElement overrides.
        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        // Callback from core, adds a Click event listener on the template's DeleteButton.
        static _Check_return_ HRESULT OnApplyTemplateHandler(_In_ CDependencyObject *pNativeTextBox);
        static _Check_return_ HRESULT ShowPlaceholderTextHandler(_In_ CDependencyObject *pNativeTextBox, _In_ bool isEnabled);
        static _Check_return_ HRESULT GetBringIntoViewOnFocusChange(_In_ CDependencyObject *pNativeTextBox, _Out_ bool *pBringIntoViewOnFocusChange);
        static _Check_return_ HRESULT OnTextChangingHandler(_In_ CTextBox* const pNativeTextBox, _In_ bool fTextChanged);
        static _Check_return_ HRESULT OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeTextBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);
        static _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeTextBox);

        static _Check_return_ HRESULT RaiseNative(
            _In_ ControlGenerated *pTextBoxBase,
            _In_ IInspectable* pArgs,
            _In_ KnownEventIndex nDelegate
            );

        _Check_return_ HRESULT get_IsDesktopPopupMenuEnabledImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT put_IsDesktopPopupMenuEnabledImpl(_In_ BOOLEAN value);

        _Check_return_ HRESULT UndoImpl();
        _Check_return_ HRESULT RedoImpl();
        _Check_return_ HRESULT PasteFromClipboardImpl();
        _Check_return_ HRESULT CopySelectionToClipboardImpl();
        _Check_return_ HRESULT CutSelectionToClipboardImpl();
        _Check_return_ HRESULT ClearUndoRedoHistoryImpl();

        _Check_return_ HRESULT ForceEditFocusLossImpl();

        _Check_return_ HRESULT GetLinguisticAlternativesAsyncImpl(_Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** ppReturnValue);

        static _Check_return_ HRESULT OnBeforeTextChangingHandler(_In_ CTextBox* const nativeTextBox, _In_ xstring_ptr* newString, _Out_ BOOLEAN* wasCanceled);
        static _Check_return_ HRESULT OnSelectionChangingHandler(
            _In_ CDependencyObject* const nativeTextBox,
            _In_ long selectionStart,
            _In_ long selectionLength,
            _Out_ BOOLEAN* wasCanceled);

        ctl::ComPtr<TextBoxTextChangingEventArgs> m_textChangingEventArgs;
        ctl::ComPtr<TextBoxBeforeTextChangingEventArgs> m_beforeTextChangingEventArgs;
        ctl::ComPtr<TextBoxSelectionChangingEventArgs> m_selectionChangingEventArgs;

        _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

    protected:

        ~TextBox() override;

        _Check_return_ HRESULT DisconnectFrameworkPeerCore() override;

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

    private:

        // "Header" and "Placeholder Text" template parts

        // Holds a reference to the Header Content Presenter so we can collapse it when not in use
        TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;
        TrackerPtr<xaml::IUIElement> m_requiredHeaderPresenter;
        TrackerPtr<xaml::IUIElement> m_tpPlaceholderTextPresenter;

        // We dont want OnPropertyChanged to trigger UpdateHeaderPresenterVisibility while the template is being loaded
        // so we use this as a flag while initializing the object
        BOOLEAN m_isInitializing;

        EventRegistrationToken m_storyboardCompletedToken;

        BOOLEAN m_isAnimatingHeight;

        _Check_return_ HRESULT OnHeightAnimationCompleted(IInspectable* pSender, IFrameworkElement* pSenderAsFE);

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();
        _Check_return_ HRESULT UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled);

        void SetPlaceholderTextPresenter(_In_ xaml::IUIElement* const pValue)
        {
            SetPtrValueWithQIOrNull(m_tpPlaceholderTextPresenter, pValue);
        }

        // "DeleteButton" template part.
        TrackerPtr<xaml_primitives::IButtonBase> m_tpDeleteButton;

        void SetDeleteButton(_In_ xaml::IDependencyObject* const pValue)
        {
            SetPtrValueWithQIOrNull(m_tpDeleteButton, pValue);
        }

        _Check_return_ HRESULT RaiseNative(_In_ IInspectable* pArgs, _In_ KnownEventIndex nDelegate);

        _Check_return_ HRESULT OnDeleteButtonClick(
            _In_ IInspectable *pSender,
            _In_ xaml::IRoutedEventArgs *pArgs
            );

        _Check_return_ HRESULT OnDeleteButtonSizeChanged(
            _In_ IInspectable *pSender,
            _In_ xaml::ISizeChangedEventArgs *pArgs
            );

        // Handle the SizeChanged event.
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        _Check_return_ HRESULT ShouldHeaderBeVisible(
            _Out_ bool* visible);

        void ReleaseTemplateParts();

        _Check_return_ HRESULT ValidateEvent(_In_ xaml::IRoutedEventArgs* pEventArgs, _Out_ bool* pIsValidEvent);
    };
}
