// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CustomProperty.h"

CustomProperty::CustomProperty(
    winrt::hstring const& name,
    winrt::TypeName const& typeName,
    std::function<winrt::IInspectable(winrt::IInspectable const&)> getter,
    std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> setter)
{
    m_name = name;
    m_typeName = typeName;
    m_getter = getter;
    m_setter = setter;
}

bool CustomProperty::CanRead()
{
    return m_getter != nullptr;
}

bool CustomProperty::CanWrite()
{
    return m_setter != nullptr;
}

winrt::hstring CustomProperty::Name()
{
    return m_name;
}

winrt::TypeName CustomProperty::Type()
{
    return m_typeName;
}

winrt::IInspectable CustomProperty::GetValue(winrt::IInspectable const& target)
{
    return m_getter(target);
}

void CustomProperty::SetValue(winrt::IInspectable const& target, winrt::IInspectable const& value)
{
    m_setter(target, value);
}

winrt::IInspectable CustomProperty::GetIndexedValue(winrt::IInspectable const& target, winrt::IInspectable const& index)
{
    throw winrt::hresult_not_implemented();
}

void CustomProperty::SetIndexedValue(winrt::IInspectable const& target, winrt::IInspectable const& value, winrt::IInspectable const& index)
{
    throw winrt::hresult_not_implemented();
}
