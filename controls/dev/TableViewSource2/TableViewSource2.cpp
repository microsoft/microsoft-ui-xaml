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

    // Runs waiters a release from inside a commit deferred (their claims would throw there).
    m_shapingManager->SetCommitEndedHandler([this]()
    {
        RunOwnerReleasedWaiters();
    });

    // The posted live drain holds this object strongly while it runs (drift #42). Resolved at
    // post time, never here: the object is not fully constructed yet.
    m_shapingManager->SetWeakOwnerResolver([this]()
    {
        winrt::IInspectable const self = static_cast<winrt::TableViewSource2>(*this);
        return winrt::make_weak(self);
    });
}

TableViewSource2::~TableViewSource2()
{
    // No callbacks during teardown: drop the owner and the waiters without running them.
    m_owner = nullptr;
    m_verbObserver.reset();
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

template <class Apply>
void TableViewSource2::ObserveVerb(ShapingVerbKind kind, bool meaningful, Apply&& apply)
{
    auto const observer = m_verbObserver;
    if (!observer || !m_shapingManager)
    {
        std::forward<Apply>(apply)();
        return;
    }

    // A verb inside an open DeferRefresh only edits the desired spec; an unchanged applied spec
    // means nothing was published (deferred or re-declared), as v1 "published".
    IShapingOwner* const owner = m_owner;
    bool const deferred = m_shapingManager->IsRefreshDeferred();
    ShapeSpec const before = m_shapingManager->AppliedSpec();
    uint64_t const generation = observer->Begin ? observer->Begin(kind, meaningful) : 0;

    ShapingVerbOutcome outcome;
    auto end = wil::scope_exit([&]() noexcept
    {
        outcome.OwnerChanged = m_owner != owner;
        outcome.Committed = outcome.Completed && !deferred && m_shapingManager &&
            !(m_shapingManager->AppliedSpec() == before);
        if (observer->End)
        {
            observer->End(kind, generation, outcome);
        }
    });

    try
    {
        std::forward<Apply>(apply)();
        outcome.Completed = true;
    }
    catch (winrt::hresult_invalid_argument const&)
    {
        outcome.InvalidArgument = true;
        outcome.Error = winrt::to_hresult();
        throw;
    }
    catch (...)
    {
        outcome.Error = winrt::to_hresult();
        throw;
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

winrt::TableViewSource2 TableViewSource2::Filter(winrt::TableViewPredicate const& predicate)
{
    if (!predicate)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.Filter: predicate is required.");
    }

    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Filter, true, [&]() { m_shapingManager->Filter(predicate); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::GroupBy(winrt::TableViewKeySelector const& key, winrt::SortDirection groupDirection)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.GroupBy: key is required.");
    }

    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Group, true, [&]() { m_shapingManager->GroupBy(key, groupDirection); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::hstring const& sortMemberPath, winrt::SortDirection direction)
{
    if (sortMemberPath.empty())
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.Sort: sortMemberPath is required.");
    }

    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Sort, true, [&]() { m_shapingManager->Sort(sortMemberPath, direction); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewKeySelector const& key, winrt::SortDirection direction)
{
    if (!key)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.Sort: key is required.");
    }

    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Sort, true, [&]() { m_shapingManager->Sort(key, direction); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::Sort(winrt::TableViewComparer2 const& comparer)
{
    if (!comparer)
    {
        throw winrt::hresult_invalid_argument(L"TableViewSource2.Sort: comparer is required.");
    }

    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Sort, true, [&]() { m_shapingManager->Sort(comparer); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ParentBy(winrt::TableViewKeySelector const& key, winrt::TableViewKeySelector const& parentKey)
{
    if (m_shapingManager)
    {
        m_shapingManager->ParentBy(key, parentKey);
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ChildrenBy(winrt::TableViewChildrenSelector2 const& childrenSelector)
{
    if (m_shapingManager)
    {
        m_shapingManager->ChildrenBy(childrenSelector);
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ClearFilter()
{
    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Filter, !m_shapingManager->AppliedSpec().Filters.empty(), [&]() { m_shapingManager->ClearFilter(); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ClearGroupBy()
{
    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Group, !m_shapingManager->AppliedSpec().Groups.empty(), [&]() { m_shapingManager->ClearGroupBy(); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ClearSort()
{
    if (m_shapingManager)
    {
        ObserveVerb(ShapingVerbKind::Sort, !m_shapingManager->AppliedSpec().Sorts.empty(), [&]() { m_shapingManager->ClearSort(); });
    }
    return *this;
}

winrt::TableViewSource2 TableViewSource2::ClearHierarchy()
{
    if (m_shapingManager)
    {
        m_shapingManager->ClearHierarchy();
    }
    return *this;
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
    if (!m_shapingManager)
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.DeferRefresh cannot be called after the source is torn down.");
    }

    // Created first, so a failed allocation leaves nothing open.
    auto handle = winrt::make<DeferRefreshHandle>(winrt::make_weak(static_cast<winrt::TableViewSource2>(*this)));
    m_shapingManager->BeginDeferRefresh();
    return handle;
}

void TableViewSource2::EndDeferRefresh()
{
    if (m_shapingManager)
    {
        m_shapingManager->EndDeferRefresh();
    }
}

bool TableViewSource2::IsApplying() const noexcept
{
    return m_shapingManager && m_shapingManager->IsApplying();
}

bool TableViewSource2::IsLiveShaping() const
{
    return m_shapingManager ? m_shapingManager->IsLiveShaping() : false;
}

void TableViewSource2::IsLiveShaping(bool value)
{
    if (m_shapingManager)
    {
        m_shapingManager->IsLiveShaping(value);
    }
}

uint64_t TableViewSource2::BeginLiveShapingHold() noexcept
{
    return m_shapingManager ? m_shapingManager->BeginLiveShapingHold() : 0;
}

void TableViewSource2::EndLiveShapingHold(uint64_t generation)
{
    if (m_shapingManager)
    {
        m_shapingManager->EndLiveShapingHold(generation);
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

    // Never throws, even from inside a commit (an owner callback may let go).
    m_shapingManager->SetOwner(nullptr);
    m_owner = nullptr;
    m_verbObserver.reset();

    // Inside a commit a waiter's claim would throw; the commit-ended handler runs them instead.
    if (!m_shapingManager->IsApplying())
    {
        RunOwnerReleasedWaiters();
    }
}

void TableViewSource2::SetShapingVerbObserver(IShapingOwner* owner, std::shared_ptr<ShapingVerbObserver> observer)
{
    if (!m_shapingManager || !owner || owner != m_owner)
    {
        return;
    }

    m_verbObserver = std::move(observer);
}

void TableViewSource2::RunOwnerReleasedWaiters()
{
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

DeferRefreshHandle::DeferRefreshHandle(winrt::weak_ref<winrt::TableViewSource2> source) noexcept
    : m_source(std::move(source))
{
}

void DeferRefreshHandle::Close()
{
    if (m_closed)
    {
        return;
    }

    auto const source = m_source.get();
    if (!source)
    {
        m_closed = true;
        return;
    }

    auto* const impl = winrt::get_self<::TableViewSource2>(source);
    if (impl->IsApplying())
    {
        // Left open, so a later Close (after the apply) still ends the deferral.
        throw winrt::hresult_illegal_method_call(L"A DeferRefresh handle cannot be closed while the source is being shaped.");
    }

    // Closed before the reshape: an exception from app code leaves the deferral ended (depth
    // released), so a second Close stays a no-op.
    m_closed = true;
    impl->EndDeferRefresh();
}
