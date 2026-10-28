// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ShapedItemsSource6.h"
#include "VisibleRowList6.h"
#include "ShapedGroup.h"
#include "GroupedEntry.h"
#include "ShapingRevoke.h"

#include <algorithm>
#include <cwchar>

using Node = v6::Node;

namespace
{
    // Beyond this many rows moved by one live restore, one Reset is cheaper for the consumer.
    constexpr size_t c_maxLiveResplices = 256;

    constexpr std::wstring_view c_rowIdentityPrefix{ L"p:" };
    constexpr std::wstring_view c_groupIdentityPrefix{ L"group:" };

    [[noreturn]] void ThrowCallerContract(winrt::hstring const& message)
    {
        throw ShapingHelpers::CallerError(winrt::hresult_invalid_argument(message), ShapingHelpers::c_callerDataFault);
    }

    void const* ObjectIdentity(winrt::IInspectable const& item)
    {
        return item ? winrt::get_abi(item.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
    }

    std::vector<winrt::IInspectable> ToVector(winrt::Microsoft::UI::Xaml::Interop::IBindableVector const& items)
    {
        std::vector<winrt::IInspectable> result;
        auto const size = items.Size();
        result.reserve(size);
        for (uint32_t i = 0; i < size; ++i)
        {
            result.push_back(items.GetAt(i));
        }
        return result;
    }

    bool StartsWith(std::wstring_view value, std::wstring_view prefix)
    {
        return value.size() > prefix.size() && value.substr(0, prefix.size()) == prefix;
    }
}

void v6::ObservedCollection::Revoke() noexcept
{
    ShapingHelpers::SafeRevoke(CollectionChangedRevoker);
    ShapingHelpers::SafeRevoke(VectorChangedRevoker);
    ShapingHelpers::SafeRevoke(BindableVectorChangedRevoker);
}

ShapedItemsSource6::ShapedItemsSource6(winrt::IInspectable const& items) :
    m_source(items),
    m_ownerQueue(winrt::Microsoft::UI::Dispatching::DispatcherQueue::GetForCurrentThread()),
    m_ownerThreadId(::GetCurrentThreadId())
{
    m_root.Visible = 0;
    m_root.Open = true;
}

ShapedItemsSource6::~ShapedItemsSource6()
{
    m_topCollection.Revoke();
    for (auto& node : m_pool)
    {
        if (node.Children)
        {
            node.Children->Revoke();
        }
    }
    ClearLiveSubscriptions();
}

void ShapedItemsSource6::Start()
{
    m_liveShaping->SetChangeHandler(
        [weakThis = weak_from_this(), queue = m_ownerQueue, ownerThreadId = m_ownerThreadId, dropped = m_liveShapingChangeDropped](winrt::IInspectable const& item, winrt::hstring const&)
        {
            if (queue && !queue.HasThreadAccess())
            {
                const bool posted = queue.TryEnqueue([weakThis, item]()
                    {
                        if (auto const strongThis = weakThis.lock())
                        {
                            strongThis->OnLiveItemChanged(item);
                        }
                    });
                if (!posted)
                {
                    dropped->store(true);
                }
                return;
            }

            if (!queue && ::GetCurrentThreadId() != ownerThreadId)
            {
                dropped->store(true);
                return;
            }

            if (auto const strongThis = weakThis.lock())
            {
                strongThis->OnLiveItemChanged(item);
            }
        });

    m_list = winrt::make_self<VisibleRowList6>(weak_from_this());
    m_topCollection.Accessor = ShapingHelpers::CollectionAccessor{ m_source, ShapingHelpers::CollectionOwner::Caller };
    Rebuild(true /* reload */, false /* clearOverlay */, false /* raise */);
    Subscribe(nullptr);
}

// --- Verbs -------------------------------------------------------------------------------------

void ShapedItemsSource6::SetFilter(ShapingHelpers::Predicate const& predicate)
{
    m_pipeline.SetFilter(predicate);
    ApplyVerb(true /* clearOverlay */);
}

void ShapedItemsSource6::ClearFilter()
{
    m_pipeline.ClearFilter();
    ApplyVerb(true /* clearOverlay */);
}

void ShapedItemsSource6::SetSort(
    winrt::hstring const& previousAxisToken,
    winrt::hstring const& axisToken,
    ShapingHelpers::KeySelector const& key,
    winrt::Windows::Foundation::IUnknown const& keyIdentity,
    winrt::hstring const& sortMemberPath,
    winrt::SortDirection direction)
{
    m_pipeline.SetSort(previousAxisToken, axisToken, key, keyIdentity, sortMemberPath, direction);
    ApplyVerb(false);
}

void ShapedItemsSource6::ClearSorts()
{
    m_pipeline.ClearSorts();
    ApplyVerb(false);
}

void ShapedItemsSource6::ClearSort(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSort(axisToken);
    ApplyVerb(false);
}

void ShapedItemsSource6::ClearSortsExcept(winrt::hstring const& axisToken)
{
    m_pipeline.ClearSortsExcept(axisToken);
    ApplyVerb(false);
}

void ShapedItemsSource6::SetGroup(ShapingHelpers::KeySelector const& key, IdentitySelector const& groupIdentity)
{
    m_groupKey = key;
    m_groupIdentity = groupIdentity;
    m_pipeline.MarkGroupVerb(key);
    ApplyVerb(false);
}

void ShapedItemsSource6::ClearGroup()
{
    m_groupKey = nullptr;
    m_groupIdentity = nullptr;
    m_pipeline.ClearGroupVerb();
    ApplyVerb(false);
}

void ShapedItemsSource6::SetChildren(ChildrenSelector const& selector, HasChildrenSelector const& hasChildren)
{
    auto previousSelector = m_childrenSelector;
    auto previousHasChildren = m_hasChildren;

    ReleaseAllChildren();
    m_childrenSelector = selector;
    m_hasChildren = hasChildren;
    try
    {
        ApplyVerb(false);
    }
    catch (...)
    {
        // Back to the previous hierarchy; the projection is rebuilt from it if that still works.
        ReleaseAllChildren();
        m_childrenSelector = std::move(previousSelector);
        m_hasChildren = std::move(previousHasChildren);
        try
        {
            Rebuild(false, false, true);
        }
        catch (...)
        {
        }
        throw;
    }
}

void ShapedItemsSource6::ClearChildren()
{
    ReleaseAllChildren();
    m_childrenSelector = nullptr;
    m_hasChildren = nullptr;
    ApplyVerb(false);
}

void ShapedItemsSource6::ApplyVerb(bool clearOverlay)
{
    Rebuild(false, clearOverlay, true);

    // Every row was just re-evaluated, so nothing live is pending.
    m_liveShapingDirty = false;
    m_liveFullRestore = false;
    m_liveDirtyItems.clear();
    m_liveShapingChangeDropped->store(false);

    if (m_projectionRebuilt)
    {
        m_projectionRebuilt();
    }
    RaiseShapingChanged(false /* reorderOnly */);
}

void ShapedItemsSource6::Rebuild(bool reload, bool clearOverlay, bool raise)
{
    InvalidateCursor();
    try
    {
        if (reload || m_cacheInvalid)
        {
            ReloadTop();
        }
        RefreshAxes();
        // Materialization evaluates as it goes, but EvaluateAll below covers every node anyway.
        m_deferEvaluate = true;
        try
        {
            if (NeedsFullMaterialization())
            {
                for (auto const top : m_top)
                {
                    MaterializeDeep(top);
                }
            }
        }
        catch (...)
        {
            m_deferEvaluate = false;
            throw;
        }
        m_deferEvaluate = false;
        EvaluateAll(clearOverlay);
        Shape();
    }
    catch (...)
    {
        ClearProjection();
        if (raise)
        {
            RaiseReset();
        }
        throw;
    }
    if (raise)
    {
        RaiseReset();
    }
}

// --- Nodes -------------------------------------------------------------------------------------

Node* ShapedItemsSource6::AllocateNode()
{
    if (!m_free.empty())
    {
        auto const node = m_free.back();
        m_free.pop_back();
        return node;
    }
    return &m_pool.emplace_back();
}

void ShapedItemsSource6::FreeNode(Node* node)
{
    if (node->Children)
    {
        node->Children->Revoke();
    }
    if (m_cursorNode == node)
    {
        InvalidateCursor();
    }
    *node = Node{};
    m_free.push_back(node);
}

Node* ShapedItemsSource6::CreateNode(winrt::IInspectable const& item, Node* dataParent)
{
    if (!item)
    {
        ThrowCallerContract(L"TableViewSource6: items cannot contain null.");
    }
    auto const id = ObjectIdentity(item);
    if (m_byItem.contains(id))
    {
        ThrowCallerContract(L"TableViewSource6: an item appears more than once in the hierarchy.");
    }

    auto const node = AllocateNode();
    node->Item = item;
    node->Id = id;
    node->DataParent = dataParent;
    node->Depth = dataParent ? dataParent->Depth + 1 : 0;
    node->Expanded = m_nodeDefaultExpanded;
    m_byItem.emplace(id, node);
    if (m_liveShapingEnabled)
    {
        m_liveShaping->Subscribe(item);
    }
    return node;
}

void ShapedItemsSource6::ReleaseSubtree(Node* node, bool notify)
{
    std::vector<Node*> stack{ node };
    while (!stack.empty())
    {
        auto const current = stack.back();
        stack.pop_back();
        stack.insert(stack.end(), current->All.begin(), current->All.end());

        m_byItem.erase(current->Id);
        if (m_liveShapingEnabled)
        {
            m_liveShaping->Unsubscribe(current->Item);
        }
        if (notify && m_rowsReleasedHook)
        {
            m_rowsReleasedHook(current->Item, {});
        }
        FreeNode(current);
    }
}

void ShapedItemsSource6::ReleaseHeader(Node* header)
{
    if (header->Item)
    {
        m_byEntry.erase(winrt::get_abi(header->Item));
    }
    FreeNode(header);
}

void ShapedItemsSource6::ReleaseAllChildren()
{
    InvalidateCursor();
    for (auto const top : m_top)
    {
        for (auto const child : top->All)
        {
            ReleaseSubtree(child, true);
        }
        top->All.clear();
        top->Kids.clear();
        top->KidVisible.Assign(0, [](size_t) { return 0; });
        if (top->Children)
        {
            top->Children->Revoke();
            top->Children.reset();
        }
        top->Materialized = false;
        top->HasChildrenHint = false;
    }
}

// --- Evaluation and materialization ------------------------------------------------------------

void ShapedItemsSource6::Evaluate(Node* node)
{
    auto& faults = m_pipeline.SwallowedFaultsTally();
    auto const& item = node->Item;

    node->SelfMatch = m_pipeline.PassesFilter(item);

    node->SortKeys.clear();
    node->SortKeys.reserve(m_axes.size());
    for (auto const& axis : m_axes)
    {
        winrt::IInspectable key{ nullptr };
        try
        {
            key = axis.Key(item);
        }
        catch (...)
        {
            faults.Note();
        }
        node->SortKeys.push_back(std::move(key));
    }

    node->GroupValue = nullptr;
    node->GroupKey = {};
    node->GroupError = nullptr;
    if (m_groupKey && !node->DataParent)
    {
        try
        {
            node->GroupValue = m_groupKey(item);
        }
        catch (...)
        {
            faults.Note();
        }

        winrt::hstring groupKey;
        wchar_t const* reason{ nullptr };
        if (RowIdentity::TryGetGroupIdentity(node->GroupValue, m_groupIdentity, groupKey, reason))
        {
            node->GroupKey = std::move(groupKey);
        }
        else
        {
            node->GroupError = reason ? reason : L"unusable group key";
        }
    }
}

void ShapedItemsSource6::EvaluateHint(Node* node)
{
    node->HasChildrenHint = false;
    if (!m_hasChildren || node->Materialized)
    {
        return;
    }
    try
    {
        node->HasChildrenHint = m_hasChildren(node->Item);
    }
    catch (...)
    {
        m_pipeline.SwallowedFaultsTally().Note();
    }
}

void ShapedItemsSource6::EvaluateAll(bool clearOverlay)
{
    ForEachDataNode([this, clearOverlay](Node* node)
    {
        Evaluate(node);
        EvaluateHint(node);
        if (clearOverlay)
        {
            node->OverlayCollapsed = false;
        }
    });
}

bool ShapedItemsSource6::NeedsFullMaterialization() const noexcept
{
    // Without a hint, whether a row has children is only known by asking for them. A filter
    // needs every descendant to decide which ancestors stay.
    return m_childrenSelector && (!m_hasChildren || m_pipeline.HasFilter() || m_nodeDefaultExpanded);
}

void ShapedItemsSource6::MaterializeOne(Node* node)
{
    if (node->Materialized || !m_childrenSelector)
    {
        return;
    }
    node->Materialized = true;
    node->HasChildrenHint = false;

    winrt::IInspectable children{ nullptr };
    try
    {
        children = m_childrenSelector(node->Item);
    }
    catch (...)
    {
        ShapingHelpers::RethrowAsCallerError(ShapingHelpers::c_callerDataFault);
    }
    if (!children)
    {
        return;
    }

    node->Children = std::make_unique<v6::ObservedCollection>();
    node->Children->Accessor = ShapingHelpers::CollectionAccessor{ children, ShapingHelpers::CollectionOwner::Caller };
    auto const items = node->Children->Accessor.Enumerate(true /* throwIfUnsupported */);
    node->All.reserve(items.size());
    for (size_t i = 0; i < items.size(); ++i)
    {
        // Pushed as it is made, so a throw leaves every made node reachable for release.
        auto const child = CreateNode(items[i], node);
        child->SrcIndex = static_cast<int32_t>(i);
        node->All.push_back(child);
        if (!m_deferEvaluate)
        {
            Evaluate(child);
            EvaluateHint(child);
        }
    }
    Subscribe(node);
}

void ShapedItemsSource6::MaterializeDeep(Node* node)
{
    std::vector<Node*> stack{ node };
    while (!stack.empty())
    {
        auto const current = stack.back();
        stack.pop_back();
        MaterializeOne(current);
        stack.insert(stack.end(), current->All.begin(), current->All.end());
    }
}

void ShapedItemsSource6::ReloadTop()
{
    std::vector<winrt::IInspectable> items;
    try
    {
        items = m_topCollection.Accessor.Enumerate(true /* throwIfUnsupported */);
    }
    catch (...)
    {
        m_cacheInvalid = true;
        throw;
    }

    ClearProjection();
    for (auto const top : m_top)
    {
        ReleaseSubtree(top, false);
    }
    m_top.clear();

    if (m_rowsReleasedHook)
    {
        m_rowsReleasedHook(nullptr, items);
    }

    try
    {
        m_top.reserve(items.size());
        for (size_t i = 0; i < items.size(); ++i)
        {
            auto const node = CreateNode(items[i], nullptr);
            node->SrcIndex = static_cast<int32_t>(i);
            m_top.push_back(node);
        }
    }
    catch (...)
    {
        for (auto const top : m_top)
        {
            ReleaseSubtree(top, false);
        }
        m_top.clear();
        m_cacheInvalid = true;
        throw;
    }
    m_cacheInvalid = false;
}

// --- Shape -------------------------------------------------------------------------------------

void ShapedItemsSource6::RefreshAxes()
{
    m_axes = m_pipeline.ActiveSortAxes(-1, -1);
    m_allAxes.clear();
    m_preAxes.clear();
    m_headerKidAxes.clear();
    for (size_t k = 0; k < m_axes.size(); ++k)
    {
        m_allAxes.push_back(k);
    }
    if (m_groupKey)
    {
        // Sorts declared before GroupBy order the groups (by first appearance); later ones order
        // the rows within each group, falling back to the earlier ones.
        auto const groupOrder = m_pipeline.GroupOrder();
        for (size_t k = 0; k < m_axes.size(); ++k)
        {
            if (m_axes[k].Order < groupOrder)
            {
                m_preAxes.push_back(k);
            }
            else
            {
                m_headerKidAxes.push_back(k);
            }
        }
        m_headerKidAxes.insert(m_headerKidAxes.end(), m_preAxes.begin(), m_preAxes.end());
    }
}

int ShapedItemsSource6::CompareNodes(Node const* a, Node const* b, std::vector<size_t> const& axes) const
{
    for (auto const k : axes)
    {
        auto const& keyA = k < a->SortKeys.size() ? a->SortKeys[k] : nullptr;
        auto const& keyB = k < b->SortKeys.size() ? b->SortKeys[k] : nullptr;
        auto const c = ShapingHelpers::ValueComparer::Compare(keyA, keyB);
        if (c != 0)
        {
            return m_axes[k].Direction == winrt::SortDirection::Descending ? -c : c;
        }
    }
    // Siblings share one owning collection, so source order breaks every tie.
    return a->SrcIndex < b->SrcIndex ? -1 : (a->SrcIndex > b->SrcIndex ? 1 : 0);
}

std::vector<size_t> const& ShapedItemsSource6::AxesFor(Node const* parent) const
{
    return parent->IsHeader ? m_headerKidAxes : m_allAxes;
}

void ShapedItemsSource6::SortKids(Node* parent)
{
    auto& kids = parent->Kids;
    if (kids.size() > 1 && !m_axes.empty())
    {
        auto const& axes = AxesFor(parent);
        std::sort(kids.begin(), kids.end(), [this, &axes](Node const* a, Node const* b) { return CompareNodes(a, b, axes) < 0; });
    }
    for (int32_t i = 0; i < static_cast<int32_t>(kids.size()); ++i)
    {
        kids[i]->Parent = parent;
        kids[i]->IndexInParent = i;
    }
}

bool ShapedItemsSource6::IsGroupExpanded(winrt::hstring const& groupKey) const
{
    return m_groupDefaultExpanded != m_groupToggled.contains(groupKey);
}

bool ShapedItemsSource6::ComputeOpen(Node const* node) const
{
    if (node->Kids.empty())
    {
        return false;
    }
    if (node->IsHeader)
    {
        return IsGroupExpanded(node->GroupKey);
    }
    if (IsContextRow(node))
    {
        // Open because of the filter, not the user; collapsing it is filter-scoped.
        return !node->OverlayCollapsed;
    }
    return node->Expanded;
}

bool ShapedItemsSource6::IsContextRow(Node const* node) const noexcept
{
    return !node->IsHeader && m_pipeline.HasFilter() && !node->SelfMatch;
}

void ShapedItemsSource6::Recount(Node* node)
{
    node->KidVisible.Assign(node->Kids.size(), [node](size_t i) { return node->Kids[i]->Visible; });
    if (node == &m_root)
    {
        node->Visible = node->KidVisible.Total();
        return;
    }
    node->Open = ComputeOpen(node);
    node->Visible = 1 + (node->Open ? node->KidVisible.Total() : 0);
}

void ShapedItemsSource6::ShapeNode(Node* node)
{
    node->Kids.clear();
    for (auto const child : node->All)
    {
        child->IndexInParent = -1;
        if (child->Included)
        {
            node->Kids.push_back(child);
        }
    }
    SortKids(node);
    node->Included = !m_pipeline.HasFilter() || node->SelfMatch || !node->Kids.empty();
    Recount(node);
}

void ShapedItemsSource6::ShapeSubtree(Node* node)
{
    std::vector<Node*> order;
    std::vector<Node*> stack{ node };
    while (!stack.empty())
    {
        auto const current = stack.back();
        stack.pop_back();
        order.push_back(current);
        stack.insert(stack.end(), current->All.begin(), current->All.end());
    }
    // Children before their parent.
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
        ShapeNode(*it);
    }
}

