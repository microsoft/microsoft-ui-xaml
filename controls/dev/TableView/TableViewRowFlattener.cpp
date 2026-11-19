// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewRowFlattener.h"
#include "RowMetadataProvider.h"
#include "GroupedEntry.h"
#include "RuntimeProfiler.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"
#include "TVDiag.h"

#include <algorithm>
#include <cwchar>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>

#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;

    constexpr wchar_t c_ungroupedIdentity[] = L"cvgroup:ungrouped";
    // Every node key ParentKeyIndex mints starts with this. A bare prefix is not a key.
    constexpr std::wstring_view c_nodeKeyPrefix{ L"node:" };
    // Above this run length, one Rebuild + Reset beats per-row splice notifications.
    constexpr int32_t c_maxSpliceRows{ 64 };

    TableViewRowItemKeySelector MakeIdentitySelector()
    {
        return [](winrt::IInspectable const& item) -> winrt::hstring
        {
            return TableViewRowFlattener::ObjectIdentity(item);
        };
    }

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

TableViewRowFlattener::TableViewRowFlattener(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view) :
    m_view(view)
{
    if (!view)
    {
        throw winrt::hresult_invalid_argument(L"view cannot be null.");
    }

    if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        m_uiQueue = winrt::make_weak(queue);
    }

    m_groupRowsView = winrt::ItemsSourceView{ m_groupRows };
    m_treeRowsView = winrt::ItemsSourceView{ m_treeRows };

    m_groupExpansion.SetChangedHandler(
        [this](ShapingHelpers::RowExpansionModel::Change const& change)
        {
            OnGroupExpansionChanged(change);
        });

    // Set before attaching the handler so it raises no Change into a half-constructed flattener.
    m_nodeExpansion.SetAllExpanded(false);
    m_nodeExpansion.SetChangedHandler(
        [this](ShapingHelpers::RowExpansionModel::Change const& change)
        {
            if (m_suppressNodeExpansionChanged)
            {
                m_nodeExpansionChangedWhileSuppressed = true;
                return;
            }
            OnNodeExpansionChanged(change);
        });
}

TableViewRowFlattener::~TableViewRowFlattener()
{
    if (m_viewChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_view.VectorChanged(m_viewChangedToken); });
    }
    if (m_viewGroups && m_viewGroupsChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_viewGroups.VectorChanged(m_viewGroupsChangedToken); });
    }
    UnsubscribeFromGroupItems();
}

winrt::hstring TableViewRowFlattener::ObjectIdentity(winrt::IInspectable const& item)
{
    if (!item)
    {
        return {};
    }
    // The canonical IUnknown is the COM identity; boxed copies of one value are distinct rows.
    auto const unknown = item.as<winrt::Windows::Foundation::IUnknown>();
    wchar_t buffer[2 + (sizeof(uintptr_t) * 2) + 1]{};
    swprintf_s(buffer, L"0x%zx", reinterpret_cast<size_t>(winrt::get_abi(unknown)));
    return winrt::hstring{ buffer };
}

bool TableViewRowFlattener::IsNodeKey(winrt::hstring const& key)
{
    const std::wstring_view view{ key.c_str(), key.size() };
    return view.size() > c_nodeKeyPrefix.size() && view.starts_with(c_nodeKeyPrefix);
}

void TableViewRowFlattener::AssertOnUiThread() const
{
    MUX_ASSERT(!m_uiQueue.get() || m_uiQueue.get().HasThreadAccess());
}

void TableViewRowFlattener::Start()
{
    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    try
    {
        if (auto const groups = m_view.CollectionGroups())
        {
            m_viewGroups = groups;
            ReadViewGroups();
            m_viewGroupsChangedToken = groups.VectorChanged(
                [weakThis](winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const&,
                    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const&)
                {
                    if (auto strongThis = weakThis.lock())
                    {
                        strongThis->OnViewGroupsChanged();
                    }
                });
        }
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] The ICollectionView's groups could not be read (HRESULT 0x%08X); showing its items ungrouped.",
            static_cast<unsigned int>(winrt::to_hresult()));
        m_viewGroups = nullptr;
        m_viewGroupsChangedToken = {};
        m_groups.clear();
    }

    m_viewChangedToken = m_view.VectorChanged(
        [weakThis](winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const&,
            winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const&)
        {
            if (auto strongThis = weakThis.lock())
            {
                strongThis->OnViewChanged();
            }
        });

    Update();
}

void TableViewRowFlattener::SetHierarchy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector)
{
    if (!keySelector || !parentKeySelector)
    {
        throw winrt::hresult_invalid_argument(L"keySelector and parentKeySelector are required.");
    }

    // Validated before anything changes, so a relation the rows cannot satisfy leaves the current rows.
    ShapingHelpers::ParentStructure probe;
    winrt::hstring error;
    if (!ShapingHelpers::BuildParentStructure(ReadViewItems(), keySelector, parentKeySelector, probe, error))
    {
        throw winrt::hresult_invalid_argument(L"TableView: " + error);
    }

    m_keySelector = keySelector;
    m_parentKeySelector = parentKeySelector;
    ++m_declarationGeneration;
    // A different relation is a different tree: its rows start collapsed.
    m_resetTreeIntent = true;
    Update();
}

void TableViewRowFlattener::ClearHierarchy()
{
    if (!m_keySelector)
    {
        return;
    }
    m_keySelector = nullptr;
    m_parentKeySelector = nullptr;
    ++m_declarationGeneration;
    Update();
}

