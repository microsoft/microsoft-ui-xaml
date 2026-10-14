// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "XamlMember.h"
#include "XamlMetadataProvider.h"

XamlMember::XamlMember(
    wstring_view const& memberName,
    wstring_view const& typeName,
    std::function<winrt::IInspectable(const winrt::IInspectable&)> getter,
    std::function<void(const winrt::IInspectable&, const winrt::IInspectable&)> setter,
    bool isDependencyProperty,
    bool isAttachable)
{
    m_memberName = memberName;
    m_typeName = typeName;
    m_getter = getter;
    m_setter = setter;
    m_isDependencyProperty = isDependencyProperty;
    m_isAttachable = isAttachable;
}

bool XamlMember::IsAttachable()
{
    return m_isAttachable;
}

bool XamlMember::IsDependencyProperty()
{
    return m_isDependencyProperty;
}

bool XamlMember::IsReadOnly()
{
    return !static_cast<bool>(m_setter);
}

winrt::hstring XamlMember::Name()
{
    return m_memberName;
}

winrt::IXamlType XamlMember::ResolveType()
{
    if (!m_type)
    {
        m_type = XamlMetadataProvider::LookupXamlType(m_typeName);
    }
    return m_type;
}

winrt::IXamlType XamlMember::TargetType()
{
    return ResolveType();
}

winrt::IXamlType XamlMember::Type()
{
    return ResolveType();
}

winrt::IInspectable XamlMember::GetValue(winrt::IInspectable const& instance)
{
    if (m_getter)
    {
        return m_getter(instance);
    }
    return nullptr;
}

void XamlMember::SetValue(winrt::IInspectable const& instance, winrt::IInspectable const& value)
{
    if (m_setter)
    {
        m_setter(instance, value);
    }
}