void ShapedItemsSource6::Shape()
{
    InvalidateCursor();
    for (auto const top : m_top)
    {
        ShapeSubtree(top);
    }
    ShapeTop();
}

void ShapedItemsSource6::ShapeTop()
{
    for (auto const top : m_top)
    {
        top->IndexInParent = -1;
    }
    m_root.Kids.clear();

    if (!m_groupKey)
    {
        for (auto const& [key, header] : m_headers)
        {
            ReleaseHeader(header);
        }
        m_headers.clear();
        m_groupToggled.clear();

        for (auto const top : m_top)
        {
            if (top->Included)
            {
                m_root.Kids.push_back(top);
            }
        }
        SortKids(&m_root);
        Recount(&m_root);
        return;
    }

    std::vector<Node*> units;
    for (auto const top : m_top)
    {
        if (top->Included)
        {
            units.push_back(top);
        }
    }
    if (units.size() > 1 && !m_preAxes.empty())
    {
        std::sort(units.begin(), units.end(), [this](Node const* a, Node const* b) { return CompareNodes(a, b, m_preAxes) < 0; });
    }

    auto previous = std::move(m_headers);
    m_headers.clear();
    std::vector<Node*> order;
    try
    {
        for (auto const unit : units)
        {
            if (unit->GroupError)
            {
                ThrowCallerContract(L"TableViewSource6: " + winrt::hstring{ unit->GroupError } + L".");
            }
            auto it = m_headers.find(unit->GroupKey);
            if (it == m_headers.end())
            {
                Node* header{ nullptr };
                if (auto const old = previous.find(unit->GroupKey); old != previous.end())
                {
                    header = old->second;
                    previous.erase(old);
                    header->Group->Key(unit->GroupValue);
                }
                else
                {
                    header = AllocateNode();
                    header->IsHeader = true;
                    header->GroupKey = unit->GroupKey;
                    header->Group = winrt::make_self<ShapedGroup>(unit->GroupValue, unit->GroupKey);
                }
                header->GroupValue = unit->GroupValue;
                header->Kids.clear();
                it = m_headers.emplace(unit->GroupKey, header).first;
                order.push_back(header);
            }
            else if (!m_groupIdentity && !RowIdentity::GroupKeysEqual(it->second->GroupValue, unit->GroupValue))
            {
                ThrowCallerContract(L"TableViewSource6: duplicate group identity '" + unit->GroupKey + L"'.");
            }
            it->second->Kids.push_back(unit);
        }
    }
    catch (...)
    {
        for (auto const& [key, header] : previous)
        {
            ReleaseHeader(header);
        }
        throw;
    }

    for (auto const& [key, header] : previous)
    {
        ReleaseHeader(header);
    }
    std::erase_if(m_groupToggled, [this](winrt::hstring const& key) { return !m_headers.contains(key); });

    for (auto const header : order)
    {
        SortKids(header);
        Recount(header);
        RefreshHeaderEntry(header, false);
    }
    m_root.Kids = std::move(order);
    SortKidsInPlace(&m_root);
    Recount(&m_root);
}

