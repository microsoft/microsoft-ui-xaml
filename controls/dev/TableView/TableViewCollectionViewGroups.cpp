// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewCollectionViewGroups.h"
#include "ShapedGroup.h"
#include "ShapingHelpers.h"
#include "TVDiag.h"

#include <cwchar>
#include <optional>
#include <string>

namespace
{
    uintptr_t CanonicalAddress(winrt::IInspectable const& value)
    {
        if (!value)
        {
            return 0;
        }
        return reinterpret_cast<uintptr_t>(winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>()));
    }

    // The group's key value when its Group object exposes one as a "Key" property (the IGrouping<K,T>
    // idiom, and CollectionViewSource's computed groups), else the Group object itself.
    winrt::IInspectable GroupKey(winrt::IInspectable const& groupObject)
    {
        auto const group = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
        if (!group)
        {
            return groupObject;
        }
        auto key = group.Group();
        if (!key)
        {
            return group;
        }
        if (auto const provider = key.try_as<winrt::Microsoft::UI::Xaml::Data::ICustomPropertyProvider>())
        {
            try
            {
                if (auto const property = provider.GetCustomProperty(L"Key"); property && property.CanRead())
                {
                    if (auto value = property.GetValue(key))
                    {
                        return value;
                    }
                }
            }
            catch (...)
            {
            }
        }
        return key;
    }

    std::optional<winrt::hstring> ScalarIdentity(winrt::IInspectable const& value)
    {
        using winrt::Windows::Foundation::PropertyType;
        auto const scalar = value.try_as<winrt::Windows::Foundation::IPropertyValue>();
        if (!scalar)
        {
            return std::nullopt;
        }

        auto const type = scalar.Type();
        std::wstring text;
        switch (type)
        {
        case PropertyType::String: text = scalar.GetString(); break;
        case PropertyType::Boolean: text = scalar.GetBoolean() ? L"1" : L"0"; break;
        case PropertyType::Char16: text = std::to_wstring(static_cast<uint32_t>(scalar.GetChar16())); break;
        case PropertyType::UInt8: text = std::to_wstring(scalar.GetUInt8()); break;
        case PropertyType::Int16: text = std::to_wstring(scalar.GetInt16()); break;
        case PropertyType::UInt16: text = std::to_wstring(scalar.GetUInt16()); break;
        case PropertyType::Int32: text = std::to_wstring(scalar.GetInt32()); break;
        case PropertyType::UInt32: text = std::to_wstring(scalar.GetUInt32()); break;
        case PropertyType::Int64: text = std::to_wstring(scalar.GetInt64()); break;
        case PropertyType::UInt64: text = std::to_wstring(scalar.GetUInt64()); break;
        case PropertyType::Single: text = std::to_wstring(scalar.GetSingle()); break;
        case PropertyType::Double: text = std::to_wstring(scalar.GetDouble()); break;
        case PropertyType::DateTime: text = std::to_wstring(scalar.GetDateTime().time_since_epoch().count()); break;
        case PropertyType::TimeSpan: text = std::to_wstring(scalar.GetTimeSpan().count()); break;
        case PropertyType::Guid:
        {
            wchar_t buffer[40]{};
            ::StringFromGUID2(scalar.GetGuid(), buffer, ARRAYSIZE(buffer));
            text = buffer;
            break;
        }
        default:
            return std::nullopt;
        }

        // The type is part of the identity, so 1 and "1" stay distinct groups.
        return winrt::hstring{ L"cvgroup:v" + std::to_wstring(static_cast<int>(type)) + L":" + text };
    }
}

std::shared_ptr<TableViewCollectionViewGroups> TableViewCollectionViewGroups::TryCreate(
    winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view)
{
    if (!view)
    {
        return nullptr;
    }
    auto groups = view.CollectionGroups();
    if (!groups)
    {
        return nullptr;
    }
    return std::make_shared<TableViewCollectionViewGroups>(groups);
}

TableViewCollectionViewGroups::TableViewCollectionViewGroups(
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& groups) :
    m_groups(groups)
{
}

TableViewCollectionViewGroups::~TableViewCollectionViewGroups()
{
    if (m_groupsChangedToken.value)
    {
        try
        {
            m_groups.VectorChanged(m_groupsChangedToken);
        }
        catch (...)
        {
        }
    }
}

void TableViewCollectionViewGroups::Start()
{
    Refresh();

    std::weak_ptr<TableViewCollectionViewGroups> weakSelf = weak_from_this();
    m_groupsChangedToken = m_groups.VectorChanged(
        [weakSelf](winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const&,
            winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const&)
        {
            if (auto strongSelf = weakSelf.lock())
            {
                strongSelf->OnGroupsChanged();
            }
        });
}

