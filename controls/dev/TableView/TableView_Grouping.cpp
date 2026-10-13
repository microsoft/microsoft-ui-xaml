// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "GroupedEntry.h"
#include "TableView.h"
#include "TableViewSource.h"
#include "TableViewRow.h"
#include "TableViewAutomationPeer.h"
#include "TableViewAutomationHelpers.h"
#include "RowMetadataProvider.h"
#include "TableViewGroupHeader.h"
#include "TableViewGroupInfo.h"
#include "TableViewGroupingHelpers.h"
#include "ResourceAccessor.h"

#include <algorithm>

namespace
{
    winrt::IInspectable GetGroupHeaderKey(GroupedEntry const& entry)
    {
        auto const groupObject = entry.Group();
        if (auto const group = groupObject.try_as<winrt::Microsoft::UI::Xaml::Data::ICollectionViewGroup>())
        {
            return group.Group();
        }
        return groupObject;
    }
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
// A grouped TableViewSource projects group headers and data rows into one flat row list, so the
// repeater realizes two container types from one source. TableViewRowTemplateSelector picks
// between them using GetRowKindForItem; everything a group header needs that a TableViewRow would
// otherwise have pushed during cell rebuild is pushed from PrepareGroupHeaderElement instead.
// ---------------------------------------------------------------------------------------------

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

    // Only a grouped TableViewSource produces GroupedEntry rows, and only for headers. A flat
    // source hands the app's item straight through, and an app item must never be shaped into a
    // group header just because it happens to satisfy the probe.
    if (!IsTableViewSourceGrouped())
    {
        return TableViewRowKind::Data;
    }

    return TryGetGroupedEntry(item) ? TableViewRowKind::GroupHeader : TableViewRowKind::Data;
}

bool TableView::TryGetTableViewSourceRowInfo(int32_t rowIndex, TableViewRowInfo& rowInfo) const
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

bool TableView::IsTableViewSourceGrouped() const
{
    if (auto const source = m_activeSource.get())
    {
        return winrt::get_self<::TableViewSource>(source)->IsGrouped();
    }

    return false;
}

bool TableView::IsGroupHeaderRow(int32_t index) const
{
    TableViewRowInfo rowInfo{};
    return TryGetTableViewSourceRowInfo(index, rowInfo) && rowInfo.Kind == TableViewRowKind::GroupHeader;
}

