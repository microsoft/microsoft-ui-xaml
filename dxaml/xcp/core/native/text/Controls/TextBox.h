// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextBoxBase.h"

class CInputScope;

//---------------------------------------------------------------------------
//
//  The DirectUI editable plain text control.
//
//---------------------------------------------------------------------------
class CTextBox final : public CTextBoxBase
{
public:
    // Public properties.
    xstring_ptr m_strText;
    xstring_ptr m_strSelectedText;
    bool m_isDesktopPopupMenuEnabled;

    _Check_return_ static HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ static HRESULT CanPasteClipboardContent(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult
    );

    _Check_return_ static HRESULT CanUndo(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult
    );

    _Check_return_ static HRESULT CanRedo(
        _In_ CDependencyObject* pObject,
        _In_ uint32_t cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult
    );

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    KnownTypeIndex GetTypeIndex() const final;
    _Check_return_ HRESULT OnApplyTemplate() override;
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    // TextBoxBase overrides.
    _Check_return_ HRESULT GetAlignment(_Out_ DirectUI::TextAlignment *pAlignment) final;
    bool IsReadOnly() const final;

    _Check_return_ HRESULT OnDeleteButtonClick();

    _Check_return_ HRESULT Select(
        _In_ XINT32 start,
        _In_ XINT32 length
        );

    _Check_return_ HRESULT GetRectFromCharacterIndex(
        _In_ XINT32 charIndex,
        _In_ BOOLEAN trailingEdge,
        _Out_ wf::Rect* returnValue
        );

    bool IsEmpty() final;

    _Check_return_ HRESULT GetInputScope(_Out_ ::InputScopeNameValue *inputScope) final;

protected:

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;
    _Check_return_ HRESULT OnCharacterReceived(_In_ CEventArgs* pEventArgs) final;

    // TextBoxBase overrides.
    _Check_return_ HRESULT Initialize() final;
    _Check_return_ HRESULT OnContentChanged(_In_ const bool fTextChanged) final;
    _Check_return_ HRESULT OnSelectionChanged() override;
    _Check_return_ HRESULT OnAcceptsReturnChanged() final;
    _Check_return_ HRESULT UpdateVisualState() final;
    bool AcceptsReturn() const final;

    KnownEventIndex GetPastePropertyID() const final;
    KnownEventIndex GetCopyPropertyID() const final;
    KnownEventIndex GetCutPropertyID() const final;
    KnownEventIndex GetTextCompositionEventPropertyID(TextCompositionStage stage) const  final;

    KnownPropertyIndex GetSelectionHighlightColorPropertyID() const final;
    KnownPropertyIndex GetSelectionHighlightColorWhenNotFocusedPropertyID() const final;
    KnownPropertyIndex GetSelectionFlyoutPropertyID() const final { return KnownPropertyIndex::TextBox_SelectionFlyout; }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    KnownPropertyIndex GetHeaderPlacementPropertyID() const final { return KnownPropertyIndex::TextBox_HeaderPlacement; }
#endif

    _Check_return_ HRESULT SetTextServicesBuffer(_In_ const xstring_ptr& strText) final;
    _Check_return_ HRESULT UpdateTextForCompositionStartedEvent() final;

private:
    bool m_fHasSpaceForDeleteButton    : 1;
    bool m_isTextChangingFiring        : 1;

    XINT32 m_iLastSelectionStart = 0;
    XINT32 m_iLastSelectionLength = 0;

    CTextBox(_In_ CCoreServices *pCore);
    ~CTextBox() override;

    _Check_return_ HRESULT SetSelectedText(
        _In_ const xstring_ptr& strText
        );

    static _Check_return_ HRESULT ValidateSetValueArguments(
        _In_ const CDependencyProperty *pProperty,
        _In_ const CValue& value
        );

    bool CanInvokeDeleteButton();

    _Check_return_ HRESULT UpdateTextProperty(_In_ const bool notifyPropertyChangeOnly, _In_ const bool fTextChanged);
};

