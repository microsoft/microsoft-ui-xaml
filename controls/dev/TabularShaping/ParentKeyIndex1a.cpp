// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ParentKeyIndex1a.h"
#include "ShapingHelpers.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <string>
namespace ShapingHelpers {}
namespace pb1a
{
namespace ShapingHelpers { using namespace ::ShapingHelpers; }

namespace ShapingHelpers
{
namespace
{
    constexpr wchar_t c_nodePrefix[] = L"node:";
    constexpr size_t c_nodePrefixLength = std::size(c_nodePrefix) - 1;
    constexpr std::wstring_view c_valueKeyPrefix{ L"value:" };
    constexpr std::wstring_view c_objectKeyPrefix{ L"object:" };

    bool IsNoKey(winrt::IInspectable const& key)
    {
        if (!key) return true;
        if (auto pv = key.try_as<winrt::Windows::Foundation::IPropertyValue>())
        {
            if (pv.Type() == winrt::Windows::Foundation::PropertyType::Empty) return true;
            if (pv.Type() == winrt::Windows::Foundation::PropertyType::String && pv.GetString().empty()) return true;
        }
        return false;
    }

    winrt::IInspectable SafeSelect(KeySelector const& selector, winrt::IInspectable const& item)
    {
        try { return selector(item); } catch (...) { return nullptr; }
    }