// A view may recreate its group objects on every recompute (CollectionViewSource shaping does), so a
// group is identified by its key value rather than by the group object's address.
winrt::hstring TableViewCollectionViewGroups::IdentityForKey(winrt::IInspectable const& key)
{
    if (!key)
    {
        return L"cvgroup:null";
    }
    if (auto identity = ScalarIdentity(key))
    {
        return *identity;
    }

    // A computed group exposes its key as a "Key" property (the IGrouping<K,T> idiom).
    if (auto const provider = key.try_as<winrt::Microsoft::UI::Xaml::Data::ICustomPropertyProvider>())
    {
        try
        {
            if (auto const property = provider.GetCustomProperty(L"Key"); property && property.CanRead())
            {
                auto const value = property.GetValue(key);
                if (!value)
                {
                    return L"cvgroup:null";
                }
                if (auto identity = ScalarIdentity(value))
                {
                    return *identity;
                }
                wchar_t buffer[48];
                swprintf_s(buffer, L"cvgroup:k%p", reinterpret_cast<void*>(CanonicalAddress(value)));
                return winrt::hstring{ buffer };
            }
        }
        catch (...)
        {
        }
    }

    wchar_t buffer[48];
    swprintf_s(buffer, L"cvgroup:%p", reinterpret_cast<void*>(CanonicalAddress(key)));
    return winrt::hstring{ buffer };
}

std::vector<TableViewCollectionViewGroups::GroupInfo> TableViewCollectionViewGroups::CurrentGroups() const
{
    std::vector<GroupInfo> groups;
    groups.reserve(m_wrappers.Size());
    for (auto const& wrapper : m_wrappers)
    {
        auto const* const group = winrt::get_self<ShapedGroup>(wrapper.as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>());
        groups.push_back({ group->StableGroupIdentity(), group->Group(), wrapper });
    }
    return groups;
}

winrt::hstring TableViewCollectionViewGroups::GroupIdentityForItem(winrt::IInspectable const& item)
{
    if (!m_identityByItemValid)
    {
        m_identityByItem.clear();
        for (auto const& wrapper : m_wrappers)
        {
            auto const* const group = winrt::get_self<ShapedGroup>(wrapper.as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>());
            auto const identity = group->StableGroupIdentity();
            for (auto const& member : group->GroupItems())
            {
                if (auto const address = CanonicalAddress(member))
                {
                    m_identityByItem.try_emplace(address, identity);
                }
            }
        }
        m_identityByItemValid = true;
    }

    auto const found = m_identityByItem.find(CanonicalAddress(item));
    return found != m_identityByItem.end() ? found->second : winrt::hstring{};
}

void TableViewCollectionViewGroups::OnGroupsChanged()
{
    try
    {
        Refresh();
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] Reading the view's CollectionGroups failed (HRESULT 0x%08X).",
            static_cast<unsigned int>(winrt::to_hresult()));
    }
}

void TableViewCollectionViewGroups::Refresh()
{
    // A change raised while the wrappers are being replaced is applied once they are.
    if (m_refreshing)
    {
        m_refreshPending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    bool changed = false;
    do
    {
        m_refreshing = true;
        m_refreshPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_refreshing = false; });

        std::vector<winrt::IInspectable> wrappers;
        std::vector<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>> wrappedItems;
        std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> byIdentity;

        uint32_t const groupCount = m_groups.Size();
        wrappers.reserve(groupCount);
        wrappedItems.reserve(groupCount);
        for (uint32_t g = 0; g < groupCount; ++g)
        {
            auto const groupObject = m_groups.GetAt(g);
            if (!groupObject)
            {
                continue;
            }

            auto const key = GroupKey(groupObject);
            auto identity = IdentityForKey(key);
            // Two groups with one key stay two groups.
            for (uint32_t suffix = 2; byIdentity.contains(identity); ++suffix)
            {
                identity = IdentityForKey(key) + L"#" + winrt::to_hstring(suffix);
            }

            winrt::com_ptr<ShapedGroup> wrapper;
            if (auto const found = m_wrapperByIdentity.find(identity); found != m_wrapperByIdentity.end())
            {
                wrapper = found->second;
            }
            else
            {
                wrapper = winrt::make_self<ShapedGroup>(key, identity);
            }

            winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> items{ nullptr };
            if (auto const viewGroup = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
            {
                items = viewGroup.GroupItems();
            }
            if (!items)
            {
                items = groupObject.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>();
            }
            if (items)
            {
                wrapper->SetItemsSource(items);
            }
            else
            {
                // A group that is only enumerable is read once; the next CollectionGroups change re-reads it.
                wrapper->SetItemsSource(nullptr);
                wrapper->SetItems(ShapingHelpers::EnumerateInspectableItems(groupObject));
            }

            wrappedItems.push_back(wrapper->GroupItems());
            wrappers.push_back(wrapper.as<winrt::IInspectable>());
            byIdentity.emplace(identity, wrapper);
        }

        // Unchanged wrappers over unchanged item collections need no Reset.
        bool same = wrappers.size() == m_wrappers.Size() && wrappedItems == m_wrappedItems;
        for (uint32_t i = 0; same && i < wrappers.size(); ++i)
        {
            same = wrappers[i] == m_wrappers.GetAt(i);
        }

        m_wrapperByIdentity = std::move(byIdentity);
        m_wrappedItems = std::move(wrappedItems);
        m_identityByItemValid = false;
        if (!same)
        {
            m_wrappers.ReplaceAll(wrappers);
            changed = true;
        }
    } while (m_refreshPending);

    if (changed && m_changed)
    {
        auto const handler = m_changed;
        handler();
    }
}
