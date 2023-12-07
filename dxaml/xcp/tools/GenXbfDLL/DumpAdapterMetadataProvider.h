// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class DumpAdapterMember: public xaml_markup::IXamlMember, public ctl::ComBase
{
    INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Markup.DumpAdapterMember");

    BEGIN_INTERFACE_MAP(DumpAdapterMember, ctl::ComBase)
        INTERFACE_ENTRY(DumpAdapterMember, xaml_markup::IXamlMember)
    END_INTERFACE_MAP(DumpAdapterMember, ctl::ComBase)

private:
    HSTRING m_strName;

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject);

public:
    void SetName(HSTRING strName)
    {
        m_strName = strName;
    }

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsAttachable(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsDependencyProperty(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsReadOnly(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Name(
        /* [out][retval] */ __RPC__deref_out_opt HSTRING *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_TargetType(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_Type(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value);

    virtual HRESULT STDMETHODCALLTYPE GetValue(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **value);

    virtual HRESULT STDMETHODCALLTYPE SetValue(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *value);
};

class DumpAdapterType: public xaml_markup::IXamlType, public ctl::ComBase
{
    INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Markup.DumpAdapterType");

    BEGIN_INTERFACE_MAP(DumpAdapterType, ctl::ComBase)
        INTERFACE_ENTRY(DumpAdapterType, xaml_markup::IXamlType)
    END_INTERFACE_MAP(DumpAdapterType, ctl::ComBase)

private:
    HSTRING m_strFullName;

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject);

public:
    void SetFullName(HSTRING strFullName)
    {
        m_strFullName = strFullName;
    }

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BaseType(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ContentProperty(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_FullName(
        /* [out][retval] */ __RPC__deref_out_opt HSTRING *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsArray(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsCollection(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsConstructible(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsDictionary(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsMarkupExtension(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_IsBindable(
        /* [out][retval] */ __RPC__out boolean *value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_ItemType(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_KeyType(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_BoxedType(
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType** value);

    virtual /* [propget] */ HRESULT STDMETHODCALLTYPE get_UnderlyingType(
        /* [out][retval] */ __RPC__out wxaml_interop::TypeName *value);

    virtual HRESULT STDMETHODCALLTYPE ActivateInstance(
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance);

    virtual HRESULT STDMETHODCALLTYPE CreateFromString(
        /* [in] */ __RPC__in HSTRING value,
        /* [out][retval] */ __RPC__deref_out_opt IInspectable **instance);

    virtual HRESULT STDMETHODCALLTYPE GetMember(
        /* [in] */ __RPC__in HSTRING name,
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlMember **xamlMember);

    virtual HRESULT STDMETHODCALLTYPE AddToVector(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *value);

    virtual HRESULT STDMETHODCALLTYPE AddToMap(
        /* [in] */ __RPC__in_opt IInspectable *instance,
        /* [in] */ __RPC__in_opt IInspectable *key,
        /* [in] */ __RPC__in_opt IInspectable *value);

    virtual HRESULT STDMETHODCALLTYPE RunInitializer();
};

class DumpAdapterMetadataProvider: public xaml_markup::IXamlMetadataProvider, public ctl::ComBase
{
    INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Markup.DumpAdapterMetadataProvider");

    BEGIN_INTERFACE_MAP(DumpAdapterMetadataProvider, ctl::ComBase)
        INTERFACE_ENTRY(DumpAdapterMetadataProvider, xaml_markup::IXamlMetadataProvider)
    END_INTERFACE_MAP(DumpAdapterMetadataProvider, ctl::ComBase)

protected:
    HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppObject);

public:
    virtual HRESULT STDMETHODCALLTYPE GetXamlType(
        /* [in] */ wxaml_interop::TypeName type,
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType);

    virtual HRESULT STDMETHODCALLTYPE GetXamlTypeByFullName(
        /* [in] */ __RPC__in HSTRING fullName,
        /* [out][retval] */ __RPC__deref_out_opt xaml_markup::IXamlType **xamlType);

    virtual HRESULT STDMETHODCALLTYPE GetXmlnsDefinitions(
        /* [out] */ __RPC__out UINT *length,
        /* [out][size_is][size_is][retval] */ __RPC__deref_out_ecount_full_opt(*length) xaml_markup::XmlnsDefinition **definitions);
};
