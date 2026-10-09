// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "HierarchicalSourceAdapter.h"
#include "RuntimeProfiler.h"

#include <algorithm>
#include <numeric>
#include <string_view>
#include <utility>

#include <winrt/Microsoft.UI.Dispatching.h>

namespace
{
    // Every node key the index mints starts with this. A bare prefix is not a key.
    constexpr std::wstring_view c_nodeKeyPrefix{ L"node:" };

    // Above this run length, one Rebuild + Reset beats per-row splice notifications.
    constexpr int32_t c_maxSpliceRows{ 64 };
}

HierarchicalSourceAdapter::HierarchicalSourceAdapter()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_HierarchicalSourceAdapter);

    auto queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    if (!queue)
    {
        throw winrt::hresult_error(RPC_E_WRONG_THREAD, L"HierarchicalSourceAdapter must be constructed on a UI thread.");
    }
    m_uiQueue = winrt::make_weak(queue);

    m_entriesView = winrt::ItemsSourceView{ m_entries };

    // Collapsed baseline so untouched nodes are not walked. Set before attaching the handler so it
    // raises no Change into a half-constructed adapter.
    m_expansion.SetAllExpanded(false);

    m_expansion.SetChangedHandler(
        [this](ShapingHelpers::RowExpansionModel::Change const& change)
        {
            if (m_suppressExpansionChanged)
            {
                m_expansionChangedWhileSuppressed = true;
                return;
            }
            OnExpansionChanged(change);
        });
}

template <typename Fn>
bool HierarchicalSourceAdapter::MutateExpansionQuietly(Fn&& fn)
{
    m_suppressExpansionChanged = true;
    m_expansionChangedWhileSuppressed = false;
    auto guard = wil::scope_exit([this]() noexcept { m_suppressExpansionChanged = false; });
    fn();
    return std::exchange(m_expansionChangedWhileSuppressed, false);
}

void HierarchicalSourceAdapter::ProjectionChanged(std::function<void()> fn)
{
    m_projectionChanged = std::move(fn);
}

void HierarchicalSourceAdapter::RaiseProjectionChanged()
{
    // Copied: the handler may reassign m_projectionChanged while running inside it.
    if (auto const handler = m_projectionChanged)
    {
        handler();
    }
}

bool HierarchicalSourceAdapter::IsNodeKey(winrt::hstring const& key)
{
    const std::wstring_view view{ key.c_str(), key.size() };
    return view.size() > c_nodeKeyPrefix.size() && view.starts_with(c_nodeKeyPrefix);
}

// --- Index hand-off -------------------------------------------------------------------------------

void HierarchicalSourceAdapter::SetIndex(std::shared_ptr<const ShapingHelpers::ParentKeyIndex> index, std::vector<size_t> rootSegments)
{
    // Pin self: a consumer reacting to the publish may drop the last external owner.
    auto const keepAlive = shared_from_this();

    m_detached = false;
    m_index = std::move(index);
    m_rootSegments = std::move(rootSegments);

    // A collapse override only means something on a row that is still a context row.
    if (m_index)
    {
        std::erase_if(m_filterOverlayCollapsed, [this](winrt::hstring const& key)
        {
            return !m_index->ContextKeys.contains(std::wstring{ key });
        });
    }
    else
    {
        m_filterOverlayCollapsed.clear();
    }

    Rebuild();

    // Pruned after publish, against the UNFILTERED key set, so intent for filtered-out rows survives.
    if (m_index && m_index->Structure && m_prunedForStructure.lock() != m_index->Structure)
    {
        // Same structure means already pruned.
        std::unordered_set<winrt::hstring> liveKeys;
        liveKeys.reserve(m_index->Structure->KeyByItem.size());
        for (auto const& [item, key] : m_index->Structure->KeyByItem)
        {
            liveKeys.emplace(winrt::hstring{ key });
        }
        m_expansion.RetainOnly(liveKeys);
        m_prunedForStructure = m_index->Structure;
    }
}

void HierarchicalSourceAdapter::ClearIndex()
{
    // No Rebuild: the repeater may still be bound mid-teardown, so an empty Reset is a hazard.
    // May run inside our own notification; m_detached stops the still-publishing frame.
    m_detached = true;
    m_index.reset();
    m_rootSegments.clear();
    m_filterOverlayCollapsed.clear();

    // Descriptors kept: stale rows may stay bound until the projection swap.
    m_indexByNodeKey.clear();
    m_indexByNodeKeyValid = true;
}

