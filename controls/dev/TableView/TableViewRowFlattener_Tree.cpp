// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Tree rows: the nodes of a view that builds a hierarchy (ICollectionViewHierarchy). The view owns the tree (keys,
// filter, sort); this only lays out the visible nodes, follows the node lists it shows, and keeps expansion intent.

#include "pch.h"
#include "common.h"
#include "TableViewRowFlattener.h"
#include "ShapingHelpers.h"
#include "ShapingRevoke.h"
#include "TVDiag.h"

#include <string>

namespace
{
    using NodeList = winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable>;
    using ViewNode = winrt::Microsoft::UI::Xaml::Data::ICollectionViewNode;

    constexpr std::wstring_view c_nodeKeyPrefix{ L"node:" };
    // Above this run length, one Rebuild + Reset beats per-row splice notifications.
    constexpr int32_t c_maxSpliceRows{ 64 };

    std::optional<std::wstring> ScalarKeyText(winrt::IInspectable const& value)
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
        case PropertyType::Single:
        case PropertyType::Double:
        {
            // Bit-exact, as the view compares them: decimal text would merge nearby values (1e-7 and 2e-7). -0 is 0.
            double value = (type == PropertyType::Single) ? static_cast<double>(scalar.GetSingle()) : scalar.GetDouble();
            value = (value == 0) ? 0 : value;
            uint64_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value));
            memcpy(&bits, &value, sizeof(bits));
            wchar_t buffer[17]{};
            swprintf_s(buffer, L"%016llx", static_cast<unsigned long long>(bits));
            text = buffer;
            break;
        }
        case PropertyType::DateTime: text = std::to_wstring(scalar.GetDateTime().time_since_epoch().count()); break;
        case PropertyType::TimeSpan: text = std::to_wstring(scalar.GetTimeSpan().count()); break;
        case PropertyType::Guid:
        {
            wchar_t buffer[40]{};
            if (::StringFromGUID2(scalar.GetGuid(), buffer, ARRAYSIZE(buffer)) == 0)
            {
                return std::nullopt;
            }
            text = buffer;
            break;
        }
        default:
            return std::nullopt;
        }

        // The type is part of the key, as the view compares keys (Int32 5 and Int64 5 differ).
        return std::to_wstring(static_cast<int>(type)) + L":" + text;
    }
}

bool TableViewRowFlattener::IsNodeKey(winrt::hstring const& key)
{
    const std::wstring_view view{ key.c_str(), key.size() };
    return view.size() > c_nodeKeyPrefix.size() && view.starts_with(c_nodeKeyPrefix);
}

// The node's Key in lookup form, so expansion survives the view recreating its nodes; a node without a key is
// addressed by its item (item: the one it holds now, when the node may not have it yet).
winrt::hstring TableViewRowFlattener::NodeKeyFor(ViewNode const& node, winrt::IInspectable const& item)
{
    auto const key = node.Key();
    if (!key)
    {
        return winrt::hstring{ std::wstring(c_nodeKeyPrefix) + L"i" + std::wstring(ObjectIdentity(item ? item : node.Item())) };
    }
    if (auto text = ScalarKeyText(key))
    {
        return winrt::hstring{ std::wstring(c_nodeKeyPrefix) + L"v" + *text };
    }
    return winrt::hstring{ std::wstring(c_nodeKeyPrefix) + L"o" + std::wstring(ObjectIdentity(key)) };
}

bool TableViewRowFlattener::AttachViewNodesNow()
{
    if (!m_hierarchy)
    {
        return false;
    }

    NodeList nodes{ nullptr };
    try
    {
        nodes = m_hierarchy.CollectionNodes();
    }
    catch (...)
    {
        TVDiag::LogRetailF(L"[TableView] The view's CollectionNodes could not be read (HRESULT 0x%08X); showing its items flat.",
            static_cast<unsigned int>(winrt::to_hresult()));
    }

    if (nodes == m_viewNodes)
    {
        return false;
    }

    if (m_viewNodes && m_viewNodesChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_viewNodes.VectorChanged(m_viewNodesChangedToken); });
    }
    if (m_viewItemsChangedToken.value)
    {
        ShapingHelpers::SafeRevokeWith([&] { m_view.VectorChanged(m_viewItemsChangedToken); });
    }
    m_viewNodesChangedToken = {};
    m_viewItemsChangedToken = {};
    m_viewNodes = nodes;

    if (nodes)
    {
        std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
        m_viewNodesChangedToken = nodes.VectorChanged(
            [weakThis](NodeList const& sender, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->OnNodeListChanged({}, sender, args);
                }
            });

        // A Replace that keeps the item's keys keeps its node; only the flat list says which item it now holds. The flat
        // list changes after the node lists, so it also ends a change whose node lists the rows could not follow.
        m_viewItemsChangedToken = m_view.VectorChanged(
            [weakThis](auto const&, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
            {
                if (auto strongThis = weakThis.lock())
                {
                    strongThis->OnViewItemsChanged(args);
                }
            });
    }

    // Read again by the next Update.
    if (m_kind == Kind::Tree)
    {
        m_kind = Kind::None;
        m_rowMetadataStale = true;
    }
    m_filterOverlayCollapsed.clear();
    return true;
}