int32_t TableView::GetDataRowPositionInSetInternal(int32_t rowIndex) const
{
    if (rowIndex < 0)
    {
        return 0;
    }

    if (!IsTableViewSourceGrouped())
    {
        return rowIndex + 1;
    }

    int32_t position = 0;
    for (int32_t index = 0; index <= rowIndex; ++index)
    {
        TableViewRowInfo rowInfo{};
        if (!TryGetTableViewSourceRowInfo(index, rowInfo))
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
    if (!IsTableViewSourceGrouped())
    {
        return rowCount;
    }

    int32_t dataRowCount = 0;
    for (int32_t index = 0; index < rowCount; ++index)
    {
        TableViewRowInfo rowInfo{};
        if (!TryGetTableViewSourceRowInfo(index, rowInfo))
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

void TableView::ExpandRowSubtree(winrt::UIElement const& container)
{
    RequestGroupExpansion(container, true, true /* subtree */);
}

bool TableView::TryFocusParentRow(winrt::UIElement const& container)
{
    auto const repeater = m_rowsRepeater.get();
    if (!repeater || !container || !m_tableViewSourceRowMetadata)
    {
        return false;
    }

    const auto index = repeater.GetElementIndex(container);
    TableViewRowInfo info{};
    int32_t parentIndex = -1;
    if (index < 0 || !TryGetTableViewSourceRowInfo(index, info) || info.ParentIdentity.empty() ||
        !m_tableViewSourceRowMetadata->TryGetIndexForIdentity(info.ParentIdentity, parentIndex))
    {
        return false;
    }

    if (!FocusRow(parentIndex))
    {
        return false;
    }

    // Selection follows the keyboard cursor, as for Up/Down (drill is never Ctrl here).
    SelectRowIndexFromInteraction(parentIndex, false /* toggle */);
    return true;
}

void TableView::RequestGroupExpansion(winrt::UIElement const& container, std::optional<bool> desired, bool subtree)
{
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
    CaptureContainerFocusForRestore(container, identity);

    QueueGroupExpansionByIdentity(identity, desired, subtree);
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
        // Best-effort: the grouping source or its state can change during cleanup.
        return {};
    }
}

void TableView::CaptureContainerFocusForRestore(winrt::UIElement const& container, winrt::hstring const& identity)
{
    // Clear any prior capture: a fresh toggle supersedes an earlier one whose restore has not run.
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;

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

    // A data row's focus is usually on an inner cell/editor, so read the focused element's state.
    auto const stateSource = container.try_as<winrt::TableViewRow>()
        ? focused.try_as<winrt::Control>()
        : container.try_as<winrt::Control>();

    if (auto const control = stateSource)
    {
        // Pointer focus draws no focus visual, so there is nothing to restore for a band click.
        // Keyboard (and programmatic, e.g. a test driving Focus) carry a visual worth preserving.
        const auto state = control.FocusState();
        if (state == winrt::FocusState::Keyboard || state == winrt::FocusState::Programmatic)
        {
            m_pendingGroupFocusIdentity = identity;
            m_pendingGroupFocusState = state;
        }
    }
}

winrt::hstring TableView::CaptureFocusedContainerForRestore()
{
    // A bulk expand/collapse supersedes any earlier capture whose restore has not run, exactly as
    // a fresh single-group toggle does.
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;

    auto const root = XamlRoot();
    if (!root)
    {
        return {};
    }

    // Walk out from the focused element to its header or data-row container, if it is inside one.
    auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
    for (winrt::DependencyObject node = focused; node; node = winrt::VisualTreeHelper::GetParent(node))
    {
        winrt::UIElement container{ nullptr };
        if (auto const header = node.try_as<winrt::TableViewGroupHeader>())
        {
            container = header;
        }
        else if (auto const row = node.try_as<winrt::TableViewRow>())
        {
            container = row;
        }

        if (container)
        {
            auto const identity = TryGetContainerIdentity(container);
            CaptureContainerFocusForRestore(container, identity);
            return identity;
        }
    }

    return {};
}

// Directional request from a UIA provider or the band gesture. Identity is resolved here, while
// the container's index is still current, because the mutation below is deferred.
void TableView::QueueGroupExpansionByIdentity(winrt::hstring const& identity, std::optional<bool> desired, bool subtree)
{
    if (identity.empty())
    {
        return;
    }

    // Capture the intent, not just the target. A directionless toggle applied on a later turn is
    // not idempotent: two Expand() calls would expand then collapse. Carrying `desired` through
    // makes the deferred half an idempotent set.
    const auto generation = m_rowMetadataGeneration;

    // Defer the structural mutation off the current callout. Running it inline re-projects rows
    // while the caller's frame is still on the stack, which faults on the pointer path and
    // surfaces to a UIA client as an exception escaping the COM boundary. Both callers are safe
    // once the mutation happens on a later turn.
    auto weakThis = get_weak();
    if (auto const queue = DispatcherQueue())
    {
        // A refused enqueue means the thread is shutting down and the UI is going away regardless.
        queue.TryEnqueue([weakThis, identity, desired, generation, subtree]()
            {
                if (auto strongThis = weakThis.get())
                {
                    strongThis->ApplyGroupExpansionByIdentity(identity, desired, generation, subtree);
                }
            });
        return;
    }

    // No dispatcher means this is not a UI thread (test host), so there is no in-flight callout to
    // unwind and the hazard above cannot apply.
    ApplyGroupExpansionByIdentity(identity, desired, generation, subtree);
}

void TableView::ApplyGroupExpansionByIdentity(winrt::hstring const& identity, std::optional<bool> desired, uint64_t generation, bool subtree)
{
    // Identities are value-based strings, not tied to a provider instance. If ItemsSource was
    // replaced while this request sat on the queue, the same string could name an unrelated group
    // in the new source -- exactly the wrong-group mutation this path exists to prevent.
    if (generation != m_rowMetadataGeneration)
    {
        return;
    }

    if (!m_tableViewSourceRowMetadata || identity.empty())
    {
        return;
    }

    // An open edit sits over a row that expansion is about to move or remove. Coalesce behind the
    // termination rather than reshaping underneath it.
    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending)
        {
            QueueCoalescedEditReshape([this, identity, desired, generation, subtree]()
            {
                ApplyGroupExpansionByIdentity(identity, desired, generation, subtree);
            });
        }
        return;
    }

    bool changed = false;
    try
    {
        // Local strong ref: a handler may re-declare the shape and replace the provider mid-call.
        auto const provider = m_tableViewSourceRowMetadata;
        if (subtree)
        {
            provider->ExpandSubtree(identity);
            changed = true;
        }
        else if (desired.has_value())
        {
            // Idempotent set: applying the state we already have is a no-op in the provider.
            if (*desired)
            {
                provider->Expand(identity);
            }
            else
            {
                provider->Collapse(identity);
            }
            changed = true;
        }
        else
        {
            changed = provider->Toggle(identity);
        }
    }
    catch (...)
    {
        // Best-effort: the grouping source or its state can change during cleanup.
    }

    DrainCoalescedEditReshape();

    // Announce only once the tree has actually moved. Raising while the mutation was still queued
    // told clients to re-read a structure that had not changed yet.
    if (changed)
    {
        QueueGroupExpansionRowRefresh();
        // The toggled row keeps its index, so it is never re-prepared; restamp it here.
        RefreshRealizedRowHierarchyState();
        RaiseGroupStructureChanged();
    }

    // Restore keyboard focus to the toggled header or row even when nothing structurally changed: a
    // non-expandable/no-op toggle still ran the container through the reshape path, and leaving
    // focus stranded is the very bug this guards. Matches on identity, so an unrelated queued
    // toggle does not consume this restore.
    RestoreContainerFocusIfPending(identity);
}

