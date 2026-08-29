// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlMember.h"
#include "XamlTypeInfoProvider.h"

#include <XamlTypeInfo.h>

namespace Internal
{
    XamlMember::XamlMember(
        UINT member,
        _In_ Private::XamlTypeInfoProvider* pTypeInfoProvider)
    {
        m_pTypeInfoProvider = pTypeInfoProvider;
        m_pUserMemberData = m_pTypeInfoProvider->GetUserMemberInfo(member);
    }

    IFACEMETHODIMP
    XamlMember::get_IsAttachable(
        _Out_ BOOLEAN* value)
    {
        *value = static_cast<BOOLEAN>(m_pUserMemberData->isAttachable);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlMember::get_IsDependencyProperty(
        _Out_ BOOLEAN* value)
    {
        *value = static_cast<BOOLEAN>(m_pUserMemberData->isDependencyProperty);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlMember::get_IsReadOnly(
        _Out_ BOOLEAN* value)
    {
        *value = static_cast<BOOLEAN>(m_pUserMemberData->isReadOnly);

        return S_OK;
    }

    IFACEMETHODIMP
    XamlMember::get_Name(
        _Outptr_ HSTRING* value)
    {
        HRESULT hr = m_pTypeInfoProvider->GetHString(m_pUserMemberData->idsName, value);

        if (FAILED(hr))
        {
            RoOriginateError(hr, nullptr);
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlMember::get_TargetType(
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = E_INVALIDARG;

        if (value)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;

            hr = m_pTypeInfoProvider->FindXamlType_FromInt32(
                m_pUserMemberData->iTargetType,
                &xamlType);

            *value = SUCCEEDED(hr) ? xamlType.Detach() : nullptr;
        }

        if (FAILED(hr))
        {
            RoOriginateError(hr, nullptr);
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlMember::get_Type(
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = E_INVALIDARG;

        if (value)
        {
            wrl::ComPtr<xaml_markup::IXamlType> xamlType;

            hr = m_pTypeInfoProvider->FindXamlType_FromInt32(
                m_pUserMemberData->iType,
                &xamlType);

            *value = SUCCEEDED(hr) ? xamlType.Detach() : nullptr;
        }

        if (FAILED(hr))
        {
            RoOriginateError(hr, nullptr);
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlMember::GetValue(
        _In_ IInspectable* instance,
        _Outptr_ IInspectable** value)
    {
        auto getFuncId = m_pUserMemberData->getFuncId;

        HRESULT hr = getFuncId ? S_OK : E_NOTIMPL;

        if (SUCCEEDED(hr))
        {
            hr = m_pTypeInfoProvider->GetValue(getFuncId, instance, value);
        }

        if (FAILED(hr))
        {
            RoOriginateError(hr, nullptr);
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlMember::SetValue(
        _In_ IInspectable* instance,
        _In_ IInspectable* value)
    {
        auto setFuncId = m_pUserMemberData->setFuncId;

        HRESULT hr = setFuncId ? S_OK : E_NOTIMPL;

        if (SUCCEEDED(hr))
        {
            hr = m_pTypeInfoProvider->SetValue(setFuncId, instance, value);
        }

        if (FAILED(hr))
        {
            RoOriginateError(hr, nullptr);
        }

        return hr;
    }
}

