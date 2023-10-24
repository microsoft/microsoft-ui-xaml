// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XBFXamlTypeWrapper.h"

using namespace ::Windows::Internal;
using namespace DirectUI;
using namespace DirectUISynonyms;

XBFXamlMetadataProviderWrapper::XBFXamlMetadataProviderWrapper()
{
    m_pXamlMetadataProvider = NULL;
}

XBFXamlMetadataProviderWrapper::~XBFXamlMetadataProviderWrapper()
{
    ReleaseInterface(m_pXamlMetadataProvider);
}

_Check_return_ HRESULT XBFXamlMetadataProviderWrapper::CreateXBFXamlMetadataProviderWrapper(_In_ xaml_markup::IXamlMetadataProvider* pInXamlMetadataProvider, _Outptr_ xaml_markup::IXamlMetadataProvider **ppWrapperXamlMetadataProvider)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<XBFXamlMetadataProviderWrapper> spXamlMetadataProviderWrapper;

    *ppWrapperXamlMetadataProvider = NULL;

    if (pInXamlMetadataProvider)
    {    
        IFC(ctl::make<XBFXamlMetadataProviderWrapper>(&spXamlMetadataProviderWrapper));
        IFC(spXamlMetadataProviderWrapper->SetXamlMetadataProvider(pInXamlMetadataProvider));
    
        *ppWrapperXamlMetadataProvider = ctl::query_interface<xaml_markup::IXamlMetadataProvider>(spXamlMetadataProviderWrapper.Get());
    }

Cleanup:
    RRETURN(hr);
}

HRESULT XBFXamlMetadataProviderWrapper::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_markup::IXamlMetadataProvider)))
    {
        *ppObject = static_cast<xaml_markup::IXamlMetadataProvider*>(this);
    }
    else 
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}


_Check_return_ HRESULT XBFXamlMetadataProviderWrapper::SetXamlMetadataProvider(_In_ xaml_markup::IXamlMetadataProvider* pXamlMetadataProvider)
{
    ReplaceInterface(m_pXamlMetadataProvider, pXamlMetadataProvider);

    RRETURN(S_OK);
}

IFACEMETHODIMP XBFXamlMetadataProviderWrapper::GetXamlType(_In_ wxaml_interop::TypeName type, _Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;

    IFC(GetXamlTypeByFullName(type.Name, value));
 
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlMetadataProviderWrapper::GetXamlTypeByFullName(_In_ HSTRING fullName, _Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType* pInXamlType = NULL;

    IFC(m_pXamlMetadataProvider->GetXamlTypeByFullName(fullName, &pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlMetadataProviderWrapper::GetXmlnsDefinitions(_Out_ UINT32* definitionsSize, _Outptr_result_buffer_(*definitionsSize) xaml_markup::XmlnsDefinition **definitions)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlMetadataProvider->GetXmlnsDefinitions(definitionsSize, definitions));
Cleanup:
    RRETURN(hr);
}

XBFXamlTypeWrapper::XBFXamlTypeWrapper()
{
    m_pXamlType = NULL;
}

XBFXamlTypeWrapper::~XBFXamlTypeWrapper()
{
    ReleaseInterface(m_pXamlType);
}

_Check_return_ HRESULT XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(_In_ xaml_markup::IXamlType* pInXamlType, _Outptr_ xaml_markup::IXamlType **ppWrapperXamlType)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<XBFXamlTypeWrapper> spXamlTypeWrapper;

    *ppWrapperXamlType = NULL;
    
    if (pInXamlType)
    {
        IFC(ctl::make<XBFXamlTypeWrapper>(&spXamlTypeWrapper));
        IFC(spXamlTypeWrapper->SetXamlType(pInXamlType));
    
        *ppWrapperXamlType = spXamlTypeWrapper.Detach();
    }

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

HRESULT XBFXamlTypeWrapper::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_markup::IXamlType)))
    {
        *ppObject = static_cast<xaml_markup::IXamlType*>(this);
    }
    else 
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}


