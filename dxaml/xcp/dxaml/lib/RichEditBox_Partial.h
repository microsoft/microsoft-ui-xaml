// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RichEditBox.g.h"
#include "RichEditBoxTextChangingEventArgs.g.h"
#include "RichEditBoxSelectionChangingEventArgs.g.h"
#include <WinUIEdit.h>
#include <RichEditBox.h>
#include <ContextMenuEventArgs.g.h>

namespace DirectUI
{
    class IsEnabledChangedEventArgs;

    //---------------------------------------------------------------------------
    //
    //  The DirectUI editable rich text control.
    //
    //---------------------------------------------------------------------------
    PARTIAL_CLASS(RichEditBox)
    {
    public:
        RichEditBox();

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
        IFACEMETHOD(add_CandidateWindowBoundsChanged)(_In_ wf::ITypedEventHandler<xaml_controls::RichEditBox*, xaml_controls::CandidateWindowBoundsChangedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;

        IFACEMETHOD(OnCreateAutomationPeer)(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        _Check_return_ HRESULT OnCharacterReceivedImpl(_In_ xaml_input::ICharacterReceivedRoutedEventArgs* pArgs) final;
        _Check_return_ HRESULT get_DocumentImpl(_COM_Outptr_ mut::ITextDocument** pValue);
        _Check_return_ HRESULT get_TextDocumentImpl(_COM_Outptr_ mut::ITextDocument** returnValue);

        static _Check_return_ HRESULT HandleHyperlinkNavigation(_In_reads_(cLinkText) WCHAR* pLinkText, XUINT32 cLinkText);

        static _Check_return_ HRESULT OnApplyTemplateHandler(_In_ CDependencyObject *pNativeRichEditBox);
        static _Check_return_ HRESULT OnTextChangingHandler(_In_ CDependencyObject *pNativeRichEditBox, _In_ bool fTextChanged);
        static _Check_return_ HRESULT OnSelectionChangingHandler(
            _In_ CDependencyObject* const nativeRichEditBox,
            _In_ long selectionStart,
            _In_ long selectionLength,
            _Out_ BOOLEAN* wasCanceled);

        static _Check_return_ HRESULT ShowPlaceholderTextHandler(_In_ CDependencyObject *pNativeRichEditBox, _In_ bool isEnabled);
        static _Check_return_ HRESULT OnContextMenuOpeningHandler(_In_ CDependencyObject* const pNativeRichEditBox, _In_ XFLOAT cursorLeft, _In_ XFLOAT cursorTop, _Out_ bool& handled);
        static _Check_return_ HRESULT QueueUpdateSelectionFlyoutVisibility(_In_ CDependencyObject *pNativeRichEditBox);

        _Check_return_ HRESULT GetLinguisticAlternativesAsyncImpl(_Outptr_ wf::IAsyncOperation<wfc::IVectorView<HSTRING>*>** ppReturnValue);

        ctl::ComPtr<RichEditBoxTextChangingEventArgs> m_textChangingEventArgs;
        ctl::ComPtr<RichEditBoxSelectionChangingEventArgs> m_selectionChangingEventArgs;

        _Check_return_ HRESULT UpdateSelectionFlyoutVisibility();

    protected:
        ~RichEditBox() override;

        // IsEnabled property changed handler.
        _Check_return_ HRESULT OnIsEnabledChanged(_In_ DirectUI::IsEnabledChangedEventArgs* pArgs) override;

        // Prepares object's state
        _Check_return_ HRESULT Initialize() override;

    private:
        // "Header" template part

        // Holds a reference to the Header Content Presenter so we can collapse it when not in use
        TrackerPtr<xaml::IUIElement> m_tpHeaderPresenter;
        TrackerPtr<xaml::IUIElement> m_requiredHeaderPresenter;
        TrackerPtr<xaml::IUIElement> m_tpPlaceholderTextPresenter;

        // We dont want OnPropertyChanged to trigger UpdateHeaderPresenterVisibility while the template is being loaded
        // so we use this as a flag while initializing the object
        BOOLEAN m_isInitializing;

        EventRegistrationToken m_storyboardCompletedToken{};

        BOOLEAN m_isAnimatingHeight;

        _Check_return_ HRESULT OnHeightAnimationCompleted(IInspectable* pSender, IFrameworkElement* pSenderAsFE);

        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
        _Check_return_ HRESULT UpdateHeaderPresenterVisibility();
        _Check_return_ HRESULT UpdatePlaceholderTextPresenterVisibility(_In_ bool isEnabled);

        void SetPlaceholderTextPresenter(_In_ xaml::IUIElement* const pValue)
        {
            SetPtrValueWithQIOrNull(m_tpPlaceholderTextPresenter, pValue);
        }

        // Handle the SizeChanged event.
        _Check_return_ HRESULT OnSizeChanged(
            _In_ IInspectable* pSender,
            _In_ xaml::ISizeChangedEventArgs* pArgs);

        void ReleaseTemplateParts();
    };
}