void HierarchicalSourceAdapter::ResetIntentQuietly()
{
    // SetAllExpanded moves the baseline and also clears every explicit intent.
    MutateExpansionQuietly([this]() { m_expansion.SetAllExpanded(false); });
    m_filterOverlayCollapsed.clear();
}

void HierarchicalSourceAdapter::ResetFilterOverlay()
{
    m_filterOverlayCollapsed.clear();
}

// --- The one responsibility: materialize the visible rows ----------------------------------------

bool HierarchicalSourceAdapter::EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const
{
    if (!hasChildren)
    {
        return false;
    }

    // Context rows follow the overlay only, so toggling them never writes intent.
    return isContext ? !m_filterOverlayCollapsed.contains(nodeKey) : m_expansion.IsExpanded(nodeKey);
}

bool HierarchicalSourceAdapter::EffectiveExpandedForKey(winrt::hstring const& nodeKey) const
{
    if (!m_index)
    {
        return m_expansion.IsExpanded(nodeKey);
    }

    const std::wstring key{ nodeKey };
    auto const* const children = m_index->TryGetChildren(key);
    return EffectiveExpanded(nodeKey, children && !children->empty(), m_index->ContextKeys.contains(key));
}

void HierarchicalSourceAdapter::EmitRange(
    std::vector<winrt::IInspectable> const& siblings,
    size_t begin,
    size_t end,
    winrt::hstring const& parentKey,
    int32_t depth,
    std::vector<winrt::IInspectable>& built,
    std::vector<NodeRow>& descriptors) const
{
    if (!m_index)
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
        auto const* const key = m_index->TryGetKey(item);
        if (!key)
        {
            // Every indexed item has a key; an unknown one cannot be addressed, so it is skipped.
            continue;
        }

        auto const* const children = m_index->TryGetChildren(*key);

        NodeRow row;
        row.Item = item;
        row.NodeKey = winrt::hstring{ *key };
        row.ParentKey = frame.ParentKey;
        row.Depth = frame.Depth;
        row.ChildCount = children ? static_cast<int32_t>(children->size()) : 0;
        row.SiblingIndex = static_cast<int32_t>(i - frame.Begin + 1);
        row.SiblingCount = static_cast<int32_t>(frame.End - frame.Begin);
        row.HasChildren = row.ChildCount > 0;
        row.IsContext = m_index->ContextKeys.contains(*key);
        row.IsExpanded = EffectiveExpanded(row.NodeKey, row.HasChildren, row.IsContext);

        // Captured before the push below, which may reallocate the stack and invalidate `frame`.
        const int32_t childDepth = frame.Depth + 1;
        const bool descend = row.IsExpanded;
        auto nodeKey = row.NodeKey;

        built.push_back(item);
        descriptors.push_back(std::move(row));

        if (descend)
        {
            stack.push_back({ children, 0, children->size(), 0, std::move(nodeKey), childDepth });
        }
    }
}

