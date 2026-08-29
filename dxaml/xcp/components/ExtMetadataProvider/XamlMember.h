// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <Microsoft.UI.Xaml.coretypes.h>

namespace Private
{
    class XamlTypeInfoProvider;
    struct UserMemberInfo;
}

namespace Internal
{
    class XamlMember
        : public wrl::RuntimeClass<
            xaml_markup::IXamlMember>
    {
        InspectableClass(nullptr /* this class is internal */, BaseTrust);

    public:
        XamlMember(
            UINT memberIndex,
            _In_ Private::XamlTypeInfoProvider* typeInfoProvider);

        IFACEMETHOD(get_IsAttachable)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsDependencyProperty)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_IsReadOnly)(
            _Out_ BOOLEAN* value);

        IFACEMETHOD(get_Name)(
            _Outptr_ HSTRING* value);

        IFACEMETHOD(get_TargetType)(
            _COM_Outptr_ xaml_markup::IXamlType** value);

        IFACEMETHOD(get_Type)(
            _COM_Outptr_ xaml_markup::IXamlType** value);

        IFACEMETHOD(GetValue)(
            _In_ IInspectable* instance,
            _Outptr_ IInspectable** value);

        IFACEMETHOD(SetValue)(
            _In_ IInspectable* instance,
            _In_ IInspectable* value);

    private:
        Private::XamlTypeInfoProvider* m_pTypeInfoProvider;

        const Private::UserMemberInfo* m_pUserMemberData;
    };
}

