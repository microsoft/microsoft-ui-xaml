// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "RowMetadataProvider.h"
#include "TableViewGroupHeader.h"
#include "TableViewGroupInfo.h"
#include "TableViewGroupingHelpers.h"
#include "TableViewGroupExpandingEventArgs.h"
#include "TableViewGroupCollapsedEventArgs.h"
#include "ShapingHelpers.h"
#include "ResourceAccessor.h"
#include "TVDiag.h"

#include <algorithm>

namespace
{
    // Holds a DeferRefresh on the view across GroupExpanding and the state change, so a GroupExpanding
    // handler that changes the source (a lazy load) regroups once, after the change.
    class GroupExpansionDeferral
    {
    public:
        explicit GroupExpansionDeferral(winrt::Microsoft::UI::Xaml::Data::ICollectionViewShaping const& shaping) noexcept
        {
            if (shaping)
            {
                try
                {
                    m_deferral = shaping.DeferRefresh();
                }
                catch (...)
                {
                }
            }
        }
        ~GroupExpansionDeferral() { End(); }
        GroupExpansionDeferral(GroupExpansionDeferral const&) = delete;
        GroupExpansionDeferral& operator=(GroupExpansionDeferral const&) = delete;

        // Ends the deferral; the view's coalesced recompute runs here and a failure in it is contained.
        void End() noexcept
        {
            if (auto const deferral = std::exchange(m_deferral, nullptr))
            {
                try
                {
                    deferral.Close();
                }
                catch (...)
                {
                    TVDiag::LogRetailF(L"[TableView] The view's recompute deferred across a group expansion threw (HRESULT 0x%08X).",
                        static_cast<unsigned int>(winrt::to_hresult()));
                }
            }
        }

    private:
        winrt::Windows::Foundation::IClosable m_deferral{ nullptr };
    };

    // Every step can fail on a locale-starved or self-contained host, and this runs during
    // measure, so nothing is allowed to escape.
    winrt::hstring LocalizedOrFallback(std::wstring_view resourceName, std::wstring_view fallback) noexcept
    {
        try
        {
            if (auto const resolved = ResourceAccessor::GetLocalizedStringResource(resourceName); !resolved.empty())
            {
                return resolved;
            }
        }
        catch (...)
        {
        }

        return winrt::hstring{ fallback };
    }
}

// ---------------------------------------------------------------------------------------------
// Grouped-projection rendering.
//
// A grouped view is projected as group headers and data rows in one flat row list, so the
// repeater realizes two container types from one source. TableViewRowTemplateSelector picks
// between them using GetRowKindForItem; everything a group header needs that a TableViewRow would
// otherwise have pushed during cell rebuild is pushed from PrepareGroupHeaderElement instead.
// ---------------------------------------------------------------------------------------------

void TableView::SetGroupAutomationElementInternal(winrt::hstring const& identity, winrt::AutomationPeer const& element)
{
    if (m_groupAutomationElements.size() >= 256)
    {
        // Prune only unshown groups, so a shown group keeps the element clients hold.
        std::erase_if(m_groupAutomationElements, [](auto const& entry) { return !TryGetBoundContainer(entry.second); });
    }
    m_groupAutomationElements.insert_or_assign(identity, element);
}

// Single mapping from a row-source item to its kind. Deliberately item-based rather than
// index-based: ElementFactoryGetArgs carries no index, and keeping one lookup here means a future
// source kind adds its entry type in this function instead of adding a parallel path in the
// factory.
TableViewRowKind TableView::GetRowKindForItem(winrt::IInspectable const& item) const
{
    if (!item)
    {
        return TableViewRowKind::Data;
    }

    // Only grouped rows include GroupedEntry rows, and only for headers. A flat
    // source hands the app's item straight through, and an app item must never be shaped into a
    // group header just because it happens to satisfy the probe.
    if (!IsGroupedRowSource())
    {
        return TableViewRowKind::Data;
    }

    return TryGetGroupedEntry(item) ? TableViewRowKind::GroupHeader : TableViewRowKind::Data;
}