_Check_return_ HRESULT XBFXamlTypeWrapper::SetXamlType(_In_ xaml_markup::IXamlType* pXamlType)
{
    ReplaceInterface(m_pXamlType, pXamlType);

    RRETURN(S_OK);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_BaseType(_Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType* pInXamlType = NULL;

    IFC(m_pXamlType->get_BaseType(&pInXamlType));    
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_ContentProperty(_Outptr_ xaml_markup::IXamlMember **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlMember* pInXamlMember = NULL;

    IFC(m_pXamlType->get_ContentProperty(&pInXamlMember));
    IFC(XBFXamlMemberWrapper::CreateXBFXamlMemberWrapper(pInXamlMember, value));
    pInXamlMember = NULL;

Cleanup:
    ReleaseInterface(pInXamlMember);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_FullName(_Outptr_ HSTRING *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_FullName(value));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsArray(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;
    
    IFC(m_pXamlType->get_IsArray(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsCollection(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_IsCollection(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsConstructible(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_IsConstructible(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsDictionary(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_IsDictionary(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsMarkupExtension(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_IsMarkupExtension(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_IsBindable( _Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_IsBindable(value));
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_ItemType(_Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType* pInXamlType = NULL;

    IFC(m_pXamlType->get_ItemType(&pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_KeyType(_Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType* pInXamlType = NULL;

    IFC(m_pXamlType->get_KeyType(&pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_BoxedType(_Outptr_ xaml_markup::IXamlType** value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType* pInXamlType = NULL;

    IFC(m_pXamlType->get_BoxedType(&pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::get_UnderlyingType(_Out_ wxaml_interop::TypeName *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlType->get_FullName(&value->Name));
    value->Kind = wxaml_interop::TypeKind_Metadata;

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::ActivateInstance(_Outptr_ IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}

IFACEMETHODIMP XBFXamlTypeWrapper::RunInitializer()
{
    RRETURN(S_OK);
}

IFACEMETHODIMP XBFXamlTypeWrapper::GetMember(_In_ HSTRING hMemberName, _Outptr_ xaml_markup::IXamlMember **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlMember* pInXamlMember = NULL;

    IFC(m_pXamlType->GetMember(hMemberName, &pInXamlMember));
    IFC(XBFXamlMemberWrapper::CreateXBFXamlMemberWrapper(pInXamlMember, value));
    pInXamlMember = NULL;

Cleanup:
    ReleaseInterface(pInXamlMember);
    RRETURN(hr);
}

IFACEMETHODIMP XBFXamlTypeWrapper::CreateFromString(_In_ HSTRING hValue, IInspectable **instance)
{
    RRETURN(E_NOTIMPL);
}

IFACEMETHODIMP XBFXamlTypeWrapper::AddToVector(_In_ IInspectable *instance, _In_ IInspectable *value)
{
    RRETURN(E_NOTIMPL);
}

IFACEMETHODIMP XBFXamlTypeWrapper::AddToMap(_In_ IInspectable *instance, _In_ IInspectable *key, _In_ IInspectable *value)
{
    RRETURN(E_NOTIMPL);
}

XBFXamlMemberWrapper::XBFXamlMemberWrapper()
{
    m_pXamlMember = NULL;
}

XBFXamlMemberWrapper::~XBFXamlMemberWrapper()
{
    ReleaseInterface(m_pXamlMember);
}

_Check_return_ HRESULT XBFXamlMemberWrapper::CreateXBFXamlMemberWrapper(_In_ xaml_markup::IXamlMember* pInXamlMember, _Outptr_ xaml_markup::IXamlMember **ppWrapperXamlMember)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<XBFXamlMemberWrapper> spXamlMemberWrapper;

    *ppWrapperXamlMember = NULL;
    
    if (pInXamlMember)
    {
        IFC(ctl::make<XBFXamlMemberWrapper>(&spXamlMemberWrapper));
        IFC(spXamlMemberWrapper->SetXamlMember(pInXamlMember));
    
        *ppWrapperXamlMember = ctl::query_interface<xaml_markup::IXamlMember>(spXamlMemberWrapper.Get());
    }

Cleanup:
    ReleaseInterface(pInXamlMember);
    RRETURN(hr);
}

HRESULT XBFXamlMemberWrapper::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_markup::IXamlMember)))
    {
        *ppObject = static_cast<xaml_markup::IXamlMember*>(this);
    }
    else 
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}


_Check_return_ HRESULT XBFXamlMemberWrapper::SetXamlMember(_In_ xaml_markup::IXamlMember* pXamlMember)
{
    ReplaceInterface(m_pXamlMember, pXamlMember);

    RRETURN(S_OK);
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_IsAttachable(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlMember->get_IsAttachable(value));
Cleanup:
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_IsDependencyProperty(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlMember->get_IsDependencyProperty(value));
Cleanup:
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_IsReadOnly(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlMember->get_IsReadOnly(value));
Cleanup:
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_Name(_Outptr_ HSTRING *value)
{
    HRESULT hr = S_OK;

    IFC(m_pXamlMember->get_Name(value));
Cleanup:
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_TargetType(_Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType *pInXamlType = NULL;

    IFC(m_pXamlMember->get_TargetType(&pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::get_Type(_Outptr_ xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    xaml_markup::IXamlType *pInXamlType = NULL;

    IFC(m_pXamlMember->get_Type(&pInXamlType));
    IFC(XBFXamlTypeWrapper::CreateXBFXamlTypeWrapper(pInXamlType, value));
    pInXamlType = NULL;

Cleanup:
    ReleaseInterface(pInXamlType);
    RRETURN(hr);    
}

IFACEMETHODIMP XBFXamlMemberWrapper::GetValue(_In_ IInspectable *instance, _Outptr_ IInspectable **value)
{
    RRETURN(E_NOTIMPL);
}

IFACEMETHODIMP XBFXamlMemberWrapper::SetValue(_In_ IInspectable *instance, _In_ IInspectable *value)
{
    RRETURN(E_NOTIMPL);
}
