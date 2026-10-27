// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource2.h"
#include "ShapedItemsSource2.h"
#include "ShapingHelpers.h"
#include "SortMemberPathResolver.h"
#include "SharedHelpers.h"

namespace
{
    // Matches RowMetadataProvider's group expansion keys, so the owner reads both variants alike.
    constexpr std::wstring_view c_groupIdentityPrefix{ L"group:" };

    bool TryStripGroupPrefix(winrt::hstring const& identity, winrt::hstring& groupKey)
    {
        std::wstring_view const view{ identity };
        if (view.size() <= c_groupIdentityPrefix.size() || view.substr(0, c_groupIdentityPrefix.size()) != c_groupIdentityPrefix)
        {
            return false;
        }
        groupKey = winrt::hstring{ view.substr(c_groupIdentityPrefix.size()) };
        return true;
    }

    winrt::hstring SortAxisTokenForPath(winrt::hstring const& sortMemberPath)
    {
        // Same token v1 mints, so the owner recognizes a path-declared axis on either variant.
        return L"path:" + sortMemberPath;
    }

    // Row metadata read straight off the engine's pre-order rows. Data rows are identified by
    // their key; group headers by "group:" + group key.
    class TableViewSource2RowMetadata final : public ITableViewRowMetadataProvider
    {
    public:
        TableViewSource2RowMetadata(std::weak_ptr<ShapedItemsSource2> engine) :
            m_engine(std::move(engine))
        {
        }

        TableViewRowInfo GetRowInfo(int32_t index) override
        {
            auto const engine = Engine();
            auto const pos = PositionAt(*engine, index);
            auto const& row = engine->PreOrderRows()[pos];

            TableViewRowInfo info;
            if (row.Src < 0)
            {
                auto const& group = engine->Groups()[row.Group];
                info.Kind = TableViewRowKind::GroupHeader;
                info.Level = 0;
                info.ChildCount = group.ItemCount;
                info.IsExpandable = group.ItemCount > 0;
                info.IsExpanded = row.Expanded;
                return info;
            }

            auto const grouped = engine->IsGrouped();
            if (!engine->IsHierarchical())
            {
                info.Level = grouped ? 1 : 0;
                return info;
            }

            // Level is 1-based for tree rows (UIA Level); under a group the header is level 0.
            info.Level = row.Depth + (grouped ? 0 : 1);
            info.IsExpandable = row.ChildCount > 0;
            info.IsExpanded = row.Expanded;
            info.ChildCount = row.ChildCount;
            if (row.ParentPos >= 0 && engine->PreOrderRows()[row.ParentPos].Src >= 0)
            {
                info.ParentIdentity = engine->KeyAt(row.ParentPos);
            }
            info.PositionInSet = row.PosInSet;
            info.SizeOfSet = row.SizeOfSet;
            return info;
        }

        winrt::hstring GetIdentity(int32_t index) override
        {
            auto const engine = Engine();
            auto const pos = PositionAt(*engine, index);
            auto const& row = engine->PreOrderRows()[pos];
            if (row.Src < 0)
            {
                return winrt::hstring{ c_groupIdentityPrefix } + engine->Groups()[row.Group].GroupKey;
            }
            return engine->KeyAt(pos);
        }

        bool TryGetIndexForIdentity(winrt::hstring const& identity, int32_t& index) override
        {
            index = -1;
            auto const engine = m_engine.lock();
            if (!engine)
            {
                return false;
            }
            auto const pos = Resolve(*engine, identity);
            if (pos < 0)
            {
                return false;
            }
            index = engine->PresentedIndexOf(pos);
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
                auto const pos = Resolve(*engine, key);
                if (pos < 0)
                {
                    return false;
                }
                auto const& row = engine->PreOrderRows()[pos];
                if (row.ChildCount == 0 || engine->PresentedIndexOf(pos) < 0)
                {
                    return false;
                }
                return !desired || *desired != row.Expanded;
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
                engine->ExpandSubtree(key);
            }
        }

        bool TryGetGroupMembership(
            winrt::Windows::Foundation::IInspectable const& rows,
            int32_t index,
            int32_t& positionInGroup,
            int32_t& groupSize) override
        {
            auto const engine = m_engine.lock();
            if (!engine || engine->Kind() != ShapedItemsSource2::ProjectionKind::Grouped)
            {
                return false;
            }
            auto const& presented = engine->Rows();
            if (!rows || IdentityOf(rows) != IdentityOf(presented))
            {
                return false;
            }
            auto const count = static_cast<int32_t>(presented.Size());
            if (index < 0 || index >= count)
            {
                return false;
            }
            auto const isHeader = [&](int32_t i)
            {
                auto const pos = engine->PositionOfPresented(presented.GetAt(static_cast<uint32_t>(i)));
                return pos >= 0 && engine->PreOrderRows()[pos].Src < 0;
            };
            if (isHeader(index))
            {
                return false;
            }
            int32_t previousHeader = index - 1;
            while (previousHeader >= 0 && !isHeader(previousHeader))
            {
                --previousHeader;
            }
            int32_t nextHeader = index + 1;
            while (nextHeader < count && !isHeader(nextHeader))
            {
                ++nextHeader;
            }
            positionInGroup = index - previousHeader;
            groupSize = nextHeader - previousHeader - 1;
            return true;
        }

