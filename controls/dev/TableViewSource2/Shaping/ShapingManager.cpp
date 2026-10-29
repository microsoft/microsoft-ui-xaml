// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Shaping/ShapingManager.h"

#include <exception>
#include <utility>

// RAII bracket for one commit. EndCommit runs from the destructor on both normal exit and
// unwind; std::uncaught_exceptions tells the two apart.
class ShapingManager::CommitScope
{
public:
    CommitScope(ShapingManager& manager, std::span<SourceRecord const* const> retired)
        : m_manager(manager)
        , m_retired(retired)
        , m_uncaught(std::uncaught_exceptions())
    {
        m_manager.BeginCommit();
    }

    ~CommitScope()
    {
        m_manager.EndCommit(m_retired, std::uncaught_exceptions() > m_uncaught);
    }

    CommitScope(CommitScope const&) = delete;
    CommitScope& operator=(CommitScope const&) = delete;

private:
    ShapingManager& m_manager;
    std::span<SourceRecord const* const> m_retired;
    int m_uncaught{ 0 };
};

ShapingManager::ShapingManager(ITrackerHandleManager const* trackerOwner, winrt::IInspectable const& items)
    : m_trackerOwner(trackerOwner)
    , m_sourceTable(SourceTable::Create(trackerOwner, items))
    , m_delegates(trackerOwner)
    , m_engine(trackerOwner, *m_sourceTable, m_delegates, m_paths)
    , m_rows(m_engine, *m_sourceTable)
{
    try
    {
        m_sourceTable->SetSourceChangedHandler([this](SourceChange const& change)
        {
            OnSourceChanged(change);
        });

        // The initial load's Reset was raised inside Create, before this handler existed. Place
        // the records now; there is no listener or owner yet, so nothing is raised.
        m_engine.Reapply(m_desired);
        m_engine.ReleaseRetired();

        // From here on every commit reaches the listener, which forwards to the owner and the
        // view as each appears (both may be null). An owner may claim before View() is called.
        m_engine.Tree().SetListener(static_cast<IShapedRowsListener*>(this));
    }
    catch (...)
    {
        // The destructor does not run for a throwing constructor; detach while the owner is alive.
        Clear();
        throw;
    }
}

ShapingManager::~ShapingManager()
{
    MUX_ASSERT(m_sourceTable->IsDetached());
    MUX_ASSERT(!m_view && !m_viewImpl && !m_rowMetadata);
}

void ShapingManager::Refresh()
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.Refresh cannot be called while the source is being shaped.");
    }

    if (m_sourceTable->IsDetached())
    {
        return;
    }

    // The re-read's Reset comes back through OnSourceChanged, which opens the commit scope.
    m_sourceTable->Refresh();
}

bool ShapingManager::IsLiveShaping() const
{
    return m_sourceTable->IsLiveShaping();
}

void ShapingManager::IsLiveShaping(bool value)
{
    if (!CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2.IsLiveShaping cannot be set while the source is being shaped.");
    }

    if (!m_sourceTable->IsDetached())
    {
        // Milestone 6 adds the per-item subscriptions behind this flag.
        m_sourceTable->SetLiveShaping(value);
    }
}

ShapeSpec const& ShapingManager::AppliedSpec() const noexcept
{
    return m_engine.Applied();
}

void ShapingManager::SetOwner(IShapingOwner* sink)
{
    // A release always succeeds: the listener re-reads m_owner at each callback, so a release from
    // inside a commit only silences the rest of that commit.
    if (sink && !CanRunVerb())
    {
        throw winrt::hresult_illegal_method_call(L"TableViewSource2 cannot change its owner while the source is being shaped.");
    }

    if (sink == m_owner)
    {
        return;
    }

    // Holds belong to the owner that took them. Milestone 6 posts the live drain here when
    // changes are pending; nothing is pending before live shaping exists.
    m_liveHoldCount = 0;
    m_owner = sink;

    if (sink)
    {
        m_sourceTable->BindOwnerThread();
    }
}