bool TableView::TryGetRowInfo(int32_t rowIndex, TableViewRowInfo& rowInfo) const
{
    if (!m_tableViewSourceRowMetadata || rowIndex < 0)
    {
        return false;
    }

    try
    {
        rowInfo = m_tableViewSourceRowMetadata->GetRowInfo(rowIndex);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool TableView::IsGroupedRowSource() const
{
    return m_rowFlattener && m_rowFlattener->IsGrouped();
}

bool TableView::IsGroupHeaderRow(int32_t index) const
{
    TableViewRowInfo rowInfo{};
    return TryGetRowInfo(index, rowInfo) && rowInfo.Kind == TableViewRowKind::GroupHeader;
}

int32_t TableView::GetDataRowPositionInSetInternal(int32_t rowIndex) const
{
    if (rowIndex < 0)
    {
        return 0;
    }

    if (!IsGroupedRowSource())
    {
        return rowIndex + 1;
    }

    int32_t position = 0;
    for (int32_t index = 0; index <= rowIndex; ++index)
    {
        TableViewRowInfo rowInfo{};
        if (!TryGetRowInfo(index, rowInfo))
        {
            return rowIndex + 1;
        }

        if (rowInfo.Kind == TableViewRowKind::Data)
        {
            ++position;
        }
    }

    return position;
}

int32_t TableView::GetDataRowSizeOfSetInternal() const
{
    const auto rowCount = GetRowCountInternal();
    if (!IsGroupedRowSource())
    {
        return rowCount;
    }

    int32_t dataRowCount = 0;
    for (int32_t index = 0; index < rowCount; ++index)
    {
        TableViewRowInfo rowInfo{};
        if (!TryGetRowInfo(index, rowInfo))
        {
            return rowCount;
        }

        if (rowInfo.Kind == TableViewRowKind::Data)
        {
            ++dataRowCount;
        }
    }

    return dataRowCount;
}

int32_t TableView::GetGroupedRowAutomationLevelInternal(int32_t rowIndex) const
{
    // Single-level groups report no Level (owner decision, dev-spec Accessibility); nested groups need it to tell depths apart.
    TableViewRowInfo rowInfo{};
    if (!m_rowFlattener || !m_rowFlattener->HasSubgroups() || !TryGetRowInfo(rowIndex, rowInfo))
    {
        return 0;
    }
    return std::max(0, rowInfo.Level) + 1;
}

// ---------------------------------------------------------------------------------------------
// Expansion
// ---------------------------------------------------------------------------------------------

void TableView::ToggleGroupExpansion(winrt::UIElement const& container)
{
    RequestGroupExpansion(container, std::nullopt);
}

// Directional request from ExpandCollapsePattern.
void TableView::SetGroupExpansion(winrt::UIElement const& container, bool expand)
{
    RequestGroupExpansion(container, expand);
}

void TableView::RequestGroupExpansion(winrt::UIElement const& container, std::optional<bool> desired)
{
    auto const header = container.try_as<winrt::TableViewGroupHeader>();
    if (!header)
    {
        return;
    }

    // Consume the gesture state first so a request that goes nowhere can't leak it to a later one.
    const winrt::FocusState gestureState = winrt::get_self<TableViewGroupHeader>(header)->TakeGestureFocusStateInternal();

    if (!m_tableViewSourceRowMetadata)
    {
        return;
    }

    // Resolve identity NOW, while this container's index is still current. The mutation below is
    // deferred, and the index is not stable across that gap: a scroll can recycle this container
    // onto a different group, or out of view entirely (GetElementIndex returns -1, silently
    // dropping the request). Identity is index-independent once captured.
    winrt::hstring const identity = TryGetContainerIdentity(container);

    // Capture focus while the container still owns it; the deferred reshape recycles it.
    CaptureContainerFocusForRestore(container, identity, gestureState);

    if (!identity.empty())
    {
        TableViewTelemetry::MarkUsed(m_telemetrySession, m_telemetrySession.generation, TableViewTelemetry::Feature::Grouping);
    }

    QueueGroupExpansionByIdentity(identity, desired);
}

winrt::hstring TableView::TryGetContainerIdentity(winrt::UIElement const& container)
{
    if (!m_tableViewSourceRowMetadata || !container)
    {
        return {};
    }

    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return {};
    }

    const auto rowIndex = repeater.GetElementIndex(container);
    if (rowIndex < 0)
    {
        return {};
    }

    try
    {
        return m_tableViewSourceRowMetadata->GetIdentity(rowIndex);
    }
    catch (...)
    {
        return {};
    }
}

void TableView::CaptureContainerFocusForRestore(
    winrt::UIElement const& container,
    winrt::hstring const& identity,
    winrt::FocusState gestureState)
{
    // Clear any prior capture: a fresh toggle supersedes an earlier one whose restore has not run.
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;
    m_pendingGroupFocusFromPointerGesture = false;

    if (identity.empty() || !container)
    {
        return;
    }

    auto const root = XamlRoot();
    if (!root)
    {
        return;
    }

    // Only restore when THIS header container (or a descendant) currently holds focus: that is the
    // gesture the reshape is about to strand. A UIA Expand()/Collapse() or a programmatic toggle on
    // an unfocused group must not yank focus across the table.
    auto focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    bool focusInContainer = false;
    for (winrt::DependencyObject node = focused; node; node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (node == container)
        {
            focusInContainer = true;
            break;
        }
    }
    if (!focusInContainer)
    {
        return;
    }

    if (auto const control = container.try_as<winrt::Control>())
    {
        // Pointer focus is restored too: a band click focuses the header, then collapses it.
        const auto state = gestureState != winrt::FocusState::Unfocused ? gestureState : control.FocusState();
        if (state != winrt::FocusState::Unfocused)
        {
            m_pendingGroupFocusIdentity = identity;
            m_pendingGroupFocusState = state;
            m_pendingGroupFocusFromPointerGesture = gestureState == winrt::FocusState::Pointer;
        }
    }
}

bool TableView::CanRestoreContainerFocus()
{
    auto const root = XamlRoot();
    if (!root || !root.IsHostVisible())
    {
        return false;
    }

    // ItemsRepeater moves focus off a cleared container to a realized neighbour, still inside.
    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    if (!focused)
    {
        return true;
    }

    winrt::DependencyObject const selfObject = *this;
    return focused == selfObject || SharedHelpers::IsAncestor(focused, selfObject, false /* checkVisibility */);
}

winrt::hstring TableView::CaptureFocusedContainerForRestore()
{
    // A bulk expand/collapse supersedes any earlier capture whose restore has not run, exactly as
    // a fresh single-group toggle does.
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;
    m_pendingGroupFocusFromPointerGesture = false;

    auto const root = XamlRoot();
    if (!root)
    {
        return {};
    }

    // Walk out from the focused element to its header container, if it is inside one. Row focus is
    // left alone.
    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    for (winrt::DependencyObject node = focused; node; node = winrt::VisualTreeHelper::GetParent(node))
    {
        if (auto const header = node.try_as<winrt::TableViewGroupHeader>())
        {
            auto const identity = TryGetContainerIdentity(header);
            CaptureContainerFocusForRestore(header, identity);
            return identity;
        }
        if (node.try_as<winrt::TableViewRow>())
        {
            break;
        }
    }

    return {};
}

// Directional request from a UIA provider or the band gesture. Identity is resolved here, while
// the container's index is still current, because the mutation below is deferred.
void TableView::QueueGroupExpansionByIdentity(
    winrt::hstring const& identity,
    std::optional<bool> desired)
{
    if (identity.empty())
    {
        return;
    }

    // Capture the intent, not just the target. A directionless toggle applied on a later turn is
    // not idempotent: two Expand() calls would expand then collapse. Carrying `desired` through
    // makes the deferred half an idempotent set.
    const auto generation = m_rowMetadataGeneration;

    // A bulk expand/collapse requested after this supersedes it (see ApplyGroupExpansionByIdentity).
    const auto bulkGeneration = m_groupBulkExpansionGeneration;

    // Defer the structural mutation off the current callout. Running it inline re-projects rows
    // while the caller's frame is still on the stack, which faults on the pointer path and
    // surfaces to a UIA client as an exception escaping the COM boundary. Both callers are safe
    // once the mutation happens on a later turn.
    auto weakThis = get_weak();
    if (auto const queue = DispatcherQueue())
    {
        // A refused enqueue means the thread is shutting down and the UI is going away regardless.
        queue.TryEnqueue([weakThis, identity, desired, generation, bulkGeneration]()
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->ApplyGroupExpansionByIdentity(identity, desired, generation, bulkGeneration);
                }
            });
        return;
    }

    // No dispatcher means this is not a UI thread (test host), so there is no in-flight callout to
    // unwind and the hazard above cannot apply.
    ApplyGroupExpansionByIdentity(identity, desired, generation, bulkGeneration);
}

