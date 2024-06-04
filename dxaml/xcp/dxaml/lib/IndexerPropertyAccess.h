// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyAccess.h"

namespace DirectUI
{

class IndexerPropertyAccess: public PropertyAccess
{
private:
    
    void Initialize(
        _In_ IIndexedPropertyAccessHost *pOwner,
        _In_ xaml_data::ICustomPropertyProvider *pSource,
        _In_ IInspectable *pIndex, 
        _In_ xaml_data::ICustomProperty *pProperty,
        _In_ const CClassInfo *pPropertyType);

protected:

    using PropertyAccess::Initialize;   // Bring in the base class Initialize

public:

    IndexerPropertyAccess(): 
           m_pOwner(NULL),
           m_pPropertyType(NULL)
       { }

    ~IndexerPropertyAccess() override;

    // IPropertyAccess
    _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) override;
    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override
    { *ppType = m_pPropertyType; return S_OK; }
    bool IsConnected() override
    { return m_tpIndexer && m_tpSource; }
    _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) override;
    _Check_return_ HRESULT GetSource(_Outptr_ IInspectable **ppSource) override;
    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;
    _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) override
    {
        bConnected = FALSE;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT DisconnectEventHandlers() override;

public:

    static _Check_return_ HRESULT CreateInstance(
        _In_ IIndexedPropertyAccessHost *pOwner,
        _In_ xaml_data::ICustomPropertyProvider *pSource,
        _In_ wxaml_interop::TypeName sTypeName,
        _In_ IInspectable *pIndex,
        _In_ bool fListenToChanges,
        _Outptr_ PropertyAccess **ppPropertyAccess);

private:

    _Check_return_ HRESULT AddPropertyChangedHandler();

    // This method is safe to be called from the destructor
    _Check_return_ HRESULT SafeRemovePropertyChangedHandler();

    _Check_return_ HRESULT PropertyChanged();

    // This function is used to workarround a CLR bug
    // that requires the index to be a different value evevery time
    // This will be removed later, once the bug is fixed
    static _Check_return_ HRESULT DuplicatePropertyValue(_In_ IInspectable *pValue, _Outptr_ IInspectable **ppDupe);

private:

    _Check_return_ HRESULT OnPropertyChanged(_In_ xaml_data::IPropertyChangedEventArgs *pArgs)
    {
        HRESULT hr = S_OK;
        wrl_wrappers::HString strProperty;
        LPCWSTR szProperty = NULL;
        WCHAR *szIndexerName = NULL;
        boolean fChanged = false;

        IFC(pArgs->get_PropertyName(strProperty.GetAddressOf()));

        szProperty = strProperty.GetRawBuffer(NULL);

        // Determine if the indexer has changed
        if (wcscmp(szProperty, L"Item[]") != 0)
        {
            IFC(m_pOwner->GetIndexedPropertyName(&szIndexerName));
            fChanged = !wcscmp(szProperty, szIndexerName);
        }
        else 
        {
            fChanged = true;
        }

        // Notify of the change    
        if (fChanged)
        {
            IFC(PropertyChanged());
        }

    Cleanup:

        return hr;
    }

private:

    IIndexedPropertyAccessHost *m_pOwner;
    TrackerPtr<xaml_data::ICustomProperty> m_tpIndexer;
    TrackerPtr<xaml_data::ICustomPropertyProvider> m_tpSource;
    TrackerPtr<IInspectable> m_tpIndex;
    ctl::EventPtr<PropertyChangedEventCallback> m_epPropertyChangedHandler;
    const CClassInfo *m_pPropertyType;
};

}
