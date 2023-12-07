// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to access properties on DependencyObject(s)

#pragma once

#include "PropertyAccess.h"

namespace DirectUI
{
// IDPChangedEventSource handler traits
struct PropertyAccessPathStepDPChangedTraits
{
    typedef xaml::IDependencyObject event_interface;

    static _Check_return_ HRESULT attach_handler(
        _In_ xaml::IDependencyObject *pSender,
        _In_ IDPChangedEventHandler *pHandler)
    {
        HRESULT hr = S_OK;
        
        IDPChangedEventSource *pDPChangedSource = NULL;
        DependencyObject *pSource = static_cast<DependencyObject*>(pSender);
        
        IFC(pSource->GetDPChangedEventSource(&pDPChangedSource));
        IFC(pDPChangedSource->AddHandler(pHandler));

    Cleanup:
        
        ReleaseInterface(pDPChangedSource);
        RRETURN(hr);
    }

    static _Check_return_ HRESULT detach_handler(
        _In_ xaml::IDependencyObject *pSender,
        _In_ IDPChangedEventHandler *pHandler)
    {
        //RRETURN(pSender->RemoveHandler(pHandler));
        
        HRESULT hr = S_OK;
        
        IDPChangedEventSource *pDPChangedSource = NULL;
        DependencyObject *pSource = static_cast<DependencyObject*>(pSender);
        IFC(pSource->GetDPChangedEventSource(&pDPChangedSource));
        IFC(pDPChangedSource->RemoveHandler(pHandler));

    Cleanup:

        ReleaseInterface(pDPChangedSource);
        RRETURN(hr);

    }
};


// IDPChangedEventSource handler 
typedef ctl::tokenless_event_handler<
    IDPChangedEventHandler,
    xaml::IDependencyObject,
    const CDependencyProperty,
    PropertyAccessPathStepDPChangedTraits> PropertyAccessPathStepDPChangedCallback;

class DependencyObjectPropertyAccess: public PropertyAccess
{
public:
    
    DependencyObjectPropertyAccess();
    ~DependencyObjectPropertyAccess() override;

    _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) override;
    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;
    bool IsConnected() override
    { return m_pProperty && m_tpSource; }
    _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) override;
    _Check_return_ HRESULT GetSource(_Outptr_ IInspectable **ppSource) override;
    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override
        { *ppType = m_pSourceType; return S_OK; }
    _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) override;
    _Check_return_ HRESULT DisconnectEventHandlers() override;

public:
    
    static _Check_return_ HRESULT CreateInstance(
        _In_ IPropertyAccessHost *pOwner, 
        _In_ IInspectable *pSource, 
        _In_ const CClassInfo* pSourceType,
        _In_ bool fListenToChanges,
        _Outptr_ PropertyAccess **ppPropertyAccess);

    static _Check_return_ HRESULT CreateInstance(
        _In_ IPropertyAccessHost *pOwner, 
        _In_ IInspectable *pSource, 
        _In_ const CClassInfo *pSourceType,
        _In_ const CDependencyProperty* pDP,
        _In_ bool fListenToChanges,
        _Outptr_ PropertyAccess **ppPropertyAccess);

protected:    

    void Initialize(
        _In_ IPropertyAccessHost* const pOwner,
        _In_ IInspectable* const pSource,
        _In_ const CClassInfo* const pSourceType,
        _In_ const CDependencyProperty* const pProperty);
    using PropertyAccess::Initialize; // Bring in the base function as well

private:

    _Check_return_ HRESULT ConnectToSourceProperty();

    _Check_return_ HRESULT AddPropertyChangedHandler();

    // This method can be called from the destructor to 
    // remove the event handlers safely
    _Check_return_ HRESULT SafeRemovePropertyChangedHandler();

    _Check_return_ HRESULT SourceDPChanged();

    static _Check_return_ HRESULT GetSource(
        _In_ IInspectable *pSource, 
        _Outptr_ DependencyObject **ppSource);

    _Check_return_ HRESULT GetSource(_Outptr_ DependencyObject **ppSource);

    // This method is safe to be called from the destructor path
    _Check_return_ HRESULT SafeGetSource(_Outptr_ DependencyObject **ppSource);

    static _Check_return_ HRESULT ResolvePropertyName(
        _In_ DependencyObject *pSource, 
        _In_ const CClassInfo *pSourceType,
        _In_z_ WCHAR *szPropertyName, 
        _Outptr_ const CDependencyProperty** ppDP);

    _Check_return_ HRESULT PropertyAccessPathStepDPChanged(_In_ const CDependencyProperty* pDP);


private:

    IPropertyAccessHost* m_pOwner;
    const CClassInfo* m_pSourceType;
    
    const CDependencyProperty* m_pProperty;
    TrackerPtr<IInspectable> m_tpSource;
    
    ctl::EventPtr<PropertyAccessPathStepDPChangedCallback> m_epSyncHandler;
};

}
