// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource6.h"
#include "ShapedItemsSource6.h"
#include "ShapingHelpers.h"
#include "SortMemberPathResolver.h"
#include "SharedHelpers.h"

namespace
{
    winrt::hstring SortAxisTokenForPath(winrt::hstring const& sortMemberPath)
    {
        // Same token v1 mints, so the owner recognizes a path-declared axis on either variant.
        return L"path:" + sortMemberPath;
    }

    void const* IdentityOf(winrt::IInspectable const& value)
    {
        return value ? winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
    }

    // Row metadata read straight off the engine's nodes. Data rows are identified by "p:" plus
    // their object address; group headers by "group:" plus the group key.
    class TableViewSource6RowMetadata final : public ITableViewRowMetadataProvider
    {
    public:
        TableViewSource6RowMetadata(std::weak_ptr<ShapedItemsSource6> engine, void const* viewIdentity) :
            m_engine(std::move(engine)),
            m_viewIdentity(viewIdentity)
        {
        }

        TableViewRowInfo GetRowInfo(int32_t index) override
        {
            auto const engine = Engine();
            auto const node = engine->NodeAt(index);

            TableViewRowInfo info;
            if (node->IsHeader)
            {
                auto const count = static_cast<int32_t>(node->Kids.size());
                info.Kind = TableViewRowKind::GroupHeader;
                info.Level = 0;
                info.ChildCount = count;
                info.IsExpandable = count > 0;
                info.IsExpanded = node->Open;
                return info;
            }

            if (!engine->IsHierarchical())
            {
                info.Level = engine->IsGrouped() ? 1 : 0;
                return info;
            }

            // Level is 1-based for tree rows (UIA Level); under a group the header is level 0.
            info.Level = node->Depth + 1;
            info.IsExpandable = engine->IsExpandable(node);
            info.IsExpanded = node->Open;
            info.ChildCount = engine->ChildCountOf(node);
            if (node->DataParent)
            {
                info.ParentIdentity = engine->IdentityOf(node->DataParent);
            }
            info.PositionInSet = node->IndexInParent + 1;
            info.SizeOfSet = static_cast<int32_t>(node->Parent->Kids.size());
            return info;
        }

        winrt::hstring GetIdentity(int32_t index) override
        {
            auto const engine = Engine();
            return engine->IdentityOf(engine->NodeAt(index));
        }

        bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) override
        {
            index = -1;
            auto const engine = m_engine.lock();
            if (!engine)
            {
                return false;
            }
            index = engine->IndexOf(engine->NodeOfIdentity(identity));
            return index >= 0;
        }

        bool Expand(winrt::hstring const& key) override { return SetExpanded(key, true); }
        bool Collapse(winrt::hstring const& key) override { return SetExpanded(key, false); }
        bool Toggle(winrt::hstring const& key) override { return SetExpanded(key, std::nullopt); }

        bool WouldChangeExpansion(winrt::hstring const& key, std::optional<bool> desired) noexcept override
        {
            try
            {
                auto const engine = m_engine.lock();
                if (!engine)
                {
                    return false;
                }
                auto const node = engine->NodeOfIdentity(key);
                if (!node || !engine->IsExpandable(node) || engine->IndexOf(node) < 0)
                {
                    return false;
                }
                return !desired || *desired != node->Open;
            }
            catch (...)
            {
                return true;
            }
        }

        void ExpandAllGroups() override { Engine()->SetAllGroupsExpanded(true); }
        void CollapseAllGroups() override { Engine()->SetAllGroupsExpanded(false); }
        void ExpandAllRows() override
        {
            auto const engine = Engine();
            if (engine->IsHierarchical())
            {
                engine->SetAllNodesExpanded(true);
            }
            else if (engine->IsGrouped())
            {
                engine->SetAllGroupsExpanded(true);
            }
        }
        void CollapseAllRows() override
        {
            auto const engine = Engine();
            if (engine->IsHierarchical())
            {
                engine->SetAllNodesExpanded(false);
            }
            else if (engine->IsGrouped())
            {
                engine->SetAllGroupsExpanded(false);
            }
        }

        bool IsHierarchicalSource() const override
        {
            auto const engine = m_engine.lock();
            return engine && engine->IsHierarchical();
        }

        void ExpandSubtree(winrt::hstring const& key) override
        {
            auto const engine = Engine();
            if (engine->IsHierarchical())
            {
                engine->ExpandSubtree(engine->NodeOfIdentity(key));
            }
        }

        bool TryGetGroupMembership(
            winrt::Windows::Foundation::IInspectable const& rows,
            int32_t index,
            int32_t& positionInGroup,
            int32_t& groupSize) override
        {
            auto const engine = m_engine.lock();
            if (!engine || !engine->IsGrouped() || !rows)
            {
                return false;
            }
            auto const identity = IdentityOf(rows);
            if (identity != m_viewIdentity && identity != IdentityOf(engine->Rows()))
            {
                return false;
            }
            if (index < 0 || index >= engine->Size())
            {
                return false;
            }
            auto const node = engine->NodeAt(index);
            if (node->IsHeader || !node->Parent || !node->Parent->IsHeader)
            {
                return false;
            }
            positionInGroup = node->IndexInParent + 1;
            groupSize = static_cast<int32_t>(node->Parent->Kids.size());
            return true;
        }

