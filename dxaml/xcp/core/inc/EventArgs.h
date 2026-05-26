// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "Indexes.g.h"

class CAutomationPeerEventArgs;
class CRoutedEventArgs;

class CEventArgs: public IUnknown
{
public:
    CEventArgs() : m_cRef(1)
    {
    }

    // Destructor
    virtual ~CEventArgs()
    {
    }

    virtual CAutomationPeerEventArgs* AsAutomationPeerEventArgs()
    {
        return nullptr;
    }

    virtual CRoutedEventArgs* AsRoutedEventArgs()
    {
        return nullptr;
    }

    virtual KnownTypeIndex GetTypeIndex() const
    {
        return KnownTypeIndex::EventArgs;
    }

    virtual _Check_return_ HRESULT GetFrameworkPeer(_Outptr_ IInspectable** ppPeer);

    virtual _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer);

    virtual bool IsErrorEventArgs() const
    {
        return false;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject) override
    {
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

private:
    XUINT32 m_cRef;
};