void TableViewRowFlattener::DetachTreeRows()
{
    if (!m_treeRowsAttached)
    {
        return;
    }
    m_treeRowsAttached = false;
    auto rows = std::exchange(m_nodeRows, {});
    for (auto& row : rows)
    {
        UnsubscribeFromChildren(row);
    }
    m_indexByNodeKey.clear();
    m_indexByNodeKeyValid = false;
    m_treeRowsStale = false;
    m_filterOverlayCollapsed.clear();
    m_removedNodeKeys.clear();
    // A new list, not Clear(): the repeater may still show the old one until the rows change, and needs no Reset. The
    // old one, and the items it holds, go when the repeater lets go of it.
    m_treeRows = winrt::single_threaded_observable_vector<winrt::IInspectable>();
    m_treeRowsView = winrt::ItemsSourceView{ m_treeRows };
}

bool TableViewRowFlattener::EffectiveExpanded(winrt::hstring const& nodeKey, bool hasChildren, bool isContext) const
{
    if (!hasChildren)
    {
        return false;
    }
    if (isContext)
    {
        return !m_filterOverlayCollapsed.contains(nodeKey);
    }
    return m_nodeExpansion.IsExpanded(nodeKey);
}

TableViewRowFlattener::NodeRow TableViewRowFlattener::MakeNodeRow(
    ViewNode const& node,
    winrt::hstring const& parentKey,
    int32_t depth,
    int32_t siblingIndex,
    int32_t siblingCount)
{
    NodeRow row;
    row.Node = node;
    row.Item = node.Item();
    row.NodeKey = NodeKeyFor(node);
    row.ParentKey = parentKey;
    row.Depth = depth;
    row.SiblingIndex = siblingIndex;
    row.SiblingCount = siblingCount;
    row.Children = node.Children();
    row.ChildCount = row.Children ? static_cast<int32_t>(row.Children.Size()) : 0;
    row.HasChildren = row.ChildCount > 0;
    row.IsContext = node.IsContext();
    row.IsExpanded = EffectiveExpanded(row.NodeKey, row.HasChildren, row.IsContext);
    return row;
}

void TableViewRowFlattener::EmitNodeRows(
    NodeList const& list,
    uint32_t begin,
    uint32_t end,
    winrt::hstring const& parentKey,
    int32_t depth,
    std::vector<winrt::IInspectable>& items,
    std::vector<NodeRow>& rows)
{
    struct Frame
    {
        NodeList List{ nullptr };
        uint32_t Next{ 0 };
        uint32_t End{ 0 };
        int32_t Count{ 0 };
        winrt::hstring ParentKey;
        int32_t Depth{ 0 };
    };

    std::vector<Frame> stack;
    stack.push_back({ list, begin, end, static_cast<int32_t>(list.Size()), parentKey, depth });
    while (!stack.empty())
    {
        auto& frame = stack.back();
        if (frame.Next >= frame.End)
        {
            stack.pop_back();
            continue;
        }

        const uint32_t index = frame.Next++;
        auto const node = frame.List.GetAt(index).try_as<ViewNode>();
        if (!node)
        {
            continue;
        }

        auto row = MakeNodeRow(node, frame.ParentKey, frame.Depth, static_cast<int32_t>(index) + 1, frame.Count);
        const int32_t childDepth = frame.Depth + 1;
        auto const children = row.IsExpanded ? row.Children : nullptr;
        auto const key = row.NodeKey;
        items.push_back(row.Item);
        rows.push_back(std::move(row));
        // May grow the stack: frame is not used after this.
        if (children)
        {
            const uint32_t count = children.Size();
            stack.push_back({ children, 0, count, static_cast<int32_t>(count), key, childDepth });
        }
    }
}

