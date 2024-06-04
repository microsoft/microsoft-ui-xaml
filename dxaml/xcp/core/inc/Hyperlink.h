// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "inline.h"
#include "FocusableHelper.h"
#include <optional>

class CPointerEventArgs;

namespace Theming {
    enum class Theme : uint8_t;
}

// Hyperlink status enum used to update the foreground color.
enum HyperlinkStates
{
    HYPERLINK_NORMAL,
    HYPERLINK_POINTOVER,
    HYPERLINK_PRESSED
};

 class CHyperlink final : public CSpan
 {

public:
    // Properties
    xstring_ptr m_strNavigateUri;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_     CREATEPARAMETERS   *pCreate
    );

    _Check_return_ HRESULT Navigate();

    // CDependencyObject overrides
    bool ShouldRaiseEvent(
        _In_ EventHandle hEvent,
        _In_ bool fInputEvent = false,
        _In_opt_ CEventArgs *pArgs = nullptr
        ) override
    {
        return (m_pEventList != nullptr);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CHyperlink>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    CAutomationPeer* OnCreateAutomationPeer() override;

    // For Click event support.
    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult ,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    _Check_return_ HRESULT UpdateForegroundColor(_In_ HyperlinkStates state)
    {
        return UpdateForegroundColor(GetTheme(), state);
    }

    bool IsFocusable()
    {
        auto element = GetContainingFrameworkElement();
        return element != nullptr &&
            element->IsActive() && IsActive() &&
            element->IsEnabled() &&
            element->IsVisible() &&
            element->AreAllAncestorsVisible() &&
            m_isTabStop;
    }

    IFocusable* GetIFocusable() const
    {
        return m_focusableHelper;
    }

    XINT32 TabIndex() const
    {
        CValue value;
        VERIFYHR(this->GetValueByIndex(KnownPropertyIndex::Hyperlink_TabIndex, &value));
        return value.AsSigned();
    }

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

    DirectUI::UnderlineStyle GetUnderlineStyle() const;
    bool ShouldDrawUnderline() const;

    _Check_return_ HRESULT GetContentText(_Outptr_ HSTRING *output);

    _Check_return_ HRESULT OnPointerEntered(_In_ CPointerEventArgs* pEventArgs);
    _Check_return_ HRESULT OnPointerExited(_In_ CPointerEventArgs* pEventArgs);

    static bool IsLinkNavigationKey(const UINT32 key);
    static void ClearUnderlineVisibleResourceDirectiveCache();
    static bool UnderlineVisibleResourceDirective(_In_ CCoreServices* core);

    bool m_isTabStop = true;

    _Check_return_ HRESULT GetTextContentStart(_Outptr_result_maybenull_  CTextPointerWrapper **ppTextPointerWrapper);
    _Check_return_ HRESULT GetTextContentEnd(_Outptr_result_maybenull_  CTextPointerWrapper **ppTextPointerWrapper);

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) override;
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

private:
    CXcpList<REQUEST> *m_pEventList;
    CAutomationPeer *m_pAP;
    HyperlinkStates m_state;
    bool m_isLinkNavigationKeyDown;
    CFocusableHelper* m_focusableHelper;

    static std::optional<bool> s_underlineVisibleResourceDirective;

    CHyperlink(_In_ CCoreServices *pCore);
    ~CHyperlink() override;

    bool UnderlineVisibleResourceDirective() const;

    _Check_return_ HRESULT UpdateForegroundColor(_In_ Theming::Theme theme, _In_ HyperlinkStates state);
    _Check_return_ HRESULT UpdateUnderline();

    static _Check_return_ HRESULT GotFocusEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    );

    static _Check_return_ HRESULT LostFocusEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    );

    static _Check_return_ HRESULT KeyUpEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    );

    static _Check_return_ HRESULT KeyDownEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    );

    _Check_return_ HRESULT FocusChangedEventListener(
    _In_ CEventArgs* pEventArgs,
    _In_ bool       bGotFocus
    );

    _Check_return_ HRESULT HasToolTip(_Out_ bool *hasToolTip);
    _Check_return_ HRESULT RegisterToolTip();
    _Check_return_ HRESULT UnregisterToolTip();
};