void TableViewRowFlattener::Update()
{
    // A request raised while the rows are being published (an app handler of the change) runs once
    // the publication unwinds, against the declaration in force then.
    if (m_updating)
    {
        m_updatePending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    ++m_updateCount;
    {
        m_updating = true;
        auto const done = wil::scope_exit([this]() noexcept { m_updating = false; });
        do
        {
            m_updatePending = false;
            UpdateCore();
        } while (m_updatePending);
    }

    RaiseRowsChanged();
}

void TableViewRowFlattener::SetKind(Kind kind, winrt::ItemsSourceView const& rows)
{
    if (m_kind != kind)
    {
        m_rowMetadataStale = true;
    }
    m_itemsSourceView = rows;
    m_kind = kind;
}

void TableViewRowFlattener::UpdateCore()
{
    Kind const desired = m_keySelector
        ? (m_viewGroups ? Kind::GroupedTree : Kind::Tree)
        : (m_viewGroups ? Kind::Grouped : Kind::Flat);

    switch (desired)
    {
    case Kind::Flat:
        ReleaseTree();
        DetachGroupRows();
        if (!m_flatView)
        {
            m_flatView = winrt::ItemsSourceView{ m_view };
        }
        // The view itself: its fine-grained changes reach the repeater as they are.
        SetKind(Kind::Flat, m_flatView);
        break;

    case Kind::Grouped:
        ReleaseTree();
        if (m_kind != Kind::Grouped)
        {
            DetachGroupRows();
            AttachGroupRows(GroupSource::ViewGroups);
            SetKind(Kind::Grouped, m_groupRowsView);
        }
        break;

    case Kind::Tree:
    case Kind::GroupedTree:
        try
        {
            UpdateTree(desired);
        }
        catch (...)
        {
            // The rows changed into something that is not a tree (a duplicate key, a cycle): keep showing the
            // last tree until they are a tree again.
            TVDiag::LogRetailF(L"[TableView] ItemKeyPath/ParentKeyPath could not build a tree over the rows (HRESULT 0x%08X); keeping the previous rows.",
                static_cast<unsigned int>(winrt::to_hresult()));
            if (m_kind != Kind::Tree && m_kind != Kind::GroupedTree)
            {
                throw;
            }
        }
        break;

    case Kind::None:
        break;
    }
}

TableViewRowMetadataProvider TableViewRowFlattener::RowMetadata()
{
    if (m_kind != Kind::Grouped && m_kind != Kind::Tree && m_kind != Kind::GroupedTree)
    {
        return nullptr;
    }
    if (!m_rowMetadataStale)
    {
        if (auto existing = m_rowMetadata.lock())
        {
            return existing;
        }
    }

    auto const self = shared_from_this();
    TableViewRowMetadataProvider created;
    switch (m_kind)
    {
    case Kind::Grouped:
        created = tabularPrimitives::RowMetadataProvider::CreateForGroupedRows(self, MakeIdentitySelector());
        break;
    case Kind::Tree:
        created = tabularPrimitives::RowMetadataProvider::CreateForHierarchicalRows(self, MakeIdentitySelector());
        break;
    case Kind::GroupedTree:
        created = tabularPrimitives::RowMetadataProvider::CreateForGroupedHierarchicalRows(self, MakeIdentitySelector());
        break;
    default:
        break;
    }
    m_rowMetadata = created;
    m_rowMetadataStale = false;
    return created;
}

std::vector<winrt::IInspectable> TableViewRowFlattener::ReadViewItems() const
{
    std::vector<winrt::IInspectable> rows;
    uint32_t const count = m_view.Size();
    rows.reserve(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        rows.push_back(m_view.GetAt(i));
    }
    return rows;
}

void TableViewRowFlattener::UpdateTree(Kind kind)
{
    // Selectors and getters are app code that can re-declare the relation; a stale build is dropped and
    // Update runs again for the new declaration.
    auto const generation = m_declarationGeneration;
    auto const keySelector = m_keySelector;
    auto const parentKeySelector = m_parentKeySelector;

    auto structure = std::make_shared<ShapingHelpers::ParentStructure>();
    winrt::hstring error;
    const bool valid = ShapingHelpers::BuildParentStructure(ReadViewItems(), keySelector, parentKeySelector, *structure, error);
    if (generation != m_declarationGeneration)
    {
        m_updatePending = true;
        return;
    }
    if (!valid)
    {
        throw winrt::hresult_invalid_argument(L"TableView: " + error);
    }

    // The view already filtered and sorted: siblings and roots keep its order.
    auto index = std::make_shared<ShapingHelpers::ParentKeyIndex>();
    ShapingHelpers::BuildParentKeyIndex(structure, {}, {}, false /* sortRoots */, *index);

    if (m_kind != Kind::Tree && m_kind != Kind::GroupedTree)
    {
        __RP_Marker_ClassById(RuntimeProfiler::ProfId_HierarchicalSourceAdapter);
    }
    // Dropped first: it would re-slice the groups this rebuild replaces.
    m_sliceTreeIntoGroups = false;
    if (std::exchange(m_resetTreeIntent, false))
    {
        ResetNodeIntentQuietly();
    }

    if (kind == Kind::Tree)
    {
        DetachGroupRows();
        m_treeGroups.clear();
        m_rootBucket.clear();

        SetTreeIndex(std::move(index), {});
        SetKind(Kind::Tree, m_treeRowsView);
        return;
    }

    // Under a grouped view the group a root's item is in buckets it; a descendant follows its root.
    // Buckets are in the order their first root appears, which is the view's group order.
    auto const identityByItem = GroupIdentityByItem();
    std::vector<winrt::hstring> bucketIdentities;
    std::vector<std::vector<winrt::IInspectable>> bucketRoots;
    std::unordered_map<winrt::hstring, size_t> bucketByIdentity;
    for (auto const& root : index->Roots)
    {
        winrt::hstring identity{ c_ungroupedIdentity };
        if (auto const group = identityByItem.find(CanonicalAddress(root)); group != identityByItem.end())
        {
            identity = group->second;
        }
        auto [found, inserted] = bucketByIdentity.try_emplace(identity, bucketIdentities.size());
        if (inserted)
        {
            bucketIdentities.push_back(identity);
            bucketRoots.emplace_back();
        }
        bucketRoots[found->second].push_back(root);
    }

    std::unordered_map<winrt::hstring, winrt::IInspectable> keyByIdentity;
    for (auto const& group : m_groups)
    {
        keyByIdentity.emplace(group.Identity, group.Key);
    }

    // Each bucket's roots are contiguous: the tree rows walk one segment per bucket and the re-slice
    // cuts them at depth-0 rows.
    std::vector<winrt::IInspectable> orderedRoots;
    orderedRoots.reserve(index->Roots.size());
    std::vector<size_t> segments;
    segments.reserve(bucketRoots.size());
    std::unordered_map<void*, size_t> rootBucket;
    for (size_t bucket = 0; bucket < bucketRoots.size(); ++bucket)
    {
        for (auto const& root : bucketRoots[bucket])
        {
            rootBucket[winrt::get_abi(root)] = bucket;
            orderedRoots.push_back(root);
        }
        segments.push_back(bucketRoots[bucket].size());
    }
    index->Roots = std::move(orderedRoots);

    m_rootBucket = std::move(rootBucket);
    m_treeGroups.clear();
    SetTreeIndex(std::move(index), std::move(segments));
    if (generation != m_declarationGeneration)
    {
        m_updatePending = true;
        return;
    }

    std::vector<TreeGroup> treeGroups;
    treeGroups.reserve(bucketIdentities.size());
    for (auto const& identity : bucketIdentities)
    {
        auto const key = keyByIdentity.find(identity);
        treeGroups.push_back({ identity, key != keyByIdentity.end() ? key->second : winrt::box_value(winrt::hstring{}) });
    }
    m_treeGroups = std::move(treeGroups);

    // Sliced before the grouped rows attach, so they publish one coherent Reset.
    DetachGroupRows();
    ResliceTreeGroups();
    AttachGroupRows(GroupSource::TreeGroups);
    // A node toggle splices only the tree rows; this carries it to the groups.
    m_sliceTreeIntoGroups = true;
    SetKind(Kind::GroupedTree, m_groupRowsView);
}

void TableViewRowFlattener::ResliceTreeGroups()
{
    if (m_treeGroups.empty())
    {
        return;
    }

    for (auto& group : m_treeGroups)
    {
        group.Items.clear();
        // A header counts roots, not the descendant rows its slice also holds.
        group.RootCount = 0;
    }

    size_t bucket = 0;
    bool haveBucket = false;
    for (auto const& node : m_nodeRows)
    {
        if (node.Depth == 0)
        {
            auto const found = m_rootBucket.find(winrt::get_abi(node.Item));
            haveBucket = found != m_rootBucket.end() && found->second < m_treeGroups.size();
            if (haveBucket)
            {
                bucket = found->second;
                ++m_treeGroups[bucket].RootCount;
            }
        }

        if (haveBucket)
        {
            m_treeGroups[bucket].Items.push_back(node.Item);
        }
    }
}

void TableViewRowFlattener::ReleaseTree()
{
    m_sliceTreeIntoGroups = false;
    // Also detaches: this may run inside a tree rows notification, which must stop here.
    ClearTreeIndex();
    ResetNodeIntentQuietly();
    m_prunedForStructure.reset();
    if (m_kind == Kind::GroupedTree)
    {
        DetachGroupRows();
    }
    m_treeGroups.clear();
    m_rootBucket.clear();

    // Fresh rows for the next tree: the released ones keep their items only while a repeater still holds them.
    if (!m_nodeRows.empty() || m_treeRows.Size() > 0)
    {
        m_nodeRows.clear();
        m_indexByItem.clear();
        m_indexByItemValid = false;
        m_treeRows = winrt::single_threaded_observable_vector<winrt::IInspectable>();
        m_treeRowsView = winrt::ItemsSourceView{ m_treeRows };
    }
}

void TableViewRowFlattener::OnViewChanged()
{
    // Flat rows are the view; grouped rows follow its groups. Only a tree is rebuilt from the items.
    if (m_kind == Kind::Tree)
    {
        Update();
    }
    else if (m_kind == Kind::GroupedTree)
    {
        ScheduleUpdate();
    }
}

void TableViewRowFlattener::ScheduleUpdate()
{
    if (m_updateQueued)
    {
        return;
    }

    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    auto const run = [weakThis]()
    {
        auto const strongThis = weakThis.lock();
        if (!strongThis)
        {
            return;
        }
        strongThis->m_updateQueued = false;
        try
        {
            // The view's groups first: a change to them rebuilds the tree (OnViewGroupSetChanged).
            auto const before = strongThis->m_updateCount;
            if (strongThis->m_viewGroups)
            {
                strongThis->ReadViewGroupsNow();
            }
            if (strongThis->m_updateCount == before)
            {
                strongThis->Update();
            }
        }
        catch (...)
        {
            TVDiag::LogRetailF(L"[TableView] Rebuilding the rows after a view change failed (HRESULT 0x%08X).",
                static_cast<unsigned int>(winrt::to_hresult()));
        }
    };

    if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        m_updateQueued = queue.TryEnqueue(run);
        if (m_updateQueued)
        {
            return;
        }
    }
    run();
}