void TableView::RestoreContainerFocusIfPending(winrt::hstring const& identity)
{
    if (m_pendingGroupFocusIdentity.empty() || m_pendingGroupFocusIdentity != identity)
    {
        return;
    }

    const auto focusState = m_pendingGroupFocusState;
    m_pendingGroupFocusIdentity.clear();
    m_pendingGroupFocusState = winrt::FocusState::Unfocused;

    // Defer to after the reshape's relayout. The reprojection triggered by the toggle recycles the
    // header container during the next layout pass; focusing now would land on a container layout
    // is about to recycle, dropping focus a second time. A one-shot LayoutUpdated fires once the
    // tree has settled, mirroring FocusRow's deferred-focus pattern.
    if (m_pendingGroupFocusLayoutToken.value)
    {
        LayoutUpdated(m_pendingGroupFocusLayoutToken);
        m_pendingGroupFocusLayoutToken = {};
    }

    auto weakThis = get_weak();
    m_pendingGroupFocusLayoutToken = LayoutUpdated(
        [weakThis, identity, focusState](winrt::IInspectable const&, winrt::IInspectable const&)
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
                strongThis->FocusContainerByIdentity(identity, focusState);
            }
            catch (...)
            {
                // Best-effort focus restore: the header or row can be gone by the time layout settles.
            }
        });
}

void TableView::FocusContainerByIdentity(winrt::hstring const& identity, winrt::FocusState focusState)
{
    if (identity.empty() || !m_tableViewSourceRowMetadata)
    {
        return;
    }

    int32_t index = -1;
    if (!m_tableViewSourceRowMetadata->TryGetIndexForIdentity(identity, index) || index < 0)
    {
        return;
    }

    auto repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return;
    }

    // The metadata provider and repeater own separate views that can absorb reshapes at different
    // times, so a resolved index can exceed what the repeater accepts. GetOrCreateElement throws
    // for that, and this deferred layout callback would fail-fast the process.
    auto const sourceView = repeater.ItemsSourceView();
    if (!sourceView || index >= sourceView.Count())
    {
        return;
    }

    // Data row: a splice keeps focus in place; restore only if the Reset recycled the container.
    TableViewRowInfo info{};
    if (TryGetTableViewSourceRowInfo(index, info) && info.Kind == TableViewRowKind::Data)
    {
        if (auto const root = XamlRoot())
        {
            auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>();
            for (winrt::DependencyObject node = focused; node; node = winrt::VisualTreeHelper::GetParent(node))
            {
                if (auto const row = node.try_as<winrt::TableViewRow>())
                {
                    if (repeater.GetElementIndex(row) == index)
                    {
                        return;
                    }
                    break;
                }
            }
        }

        FocusRow(index);
        return;
    }

    auto element = repeater.GetOrCreateElement(index);
    if (!element)
    {
        return;
    }

    // Only a group header is a valid target: after a re-sort or source swap the identity's index
    // could now name a data row, and focusing that with a header's intent would be wrong.
    if (!element.try_as<winrt::TableViewGroupHeader>())
    {
        return;
    }

    if (auto const frameworkElement = element.try_as<winrt::FrameworkElement>())
    {
        frameworkElement.StartBringIntoView();
    }

    if (auto const control = element.try_as<winrt::Control>())
    {
        control.Focus(focusState);
    }
}