void ShapedItemsSource6::SortKidsInPlace(Node* parent)
{
    for (int32_t i = 0; i < static_cast<int32_t>(parent->Kids.size()); ++i)
    {
        parent->Kids[i]->Parent = parent;
        parent->Kids[i]->IndexInParent = i;
    }
}

void ShapedItemsSource6::RecountAll()
{
    InvalidateCursor();
    std::vector<Node*> order;
    ForEachDataNode([&order](Node* node) { order.push_back(node); });
    for (auto it = order.rbegin(); it != order.rend(); ++it)
    {
        Recount(*it);
    }
    for (auto const& [key, header] : m_headers)
    {
        Recount(header);
        RefreshHeaderEntry(header, false);
    }
    Recount(&m_root);
}

void ShapedItemsSource6::ClearProjection()
{
    InvalidateCursor();
    for (auto const& [key, header] : m_headers)
    {
        ReleaseHeader(header);
    }
    m_headers.clear();
    for (auto const top : m_top)
    {
        top->IndexInParent = -1;
    }
    m_root.Kids.clear();
    Recount(&m_root);
}

void ShapedItemsSource6::RefreshHeaderEntry(Node* header, bool raise)
{
    auto const count = static_cast<int32_t>(header->Kids.size());
    auto const expanded = header->Open;
    // A header's object carries its count and expansion, so it is re-minted when either moves.
    if (!header->Item || header->EntryCount != count || header->EntryExpanded != expanded)
    {
        if (header->Item)
        {
            m_byEntry.erase(winrt::get_abi(header->Item));
        }
        header->Item = winrt::make<GroupedEntry>(header->Group.as<winrt::IInspectable>(), count, expanded);
        header->EntryCount = count;
        header->EntryExpanded = expanded;
        m_byEntry[winrt::get_abi(header->Item)] = header;
        if (raise)
        {
            if (auto const index = IndexOf(header); index >= 0)
            {
                RaiseReplaced(index);
            }
        }
    }

    std::vector<winrt::IInspectable> items;
    items.reserve(header->Kids.size());
    for (auto const kid : header->Kids)
    {
        items.push_back(kid->Item);
    }
    auto const current = header->Group->GroupItems();
    bool same = current && current.Size() == items.size();
    for (uint32_t i = 0; same && i < items.size(); ++i)
    {
        same = winrt::get_abi(current.GetAt(i)) == winrt::get_abi(items[i]);
    }
    if (!same)
    {
        header->Group->SetItems(items);
    }
    // Over a hierarchy the header counts its roots.
    header->Group->GroupChildCount(m_childrenSelector ? count : -1);
}

