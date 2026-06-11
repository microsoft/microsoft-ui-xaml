// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IXbfType;

MIDL_INTERFACE("d0aa6fc8-087f-46cf-b36a-7e68f8295ceb")
IXbfMember : public IUnknown
{
public:
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsAttachable(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsDependencyProperty(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsReadOnly(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name(
        /* [out][retval] */ __RPC__deref_out_opt BSTR *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetType(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Type(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **value) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetValue(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **value) = 0;

    virtual HRESULT STDMETHODCALLTYPE SetValue(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *value) = 0;

};

MIDL_INTERFACE("a50fc345-4c61-411b-8a68-13da7b7c4ee4")
IXbfType : public IUnknown
{
public:
    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BaseType(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContentProperty(
        /* [out][retval] */ __RPC__deref_out_opt IXbfMember **value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FullName(
        /* [out][retval] */ __RPC__deref_out_opt BSTR *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsArray(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsCollection(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsConstructible(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsDictionary(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsMarkupExtension(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsBindable(
        /* [out][retval] */ __RPC__out boolean *value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ItemType(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_KeyType(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BoxedType(
        /* [out][retval] */ __RPC__deref_out_opt IXbfType** value) = 0;

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UnderlyingType(
        /* [out][retval] */ __RPC__out wxaml_interop::TypeName *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE ActivateInstance(
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance) = 0;

    virtual HRESULT STDMETHODCALLTYPE CreateFromString(
        /* [in] */ __RPC__in BSTR value,
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetMember(
        /* [in] */ __RPC__in BSTR name,
        /* [out][retval] */ __RPC__deref_out_opt IXbfMember **xamlMember) = 0;

    virtual HRESULT STDMETHODCALLTYPE AddToVector(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE AddToMap(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *key,
        /* [in] */ __RPC__in_opt IInspectable *value) = 0;

    virtual HRESULT STDMETHODCALLTYPE RunInitializer( void) = 0;

};

MIDL_INTERFACE("ef46679c-4ec5-447a-bd26-e04f1d2c2551")
IXbfMetadataProvider : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetXamlType(
        /* [in] */ wxaml_interop::TypeName type,
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **xamlType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetXamlTypeByFullName(
        /* [in] */ __RPC__in BSTR fullName,
        /* [out][retval] */ __RPC__deref_out_opt IXbfType **xamlType) = 0;

    virtual HRESULT STDMETHODCALLTYPE GetXmlnsDefinitions(
        /* [out] */ __RPC__out UINT *length,
        /* [out][size_is][size_is][retval] */ __RPC__deref_out_ecount_full_opt(*length) xaml_markup::XmlnsDefinition **definitions) = 0;

};