void TableViewRowFlattener::RaiseRowsChanged() const
{
    if (auto const handler = m_rowsChanged)
    {
        handler();
    }
}

std::optional<winrt::hstring> TableViewRowFlattener::TryGetGroupIdentityForKey(winrt::IInspectable const& key) const
{
    if (!m_viewGroups)
    {
        return std::nullopt;
    }
    return IdentityForKey(key);
}

std::vector<TableViewRowFlattener::GroupInfo> TableViewRowFlattener::CurrentGroups() const
{
    std::vector<GroupInfo> groups;
    if (m_kind == Kind::GroupedTree)
    {
        for (auto const& group : m_treeGroups)
        {
            groups.push_back({ group.Identity, group.Key });
        }
    }
    else if (m_kind == Kind::Grouped)
    {
        for (auto const& group : m_groups)
        {
            groups.push_back({ group.Identity, group.Key });
        }
    }
    return groups;
}

uint32_t TableViewRowFlattener::SourceRowCount() const
{
    return m_view.Size();
}

uint32_t TableViewRowFlattener::DataRowCount() const
{
    if (m_treeIndex)
    {
        return static_cast<uint32_t>(m_treeIndex->KeptNodeCount);
    }
    return m_view.Size();
}

// --- The view's groups ----------------------------------------------------------------------------

// A view may recreate its group objects on every recompute (CollectionViewSource shaping does), so a
// group is identified by its key value rather than by the group object's address.
winrt::hstring TableViewRowFlattener::IdentityForKey(winrt::IInspectable const& key)
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

std::unordered_map<uintptr_t, winrt::hstring> TableViewRowFlattener::GroupIdentityByItem() const
{
    // Read now: a view may move items between its groups without replacing a group.
    std::unordered_map<uintptr_t, winrt::hstring> identityByItem;
    for (auto const& group : m_groups)
    {
        auto const items = group.Items ? ShapingHelpers::EnumerateInspectableItems(group.Items) : group.Snapshot;
        for (auto const& member : items)
        {
            if (auto const address = CanonicalAddress(member))
            {
                identityByItem.try_emplace(address, group.Identity);
            }
        }
    }
    return identityByItem;
}

void TableViewRowFlattener::OnViewGroupsChanged()
{
    // A view replaces its groups in several notifications (CollectionViewSource clears and refills them on
    // a recompute). Read them once it has finished, so the grouped rows never see a partial group set: that
    // would drop the expansion state of the groups missing from it.
    if (m_readGroupsQueued)
    {
        return;
    }
    if (auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread())
    {
        std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
        m_readGroupsQueued = queue.TryEnqueue([weakThis]()
        {
            if (auto const strongThis = weakThis.lock())
            {
                strongThis->m_readGroupsQueued = false;
                strongThis->ReadViewGroupsNow();
            }
        });
        if (m_readGroupsQueued)
        {
            return;
        }
    }
    ReadViewGroupsNow();
}

void TableViewRowFlattener::ReadViewGroupsNow() noexcept
{
    try
    {
        ReadViewGroups();
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] Reading the view's CollectionGroups failed (HRESULT 0x%08X).",
            static_cast<unsigned int>(winrt::to_hresult()));
    }
}

