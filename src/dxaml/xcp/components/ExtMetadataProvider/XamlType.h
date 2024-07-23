// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <Microsoft.UI.Xaml.coretypes.h>
#include <XamlTypeInfo.h>

namespace Private
{
    class XamlTypeInfoProvider;
}

namespace Internal
{
    class XamlType
        : public wrl::RuntimeClass<
            xaml_markup::IXamlType>
    {
        InspectableClass(nullptr /* this class is internal */, BaseTrust);

    public:
        XamlType(
            UINT typeIndex,
            _In_ Private::XamlTypeInfoProvider* typeInfoProvider);

        IFACEMETHOD(get_BaseType)(
            _COM_Outptr_ xaml_markup::IXamlType** value);

        IFACEMETHOD(get_ContentProperty)(
            _COM_Outptr_result_maybenull_ xaml_markup::IXamlMember** value);

        IFACEMETHOD(get_FullName)(
            _Outptr_ HSTRING* value);

        IFACEMETHOD(get_IsArray)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsCollection)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsConstructible)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsDictionary)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsMarkupExtension)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsSystemType)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsBindable)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_ItemType)(
            _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value);

        IFACEMETHOD(get_KeyType)(
            _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value);

        IFACEMETHOD(get_BoxedType)(
            _COM_Outptr_result_maybenull_ xaml_markup::IXamlType** value);

        IFACEMETHOD(get_Name)(
            _Outptr_ HSTRING* value);

        IFACEMETHOD(get_UnderlyingType)(
            _Out_ wxaml_interop::TypeName* value);

        IFACEMETHOD(ActivateInstance)(
            _Outptr_ IInspectable** instance);

        IFACEMETHOD(RunInitializer)();

        IFACEMETHOD(GetMember)(
            _In_ HSTRING name,
            _COM_Outptr_result_maybenull_ xaml_markup::IXamlMember** xamlMember);

        IFACEMETHOD(CreateFromString)(
            _In_ HSTRING hValue,
            _Outptr_ IInspectable** instance);

        IFACEMETHOD(AddToVector)(
            _In_ IInspectable* instance,
            _In_ IInspectable* value);

        IFACEMETHOD(AddToMap)(
            _In_ IInspectable* instance,
            _In_ IInspectable* key,
            _In_ IInspectable* value);

    private:
        bool IsSystemType()     { return (m_pUserTypeData == nullptr); }
        bool IsValueType()      { return (m_pUserTypeData->activatorId == 0);}
        bool IsEnum()           { return (m_pUserTypeData->iEnumIndex != -1); }

    private:
        Private::XamlTypeInfoProvider* m_pTypeInfoProvider{};

        const Private::XamlTypeName* m_pTypeName{};
        const Private::UserTypeInfo* m_pUserTypeData{};
        const INT32* m_piMemberIndices{};
    };
}