void TableView::ExpandAllGroups()
{
    SetBulkExpansion(true, BulkExpansionAxis::Groups);
}

void TableView::CollapseAllGroups()
{
    SetBulkExpansion(false, BulkExpansionAxis::Groups);
}

void TableView::ExpandAllRows()
{
    SetBulkExpansion(true, BulkExpansionAxis::Rows);
}

void TableView::CollapseAllRows()
{
    SetBulkExpansion(false, BulkExpansionAxis::Rows);
}

// Bulk counterpart of ApplyGroupExpansionByIdentity for either axis. Runs inline (no UIA
// callout), but still coalesces behind an in-flight edit.
void TableView::SetBulkExpansion(bool expand, BulkExpansionAxis axis)
{
    if (!m_tableViewSourceRowMetadata)
    {
        return;
    }

    // Keep documented no-ops from committing an open edit.
    const bool hasAxis = (axis == BulkExpansionAxis::Rows)
        ? m_tableViewSourceRowMetadata->IsHierarchicalSource()
        : IsTableViewSourceGrouped();
    if (!hasAxis)
    {
        return;
    }

    if (!TryTerminateEditForControlInitiatedReshape())
    {
        if (m_editState == EditState::Ending)
        {
            QueueCoalescedEditReshape([this, expand, axis]()
            {
                SetBulkExpansion(expand, axis);
            });
        }
        return;
    }

    // Both axes: row expansion under groups also recycles focused header containers.
    auto const focusedGroupIdentity = CaptureFocusedContainerForRestore();

    bool changed = false;
    try
    {
        // Local strong ref, as in ApplyGroupExpansionByIdentity.
        auto const provider = m_tableViewSourceRowMetadata;
        switch (axis)
        {
        case BulkExpansionAxis::Groups:
            expand
                ? provider->ExpandAllGroups()
                : provider->CollapseAllGroups();
            break;

        case BulkExpansionAxis::Rows:
            expand
                ? provider->ExpandAllRows()
                : provider->CollapseAllRows();
            break;
        }

        changed = true;
    }
    catch (...)
    {
        // Best-effort: the grouping source or its state can change during cleanup.
    }

    DrainCoalescedEditReshape();

    if (changed)
    {
        QueueGroupExpansionRowRefresh();
        RefreshRealizedRowHierarchyState();

        // Both axes: under a grouped tree, row expansion changes group contents.
        RaiseGroupStructureChanged();
    }

    RestoreContainerFocusIfPending(focusedGroupIdentity);
}

// Restamps every realized row; rows that kept their index after a reshape are stale.
void TableView::RefreshRealizedRowHierarchyState()
{
    auto const repeater = m_rowsRepeater.get();
    if (!repeater)
    {
        return;
    }

    ForEachRealizedRow([this, repeater](winrt::TableViewRow const& row)
    {
        // A container awaiting recycle reports -1; its state is about to be re-derived on prepare.
        const auto index = repeater.GetElementIndex(row);
        if (index >= 0)
        {
            RefreshRowHierarchyState(row, index);
        }
    });
}