void TableView::ApplyGroupExpansionByIdentity(
    winrt::hstring const& identity,
    std::optional<bool> desired,
    uint64_t generation,
    uint64_t bulkGeneration)
{
    // Identities are value-based strings, not tied to a provider instance. If ItemsSource was
    // replaced while this request sat on the queue, the same string could name an unrelated group
    // in the new source -- exactly the wrong-group mutation this path exists to prevent.
    if (generation != m_rowMetadataGeneration)
    {
        return;
    }

    // A bulk expand/collapse was requested after this one. It is the later intent and clears
    // per-group exceptions, so applying this now would resurrect one; drop it.
    if (bulkGeneration != m_groupBulkExpansionGeneration)
    {
        return;
    }

    if (!m_tableViewSourceRowMetadata || identity.empty())
    {
        return;
    }

    // A GroupExpanding handler asking for its own group again: the outer request applies it.
    if (IsRaisingGroupExpanding(identity))
    {
        return;
    }

    bool changed = false;
    // Focus restore still runs if anything below throws.
    bool finished = false;
    auto finishOnThrow = wil::scope_exit([this, &finished, &identity]() noexcept
    {
        if (finished) { return; }
        try { RestoreContainerFocusIfPending(identity); } catch (...) {}
    });

    // Captured before the change: GroupExpanding precedes it and GroupCollapsed reports it.
    std::optional<TableViewGroupExpansionInfo> changingGroup;
    // Local strong ref: a handler may re-declare the shape and replace the provider mid-call, so re-check the source
    // before mutating.
    auto provider = m_tableViewSourceRowMetadata;
    bool targetResolves = provider && generation == m_rowMetadataGeneration;

    std::optional<winrt::ExpandCollapseState> groupStateBefore;
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        groupStateBefore = TryGetGroupExpandCollapseState(identity);
    }

    // A GroupExpanding handler that changes the source (a lazy load, a Filter) is coalesced into one
    // reshape after the state change, so the expansion is not lost to it.
    GroupExpansionDeferral deferral{ m_collectionViewShaping };
    if (targetResolves)
    {
        TableViewGroupExpansionInfo info{};
        if (provider->TryGetGroupExpansionInfo(identity, info) && desired.value_or(!info.IsExpanded) != info.IsExpanded)
        {
            changingGroup = info;
            if (!info.IsExpanded)
            {
                // Pinned: once GroupExpanding is raised this request expands, never toggles.
                desired = true;
                RaiseGroupExpandingFor(identity, info.Key);

                // The handler may have replaced the source or expanded the group itself.
                provider = m_tableViewSourceRowMetadata;
                TableViewGroupExpansionInfo current{};
                targetResolves = provider && generation == m_rowMetadataGeneration &&
                    provider->TryGetGroupExpansionInfo(identity, current) && !current.IsExpanded;
                if (!targetResolves)
                {
                    changingGroup.reset();
                }
            }
        }
    }
    if (targetResolves)
    {
        // While actions are tracked, every request is, so a failure writes TableView_Error. A request that changes one
        // group also runs a TiP test; a request that changes nothing runs none: each arrow on a group header is
        // requested by the header and again by TryHandleGroupHeaderExpandCollapseKey. The lookup runs only while
        // actions are tracked.
        auto const withTest = TracksTipActions() && provider->WouldChangeExpansion(identity, desired);
        ActionTip tip{ *this, TableViewTip::Operation::GroupExpansion, TableViewTip::Stage::Expansion, true, withTest };
        try
        {
            if (desired.has_value())
            {
                // Idempotent set: applying the state we already have is a no-op, and reports no change.
                changed = *desired
                    ? provider->Expand(identity)
                    : provider->Collapse(identity);
            }
            else
            {
                changed = provider->Toggle(identity);
            }

            if (generation != m_rowMetadataGeneration) { tip.Ignore(TableViewTip::IgnoreReason::Stale); }
            else if (changed) { tip.Complete(); }
            // No change shown yet. A tested request expected one; an untested one closes without a TiP result.
            else { tip.Ignore(TableViewTip::IgnoreReason::DeferredOrUnproven); }
        }
        catch (...)
        {
            // Best-effort: the grouping source or its state can change during cleanup. Expansion reads only the
            // projection TableView built, so a failure is TableView's unless the source was replaced or the exception
            // is tagged as the app's.
            auto const fault = TableViewTip::CurrentFault();
            if (generation != m_rowMetadataGeneration) { tip.Ignore(TableViewTip::IgnoreReason::Stale); }
            else if (fault.appFault != TableViewTip::AppFaultKind::None) { tip.ApplicationFault(fault.appFault, fault.error); }
            else { tip.Fail(fault.error); }
        }
    }

    deferral.End();

    // Announce only once the tree has actually moved. Raising while the mutation was still queued
    // told clients to re-read a structure that had not changed yet.
    if (changed)
    {
        if (groupStateBefore)
        {
            // Keep the earliest state: two toggles before the header is re-prepared net out to no change.
            m_pendingGroupExpansionStates.try_emplace(identity, *groupStateBefore);
        }
        QueueGroupExpansionRowRefresh();
        RaiseGroupStructureChanged();
    }

    // Restore keyboard focus to the toggled header or row even when nothing structurally changed: a
    // non-expandable/no-op toggle still ran the container through the reshape path, and leaving
    // focus stranded is the very bug this guards. Matches on identity, so an unrelated queued
    // toggle does not consume this restore.
    RestoreContainerFocusIfPending(identity);
    finished = true;

    if (changed && changingGroup && changingGroup->IsExpanded)
    {
        RaiseGroupCollapsed(changingGroup->Key);
    }
}

