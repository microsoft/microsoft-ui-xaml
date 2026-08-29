// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the interfaces for accessing a single property
//      Abstracts away the differences between the types of properties
//      supported.    

#pragma once

namespace DirectUI
{
    
class PropertyAccess: public ctl::WeakReferenceSource
{
public:    
    virtual _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) = 0;
    virtual _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) = 0;
    virtual _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) = 0;
    virtual _Check_return_ HRESULT GetSource(_Outptr_ IInspectable **ppSource) = 0;
    virtual _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) = 0;
    virtual _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) = 0;
    virtual _Check_return_ HRESULT DisconnectEventHandlers() = 0;
    virtual _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) = 0;
    virtual bool IsConnected() = 0;
};

interface IPropertyAccessHost
{
    virtual _Check_return_ HRESULT SourceChanged() = 0;
    virtual WCHAR *GetPropertyName() = 0;
};

interface IIndexedPropertyAccessHost: public IPropertyAccessHost
{
    virtual _Check_return_ HRESULT GetIndexedPropertyName(_Outptr_result_z_ WCHAR **pszPropertyName) = 0;
};

}

