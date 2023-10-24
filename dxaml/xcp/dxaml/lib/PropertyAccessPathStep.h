// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to manipulate the property paths.

#pragma once

#include "PropertyAccess.h"
#include "PropertyPathStep.h"

namespace DirectUI
{

class PropertyAccessPathStep: 
    public IPropertyAccessHost,
    public PropertyPathStep
{
public:
    PropertyAccessPathStep():
        m_szProperty(nullptr),
        m_fListenToChanges(FALSE),
        m_pDP(nullptr)
    { }

    _Check_return_ HRESULT Initialize(
       _In_ PropertyPathListener *pListener,
       _In_z_ WCHAR *szProperty, 
       _In_ bool fListenToChanges)
    {
        HRESULT hr = S_OK;
        
        IFC(PropertyPathStep::Initialize(pListener));
        m_szProperty = szProperty;
        m_fListenToChanges = fListenToChanges;

    Cleanup:

        RRETURN(hr);
    }

    _Check_return_ HRESULT Initialize(
        _In_ PropertyPathListener *pListener,
        _In_ const CDependencyProperty *pDP, 
        _In_ bool fListenToChanges)
    {
        HRESULT hr = S_OK;

        IFC(PropertyPathStep::Initialize(pListener));
        m_pDP = pDP;
        m_fListenToChanges = fListenToChanges;

    Cleanup:

        RRETURN(hr);
    }

    using PropertyPathStep::Initialize;

    ~PropertyAccessPathStep() override;

public:

    // PropertyPathStep overrides
    _Check_return_ HRESULT ReConnect(_In_ IInspectable *pSource) override;
    
    _Check_return_ HRESULT GetValue(_Outptr_ IInspectable **ppValue) override; 
    _Check_return_ HRESULT SetValue(_In_  IInspectable *pValue) override; 

    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;

    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;

    bool IsConnected() override 
    { return m_tpPropertyAccess && m_tpPropertyAccess->IsConnected(); }

    WCHAR *DebugGetPropertyName() override
    { return m_szProperty; }

public:

    // IPropertyAccessHost
    _Check_return_ HRESULT SourceChanged() override;
    
    WCHAR *GetPropertyName() override
    { return m_szProperty; }
        

protected:

    // PropertyPathStep overrides
    void DisconnectCurrentItem() override;
    _Check_return_ HRESULT CollectionViewCurrentChanged() override;

private:

    _Check_return_ HRESULT ConnectToPropertyOnSource(
        _In_ IInspectable *pSource,
        _In_ bool fListenToChanges);

    _Check_return_ HRESULT ConnectPropertyAccessForObject(
        _In_ IInspectable *pSource,
        _In_ bool fListenToChanges,
        _Out_ BOOLEAN* pbConnected);

    void TraceGetterError();
    void TraceConnectionError(_In_ IInspectable *pSource);

protected:

    bool m_fListenToChanges;
    WCHAR *m_szProperty;
    const CDependencyProperty* m_pDP;
    TrackerPtr<PropertyAccess> m_tpPropertyAccess;
};

}
