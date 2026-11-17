// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewRowProjection.h"
#include "RowMetadataProvider.h"
#include "GroupedSourceAdapter.h"
#include "HierarchicalSourceAdapter.h"
#include "ParentKeyIndex.h"
#include "ShapedGroup.h"
#include "TableViewCollectionViewGroups.h"
#include "TVDiag.h"

#include <cwchar>

#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{
    namespace tabularPrimitives = winrt::Microsoft::UI::Xaml::Controls::Tabular::Primitives::implementation;

    TableViewRowItemKeySelector MakeIdentitySelector()
    {
        return [](winrt::IInspectable const& item) -> winrt::hstring
        {
            return TableViewRowProjection::ObjectIdentity(item);
        };
    }

    constexpr wchar_t c_ungroupedIdentity[] = L"cvgroup:ungrouped";
}

TableViewRowProjection::TableViewRowProjection(winrt::Microsoft::UI::Xaml::Data::ICollectionView const& view) :
    m_view(view)
{
    if (!view)
    {
        throw winrt::hresult_invalid_argument(L"view cannot be null.");
    }
}

TableViewRowProjection::~TableViewRowProjection()
{
    if (m_viewChangedToken.value)
    {
        try { m_view.VectorChanged(m_viewChangedToken); } catch (...) {}
    }
    if (m_groups)
    {
        m_groups->SetChangedHandler(nullptr);
    }
    if (m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
    }
}

winrt::hstring TableViewRowProjection::ObjectIdentity(winrt::IInspectable const& item)
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

void TableViewRowProjection::Start()
{
    std::weak_ptr<TableViewRowProjection> weakThis = weak_from_this();
    try
    {
        if (auto groups = TableViewCollectionViewGroups::TryCreate(m_view))
        {
            groups->Start();
            groups->SetChangedHandler([weakThis]()
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->OnGroupsChanged();
                }
            });
            m_groups = std::move(groups);
        }
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] The ICollectionView's groups could not be read (HRESULT 0x%08X); showing its items ungrouped.",
            static_cast<unsigned int>(winrt::to_hresult()));
        m_groups = nullptr;
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

void TableViewRowProjection::SetHierarchy(ShapingHelpers::KeySelector const& keySelector, ShapingHelpers::KeySelector const& parentKeySelector)
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

void TableViewRowProjection::ClearHierarchy()
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

void TableViewRowProjection::Update()
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

    RaiseProjectionChanged();
}

