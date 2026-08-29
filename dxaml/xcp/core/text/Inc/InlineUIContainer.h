// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef INLINE_UI_CONTAINER_H
#define INLINE_UI_CONTAINER_H

#ifndef NO_HEADER_INCLUDES
#include "Inline.h"
#endif

struct IEmbeddedElementHost;

//------------------------------------------------------------------------
//  Summary:
//      Object created for <InlineUIContainer> tag. An InlineUIContainer merely acts as
//      storage for the embedded UIElements--it doesn't contain any measurement or
//      arrangement logic.
//
//      A CInlineUIContainer exposes one property, Child, that can be used to set or get
//      the embedded UI element.
//
//------------------------------------------------------------------------
class CInlineUIContainer final : public CInline
{
private:
    CInlineUIContainer(_In_ CCoreServices *pCore);

public:
    CUIElement *m_pChild;

public:
    DECLARE_CREATE(CInlineUIContainer);
    ~CInlineUIContainer() override;

    //
    // <CDependencyObject overrides>
    //
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInlineUIContainer>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject *pNamescopeOwner,
             EnterParams        params) override;

    _Check_return_ HRESULT LeaveImpl(
        _In_ CDependencyObject *pNamescopeOwner,
             LeaveParams        params) override;

    //
    // </CDependencyObject>
    //

    //
    // <CInline overrides>
    //
    _Check_return_ HRESULT GetRun(
        _In_                              XUINT32                characterPosition,
        _Out_opt_                   const TextFormatting       **ppTextFormatting,
        _Out_opt_                   const InheritedProperties  **ppInheritedProperties,
        _Out_opt_                         TextNestingType       *pNestingType,
        _Out_opt_                         CTextElement         **ppNestedElement,
        _Outptr_result_buffer_(*pcCharacters) const WCHAR          **ppCharacters,
        _Out_                             XUINT32               *pcCharacters
    ) override;

    //
    // </CInline>
    //

    //
    // <Child accessors>
    //

    _Check_return_ HRESULT GetChild(_Outptr_ CUIElement** ppChild);

    //
    // </Child>
    //

    _Check_return_ HRESULT EnsureAttachedToHost(IEmbeddedElementHost *pHost);
    _Check_return_ HRESULT EnsureDetachedFromHost();
    IEmbeddedElementHost* GetCachedHost() const;
    void ClearCachedHost();

    void SetChildLayoutCache(
        _In_ XFLOAT width,
        _In_ XFLOAT height,
        _In_ XFLOAT baseline
        );
    void GetChildLayoutCache(
        _Out_ XFLOAT *pWidth,
        _Out_ XFLOAT *pHeight,
        _Out_ XFLOAT *pBaseline
        ) const;

    _Check_return_ HRESULT Shutdown();

private:
    _Check_return_ HRESULT AddLogicalChild(_In_ CDependencyObject* pNewLogicalChild);
    void RemoveLogicalChild(_Inout_opt_ CDependencyObject* pOldLogicalChild);

private:
    IEmbeddedElementHost *m_pCachedHost;
    bool                 m_isChildAttached;
    XFLOAT                m_childDesiredWidth;  // Cached child desired width. Only used in RichTextBlock.
    XFLOAT                m_childDesiredHeight; // Cached child desired height. Only used in RichTextBlock.
    XFLOAT                m_childBaseline;      // Cached child baseline. Only used in RichTextBlock.
};

inline IEmbeddedElementHost* CInlineUIContainer::GetCachedHost() const
{
    return m_pCachedHost;
}
#endif
