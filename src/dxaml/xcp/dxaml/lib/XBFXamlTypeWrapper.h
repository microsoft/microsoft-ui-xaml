// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Creates a wrapper layer over a IXamlMetadataProvider, IXamlType 
//      and IXamlMember to prevent projections and interface callouts
//      that are not required for XBF Generation.

#pragma once

namespace DirectUI
{
    class XBFXamlMetadataProviderWrapper: public xaml_markup::IXamlMetadataProvider, public ctl::ComBase
    {
        INSPECTABLE_CLASS(L"DirectUI.XBFXamlMetadataProviderWrapper");

        BEGIN_INTERFACE_MAP(XBFXamlMetadataProviderWrapper, ctl::ComBase)
            INTERFACE_ENTRY(XBFXamlMetadataProviderWrapper, xaml_markup::IXamlMetadataProvider)
        END_INTERFACE_MAP(XBFXamlMetadataProviderWrapper, ctl::ComBase)

    public:
        XBFXamlMetadataProviderWrapper();
        ~XBFXamlMetadataProviderWrapper() override;

        static _Check_return_ 
        HRESULT CreateXBFXamlMetadataProviderWrapper(
            _In_     xaml_markup::IXamlMetadataProvider  *pInXamlMetadataProvider,
            _Outptr_ xaml_markup::IXamlMetadataProvider **ppWrapperXamlMetadataProvider
        );

        _Check_return_ HRESULT SetXamlMetadataProvider(_In_ xaml_markup::IXamlMetadataProvider* pXamlMetadataProvider);
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        IFACEMETHOD(GetXamlType)(_In_ wxaml_interop::TypeName type, _Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(GetXamlTypeByFullName)(_In_ HSTRING fullName, _Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(GetXmlnsDefinitions)(_Out_ UINT32* definitionsSize, _Outptr_result_buffer_(*definitionsSize) xaml_markup::XmlnsDefinition **definitions);

    private:
        xaml_markup::IXamlMetadataProvider* m_pXamlMetadataProvider;
    };

    class XBFXamlTypeWrapper: public xaml_markup::IXamlType, public ctl::ComBase
    {
        INSPECTABLE_CLASS(L"DirectUI.XBFXamlTypeWrapper");

        BEGIN_INTERFACE_MAP(XBFXamlTypeWrapper, ctl::ComBase)
            INTERFACE_ENTRY(XBFXamlTypeWrapper, xaml_markup::IXamlType)
        END_INTERFACE_MAP(XBFXamlTypeWrapper, ctl::ComBase)

    public:
        XBFXamlTypeWrapper();
        ~XBFXamlTypeWrapper() override;

        static _Check_return_ 
        HRESULT CreateXBFXamlTypeWrapper(
            _In_     xaml_markup::IXamlType  *pInXamlType,
            _Outptr_ xaml_markup::IXamlType **ppWrapperXamlType
        );

        _Check_return_ HRESULT SetXamlType(_In_ xaml_markup::IXamlType* pXamlType);
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        IFACEMETHOD(get_BaseType)(_Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(get_ContentProperty)(_Outptr_ xaml_markup::IXamlMember **value);
        IFACEMETHOD(get_FullName)(_Outptr_ HSTRING *value);
        IFACEMETHOD(get_IsArray)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsCollection)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsConstructible)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsDictionary)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsMarkupExtension)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsBindable)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_ItemType)(_Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(get_KeyType)(_Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(get_BoxedType)(_Outptr_ xaml_markup::IXamlType** value);
        IFACEMETHOD(get_UnderlyingType)(_Out_ wxaml_interop::TypeName *value);
        IFACEMETHOD(ActivateInstance)(_Outptr_ IInspectable **instance);
        IFACEMETHOD(RunInitializer)();
        IFACEMETHOD(GetMember)(_In_ HSTRING name, _Outptr_ xaml_markup::IXamlMember **xamlMember);
        IFACEMETHOD(AddToVector)(_In_ IInspectable *instance, _In_ IInspectable *value);
        IFACEMETHOD(CreateFromString)(_In_ HSTRING hValue, IInspectable **instance);
        IFACEMETHOD(AddToMap)(_In_ IInspectable *instance, _In_ IInspectable *key, _In_ IInspectable *value);

    private:
        xaml_markup::IXamlType* m_pXamlType{};
    };

    class XBFXamlMemberWrapper: public xaml_markup::IXamlMember, public ctl::ComBase
    {
        INSPECTABLE_CLASS(L"DirectUI.XBFXamlMemberWrapper");

        BEGIN_INTERFACE_MAP(XBFXamlMemberWrapper, ctl::ComBase)
            INTERFACE_ENTRY(XBFXamlMemberWrapper, xaml_markup::IXamlMember)
        END_INTERFACE_MAP(XBFXamlMemberWrapper, ctl::ComBase)

    public:
        XBFXamlMemberWrapper();
        ~XBFXamlMemberWrapper() override;

        static _Check_return_ 
        HRESULT CreateXBFXamlMemberWrapper(
            _In_     xaml_markup::IXamlMember  *pInXamlMember,
            _Outptr_ xaml_markup::IXamlMember **ppWrapperXamlMember
        );

        _Check_return_ HRESULT SetXamlMember(_In_ xaml_markup::IXamlMember* pXamlMember);
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        IFACEMETHOD(get_IsAttachable)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsDependencyProperty)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_IsReadOnly)(_Out_ BOOLEAN *value);
        IFACEMETHOD(get_Name)(_Outptr_ HSTRING *value);
        IFACEMETHOD(get_TargetType)(_Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(get_Type)(_Outptr_ xaml_markup::IXamlType **value);
        IFACEMETHOD(GetValue)(_In_ IInspectable *instance, _Outptr_ IInspectable **value);
        IFACEMETHOD(SetValue)(_In_ IInspectable *instance, _In_ IInspectable *value);

    private:
        xaml_markup::IXamlMember* m_pXamlMember;
    };
}