static winrt::ExpandCollapseState GroupExpandCollapseState(bool isExpandable, bool isExpanded)
{
    return !isExpandable ? winrt::ExpandCollapseState::LeafNode
        : isExpanded ? winrt::ExpandCollapseState::Expanded
        : winrt::ExpandCollapseState::Collapsed;
}

std::optional<winrt::ExpandCollapseState> TableView::TryGetGroupExpandCollapseState(winrt::hstring const& identity)
{
    try
    {
        int32_t index = -1;
        TableViewRowInfo info{};
        if (m_tableViewSourceRowMetadata &&
            m_tableViewSourceRowMetadata->TryGetIndexForIdentity(identity, index) &&
            TryGetRowInfo(index, info) && info.Kind == TableViewRowKind::GroupHeader)
        {
            return GroupExpandCollapseState(info.IsExpandable, info.IsExpanded);
        }
    }
    catch (...)
    {
    }
    return std::nullopt;
}

void TableView::RestoreContainerFocusIfPending(winrt::hstring const& identity)
{
    if (m_pendingGroupFocusIdentity.empty() || m_pendingGroupFocusIdentity != identity)
    {
        return;
    }

    const auto focusState = m_pendingGroupFocusState;
    const bool fromPointerGesture = m_pendingGroupFocusFromPointerGesture;
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;
    m_pendingGroupFocusFromPointerGesture = false;

    // Defer to after the reshape's relayout. The reprojection triggered by the toggle recycles the
    // header container during the next layout pass; focusing now would land on a container layout
    // is about to recycle, dropping focus a second time. A one-shot LayoutUpdated fires once the
    // tree has settled.
    ArmContainerFocusRestore(identity, focusState, fromPointerGesture, s_containerFocusRestoreAttempts);
}

// Re-armed while the header is still unarranged after the relayout; the last attempt focuses anyway.
void TableView::ArmContainerFocusRestore(
    winrt::hstring const& identity, winrt::FocusState focusState, bool fromPointerGesture, int32_t attemptsLeft)
{
    if (m_pendingGroupFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupFocusLayoutToken);
        m_pendingGroupFocusLayoutToken = {};
    }

    auto weakThis = get_weak();
    m_pendingGroupFocusLayoutToken = LayoutUpdated(
        [weakThis, identity, focusState, fromPointerGesture, attemptsLeft](winrt::IInspectable const&, winrt::IInspectable const&)
        {
            auto strongThis = weakThis.get();
            if (!strongThis)
            {
                return;
            }

            if (strongThis->m_pendingGroupFocusLayoutToken.value)
            {
                strongThis->LayoutUpdated(strongThis->m_pendingGroupFocusLayoutToken);
                strongThis->m_pendingGroupFocusLayoutToken = {};
            }

            // This runs from a LAYOUT callback, where an escaping exception reaches no app handler
            // and fails the process fast instead. FocusContainerByIdentity realizes a container
            // and moves focus, both of which can throw when the projection has moved underneath a
            // deferred restore.
            try
            {
                if (!strongThis->CanRestoreContainerFocus())
                {
                    return;
                }

                const bool finalAttempt = attemptsLeft <= 1;
                if (!strongThis->FocusContainerByIdentity(identity, focusState, fromPointerGesture, finalAttempt))
                {
                    strongThis->ArmContainerFocusRestore(identity, focusState, fromPointerGesture, attemptsLeft - 1);
                }
            }
            catch (...)
            {
                // Best-effort focus restore: the header or row can be gone by the time layout settles.
            }
        });
}

// Returns false only when a non-final attempt found the group header not yet arranged.
bool TableView::FocusContainerByIdentity(
    winrt::hstring const& identity, winrt::FocusState focusState, bool fromPointerGesture, bool finalAttempt)
{
    if (identity.empty() || !m_tableViewSourceRowMetadata)
    {
        return true;
    }

    int32_t index = -1;
    if (!m_tableViewSourceRowMetadata->TryGetIndexForIdentity(identity, index) || index < 0)
    {
        return true;
    }

    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return true;
    }

    // The metadata provider and repeater own separate views that can absorb reshapes at different
    // times, so a resolved index can exceed what the repeater accepts. GetOrCreateElement throws
    // for that, and this deferred layout callback would fail-fast the process.
    auto const sourceView = repeater.ItemsSourceView();
    if (!sourceView || index >= sourceView.Count())
    {
        return true;
    }

    auto element = repeater.GetOrCreateElement(index);
    if (!element)
    {
        return true;
    }

    // Only a group header is a valid target: after a re-sort or source swap the identity's index
    // could now name a data row, and focusing that with a header's intent would be wrong.
    if (!element.try_as<winrt::TableViewGroupHeader>())
    {
        return true;
    }

    if (!IsRowElementArranged(element))
    {
        // The layout can re-project the source; the header must still be this group's, at index.
        auto const expectedItem = GetRowsSourceItemAt(index);
        int32_t revalidatedIndex = -1;
        const bool sameHeader = ArrangeAndRevalidate(element, index, expectedItem) &&
            m_tableViewSourceRowMetadata &&
            m_tableViewSourceRowMetadata->TryGetIndexForIdentity(identity, revalidatedIndex) &&
            revalidatedIndex == index &&
            element.try_as<winrt::TableViewGroupHeader>();
        if (!sameHeader)
        {
            if (!finalAttempt)
            {
                return false;
            }
            if (!fromPointerGesture && expectedItem)
            {
                BeginPendingBodyFocus(index, m_currentCellColumn, false /* cellLevel */, std::nullopt, expectedItem);
                m_pendingReshapeFocusState = focusState == winrt::FocusState::Unfocused ? winrt::FocusState::Keyboard : focusState;
            }
            return true;
        }
        if (!IsRowElementArranged(element) && !finalAttempt)
        {
            return false;
        }
    }

    if (auto const control = element.try_as<winrt::Control>())
    {
        control.Focus(focusState);
    }

    // The body scrolls itself to the header; finish through the attempt loop if the header still
    // sits at an estimated position outside the viewport.
    if (!fromPointerGesture && !IsRowElementInViewport(element))
    {
        BringRowElementIntoView(element, std::nullopt);
        BeginPendingBodyFocus(index, m_currentCellColumn, false /* cellLevel */, std::nullopt);
        m_pendingReshapeFocusState = focusState == winrt::FocusState::Unfocused ? winrt::FocusState::Keyboard : focusState;
    }
    return true;
}