winrt::Microsoft::UI::Xaml::Interop::IBindableVector ShapingManager::View()
{
    if (m_sourceTable->IsDetached())
    {
        return nullptr;
    }

    if (!m_view)
    {
        auto const view = winrt::make_self<ShapedCollectionView>(m_rows);
        m_view.emplace(m_trackerOwner, view.as<winrt::Microsoft::UI::Xaml::Interop::IBindableVector>());
        m_viewImpl = view.get();
    }

    return m_view->get();
}

bool ShapingManager::IsItemInShapedTree(winrt::IInspectable const& item) const
{
    return m_rows.IsItemInShapedTree(item);
}

std::shared_ptr<ITableViewRowMetadataProvider> ShapingManager::RowMetadata()
{
    if (m_sourceTable->IsDetached())
    {
        return nullptr;
    }

    if (!m_rowMetadata)
    {
        m_rowMetadata = std::make_shared<ShapedRowMetadata>(m_rows, static_cast<IShapedExpansion&>(*this));
    }

    return m_rowMetadata;
}

void ShapingManager::Clear()
{
    // No callbacks during teardown; nothing below calls get() on a tracker_ref.
    m_owner = nullptr;
    m_commitEnded = nullptr;
    m_liveHoldCount = 0;
    m_engine.Tree().SetListener(nullptr);

    if (m_viewImpl)
    {
        m_viewImpl->Detach();
    }

    if (m_rowMetadata)
    {
        m_rowMetadata->Detach();
    }

    m_engine.Clear();
    m_sourceTable->Clear();
    m_delegates.Clear();
    m_paths.Clear();

    m_viewImpl = nullptr;
    m_view.reset();
    m_rowMetadata.reset();

    m_liveDirty.clear();
    m_deferDepth = 0;
}

void ShapingManager::OnSourceChanged(SourceChange const& change)
{
    {
        CommitScope scope{ *this, change.Removed };
        if (change.Kind == SourceChangeKind::Reset)
        {
            Reapply();
        }
        else
        {
            m_engine.ApplySourceChange(change);

            // Until Milestone 5 this is a full rebuild: the owner hears Reset | Membership.
            if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
            {
                m_commit.changes |= ShapingChangeFlags::Membership;
            }
        }
    }
    AfterCommit();
}

void ShapingManager::Reapply()
{
    m_liveDirty.clear();
    m_engine.Reapply(m_desired);

    // A Reapply replaces the list: the owner hears Reset | Membership.
    if ((m_commit.changes & ShapingChangeFlags::Reset) == ShapingChangeFlags::Reset)
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }
}