void HierarchicalSourceAdapter::Rebuild()
{
    AssertRebuildOnUiThread();

    if (m_detached)
    {
        // Retracted: publishing even an empty Reset would reach a repeater mid-teardown.
        return;
    }

    if (m_rebuildInFlight)
    {
        // Re-entered from a ReplaceAll notification: coalesce into one follow-up after unwind.
        m_pendingRebuild = true;
        return;
    }

    bool runPending = false;
    do
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        std::vector<winrt::IInspectable> built;
        std::vector<NodeRow> descriptors;

        if (m_index)
        {
            auto const& roots = m_index->Roots;
            if (m_rootSegments.empty())
            {
                EmitRange(roots, 0, roots.size(), winrt::hstring{}, 0, built, descriptors);
            }
            else
            {
                // One segment per group bucket, so a root's sibling position is per bucket.
                MUX_ASSERT(std::accumulate(m_rootSegments.begin(), m_rootSegments.end(), size_t{ 0 }) == roots.size());

                size_t segmentBegin = 0;
                for (const size_t length : m_rootSegments)
                {
                    const size_t segmentEnd = (std::min)(segmentBegin + length, roots.size());
                    EmitRange(roots, segmentBegin, segmentEnd, winrt::hstring{}, 0, built, descriptors);
                    segmentBegin = segmentEnd;
                }
            }
        }

        // Built into a local so a failure cannot publish a partial map.
        std::unordered_map<winrt::hstring, int32_t> indexByNodeKey;
        indexByNodeKey.reserve(descriptors.size());
        for (size_t i = 0; i < descriptors.size(); ++i)
        {
            indexByNodeKey.emplace(descriptors[i].NodeKey, static_cast<int32_t>(i));
        }

        // Publish metadata before the Reset so synchronous handlers see agreeing state. Moves; no-throw.
        auto previousDescriptors = std::exchange(m_descriptors, std::move(descriptors));
        auto previousIndex = std::exchange(m_indexByNodeKey, std::move(indexByNodeKey));
        const bool previousIndexValid = std::exchange(m_indexByNodeKeyValid, true);
        m_indexByItemValid = false;

        // Roll back metadata only if ReplaceAll's assignment threw (old rows still published, detected
        // by row count). A throw from a Reset handler happens after the new rows are live; keep them.
        const auto publishedCount = static_cast<uint32_t>(built.size());
        auto publishGuard = wil::scope_exit([this, publishedCount, &previousDescriptors, &previousIndex, previousIndexValid]() noexcept
        {
            if (m_entries.Size() == publishedCount)
            {
                return;
            }

            m_descriptors = std::move(previousDescriptors);
            m_indexByNodeKey = std::move(previousIndex);
            m_indexByNodeKeyValid = previousIndexValid;
            m_indexByItemValid = false;
        });

        m_entries.ReplaceAll(built);
        publishGuard.release();

        // resetGuard runs here, publishing into runPending whether a re-entrant request arrived.
    } while (runPending && !m_detached);

    if (m_detached)
    {
        // A handler of the Reset above retracted the projection; see ClearIndex.
        return;
    }

    // One edge for the whole rebuild.
    RaiseProjectionChanged();
}

// --- Expansion -----------------------------------------------------------------------------------

bool HierarchicalSourceAdapter::IsNodeExpanded(winrt::hstring const& nodeKey) const
{
    // Answer the presented state (incl. overlay), so toggling an overlay-expanded context row
    // collapses it rather than no-op expanding.
    int32_t index = -1;
    if (TryGetIndexForNodeKey(nodeKey, index))
    {
        if (auto const* const row = TryGetNodeRow(index))
        {
            return row->IsExpanded;
        }
    }

    // Not visible: context rows answer from the overlay, others from intent.
    if (m_index && m_index->ContextKeys.contains(std::wstring{ nodeKey }))
    {
        return EffectiveExpandedForKey(nodeKey);
    }
    return m_expansion.IsExpanded(nodeKey);
}

void HierarchicalSourceAdapter::SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded)
{
    auto const keepAlive = shared_from_this();

    if (!m_index)
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
        // Overlay-only for context rows; discarded when the filter changes.
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
            ApplySingleToggle(nodeKey, after);
        }
        return;
    }

    if (m_expansion.IsExpanded(nodeKey) != isExpanded)
    {
        // Through the model, so its Changed notification drives ApplySingleToggle.
        m_expansion.SetExpanded(nodeKey, isExpanded);
    }
    else if (before != isExpanded)
    {
        // Intent agrees but presented state does not (earlier materialization never landed); reconcile.
        ApplySingleToggle(nodeKey, isExpanded);
    }
}

void HierarchicalSourceAdapter::ExpandAll()
{
    auto const keepAlive = shared_from_this();

    const bool hadOverlay = !m_filterOverlayCollapsed.empty();
    m_filterOverlayCollapsed.clear();

    // Moves the baseline, so future nodes also arrive expanded.
    const bool changed = MutateExpansionQuietly([this]() { m_expansion.SetAllExpanded(true); });
    if (changed || hadOverlay)
    {
        Rebuild();
    }
}

void HierarchicalSourceAdapter::CollapseAll()
{
    auto const keepAlive = shared_from_this();

    const bool changed = MutateExpansionQuietly([this]() { m_expansion.SetAllExpanded(false); });

    // Context rows are overlay-expanded, so collapse-all must override each one.
    bool overlayChanged = false;
    if (m_index)
    {
        for (auto const& key : m_index->ContextKeys)
        {
            overlayChanged |= m_filterOverlayCollapsed.emplace(winrt::hstring{ key }).second;
        }
    }

    if (changed || overlayChanged)
    {
        Rebuild();
    }
}

