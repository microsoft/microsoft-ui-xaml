// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to manipulate the property paths.

#pragma once

#include "PropertyAccess.h"
#include "PropertyPathStep.h"
#include "XamlTelemetry.h"
#include "DependencyObjectPropertyAccess.h"
#include "PerfOptIn.h"

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

    static bool IsInlineDOAccessEnabled()
    {
        return IsPerfOptInEnabled();
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

    bool IsConnected() override;

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

    // Returns true when this step is using the inline DO fast path
    // (no heap-allocated DependencyObjectPropertyAccess).
    bool IsUsingInlineDOAccess() const
    { 
        ASSERT(IsInlineDOAccessEnabled()); 
        return m_inlineDO.IsActive();
    }

    // Inline DO accessor helpers
    _Check_return_ HRESULT InlineDOConnect(
        _In_ IInspectable *pSource,
        _In_ IInspectable *pSourceForDP,
        _In_ const CClassInfo *pSourceType,
        _In_ bool fListenToChanges);

    _Check_return_ HRESULT InlineDOAddPropertyChangedHandler();
    _Check_return_ HRESULT InlineDOSafeRemovePropertyChangedHandler();
    void InlineDODisconnect();

    _Check_return_ HRESULT InlineDOGetDependencyObject(_Outptr_ DependencyObject **ppSource);
    _Check_return_ HRESULT InlineDOSafeGetDependencyObject(_Outptr_ DependencyObject **ppSource);

    static _Check_return_ HRESULT ResolveDependencyObject(
        _In_ IInspectable *pSource,
        _Outptr_ DependencyObject **ppSource);

    _Check_return_ HRESULT InlineDOPropertyChanged(_In_ const CDependencyProperty* pDP);

protected:

    bool m_fListenToChanges;
    WCHAR *m_szProperty;
    const CDependencyProperty* m_pDP;
    TrackerPtr<PropertyAccess> m_tpPropertyAccess;

    // Inline DependencyObject accessor state (only used when IsInlineDOAccessEnabled() is true).
    // When active, we access the DP directly instead of
    // allocating a separate heap DependencyObjectPropertyAccess object.
    struct InlineDOAccessor
    {
        TrackerPtr<IInspectable> m_tpSource;       // The binding source object
        ctl::EventPtr<PropertyAccessPathStepDPChangedCallback> m_epSyncHandler;  // DP-changed event
        const CClassInfo* m_pSourceType = nullptr;  // Source type for reconnect
        const CDependencyProperty* m_pProperty = nullptr;  // Resolved DP 

        bool IsActive() const
        {
            return m_tpSource.Get() != nullptr;
        }
        void Clear()
        {
            m_tpSource.Clear();
            // m_epSyncHandler is detached in PropertyAccessPathStep dtor.
            // Just for safety, move-assign from a default-constructed EventPtr to neuter any
            // handler that wasn't properly detached from its source.            
            m_epSyncHandler = decltype(m_epSyncHandler){};
            m_pSourceType = nullptr;
            m_pProperty = nullptr;
        }
    };
    InlineDOAccessor m_inlineDO;  // Only valid when IsInlineDOAccessEnabled() is true
};

}