void ShapingManager::OnRowsRemoved(uint32_t index, uint32_t count, bool isMove) noexcept
{
    if (isMove)
    {
        m_commit.changes |= ShapingChangeFlags::Order;
        m_commit.moveNet -= static_cast<int64_t>(count);
    }
    else
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyRemoved(index, count);
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsInserted(uint32_t index, uint32_t count, bool isMove) noexcept
{
    if (isMove)
    {
        m_commit.changes |= ShapingChangeFlags::Order;
        m_commit.moveNet += static_cast<int64_t>(count);
    }
    else
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyAdded(index, count);
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsResetting() noexcept
{
    if (m_commit.listenerFault || !m_owner)
    {
        return;
    }

    try
    {
        m_owner->OnResetting();
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

void ShapingManager::OnRowsReset() noexcept
{
    m_commit.changes |= ShapingChangeFlags::Reset;

    if (m_commit.listenerFault || !m_viewImpl)
    {
        return;
    }

    try
    {
        m_viewImpl->NotifyReset();
    }
    catch (...)
    {
        m_commit.listenerFault = std::current_exception();
    }
}

bool ShapingManager::SetExpanded(ShapedNode* /*node*/, std::optional<bool> /*desired*/)
{
    // Milestone 7. No flat row is expandable, so ShapedRowMetadata never calls this in Milestone 1.
    throw winrt::hresult_not_implemented(L"TableViewSource2: expansion is not implemented yet.");
}

void ShapingManager::SetAllExpanded(ExpansionAxis /*axis*/, bool /*expanded*/)
{
    throw winrt::hresult_not_implemented(L"TableViewSource2: expansion is not implemented yet.");
}

void ShapingManager::ExpandSubtree(ShapedNode* /*node*/)
{
    throw winrt::hresult_not_implemented(L"TableViewSource2: expansion is not implemented yet.");
}

std::optional<uint32_t> ShapingManager::Reveal(ShapedNode* /*node*/)
{
    throw winrt::hresult_not_implemented(L"TableViewSource2: Reveal is not implemented yet.");
}

void ShapingManager::BeginCommit()
{
    MUX_ASSERT(CanRunVerb());
    m_isApplying = true;
    m_sourceTable->BeginFreeze();
    m_commit = {};
}

void ShapingManager::EndCommit(std::span<SourceRecord const* const> retired, bool unwinding) noexcept
{
    if (m_commit.moveNet != 0)
    {
        m_commit.changes |= ShapingChangeFlags::Membership;
    }

    if (m_commit.listenerFault && m_viewImpl)
    {
        // The view and the repeater may disagree after a suppressed event; one Reset realigns them.
        try
        {
            m_viewImpl->NotifyReset();
        }
        catch (...)
        {
        }
        m_commit.changes |= ShapingChangeFlags::Reset;
    }

    if (!unwinding && m_owner && (m_commit.changes != ShapingChangeFlags::None || !retired.empty()))
    {
        try
        {
            m_owner->OnShapingApplied(ShapingAppliedArgs{ m_commit.changes, retired });
        }
        catch (...)
        {
            if (!m_commit.listenerFault)
            {
                m_commit.listenerFault = std::current_exception();
            }
        }
    }

    m_engine.ReleaseRetired();
    bool const changePending = m_sourceTable->EndFreeze();
    m_isApplying = false;

    // From AfterCommit normally; posted when the scope ends by an exception (never from a
    // destructor path).
    m_commit.resyncDue = changePending && !unwinding;

    if (unwinding)
    {
        // The engine's exception propagates; a handler fault recorded on the way is dropped.
        m_commit.listenerFault = nullptr;
        if (changePending)
        {
            ResyncSource(true /* post */);
        }
    }
}

void ShapingManager::AfterCommit()
{
    CommitState commit = std::exchange(m_commit, CommitState{});

    // A handler fault means the owner is in an unknown state: post rather than re-enter it now.
    if (commit.resyncDue)
    {
        ResyncSource(commit.listenerFault != nullptr /* post */);
    }

    std::exception_ptr endedFault;
    if (m_commitEnded)
    {
        try
        {
            m_commitEnded();
        }
        catch (...)
        {
            endedFault = std::current_exception();
        }
    }

    if (commit.listenerFault)
    {
        std::rethrow_exception(commit.listenerFault);
    }

    if (endedFault)
    {
        std::rethrow_exception(endedFault);
    }
}

void ShapingManager::ResyncSource(bool post) noexcept
{
    if (!post)
    {
        try
        {
            // Its Reset comes back through OnSourceChanged; a change during it is posted by Refresh.
            m_sourceTable->Refresh();
            return;
        }
        catch (...)
        {
            // The verb that opened the scope already succeeded; retry once, posted.
        }
    }

    m_sourceTable->PostReread();
}

void ShapingManager::SetCommitEndedHandler(std::function<void()> handler) noexcept
{
    m_commitEnded = std::move(handler);
}

bool ShapingManager::IsApplying() const noexcept
{
    return m_isApplying;
}

bool ShapingManager::CanRunVerb() const noexcept
{
    return !m_isApplying;
}

bool ShapingManager::CanReshape() const noexcept
{
    return m_deferDepth == 0;
}

bool ShapingManager::CanDrain() const noexcept
{
    return !m_isApplying && m_deferDepth == 0 && m_liveHoldCount == 0;
}