    private:
        static void const* IdentityOf(winrt::IInspectable const& value)
        {
            return value ? winrt::get_abi(value.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;
        }

        std::shared_ptr<ShapedItemsSource2> Engine() const
        {
            auto engine = m_engine.lock();
            if (!engine)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return engine;
        }

        // Resolved through the presented object, so metadata matches the row the consumer sees even
        // while a change notification is mid-flight.
        static int32_t PositionAt(ShapedItemsSource2 const& engine, int32_t index)
        {
            auto const& presented = engine.Rows();
            if (index < 0 || static_cast<uint32_t>(index) >= presented.Size())
            {
                throw winrt::hresult_out_of_bounds();
            }
            auto const pos = engine.PositionOfPresented(presented.GetAt(static_cast<uint32_t>(index)));
            if (pos < 0)
            {
                throw winrt::hresult_out_of_bounds();
            }
            return pos;
        }

        // A data key wins over a header identity that spells the same string.
        static int32_t Resolve(ShapedItemsSource2 const& engine, winrt::hstring const& identity)
        {
            if (auto const pos = engine.PositionOfKey(identity); pos >= 0)
            {
                return pos;
            }
            winrt::hstring groupKey;
            if (engine.IsGrouped() && TryStripGroupPrefix(identity, groupKey))
            {
                return engine.PositionOfGroup(groupKey);
            }
            return -1;
        }

        bool SetExpanded(winrt::hstring const& key, std::optional<bool> desired)
        {
            auto const engine = m_engine.lock();
            if (!engine)
            {
                return false;
            }
            auto const pos = Resolve(*engine, key);
            if (pos < 0)
            {
                return false;
            }
            auto const& row = engine->PreOrderRows()[pos];
            auto const expanded = desired.value_or(!row.Expanded);
            if (row.Src < 0)
            {
                return engine->SetGroupExpanded(engine->Groups()[row.Group].GroupKey, expanded);
            }
            return engine->SetNodeExpanded(engine->KeyAt(pos), expanded);
        }

        std::weak_ptr<ShapedItemsSource2> m_engine;
    };
}

TableViewSource2::~TableViewSource2() = default;

TableViewSource2::TableViewSource2(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector) :
    m_engine(std::make_shared<::ShapedItemsSource2>(
        items,
        [keySelector](winrt::IInspectable const& item) { return keySelector(item); }))
{
    m_engine->SetProjectionRebuiltHandler([this]() { NotifyOwnerProjectionChanged(); });
    m_engine->SetShapingChangedHandler([this](bool reorderOnly) { NotifyOwnerShapingChanged(reorderOnly); });
    m_engine->Start();

    m_itemsSourceView.set(winrt::ItemsSourceView{ m_engine->Rows() });
    m_rowMetadata = std::make_shared<TableViewSource2RowMetadata>(m_engine);
}

winrt::TableViewSource2 TableViewSource2::From(winrt::IInspectable const&)
{
    throw winrt::hresult_invalid_argument(L"TableViewSource2 requires a key selector: use From(items, keySelector).");
}

winrt::TableViewSource2 TableViewSource2::From(winrt::IInspectable const& items, winrt::TableViewIdentitySelector const& keySelector)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }
    if (!keySelector)
    {
        throw winrt::hresult_invalid_argument(L"keySelector cannot be null.");
    }
    return winrt::make<::TableViewSource2>(items, keySelector);
}

