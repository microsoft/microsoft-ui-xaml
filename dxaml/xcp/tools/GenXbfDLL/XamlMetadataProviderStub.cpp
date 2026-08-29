// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace ::Windows::Internal;
using namespace DirectUI;
using namespace DirectUISynonyms;

HRESULT XamlMemberAdapter::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlMember)
    {
        *ppObject = static_cast<xaml_markup::IXamlMember *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        RRETURN(ctl::ComBase::QueryInterfaceImpl(riid, ppObject));
    }
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_IsAttachable(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlMember->get_IsAttachable(value));
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_IsDependencyProperty(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlMember->get_IsDependencyProperty(value));
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_IsReadOnly(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlMember->get_IsReadOnly(value));
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_Name(
    /* [out][retval] */ __RPC__deref_out_opt HSTRING *value)
{
    HRESULT hr = S_OK;
    BSTR bstrValue = NULL;

    IFC(m_pXamlMember->get_Name(&bstrValue));
    IFC(WindowsCreateString(bstrValue, SysStringLen(bstrValue), value));

Cleanup:
    SysFreeString(bstrValue);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_TargetType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlMember->get_TargetType(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }
Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::get_Type(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlMember->get_Type(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::GetValue(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **value)
{
    RRETURN(m_pXamlMember->GetValue(instance, value));
}

HRESULT STDMETHODCALLTYPE XamlMemberAdapter::SetValue(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    RRETURN(m_pXamlMember->SetValue(instance, value));
}

HRESULT XamlTypeAdapter::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlType)
    {
        *ppObject = static_cast<xaml_markup::IXamlType *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        RRETURN(ctl::ComBase::QueryInterfaceImpl(riid, ppObject));
    }
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_BaseType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlType->get_BaseType(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_ContentProperty(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **value)
{
    HRESULT hr = S_OK;
    IXbfMember *pXamlMember = NULL;
    ctl::ComPtr<XamlMemberAdapter> spXamlMemberAdapter;

    IFC(m_pXamlType->get_ContentProperty(&pXamlMember));
    if (pXamlMember != NULL)
    {
        IFC(ctl::make(&spXamlMemberAdapter));
        spXamlMemberAdapter->SetXamlMember(pXamlMember);
        *value = spXamlMemberAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlMember);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_FullName(
    /* [out][retval] */ __RPC__deref_out_opt HSTRING *value)
{
    HRESULT hr = S_OK;
    BSTR bstrValue = NULL;

    IFC(m_pXamlType->get_FullName(&bstrValue));
    IFC(WindowsCreateString(bstrValue, SysStringLen(bstrValue), value));

Cleanup:
    SysFreeString(bstrValue);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsArray(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsArray(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsCollection(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsCollection(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsConstructible(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsConstructible(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsDictionary(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsDictionary(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsMarkupExtension(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsMarkupExtension(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_IsBindable(
    /* [out][retval] */ __RPC__out boolean *value)
{
    RRETURN(m_pXamlType->get_IsBindable(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_ItemType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlType->get_ItemType(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_KeyType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlType->get_KeyType(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_BoxedType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType** value)
{
    HRESULT hr = S_OK;
    IXbfType* pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;

    IFC(m_pXamlType->get_BoxedType(&pXamlType));
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *value = spXamlTypeAdapter.Detach();
    }
    else
    {
        *value = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::get_UnderlyingType(
    /* [out][retval] */ __RPC__out wxaml_interop::TypeName *value)
{
    HRESULT hr = S_OK;

    IFC(get_FullName(&value->Name));
    value->Kind = wxaml_interop::TypeKind_Metadata;

Cleanup:
    RRETURN(hr);

//    RRETURN(m_pXamlType->get_UnderlyingType(value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::ActivateInstance(
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance)
{
    RRETURN(m_pXamlType->ActivateInstance(instance));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::CreateFromString(
    /* [in] */ __RPC__in HSTRING value,
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance)
{
    HRESULT hr = S_OK;
    BSTR bstrValue = NULL;
    UINT32 length = 0;

    const WCHAR *pszStr = WindowsGetStringRawBuffer(value, &length);
    bstrValue = SysAllocStringLen(pszStr, length);
    IFCOOMFAILFAST(bstrValue);
    IFC(m_pXamlType->CreateFromString(bstrValue, instance));

Cleanup:
    SysFreeString(bstrValue);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::GetMember(
    /* [in] */ __RPC__in HSTRING name,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **xamlMember)
{
    HRESULT hr = S_OK;
    BSTR bstrName = NULL;
    UINT32 length = 0;
    IXbfMember *pXamlMember = NULL;
    ctl::ComPtr<XamlMemberAdapter> spXamlMemberAdapter;

    const WCHAR *pszStr = WindowsGetStringRawBuffer(name, &length);
    bstrName = SysAllocStringLen(pszStr, length);
    IFCOOMFAILFAST(bstrName);
    IFC(m_pXamlType->GetMember(bstrName, &pXamlMember));
    if (pXamlMember != NULL)
    {
        IFC(ctl::make(&spXamlMemberAdapter));
        spXamlMemberAdapter->SetXamlMember(pXamlMember);
        *xamlMember = spXamlMemberAdapter.Detach();
    }
    else
    {
        *xamlMember = NULL;
    }

Cleanup:
    ReleaseInterface(pXamlMember);
    SysFreeString(bstrName);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::AddToVector(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    RRETURN(m_pXamlType->AddToVector(instance, value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::AddToMap(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *key,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    RRETURN(m_pXamlType->AddToMap(instance, key, value));
}

HRESULT STDMETHODCALLTYPE XamlTypeAdapter::RunInitializer()
{
    RRETURN(m_pXamlType->RunInitializer());
}

HRESULT XamlMetadataProviderAdapter::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlMetadataProvider)
    {
        *ppObject = static_cast<xaml_markup::IXamlMetadataProvider *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        RRETURN(ctl::ComBase::QueryInterfaceImpl(riid, ppObject));
    }
}

HRESULT STDMETHODCALLTYPE XamlMetadataProviderAdapter::GetXamlType(
    /* [in] */ wxaml_interop::TypeName type,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType)
{
    HRESULT hr = S_OK;

    IFC(GetXamlTypeByFullName(type.Name, xamlType));

Cleanup:
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlMetadataProviderAdapter::GetXamlTypeByFullName(
    /* [in] */ __RPC__in HSTRING fullName,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType)
{
    HRESULT hr = S_OK;
    IXbfType *pXamlType = NULL;
    ctl::ComPtr<XamlTypeAdapter> spXamlTypeAdapter;
    BSTR bstrFullName = NULL;
    UINT32 length = 0;

    const WCHAR *pszStr = WindowsGetStringRawBuffer(fullName, &length);
    bstrFullName = SysAllocStringLen(pszStr, length);
    IFCOOMFAILFAST(bstrFullName);
    if (m_pXmp)
    {
        IFC(m_pXmp->GetXamlTypeByFullName(bstrFullName, &pXamlType));
    }
    if (pXamlType != NULL)
    {
        IFC(ctl::make(&spXamlTypeAdapter));
        spXamlTypeAdapter->SetXamlType(pXamlType);
        *xamlType = spXamlTypeAdapter.Detach();
    }
    else
    {
        *xamlType = NULL;
    }

Cleanup:
    SysFreeString(bstrFullName);
    ReleaseInterface(pXamlType);
    RRETURN(hr);
}

HRESULT STDMETHODCALLTYPE XamlMetadataProviderAdapter::GetXmlnsDefinitions(
    /* [out] */ __RPC__out UINT *length,
    /* [out][size_is][size_is][retval] */ __RPC__deref_out_ecount_full_opt(*length) xaml_markup::XmlnsDefinition **definitions)
{
    RRETURN(m_pXmp->GetXmlnsDefinitions(length, definitions));
}