    private:
        std::shared_ptr<ShapedItemsSource6> Engine() const
        {
            auto engine = m_engine.lock();
            if (!engine)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return engine;
        }

        bool SetExpanded(winrt::hstring const& key, std::optional<bool> desired)
        {
            auto const engine = m_engine.lock();
            if (!engine)
            {
                return false;
            }
            auto const node = engine->NodeOfIdentity(key);
            if (!node)
            {
                return false;
            }
            return engine->SetExpanded(node, desired.value_or(!node->Open));
        }

        std::weak_ptr<ShapedItemsSource6> m_engine;
        void const* m_viewIdentity;
    };
}

TableViewSource6::~TableViewSource6() = default;

TableViewSource6::TableViewSource6(winrt::IInspectable const& items) :
    m_engine(std::make_shared<::ShapedItemsSource6>(items))
{
    m_engine->SetProjectionRebuiltHandler([this]() { NotifyOwnerProjectionChanged(); });
    m_engine->SetShapingChangedHandler([this](bool reorderOnly) { NotifyOwnerShapingChanged(reorderOnly); });
    m_engine->Start();

    winrt::ItemsSourceView view{ m_engine->Rows() };
    m_itemsSourceView.set(view);
    m_rowMetadata = std::make_shared<TableViewSource6RowMetadata>(m_engine, IdentityOf(view));
}

winrt::TableViewSource6 TableViewSource6::From(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }
    return winrt::make<::TableViewSource6>(items);
}

winrt::TableViewSource6 TableViewSource6::From(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector)
{
    if (keySelector)
    {
        throw winrt::hresult_not_implemented(L"TableViewSource6 identifies rows by object identity; a key selector is not supported.");
    }
    return From(items);
}

winrt::ITableViewShapingSource TableViewSource6::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!predicate)
    {
        throw winrt::hresult_invalid_argument(L"predicate cannot be null.");
    }
    m_engine->SetFilter([predicate](winrt::IInspectable const& item) { return predicate(item); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::GroupBy(winrt::TableViewKeySelector const& key)
{
    return GroupBy(key, nullptr);
}

winrt::ITableViewShapingSource TableViewSource6::GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }
    RowIdentity::IdentitySelector groupIdentity{ nullptr };
    if (groupIdentitySelector)
    {
        groupIdentity = [groupIdentitySelector](winrt::IInspectable const& item) { return groupIdentitySelector(item); };
    }
    m_engine->SetGroup([key](winrt::IInspectable const& item) { return key(item); }, groupIdentity);
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::ChildrenWith(winrt::TableViewChildrenSelector const& childrenSelector)
{
    return ChildrenWith(childrenSelector, nullptr);
}

winrt::ITableViewShapingSource TableViewSource6::ChildrenWith(
    winrt::TableViewChildrenSelector const& childrenSelector,
    winrt::TableViewHasChildrenPredicate const& hasChildrenSelector)
{
    if (!childrenSelector)
    {
        throw winrt::hresult_invalid_argument(L"childrenSelector cannot be null.");
    }

    ShapedItemsSource6::HasChildrenSelector hasChildren{ nullptr };
    if (hasChildrenSelector)
    {
        hasChildren = [hasChildrenSelector](winrt::IInspectable const& item) { return hasChildrenSelector(item); };
    }
    m_engine->SetChildren(
        [childrenSelector](winrt::IInspectable const& item) { return childrenSelector(item); },
        hasChildren);
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::ClearChildrenWith()
{
    m_engine->ClearChildren();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::ClearFilter()
{
    m_engine->ClearFilter();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::ClearGroupBy()
{
    m_engine->ClearGroup();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::ClearSort()
{
    m_engine->ClearSorts();
    return *this;
}

void TableViewSource6::ClearSort(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSort(sortAxisToken);
}

void TableViewSource6::ClearSortsExcept(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSortsExcept(sortAxisToken);
}

winrt::ITableViewShapingSource TableViewSource6::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    SortReplacing({}, {}, key, {}, direction);
    return *this;
}

winrt::ITableViewShapingSource TableViewSource6::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (sortMemberPath.empty())
    {
        throw winrt::hresult_invalid_argument(L"sortMemberPath cannot be empty.");
    }
    auto resolver = std::make_shared<SortMemberPathResolver>(sortMemberPath);
    winrt::TableViewKeySelector key{
        [resolver](winrt::IInspectable const& item) -> winrt::IInspectable
        {
            return resolver->Resolve(item);
        }
    };
    SortReplacing({}, SortAxisTokenForPath(sortMemberPath), key, sortMemberPath, direction);
    return *this;
}

void TableViewSource6::SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"key cannot be null.");
    }
    if (direction != winrt::SortDirection::None &&
        direction != winrt::SortDirection::Ascending &&
        direction != winrt::SortDirection::Descending)
    {
        throw winrt::hresult_invalid_argument(L"direction must be a defined SortDirection value.");
    }

    // The delegate is the axis identity: std::function cannot be compared.
    auto const keyIdentity = key.as<winrt::Windows::Foundation::IUnknown>();
    m_engine->SetSort(
        previousSortAxisToken,
        sortAxisToken,
        [key](winrt::IInspectable const& item) { return key(item); },
        keyIdentity,
        sortMemberPath,
        direction);
}