void TableViewRowFlattener::RebuildTreeRows()
{
    AssertOnUiThread();

    if (m_treeRowsInFlight)
    {
        m_treeRowsPending = true;
        return;
    }

    auto const keepAlive = shared_from_this();
    m_treeRowsStale = false;
    int passes = 0;
    do
    {
        if (passes++ == c_maxTreeRebuildPasses)
        {
            // A handler that changes the tree on every rebuild would keep the UI thread here; the rest waits a turn.
            TVDiag::LogRetailF(L"[TableView] The view's tree changed during each of %d consecutive row rebuilds; rebuilding again on the next turn.",
                c_maxTreeRebuildPasses);
            m_treeRowsPending = false;
            QueueTreeRowsRebuild();
            break;
        }

        m_treeRowsInFlight = true;
        m_treeRowsPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_treeRowsInFlight = false; });

        std::vector<winrt::IInspectable> items;
        std::vector<NodeRow> rows;
        if (auto const nodes = m_viewNodes)
        {
            try
            {
                EmitNodeRows(nodes, 0, nodes.Size(), {}, 0, items, rows);
            }
            catch (...)
            {
                TVDiag::LogRetailF(L"[TableView] The view's nodes could not be read (HRESULT 0x%08X).",
                    static_cast<unsigned int>(winrt::to_hresult()));
                items.clear();
                rows.clear();
            }
        }
        ApplyTreeRows(std::move(items), std::move(rows));
    } while (m_treeRowsPending);

    RunPendingAttachViewGroups();
}

void TableViewRowFlattener::QueueTreeRowsRebuild()
{
    m_treeRowsStale = true;
    if (m_treeRebuildQueued)
    {
        return;
    }
    auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    m_treeRebuildQueued = queue && queue.TryEnqueue([weakThis]()
    {
        if (auto const strongThis = weakThis.lock())
        {
            strongThis->m_treeRebuildQueued = false;
            if (strongThis->m_treeRowsAttached && strongThis->m_treeRowsStale)
            {
                strongThis->RebuildTreeRows();
            }
        }
    });
}

void TableViewRowFlattener::ApplyTreeRows(std::vector<winrt::IInspectable>&& items, std::vector<NodeRow>&& rows)
{
    auto previous = std::exchange(m_nodeRows, std::move(rows));
    for (auto& row : previous)
    {
        UnsubscribeFromChildren(row);
    }
    for (auto& row : m_nodeRows)
    {
        SubscribeToChildren(row);
    }
    m_indexByNodeKeyValid = false;
    m_treeRows.ReplaceAll(items);
}

