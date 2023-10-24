// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Defines the class to access properties using the IXamlMetadataProvider
//      interface

#pragma once

#include "PropertyAccess.h"
#include "INPCListenerBase.h"

namespace DirectUI
{
    class PropertyInfoPropertyAccess: 
        public INPCListenerBase,
        public PropertyAccess
    {
    public:
        
        _Check_return_ HRESULT Initialize(
            _In_ IPropertyAccessHost *pOwner,
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pSourceType,
            _In_ const CCustomProperty* pProperty);
        using PropertyAccess::Initialize;

        PropertyInfoPropertyAccess():
            m_pOwner(nullptr),
            m_pSourceType(nullptr),
            m_pProperty(nullptr)
        { }

        _Check_return_ HRESULT OnPropertyChanged() override;
        _Ret_notnull_ const wchar_t* GetPropertyName() override;

    public:

        ~PropertyInfoPropertyAccess() override;
        
    public:

        // IPropertyAccess
        _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) override;
        _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) override;
        _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override;
        bool IsConnected() override;
        _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) override;
        _Check_return_ HRESULT GetSource(_Outptr_ IInspectable** ppSource) override;
        _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override
            { *ppType = m_pSourceType; return S_OK; }
        _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) override;
        _Check_return_ HRESULT DisconnectEventHandlers() override;

    public:

        static _Check_return_ HRESULT CreateInstance(
            _In_ IPropertyAccessHost *pOwner,
            _In_ IInspectable *pSource, 
            _In_ const CClassInfo *pSourceType,
            _In_ bool fListenToChanges,
            _Outptr_ PropertyAccess **ppPropertyAccess);

    private:

        IPropertyAccessHost* m_pOwner;
        const CClassInfo* m_pSourceType;
        TrackerPtr<IInspectable> m_tpSource;
        const CCustomProperty* m_pProperty;
    };
}