void HierarchicalSourceAdapter::ExpandSubtree(winrt::hstring const& nodeKey)
{
    auto const keepAlive = shared_from_this();

    if (!m_index || !IsNodeKey(nodeKey))
    {
        return;
    }

    bool overlayChanged = false;
    const bool changed = MutateExpansionQuietly([&]()
    {
        std::vector<std::wstring> pending{ std::wstring{ nodeKey } };
        while (!pending.empty())
        {
            const std::wstring key = std::move(pending.back());
            pending.pop_back();

            auto const* const children = m_index->TryGetChildren(key);
            if (!children || children->empty())
            {
                continue;
            }

            const winrt::hstring hkey{ key };
            if (m_index->ContextKeys.contains(key))
            {
                // A context row is presented from the overlay alone; expanding it writes no intent.
                overlayChanged |= m_filterOverlayCollapsed.erase(hkey) > 0;
            }
            else
            {
                m_expansion.SetExpanded(hkey, true);
            }

            for (auto const& child : *children)
            {
                if (auto const* const childKey = m_index->TryGetKey(child))
                {
                    pending.push_back(*childKey);
                }
            }
        }
    });

    if (changed || overlayChanged)
    {
        Rebuild();
    }
}

void HierarchicalSourceAdapter::OnExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertRebuildOnUiThread();

    // Only a single-node toggle can splice; baseline moves and batches rebuild.
    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        Rebuild();
        return;
    }

    // Splice to the presented state; for context rows intent changes move nothing.
    ApplySingleToggle(change.Keys.front(), EffectiveExpandedForKey(change.Keys.front()));
}

void HierarchicalSourceAdapter::ApplySingleToggle(winrt::hstring const& nodeKey, bool isExpanded)
{
    AssertRebuildOnUiThread();

    if (m_rebuildInFlight)
    {
        // Raised mid-publish; the outer operation runs one coalesced Rebuild afterward.
        m_pendingRebuild = true;
        return;
    }

    bool runPending = false;
    bool applied = false;
    {
        m_rebuildInFlight = true;
        m_pendingRebuild = false;
        auto resetGuard = wil::scope_exit([this, &runPending]() noexcept
        {
            m_rebuildInFlight = false;
            runPending = m_pendingRebuild;
            m_pendingRebuild = false;
        });

        applied = TryApplyExpansionSplice(nodeKey, isExpanded);
    }

    // Re-entrant or unresolved: one Rebuild after unwind (no-op once detached).
    if (runPending || !applied)
    {
        Rebuild();
    }
}

bool HierarchicalSourceAdapter::TryApplyExpansionSplice(winrt::hstring const& nodeKey, bool expand)
{
    // Structures must already agree or a ranged splice would corrupt the pairing.
    if (!m_index || m_descriptors.size() != static_cast<size_t>(m_entries.Size()))
    {
        return false;
    }

    int32_t index = -1;
    if (!TryGetIndexForNodeKey(nodeKey, index) || index < 0 || static_cast<size_t>(index) >= m_descriptors.size())
    {
        // Not visible (behind a collapsed ancestor); Rebuild reconciles.
        return false;
    }

    auto const& node = m_descriptors[static_cast<size_t>(index)];
    if (node.IsExpanded == expand || !node.HasChildren)
    {
        return true;
    }

    const int32_t nodeDepth = node.Depth;

    if (!expand)
    {
        // Visible descendants = contiguous following rows with greater Depth.
        int32_t runEnd = index + 1;
        while (static_cast<size_t>(runEnd) < m_descriptors.size() && m_descriptors[static_cast<size_t>(runEnd)].Depth > nodeDepth)
        {
            ++runEnd;
        }

        if (runEnd - (index + 1) > c_maxSpliceRows)
        {
            // A large run costs one notification per row; one Reset is cheaper.
            return false;
        }

        // Descriptor first, so the row reads collapsed when RemoveRows notifies.
        m_descriptors[static_cast<size_t>(index)].IsExpanded = false;
        RemoveRows(index + 1, runEnd - (index + 1));
        if (!m_detached)
        {
            RaiseProjectionChanged();
        }
        return true;
    }

    auto const* const children = m_index->TryGetChildren(std::wstring{ nodeKey });
    if (!children || children->empty())
    {
        return false;
    }

    // Expand: emit the subtree from the index (honouring nested intent), then insert.
    std::vector<winrt::IInspectable> built;
    std::vector<NodeRow> descriptors;
    EmitRange(*children, 0, children->size(), nodeKey, nodeDepth + 1, built, descriptors);

    if (built.size() > static_cast<size_t>(c_maxSpliceRows))
    {
        return false;
    }

    // Parent first, for the same reason the collapse path does it.
    m_descriptors[static_cast<size_t>(index)].IsExpanded = true;
    InsertRows(index + 1, built, descriptors);
    if (!m_detached)
    {
        RaiseProjectionChanged();
    }
    return true;
}