void TableViewRowFlattener::SubscribeToChildren(NodeRow& row)
{
    if (!row.Children || row.ChildrenToken.value)
    {
        return;
    }

    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    row.ChildrenToken = row.Children.VectorChanged(
        [weakThis, key = row.NodeKey](NodeList const& sender, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
        {
            if (auto strongThis = weakThis.lock())
            {
                strongThis->OnNodeListChanged(key, sender, args);
            }
        });
}

void TableViewRowFlattener::UnsubscribeFromChildren(NodeRow& row)
{
    if (row.Children && row.ChildrenToken.value)
    {
        auto const children = row.Children;
        auto const token = std::exchange(row.ChildrenToken, {});
        ShapingHelpers::SafeRevokeWith([&] { children.VectorChanged(token); });
    }
}

void TableViewRowFlattener::EnsureNodeKeyIndex() const
{
    if (m_indexByNodeKeyValid)
    {
        return;
    }
    m_indexByNodeKey.clear();
    m_indexByNodeKey.reserve(m_nodeRows.size());
    m_indexByNode.clear();
    m_indexByNode.reserve(m_nodeRows.size());
    for (int32_t i = 0; i < static_cast<int32_t>(m_nodeRows.size()); ++i)
    {
        m_indexByNodeKey.emplace(m_nodeRows[i].NodeKey, i);
        m_indexByNode.emplace(winrt::get_abi(m_nodeRows[i].Node), i);
    }
    m_indexByNodeKeyValid = true;
}

bool TableViewRowFlattener::TryGetIndexForNode(ViewNode const& node, int32_t& index) const
{
    index = -1;
    if (m_kind != Kind::Tree || !node)
    {
        return false;
    }
    EnsureNodeKeyIndex();
    auto const found = m_indexByNode.find(winrt::get_abi(node));
    if (found == m_indexByNode.end())
    {
        return false;
    }
    index = found->second;
    return true;
}

TableViewRowFlattener::NodeRow const* TableViewRowFlattener::TryGetNodeRow(int32_t index) const
{
    if (m_kind != Kind::Tree || index < 0 || index >= static_cast<int32_t>(m_nodeRows.size()))
    {
        return nullptr;
    }
    return &m_nodeRows[index];
}

bool TableViewRowFlattener::TryGetIndexForNodeKey(winrt::hstring const& nodeKey, int32_t& index) const
{
    index = -1;
    if (m_kind != Kind::Tree)
    {
        return false;
    }
    EnsureNodeKeyIndex();
    auto const found = m_indexByNodeKey.find(nodeKey);
    if (found == m_indexByNodeKey.end())
    {
        return false;
    }
    index = found->second;
    return true;
}

int32_t TableViewRowFlattener::SubtreeEnd(int32_t row) const
{
    const int32_t size = static_cast<int32_t>(m_nodeRows.size());
    const int32_t depth = m_nodeRows[row].Depth;
    int32_t end = row + 1;
    while (end < size && m_nodeRows[end].Depth > depth)
    {
        ++end;
    }
    return end;
}

void TableViewRowFlattener::RenumberSiblingRows(int32_t parentRow)
{
    const int32_t size = static_cast<int32_t>(m_nodeRows.size());
    const int32_t depth = (parentRow < 0) ? 0 : m_nodeRows[parentRow].Depth + 1;
    std::vector<int32_t> siblings;
    for (int32_t row = parentRow + 1; row < size && m_nodeRows[row].Depth == depth; row = SubtreeEnd(row))
    {
        siblings.push_back(row);
    }

    const int32_t count = static_cast<int32_t>(siblings.size());
    for (int32_t i = 0; i < count; ++i)
    {
        m_nodeRows[siblings[i]].SiblingIndex = i + 1;
        m_nodeRows[siblings[i]].SiblingCount = count;
    }

    if (parentRow >= 0)
    {
        auto& parent = m_nodeRows[parentRow];
        parent.ChildCount = parent.Children ? static_cast<int32_t>(parent.Children.Size()) : 0;
        parent.HasChildren = parent.ChildCount > 0;
        parent.IsExpanded = parent.IsExpanded && parent.HasChildren;
    }
}

void TableViewRowFlattener::InsertTreeRows(int32_t index, std::vector<winrt::IInspectable> const& items, std::vector<NodeRow>&& rows)
{
    m_indexByNodeKeyValid = false;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        SubscribeToChildren(rows[i]);
        m_nodeRows.insert(m_nodeRows.begin() + index + i, std::move(rows[i]));
        m_treeRows.InsertAt(static_cast<uint32_t>(index + i), items[i]);
    }
}

void TableViewRowFlattener::RemoveTreeRows(int32_t index, int32_t count)
{
    m_indexByNodeKeyValid = false;
    for (int32_t i = 0; i < count; ++i)
    {
        UnsubscribeFromChildren(m_nodeRows[index]);
        m_nodeRows.erase(m_nodeRows.begin() + index);
        m_treeRows.RemoveAt(static_cast<uint32_t>(index));
    }
}

void TableViewRowFlattener::RaiseRowMetadataChanged() const
{
    if (auto const handler = m_rowMetadataChanged)
    {
        handler();
    }
}