void TableView::ExpandAllGroups()
{
    SetBulkExpansion(true);
}

void TableView::CollapseAllGroups()
{
    SetBulkExpansion(false);
}

// Bulk counterpart of ApplyGroupExpansionByIdentity. Runs inline (no UIA callout).
void TableView::SetBulkExpansion(bool expand)
{
    // A documented no-op when the rows are not grouped.
    if (!m_tableViewSourceRowMetadata || !IsGroupedRowSource())
    {
        return;
    }

    // The group events must not run app code inside measure or arrange, and a bulk request from a
    // GroupExpanding handler waits for the request that raised it.
    if (IsInLayoutCallback() || !m_groupsRaisingExpanding.empty())
    {
        if (auto const queue = DispatcherQueue())
        {
            if (queue.TryEnqueue([weakThis = get_weak(), expand]()
                {
                    if (auto const strongThis = weakThis.get())
                    {
                        strongThis->SetBulkExpansion(expand);
                    }
                }))
            {
                return;
            }
        }
    }

    // Stamped at the request, so a per-group request made after this one - even while this one is
    // deferred - is not mistaken for an older intent.
    ++m_groupBulkExpansionGeneration;

    ApplyBulkExpansion(expand);
}

void TableView::ApplyBulkExpansion(bool expand)
{
    if (!m_tableViewSourceRowMetadata || !IsGroupedRowSource())
    {
        return;
    }
    const uint64_t generation = m_rowMetadataGeneration;

    auto const focusedGroupIdentity = CaptureFocusedContainerForRestore();

    bool finished = false;
    auto finishOnThrow = wil::scope_exit([this, &finished, &focusedGroupIdentity]() noexcept
    {
        if (finished) { return; }
        try { RestoreContainerFocusIfPending(focusedGroupIdentity); } catch (...) {}
    });

    // GroupExpanding handlers that change the source are coalesced into one reshape after the change.
    GroupExpansionDeferral deferral{ m_collectionViewShaping };

    // The groups this command changes, for GroupExpanding before it and GroupCollapsed after.
    std::vector<TableViewGroupExpansionInfo> changingGroups;
    if (m_groupExpandingEventSource || m_groupCollapsedEventSource)
    {
        if (auto const provider = m_tableViewSourceRowMetadata)
        {
            try
            {
                for (auto&& info : provider->GetGroupExpansionInfos())
                {
                    if (info.IsExpanded != expand)
                    {
                        changingGroups.push_back(std::move(info));
                    }
                }
            }
            catch (...)
            {
                changingGroups.clear();
            }
        }
        if (expand)
        {
            // Each group is checked again after the previous handler, which may have changed it.
            std::vector<TableViewGroupExpansionInfo> raised;
            for (auto const& info : changingGroups)
            {
                auto const provider = m_tableViewSourceRowMetadata;
                TableViewGroupExpansionInfo current{};
                if (!provider || generation != m_rowMetadataGeneration ||
                    !provider->TryGetGroupExpansionInfo(info.Identity, current) || current.IsExpanded)
                {
                    continue;
                }
                RaiseGroupExpandingFor(info.Identity, current.Key);
                raised.push_back(current);
            }
            changingGroups = std::move(raised);
        }
    }

    bool changed = false;
    try
    {
        // Local strong ref, as in ApplyGroupExpansionByIdentity; GroupExpanding handlers may have
        // replaced or cleared the source.
        auto const provider = m_tableViewSourceRowMetadata;
        if (provider && generation == m_rowMetadataGeneration)
        {
            expand
                ? provider->ExpandAllGroups()
                : provider->CollapseAllGroups();
            changed = true;
        }
    }
    catch (...)
    {
    }

    deferral.End();

    if (changed)
    {
        QueueGroupExpansionRowRefresh();
        RaiseGroupStructureChanged();
    }

    RestoreContainerFocusIfPending(focusedGroupIdentity);
    finished = true;

    if (changed && !expand)
    {
        for (auto const& info : changingGroups)
        {
            RaiseGroupCollapsed(info.Key);
        }
    }
}

