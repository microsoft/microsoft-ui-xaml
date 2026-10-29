// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewSource2.h"

TableViewSource2::TableViewSource2(winrt::IInspectable const& items)
{
    // Builds the SourceTable over items (initial load and validation). A throw here (null or
    // duplicate item, unsupported collection) propagates out of From.
    m_shapingManager = std::make_unique<ShapingManager>(this, items);
}

TableViewSource2::~TableViewSource2()
{
    // No callbacks during teardown: drop the owner and the waiters without running them.
    m_owner = nullptr;
    m_ownerReleasedWaiters.clear();

    // Drops the tracker handle without get().
    m_itemsSourceView.reset();
    m_itemsSourceViewCollection = nullptr;

    if (m_shapingManager)
    {
        // Its tracker_refs are bound to this object, so it is cleared and destroyed while this
        // object is still alive. OPEN (spec): the thread this runs on for a source created off
        // the UI thread.
        m_shapingManager->Clear();
        m_shapingManager.reset();
    }
}

winrt::TableViewSource2 TableViewSource2::From(winrt::IInspectable const& items)
{
    if (!items)
    {
        throw winrt::hresult_invalid_argument(L"items cannot be null.");
    }

    return winrt::make<::TableViewSource2>(items);
}

winrt::TableViewSource2 TableViewSource2::Filter(winrt::TableViewPredicate const& /*predicate*/)
{
    ThrowNotImplemented(L"Filter");
}

winrt::TableViewSource2 TableViewSource2::GroupBy(winrt::TableViewKeySelector const& /*key*/, winrt::SortDirection /*groupDirection*/)
{
    ThrowNotImplemented(L"GroupBy");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::hstring const& /*sortMemberPath*/, winrt::SortDirection /*direction*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewKeySelector const& /*key*/, winrt::SortDirection /*direction*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewComparer2 const& /*comparer*/)
{
    ThrowNotImplemented(L"Sort");
}

winrt::TableViewSource2 TableViewSource2::ParentBy(winrt::TableViewKeySelector const& /*key*/, winrt::TableViewKeySelector const& /*parentKey*/)
{
    ThrowNotImplemented(L"ParentBy");
}

winrt::TableViewSource2 TableViewSource2::ChildrenBy(winrt::TableViewChildrenSelector2 const& /*childrenSelector*/)
{
    ThrowNotImplemented(L"ChildrenBy");
}

winrt::TableViewSource2 TableViewSource2::ClearFilter()
{
    ThrowNotImplemented(L"ClearFilter");
}

winrt::TableViewSource2 TableViewSource2::ClearGroupBy()
{
    ThrowNotImplemented(L"ClearGroupBy");
}

winrt::TableViewSource2 TableViewSource2::ClearSort()
{
    ThrowNotImplemented(L"ClearSort");
}

winrt::TableViewSource2 TableViewSource2::ClearHierarchy()
{
    ThrowNotImplemented(L"ClearHierarchy");
}

void TableViewSource2::Refresh()
{
    if (m_shapingManager)
    {
        m_shapingManager->Refresh();
    }
}

winrt::Windows::Foundation::IClosable TableViewSource2::DeferRefresh()
{
    // Milestone 8.
    ThrowNotImplemented(L"DeferRefresh");
}

bool TableViewSource2::IsLiveShaping() const
{
    return m_shapingManager ? m_shapingManager->IsLiveShaping() : false;
}

void TableViewSource2::IsLiveShaping(bool value)
{
    // Milestone 6 adds the behavior; Milestone 1 stores the flag.
    if (m_shapingManager)
    {
        m_shapingManager->IsLiveShaping(value);
    }
}

ShapeSpec const* TableViewSource2::AppliedSpec() const noexcept
{
    return m_shapingManager ? &m_shapingManager->AppliedSpec() : nullptr;
}

winrt::ItemsSourceView TableViewSource2::GetItemsSourceView()
{
    auto const collection = m_shapingManager ? m_shapingManager->View() : nullptr;
    if (!collection)
    {
        m_itemsSourceView.reset();
        m_itemsSourceViewCollection = nullptr;
        return nullptr;
    }

    void* const identity = winrt::get_abi(collection);
    if (!m_itemsSourceView || m_itemsSourceViewCollection != identity)
    {
        winrt::ItemsSourceView const view{ collection };
        m_itemsSourceView.reset();
        m_itemsSourceView.emplace(this, view);
        m_itemsSourceViewCollection = identity;
    }

    return m_itemsSourceView->get();
}

TableViewRowMetadataProvider TableViewSource2::GetRowMetadata()
{
    return m_shapingManager ? m_shapingManager->RowMetadata() : nullptr;
}

bool TableViewSource2::IsItemInShapedTree(winrt::IInspectable const& item) const
{
    return m_shapingManager ? m_shapingManager->IsItemInShapedTree(item) : false;
}

bool TableViewSource2::TrySetOwner(IShapingOwner* owner)
{
    MUX_ASSERT(owner);
    if (!m_shapingManager || !owner)
    {
        return false;
    }

    if (m_owner == owner)
    {
        return true;
    }

    if (m_owner)
    {
        return false;
    }

    // Throws while an apply runs (#19), before anything here changes. Waiters are not touched:
    // the caller stops awaiting before it claims.
    m_shapingManager->SetOwner(owner);
    m_owner = owner;
    return true;
}

void TableViewSource2::ReleaseOwner(IShapingOwner* owner)
{
    if (!m_shapingManager || !owner || owner != m_owner)
    {
        return;
    }

    m_shapingManager->SetOwner(nullptr);
    m_owner = nullptr;

    // FIFO while nobody owns the source. Pop before running: the handler re-enters TrySetOwner,
    // and one that does not claim is not retried.
    while (!m_owner && !m_ownerReleasedWaiters.empty())
    {
        OwnerReleasedWaiter waiter = std::move(m_ownerReleasedWaiters.front());
        m_ownerReleasedWaiters.erase(m_ownerReleasedWaiters.begin());

        if (waiter.Waiter.get() && waiter.Handler)
        {
            waiter.Handler();
        }
    }
}

void TableViewSource2::AddOwnerReleasedWaiter(winrt::IInspectable const& waiter, std::function<void()> handler)
{
    if (!m_shapingManager || !waiter)
    {
        return;
    }

    RemoveOwnerReleasedWaiter(waiter);
    m_ownerReleasedWaiters.push_back(OwnerReleasedWaiter{ winrt::make_weak(waiter), std::move(handler) });
}

void TableViewSource2::RemoveOwnerReleasedWaiter(winrt::IInspectable const& waiter)
{
    void* const key = waiter ? winrt::get_abi(waiter.as<winrt::Windows::Foundation::IUnknown>()) : nullptr;

    std::erase_if(m_ownerReleasedWaiters, [key](OwnerReleasedWaiter const& entry)
    {
        auto const live = entry.Waiter.get();
        return !live || winrt::get_abi(live.as<winrt::Windows::Foundation::IUnknown>()) == key;
    });
}

void TableViewSource2::ThrowNotImplemented(wchar_t const* verb)
{
    throw winrt::hresult_not_implemented(std::wstring(L"TableViewSource2.").append(verb).append(L" is not implemented yet."));
}