void TableViewRowFlattener::OnNodeListChanged(
    winrt::hstring const& parentKey,
    NodeList const& list,
    winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    AssertOnUiThread();
    if (!m_treeRowsAttached)
    {
        return;
    }

    if (m_treeRowsInFlight)
    {
        m_treeRowsPending = true;
        return;
    }

    const auto change = args.CollectionChange();
    const bool isRootsReset = parentKey.empty() && change == winrt::Windows::Foundation::Collections::CollectionChange::Reset;
    if (m_treeRowsStale && !isRootsReset)
    {
        // Rebuilt once the change ends (OnViewItemsChanged), not per node list it touches.
        return;
    }

    auto const keepAlive = shared_from_this();
    bool applied = false;
    {
        m_treeRowsInFlight = true;
        m_treeRowsPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_treeRowsInFlight = false; });
        try
        {
            applied = !isRootsReset && TryApplyNodeListChange(parentKey, list, change, args.Index());
        }
        catch (...)
        {
            applied = false;
        }
    }

    if (isRootsReset)
    {
        // The view recomputed (a new filter, sort or Refresh): context rows start expanded again.
        m_filterOverlayCollapsed.clear();
        RebuildTreeRows();
    }
    else if (!applied)
    {
        // A long splice: one rebuild when the view's flat list reports the change (a move is a remove and an insert).
        QueueTreeRowsRebuild();
    }
    else if (m_treeRowsPending)
    {
        RebuildTreeRows();
    }
    else
    {
        RunPendingAttachViewGroups();
    }
}

bool TableViewRowFlattener::TryApplyNodeListChange(
    winrt::hstring const& parentKey,
    NodeList const& list,
    winrt::Windows::Foundation::Collections::CollectionChange change,
    uint32_t index)
{
    using winrt::Windows::Foundation::Collections::CollectionChange;

    int32_t parentRow = -1;
    if (!parentKey.empty())
    {
        // By the list, not the key: cheaper than the key index, and a stale list (its row left) matches nothing.
        auto const found = std::find_if(m_nodeRows.begin(), m_nodeRows.end(), [&list](NodeRow const& row) { return row.Children == list; });
        if (found == m_nodeRows.end())
        {
            return true;
        }
        parentRow = static_cast<int32_t>(found - m_nodeRows.begin());
    }
    else if (list != m_viewNodes)
    {
        return true;
    }

    if (change != CollectionChange::ItemInserted && change != CollectionChange::ItemRemoved)
    {
        return false;
    }

    if (parentRow >= 0 && !m_nodeRows[parentRow].IsExpanded)
    {
        // A collapsed node's children are not rows: its chevron may appear or go, or, if it is meant to be expanded
        // and just got its first child, its children are spliced in.
        auto& parent = m_nodeRows[parentRow];
        parent.ChildCount = static_cast<int32_t>(list.Size());
        parent.HasChildren = parent.ChildCount > 0;
        auto const key = parent.NodeKey;
        if (EffectiveExpanded(key, parent.HasChildren, parent.IsContext))
        {
            return TryApplyNodeExpansionSplice(key);
        }
        RaiseRowMetadataChanged();
        return true;
    }

    const int32_t size = static_cast<int32_t>(m_nodeRows.size());
    const int32_t depth = (parentRow < 0) ? 0 : m_nodeRows[parentRow].Depth + 1;
    int32_t row = parentRow + 1;
    for (uint32_t sibling = 0; sibling < index; ++sibling)
    {
        if (row >= size || m_nodeRows[row].Depth != depth)
        {
            return false;
        }
        row = SubtreeEnd(row);
    }

    if (change == CollectionChange::ItemInserted)
    {
        if (row < size && m_nodeRows[row].Depth > depth)
        {
            return false;
        }
        std::vector<winrt::IInspectable> items;
        std::vector<NodeRow> rows;
        EmitNodeRows(list, index, index + 1, parentKey, depth, items, rows);
        if (static_cast<int32_t>(rows.size()) > c_maxSpliceRows)
        {
            return false;
        }
        InsertTreeRows(row, items, std::move(rows));
    }
    else
    {
        if (row >= size || m_nodeRows[row].Depth != depth)
        {
            return false;
        }
        const int32_t end = SubtreeEnd(row);
        if (end - row > c_maxSpliceRows)
        {
            return false;
        }
        m_removedNodeKeys.push_back(m_nodeRows[row].NodeKey);
        RemoveTreeRows(row, end - row);
        QueueForgetRemovedKeys();
    }

    RenumberSiblingRows(parentRow);
    RaiseRowMetadataChanged();
    return true;
}

void TableViewRowFlattener::OnViewItemsChanged(winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    if (!m_treeRowsAttached)
    {
        return;
    }

    if (m_treeRowsStale)
    {
        RebuildTreeRows();
        return;
    }

    if (args.CollectionChange() == winrt::Windows::Foundation::Collections::CollectionChange::ItemChanged)
    {
        OnViewItemChanged(args.Index());
    }
}