    // Value boxes the shared formatter misses (e.g. WinRT enums as OtherType), keyed by
    // (type, value) so a fresh box per call keeps the key. Empty when not handled.
    std::wstring TryFormatExtraValueKey(winrt::IInspectable const& key)
    {
        auto const pv = key.try_as<winrt::Windows::Foundation::IPropertyValue>();
        if (!pv)
        {
            return {};
        }

        wchar_t buffer[64];
        switch (pv.Type())
        {
        case winrt::Windows::Foundation::PropertyType::OtherType:
        {
            std::wstring value;
            try
            {
                value = std::to_wstring(pv.GetInt64());
            }
            catch (...)
            {
                try
                {
                    value = L"u" + std::to_wstring(pv.GetUInt64());
                }
                catch (...)
                {
                    return {};
                }
            }

            std::wstring typeName;
            try { typeName = winrt::get_class_name(key); } catch (...) {}
            return std::wstring{ c_valueKeyPrefix } + L"e:" + typeName + L":" + value;
        }
        case winrt::Windows::Foundation::PropertyType::Point:
        {
            const auto p = pv.GetPoint();
            swprintf_s(buffer, L"pt:%08X,%08X", CanonicalFloatBits(p.X), CanonicalFloatBits(p.Y));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        case winrt::Windows::Foundation::PropertyType::Size:
        {
            const auto s = pv.GetSize();
            swprintf_s(buffer, L"sz:%08X,%08X", CanonicalFloatBits(s.Width), CanonicalFloatBits(s.Height));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        case winrt::Windows::Foundation::PropertyType::Rect:
        {
            const auto r = pv.GetRect();
            swprintf_s(buffer, L"rc:%08X,%08X,%08X,%08X",
                CanonicalFloatBits(r.X), CanonicalFloatBits(r.Y),
                CanonicalFloatBits(r.Width), CanonicalFloatBits(r.Height));
            return std::wstring{ c_valueKeyPrefix } + buffer;
        }
        default:
            return {};
        }
    }

    bool SafeFilter(ParentKeyFilter const& filter, winrt::IInspectable const& item)
    {
        try { return filter(item); } catch (...) { return false; }
    }

    // User-facing node key for error text: raw value, or a placeholder for an object key.
    winrt::hstring DescribeNodeKey(std::wstring const& nodeKey)
    {
        std::wstring_view view{ nodeKey };
        if (view.starts_with(c_nodePrefix))
        {
            view.remove_prefix(c_nodePrefixLength);
        }

        if (view.starts_with(c_objectKeyPrefix))
        {
            return L"(object key)";
        }

        if (view.starts_with(c_valueKeyPrefix))
        {
            view.remove_prefix(c_valueKeyPrefix.size());
            const bool isEnum = view.starts_with(L"e:");
            if (const auto tagEnd = view.find(L':'); tagEnd != std::wstring_view::npos)
            {
                view.remove_prefix(tagEnd + 1);
            }

            // An enum key is "<type>:<value>"; the value alone is what the app would recognise.
            if (isEnum)
            {
                if (const auto valueStart = view.rfind(L':'); valueStart != std::wstring_view::npos)
                {
                    view.remove_prefix(valueStart + 1);
                }
            }
        }

        return winrt::hstring{ view };
    }

    constexpr std::wstring_view c_identityPrefix{ L"node:0x" };

    std::atomic<uint64_t> s_nextStructureEpoch{ 0 };

    winrt::hstring NullKeyError(winrt::IInspectable const& item)
    {
        // Name the item's type rather than a position: callers may pass rows in any order.
        winrt::hstring typeName;
        if (item)
        {
            try { typeName = winrt::get_class_name(item); } catch (...) {}
        }
        return L"ParentBy: key selector returned null or empty for an item"
            + (typeName.empty() ? winrt::hstring{} : L" of type '" + typeName + L"'")
            + L".";
    }

    std::vector<NodeId>& SiblingsOf(ParentStructure& s, NodeId parent)
    {
        return parent ? s.Children[parent] : s.Roots;
    }

    void InsertByRank(ParentStructure& s, std::vector<NodeId>& list, NodeId node)
    {
        const size_t rank = s.Nodes.at(node).Rank;
        auto const at = std::lower_bound(list.begin(), list.end(), rank,
            [&s](NodeId sibling, size_t r) { return s.Nodes.at(sibling).Rank < r; });
        list.insert(at, node);
    }

    void EraseNode(std::vector<NodeId>& list, NodeId node)
    {
        if (auto const it = std::find(list.begin(), list.end(), node); it != list.end())
        {
            list.erase(it);
        }
    }

    void Link(ParentStructure& s, NodeId node, NodeId parent)
    {
        s.Nodes.at(node).Parent = parent;
        InsertByRank(s, SiblingsOf(s, parent), node);
        s.DirtyLists.insert(parent);
        s.EdgesChanged = true;
    }

    // Detaches `node` from its sibling list and from the orphan entry it may wait in.
    void Unlink(ParentStructure& s, NodeId node)
    {
        auto& n = s.Nodes.at(node);
        if (n.Parent)
        {
            auto& siblings = s.Children[n.Parent];
            EraseNode(siblings, node);
            if (siblings.empty())
            {
                s.Children.erase(n.Parent);
            }
        }
        else
        {
            EraseNode(s.Roots, node);
            if (!n.ParentKey.empty())
            {
                if (auto const it = s.Orphans.find(n.ParentKey); it != s.Orphans.end())
                {
                    EraseNode(it->second, node);
                    if (it->second.empty())
                    {
                        s.Orphans.erase(it);
                    }
                }
            }
        }
        s.DirtyLists.insert(n.Parent);
        s.EdgesChanged = true;
        n.Parent = nullptr;
    }

    // True when `ancestor` is `node` or above it.
    bool IsAncestorOrSelf(ParentStructure const& s, NodeId ancestor, NodeId node)
    {
        // Bounded: the structure is acyclic, so a longer walk means it is already corrupt.
        for (size_t steps = 0; node && steps <= s.Nodes.size(); ++steps)
        {
            if (node == ancestor)
            {
                return true;
            }
            node = s.Nodes.at(node).Parent;
        }
        return node != nullptr;
    }

    // Links `node` to the node its parent key names, or leaves it a root (an orphan root when the
    // key is unknown). False when the link would close a cycle.
    bool ResolveParent(ParentStructure& s, NodeId node)
    {
        auto const& parentKey = s.Nodes.at(node).ParentKey;
        if (parentKey.empty())
        {
            Link(s, node, nullptr);
            return true;
        }
        if (auto const it = s.NodeByKey.find(parentKey); it != s.NodeByKey.end())
        {
            if (IsAncestorOrSelf(s, node, it->second))
            {
                return false;
            }
            Link(s, node, it->second);
            return true;
        }
        s.Orphans[parentKey].push_back(node);
        Link(s, node, nullptr);
        return true;
    }

    // Moves the orphans waiting for `node`'s key under it.
    bool AdoptOrphans(ParentStructure& s, NodeId node)
    {
        auto const it = s.Orphans.find(s.Nodes.at(node).Key);
        if (it == s.Orphans.end())
        {
            return true;
        }
        auto const waiting = std::move(it->second);
        s.Orphans.erase(it);
        for (NodeId const orphan : waiting)
        {
            // May be a false positive while `node`'s own parent is about to move; a failed patch
            // only costs a rebuild.
            if (IsAncestorOrSelf(s, orphan, node))
            {
                return false;
            }
            EraseNode(s.Roots, orphan);
            s.DirtyLists.insert(nullptr);
            Link(s, orphan, node);
        }
        return true;
    }

    // Makes `node`'s children orphan roots waiting for its key.
    void OrphanChildren(ParentStructure& s, NodeId node)
    {
        auto const it = s.Children.find(node);
        if (it == s.Children.end())
        {
            return;
        }
        auto const children = std::move(it->second);
        s.Children.erase(it);
        s.DirtyLists.insert(node);
        for (NodeId const child : children)
        {
            auto& c = s.Nodes.at(child);
            c.Parent = nullptr;
            s.Orphans[c.ParentKey].push_back(child);
            Link(s, child, nullptr);
        }
    }

    void Rerank(ParentStructure& s, size_t from, size_t to)
    {
        for (size_t i = from; i < to; ++i)
        {
            s.Nodes.at(GetNodeId(s.Rows[i])).Rank = i;
        }
    }

    void SortByRank(ParentStructure& s, std::vector<NodeId>& list)
    {
        std::sort(list.begin(), list.end(),
            [&s](NodeId a, NodeId b) { return s.Nodes.at(a).Rank < s.Nodes.at(b).Rank; });
    }

    void ClearChangeLog(ParentStructure& s) noexcept
    {
        s.DirtyLists.clear();
        s.DirtyNodes.clear();
        s.RemovedItems.clear();
        s.EdgesChanged = false;
    }
}

NodeId GetNodeId(winrt::IInspectable const& item)
{
    // The canonical IUnknown: other interface pointers of one object may differ (tear-offs).
    return item ? winrt::get_abi(item.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
}

winrt::hstring FormatNodeIdentity(NodeId node)
{
    wchar_t buffer[32];
    swprintf_s(buffer, L"node:0x%llX", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(node)));
    return winrt::hstring{ buffer };
}

NodeId TryParseNodeIdentity(winrt::hstring const& identity) noexcept
{
    std::wstring_view view{ identity };
    if (!view.starts_with(c_identityPrefix) || view.size() == c_identityPrefix.size() ||
        view.size() > c_identityPrefix.size() + 16)
    {
        return nullptr;
    }
    view.remove_prefix(c_identityPrefix.size());

    uintptr_t value = 0;
    for (wchar_t const ch : view)
    {
        uint32_t digit;
        if (ch >= L'0' && ch <= L'9') digit = ch - L'0';
        else if (ch >= L'A' && ch <= L'F') digit = ch - L'A' + 10;
        else return nullptr;
        value = (value << 4) | digit;
    }
    return reinterpret_cast<NodeId>(value);
}

ParentNode const* ParentStructure::TryGetNode(NodeId node) const
{
    auto const it = Nodes.find(node);
    return it == Nodes.end() ? nullptr : &it->second;
}

std::vector<NodeId> const* ParentStructure::TryGetChildren(NodeId parent) const
{
    auto const it = Children.find(parent);
    return it == Children.end() ? nullptr : &it->second;
}

ParentKeyIndex::SiblingList const* ParentKeyIndex::TryGetChildren(NodeId node) const
{
    if (!Children)
    {
        return nullptr;
    }
    auto const it = Children->find(node);
    return it == Children->end() ? nullptr : it->second.get();
}

bool ParentKeyIndex::IsContext(NodeId node) const
{
    return ContextNodes && ContextNodes->contains(node);
}

bool ParentKeyIndex::ContainsNode(NodeId node) const
{
    return Structure && Structure->TryGetNode(node);
}

void ParentShapeCache::Clear() noexcept
{
    Epoch = 0;
    Passes.clear();
    State.clear();
    ContextNodes.reset();
    Lists.reset();
    Roots.clear();
}

ResolvedNode ResolveParentNode(
    winrt::IInspectable const& item,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector)
{
    ResolvedNode result;
    result.Item = item;
    result.KeyObject = SafeSelect(keySelector, item);
    result.Key = MakeNodeKey(result.KeyObject);
    result.ParentKeyObject = SafeSelect(parentKeySelector, item);
    result.ParentKey = MakeNodeKey(result.ParentKeyObject);
    return result;
}

bool BuildParentStructure(
    std::vector<winrt::IInspectable> const& rows,
    KeySelector const& keySelector,
    KeySelector const& parentKeySelector,
    ParentStructure& out,
    winrt::hstring& error)
{
    const size_t n = rows.size();

    ParentStructure result;
    result.Rows = rows;
    result.Nodes.reserve(n);
    result.NodeByKey.reserve(n);

    std::vector<NodeId> ids(n, nullptr);

    // Pass 1: keys, duplicates, null keys. Each node holds its key object, whose address is the
    // lookup key of an object key.
    for (size_t i = 0; i < n; ++i)
    {
        auto key = SafeSelect(keySelector, rows[i]);
        auto nodeKey = MakeNodeKey(key);
        if (nodeKey.empty() || !rows[i])
        {
            error = NullKeyError(rows[i]);
            return false;
        }
        ids[i] = GetNodeId(rows[i]);
        // The same object on two rows is reported as a duplicate key.
        auto const [node, inserted] = result.Nodes.try_emplace(ids[i]);
        if (!inserted || !result.NodeByKey.emplace(nodeKey, ids[i]).second)
        {
            error = L"ParentBy: duplicate key '" + DescribeNodeKey(inserted ? nodeKey : node->second.Key) + L"'.";
            return false;
        }
        node->second.Item = rows[i];
        node->second.KeyObject = std::move(key);
        node->second.Key = std::move(nodeKey);
        node->second.Rank = i;
    }

    // Pass 2: resolve parents; self-parent is an error, unknown parent = orphan root.
    for (size_t i = 0; i < n; ++i)
    {
        auto& node = result.Nodes.at(ids[i]);
        node.ParentKeyObject = SafeSelect(parentKeySelector, rows[i]);
        node.ParentKey = MakeNodeKey(node.ParentKeyObject);
        if (node.ParentKey.empty()) continue;
        if (node.ParentKey == node.Key)
        {
            error = L"ParentBy: item with key '" + DescribeNodeKey(node.Key) + L"' is its own parent.";
            return false;
        }
        if (auto const it = result.NodeByKey.find(node.ParentKey); it != result.NodeByKey.end())
        {
            node.Parent = it->second;
        }
        else
        {
            result.Orphans[node.ParentKey].push_back(ids[i]);
        }
    }

    // Sibling lists in source order.
    for (size_t i = 0; i < n; ++i)
    {
        const NodeId parent = result.Nodes.at(ids[i]).Parent;
        (parent ? result.Children[parent] : result.Roots).push_back(ids[i]);
    }

    // Cycle check: everything must be reachable from a root.
    std::unordered_set<NodeId> seen;
    seen.reserve(n);
    {
        std::vector<NodeId> stack(result.Roots.begin(), result.Roots.end());
        while (!stack.empty())
        {
            const NodeId node = stack.back();
            stack.pop_back();
            seen.insert(node);
            if (auto const children = result.TryGetChildren(node))
            {
                stack.insert(stack.end(), children->begin(), children->end());
            }
        }
    }
    if (seen.size() != n)
    {
        for (size_t i = 0; i < n; ++i)
        {
            if (!seen.contains(ids[i]))
            {
                // Unreached nodes hang below a cycle; the first revisited node is on the cycle.
                std::unordered_set<NodeId> onPath;
                NodeId p = ids[i];
                while (onPath.insert(p).second)
                {
                    p = result.Nodes.at(p).Parent;
                }
                error = L"ParentBy: cycle detected involving key '" + DescribeNodeKey(result.Nodes.at(p).Key) + L"'.";
                return false;
            }
        }
    }

    result.Epoch = ++s_nextStructureEpoch;
    out = std::move(result);
    return true;
}

bool InsertParentNodes(ParentStructure& s, size_t index, std::vector<ResolvedNode> const& nodes)
{
    if (index > s.Rows.size())
    {
        return false;
    }

    // Register every node first, so one inserted in the same batch can parent another.
    std::vector<NodeId> ids;
    ids.reserve(nodes.size());
    for (auto const& resolved : nodes)
    {
        if (!resolved.Item || resolved.Key.empty() || resolved.ParentKey == resolved.Key)
        {
            return false;
        }
        const NodeId id = GetNodeId(resolved.Item);
        if (!s.Nodes.try_emplace(id).second || !s.NodeByKey.emplace(resolved.Key, id).second)
        {
            return false;
        }
        auto& node = s.Nodes.at(id);
        node.Item = resolved.Item;
        node.KeyObject = resolved.KeyObject;
        node.ParentKeyObject = resolved.ParentKeyObject;
        node.Key = resolved.Key;
        node.ParentKey = resolved.ParentKey;
        ids.push_back(id);
    }

    s.Rows.insert(s.Rows.begin() + index, nodes.size(), nullptr);
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        s.Rows[index + i] = nodes[i].Item;
    }
    Rerank(s, index, s.Rows.size());

    // A new node has no parent yet, so adopting cannot close a cycle; linking the new node's own
    // parent edge afterwards is checked.
    for (NodeId const id : ids)
    {
        if (!AdoptOrphans(s, id))
        {
            return false;
        }
    }
    for (NodeId const id : ids)
    {
        if (!ResolveParent(s, id))
        {
            return false;
        }
        s.DirtyNodes.insert(id);
    }
    return true;
}

bool RemoveParentNodes(ParentStructure& s, size_t index, size_t count, std::vector<winrt::IInspectable> const& expected)
{
    if (index > s.Rows.size() || count > s.Rows.size() - index ||
        (!expected.empty() && expected.size() != count))
    {
        return false;
    }

    std::vector<NodeId> ids;
    ids.reserve(count);
    for (size_t i = 0; i < count; ++i)
    {
        const NodeId id = GetNodeId(s.Rows[index + i]);
        if (!expected.empty() && GetNodeId(expected[i]) != id)
        {
            return false;
        }
        ids.push_back(id);
    }

    // Ranks stay valid until the rows are erased, so orphaned children re-enter the roots in order.
    for (NodeId const id : ids)
    {
        Unlink(s, id);
        OrphanChildren(s, id);
    }
    for (NodeId const id : ids)
    {
        auto node = s.Nodes.extract(id);
        s.NodeByKey.erase(node.mapped().Key);
        s.DirtyLists.erase(id);
        s.DirtyNodes.erase(id);
        s.RemovedItems.push_back(std::move(node.mapped().Item));
    }

    s.Rows.erase(s.Rows.begin() + index, s.Rows.begin() + index + count);
    Rerank(s, index, s.Rows.size());
    return true;
}

bool MoveParentNodes(ParentStructure& s, size_t oldIndex, size_t newIndex, size_t count)
{
    const size_t n = s.Rows.size();
    if (oldIndex > n || count > n - oldIndex || newIndex > n - count)
    {
        return false;
    }

    std::vector<winrt::IInspectable> moved(s.Rows.begin() + oldIndex, s.Rows.begin() + oldIndex + count);
    s.Rows.erase(s.Rows.begin() + oldIndex, s.Rows.begin() + oldIndex + count);
    s.Rows.insert(s.Rows.begin() + newIndex, moved.begin(), moved.end());
    Rerank(s, std::min(oldIndex, newIndex), std::max(oldIndex, newIndex) + count);

    // Only the moved rows' sibling lists can have changed order.
    std::unordered_set<NodeId> parents;
    for (auto const& item : moved)
    {
        parents.insert(s.Nodes.at(GetNodeId(item)).Parent);
    }
    for (NodeId const parent : parents)
    {
        SortByRank(s, SiblingsOf(s, parent));
        s.DirtyLists.insert(parent);
    }
    return true;
}

bool UpdateParentNode(ParentStructure& s, ResolvedNode const& resolved)
{
    const NodeId id = GetNodeId(resolved.Item);
    auto const found = s.Nodes.find(id);
    if (found == s.Nodes.end())
    {
        return true;
    }

    // Sort and filter inputs may have moved with or without the keys.
    s.DirtyNodes.insert(id);
    s.DirtyLists.insert(found->second.Parent);

    if (resolved.Key.empty() || resolved.ParentKey == resolved.Key)
    {
        return false;
    }

    auto& node = found->second;
    const bool keyChanged = node.Key != resolved.Key;
    const bool parentChanged = node.ParentKey != resolved.ParentKey;
    node.KeyObject = resolved.KeyObject;

    if (keyChanged)
    {
        if (s.NodeByKey.contains(resolved.Key))
        {
            return false;
        }
        s.NodeByKey.erase(node.Key);
        OrphanChildren(s, id);
        node.Key = resolved.Key;
        s.NodeByKey.emplace(node.Key, id);
        if (!AdoptOrphans(s, id))
        {
            return false;
        }
    }

    // A key change can also resolve the parent differently (adopting moved nodes above it).
    if (keyChanged || parentChanged)
    {
        Unlink(s, id);
        node.ParentKey = resolved.ParentKey;
        node.ParentKeyObject = resolved.ParentKeyObject;
        return ResolveParent(s, id);
    }

    node.ParentKeyObject = resolved.ParentKeyObject;
    return true;
}

void BuildParentKeyIndex(
    std::shared_ptr<ParentStructure> const& structure,
    ParentShapeCache& cache,
    ParentKeyFilter const& filter,
    uint64_t filterGeneration,
    SiblingSorter const& sort,
    uint64_t sortGeneration,
    bool sortRoots,
    ParentKeyIndex& out)
{
    auto& s = *structure;
    const bool hasFilter = static_cast<bool>(filter);
    const bool hasSort = static_cast<bool>(sort);

    // Everything is rebuilt for another structure, or when the sort or the presence of a filter
    // changed. A changed filter re-reads every verdict but rebuilds only lists whose membership moved.
    const bool reset = cache.Epoch != s.Epoch || !cache.Lists;
    const bool listsReset = reset ||
        cache.HasSort != hasSort || (hasSort && cache.SortGeneration != sortGeneration) ||
        cache.RootsSorted != sortRoots || cache.HasFilter != hasFilter;
    const bool filterReset = listsReset || (hasFilter && cache.FilterGeneration != filterGeneration);

    try
    {
        if (reset)
        {
            cache.Clear();
        }

        for (auto const& item : s.RemovedItems)
        {
            const NodeId id = GetNodeId(item);
            cache.Passes.erase(id);
            cache.State.erase(id);
        }

        std::unordered_set<NodeId> dirtyLists = s.DirtyLists;

        // Filter: 0 = out, 1 = match, 2 = context (ancestor of a match).
        if (hasFilter)
        {
            bool contextDirty = filterReset || s.EdgesChanged || !s.RemovedItems.empty() || !cache.ContextNodes;
            auto const evaluate = [&](NodeId id, winrt::IInspectable const& item)
            {
                const bool passes = SafeFilter(filter, item);
                auto const [entry, inserted] = cache.Passes.try_emplace(id, passes);
                if (inserted || entry->second != passes)
                {
                    entry->second = passes;
                    contextDirty = true;
                }
            };
            if (filterReset)
            {
                for (auto const& [id, node] : s.Nodes) evaluate(id, node.Item);
            }
            else
            {
                for (NodeId const id : s.DirtyNodes)
                {
                    if (auto const node = s.TryGetNode(id)) evaluate(id, node->Item);
                }
            }

            if (contextDirty)
            {
                // Flat O(n) pass; runs only when a verdict, an edge or the filter moved.
                std::unordered_map<NodeId, uint8_t> state;
                state.reserve(s.Nodes.size());
                for (auto const& [id, node] : s.Nodes) state.emplace(id, static_cast<uint8_t>(cache.Passes.at(id) ? 1 : 0));
                auto context = std::make_shared<std::unordered_set<NodeId>>();
                for (auto const& [id, node] : s.Nodes)
                {
                    if (state.at(id) != 1) continue;
                    for (NodeId p = node.Parent; p; p = s.Nodes.at(p).Parent)
                    {
                        auto& parentState = state.at(p);
                        if (parentState != 0) break;
                        parentState = 2;
                        context->insert(p);
                    }
                }
                // A node joining or leaving the view changes its parent's list, and its own list
                // (skipped while it was out) must be (re)built.
                for (auto const& [id, value] : state)
                {
                    auto const old = cache.State.find(id);
                    if (old == cache.State.end() || (old->second != 0) != (value != 0))
                    {
                        dirtyLists.insert(s.Nodes.at(id).Parent);
                        dirtyLists.insert(id);
                    }
                }
                cache.State = std::move(state);
                cache.ContextNodes = std::move(context);
            }
        }
        else
        {
            cache.Passes.clear();
            cache.State.clear();
            if (!cache.ContextNodes || !cache.ContextNodes->empty())
            {
                cache.ContextNodes = std::make_shared<const std::unordered_set<NodeId>>();
            }
        }

        auto const isMember = [&](NodeId id) { return !hasFilter || cache.State.at(id) != 0; };
        auto const shape = [&](std::vector<NodeId> const& ids, bool sorted)
        {
            ParentKeyIndex::SiblingList list;
            for (NodeId const id : ids)
            {
                if (isMember(id)) list.push_back(s.Nodes.at(id).Item);
            }
            // Per-sibling-list stable sort over source order breaks ties like one global sort.
            if (sorted && hasSort && list.size() > 1) sort(list);
            return list;
        };

        if (listsReset)
        {
            auto lists = std::make_shared<ParentShapeCache::ListMap>();
            lists->reserve(s.Children.size());
            for (auto const& [parent, children] : s.Children)
            {
                if (!isMember(parent)) continue;
                auto list = shape(children, true);
                if (!list.empty()) lists->emplace(parent, std::make_shared<const ParentKeyIndex::SiblingList>(std::move(list)));
            }
            cache.Lists = std::move(lists);
            cache.Roots = shape(s.Roots, sortRoots);
        }
        else
        {
            bool copied = false;
            auto const lists = [&]() -> ParentShapeCache::ListMap&
            {
                // Copy-on-write: the published index shares the map.
                if (!copied)
                {
                    cache.Lists = std::make_shared<ParentShapeCache::ListMap>(*cache.Lists);
                    copied = true;
                }
                return *cache.Lists;
            };
            for (auto const& item : s.RemovedItems)
            {
                if (cache.Lists->contains(GetNodeId(item))) lists().erase(GetNodeId(item));
            }
            for (NodeId const parent : dirtyLists)
            {
                if (!parent)
                {
                    cache.Roots = shape(s.Roots, sortRoots);
                    continue;
                }
                ParentKeyIndex::SiblingList list;
                if (s.TryGetNode(parent) && isMember(parent))
                {
                    if (auto const children = s.TryGetChildren(parent)) list = shape(*children, true);
                }
                if (!list.empty())
                {
                    lists()[parent] = std::make_shared<const ParentKeyIndex::SiblingList>(std::move(list));
                }
                else if (cache.Lists->contains(parent))
                {
                    lists().erase(parent);
                }
            }
        }
    }
    catch (...)
    {
        cache.Clear();
        throw;
    }

    cache.Epoch = s.Epoch;
    cache.HasFilter = hasFilter;
    cache.FilterGeneration = filterGeneration;
    cache.HasSort = hasSort;
    cache.SortGeneration = sortGeneration;
    cache.RootsSorted = sortRoots;
    ClearChangeLog(s);

    ParentKeyIndex result;
    result.Roots = cache.Roots;
    result.Children = cache.Lists;
    result.ContextNodes = cache.ContextNodes;
    result.Structure = structure;
    out = std::move(result);
}

std::wstring MakeNodeKey(winrt::IInspectable const& key)
{
    if (IsNoKey(key)) return {};
    if (auto extra = TryFormatExtraValueKey(key); !extra.empty())
    {
        return std::wstring{ c_nodePrefix } + extra;
    }
    return std::wstring{ c_nodePrefix } + std::wstring{ ValueKey::ToObjectLookupKey(key, false) };
}
}

} // namespace pb1a
