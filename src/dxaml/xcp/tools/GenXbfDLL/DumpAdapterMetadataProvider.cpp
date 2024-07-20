// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// IXamlMetadataProvider COM Stub
//   Adapter layer to redirect IXMP calls to report dummy data.

#include "precomp.h"
#include "DumpAdapterMetadataProvider.h"
#include <xstring_ptr.h>

using namespace ::Windows::Internal;
using namespace DirectUI;
using namespace DirectUISynonyms;

HRESULT DumpAdapterMember::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlMember)
    {
        *ppObject = static_cast<xaml_markup::IXamlMember *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_IsAttachable(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_IsDependencyProperty(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_IsReadOnly(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_Name(
    /* [out][retval] */ __RPC__deref_out_opt HSTRING *value)
{
    IFC_RETURN(WindowsDuplicateString(m_strName, value));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_TargetType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::get_Type(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::GetValue(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMember::SetValue(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    return S_OK;
}

HRESULT DumpAdapterType::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlType)
    {
        *ppObject = static_cast<xaml_markup::IXamlType *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_BaseType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_ContentProperty(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_FullName(
    /* [out][retval] */ __RPC__deref_out_opt HSTRING *value)
{
    IFC_RETURN(WindowsDuplicateString(m_strFullName, value));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsArray(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsCollection(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsConstructible(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsDictionary(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsMarkupExtension(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_IsBindable(
    /* [out][retval] */ __RPC__out boolean *value)
{
    *value = false;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_ItemType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_KeyType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value)
{
    *value = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::get_BoxedType(
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType** value)
{
    *value = nullptr;

    return S_OK;
}


HRESULT STDMETHODCALLTYPE DumpAdapterType::get_UnderlyingType(
    /* [out][retval] */ __RPC__out wxaml_interop::TypeName *value)
{
    IFC_RETURN(get_FullName(&value->Name));
    value->Kind = wxaml_interop::TypeKind_Metadata;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::ActivateInstance(
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance)
{
    *instance = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::CreateFromString(
    /* [in] */ __RPC__in HSTRING value,
    /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance)
{
    *instance = nullptr;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::GetMember(
    /* [in] */ __RPC__in HSTRING name,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **xamlMember)
{
    ctl::ComPtr<DumpAdapterMember> spXamlMemberAdapter;

    IFC_RETURN(ctl::make(&spXamlMemberAdapter));
    spXamlMemberAdapter->SetName(name);
    *xamlMember = spXamlMemberAdapter.Detach();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::AddToVector(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::AddToMap(
    /* [in] */ __RPC__in_opt IInspectable *instance,
    /* [in] */ __RPC__in_opt IInspectable *key,
    /* [in] */ __RPC__in_opt IInspectable *value)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterType::RunInitializer()
{
    return S_OK;
}

HRESULT DumpAdapterMetadataProvider::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject)
{
    if (riid == xaml_markup::IID_IXamlMetadataProvider)
    {
        *ppObject = static_cast<xaml_markup::IXamlMetadataProvider *>(this);
        AddRefOuter();
        return S_OK;
    }
    else
    {
        return ctl::ComBase::QueryInterfaceImpl(riid, ppObject);
    }
}

HRESULT STDMETHODCALLTYPE DumpAdapterMetadataProvider::GetXamlType(
    /* [in] */ wxaml_interop::TypeName type,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType)
{
    IFC_RETURN(GetXamlTypeByFullName(type.Name, xamlType));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMetadataProvider::GetXamlTypeByFullName(
    /* [in] */ __RPC__in HSTRING fullName,
    /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType)
{
    ctl::ComPtr<DumpAdapterType> spDumpAdapterType;

    IFC_RETURN(ctl::make(&spDumpAdapterType));
    spDumpAdapterType->SetFullName(fullName);
    *xamlType = spDumpAdapterType.Detach();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE DumpAdapterMetadataProvider::GetXmlnsDefinitions(
    /* [out] */ __RPC__out UINT *length,
    /* [out][size_is][size_is][retval] */ __RPC__deref_out_ecount_full_opt(*length) xaml_markup::XmlnsDefinition **definitions)
{
    return S_OK;
}