void TableViewRowFlattener::OnViewItemChanged(uint32_t index)
{
    if (!m_hierarchy || m_treeRowsInFlight)
    {
        return;
    }

    try
    {
        // By node, not key: a node without a key is keyed by its item, which this change replaced.
        auto const item = m_view.GetAt(index);
        auto const node = m_hierarchy.NodeFromItem(item);
        int32_t row = -1;
        if (node && TryGetIndexForNode(node, row))
        {
            auto& nodeRow = m_nodeRows[row];
            nodeRow.Item = item;
            if (!node.Key())
            {
                auto key = NodeKeyFor(node, item);
                if (key != nodeRow.NodeKey)
                {
                    nodeRow.NodeKey = std::move(key);
                    m_indexByNodeKeyValid = false;
                }
            }
            m_treeRows.SetAt(static_cast<uint32_t>(row), item);
        }
    }
    catch (...)
    {
        RebuildTreeRows();
    }
}

void TableViewRowFlattener::QueueForgetRemovedKeys()
{
    if (m_forgetQueued)
    {
        return;
    }
    auto const queue = winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread();
    std::weak_ptr<TableViewRowFlattener> weakThis = weak_from_this();
    m_forgetQueued = queue && queue.TryEnqueue([weakThis]()
    {
        if (auto const strongThis = weakThis.lock())
        {
            strongThis->m_forgetQueued = false;
            auto const keys = std::exchange(strongThis->m_removedNodeKeys, {});
            for (auto const& key : keys)
            {
                int32_t row = -1;
                if (!strongThis->TryGetIndexForNodeKey(key, row))
                {
                    strongThis->m_nodeExpansion.Forget(key);
                    strongThis->m_filterOverlayCollapsed.erase(key);
                }
            }
        }
    });
    if (!m_forgetQueued)
    {
        m_removedNodeKeys.clear();
    }
}

bool TableViewRowFlattener::IsNodeExpanded(winrt::hstring const& nodeKey) const
{
    int32_t row = -1;
    if (TryGetIndexForNodeKey(nodeKey, row))
    {
        return m_nodeRows[row].IsExpanded;
    }
    return m_nodeExpansion.IsExpanded(nodeKey);
}

void TableViewRowFlattener::SetNodeExpanded(winrt::hstring const& nodeKey, bool isExpanded)
{
    if (!IsNodeKey(nodeKey))
    {
        return;
    }

    int32_t row = -1;
    const bool isVisible = TryGetIndexForNodeKey(nodeKey, row);
    if (isVisible && m_nodeRows[row].IsContext)
    {
        if (isExpanded)
        {
            m_filterOverlayCollapsed.erase(nodeKey);
        }
        else
        {
            m_filterOverlayCollapsed.insert(nodeKey);
        }
        OnNodeExpansionChanged({ { nodeKey }, false, isExpanded });
        return;
    }

    // Through the model, so a per-node toggle and ExpandAll share one path (OnNodeExpansionChanged).
    if (m_nodeExpansion.IsExpanded(nodeKey) != isExpanded)
    {
        m_nodeExpansion.SetExpanded(nodeKey, isExpanded);
    }
    else if (isVisible && m_nodeRows[row].IsExpanded != isExpanded)
    {
        OnNodeExpansionChanged({ { nodeKey }, false, isExpanded });
    }
}

void TableViewRowFlattener::ExpandAllNodes()
{
    m_filterOverlayCollapsed.clear();
    {
        // The model reports the change at once; one rebuild below instead of one there and one here.
        auto const wasInFlight = std::exchange(m_treeRowsInFlight, true);
        auto const done = wil::scope_exit([this, wasInFlight]() noexcept { m_treeRowsInFlight = wasInFlight; });
        m_nodeExpansion.SetAllExpanded(true);
    }
    RebuildTreeRows();
}

void TableViewRowFlattener::CollapseAllNodes()
{
    {
        auto const wasInFlight = std::exchange(m_treeRowsInFlight, true);
        auto const done = wil::scope_exit([this, wasInFlight]() noexcept { m_treeRowsInFlight = wasInFlight; });
        m_nodeExpansion.SetAllExpanded(false);
    }
    // Context rows collapse too; the overlay records the visible ones.
    for (auto const& row : m_nodeRows)
    {
        if (row.IsContext)
        {
            m_filterOverlayCollapsed.insert(row.NodeKey);
        }
    }
    RebuildTreeRows();
}