// The group identity the key maps to (its value identity), then the live group with that identity:
// O(groups).
void TableView::SetGroupExpansionByKey(winrt::IInspectable const& groupKey, bool expand)
{
    auto const provider = m_tableViewSourceRowMetadata;
    auto const flattener = m_rowFlattener;
    if (!provider || !flattener || !IsGroupedRowSource())
    {
        return;
    }

    if (IsInLayoutCallback())
    {
        if (auto const queue = DispatcherQueue())
        {
            if (queue.TryEnqueue([weakThis = get_weak(), groupKey, expand]()
                {
                    if (auto const strongThis = weakThis.get())
                    {
                        strongThis->SetGroupExpansionByKey(groupKey, expand);
                    }
                }))
            {
                return;
            }
        }
    }

    auto const groupIdentity = flattener->TryGetGroupIdentityForKey(groupKey);
    if (!groupIdentity)
    {
        return;
    }

    winrt::hstring expansionIdentity;
    for (auto const& group : flattener->CurrentGroups())
    {
        if (group.Identity == *groupIdentity)
        {
            expansionIdentity = provider->GetGroupExpansionIdentity(group.Identity);
            break;
        }
    }

    TableViewGroupExpansionInfo info{};
    if (expansionIdentity.empty() || !provider->TryGetGroupExpansionInfo(expansionIdentity, info) || info.IsExpanded == expand)
    {
        return;
    }

    ApplyGroupExpansionByIdentity(expansionIdentity, expand, m_rowMetadataGeneration, m_groupBulkExpansionGeneration);
}

void TableView::ExpandGroup(winrt::IInspectable const& groupKey)
{
    SetGroupExpansionByKey(groupKey, true);
}

void TableView::CollapseGroup(winrt::IInspectable const& groupKey)
{
    SetGroupExpansionByKey(groupKey, false);
}

// Keyed with the metadata generation: the same identity in a source swapped in by the handler is
// another group, and a request for it goes ahead.
bool TableView::IsRaisingGroupExpanding(winrt::hstring const& identity) const
{
    return std::find(m_groupsRaisingExpanding.begin(), m_groupsRaisingExpanding.end(),
        std::make_pair(identity, m_rowMetadataGeneration)) != m_groupsRaisingExpanding.end();
}

void TableView::RaiseGroupExpandingFor(winrt::hstring const& identity, winrt::IInspectable const& groupKey)
{
    m_groupsRaisingExpanding.emplace_back(identity, m_rowMetadataGeneration);
    auto const pop = wil::scope_exit([this]() noexcept { m_groupsRaisingExpanding.pop_back(); });
    RaiseGroupExpanding(groupKey);
}

// A handler's exception propagates to the caller, as Sorting's and Sorted's do. A request posted to the DispatcherQueue
// (from layout or UI Automation) has no caller: the exception leaves the dispatcher callback and ends the app, as any
// DispatcherQueue callback's does (it does not reach Application.UnhandledException). GroupExpanding is raised before the change, so a throwing handler stops
// the expansion; the callers' scope guards restore focus and end the deferral.
void TableView::RaiseGroupExpanding(winrt::IInspectable const& groupKey)
{
    if (!m_groupExpandingEventSource)
    {
        return;
    }
    auto const args = winrt::make<TableViewGroupExpandingEventArgs>(groupKey);
    try { m_groupExpandingEventSource(*this, args); }
    catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
}

void TableView::RaiseGroupCollapsed(winrt::IInspectable const& groupKey)
{
    if (!m_groupCollapsedEventSource)
    {
        return;
    }
    // Raised after the change, which a throwing handler does not undo.
    auto const args = winrt::make<TableViewGroupCollapsedEventArgs>(groupKey);
    try { m_groupCollapsedEventSource(*this, args); }
    catch (...) { TableViewTip::RethrowAppFault(TableViewTip::AppFaultKind::Callback); }
}

void TableView::RaiseGroupStructureChanged()
{
    QueueTerminalGridLineRefresh();

    if (!winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::StructureChanged))
    {
        return;
    }

    if (auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::TableViewAutomationPeer>())
    {
        winrt::get_self<TableViewAutomationPeer>(peer)->RaiseStructureChangedForGroupExpansion();
    }
}

// ---------------------------------------------------------------------------------------------
// Group-key text
// ---------------------------------------------------------------------------------------------

winrt::DecimalFormatter TableView::GetGroupKeyDecimalFormatter()
{
    if (!m_groupKeyDecimalFormatter)
    {
        try
        {
            m_groupKeyDecimalFormatter = TableViewDetails::CreateCurrentCultureDecimalFormatter();
            if (m_groupKeyDecimalFormatter)
            {
                m_groupKeyDefaultFractionDigits = m_groupKeyDecimalFormatter.FractionDigits();
            }
        }
        catch (...)
        {
            m_groupKeyDecimalFormatter = nullptr;
            m_groupKeyDefaultFractionDigits = 0;
        }
    }

    return m_groupKeyDecimalFormatter;
}

winrt::hstring TableView::StringifyGroupKey(winrt::IInspectable const& key)
{
    if (!key)
    {
        return LocalizedOrFallback(SR_TableViewGroupHeaderNull, L"(null)");
    }

    if (auto const stringable = key.try_as<winrt::IStringable>())
    {
        try
        {
            return stringable.ToString();
        }
        catch (...)
        {
            // App-supplied ToString may throw; fall through to the remaining strategies rather
            // than letting it escape into the repeater's element-prepared callback.
        }
    }

    if (auto const propertyValue = key.try_as<winrt::IPropertyValue>())
    {
        auto const formatter = GetGroupKeyDecimalFormatter();
        try
        {
            switch (propertyValue.Type())
            {
            case winrt::PropertyType::String:
                return propertyValue.GetString();
            case winrt::PropertyType::Int32:
                if (formatter) { formatter.FractionDigits(0); return formatter.FormatInt(propertyValue.GetInt32()); }
                break;
            case winrt::PropertyType::Int64:
                if (formatter) { formatter.FractionDigits(0); return formatter.FormatInt(propertyValue.GetInt64()); }
                break;
            case winrt::PropertyType::UInt32:
                if (formatter) { formatter.FractionDigits(0); return formatter.FormatUInt(propertyValue.GetUInt32()); }
                break;
            case winrt::PropertyType::Double:
                if (formatter) { formatter.FractionDigits(m_groupKeyDefaultFractionDigits); return formatter.FormatDouble(propertyValue.GetDouble()); }
                break;
            default:
                break;
            }
        }
        catch (...)
        {
        }
    }

    // A C# IGrouping<TKey,TItem>.Key projects through ICustomPropertyProvider. GetValue runs
    // consumer reflection and can throw during measure -- degrade to the fallback label.
    if (auto const customProperties = key.try_as<winrt::ICustomPropertyProvider>())
    {
        try
        {
            if (auto const keyProperty = customProperties.GetCustomProperty(L"Key"))
            {
                return StringifyGroupKey(keyProperty.GetValue(key));
            }
        }
        catch (...)
        {
        }
    }

    return LocalizedOrFallback(SR_TableViewGroupHeaderFallback, L"(group)");
}

