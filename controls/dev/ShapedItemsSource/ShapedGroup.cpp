// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedGroup.h"
#include "SharedHelpers.h"

ShapedGroup::ShapedGroup(winrt::IInspectable const& key, winrt::hstring const& groupKey) :
    m_key(key),
    m_groupKey(groupKey),
    m_items(winrt::single_threaded_observable_vector<winrt::IInspectable>())
{
}

winrt::IInspectable ShapedGroup::Group() const
{
    return m_key;
}

winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> ShapedGroup::GroupItems() const
{
    return m_items;
}

winrt::Windows::Foundation::Collections::IIterator<winrt::IInspectable> ShapedGroup::First()
{
    return m_items.First();
}

winrt::hstring ShapedGroup::ToString()
{
    auto text = SharedHelpers::TryGetStringRepresentationFromObject(m_key);
    return text.empty() ? L"<null>" : text;
}

winrt::TypeName ShapedGroup::Type()
{
    winrt::TypeName typeName;
    typeName.Kind = winrt::TypeKind::Custom;
    typeName.Name = L"ShapedGroup";
    return typeName;
}

winrt::Microsoft::UI::Xaml::Data::ICustomProperty ShapedGroup::GetCustomProperty(winrt::hstring const&)
{
    // Deliberately property-less. The two properties this used to expose were magic-string
    // back-channels for the group's identity and key; both are now read through
    // ICollectionViewGroup and GroupKey(). Re-adding one would re-introduce the layering
    // violation those interfaces exist to remove.
    return nullptr;
}

winrt::Microsoft::UI::Xaml::Data::ICustomProperty ShapedGroup::GetIndexedProperty(winrt::hstring const&, winrt::TypeName const&)
{
    return nullptr;
}

winrt::hstring ShapedGroup::GetStringRepresentation()
{
    return ToString();
}

winrt::hstring ShapedGroup::GroupKey() const
{
    return m_groupKey;
}

winrt::hstring ShapedGroup::StableGroupIdentity() const
{
    return m_groupKey;
}

void ShapedGroup::GroupKey(winrt::hstring const& value)
{
    m_groupKey = value;
}

void ShapedGroup::Key(winrt::IInspectable const& value)
{
    m_key = value;
}

void ShapedGroup::SetItems(std::vector<winrt::IInspectable> const& items)
{
    m_items.ReplaceAll(items);
}