winrt::ITableViewShapingSource TableViewSource2::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!predicate)
    {
        throw winrt::hresult_invalid_argument(L"predicate cannot be null.");
    }
    m_engine->SetFilter([predicate](winrt::IInspectable const& item) { return predicate(item); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::GroupBy(winrt::TableViewKeySelector const& key)
{
    return GroupBy(key, nullptr);
}

winrt::ITableViewShapingSource TableViewSource2::GroupBy(winrt::TableViewKeySelector const& key, winrt::TableViewIdentitySelector const& groupIdentitySelector)
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

winrt::ITableViewShapingSource TableViewSource2::ParentBy(winrt::TableViewIdentitySelector const& parentSelector)
{
    if (!parentSelector)
    {
        throw winrt::hresult_invalid_argument(L"parentSelector cannot be null.");
    }
    m_engine->SetParent([parentSelector](winrt::IInspectable const& item) { return parentSelector(item); });
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::ClearFilter()
{
    m_engine->ClearFilter();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::ClearGroupBy()
{
    m_engine->ClearGroup();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::ClearParentBy()
{
    m_engine->ClearParent();
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::ClearSort()
{
    m_engine->ClearSorts();
    return *this;
}

void TableViewSource2::ClearSort(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSort(sortAxisToken);
}

void TableViewSource2::ClearSortsExcept(winrt::hstring const& sortAxisToken)
{
    m_engine->ClearSortsExcept(sortAxisToken);
}

winrt::ITableViewShapingSource TableViewSource2::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    SortReplacing({}, {}, key, {}, direction);
    return *this;
}

winrt::ITableViewShapingSource TableViewSource2::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
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

void TableViewSource2::SortReplacing(winrt::hstring const& previousSortAxisToken, winrt::hstring const& sortAxisToken, winrt::TableViewKeySelector const& key, winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
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

bool TableViewSource2::IsLiveShaping() const
{
    return m_engine->IsLiveShapingEnabled();
}

void TableViewSource2::IsLiveShaping(bool value)
{
    m_engine->SetLiveShaping(value);
}

std::vector<TableViewSource2::ActiveSortAxisInfo> TableViewSource2::ActiveSortAxisInfos() const
{
    std::vector<ActiveSortAxisInfo> infos;
    for (auto const& axis : m_engine->ActiveSortAxes())
    {
        infos.push_back({ axis.AxisToken, axis.SortMemberPath, axis.Direction });
    }
    return infos;
}

winrt::ItemsSourceView TableViewSource2::GetItemsSourceView()
{
    return m_itemsSourceView.get();
}

TableViewRowMetadataProvider TableViewSource2::GetRowMetadata()
{
    return m_rowMetadata;
}

bool TableViewSource2::IsGrouped() const
{
    return m_engine->IsGrouped();
}

bool TableViewSource2::ContainsShapedItem(winrt::IInspectable const& item) const
{
    return m_engine->ContainsItem(item);
}

bool TableViewSource2::HasFilter() const
{
    return m_engine->HasFilter();
}

bool TableViewSource2::HasGroupBy() const
{
    return m_engine->HasGroup();
}

bool TableViewSource2::HasParentBy() const
{
    return m_engine->HasParent();
}

uint32_t TableViewSource2::SourceRowCount() const
{
    return m_engine->SourceRowCount();
}

uint32_t TableViewSource2::DataRowCount() const
{
    return m_engine->IncludedDataRowCount();
}

bool TableViewSource2::SetOwningTableView(winrt::IInspectable const& owner)
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

void TableViewSource2::AddOwnerReleasedHandler(winrt::IInspectable const& waiter, std::function<void()> handler)
{
    RemoveOwnerReleasedHandler(waiter);
    m_ownerReleasedWaiters.push_back({ winrt::weak_ref<winrt::IInspectable>{ waiter }, std::move(handler) });
}

void TableViewSource2::RemoveOwnerReleasedHandler(winrt::IInspectable const& waiter)
{
    std::erase_if(m_ownerReleasedWaiters, [&waiter](OwnerReleasedWaiter const& entry)
    {
        auto const entryWaiter = entry.Waiter.get();
        return !entryWaiter || entryWaiter == waiter;
    });
}

void TableViewSource2::ReleaseOwningTableView(winrt::IInspectable const& owner)
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

void TableViewSource2::SetLiveShapingHold(std::function<bool()> isHeld)
{
    m_engine->SetLiveShapingHold(std::move(isHeld));
}

void TableViewSource2::ResumeHeldLiveShaping()
{
    m_engine->ResumeHeldLiveShaping();
}

void TableViewSource2::SetLiveItemChangedHook(std::function<bool(winrt::IInspectable const&, bool)> hook)
{
    m_engine->SetLiveItemChangedHook(std::move(hook));
}

void TableViewSource2::SetRowsReleasedHook(std::function<void(winrt::IInspectable const&, std::vector<winrt::IInspectable> const&)> hook)
{
    m_engine->SetRowsReleasedHook(std::move(hook));
}

void TableViewSource2::SetLiveItemReshapeHandler(std::function<void()> handler)
{
    m_engine->SetLiveItemReshapeHandler(std::move(handler));
}

void TableViewSource2::NotifyOwnerProjectionChanged()
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

void TableViewSource2::NotifyOwnerShapingChanged(bool reorderOnly)
{
    if (m_shapingChanged)
    {
        auto const strongThis = get_strong();
        ++m_notificationDepth;
        auto restore = gsl::finally([this]() noexcept { --m_notificationDepth; });
        m_shapingChanged(reorderOnly);
    }
}
