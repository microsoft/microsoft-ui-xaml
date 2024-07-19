// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <EventArgs.h>

class CRoutedEventArgs : public CEventArgs
{
public:
    CRoutedEventArgs();
    ~CRoutedEventArgs() override;

    CRoutedEventArgs* AsRoutedEventArgs() override
    {
        return this;
    }

    _Check_return_ HRESULT GetFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT PegManagedPeerForRoutedEventArgs();
    void UnpegManagedPeerForRoutedEventArgs();

    _Check_return_ HRESULT get_Source(_Outptr_ CDependencyObject** ppSource);
    _Check_return_ HRESULT put_Source(_In_ CDependencyObject* pSource);

    // Handled property.
    _Check_return_ HRESULT get_Handled(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_bHandled;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_Handled(_In_ BOOLEAN value)
    {
        m_bHandled = !!value;
        RRETURN(S_OK);
    }

    CDependencyObject* m_pSource = nullptr;
    IInspectable* m_pPeggedDXamlPeer = nullptr;
    bool m_bHandled = false;
#ifdef DBG
    DWORD m_threadId = 0;
#endif
};