bool TableViewSource6::IsLiveShaping() const
{
    return m_engine->IsLiveShapingEnabled();
}

void TableViewSource6::IsLiveShaping(bool value)
{
    m_engine->SetLiveShaping(value);
}

std::vector<TableViewSource6::ActiveSortAxisInfo> TableViewSource6::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_engine->ActiveSortAxes())
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

winrt::ItemsSourceView TableViewSource6::GetItemsSourceView()
{
    return m_itemsSourceView.get();
}

TableViewRowMetadataProvider TableViewSource6::GetRowMetadata()
{
    return m_rowMetadata;
}

bool TableViewSource6::IsGrouped() const
{
    return m_engine->IsGrouped();
}

bool TableViewSource6::ContainsShapedItem(winrt::IInspectable const& item) const
{
    return m_engine->ContainsItem(item);
}

bool TableViewSource6::SetOwningTableView(winrt::IInspectable const& owner)
{
    // One owner at a time, as for TableViewSource: two controls would compete for one projection.
    if (auto existingOwner = m_owningTableView.get(); existingOwner && existingOwner != owner)
    {
        return false;
    }
    m_owningTableView = winrt::weak_ref<winrt::IInspectable>{ owner };
    RemoveOwnerReleasedHandler(owner);
    return true;
}

void TableViewSource6::AddOwnerReleasedHandler(winrt::IInspectable const& waiter, std::function<void()> handler)
{
    RemoveOwnerReleasedHandler(waiter);
    m_ownerReleasedWaiters.push_back({ winrt::weak_ref<winrt::IInspectable>{ waiter }, std::move(handler) });
}

void TableViewSource6::RemoveOwnerReleasedHandler(winrt::IInspectable const& waiter)
{
    std::erase_if(m_ownerReleasedWaiters, [&waiter](OwnerReleasedWaiter const& entry)
    {
        auto const entryWaiter = entry.Waiter.get();
        return !entryWaiter || entryWaiter == waiter;
    });
}

void TableViewSource6::ReleaseOwningTableView(winrt::IInspectable const& owner)
{
    if (auto existingOwner = m_owningTableView.get(); existingOwner && existingOwner != owner)
    {
        return;
    }

    m_owningTableView = winrt::weak_ref<winrt::IInspectable>{ nullptr };
    m_projectionChanged = nullptr;
    m_shapingChanged = nullptr;
    m_engine->SetLiveShapingHold(nullptr);
    m_engine->SetLiveItemChangedHook(nullptr);
    m_engine->SetRowsReleasedHook(nullptr);
    m_engine->SetLiveItemReshapeHandler(nullptr);
    m_engine->SetRowsMetadataChangedHook(nullptr);
    m_engine->ResumeHeldLiveShaping();

    // Each waiter is popped before it runs: it re-enters SetOwningTableView.
    while (!m_owningTableView.get() && !m_ownerReleasedWaiters.empty())
    {
        auto waiter = std::move(m_ownerReleasedWaiters.front());
        m_ownerReleasedWaiters.erase(m_ownerReleasedWaiters.begin());
        if (waiter.Waiter.get() && waiter.Handler)
        {
            waiter.Handler();
        }
    }
}

void TableViewSource6::SetLiveShapingHold(std::function<bool()> isHeld)
{
    m_engine->SetLiveShapingHold(std::move(isHeld));
}

void TableViewSource6::ResumeHeldLiveShaping()
{
    m_engine->ResumeHeldLiveShaping();
}

void TableViewSource6::SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook)
{
    m_engine->SetLiveItemChangedHook(std::move(hook));
}

void TableViewSource6::SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook)
{
    m_engine->SetRowsReleasedHook(std::move(hook));
}

void TableViewSource6::SetLiveItemReshapeHandler(std::function<void()> handler)
{
    m_engine->SetLiveItemReshapeHandler(std::move(handler));
}

void TableViewSource6::SetRowsMetadataChangedHook(std::function<void()> hook)
{
    m_engine->SetRowsMetadataChangedHook(std::move(hook));
}

void TableViewSource6::NotifyOwnerProjectionChanged()
{
    if (m_projectionChanged)
    {
        // The owner can run app code that releases the last reference to this source.
        auto const strongThis = get_strong();
        ++m_notificationDepth;
        auto restore = gsl::finally([this]() noexcept { --m_notificationDepth; });
        m_projectionChanged();
    }
}

void TableViewSource6::NotifyOwnerShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        auto const strongThis = get_strong();
        ++m_notificationDepth;
        auto restore = gsl::finally([this]() noexcept { --m_notificationDepth; });
        m_shapingChanged(reorderOnly);
    }
}