void TableViewRowFlattener::ReadViewGroups()
{
    // A change raised while the groups are being replaced is applied once they are.
    if (m_readingGroups)
    {
        m_readGroupsPending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    bool changed = false;
    do
    {
        m_readingGroups = true;
        m_readGroupsPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_readingGroups = false; });

        std::vector<ViewGroup> groups;
        std::unordered_set<winrt::hstring> identities;
        uint32_t const groupCount = m_viewGroups.Size();
        groups.reserve(groupCount);
        for (uint32_t g = 0; g < groupCount; ++g)
        {
            auto const groupObject = m_viewGroups.GetAt(g);
            if (!groupObject)
            {
                continue;
            }

            ViewGroup group;
            group.Key = GroupKey(groupObject);
            group.Identity = IdentityForKey(group.Key);
            // Two groups with one key stay two groups.
            for (uint32_t suffix = 2; identities.contains(group.Identity); ++suffix)
            {
                group.Identity = IdentityForKey(group.Key) + L"#" + winrt::to_hstring(suffix);
            }
            identities.insert(group.Identity);

            if (auto const viewGroup = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
            {
                group.Items = viewGroup.GroupItems();
                if (auto const inner = viewGroup.Group())
                {
                    group.Inner = inner.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
                }
            }
            if (!group.Items)
            {
                group.Items = groupObject.try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>();
            }
            if (!group.Items)
            {
                // A group that is only enumerable is read once; the next CollectionGroups change re-reads it.
                group.Snapshot = ShapingHelpers::EnumerateInspectableItems(groupObject);
            }
            groups.push_back(std::move(group));
        }

        // The same groups over the same item collections need no Reset.
        bool same = groups.size() == m_groups.size();
        for (size_t i = 0; same && i < groups.size(); ++i)
        {
            same = groups[i].Identity == m_groups[i].Identity && groups[i].Items && groups[i].Items == m_groups[i].Items;
        }

        m_groups = std::move(groups);
        if (!same)
        {
            if (m_groupSource == GroupSource::ViewGroups)
            {
                RebuildGroupRows();
            }
            changed = true;
        }
    } while (m_readGroupsPending);

    if (changed)
    {
        OnViewGroupSetChanged();
    }
}

void TableViewRowFlattener::OnViewGroupSetChanged()
{
    if (m_kind == Kind::GroupedTree)
    {
        Update();
    }
    else if (m_kind == Kind::Grouped && !m_updating)
    {
        RaiseRowsChanged();
    }
}

// --- Grouped rows ---------------------------------------------------------------------------------

winrt::hstring TableViewRowFlattener::GroupIntentKey(winrt::hstring const& groupIdentity)
{
    return L"identity:" + groupIdentity;
}

void TableViewRowFlattener::AttachGroupRows(GroupSource source)
{
    if (!std::exchange(m_groupRowsProfiled, true))
    {
        __RP_Marker_ClassById(RuntimeProfiler::ProfId_GroupedSourceAdapter);
    }
    m_groupSource = source;
    RebuildGroupRows();
}

void TableViewRowFlattener::DetachGroupRows()
{
    UnsubscribeFromGroupItems();
    m_groupSource = GroupSource::None;
}

std::vector<TableViewRowFlattener::GroupSlice> TableViewRowFlattener::ReadGroupSlices() const
{
    std::vector<GroupSlice> slices;
    if (m_groupSource == GroupSource::ViewGroups)
    {
        slices.reserve(m_groups.size());
        for (auto const& group : m_groups)
        {
            GroupSlice slice{ group.Identity, group.Key };
            if (group.Items)
            {
                slice.Items = ShapingHelpers::EnumerateInspectableItems(group.Items);
                slice.Observed = group.Items;
            }
            else
            {
                slice.Items = group.Snapshot;
            }
            if (group.Inner)
            {
                AppendSubgroupSlices(group.Inner, std::move(slice), slices);
            }
            else
            {
                slices.push_back(std::move(slice));
            }
        }
    }
    else if (m_groupSource == GroupSource::TreeGroups)
    {
        slices.reserve(m_treeGroups.size());
        for (auto const& group : m_treeGroups)
        {
            slices.push_back({ group.Identity, group.Key, group.Items, group.RootCount });
        }
    }
    return slices;
}

// A multi-level view (WPF: nested CollectionViewGroups): a group whose GroupItems are groups is followed by its
// subgroups' slices, one level deeper, depth first.
void TableViewRowFlattener::AppendSubgroupSlices(
    winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup const& group,
    GroupSlice&& slice,
    std::vector<GroupSlice>& slices)
{
    auto const children = group.GroupItems();
    auto const firstChild = (children && children.Size() > 0) ? children.GetAt(0) : nullptr;
    if (!firstChild || !firstChild.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
    {
        slices.push_back(std::move(slice));
        return;
    }

    auto const identity = slice.Identity;
    auto const level = slice.Level;
    auto const childCount = children.Size();
    slice.IsBottomLevel = false;
    slices.push_back(std::move(slice));

    std::unordered_set<winrt::hstring> identities;
    for (uint32_t i = 0; i < childCount; ++i)
    {
        auto const subgroup = children.GetAt(i).try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>();
        if (!subgroup)
        {
            continue;
        }

        GroupSlice child;
        child.Key = GroupKey(subgroup);
        auto const childIdentity = identity + L"/" + IdentityForKey(child.Key);
        child.Identity = childIdentity;
        for (uint32_t suffix = 2; identities.contains(child.Identity); ++suffix)
        {
            child.Identity = childIdentity + L"#" + winrt::to_hstring(suffix);
        }
        identities.insert(child.Identity);
        child.Level = level + 1;
        // Not observed: every change below a top-level group reaches its leaves last, and those are observed.
        if (auto const leaves = subgroup.Group().try_as<winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>>())
        {
            child.Items = ShapingHelpers::EnumerateInspectableItems(leaves);
        }
        AppendSubgroupSlices(subgroup, std::move(child), slices);
    }
}

std::vector<winrt::IInspectable> TableViewRowFlattener::ReadGroupItems(winrt::hstring const& groupIdentity) const
{
    if (m_groupSource == GroupSource::ViewGroups)
    {
        for (auto const& group : m_groups)
        {
            if (group.Identity == groupIdentity)
            {
                return group.Items ? ShapingHelpers::EnumerateInspectableItems(group.Items) : group.Snapshot;
            }
        }
    }
    else if (m_groupSource == GroupSource::TreeGroups)
    {
        for (auto const& group : m_treeGroups)
        {
            if (group.Identity == groupIdentity)
            {
                return group.Items;
            }
        }
    }
    return {};
}

void TableViewRowFlattener::RebuildGroupRows()
{
    AssertOnUiThread();

    if (m_groupRowsInFlight)
    {
        // A change notification raised synchronously inside ReplaceAll re-entered: run once after unwind.
        m_groupRowsPending = true;
        return;
    }

    bool runPending = false;
    do
    {
        m_groupRowsInFlight = true;
        m_groupRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_groupRowsInFlight = false;
            runPending = m_groupRowsPending;
            m_groupRowsPending = false;
        });

        UnsubscribeFromGroupItems();

        // One Reset for the whole set: per-item Adds during a known full rebuild cost one layout pass each.
        std::vector<winrt::IInspectable> built;
        std::unordered_set<winrt::hstring> liveKeys;
        std::vector<GroupInfo> liveGroups;
        // Slices deeper than a collapsed group's level are hidden under it.
        int32_t collapsedLevel = (std::numeric_limits<int32_t>::max)();
        bool hasSubgroupRows = false;

        for (auto& slice : ReadGroupSlices())
        {
            auto const intentKey = GroupIntentKey(slice.Identity);
            liveKeys.insert(intentKey);
            liveGroups.push_back({ slice.Identity, slice.Key });
            hasSubgroupRows = hasSubgroupRows || slice.Level > 0;

            const bool isExpanded = m_groupExpansion.IsExpanded(intentKey);
            if (slice.Level <= collapsedLevel)
            {
                collapsedLevel = isExpanded ? (std::numeric_limits<int32_t>::max)() : slice.Level;

                // An empty group keeps its header: it stays targetable.
                const int32_t groupItemCount = static_cast<int32_t>(slice.Items.size());
                built.push_back(winrt::make<GroupedEntry>(slice.Identity, slice.Key, groupItemCount,
                    slice.ChildCount >= 0 ? slice.ChildCount : groupItemCount, isExpanded, slice.Level));
                if (isExpanded && slice.IsBottomLevel)
                {
                    built.insert(built.end(), slice.Items.begin(), slice.Items.end());
                }
            }

            // Any change inside a view group rebuilds the rows.
            if (slice.Observed)
            {
                GroupItemsSubscription subscription{ slice.Observed };
                subscription.Token = slice.Observed.VectorChanged(
                    [weakThis = weak_from_this()](auto&&, auto&&)
                    {
                        if (auto strongThis = weakThis.lock())
                        {
                            strongThis->RebuildGroupRows();
                        }
                    });
                m_groupItemsSubscriptions.push_back(std::move(subscription));
            }
        }

        m_liveGroups = std::move(liveGroups);
        m_hasSubgroupRows = hasSubgroupRows;
        // Silent: pruning a dead key changes no live key's state.
        m_groupExpansion.RetainOnly(liveKeys);

        m_groupRows.ReplaceAll(built);
    } while (runPending);
}