void TableViewRowFlattener::ExpandSubtree(winrt::hstring const& nodeKey)
{
    int32_t row = -1;
    if (!TryGetIndexForNodeKey(nodeKey, row))
    {
        return;
    }

    {
        // One rebuild for the whole subtree: the toggles below only record intent.
        auto const wasInFlight = std::exchange(m_treeRowsInFlight, true);
        auto const done = wil::scope_exit([this, wasInFlight]() noexcept { m_treeRowsInFlight = wasInFlight; });
        std::vector<ViewNode> stack{ m_nodeRows[row].Node };
        while (!stack.empty())
        {
            auto const node = stack.back();
            stack.pop_back();
            auto const children = node.Children();
            if (!children || children.Size() == 0)
            {
                continue;
            }
            auto const key = NodeKeyFor(node);
            m_filterOverlayCollapsed.erase(key);
            m_nodeExpansion.SetExpanded(key, true);
            for (auto const& child : children)
            {
                if (auto const childNode = child.try_as<ViewNode>())
                {
                    stack.push_back(childNode);
                }
            }
        }
    }
    RebuildTreeRows();
}

bool TableViewRowFlattener::SetItemExpanded(winrt::IInspectable const& item, bool isExpanded)
{
    if (m_kind != Kind::Tree || !m_hierarchy)
    {
        return false;
    }

    auto const node = m_hierarchy.NodeFromItem(item);
    if (!node)
    {
        return false;
    }

    if (isExpanded)
    {
        // Top down, so each ancestor's row exists when it is expanded.
        std::vector<ViewNode> ancestors;
        for (auto parent = node.Parent(); parent; parent = parent.Parent())
        {
            ancestors.push_back(parent);
        }
        for (auto ancestor = ancestors.rbegin(); ancestor != ancestors.rend(); ++ancestor)
        {
            SetNodeExpanded(NodeKeyFor(*ancestor), true);
        }
    }
    SetNodeExpanded(NodeKeyFor(node), isExpanded);
    return true;
}

void TableViewRowFlattener::OnNodeExpansionChanged(ShapingHelpers::RowExpansionModel::Change const& change)
{
    AssertOnUiThread();
    if (!m_treeRowsAttached)
    {
        return;
    }

    if (m_treeRowsInFlight)
    {
        m_treeRowsPending = true;
        return;
    }

    if (change.AffectsAllKeys || change.Keys.size() != 1)
    {
        RebuildTreeRows();
        return;
    }

    auto const keepAlive = shared_from_this();
    bool applied = false;
    {
        m_treeRowsInFlight = true;
        m_treeRowsPending = false;
        auto const done = wil::scope_exit([this]() noexcept { m_treeRowsInFlight = false; });
        try
        {
            applied = TryApplyNodeExpansionSplice(change.Keys.front());
        }
        catch (...)
        {
            applied = false;
        }
    }

    if (!applied || m_treeRowsPending)
    {
        RebuildTreeRows();
    }
    else
    {
        RunPendingAttachViewGroups();
    }
}

// Brings one row's shown children in line with its effective expansion; false (a rebuild then) for a long run.
bool TableViewRowFlattener::TryApplyNodeExpansionSplice(winrt::hstring const& nodeKey)
{
    int32_t row = -1;
    if (!TryGetIndexForNodeKey(nodeKey, row))
    {
        // Not shown (under a collapsed ancestor): the intent applies when it is.
        return true;
    }

    auto& node = m_nodeRows[row];
    const bool isExpanded = EffectiveExpanded(nodeKey, node.HasChildren, node.IsContext);
    if (isExpanded == node.IsExpanded)
    {
        return true;
    }

    node.IsExpanded = isExpanded;
    if (isExpanded)
    {
        auto const children = node.Children;
        const int32_t depth = node.Depth + 1;
        std::vector<winrt::IInspectable> items;
        std::vector<NodeRow> rows;
        EmitNodeRows(children, 0, children.Size(), nodeKey, depth, items, rows);
        if (static_cast<int32_t>(rows.size()) > c_maxSpliceRows)
        {
            return false;
        }
        InsertTreeRows(row + 1, items, std::move(rows));
    }
    else
    {
        const int32_t end = SubtreeEnd(row);
        if (end - row - 1 > c_maxSpliceRows)
        {
            return false;
        }
        RemoveTreeRows(row + 1, end - row - 1);
    }

    RaiseRowMetadataChanged();
    return true;
}
