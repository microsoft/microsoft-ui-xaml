// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  A sample XBF type provider that attempts to mimic the metadata information that
//  Visual Studio may provide to the Xbf Generator.

#pragma once
#include "Microsoft.UI.Xaml.coretypes.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools { namespace XbfGenerator {

    enum class KnownXbfTypeIndex
    {
        System_Object,
        System_ItemsControl,
        System_ContentControl,
        Pivot,
        PivotItem
    };

    enum class KnownXbfMemberIndex
    {
        Pivot_Title,
        PivotItem_Header,
    };

    class SampleXbfMetadataProvider : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IXbfMetadataProvider>
    {
    public:
        ~SampleXbfMetadataProvider();

        STDMETHODIMP GetXamlType(_In_ wxaml_interop::TypeName, _Out_ IXbfType **);
        STDMETHODIMP GetXamlTypeByFullName(_In_ BSTR hFullName, _Out_ IXbfType **xamlType);
        STDMETHODIMP GetXmlnsDefinitions(_In_ UINT *,_Out_ xaml_markup::XmlnsDefinition **);

        HRESULT RuntimeClassInitialize();

    private:
        wrl::ComPtr<xaml_markup::IXamlMetadataProvider> m_xmp;
        wrl::ComPtr<xaml_markup::IXamlMetadataProvider> m_muxcXmp;
        wil::unique_hmodule m_hModuleMuxc;
    };

    class XbfType : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IXbfType>
    {
    public:
        XbfType(_In_ const wrl::ComPtr<xaml_markup::IXamlType>& type) : m_xamlType(type) {}

        STDMETHODIMP get_BaseType(_Out_ IXbfType **xamlType);
        STDMETHODIMP get_ContentProperty(_Out_ IXbfMember **xamlMember);
        STDMETHODIMP get_FullName(_Out_ BSTR *hFullName);
        STDMETHODIMP get_IsArray(_Out_ boolean *result);
        STDMETHODIMP get_IsCollection(_Out_ boolean *result);
        STDMETHODIMP get_IsConstructible(_Out_ boolean *result);
        STDMETHODIMP get_IsDictionary(_Out_ boolean *result);
        STDMETHODIMP get_IsMarkupExtension(_Out_ boolean *result);
        STDMETHODIMP get_IsBindable(_Out_ boolean *result);
        STDMETHODIMP get_ItemType(_Out_ IXbfType **xamlType);
        STDMETHODIMP get_KeyType(_Out_ IXbfType **xamlType);
        STDMETHODIMP get_BoxedType(_Out_ IXbfType** xamlType);
        STDMETHODIMP get_UnderlyingType(_Out_ wxaml_interop::TypeName *xamlType);
        STDMETHODIMP GetMember(_In_ BSTR hFullName, _Out_ IXbfMember **xamlMember);

        STDMETHODIMP ActivateInstance(_Out_ IInspectable **);
        STDMETHODIMP CreateFromString(_In_ BSTR, _Out_ IInspectable **);
        STDMETHODIMP AddToVector(_In_ IInspectable *, _In_ IInspectable *);
        STDMETHODIMP AddToMap(_In_ IInspectable *, _In_ IInspectable *, _In_ IInspectable *);
        STDMETHODIMP RunInitializer(void);

    private:
        wrl::ComPtr<xaml_markup::IXamlType> m_xamlType;
    };

    class XbfMember : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>, IXbfMember>
    {
    public:
        XbfMember(_In_ const wrl::ComPtr<xaml_markup::IXamlMember> member) : m_xamlMember(member) {}

        STDMETHODIMP get_IsAttachable(_Out_ boolean *result);
        STDMETHODIMP get_IsDependencyProperty(_Out_ boolean *result);
        STDMETHODIMP get_IsReadOnly(_Out_ boolean *result);
        STDMETHODIMP get_Name(_Out_ BSTR *hFullName);
        STDMETHODIMP get_TargetType(_Out_ IXbfType **xamlType);
        STDMETHODIMP get_Type(_Out_ IXbfType **xamlType);
        STDMETHODIMP GetValue(_In_ IInspectable *, _Out_ IInspectable **);
        STDMETHODIMP SetValue(_In_ IInspectable *, _In_ IInspectable *);

    private:
        wrl::ComPtr<xaml_markup::IXamlMember> m_xamlMember;
    };

} } } } } }