void TableViewRowFlattener::UnsubscribeFromGroupItems()
{
    for (auto& subscription : m_groupItemsSubscriptions)
    {
        if (subscription.Items && subscription.Token.value)
        {
            ShapingHelpers::SafeRevokeWith([&] { subscription.Items.VectorChanged(subscription.Token); });
        }
    }
    m_groupItemsSubscriptions.clear();
}

bool TableViewRowFlattener::IsGroupExpanded(winrt::hstring const& groupIdentity) const
{
    return m_groupExpansion.IsExpanded(GroupIntentKey(groupIdentity));
}

void TableViewRowFlattener::SetGroupExpanded(winrt::hstring const& groupIdentity, bool isExpanded)
{
    if (groupIdentity.empty())
    {
        return;
    }

    auto const key = GroupIntentKey(groupIdentity);
    if (m_groupExpansion.IsExpanded(key) != isExpanded)
    {
        // Through the model, so a per-group toggle and ExpandAll share one path (OnGroupExpansionChanged).
        m_groupExpansion.SetExpanded(key, isExpanded);
    }
}

void TableViewRowFlattener::ExpandAllGroups()
{
    m_groupExpansion.SetAllExpanded(true);
}

void TableViewRowFlattener::CollapseAllGroups()
{
    m_groupExpansion.SetAllExpanded(false);
}

TableViewRowFlattener::GroupInfo const* TableViewRowFlattener::TryGetLiveGroup(winrt::hstring const& groupIdentity) const
{
    if (groupIdentity.empty())
    {
        return nullptr;
    }
    auto const found = std::find_if(m_liveGroups.begin(), m_liveGroups.end(),
        [&](GroupInfo const& group) { return group.Identity == groupIdentity; });
    return found != m_liveGroups.end() ? &*found : nullptr;
}

void TableViewRowFlattener::OnGroupExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertOnUiThread();

    if (m_groupRowsInFlight)
    {
        // The model already holds the new intent; the outer operation rebuilds once afterward.
        m_groupRowsPending = true;
        return;
    }

    // Only a single-group toggle can splice; a baseline move or a batch rebuilds into one Reset.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        RebuildGroupRows();
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_groupRowsInFlight = true;
        m_groupRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_groupRowsInFlight = false;
            runPending = m_groupRowsPending;
            m_groupRowsPending = false;
        });

        applied = TryApplyGroupExpansionSplice(change.Keys.front(), change.IsExpanded);
    }

    if (runPending || !applied)
    {
        RebuildGroupRows();
    }
}

bool TableViewRowFlattener::TryApplyGroupExpansionSplice(winrt::hstring const& intentKey, bool expand)
{
    if (m_hasSubgroupRows)
    {
        return false;
    }

    const uint32_t count = m_groupRows.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        auto const entry = TryGetGroupedEntry(m_groupRows.GetAt(i));
        if (!entry || GroupIntentKey(entry->Identity()) != intentKey)
        {
            continue;
        }

        if (entry->IsExpanded() == expand)
        {
            return true;
        }

        const int32_t groupItemCount = entry->GroupItemCount();

        // GroupedEntry is immutable: one Replace refreshes the chevron without touching any other row.
        m_groupRows.SetAt(i, winrt::make<GroupedEntry>(entry->Identity(), entry->Key(), groupItemCount, entry->ChildCount(), expand));

        if (expand)
        {
            // A count that disagrees means the group changed without a notification we processed.
            auto const items = ReadGroupItems(entry->Identity());
            if (static_cast<int32_t>(items.size()) != groupItemCount)
            {
                return false;
            }

            uint32_t insertAt = i + 1;
            for (auto const& item : items)
            {
                m_groupRows.InsertAt(insertAt++, item);
            }
        }
        else
        {
            // The group's rows run from the header to the next header (or the end).
            const uint32_t firstData = i + 1;
            for (int32_t removed = 0; removed < groupItemCount; ++removed)
            {
                if (firstData >= m_groupRows.Size() || TryGetGroupedEntry(m_groupRows.GetAt(firstData)))
                {
                    return false;
                }
                m_groupRows.RemoveAt(firstData);
            }
        }

        return true;
    }

    // The header is not in the rows (mid-reshape); a rebuild reconciles.
    return false;
}

// --- Tree rows ------------------------------------------------------------------------------------

