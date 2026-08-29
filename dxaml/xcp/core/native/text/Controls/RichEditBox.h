// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#ifndef RICHEDIT_BOX_H
#define RICHEDIT_BOX_H

#include "TextBoxBase.h"

class CInputScope;
//---------------------------------------------------------------------------
//
//  The DirectUI editable rich text control.
//
//---------------------------------------------------------------------------
class CRichEditBox final : public CTextBoxBase
{
public:
    // Public properties.

    _Check_return_ static HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
    );

    // CDependencyObject overrides.
    KnownTypeIndex GetTypeIndex() const  final
    {
        return DependencyObjectTraits<CRichEditBox>::Index;
    }
    _Check_return_ HRESULT OnApplyTemplate() final;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    _Check_return_ HRESULT UpdateVisualState() final;
    // CTextBoxBase overrides.
    _Check_return_ HRESULT GetAlignment(_Out_ DirectUI::TextAlignment *pAlignment) final;
    bool IsReadOnly() const final;
    bool IsEmpty() final;

    // Gets the inner RichEdit document.
    _Check_return_ HRESULT GetDocument(
        _In_ REFIID iid,
        _Outptr_ IUnknown **ppDocument
    );

    _Check_return_ HRESULT TxNotify(
        _In_ UINT32 notification,
        _In_ void *pData
    ) final;

    _Check_return_ HRESULT GetInputScope(_Out_::InputScopeNameValue *inputScope) final;

protected:

    // CDependencyObject/CUIElement/CFrameworkElement overrides.
    _Check_return_ HRESULT OnCharacterReceived(_In_ CEventArgs* pEventArgs) final;

    // CTextBoxBase overrides.
    _Check_return_ HRESULT Initialize() final;
    _Check_return_ HRESULT OnContentChanged(_In_ const bool fTextChanged) final;
    _Check_return_ HRESULT OnSelectionChanged() final;
    bool AcceptsReturn() const final;
    bool AcceptsRichText() const final;
    _Check_return_ HRESULT OnKeyDown(_In_ CEventArgs* pEventArgs) final;
    _Check_return_ HRESULT OnKeyUp(_In_ CEventArgs* pEventArgs) final;

    KnownEventIndex GetPastePropertyID() const final;
    KnownEventIndex GetCopyPropertyID() const final;
    KnownEventIndex GetCutPropertyID() const final;
    KnownEventIndex GetTextCompositionEventPropertyID(TextCompositionStage stage) const  final;

    KnownPropertyIndex GetSelectionHighlightColorPropertyID() const final;
    KnownPropertyIndex GetSelectionHighlightColorWhenNotFocusedPropertyID() const final;
    KnownPropertyIndex GetSelectionFlyoutPropertyID() const final { return KnownPropertyIndex::RichEditBox_SelectionFlyout; }

#if WI_IS_FEATURE_PRESENT(Feature_HeaderPlacement)
    KnownPropertyIndex GetHeaderPlacementPropertyID() const final { return KnownPropertyIndex::RichEditBox_HeaderPlacement; }
#endif

    _Check_return_ HRESULT SetTextServicesBuffer(_In_ const xstring_ptr& strText) final;
    _Check_return_ HRESULT OnFormattingAcceleratorsChanged();

private:

    CRichEditBox(_In_ CCoreServices *pCore);
    ~CRichEditBox() override;

    static _Check_return_ HRESULT ValidateSetValueArguments(
        _In_ const CDependencyProperty *pProperty,
        _In_ const CValue& value
    );

    _Check_return_ HRESULT ShouldNavigateLinkOnMouseClick(_Out_ bool *pfNavigate);
    _Check_return_ HRESULT HandleLinkNavigation(_In_ UINT32 linkType, _In_ UINT msg, _In_ void* pData);

    enum class LinkMouseHoverTarget
    {
        None = 0,
        HyperLink,
    } m_linkHoverTarget;
    MouseCursor m_eNonHyperlinkCursor;
};
#endif // RICHEDIT_BOX_H