void TableViewRowProjection::UpdateCore()
{
    Kind const desired = m_keySelector
        ? (m_groups ? Kind::GroupedTree : Kind::Tree)
        : (m_groups ? Kind::Grouped : Kind::Flat);

    switch (desired)
    {
    case Kind::Flat:
        ReleaseTree();
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        if (!m_flatView)
        {
            m_flatView = winrt::ItemsSourceView{ m_view };
        }
        // The view itself: its fine-grained changes reach the repeater as they are. A flat view carries
        // no row metadata; a row is its item.
        m_itemsSourceView = m_flatView;
        m_rowMetadata = nullptr;
        m_kind = Kind::Flat;
        break;

    case Kind::Grouped:
        ReleaseTree();
        if (!m_groupedAdapter)
        {
            m_groupedAdapter = std::make_shared<GroupedSourceAdapter>();
        }
        if (m_kind != Kind::Grouped)
        {
            m_groupedAdapter->DetachSourceQuietly();
            m_groupedAdapter->Source(m_groups->Groups());
            m_itemsSourceView = m_groupedAdapter->Entries();
            m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForGroupedRows(m_groupedAdapter, MakeIdentitySelector());
        }
        m_kind = Kind::Grouped;
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

std::vector<winrt::IInspectable> TableViewRowProjection::ReadViewItems() const
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

void TableViewRowProjection::UpdateTree(Kind kind)
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

    if (!m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter = std::make_shared<HierarchicalSourceAdapter>();
    }
    auto const adapter = m_hierarchicalAdapter;
    // Dropped first: it would re-slice the groups this rebuild replaces.
    adapter->ProjectionChanged(nullptr);
    if (std::exchange(m_resetTreeIntent, false))
    {
        adapter->ResetIntentQuietly();
    }

    if (kind == Kind::Tree)
    {
        if (m_groupedAdapter)
        {
            m_groupedAdapter->DetachSourceQuietly();
        }
        m_treeGroups.clear();
        m_rootBucket.clear();
        ++m_treeGeneration;

        adapter->SetIndex(std::move(index), {});
        if (m_kind != Kind::Tree)
        {
            m_itemsSourceView = adapter->Entries();
            m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForHierarchicalRows(adapter, MakeIdentitySelector());
        }
        m_kind = Kind::Tree;
        return;
    }

    // Under a grouped view the group a root's item is in buckets it; a descendant follows its root.
    // Buckets are in the order their first root appears, which is the view's group order.
    m_groups->InvalidateItemIdentities();
    std::vector<winrt::hstring> bucketIdentities;
    std::vector<std::vector<winrt::IInspectable>> bucketRoots;
    std::unordered_map<winrt::hstring, size_t> bucketByIdentity;
    for (auto const& root : index->Roots)
    {
        auto identity = m_groups->GroupIdentityForItem(root);
        if (identity.empty())
        {
            identity = c_ungroupedIdentity;
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
    for (auto const& group : m_groups->CurrentGroups())
    {
        keyByIdentity.emplace(group.Identity, group.Key);
    }

    // Each bucket's roots are contiguous: the tree adapter walks one segment per bucket and the re-slice
    // cuts its rows at depth-0 rows.
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
    // Keeps the publication below from re-slicing the previous groups.
    m_treeGroups.clear();
    ++m_treeGeneration;
    adapter->SetIndex(std::move(index), std::move(segments));
    if (generation != m_declarationGeneration)
    {
        m_updatePending = true;
        return;
    }

    // Groups are reused by identity, so a group's object (and its expansion) survives a rebuild.
    std::vector<winrt::com_ptr<ShapedGroup>> treeGroups;
    std::vector<winrt::IInspectable> groupObjects;
    std::unordered_map<winrt::hstring, winrt::com_ptr<ShapedGroup>> treeGroupByIdentity;
    for (auto const& identity : bucketIdentities)
    {
        winrt::com_ptr<ShapedGroup> group;
        if (auto const found = m_treeGroupByIdentity.find(identity); found != m_treeGroupByIdentity.end())
        {
            group = found->second;
        }
        else
        {
            auto const key = keyByIdentity.find(identity);
            group = winrt::make_self<ShapedGroup>(
                key != keyByIdentity.end() ? key->second : winrt::box_value(winrt::hstring{}), identity);
        }
        treeGroups.push_back(group);
        groupObjects.push_back(group.as<winrt::IInspectable>());
        treeGroupByIdentity.emplace(identity, group);
    }
    m_treeGroupByIdentity = std::move(treeGroupByIdentity);
    m_treeGroups = std::move(treeGroups);

    if (!m_treeGroupSource)
    {
        m_treeGroupSource = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    }
    if (!m_groupedAdapter)
    {
        m_groupedAdapter = std::make_shared<GroupedSourceAdapter>();
    }

    // Filled before the grouped adapter attaches, so it publishes one coherent Reset.
    m_groupedAdapter->DetachSourceQuietly();
    ResliceTreeGroups();
    m_treeGroupSource.ReplaceAll(groupObjects);
    m_groupedAdapter->Source(m_treeGroupSource);

    // A node toggle splices only the tree adapter; this carries it to the groups.
    std::weak_ptr<TableViewRowProjection> weakThis = weak_from_this();
    adapter->ProjectionChanged([weakThis]()
    {
        if (auto strongThis = weakThis.lock())
        {
            strongThis->ResliceTreeGroups();
        }
    });

    if (m_kind != Kind::GroupedTree)
    {
        m_itemsSourceView = m_groupedAdapter->Entries();
        m_rowMetadata = tabularPrimitives::RowMetadataProvider::CreateForGroupedHierarchicalRows(
            m_groupedAdapter, adapter, MakeIdentitySelector());
    }
    m_kind = Kind::GroupedTree;
}

void TableViewRowProjection::ResliceTreeGroups()
{
    auto const adapter = m_hierarchicalAdapter;
    if (!adapter || m_treeGroups.empty() || m_reslicing)
    {
        return;
    }

    m_reslicing = true;
    auto const done = wil::scope_exit([this]() noexcept { m_reslicing = false; });

    // Snapshot: SetItems below runs handlers that may rebuild the tree.
    auto const groups = m_treeGroups;
    uint64_t const generation = m_treeGeneration;

    std::vector<std::vector<winrt::IInspectable>> slices(groups.size());
    // A header counts roots, not the descendant rows its slice also holds.
    std::vector<int32_t> rootCounts(groups.size(), 0);

    auto const entries = adapter->Entries();
    int32_t const count = entries ? entries.Count() : 0;
    size_t bucket = 0;
    bool haveBucket = false;
    for (int32_t i = 0; i < count; ++i)
    {
        auto const* const node = adapter->TryGetNodeRow(i);
        if (!node)
        {
            continue;
        }

        if (node->Depth == 0)
        {
            auto const found = m_rootBucket.find(winrt::get_abi(node->Item));
            haveBucket = found != m_rootBucket.end() && found->second < groups.size();
            if (haveBucket)
            {
                bucket = found->second;
                ++rootCounts[bucket];
            }
        }

        if (haveBucket)
        {
            slices[bucket].push_back(node->Item);
        }
    }

    for (size_t i = 0; i < groups.size(); ++i)
    {
        if (generation != m_treeGeneration)
        {
            break;
        }
        // Count first: SetItems re-mints the header from it.
        groups[i]->GroupChildCount(rootCounts[i]);
        groups[i]->SetItems(slices[i]);
    }
}

void TableViewRowProjection::ReleaseTree()
{
    if (m_hierarchicalAdapter)
    {
        m_hierarchicalAdapter->ProjectionChanged(nullptr);
        // Also detaches: this may run inside its own notification, which must stop here.
        m_hierarchicalAdapter->ClearIndex();
        m_hierarchicalAdapter->ResetIntentQuietly();
        m_hierarchicalAdapter.reset();
    }
    if ((m_kind == Kind::GroupedTree) && m_groupedAdapter)
    {
        m_groupedAdapter->DetachSourceQuietly();
    }
    m_treeGroups.clear();
    m_treeGroupByIdentity.clear();
    m_rootBucket.clear();
    ++m_treeGeneration;
    if (m_treeGroupSource)
    {
        m_treeGroupSource.Clear();
    }
}

void TableViewRowProjection::OnViewChanged()
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

void TableViewRowProjection::ScheduleUpdate()
{
    if (m_updateQueued)
    {
        return;
    }

    std::weak_ptr<TableViewRowProjection> weakThis = weak_from_this();
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
            // The view's groups first: a change to them rebuilds the tree (OnGroupsChanged).
            auto const before = strongThis->m_updateCount;
            if (strongThis->m_groups)
            {
                strongThis->m_groups->RefreshNow();
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

void TableViewRowProjection::OnGroupsChanged()
{
    if (m_kind == Kind::GroupedTree)
    {
        Update();
    }
    else if (m_kind == Kind::Grouped && !m_updating)
    {
        RaiseProjectionChanged();
    }
}

void TableViewRowProjection::RaiseProjectionChanged() const
{
    if (auto const handler = m_projectionChanged)
    {
        handler();
    }
}

std::optional<winrt::hstring> TableViewRowProjection::TryGetGroupIdentityForKey(winrt::IInspectable const& key) const
{
    if (!m_groups)
    {
        return std::nullopt;
    }
    return TableViewCollectionViewGroups::IdentityForKey(key);
}

std::vector<TableViewRowProjection::GroupInfo> TableViewRowProjection::CurrentGroups() const
{
    std::vector<GroupInfo> groups;
    if (m_kind == Kind::GroupedTree)
    {
        for (auto const& group : m_treeGroups)
        {
            groups.push_back({ group->StableGroupIdentity(), group->Group(), group.as<winrt::IInspectable>() });
        }
    }
    else if (m_kind == Kind::Grouped && m_groups)
    {
        for (auto const& group : m_groups->CurrentGroups())
        {
            groups.push_back({ group.Identity, group.Key, group.Group });
        }
    }
    return groups;
}

uint32_t TableViewRowProjection::SourceRowCount() const
{
    return m_view.Size();
}

uint32_t TableViewRowProjection::DataRowCount() const
{
    if (m_hierarchicalAdapter && m_hierarchicalAdapter->HasIndex())
    {
        return m_hierarchicalAdapter->KeptNodeCount();
    }
    return m_view.Size();
}
