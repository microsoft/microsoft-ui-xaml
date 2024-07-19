// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the class to access properties on ICustomPropertyProvider

#pragma once

#include <TypeNamePtr.h>
#include "PropertyAccess.h"
#include "INPCListenerBase.h"

namespace DirectUI
{

class PropertyProviderPropertyAccess: 
    public INPCListenerBase,
    public PropertyAccess
{
public:
    ~PropertyProviderPropertyAccess() override;

    // IPropertyAccess
    _Check_return_ HRESULT GetValue(_COM_Outptr_result_maybenull_ IInspectable **ppValue) override;
    _Check_return_ HRESULT SetValue(_In_ IInspectable *pValue) override;
    _Check_return_ HRESULT GetType(_Outptr_ const CClassInfo **ppType) override
    { *ppType = m_pPropertyType; return S_OK; }
    bool IsConnected() override
    { return m_tpProperty && m_tpSource; }
    _Check_return_ HRESULT SetSource(_In_opt_ IInspectable *pSource, _In_ BOOLEAN fListenToChanges) override;
    _Check_return_ HRESULT GetSource(_COM_Outptr_result_maybenull_ IInspectable **ppSource) override;
    _Check_return_ HRESULT GetSourceType(_Outptr_ const CClassInfo **ppType) override;
    _Check_return_ HRESULT TryReconnect(_In_ IInspectable* pSource, _In_ BOOLEAN fListenToChanges, _Inout_ BOOLEAN& bConnected, _Inout_ const CClassInfo*& pResolvedType) override;
    _Check_return_ HRESULT DisconnectEventHandlers() override;

protected:

    using PropertyAccess::Initialize;

public:

    static _Check_return_ HRESULT CreateInstance(
        _In_ IPropertyAccessHost *pOwner,
        _In_ xaml_data::ICustomPropertyProvider *pSource,
        _In_ bool fListenToChanges,
        _Outptr_ PropertyAccess **ppPropertyAccess);

private:

    void Initialize(
        _In_ IPropertyAccessHost *pOwner,
        _In_ xaml_data::ICustomPropertyProvider *pSource,
        _In_ xaml_data::ICustomProperty *pProperty,
        _In_ const CClassInfo *pPropertyType);

private:

    _Check_return_ HRESULT OnPropertyChanged() override;
    _Ret_notnull_ const wchar_t* GetPropertyName() override;

private:

    IPropertyAccessHost *m_pOwnerNoRef = nullptr;
    TypeNamePtr m_sourceType;
    TrackerPtr<xaml_data::ICustomProperty> m_tpProperty;
    TrackerPtr<xaml_data::ICustomPropertyProvider> m_tpSource;
    const CClassInfo *m_pPropertyType = nullptr;
};

};