template <typename Fn>
bool TableViewRowFlattener::MutateNodeExpansionQuietly(Fn&& fn)
{
    m_suppressNodeExpansionChanged = true;
    m_nodeExpansionChangedWhileSuppressed = false;
    auto guard = wil::scope_exit([this]() noexcept { m_suppressNodeExpansionChanged = false; });
    fn();
    return std::exchange(m_nodeExpansionChangedWhileSuppressed, false);
}

void TableViewRowFlattener::SetTreeIndex(std::shared_ptr<const ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments)
{
    // A handler of the publish may drop the last external owner.
    auto const keepAlive = shared_from_this();

    m_treeDetached = false;
    m_treeIndex = std::move(index);
    m_rootSegments = std::move(rootSegments);

    // A collapse override only means something on a row that is still a context row.
    if (m_treeIndex)
    {
        std::erase_if(m_filterOverlayCollapsed, [this](winrt::hstring const& key)
        {
            return !m_treeIndex->ContextKeys.contains(std::wstring{ key });
        });
    }
    else
    {
        m_filterOverlayCollapsed.clear();
    }

    RebuildTreeRows();

    // Pruned after publish, against the UNFILTERED key set, so intent for filtered-out rows survives.
    if (m_treeIndex && m_treeIndex->Structure && m_prunedForStructure.lock() != m_treeIndex->Structure)
    {
        std::unordered_set<winrt::hstring> liveKeys;
        liveKeys.reserve(m_treeIndex->Structure->KeyByItem.size());
        for (auto const& [item, key] : m_treeIndex->Structure->KeyByItem)
        {
            liveKeys.emplace(winrt::hstring{ key });
        }
        m_nodeExpansion.RetainOnly(liveKeys);
        m_prunedForStructure = m_treeIndex->Structure;
    }
}

void TableViewRowFlattener::ClearTreeIndex()
{
    // No rebuild: the repeater may still be bound mid-teardown, so an empty Reset is a hazard. The rows and
    // node rows stay as they are until the rows are swapped.
    m_treeDetached = true;
    m_treeIndex.reset();
    m_rootSegments.clear();
    m_filterOverlayCollapsed.clear();
    m_indexByNodeKey.clear();
    m_indexByNodeKeyValid = true;
}

void TableViewRowFlattener::ResetNodeIntentQuietly()
{
    // SetAllExpanded moves the baseline and also clears every explicit intent.
    MutateNodeExpansionQuietly([this]() { m_nodeExpansion.SetAllExpanded(false); });
    m_filterOverlayCollapsed.clear();
}

bool TableViewRowFlattener::EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const
{
    if (!hasChildren)
    {
        return false;
    }

    // Context rows follow the overlay only, so toggling them never writes intent.
    return isContext ? !m_filterOverlayCollapsed.contains(nodeKey) : m_nodeExpansion.IsExpanded(nodeKey);
}

bool TableViewRowFlattener::EffectiveExpandedForKey(winrt::hstring const& nodeKey) const
{
    if (!m_treeIndex)
    {
        return m_nodeExpansion.IsExpanded(nodeKey);
    }

    const std::wstring key{ nodeKey };
    auto const* const children = m_treeIndex->TryGetChildren(key);
    return EffectiveExpanded(nodeKey, children && !children->empty(), m_treeIndex->ContextKeys.contains(key));
}

void TableViewRowFlattener::EmitNodeRange(
    std::vector<winrt::IInspectable> const& siblings,
    size_t begin,
    size_t end,
    winrt::hstring const& parentKey,
    int32_t depth,
    std::vector<winrt::IInspectable>& built,
    std::vector<NodeRow>& nodeRows) const
{
    if (!m_treeIndex)
    {
        return;
    }

    struct Frame
    {
        std::vector<winrt::IInspectable> const* Siblings;
        size_t Begin;
        size_t End;
        size_t Next;
        winrt::hstring ParentKey;
        int32_t Depth;
    };

    std::vector<Frame> stack;
    stack.push_back({ &siblings, begin, end, begin, parentKey, depth });

    while (!stack.empty())
    {
        Frame& frame = stack.back();
        if (frame.Next >= frame.End)
        {
            stack.pop_back();
            continue;
        }

        const size_t i = frame.Next++;
        auto const& item = (*frame.Siblings)[i];
        auto const* const key = m_treeIndex->TryGetKey(item);
        if (!key)
        {
            // Every indexed item has a key; an unknown one cannot be addressed, so it is skipped.
            continue;
        }

        auto const* const children = m_treeIndex->TryGetChildren(*key);

        NodeRow row;
        row.Item = item;
        row.NodeKey = winrt::hstring{ *key };
        row.ParentKey = frame.ParentKey;
        row.Depth = frame.Depth;
        row.ChildCount = children ? static_cast<int32_t>(children->size()) : 0;
        row.SiblingIndex = static_cast<int32_t>(i - frame.Begin + 1);
        row.SiblingCount = static_cast<int32_t>(frame.End - frame.Begin);
        row.HasChildren = row.ChildCount > 0;
        row.IsContext = m_treeIndex->ContextKeys.contains(*key);
        row.IsExpanded = EffectiveExpanded(row.NodeKey, row.HasChildren, row.IsContext);

        // Captured before the push below, which may reallocate the stack and invalidate `frame`.
        const int32_t childDepth = frame.Depth + 1;
        const bool descend = row.IsExpanded;
        auto nodeKey = row.NodeKey;

        built.push_back(item);
        nodeRows.push_back(std::move(row));

        if (descend)
        {
            stack.push_back({ children, 0, children->size(), 0, std::move(nodeKey), childDepth });
        }
    }
}