void TableView::RaiseGroupStructureChanged()
{
    QueueTerminalGridLineRefresh();

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
    auto const key = GetGroupHeaderKey(entry);
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
        TryGetTableViewSourceRowInfo(index, rowInfo) && rowInfo.Kind == TableViewRowKind::GroupHeader;

    auto* const headerImpl = winrt::get_self<TableViewGroupHeader>(header);
    headerImpl->SetOwningTableViewInternal(*this);

    // Subscribe ONCE per container: it is pooled and re-prepared many times, and subscribing per
    // prepare would fan one band gesture into N toggles. The guard lives here because containers
    // come from a DataTemplate and have no single construction site to hook.
    if (!headerImpl->IsToggleHooked())
    {
        winrt::weak_ref<winrt::TableView> weakThis{ *this };
        header.ToggleRequested(
            [weakThis](winrt::TableViewGroupHeader const& sender, winrt::TableViewGroupHeaderToggleRequestedEventArgs const&)
            {
                if (auto const tableView = weakThis.get())
                {
                    winrt::get_self<TableView>(tableView)->ToggleGroupExpansion(sender);
                }
            });
        headerImpl->SetToggleHooked();
    }

    winrt::IInspectable groupKey{ nullptr };
    int32_t itemCount{ 0 };
    int32_t level{ 0 };
    bool isExpanded{ false };
    bool isExpandable{ false };

    if (auto const entry = TryGetGroupedEntry(header.DataContext()))
    {
        // TableViewGroupInfo.Key is contracted (TableView.idl) as the GroupBy key, not the
        // internal group object. entry->Group() is the ShapedGroup (an ICollectionViewGroup);
        // unwrap it to the key it carries so an app template binding {Binding Key} sees the key
        // value, not the projection wrapper. KeyText / display is unaffected either way.
        groupKey = GetGroupHeaderKey(*entry);
        // Prefer row metadata (root count) over the entry's span count under a hierarchy; the entry is
        // the fallback while metadata describes a different row.
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
        groupKey = header.DataContext();
        if (hasRowInfo)
        {
            itemCount = rowInfo.ChildCount;
            isExpanded = rowInfo.IsExpanded;
            isExpandable = rowInfo.IsExpandable;
            level = std::max(0, rowInfo.Level);
        }
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

    // Update the projection in place. Re-pointing Content at a fresh instance every prepare would
    // re-evaluate every binding in the content template and drop any binding the app made against
    // the previous instance.
    auto const keyText = StringifyGroupKey(groupKey);
    auto const existing = header.Content().try_as<winrt::TableViewGroupInfo>();

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
    const bool sameGroup = existing && SameGroupKey(existing.Key(), groupKey);
    const auto previousState = !existing ? winrt::ExpandCollapseState::LeafNode
        : !existing.IsExpandable() ? winrt::ExpandCollapseState::LeafNode
        : existing.IsExpanded() ? winrt::ExpandCollapseState::Expanded
        : winrt::ExpandCollapseState::Collapsed;
    const auto newState = !isExpandable ? winrt::ExpandCollapseState::LeafNode
        : isExpanded ? winrt::ExpandCollapseState::Expanded
        : winrt::ExpandCollapseState::Collapsed;

    if (existing)
    {
        winrt::get_self<TableViewGroupInfo>(existing)->UpdateInternal(groupKey, itemCount, level, keyText);
    }
    else
    {
        header.Content(winrt::make<::TableViewGroupInfo>(
            groupKey, itemCount, level, isExpandable, isExpanded, keyText));
    }

    header.IsExpandable(isExpandable);
    header.IsExpanded(isExpanded);

    if (sameGroup && previousState != newState)
    {
        headerImpl->RaiseExpandCollapseStateChanged(previousState, newState);
    }

    UpdateGroupHeaderWidth(header);
}

void TableView::ClearGroupHeaderElement(winrt::TableViewGroupHeader const& header)
{
    if (!header)
    {
        return;
    }

    // Leave Content in place: the pooled header is handed back for the next group and
    // UpdateInternal re-points the same projection, which is the whole reason bindings survive
    // recycling. Only the app-owned template is dropped, so a GroupHeaderTemplate change while
    // this header sits in the pool cannot resurface.
    //
    // The owning-TableView weak ref is deliberately NOT cleared. On the element-factory path the
    // repeater leaves recycled containers parented, so an AT client that holds a provider across a
    // scroll can still call Expand()/Row()/ContainingGrid() on this element -- nulling the owner
    // turns those into silent no-ops, which is worse than the ancestor walk it replaced. A weak
    // ref costs nothing to keep and stays correct.
    header.ClearValue(winrt::ContentControl::ContentTemplateProperty());
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
