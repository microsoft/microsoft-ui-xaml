// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to access a indexer with a string parameter

#pragma once

#include "PropertyAccess.h"
#include "PropertyPathStep.h"

namespace DirectUI
{

class StringIndexerPathStep:
    public IIndexedPropertyAccessHost,
    public PropertyPathStep
{
public:

    StringIndexerPathStep():
        m_szIndex(NULL),
        m_fListenToChanges(FALSE),
        m_szIndexerName(NULL)
    { }

    _Check_return_ HRESULT Initialize(
        _In_ PropertyPathListener *pOwner,
        _In_ WCHAR *szIndex,
        _In_ bool fListenToChanges)
    {
        HRESULT hr = S_OK;

        IFC(PropertyPathStep::Initialize(pOwner));
        m_szIndex = szIndex;
        m_fListenToChanges = fListenToChanges;

    Cleanup:

        RRETURN(hr);
    }
    using PropertyPathStep::Initialize;

    ~StringIndexerPathStep() override;
    
public:

    // PropertyPathStep overrides    
    _Check_return_ HRESULT ReConnect(_In_ IInspectable *pSource) override;
    
    _Check_return_ HRESULT GetValue(_Out_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_  IInspectable *pValue) override;

    bool IsConnected() override
    { return m_tpPropertyAccess && m_tpPropertyAccess->IsConnected(); }

    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;

    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;

protected:

    // PropertyPathStep overrides
    void DisconnectCurrentItem() override;
    _Check_return_ HRESULT CollectionViewCurrentChanged() override;

private:

    _Check_return_ HRESULT InitializeFromSource(_In_ IInspectable *pRawSource);

    void TraceGetterFailure();
    void TraceConnectionFailure(_In_ IInspectable *pSource);

public:

    // IPropertyAccessHost 
    _Check_return_ HRESULT SourceChanged() override;
    WCHAR *GetPropertyName() override
    { return m_szIndex; }

    // IIndexedPropertyAccessHost
    _Check_return_ HRESULT GetIndexedPropertyName(_Outptr_result_z_ WCHAR **pszPropertyName) override;

private:

    TrackerPtr<PropertyAccess> m_tpPropertyAccess;
    WCHAR *m_szIndex;
    bool m_fListenToChanges;
    WCHAR *m_szIndexerName;
};

}