// ---------------------------------------------------------------------------------------------
// Group-header containers
// ---------------------------------------------------------------------------------------------

winrt::hstring TableView::GetGroupHeaderNameCandidate(GroupedEntry const& entry)
{
    auto const key = entry.Key();
    return GroupInfoToName(winrt::make<::TableViewGroupInfo>(
        key, entry.GroupItemCount(), 0, false, false, StringifyGroupKey(key)));
}

void TableView::PrepareGroupHeaderElement(winrt::TableViewGroupHeader const& header, int32_t index)
{
    if (!header)
    {
        return;
    }

    TableViewRowInfo rowInfo{};
    // Trust the index's metadata only when it actually describes a group header. A realized header
    // can be re-prepared (OnRowElementIndexChanged) at an index that has just become a DATA row,
    // where hasRowInfo is true but Kind == Data -- taking IsExpandable/IsExpanded from that would
    // push false/false onto a real group header and give it a dead chevron. Fall back to the
    // entry's own state in that window; the repeater re-prepares this position with the right
    // container type immediately after.
    const bool hasRowInfo =
        TryGetRowInfo(index, rowInfo) && rowInfo.Kind == TableViewRowKind::GroupHeader;

    auto* const headerImpl = winrt::get_self<TableViewGroupHeader>(header);
    headerImpl->SetOwningTableViewInternal(*this);
    headerImpl->RefreshDensity();

    winrt::IInspectable groupKey{ nullptr };
    int32_t itemCount{ 0 };
    int32_t level{ 0 };
    bool isExpanded{ false };
    bool isExpandable{ false };

    // Resolve from the PROJECTION at `index`: a reshape can rebind a pooled header in place, so its
    // DataContext may still be the previous projection's entry. DataContext is only the fallback.
    winrt::IInspectable groupRow{ nullptr };
    if (hasRowInfo && m_rowsItemsSourceView && index >= 0 && index < m_rowsItemsSourceView.Count())
    {
        try
        {
            groupRow = m_rowsItemsSourceView.GetAt(index);
        }
        catch (...)
        {
            // Best-effort: the projection can change underneath a deferred reshape.
        }
    }
    if (!groupRow)
    {
        groupRow = header.DataContext();
        if (groupRow && groupRow.try_as<winrt::TableViewGroupInfo>())
        {
            // Already prepared once: DataContext is the projection set below, not the entry.
            groupRow = headerImpl->GroupEntryInternal();
        }
    }

    if (auto const entry = TryGetGroupedEntry(groupRow))
    {
        // TableViewGroupInfo.Key is contracted (TableView.idl) as the group's key value, not the view's
        // group object, so an app template binding {Binding Key} sees the key.
        groupKey = entry->Key();
        // The entry is the fallback while metadata describes a different row.
        itemCount = hasRowInfo ? rowInfo.ChildCount : entry->GroupItemCount();
        isExpanded = hasRowInfo ? rowInfo.IsExpanded : entry->IsExpanded();
        isExpandable = hasRowInfo ? rowInfo.IsExpandable : (entry->GroupItemCount() > 0);
        level = hasRowInfo ? std::max(0, rowInfo.Level) : 0;
    }
    else
    {
        // Reachable only if a non-GroupedEntry row value is ever classified as a header (it is not
        // today, since GetRowKindForItem derives the kind from exactly this probe). Guard on
        // hasRowInfo so this cannot silently render a default-initialized TableViewRowInfo --
        // IsExpandable=false would give a dead chevron.
        groupKey = groupRow;
        if (hasRowInfo)
        {
            itemCount = rowInfo.ChildCount;
            isExpanded = rowInfo.IsExpanded;
            isExpandable = rowInfo.IsExpandable;
            level = std::max(0, rowInfo.Level);
        }
    }

    // Keep bindings live within a group, but never retarget an object held by an open menu to a
    // different group when its header is recycled.
    auto const keyText = StringifyGroupKey(groupKey);
    auto const existing = header.Content().try_as<winrt::TableViewGroupInfo>();

    // Drop the previous group's content so held content peers go stale.
    if (existing && !SameGroupKey(existing.Key(), groupKey) && headerImpl->HasContentAutomationPeersInternal())
    {
        header.ContentTemplate(nullptr);
    }

    // Forward an app-supplied template, else fall back to the control's own. ClearValue removes
    // only the local value, so the default Style's ContentTemplate setter re-applies through DP
    // precedence -- a recycled header can never keep a previous render's template.
    if (auto const appTemplate = GroupHeaderTemplate())
    {
        header.ContentTemplate(appTemplate);
    }
    else
    {
        header.ClearValue(winrt::ContentControl::ContentTemplateProperty());
    }

    // Announce only when THIS group's reported state changed. Two things make the naive "did the
    // boolean flip" test wrong:
    //
    //  - The container is pooled and its projection survives recycling, so a header handed to a
    //    different group would otherwise report that group's state as a change on the new one --
    //    a pure scroll, announced to the user as a collapse. Comparing the key is what makes the
    //    announcement belong to a group rather than to a container.
    //  - The peer reports LeafNode when the group is not expandable, so the announced pair has to
    //    be built the same way the peer builds it, or a cached client ends up holding a value the
    //    provider will never return.
    //
    // Raised from here, not from the IsExpanded DP: a recycled container often already carries the
    // incoming value, so the DP never changes and a DP-based raise stays silent.
    // A toggled group may get another group's container, so its prior state comes from m_pendingGroupExpansionStates.
    const bool sameGroup = existing && SameGroupKey(existing.Key(), groupKey);
    auto previousState = !existing ? winrt::ExpandCollapseState::LeafNode
        : GroupExpandCollapseState(existing.IsExpandable(), existing.IsExpanded());
    bool announce = sameGroup;
    winrt::hstring identity;
    if (hasRowInfo && m_tableViewSourceRowMetadata)
    {
        try
        {
            identity = m_tableViewSourceRowMetadata->GetIdentity(index);
        }
        catch (...)
        {
        }
    }
    if (auto const pending = identity.empty() ? m_pendingGroupExpansionStates.end() : m_pendingGroupExpansionStates.find(identity);
        pending != m_pendingGroupExpansionStates.end())
    {
        previousState = pending->second;
        announce = true;
        m_pendingGroupExpansionStates.erase(pending);
    }
    const auto newState = GroupExpandCollapseState(isExpandable, isExpanded);

    // Internal writes to the read-only DPs; they request nothing and raise no group events. Both
    // are written before the projection changes, which is then synced once.
    headerImpl->SetExpansionStateInternal(isExpandable, isExpanded, false /* syncContent */, false /* useTransitions */);

    if (sameGroup)
    {
        winrt::get_self<TableViewGroupInfo>(existing)->UpdateInternal(groupKey, itemCount, level, keyText);
    }
    else
    {
        header.Content(winrt::make<::TableViewGroupInfo>(
            groupKey, itemCount, level, isExpandable, isExpanded, keyText));
    }
    headerImpl->SyncExpansionToContentInternal();

    // Expose the projection as the header's DataContext, as a data row exposes its item, so
    // bindings in a group header Style and in its ContextFlyout (the flyout takes its target's
    // DataContext) see TableViewGroupInfo rather than the internal projection entry.
    headerImpl->SetGroupEntryInternal(groupRow);
    if (auto const info = header.Content(); header.DataContext() != info)
    {
        header.DataContext(info);
    }
    if (auto const headerPeer = winrt::FrameworkElementAutomationPeer::FromElement(header))
    {
        headerImpl->SyncGroupAutomationPeerInternal(headerPeer, !sameGroup, identity);
    }
    else if (!identity.empty() && GetGroupAutomationElementInternal(identity))
    {
        winrt::FrameworkElementAutomationPeer::CreatePeerForElement(header);
    }

    if (announce && previousState != newState)
    {
        headerImpl->RaiseExpandCollapseStateChanged(previousState, newState);
    }

    UpdateGroupHeaderWidth(header);
}

