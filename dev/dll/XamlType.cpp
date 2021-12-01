// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "XamlType.h"
#include "XamlMember.h"
#include "XamlMetadataProvider.h"

void XamlTypeBase::AddDPMember(
    wstring_view const& name,
    wstring_view const& baseTypeName,
    winrt::DependencyProperty const& dp,
    bool isContent)
{
    MUX_ASSERT(dp);
    AddMember(
        name,
        baseTypeName,
        [dp](winrt::IInspectable instance) 
            { 
                return instance.as<winrt::DependencyObject>().GetValue(dp); 
            },
        [dp](winrt::IInspectable instance, winrt::IInspectable value)
            {
                return instance.as<winrt::DependencyObject>().SetValue(dp, value);
            },
        isContent, 
        true /* isDependencyProperty */,
        false /* isAttachable */
         );
}

winrt::IXamlType XamlTypeBase::BaseType()
{
    return XamlMetadataProvider::GetXamlType(m_baseTypeName);
}

winrt::IXamlMember XamlTypeBase::ContentProperty()
{
    EnsureProperties();
    return m_contentProperty;
}

winrt::hstring XamlTypeBase::FullName()
{
    return m_typeName;
}

bool XamlTypeBase::IsArray()
{
    return false;
}

bool XamlTypeBase::IsCollection()
{
    return static_cast<bool>(m_collectionAdd);
}

bool XamlTypeBase::IsConstructible()
{
    return static_cast<bool>(m_activator);
}

bool XamlTypeBase::IsDictionary()
{
    return static_cast<bool>(m_addToMap);
}

bool XamlTypeBase::IsMarkupExtension()
{
    return false;
}

bool XamlTypeBase::IsBindable()
{
    return false;
}

winrt::IXamlType XamlTypeBase::ItemType()
{
    return nullptr;
}

winrt::IXamlType XamlTypeBase::KeyType()
{
    return nullptr;
}

winrt::TypeName XamlTypeBase::UnderlyingType()
{
    winrt::TypeName typeName;
    typeName.Name = m_typeName;
    typeName.Kind =
        m_isSystemType ?
        winrt::TypeKind::Primitive:
        winrt::TypeKind::Metadata;
    return typeName;
}

winrt::IInspectable XamlTypeBase::ActivateInstance()
{
    if (m_activator)
    {
        return m_activator();
    }

    return nullptr;
}

winrt::IInspectable XamlTypeBase::CreateFromString(winrt::hstring const& value)
{
    if (m_createFromString)
    {
        return m_createFromString(value);
    }

    return nullptr;
}

winrt::IXamlMember XamlTypeBase::GetMember(winrt::hstring const& name)
{
    EnsureProperties();

    for (const auto& member : m_members)
    {
        if (member.Name() == name)
        {
            return member;
        }
    }

    return nullptr;
}

void XamlTypeBase::AddToVector(winrt::IInspectable const& instance, winrt::IInspectable const& value)
{
    if (m_collectionAdd)
    {
        m_collectionAdd(instance, value);
    }
}

void XamlTypeBase::AddToMap(winrt::IInspectable const& instance, winrt::IInspectable const& key, winrt::IInspectable const& value)
{
    if (m_addToMap)
    {
        m_addToMap(instance, key, value);
    }
}

void XamlTypeBase::RunInitializer()
{
}

void XamlTypeBase::AddMember(
    wstring_view const& name,
    wstring_view const& baseTypeName,
    std::function<winrt::IInspectable(const winrt::IInspectable&)> getter,
    std::function<void(const winrt::IInspectable&, const winrt::IInspectable&)> setter,
    bool isContent, 
    bool isDependencyProperty,
    bool isAttachable
)
{
    auto baseType = XamlMetadataProvider::GetXamlType(baseTypeName);

    auto member = 
        winrt::make<XamlMember>(
            name,
            baseType,
            getter,
            setter, 
            isDependencyProperty,
            isAttachable);

    m_members.push_back(member);
    if (isContent)
    {
        MUX_ASSERT(!m_contentProperty);
        m_contentProperty = member;
    }
}

void XamlTypeBase::EnsureProperties()
{
    if (m_populatePropertiesFunc)
    {
        m_populatePropertiesFunc(*this);
        m_populatePropertiesFunc = nullptr;
    }
}

XamlType::XamlType(
    wstring_view const& typeName,
    wstring_view const& baseTypeName,
    std::function<winrt::IInspectable()> activator,
    std::function<void(XamlTypeBase&)> populatePropertiesFunc
    )
{
    m_typeName = typeName;
    m_baseTypeName = baseTypeName;
    m_activator = activator;
    m_populatePropertiesFunc = populatePropertiesFunc;
}

void XamlType::SetCollectionAddFunc(std::function<void(winrt::IInspectable const&, winrt::IInspectable const&)> collectionAdd)
{
    m_collectionAdd = collectionAdd;
}

void XamlType::SetAddToMapFunc(std::function<void(winrt::IInspectable const&, winrt::IInspectable const&, winrt::IInspectable const&)> addToMap)
{
    m_addToMap = addToMap;
}

EnumXamlType::EnumXamlType(
    wstring_view const& typeName,
    std::function<winrt::IInspectable(hstring)> createFromString
    )
{
    m_typeName = typeName;
    m_baseTypeName = winrt::name_of<int32_t>();
    m_createFromString = createFromString;
}