// --- Incremental structure ---------------------------------------------------------------------

void ShapedItemsSource6::PropagateFrom(Node* node, int32_t delta)
{
    while (delta != 0 && node != &m_root)
    {
        if (node->IndexInParent < 0)
        {
            return;
        }
        auto const parent = node->Parent;
        parent->KidVisible.Add(node->IndexInParent, delta);
        if (parent == &m_root)
        {
            m_root.Visible += delta;
            return;
        }
        if (!parent->Open)
        {
            return;
        }
        parent->Visible += delta;
        node = parent;
    }
}

void ShapedItemsSource6::AfterKidsChanged(Node* parent)
{
    parent->KidVisible.Assign(parent->Kids.size(), [parent](size_t i) { return parent->Kids[i]->Visible; });
    if (parent == &m_root)
    {
        m_root.Visible = m_root.KidVisible.Total();
        return;
    }
    // The first or last kid can open or close the parent, so recompute it rather than add.
    auto const old = parent->Visible;
    parent->Open = ComputeOpen(parent);
    parent->Visible = 1 + (parent->Open ? parent->KidVisible.Total() : 0);
    PropagateFrom(parent, parent->Visible - old);
}

void ShapedItemsSource6::AttachCore(Node* node, Node* parent)
{
    InvalidateCursor();
    auto& kids = parent->Kids;
    auto const& axes = AxesFor(parent);
    auto const it = std::upper_bound(kids.begin(), kids.end(), node,
        [this, &axes](Node const* a, Node const* b) { return CompareNodes(a, b, axes) < 0; });
    auto const pos = static_cast<int32_t>(it - kids.begin());
    kids.insert(it, node);
    node->Parent = parent;
    for (auto i = pos; i < static_cast<int32_t>(kids.size()); ++i)
    {
        kids[i]->IndexInParent = i;
    }
    AfterKidsChanged(parent);
}

void ShapedItemsSource6::DetachCore(Node* node)
{
    InvalidateCursor();
    auto const parent = node->Parent;
    auto& kids = parent->Kids;
    auto const pos = node->IndexInParent;
    kids.erase(kids.begin() + pos);
    node->IndexInParent = -1;
    for (auto i = pos; i < static_cast<int32_t>(kids.size()); ++i)
    {
        kids[i]->IndexInParent = i;
    }
    AfterKidsChanged(parent);
}

Node* ShapedItemsSource6::PresentationParentFor(Node const* node) const
{
    if (node->DataParent)
    {
        return node->DataParent;
    }
    if (!m_groupKey)
    {
        return const_cast<Node*>(&m_root);
    }
    if (node->GroupError)
    {
        return nullptr;
    }
    auto const it = m_headers.find(node->GroupKey);
    return it == m_headers.end() ? nullptr : it->second;
}

bool ShapedItemsSource6::Attach(Node* node, bool refreshHeader)
{
    auto const parent = PresentationParentFor(node);
    if (!parent)
    {
        return false;
    }
    if (parent->IsHeader && !m_groupIdentity && !RowIdentity::GroupKeysEqual(parent->GroupValue, node->GroupValue))
    {
        return false;
    }

    AttachCore(node, parent);
    if (auto const index = IndexOf(node); index >= 0)
    {
        RaiseAdded(index, node->Visible);
    }

    if (parent->IsHeader)
    {
        if (refreshHeader)
        {
            RefreshHeaderEntry(parent, true);
        }
    }
    else if (parent != &m_root && !parent->Included)
    {
        // A context row comes back with its first kept descendant.
        parent->Included = true;
        return Attach(parent, refreshHeader);
    }
    return true;
}

