// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      Implements logical selection object for plain-text controls.
//
//  See also:
//      <ITextSelection>.

struct ITextSelectionNotify;

class CTextSelection final : public IJupiterTextSelection
{
private:
    CTextSelection(
        _In_ ITextContainer *pContainer,
        _In_ ITextView      *pTextView,
        _In_opt_ ITextSelectionNotify * textSelectionNotify
    );

public:
    ~CTextSelection() override {}

    _Check_return_ HRESULT static Create(
        _In_        ITextContainer  *pContainer,
        _In_        ITextView       *pTextView,
        _In_opt_ ITextSelectionNotify * textSelectionNotify,
        _Outptr_ CTextSelection **ppSelection
    );

    _Check_return_ HRESULT SetCaretPositionFromPoint(_In_ XPOINTF point) override;

    _Check_return_ HRESULT ExtendSelectionByMouse(_In_ XPOINTF point) override;

    _Check_return_ HRESULT SelectWord(_In_ XPOINTF point) override;

    _Check_return_ HRESULT GetMovingTextPosition(_Out_ CTextPosition *pPosition) const override;
    _Check_return_ HRESULT GetAnchorTextPosition(_Out_ CTextPosition *pPosition) const override;
    _Check_return_ HRESULT GetStartTextPosition(_Out_ CTextPosition *pPosition) const override;
    _Check_return_ HRESULT GetEndTextPosition(_Out_ CTextPosition *pPosition) const override;

    TextGravity GetCursorGravity() const override;
    TextGravity GetStartGravity() const override;
    TextGravity GetEndGravity() const override;
    TextGravity GetMovingGravity() const override;

    _Check_return_ bool IsEmpty() const override;
    _Check_return_ HRESULT GetLength(_Out_ XUINT32 *pLength) const override;

    _Check_return_ HRESULT Select(
        _In_ XUINT32     iAnchorTextPosition,
        _In_ XUINT32     iMovingTextPosition,
        _In_ TextGravity eCursorGravity
    ) override;

    _Check_return_ HRESULT Select(
        _In_ const CTextPosition &anchorTextPosition,
        _In_ const CTextPosition &movingTextPosition,
        _In_ TextGravity          eCursorGravity
    ) override;

    _Check_return_ HRESULT GetText(_Out_ xstring_ptr* pstrText) const override;

    _Check_return_ HRESULT GetXaml(
        _Out_ xstring_ptr* pstrXaml
    ) const override;

    _Check_return_ HRESULT ExtendSelectionByMouse(_In_ const CTextPosition &cursorPosition) override;

    _Check_return_ HRESULT SetCaretPositionFromTextPosition(_In_ const CTextPosition &position) override;

    _Check_return_ HRESULT SelectWord(_In_ const CTextPosition &position) override;

    void ResetSelection() override;

private:
    ITextContainer         *m_pContainer;
    CPlainTextPosition      m_movingPosition;
    CPlainTextPosition      m_anchorPosition;
    TextGravity             m_eCursorGravity;
    ITextView              *m_pTextView;
    ITextContainer         *GetContainer()   {ASSERT(m_pContainer != NULL); return m_pContainer;}
    ITextSelectionNotify   *m_textSelectionNotify;

    static _Check_return_ HRESULT MoveToInsertionPosition(
        _In_    ITextContainer   *pContainer,
        _In_    ITextView        *pTextBoxView,
        _Inout_ XUINT32          *pTextPosition,
        _In_    DirectUI::LogicalDirection  eDirection
    );

    static _Check_return_ HRESULT NormalizeRange(
        _In_    ITextContainer *pContainer,
        _In_    ITextView      *pTextBoxView,
        _Inout_ XUINT32        *piStartTextPosition,
        _Inout_ XUINT32        *piEndTextPosition
    );

    static _Check_return_ HRESULT NormalizeSelection(
        _In_    ITextContainer *pContainer,
        _In_    ITextView      *pTextBoxView,
        _Inout_ XUINT32        *piAnchorTextPosition,
        _Inout_ XUINT32        *piMovingTextPosition
    );
};