void TableViewRowFlattener::RebuildTreeRows()
{
    AssertOnUiThread();

    if (m_treeDetached)
    {
        // Retracted: publishing even an empty Reset would reach a repeater mid-teardown.
        return;
    }

    if (m_treeRowsInFlight)
    {
        m_treeRowsPending = true;
        return;
    }

    bool runPending = false;
    do
    {
        m_treeRowsInFlight = true;
        m_treeRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_treeRowsInFlight = false;
            runPending = m_treeRowsPending;
            m_treeRowsPending = false;
        });

        std::vector<winrt::IInspectable> built;
        std::vector<NodeRow> nodeRows;

        if (m_treeIndex)
        {
            auto const& roots = m_treeIndex->Roots;
            if (m_rootSegments.empty())
            {
                EmitNodeRange(roots, 0, roots.size(), winrt::hstring{}, 0, built, nodeRows);
            }
            else
            {
                // One segment per group, so a root's sibling position is per group.
                MUX_ASSERT(std::accumulate(m_rootSegments.begin(), m_rootSegments.end(), size_t{ 0 }) == roots.size());

                size_t segmentBegin = 0;
                for (const size_t length : m_rootSegments)
                {
                    const size_t segmentEnd = (std::min)(segmentBegin + length, roots.size());
                    EmitNodeRange(roots, segmentBegin, segmentEnd, winrt::hstring{}, 0, built, nodeRows);
                    segmentBegin = segmentEnd;
                }
            }
        }

        // Built into a local so a failure cannot publish a partial map.
        std::unordered_map<winrt::hstring, int32_t> indexByNodeKey;
        indexByNodeKey.reserve(nodeRows.size());
        for (size_t i = 0; i < nodeRows.size(); ++i)
        {
            indexByNodeKey.emplace(nodeRows[i].NodeKey, static_cast<int32_t>(i));
        }

        // Node rows before the Reset, so synchronous handlers see agreeing state.
        auto previousNodeRows = std::exchange(m_nodeRows, std::move(nodeRows));
        auto previousIndex = std::exchange(m_indexByNodeKey, std::move(indexByNodeKey));
        const bool previousIndexValid = std::exchange(m_indexByNodeKeyValid, true);
        m_indexByItemValid = false;

        // Roll back only if ReplaceAll's assignment threw (old rows still published, detected by row
        // count). A throw from a Reset handler happens after the new rows are live; keep them.
        const auto publishedCount = static_cast<uint32_t>(built.size());
        // Captured: a handler of the Reset may release the tree, which replaces m_treeRows.
        auto const rows = m_treeRows;
        auto publishGuard = wil::scope_exit([this, rows, publishedCount, &previousNodeRows, &previousIndex, previousIndexValid]() noexcept
        {
            if (rows.Size() == publishedCount || m_treeDetached)
            {
                return;
            }

            m_nodeRows = std::move(previousNodeRows);
            m_indexByNodeKey = std::move(previousIndex);
            m_indexByNodeKeyValid = previousIndexValid;
            m_indexByItemValid = false;
        });

        rows.ReplaceAll(built);
        publishGuard.release();
    } while (runPending && !m_treeDetached);

    if (m_treeDetached)
    {
        // A handler of the Reset above retracted the tree; see ClearTreeIndex.
        return;
    }

    OnTreeRowsPublished();
}

void TableViewRowFlattener::OnTreeRowsPublished()
{
    if (m_sliceTreeIntoGroups)
    {
        ResliceTreeGroups();
        RebuildGroupRows();
    }
}

bool TableViewRowFlattener::IsNodeExpanded(winrt::hstring const& nodeKey) const
{
    // The presented state (incl. overlay), so toggling an overlay-expanded context row collapses it.
    int32_t index = -1;
    if (TryGetIndexForNodeKey(nodeKey, index))
    {
        if (auto const* const row = TryGetNodeRow(index))
        {
            return row->IsExpanded;
        }
    }

    // Not visible: context rows answer from the overlay, others from intent.
    if (m_treeIndex && m_treeIndex->ContextKeys.contains(std::wstring{ nodeKey }))
    {
        return EffectiveExpandedForKey(nodeKey);
    }
    return m_nodeExpansion.IsExpanded(nodeKey);
}

void TableViewRowFlattener::SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded)
{
    auto const keepAlive = shared_from_this();

    if (!m_treeIndex)
    {
        return;
    }

    int32_t index = -1;
    if (!TryGetIndexForNodeKey(nodeKey, index))
    {
        return;
    }

    auto const* const row = TryGetNodeRow(index);
    if (!row || !row->HasChildren)
    {
        return;
    }

    const bool before = row->IsExpanded;

    if (row->IsContext)
    {
        // Overlay only; discarded when the filter changes.
        if (isExpanded)
        {
            m_filterOverlayCollapsed.erase(nodeKey);
        }
        else
        {
            m_filterOverlayCollapsed.insert(nodeKey);
        }

        const bool after = EffectiveExpanded(nodeKey, true, true);
        if (before != after)
        {
            ApplySingleNodeToggle(nodeKey, after);
        }
        return;
    }

    if (m_nodeExpansion.IsExpanded(nodeKey) != isExpanded)
    {
        // Through the model, so its Changed notification drives ApplySingleNodeToggle.
        m_nodeExpansion.SetExpanded(nodeKey, isExpanded);
    }
    else if (before != isExpanded)
    {
        // Intent agrees but the presented state does not (an earlier build never landed); reconcile.
        ApplySingleNodeToggle(nodeKey, isExpanded);
    }
}

void TableViewRowFlattener::ExpandAllNodes()
{
    auto const keepAlive = shared_from_this();

    const bool hadOverlay = !m_filterOverlayCollapsed.empty();
    m_filterOverlayCollapsed.clear();

    // Moves the baseline, so future nodes also arrive expanded.
    const bool changed = MutateNodeExpansionQuietly([this]() { m_nodeExpansion.SetAllExpanded(true); });
    if (changed || hadOverlay)
    {
        RebuildTreeRows();
    }
}

void TableViewRowFlattener::CollapseAllNodes()
{
    auto const keepAlive = shared_from_this();

    const bool changed = MutateNodeExpansionQuietly([this]() { m_nodeExpansion.SetAllExpanded(false); });

    // Context rows are overlay-expanded, so collapse-all must override each one.
    bool overlayChanged = false;
    if (m_treeIndex)
    {
        for (auto const& key : m_treeIndex->ContextKeys)
        {
            overlayChanged |= m_filterOverlayCollapsed.emplace(winrt::hstring{ key }).second;
        }
    }

    if (changed || overlayChanged)
    {
        RebuildTreeRows();
    }
}

void TableViewRowFlattener::ExpandSubtree(winrt::hstring const& nodeKey)
{
    auto const keepAlive = shared_from_this();

    if (!m_treeIndex || !IsNodeKey(nodeKey))
    {
        return;
    }

    bool overlayChanged = false;
    const bool changed = MutateNodeExpansionQuietly([&]()
    {
        std::vector<std::wstring> pending{ std::wstring{ nodeKey } };
        while (!pending.empty())
        {
            const std::wstring key = std::move(pending.back());
            pending.pop_back();

            auto const* const children = m_treeIndex->TryGetChildren(key);
            if (!children || children->empty())
            {
                continue;
            }

            const winrt::hstring hkey{ key };
            if (m_treeIndex->ContextKeys.contains(key))
            {
                // A context row is presented from the overlay alone; expanding it writes no intent.
                overlayChanged |= m_filterOverlayCollapsed.erase(hkey) > 0;
            }
            else
            {
                m_nodeExpansion.SetExpanded(hkey, true);
            }

            for (auto const& child : *children)
            {
                if (auto const* const childKey = m_treeIndex->TryGetKey(child))
                {
                    pending.push_back(*childKey);
                }
            }
        }
    });

    if (changed || overlayChanged)
    {
        RebuildTreeRows();
    }
}