void ShapedItemsSource6::Detach(Node* node, bool prune)
{
    auto const index = IndexOf(node);
    auto const count = node->Visible;
    auto const parent = node->Parent;
    DetachCore(node);
    if (index >= 0)
    {
        RaiseRemoved(index, count);
    }
    if (!prune)
    {
        return;
    }

    if (parent->IsHeader)
    {
        if (parent->Kids.empty())
        {
            RemoveHeader(parent);
        }
        else
        {
            RefreshHeaderEntry(parent, true);
        }
    }
    else if (parent != &m_root && parent->Kids.empty() && IsContextRow(parent) && parent->Included)
    {
        // A context row goes with its last kept descendant.
        parent->Included = false;
        if (parent->IndexInParent >= 0)
        {
            Detach(parent, true);
        }
    }
}

void ShapedItemsSource6::RemoveHeader(Node* header)
{
    auto const index = IndexOf(header);
    auto const count = header->Visible;
    DetachCore(header);
    if (index >= 0)
    {
        RaiseRemoved(index, count);
    }
    m_groupToggled.erase(header->GroupKey);
    m_headers.erase(header->GroupKey);
    ReleaseHeader(header);
}

// --- Projection --------------------------------------------------------------------------------

winrt::IInspectable ShapedItemsSource6::Rows() const
{
    return m_list.as<winrt::IInspectable>();
}

Node* ShapedItemsSource6::NextVisible(Node* node) const
{
    if (node->Open && !node->Kids.empty())
    {
        return node->Kids.front();
    }
    while (node != &m_root)
    {
        auto const parent = node->Parent;
        auto const next = node->IndexInParent + 1;
        if (next < static_cast<int32_t>(parent->Kids.size()))
        {
            return parent->Kids[next];
        }
        node = parent;
    }
    return nullptr;
}

Node* ShapedItemsSource6::NodeAt(int32_t index) const
{
    if (index < 0 || index >= m_root.Visible)
    {
        throw winrt::hresult_out_of_bounds();
    }

    if (m_cursorNode)
    {
        if (index == m_cursorIndex)
        {
            return m_cursorNode;
        }
        if (index == m_cursorIndex + 1)
        {
            if (auto const next = NextVisible(m_cursorNode))
            {
                m_cursorNode = next;
                m_cursorIndex = index;
                return next;
            }
        }
    }

    // Each level: find the kid whose range holds the offset, then step past that kid's own row.
    auto parent = const_cast<Node*>(&m_root);
    int32_t offset = index;
    while (true)
    {
        auto const k = parent->KidVisible.Find(offset);
        auto const kid = parent->Kids[k];
        if (offset == 0)
        {
            m_cursorNode = kid;
            m_cursorIndex = index;
            return kid;
        }
        offset -= 1;
        parent = kid;
    }
}

int32_t ShapedItemsSource6::IndexOf(Node const* node) const
{
    if (!node || node == &m_root)
    {
        return -1;
    }
    int32_t index = 0;
    while (node != &m_root)
    {
        if (node->IndexInParent < 0)
        {
            return -1;
        }
        auto const parent = node->Parent;
        if (parent != &m_root)
        {
            if (!parent->Open)
            {
                return -1;
            }
            index += 1;
        }
        index += parent->KidVisible.Prefix(node->IndexInParent);
        node = parent;
    }
    return index;
}

bool ShapedItemsSource6::IsAttached(Node const* node) const
{
    while (node && node != &m_root)
    {
        if (node->IndexInParent < 0)
        {
            return false;
        }
        node = node->Parent;
    }
    return node == &m_root;
}

Node* ShapedItemsSource6::NodeOfPresented(winrt::IInspectable const& value) const
{
    if (!value)
    {
        return nullptr;
    }
    if (auto const entry = m_byEntry.find(winrt::get_abi(value)); entry != m_byEntry.end())
    {
        return entry->second;
    }
    auto const it = m_byItem.find(ObjectIdentity(value));
    return it == m_byItem.end() ? nullptr : it->second;
}

bool ShapedItemsSource6::ContainsItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return false;
    }
    auto const it = m_byItem.find(ObjectIdentity(item));
    return it != m_byItem.end() && IsAttached(it->second);
}

winrt::hstring ShapedItemsSource6::IdentityOf(Node const* node) const
{
    if (node->IsHeader)
    {
        return winrt::hstring{ c_groupIdentityPrefix } + node->GroupKey;
    }
    wchar_t buffer[2 + 16 + 1]{};
    swprintf_s(buffer, L"%llx", static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(node->Id)));
    return winrt::hstring{ c_rowIdentityPrefix } + buffer;
}

Node* ShapedItemsSource6::NodeOfIdentity(winrt::hstring const& identity) const
{
    std::wstring_view const view{ identity };
    if (StartsWith(view, c_rowIdentityPrefix))
    {
        std::wstring const digits{ view.substr(c_rowIdentityPrefix.size()) };
        wchar_t* end{ nullptr };
        auto const address = std::wcstoull(digits.c_str(), &end, 16);
        if (end && *end == L'\0')
        {
            auto const it = m_byItem.find(reinterpret_cast<void const*>(static_cast<uintptr_t>(address)));
            if (it != m_byItem.end())
            {
                return it->second;
            }
        }
    }
    if (m_groupKey && StartsWith(view, c_groupIdentityPrefix))
    {
        auto const it = m_headers.find(winrt::hstring{ view.substr(c_groupIdentityPrefix.size()) });
        if (it != m_headers.end())
        {
            return it->second;
        }
    }
    return nullptr;
}

bool ShapedItemsSource6::IsExpandable(Node const* node) const noexcept
{
    if (!node->Kids.empty())
    {
        return true;
    }
    return !node->IsHeader && m_childrenSelector && !node->Materialized && node->HasChildrenHint;
}

int32_t ShapedItemsSource6::ChildCountOf(Node const* node) const noexcept
{
    if (node->IsHeader || node->Materialized || !m_childrenSelector)
    {
        return static_cast<int32_t>(node->Kids.size());
    }
    return node->HasChildrenHint ? 1 : 0;
}

// --- Expansion ---------------------------------------------------------------------------------

bool ShapedItemsSource6::SetExpanded(Node* node, bool expanded)
{
    if (!node || node == &m_root || !IsExpandable(node) || node->Open == expanded || IndexOf(node) < 0)
    {
        return false;
    }

    auto const old = node->Visible;
    if (node->IsHeader)
    {
        if (expanded == m_groupDefaultExpanded)
        {
            m_groupToggled.erase(node->GroupKey);
        }
        else
        {
            m_groupToggled.insert(node->GroupKey);
        }
        Recount(node);
    }
    else
    {
        if (IsContextRow(node))
        {
            node->OverlayCollapsed = !expanded;
        }
        else
        {
            node->Expanded = expanded;
        }

        if (expanded && !node->Materialized && m_childrenSelector)
        {
            MaterializeOne(node);
            for (auto const child : node->All)
            {
                ShapeNode(child);
            }
            ShapeNode(node);
        }
        else
        {
            Recount(node);
        }
    }

    auto const delta = node->Visible - old;
    PropagateFrom(node, delta);
    InvalidateCursor();
    auto const index = IndexOf(node);
    if (delta > 0)
    {
        RaiseAdded(index + 1, delta);
    }
    else if (delta < 0)
    {
        RaiseRemoved(index + 1, -delta);
    }
    if (node->IsHeader)
    {
        RefreshHeaderEntry(node, true);
    }
    NotifyRowsMetadataChanged();
    return delta != 0;
}