// --- The triple invariant ------------------------------------------------------------------------

void HierarchicalSourceAdapter::InsertRows(
    int32_t index,
    std::vector<winrt::IInspectable> const& items,
    std::vector<NodeRow> const& descriptors)
{
    MUX_ASSERT(items.size() == descriptors.size());
    if (items.empty())
    {
        return;
    }

    // One row at a time, descriptor before its row: InsertAt notifies synchronously and handlers
    // read descriptors by index. The node-key map is invalidated and rebuilt lazily.
    uint32_t insertAt = static_cast<uint32_t>(index);
    for (size_t i = 0; i < items.size() && !m_detached; ++i)
    {
        m_descriptors.insert(m_descriptors.begin() + insertAt, descriptors[i]);
        m_indexByNodeKeyValid = false;
        m_indexByItemValid = false;
        m_entries.InsertAt(insertAt++, items[i]);
    }
}

void HierarchicalSourceAdapter::RemoveRows(int32_t index, int32_t count)
{
    if (count <= 0)
    {
        return;
    }

    // Mirror of InsertRows, removed from the end so remaining rows keep indices until removed.
    // Stops if a handler retracts the projection (see ClearIndex).
    for (int32_t i = index + count - 1; i >= index && !m_detached; --i)
    {
        m_descriptors.erase(m_descriptors.begin() + i);
        m_indexByNodeKeyValid = false;
        m_indexByItemValid = false;
        m_entries.RemoveAt(static_cast<uint32_t>(i));
    }
}

// --- Lookups --------------------------------------------------------------------------------------

HierarchicalSourceAdapter::NodeRow const* HierarchicalSourceAdapter::TryGetNodeRow(int32_t index) const
{
    if (index < 0 || static_cast<size_t>(index) >= m_descriptors.size())
    {
        return nullptr;
    }

    return &m_descriptors[static_cast<size_t>(index)];
}

HierarchicalSourceAdapter::NodeRow const* HierarchicalSourceAdapter::TryGetNodeRowForItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return nullptr;
    }

    EnsureItemIndex();

    const auto found = m_indexByItem.find(winrt::get_abi(item));
    if (found == m_indexByItem.end() ||
        found->second < 0 ||
        static_cast<size_t>(found->second) >= m_descriptors.size())
    {
        return nullptr;
    }

    return &m_descriptors[static_cast<size_t>(found->second)];
}

void HierarchicalSourceAdapter::EnsureItemIndex() const
{
    if (m_indexByItemValid)
    {
        return;
    }

    m_indexByItem.clear();
    m_indexByItem.reserve(m_descriptors.size());

    for (size_t i = 0; i < m_descriptors.size(); ++i)
    {
        if (auto const& descriptorItem = m_descriptors[i].Item)
        {
            m_indexByItem.emplace(winrt::get_abi(descriptorItem), static_cast<int32_t>(i));
        }
    }

    m_indexByItemValid = true;
}

void HierarchicalSourceAdapter::EnsureNodeKeyIndex() const
{
    if (m_indexByNodeKeyValid)
    {
        return;
    }

    m_indexByNodeKey.clear();
    m_indexByNodeKey.reserve(m_descriptors.size());

    for (size_t i = 0; i < m_descriptors.size(); ++i)
    {
        m_indexByNodeKey.emplace(m_descriptors[i].NodeKey, static_cast<int32_t>(i));
    }

    m_indexByNodeKeyValid = true;
}

bool HierarchicalSourceAdapter::TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const
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

// --- Threading ------------------------------------------------------------------------------------

bool HierarchicalSourceAdapter::OnUiThread() const
{
    auto const queue = m_uiQueue.get();
    return queue && queue.HasThreadAccess();
}

void HierarchicalSourceAdapter::AssertRebuildOnUiThread() const
{
    MUX_ASSERT(!m_uiQueue.get() || OnUiThread());
}