void TableViewRowFlattener::OnNodeExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertOnUiThread();

    // Only a single-node toggle can splice; baseline moves and batches rebuild.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        RebuildTreeRows();
        return;
    }

    // Splice to the presented state; for context rows intent changes move nothing.
    ApplySingleNodeToggle(change.Keys.front(), EffectiveExpandedForKey(change.Keys.front()));
}

void TableViewRowFlattener::ApplySingleNodeToggle(winrt::hstring const& nodeKey, bool isExpanded)
{
    AssertOnUiThread();

    if (m_treeRowsInFlight)
    {
        // Raised mid-publish; the outer operation rebuilds once afterward.
        m_treeRowsPending = true;
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_treeRowsInFlight = true;
        m_treeRowsPending = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_treeRowsInFlight = false;
            runPending = m_treeRowsPending;
            m_treeRowsPending = false;
        });

        applied = TryApplyNodeExpansionSplice(nodeKey, isExpanded);
    }

    // Re-entrant or unresolved: one rebuild after unwind (no-op once detached).
    if (runPending || !applied)
    {
        RebuildTreeRows();
    }
}

bool TableViewRowFlattener::TryApplyNodeExpansionSplice(winrt::hstring const& nodeKey, bool expand)
{
    // The rows and node rows must already agree or a ranged splice would corrupt the pairing.
    if (!m_treeIndex || m_nodeRows.size() != static_cast<size_t>(m_treeRows.Size()))
    {
        return false;
    }

    int32_t index = -1;
    if (!TryGetIndexForNodeKey(nodeKey, index) || index < 0 || static_cast<size_t>(index) >= m_nodeRows.size())
    {
        // Not visible (behind a collapsed ancestor); a rebuild reconciles.
        return false;
    }

    auto const& node = m_nodeRows[static_cast<size_t>(index)];
    if (node.IsExpanded == expand || !node.HasChildren)
    {
        return true;
    }

    const int32_t nodeDepth = node.Depth;

    if (!expand)
    {
        // Visible descendants = contiguous following rows with greater Depth.
        int32_t runEnd = index + 1;
        while (static_cast<size_t>(runEnd) < m_nodeRows.size() && m_nodeRows[static_cast<size_t>(runEnd)].Depth > nodeDepth)
        {
            ++runEnd;
        }

        if (runEnd - (index + 1) > c_maxSpliceRows)
        {
            return false;
        }

        // Node row first, so the row reads collapsed when RemoveTreeRows notifies.
        m_nodeRows[static_cast<size_t>(index)].IsExpanded = false;
        RemoveTreeRows(index + 1, runEnd - (index + 1));
        if (!m_treeDetached)
        {
            OnTreeRowsPublished();
        }
        return true;
    }

    auto const* const children = m_treeIndex->TryGetChildren(std::wstring{ nodeKey });
    if (!children || children->empty())
    {
        return false;
    }

    // Expand: emit the subtree from the index (honouring nested intent), then insert.
    std::vector<winrt::IInspectable> built;
    std::vector<NodeRow> nodeRows;
    EmitNodeRange(*children, 0, children->size(), nodeKey, nodeDepth + 1, built, nodeRows);

    if (built.size() > static_cast<size_t>(c_maxSpliceRows))
    {
        return false;
    }

    m_nodeRows[static_cast<size_t>(index)].IsExpanded = true;
    InsertTreeRows(index + 1, built, nodeRows);
    if (!m_treeDetached)
    {
        OnTreeRowsPublished();
    }
    return true;
}

void TableViewRowFlattener::InsertTreeRows(
    int32_t index,
    std::vector<winrt::IInspectable> const& items,
    std::vector<NodeRow> const& nodeRows)
{
    MUX_ASSERT(items.size() == nodeRows.size());

    // One row at a time, node row before its row: InsertAt notifies synchronously and handlers read node
    // rows by index.
    uint32_t insertAt = static_cast<uint32_t>(index);
    for (size_t i = 0; i < items.size() && !m_treeDetached; ++i)
    {
        m_nodeRows.insert(m_nodeRows.begin() + insertAt, nodeRows[i]);
        m_indexByNodeKeyValid = false;
        m_indexByItemValid = false;
        m_treeRows.InsertAt(insertAt++, items[i]);
    }
}

void TableViewRowFlattener::RemoveTreeRows(int32_t index, int32_t count)
{
    // From the end, so remaining rows keep their indices until removed.
    for (int32_t i = index + count - 1; i >= index && !m_treeDetached; --i)
    {
        m_nodeRows.erase(m_nodeRows.begin() + i);
        m_indexByNodeKeyValid = false;
        m_indexByItemValid = false;
        m_treeRows.RemoveAt(static_cast<uint32_t>(i));
    }
}

TableViewRowFlattener::NodeRow const* TableViewRowFlattener::TryGetNodeRow(int32_t index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_nodeRows.size())
    {
        return nullptr;
    }
    return &m_nodeRows[static_cast<size_t>(index)];
}

TableViewRowFlattener::NodeRow const* TableViewRowFlattener::TryGetNodeRowForItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return nullptr;
    }

    EnsureItemIndex();

    const auto found = m_indexByItem.find(winrt::get_abi(item));
    if (found == m_indexByItem.end() || found->second < 0 || static_cast<size_t>(found->second) >= m_nodeRows.size())
    {
        return nullptr;
    }
    return &m_nodeRows[static_cast<size_t>(found->second)];
}

void TableViewRowFlattener::EnsureItemIndex() const
{
    if (m_indexByItemValid)
    {
        return;
    }

    m_indexByItem.clear();
    m_indexByItem.reserve(m_nodeRows.size());
    for (size_t i = 0; i < m_nodeRows.size(); ++i)
    {
        if (auto const& item = m_nodeRows[i].Item)
        {
            m_indexByItem.emplace(winrt::get_abi(item), static_cast<int32_t>(i));
        }
    }
    m_indexByItemValid = true;
}

void TableViewRowFlattener::EnsureNodeKeyIndex() const
{
    if (m_indexByNodeKeyValid)
    {
        return;
    }

    m_indexByNodeKey.clear();
    m_indexByNodeKey.reserve(m_nodeRows.size());
    for (size_t i = 0; i < m_nodeRows.size(); ++i)
    {
        m_indexByNodeKey.emplace(m_nodeRows[i].NodeKey, static_cast<int32_t>(i));
    }
    m_indexByNodeKeyValid = true;
}

bool TableViewRowFlattener::TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const
{
    index = -1;
    if (nodeKey.empty())
    {
        return false;
    }

    EnsureNodeKeyIndex();

    const auto found = m_indexByNodeKey.find(nodeKey);
    if (found == m_indexByNodeKey.end())
    {
        return false;
    }

    index = found->second;
    return true;
}