void TableView::ForEachRealizedGroupHeader(std::function<void(winrt::TableViewGroupHeader const&, int32_t)> const& fn)
{
    auto repeater = m_rowsRepeater.get();
    if (!repeater || !fn)
    {
        return;
    }

    const auto childCount = winrt::VisualTreeHelper::GetChildrenCount(repeater);
    for (int32_t i = 0; i < childCount; ++i)
    {
        if (auto header = winrt::VisualTreeHelper::GetChild(repeater, i).try_as<winrt::TableViewGroupHeader>())
        {
            const auto index = repeater.GetElementIndex(header);
            if (index < 0)
            {
                continue;
            }

            fn(header, index);
        }
    }
}

void TableView::RefreshRealizedGroupHeadersAfterExpansion()
{
    if (!IsGroupedRowSource())
    {
        return;
    }

    ForEachRealizedGroupHeader([this](winrt::TableViewGroupHeader const& header, int32_t index)
    {
        PrepareGroupHeaderElement(header, index);
    });

    m_pendingGroupExpansionStates.clear();
}

void TableView::OnGroupHeaderTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue() == args.NewValue())
    {
        return;
    }

    ForEachRealizedGroupHeader([this](winrt::TableViewGroupHeader const& header, int32_t index)
    {
        PrepareGroupHeaderElement(header, index);
    });
}

void TableView::ClearGroupHeaderElement(winrt::TableViewGroupHeader const& header)
{
    if (!header)
    {
        return;
    }

    // Leave Content in place for a same-group prepare. A different group gets a fresh projection,
    // so references held by an open menu retain their original identity. Drop the app-owned
    // template so a GroupHeaderTemplate change while this header sits in the pool cannot resurface.
    //
    // The owning-TableView weak ref is deliberately NOT cleared. On the element-factory path the
    // repeater leaves recycled containers parented, so an AT client that holds a provider across a
    // scroll can still call Expand()/Row()/ContainingGrid() on this element -- nulling the owner
    // turns those into silent no-ops, which is worse than the ancestor walk it replaced. A weak
    // ref costs nothing to keep and stays correct.
    winrt::get_self<TableViewGroupHeader>(header)->ResetPointerStateInternal();
    header.ClearValue(winrt::ContentControl::ContentTemplateProperty());
    winrt::get_self<TableViewGroupHeader>(header)->SetGroupEntryInternal(nullptr);
    if (auto const headerPeer = winrt::FrameworkElementAutomationPeer::FromElement(header))
    {
        winrt::get_self<TableViewGroupHeader>(header)->SyncGroupAutomationPeerInternal(headerPeer, false, {});
    }
}

void TableView::UpdateGroupHeaderWidth(winrt::TableViewGroupHeader const& header)
{
    if (!header)
    {
        return;
    }

    // The band sizes itself in TableViewGroupHeader::MeasureOverride, from the same visible-columns
    // sum the cells panel uses. All that is needed when columns move is to ask for a fresh measure
    // -- pushing a Width here races the layout and produces a short band on grow, because column
    // ActualWidths are not final when column resolve runs.
    header.InvalidateMeasure();
}
