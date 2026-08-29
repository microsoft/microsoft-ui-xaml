// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Phone_XamlTypeInfo {

    XamlLibMetadataProvider::XamlLibMetadataProvider()
        : _provider(nullptr)
    {
    }

    XamlLibMetadataProvider::~XamlLibMetadataProvider()
    {
        delete _provider;
        _provider = nullptr;
    }

    HRESULT
    XamlLibMetadataProvider::RuntimeClassInitialize(
        _In_opt_ IInspectable* hostMetadataProvider)
    {

        if (hostMetadataProvider != NULL)
        {
            IGNOREHR(hostMetadataProvider->QueryInterface(
                __uuidof(xaml_markup::IXamlMetadataProvider),
                reinterpret_cast<void**>(_hostMetadataProvider.GetAddressOf())));
        }

        return S_OK;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXmlnsDefinitions(
        _Out_ UINT* pnLength,
        _Outptr_result_buffer_maybenull_(*pnLength) xaml_markup::XmlnsDefinition** ppDefinitions)
    {
        HRESULT hr = (pnLength == nullptr || ppDefinitions == nullptr) ? E_INVALIDARG : S_OK;

        if (SUCCEEDED(hr))
        {
            if (_hostMetadataProvider)
            {
                hr = _hostMetadataProvider->GetXmlnsDefinitions(
                    pnLength,
                    ppDefinitions);
            }
            else
            {
                *pnLength = 0;
                *ppDefinitions = nullptr;
            }
        }

        return hr;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXamlType(
        wxaml_interop::TypeName type,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        IFCPTRRC(value, E_INVALIDARG);

        IFC(GetXamlTypeByFullNameWorker(
            type.Name,
            &xamlType));

        if (!xamlType)
        {
            if (_hostMetadataProvider)
            {
                hr = _hostMetadataProvider->GetXamlType(
                    type,
                    &xamlType);
            }
        }

        IFC(hr);

        *value = xamlType.Detach();

    Cleanup:
        return hr;
    }

    IFACEMETHODIMP
    XamlLibMetadataProvider::GetXamlTypeByFullName(
        _In_ HSTRING fullName,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        IFCPTRRC(value, E_INVALIDARG);

        IFC(GetXamlTypeByFullNameWorker(
            fullName,
            &xamlType));

        if (!xamlType)
        {
            if (_hostMetadataProvider)
            {
                hr = _hostMetadataProvider->GetXamlTypeByFullName(
                    fullName,
                    &xamlType);
            }
        }

        IFC(hr);

        *value = xamlType.Detach();

    Cleanup:
        return hr;
    }

    HRESULT
    XamlLibMetadataProvider::GetXamlTypeByFullNameWorker(
        _In_ HSTRING fullName,
        _COM_Outptr_ xaml_markup::IXamlType** value)
    {
        HRESULT hr = S_OK;

        wrl::ComPtr<xaml_markup::IXamlType> xamlType;

        ASSERT(value != NULL);

        if (!_provider)
        {
            IFC(Private::XamlTypeInfoProvider::Create(
                &_provider));
        }

        IFC_NOTRACE(_provider->FindXamlType_FromHSTRING(
            fullName,
            &xamlType));

        *value = xamlType.Detach();

    Cleanup:
        return hr;
    }

} } } } XAML_ABI_NAMESPACE_END

