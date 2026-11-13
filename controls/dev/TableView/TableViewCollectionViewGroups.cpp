// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewCollectionViewGroups.h"
#include "TVDiag.h"

#include <cwchar>
#include <optional>
#include <string>

#include <winrt/Microsoft.UI.Dispatching.h>

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
    winrt::IInspectable GroupKey(winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup const& group)
    {
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

    // A view may recreate its group objects on every recompute (CollectionViewSource shaping does), so a
    // group is identified by its key value rather than by the group object's address. Expansion state
    // follows the identity, so a collapsed group stays collapsed across a Refresh.
    winrt::hstring KeyIdentity(winrt::IInspectable const& key)
    {
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
    return std::make_shared<TableViewCollectionViewGroups>(view, groups);
}

TableViewCollectionViewGroups::TableViewCollectionViewGroups(
    winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view,
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& groups) :
    m_view(view),
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

void TableViewCollectionViewGroups::Attach(winrt::TableViewSource const& source)
{
    auto self = shared_from_this();
    source.GroupBy(
        winrt::TableViewKeySelector{ [self](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            return self->KeyFor(item);
        } },
        winrt::TableViewGroupIdentitySelector{ [self](winrt::IInspectable const& key) -> winrt::hstring
        {
            return self->IdentityFor(key);
        } });

    m_source = winrt::make_weak(source);
    std::weak_ptr<TableViewCollectionViewGroups> weakSelf = self;
    m_groupsChangedToken = m_groups.VectorChanged(
        [weakSelf](winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const&,
            winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
        {
            if (auto strongSelf = weakSelf.lock())
            {
                strongSelf->OnGroupsChanged(args.CollectionChange());
            }
        });
}

winrt::IInspectable TableViewCollectionViewGroups::KeyFor(winrt::IInspectable const& item)
{
    bool rebuilt = false;
    if (m_dirty)
    {
        Rebuild();
        rebuilt = true;
    }

    auto const address = CanonicalAddress(item);
    if (!address)
    {
        return m_ungroupedKey;
    }
    if (auto key = TryFind(address))
    {
        return key;
    }

    // A miss is either an item no group holds, or a change the map has not seen yet.
    if (!rebuilt && !m_ungrouped.contains(address))
    {
        Rebuild();
        if (auto key = TryFind(address))
        {
            return key;
        }
    }
    return m_ungroupedKey;
}

winrt::IInspectable TableViewCollectionViewGroups::TryFind(uintptr_t address) const
{
    if (!address)
    {
        return nullptr;
    }
    auto const found = m_entries.find(address);
    if (found == m_entries.end())
    {
        return nullptr;
    }

    auto const& entry = found->second;
    auto const items = entry.Group.GroupItems();
    if (!items || entry.Index >= items.Size() || CanonicalAddress(items.GetAt(entry.Index)) != address)
    {
        return nullptr;
    }
    return entry.Key;
}

winrt::hstring TableViewCollectionViewGroups::IdentityFor(winrt::IInspectable const& key) const
{
    if (!key || winrt::get_abi(key) == winrt::get_abi(m_ungroupedKey))
    {
        return L"cvgroup:ungrouped";
    }
    return KeyIdentity(key);
}

void TableViewCollectionViewGroups::Rebuild()
{
    m_dirty = false;
    m_entries.clear();
    m_ungrouped.clear();

    uint32_t const groupCount = m_groups.Size();
    for (uint32_t g = 0; g < groupCount; ++g)
    {
        auto const group = m_groups.GetAt(g).try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
        if (!group)
        {
            continue;
        }
        auto const items = group.GroupItems();
        if (!items)
        {
            continue;
        }
        auto const key = GroupKey(group);
        uint32_t const itemCount = items.Size();
        for (uint32_t i = 0; i < itemCount; ++i)
        {
            if (auto const address = CanonicalAddress(items.GetAt(i)))
            {
                m_entries.try_emplace(address, Entry{ group, i, key });
            }
        }
    }

    uint32_t const viewCount = m_view.Size();
    for (uint32_t i = 0; i < viewCount; ++i)
    {
        auto const address = CanonicalAddress(m_view.GetAt(i));
        if (address && !m_entries.contains(address))
        {
            m_ungrouped.insert(address);
        }
    }
}

void TableViewCollectionViewGroups::OnGroupsChanged(winrt::Windows::Foundation::Collections::CollectionChange change)
{
    m_dirty = true;

    // A view may report a group's items as removed before it drops the group itself (CollectionViewSource
    // does), so the rows last read from it can be stale; it is consistent again once the group is gone.
    if (change == winrt::Windows::Foundation::Collections::CollectionChange::ItemRemoved && TryRefreshSource())
    {
        return;
    }
    if (m_refreshQueued)
    {
        return;
    }

    // Other group changes need not move any item (an empty group, a re-keyed group), so regroup once the
    // view has finished applying them; queued, so a burst of group changes regroups once.
    auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        return;
    }
    std::weak_ptr<TableViewCollectionViewGroups> weakSelf = weak_from_this();
    m_refreshQueued = queue.TryEnqueue([weakSelf]()
    {
        if (auto const strongSelf = weakSelf.lock())
        {
            strongSelf->m_refreshQueued = false;
            strongSelf->TryRefreshSource();
        }
    });
}

bool TableViewCollectionViewGroups::TryRefreshSource()
{
    auto const source = m_source.get();
    if (!source)
    {
        return true;
    }
    try
    {
        source.Refresh();
        return true;
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] Regrouping after a CollectionGroups change failed (HRESULT 0x%08X).",
            static_cast<unsigned int>(winrt::to_hresult()));
        return false;
    }
}