void ShapedItemsSource6::SetAllGroupsExpanded(bool expanded)
{
    m_groupDefaultExpanded = expanded;
    m_groupToggled.clear();
    InvalidateCursor();
    for (auto const header : m_root.Kids)
    {
        if (header->IsHeader)
        {
            Recount(header);
            RefreshHeaderEntry(header, false);
        }
    }
    Recount(&m_root);
    RaiseReset();
}

void ShapedItemsSource6::SetAllNodesExpanded(bool expanded)
{
    m_nodeDefaultExpanded = expanded;
    ForEachDataNode([expanded](Node* node)
    {
        node->Expanded = expanded;
        node->OverlayCollapsed = !expanded;
    });

    if (expanded && m_childrenSelector)
    {
        for (auto const top : m_top)
        {
            MaterializeDeep(top);
        }
        Shape();
    }
    else
    {
        RecountAll();
    }
    RaiseReset();
}

void ShapedItemsSource6::ExpandSubtree(Node* node)
{
    if (!node || node->IsHeader || node == &m_root)
    {
        return;
    }
    MaterializeDeep(node);
    std::vector<Node*> stack{ node };
    while (!stack.empty())
    {
        auto const current = stack.back();
        stack.pop_back();
        current->Expanded = true;
        current->OverlayCollapsed = false;
        stack.insert(stack.end(), current->All.begin(), current->All.end());
    }
    auto const old = node->Visible;
    ShapeSubtree(node);
    PropagateFrom(node, node->Visible - old);
    InvalidateCursor();
    RaiseReset();
}

// --- Source ------------------------------------------------------------------------------------

void ShapedItemsSource6::Subscribe(Node* owner)
{
    auto& collection = CollectionOf(owner);
    collection.Revoke();

    auto weakThis = weak_from_this();
    auto const& accessor = collection.Accessor;
    if (auto const& incc = accessor.AsNotifyCollectionChanged())
    {
        collection.CollectionChangedRevoker = incc.CollectionChanged(winrt::auto_revoke,
            [weakThis, owner](auto&&, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->OnCollectionChanged(owner, args);
                }
            });
    }
    else if (auto const& vector = accessor.AsObservableVector())
    {
        collection.VectorChangedRevoker = vector.VectorChanged(winrt::auto_revoke,
            [weakThis, owner](auto&&, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->OnVectorChanged(owner, args);
                }
            });
    }
    else if (auto const& bindableVector = accessor.AsBindableObservableVector())
    {
        collection.BindableVectorChangedRevoker = bindableVector.VectorChanged(winrt::auto_revoke,
            [weakThis, owner](auto&&, winrt::IInspectable const& args)
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->OnVectorChanged(owner, args.try_as<winrt::Windows::Foundation::Collections::IVectorChangedEventArgs>());
                }
            });
    }
}

void ShapedItemsSource6::OnCollectionChanged(Node* owner, winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedEventArgs const& args)
{
    using Action = winrt::Microsoft::UI::Xaml::Interop::NotifyCollectionChangedAction;
    HandleSourceChange([&]()
    {
        auto const newItems = args.NewItems();
        auto const oldItems = args.OldItems();
        auto const newIndex = args.NewStartingIndex();
        auto const oldIndex = args.OldStartingIndex();
        switch (args.Action())
        {
        case Action::Add:
            if (newItems && newIndex >= 0)
            {
                InsertItems(owner, static_cast<uint32_t>(newIndex), ToVector(newItems));
                return;
            }
            break;
        case Action::Remove:
            if (oldItems && oldIndex >= 0)
            {
                RemoveItems(owner, static_cast<uint32_t>(oldIndex), oldItems.Size());
                return;
            }
            break;
        case Action::Replace:
            if (newItems && oldItems && oldIndex >= 0)
            {
                RemoveItems(owner, static_cast<uint32_t>(oldIndex), oldItems.Size());
                InsertItems(owner, static_cast<uint32_t>(newIndex >= 0 ? newIndex : oldIndex), ToVector(newItems));
                return;
            }
            break;
        case Action::Move:
            if (oldItems && oldIndex >= 0 && newIndex >= 0)
            {
                auto const moved = ToVector(newItems ? newItems : oldItems);
                RemoveItems(owner, static_cast<uint32_t>(oldIndex), oldItems.Size());
                InsertItems(owner, static_cast<uint32_t>(newIndex), moved);
                return;
            }
            break;
        default:
            break;
        }
        ResetOwner(owner);
    });
}

void ShapedItemsSource6::OnVectorChanged(Node* owner, winrt::Windows::Foundation::Collections::IVectorChangedEventArgs const& args)
{
    using Change = winrt::Windows::Foundation::Collections::CollectionChange;
    HandleSourceChange([&]()
    {
        if (args)
        {
            auto const& accessor = CollectionOf(owner).Accessor;
            auto const index = args.Index();
            winrt::IInspectable item{ nullptr };
            switch (args.CollectionChange())
            {
            case Change::ItemInserted:
                if (accessor.TryGetAt(index, item))
                {
                    InsertItems(owner, index, { item });
                    return;
                }
                break;
            case Change::ItemRemoved:
                RemoveItems(owner, index, 1);
                return;
            case Change::ItemChanged:
                if (accessor.TryGetAt(index, item))
                {
                    RemoveItems(owner, index, 1);
                    InsertItems(owner, index, { item });
                    return;
                }
                break;
            default:
                break;
            }
        }
        ResetOwner(owner);
    });
}

void ShapedItemsSource6::HandleSourceChange(std::function<void()> const& apply)
{
    try
    {
        if (m_cacheInvalid)
        {
            Rebuild(true, false, true);
        }
        else
        {
            apply();
        }
    }
    catch (...)
    {
        // The nodes no longer match the source. A transient conflict (an item added to its new
        // parent before it left the old one) resolves by the time a posted reload runs.
        m_cacheInvalid = true;
        ClearProjection();
        RaiseReset();
        if (!PostDeferredReload())
        {
            throw;
        }
        return;
    }
    NotifyRowsMetadataChanged();
}

bool ShapedItemsSource6::PostDeferredReload()
{
    if (!m_ownerQueue)
    {
        return false;
    }
    auto weakThis = weak_from_this();
    return m_ownerQueue.TryEnqueue([weakThis]()
        {
            if (auto const strongThis = weakThis.lock(); strongThis && strongThis->m_cacheInvalid)
            {
                try
                {
                    strongThis->Rebuild(true, false, true);
                }
                catch (...)
                {
                    // No app call to receive this; the next verb or change retries.
                }
            }
        });
}

