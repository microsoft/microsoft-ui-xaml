// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextElement.g.h"

namespace DirectUI
{
    // Represents the RichTextBlock
    PARTIAL_CLASS(TextElement)
    {
    public:
        ~TextElement() override;

        _Check_return_ HRESULT get_NameImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT get_ContentStartImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
        _Check_return_ HRESULT get_ContentEndImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
        _Check_return_ HRESULT get_ElementStartImpl(_Outptr_ xaml_docs::ITextPointer** pValue);
        _Check_return_ HRESULT get_ElementEndImpl(_Outptr_ xaml_docs::ITextPointer** pValue);

        _Check_return_ HRESULT FindNameImpl(_In_ HSTRING name, _Out_ IInspectable** ppReturnValue);

        _Check_return_ HRESULT get_AccessKeyScopeOwnerImpl(_Outptr_result_maybenull_ xaml::IDependencyObject** ppValue);
        _Check_return_ HRESULT put_AccessKeyScopeOwnerImpl(_In_opt_ xaml::IDependencyObject* pValue);

        HRESULT DisconnectVisualChildrenRecursive()
        {
            return OnDisconnectVisualChildren();
        }

        // UIA callback
        static _Check_return_ HRESULT OnCreateAutomationPeer(_In_ CDependencyObject* nativeTarget, _Out_ CAutomationPeer** returnAP);

        _Check_return_ HRESULT virtual AppendAutomationPeerChildren(_In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAutomationPeerChildren, _In_ INT startPos, _In_ INT endPos);
        _Check_return_ HRESULT GetOrCreateAutomationPeer(xaml_automation_peers::IAutomationPeer** ppAutomationPeer);
        _Check_return_ HRESULT OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue) override;

        _Check_return_ HRESULT OnDisconnectVisualChildrenImpl()
        {
            RRETURN(S_OK);
        }

        _Check_return_ HRESULT get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue);
        _Check_return_ HRESULT put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue);

        void ResetAutomationPeer();

    private:
        TrackerPtr<xaml_automation_peers::IAutomationPeer> m_tpAP;
    };
}
