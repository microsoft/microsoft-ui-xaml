// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to access an indexer property with an int index

#pragma once

#include "PropertyAccess.h"
#include "PropertyPathStep.h"

namespace DirectUI
{

class IntIndexerPathStep:
    public IIndexedPropertyAccessHost,
    public PropertyPathStep
{
public:
    IntIndexerPathStep():
        m_nIndex(0),
        m_fListenToChanges(FALSE),
        m_szIndexerName(NULL)
    { }

    _Check_return_ HRESULT Initialize(
        _In_ PropertyPathListener *pOwner,
        _In_ XUINT32 nIndex,
        _In_ bool fListenToChanges)
    {
        HRESULT hr = S_OK;

        IFC(PropertyPathStep::Initialize(pOwner));
        m_nIndex = nIndex;
        m_fListenToChanges = fListenToChanges;

    Cleanup:

        RRETURN(hr);
    }
    using PropertyPathStep::Initialize;

    ~IntIndexerPathStep() override;

public:

    // IPropertyAccessHost
    _Check_return_ HRESULT SourceChanged() override;
    WCHAR *GetPropertyName() override
    {
        return NULL; // No name for the property
    }

    // IIndexedPropertyAccessHost
    _Check_return_ HRESULT GetIndexedPropertyName(_Outptr_result_z_ WCHAR **pszPropertyName) override;

    // PropertyPathStep overrides
    _Check_return_ HRESULT ReConnect(_In_ IInspectable *pSource) override;

    _Check_return_ HRESULT GetValue(_Out_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_  IInspectable *pValue) override;

    bool IsConnected() override;

    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;

    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;

protected:

    // PropertyPathStep overrides
    void DisconnectCurrentItem() override;
    _Check_return_ HRESULT CollectionViewCurrentChanged() override;

private:

    _Check_return_ HRESULT VectorChanged(_In_ wfc::IVectorChangedEventArgs *pArgs);

    _Check_return_ HRESULT AddVectorChangedHandler();
    _Check_return_ HRESULT SafeRemoveVectorChangedHandler();

    _Check_return_ HRESULT GetVectorSize(_Out_ XUINT32 *pnSize);

    _Check_return_ HRESULT GetValueAtIndex(_Out_ IInspectable **ppValue);
    _Check_return_ HRESULT SetValueAtIndex(_Out_ IInspectable *pValue);

    _Check_return_ HRESULT InitializeFromSource(_In_ IInspectable *pRawSource);
    _Check_return_ HRESULT InitializeFromVector(_In_ IInspectable *pSource, _Out_ bool *pfResult);

    void TraceGetterFailure();
    void TraceConnectionFailure(_In_ IInspectable *pSource);

private:

    TrackerPtr<wfc::IVector<IInspectable *>> m_tpVector;
    TrackerPtr<wfc::IVectorView<IInspectable *>> m_tpVectorView;
    TrackerPtr<PropertyAccess> m_tpIndexer;
    XUINT32 m_nIndex;
    WCHAR *m_szIndexerName;
    bool m_fListenToChanges;

    ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedEventHandler;
};

}