void ShapedItemsSource6::InsertItems(Node* owner, uint32_t index, std::vector<winrt::IInspectable> const& items)
{
    auto& all = AllOf(owner);
    if (index > all.size())
    {
        ResetOwner(owner);
        return;
    }

    std::vector<Node*> created;
    created.reserve(items.size());
    try
    {
        for (auto const& item : items)
        {
            created.push_back(CreateNode(item, owner));
        }
    }
    catch (...)
    {
        for (auto const node : created)
        {
            ReleaseSubtree(node, false);
        }
        throw;
    }

    all.insert(all.begin() + index, created.begin(), created.end());
    for (auto i = index; i < all.size(); ++i)
    {
        all[i]->SrcIndex = static_cast<int32_t>(i);
    }

    for (auto const node : created)
    {
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(node->Item, true);
        }
        Evaluate(node);
        EvaluateHint(node);
        if (NeedsFullMaterialization())
        {
            MaterializeDeep(node);
        }
        ShapeSubtree(node);
    }

    // An unmaterialized owner has no presented children yet; its hint said whether it has any.
    if (owner && !owner->Materialized)
    {
        return;
    }
    for (auto const node : created)
    {
        if (node->Included && !Attach(node, true))
        {
            // A new group: its header's place depends on every group, so reshape.
            Shape();
            RaiseReset();
            return;
        }
    }
}

void ShapedItemsSource6::RemoveItems(Node* owner, uint32_t index, uint32_t count)
{
    auto& all = AllOf(owner);
    if (static_cast<size_t>(index) + count > all.size())
    {
        ResetOwner(owner);
        return;
    }

    std::vector<Node*> removed(all.begin() + index, all.begin() + index + count);
    for (auto it = removed.rbegin(); it != removed.rend(); ++it)
    {
        if ((*it)->IndexInParent >= 0)
        {
            Detach(*it, true);
        }
    }
    all.erase(all.begin() + index, all.begin() + index + count);
    for (auto i = index; i < all.size(); ++i)
    {
        all[i]->SrcIndex = static_cast<int32_t>(i);
    }
    for (auto const node : removed)
    {
        ReleaseSubtree(node, true);
    }
}

// A Reset re-reads the owner's collection but keeps every node whose object is still in it, with
// its subtree, subscriptions and expansion. Only objects that left are released and only new ones
// are created; the UI gets the difference between the old and new presented rows, not a Reset.
void ShapedItemsSource6::ResetOwner(Node* owner)
{
    if (m_cacheInvalid)
    {
        Rebuild(true, false, true);
        return;
    }

    auto const items = CollectionOf(owner).Accessor.Enumerate(true /* throwIfUnsupported */);
    auto const before = SnapshotPresented();
    InvalidateCursor();

    auto& all = AllOf(owner);
    std::unordered_map<void const*, Node*> previous;
    previous.reserve(all.size());
    for (auto const node : all)
    {
        previous.emplace(node->Id, node);
    }

    // Claim the survivors first, so an object that moved deeper or shallower within this owner's
    // subtree is released before it is created again.
    std::vector<Node*> next(items.size(), nullptr);
    for (size_t i = 0; i < items.size(); ++i)
    {
        if (!items[i])
        {
            ThrowCallerContract(L"TableViewSource6: items cannot contain null.");
        }
        if (auto const it = previous.find(ObjectIdentity(items[i])); it != previous.end() && it->second)
        {
            next[i] = it->second;
            it->second = nullptr;
        }
    }

    for (auto const node : all)
    {
        node->IndexInParent = -1;
    }
    for (auto const& [id, node] : previous)
    {
        if (node)
        {
            ReleaseSubtree(node, true);
        }
    }
    all.clear();

    std::vector<Node*> created;
    try
    {
        for (size_t i = 0; i < items.size(); ++i)
        {
            if (!next[i])
            {
                next[i] = CreateNode(items[i], owner);
                created.push_back(next[i]);
            }
            next[i]->SrcIndex = static_cast<int32_t>(i);
        }
    }
    catch (...)
    {
        // Keep the reachable nodes owned so the deferred reload releases them.
        for (auto const node : next)
        {
            if (node)
            {
                all.push_back(node);
            }
        }
        throw;
    }
    all = std::move(next);

    // Survivors are re-read too: a Reset is the app saying anything may have changed.
    std::vector<Node*> stack(all.begin(), all.end());
    while (!stack.empty())
    {
        auto const n = stack.back();
        stack.pop_back();
        Evaluate(n);
        EvaluateHint(n);
        stack.insert(stack.end(), n->All.begin(), n->All.end());
    }
    for (auto const node : created)
    {
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(node->Item, true);
        }
        if (NeedsFullMaterialization())
        {
            MaterializeDeep(node);
        }
    }

    Shape();
    RaisePresentedDiff(before);
}

std::vector<winrt::IInspectable> ShapedItemsSource6::SnapshotPresented() const
{
    // Strong references pin the addresses, so a released object's address cannot be reused by a
    // new one before the comparison.
    std::vector<winrt::IInspectable> rows;
    rows.reserve(static_cast<size_t>(m_root.Visible));
    if (!m_root.Kids.empty())
    {
        for (auto node = m_root.Kids.front(); node; node = NextVisible(node))
        {
            rows.push_back(node->Item);
        }
    }
    return rows;
}

void ShapedItemsSource6::RaisePresentedDiff(std::vector<winrt::IInspectable> const& before)
{
    InvalidateCursor();
    std::vector<void*> after;
    after.reserve(static_cast<size_t>(m_root.Visible));
    if (!m_root.Kids.empty())
    {
        for (auto node = m_root.Kids.front(); node; node = NextVisible(node))
        {
            after.push_back(winrt::get_abi(node->Item));
        }
    }

    size_t prefix = 0;
    auto const common = std::min(before.size(), after.size());
    while (prefix < common && winrt::get_abi(before[prefix]) == after[prefix])
    {
        ++prefix;
    }
    size_t suffix = 0;
    while (suffix < common - prefix &&
        winrt::get_abi(before[before.size() - 1 - suffix]) == after[after.size() - 1 - suffix])
    {
        ++suffix;
    }

    auto const removed = static_cast<int32_t>(before.size() - prefix - suffix);
    auto const added = static_cast<int32_t>(after.size() - prefix - suffix);
    // The list already reports the new size, so only a single-sided change is one consistent event.
    if (removed == 0 && added == 0)
    {
        return;
    }
    if (removed == 0)
    {
        RaiseAdded(static_cast<int32_t>(prefix), added);
    }
    else if (added == 0)
    {
        RaiseRemoved(static_cast<int32_t>(prefix), removed);
    }
    else
    {
        RaiseReset();
    }
}

// --- Publishing --------------------------------------------------------------------------------

void ShapedItemsSource6::RaiseAdded(int32_t index, int32_t count)
{
    InvalidateCursor();
    if (count > 0)
    {
        m_list->RaiseAdded(index, count);
    }
}

void ShapedItemsSource6::RaiseRemoved(int32_t index, int32_t count)
{
    InvalidateCursor();
    if (count > 0)
    {
        m_list->RaiseRemoved(index, count);
    }
}

void ShapedItemsSource6::RaiseReplaced(int32_t index)
{
    InvalidateCursor();
    m_list->RaiseReplaced(index);
}

void ShapedItemsSource6::RaiseReset()
{
    InvalidateCursor();
    if (m_list)
    {
        m_list->RaiseReset();
    }
}

void ShapedItemsSource6::RaiseShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        m_shapingChanged(reorderOnly);
    }
}

void ShapedItemsSource6::NotifyRowsMetadataChanged()
{
    if (m_rowsMetadataChanged)
    {
        m_rowsMetadataChanged();
    }
}

// --- Live shaping ------------------------------------------------------------------------------

void ShapedItemsSource6::SetLiveShaping(bool enabled)
{
    if (m_liveShapingEnabled == enabled)
    {
        return;
    }

    m_liveShapingEnabled = enabled;
    if (enabled)
    {
        // In-place edits made while untracked are invisible to the nodes, so catch up once.
        RefreshLiveSubscriptions();
        if (m_liveItemChangedHook)
        {
            m_liveItemChangedHook(nullptr, true);
        }
        m_liveFullRestore = true;
        MarkLiveDirty();
    }
    else
    {
        ClearLiveSubscriptions();
    }
}

void ShapedItemsSource6::RefreshLiveSubscriptions()
{
    if (!m_liveShapingEnabled)
    {
        return;
    }
    std::unordered_set<void const*> live;
    live.reserve(m_byItem.size());
    for (auto const& [id, node] : m_byItem)
    {
        m_liveShaping->Subscribe(node->Item);
        live.insert(id);
    }
    m_liveShaping->RetainOnly(live);
}

void ShapedItemsSource6::ClearLiveSubscriptions()
{
    m_liveShaping->UnsubscribeAll();
    m_liveShapingDirty = false;
    m_liveShapingHeld = false;
    m_liveFullRestore = false;
    m_liveDirtyItems.clear();
    m_liveShapingChangeDropped->store(false);
}

bool ShapedItemsSource6::ShapeChanged(Node const* node, Node const& fresh, bool& needsFull) const
{
    if (node->SelfMatch != fresh.SelfMatch ||
        node->GroupKey != fresh.GroupKey ||
        (node->GroupError == nullptr) != (fresh.GroupError == nullptr))
    {
        needsFull = true;
        return true;
    }
    if (node->SortKeys.size() != fresh.SortKeys.size())
    {
        return true;
    }
    for (size_t k = 0; k < node->SortKeys.size(); ++k)
    {
        if (ShapingHelpers::ValueComparer::Compare(node->SortKeys[k], fresh.SortKeys[k]) != 0)
        {
            return true;
        }
    }
    return false;
}

void ShapedItemsSource6::OnLiveItemChanged(winrt::IInspectable const& item)
{
    if (!m_liveShapingEnabled || !item)
    {
        return;
    }

    if (m_liveShapingChangeDropped->exchange(false))
    {
        // A change was lost; only a full restore is sure to see it.
        m_liveFullRestore = true;
        MarkLiveDirtyForItem();
    }

    if (m_liveItemChangedHook && m_liveItemChangedHook(item, m_liveShapingDirty))
    {
        m_liveFullRestore = true;
        MarkLiveDirtyForItem();
        return;
    }

    if (m_liveFullRestore)
    {
        EnsureLiveRestore();
        return;
    }

    auto const it = m_byItem.find(ObjectIdentity(item));
    if (it == m_byItem.end())
    {
        return;
    }

    // Compare, don't store: the nodes must keep matching the projection until the restore runs.
    Node fresh;
    fresh.Item = item;
    fresh.DataParent = it->second->DataParent;
    Evaluate(&fresh);
    bool needsFull = false;
    if (ShapeChanged(it->second, fresh, needsFull))
    {
        if (needsFull)
        {
            m_liveFullRestore = true;
        }
        else
        {
            m_liveDirtyItems.push_back(item);
        }
        MarkLiveDirtyForItem();
    }
}

void ShapedItemsSource6::MarkLiveDirty()
{
    m_liveShapingDirty = true;
    EnsureLiveRestore();
}

void ShapedItemsSource6::MarkLiveDirtyForItem()
{
    if (!m_liveShapingDirty && m_liveItemReshape)
    {
        m_liveItemReshape();
    }
    MarkLiveDirty();
}

void ShapedItemsSource6::EnsureLiveRestore()
{
    if (!m_liveShapingDirty || m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_liveShapingHeld)
    {
        if (m_liveShapingHold && m_liveShapingHold())
        {
            return;
        }
        m_liveShapingHeld = false;
    }

    PostLiveRestore();
}

void ShapedItemsSource6::PostLiveRestore()
{
    if (m_liveShapingRestorePosted)
    {
        return;
    }

    if (m_ownerQueue)
    {
        auto weakThis = weak_from_this();
        // Posted: coalesces a turn's writes into one restore, outside the app's PropertyChanged.
        m_liveShapingRestorePosted = m_ownerQueue.TryEnqueue([weakThis]()
            {
                if (auto const strongThis = weakThis.lock())
                {
                    strongThis->m_liveShapingRestorePosted = false;
                    try
                    {
                        strongThis->RestoreLive();
                    }
                    catch (...)
                    {
                    }
                }
            });
        return;
    }

    try
    {
        RestoreLive();
    }
    catch (...)
    {
    }
}

void ShapedItemsSource6::RestoreLive()
{
    if (!m_liveShapingDirty)
    {
        return;
    }

    if (!m_liveShapingEnabled)
    {
        m_liveShapingDirty = false;
        m_liveFullRestore = false;
        m_liveDirtyItems.clear();
        return;
    }

    if (m_liveShapingHold && m_liveShapingHold())
    {
        m_liveShapingHeld = true;
        return;
    }

    auto const full = m_liveFullRestore;
    auto dirtyItems = std::move(m_liveDirtyItems);
    m_liveDirtyItems.clear();
    m_liveShapingDirty = false;
    m_liveFullRestore = false;
    m_liveShapingChangeDropped->store(false);

    if (full || !TryResplice(dirtyItems))
    {
        Rebuild(false, false, true);
    }
    RaiseShapingChanged(false /* reorderOnly */);
}

bool ShapedItemsSource6::TryResplice(std::vector<winrt::IInspectable> const& items)
{
    // Group order follows the earlier sorts' first appearance, so moving a top row can reorder headers.
    if (m_groupKey && !m_preAxes.empty())
    {
        return false;
    }

    // Re-evaluate first, then detach every moved row, then attach them: each attach binary-searches
    // siblings that are all in order.
    std::vector<Node*> moved;
    std::unordered_set<Node*> seen;
    for (auto const& item : items)
    {
        auto const it = m_byItem.find(ObjectIdentity(item));
        if (it == m_byItem.end() || !seen.insert(it->second).second)
        {
            continue;
        }
        auto const node = it->second;
        Node fresh;
        fresh.Item = item;
        fresh.DataParent = node->DataParent;
        Evaluate(&fresh);
        bool needsFull = false;
        if (!ShapeChanged(node, fresh, needsFull))
        {
            continue;
        }
        if (needsFull)
        {
            return false;
        }
        node->SortKeys = std::move(fresh.SortKeys);
        if (node->IndexInParent >= 0)
        {
            moved.push_back(node);
        }
    }

    if (moved.size() > c_maxLiveResplices)
    {
        return false;
    }

    for (auto const node : moved)
    {
        Detach(node, false);
    }
    std::unordered_set<Node*> headers;
    for (auto const node : moved)
    {
        if (!Attach(node, false))
        {
            return false;
        }
        if (node->Parent->IsHeader)
        {
            headers.insert(node->Parent);
        }
    }
    for (auto const header : headers)
    {
        RefreshHeaderEntry(header, false);
    }
    NotifyRowsMetadataChanged();
    return true;
}

void ShapedItemsSource6::ResumeHeldLiveShaping()
{
    if (!std::exchange(m_liveShapingHeld, false))
    {
        return;
    }
    EnsureLiveRestore();
}
