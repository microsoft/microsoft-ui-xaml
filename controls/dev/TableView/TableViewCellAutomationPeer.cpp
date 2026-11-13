// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableView.h"
#include "TableViewRow.h"
#include "TableViewAutomationPeer.h"
#include "TableViewColumnHeaderAutomationPeer.h"
#include "TableViewCellAutomationPeer.h"
#include "TableViewCell.h"
#include "TableViewAutomationHelpers.h"
#include "TableViewToolTipHelpers.h"
#include "ResourceAccessor.h"
#include "TableViewCellAutomationPeer.properties.cpp"
#include "TVDiag.h"

#include <UIAutomationCore.h>
#include <UIAutomationCoreApi.h>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

TableViewCellAutomationPeer::CellOwner TableViewCellAutomationPeer::CellOwnerForPublicConstructor(winrt::FrameworkElement const& owner)
{
    auto const access = owner ? owner.try_as<ITableViewCellAutomationPeerAccess>() : nullptr;
    if (!access)
    {
        throw winrt::hresult_invalid_argument();
    }

    winrt::IInspectable row{ nullptr };
    winrt::IInspectable column{ nullptr };
    TableViewCellAutomationPeer::CellOwner result;
    winrt::check_hresult(access->GetCellOwner(
        reinterpret_cast<::IInspectable**>(winrt::put_abi(row)),
        reinterpret_cast<::IInspectable**>(winrt::put_abi(column)),
        &result.ColumnIndex));
    result.Row = row.try_as<winrt::TableViewRow>();
    result.Column = column.try_as<winrt::TableViewColumn>();
    if (!result.Row || !result.Column || result.ColumnIndex < 0)
    {
        throw winrt::hresult_invalid_argument();
    }
    return result;
}

TableViewCellAutomationPeer::TableViewCellAutomationPeer(winrt::FrameworkElement const& owner)
    : TableViewCellAutomationPeer(owner, CellOwnerForPublicConstructor(owner))
{
}

TableViewCellAutomationPeer::TableViewCellAutomationPeer(winrt::FrameworkElement const& owner, CellOwner const& cell)
    : TableViewCellAutomationPeer(TrustedOwnerTag{}, owner, cell.Row, cell.Column, cell.ColumnIndex)
{
}

TableViewCellAutomationPeer::TableViewCellAutomationPeer(
    TrustedOwnerTag,
    winrt::FrameworkElement const& cell,
    winrt::TableViewRow const& row,
    winrt::TableViewColumn const& column,
    int32_t columnIndex)
    : ReferenceTracker(cell)
    , m_columnIndex(columnIndex)
{
    // The cell owner supplies bounds; weak refs avoid extending row/column lifetimes.
    if (row)
    {
        m_row = winrt::make_weak(row);
    }
    if (column)
    {
        m_column = winrt::make_weak(column);
    }
    if (row)
    {
        if (auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
        {
            TrackRowItem(row, tableView);
        }
    }
    // Primes m_lastOwningTable / m_lastKnownRowIndex for IsVirtualized(). It has to happen while
    // the row is still in the tree: once the row is recycled the index is unresolvable, and a
    // client may not query VirtualizedItem until after that point.
    std::ignore = GetRowIndex();
}

TableViewCellAutomationPeer::~TableViewCellAutomationPeer()
{
    ResetAutomationContentViewCache();
}

winrt::IInspectable TableViewCellAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // GridItem + TableItem are structural and always meaningful for realized cells.
    if (patternInterface == winrt::PatternInterface::GridItem ||
        patternInterface == winrt::PatternInterface::TableItem)
    {
        return *this;
    }

    if (patternInterface == winrt::PatternInterface::VirtualizedItem && IsVirtualized())
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

hstring TableViewCellAutomationPeer::GetClassNameCore()
{
    return L"TableViewCell";
}

winrt::AutomationControlType TableViewCellAutomationPeer::GetAutomationControlTypeCore()
{
    // DataItem lets Narrator read the composed cell name instead of a generic container.
    return winrt::AutomationControlType::DataItem;
}

hstring TableViewCellAutomationPeer::GetLocalizedControlTypeCore()
{
    // A host app may not merge the control's PRI; degrade to the framework default rather than
    // throwing into UIA.
    if (auto const localized = TryGetLocalizedString(SR_TableViewCellLocalizedControlType); !localized.empty())
    {
        return localized;
    }

    return __super::GetLocalizedControlTypeCore();
}

// UIA focusability is not Tab reachability: row-level cells still need SetFocus and
// IsKeyboardFocusable to succeed when enabled, visible and actionable.
bool TableViewCellAutomationPeer::IsKeyboardFocusableCore()
{
    auto const cell = Owner().try_as<winrt::FrameworkElement>();
    if (!cell || cell.Visibility() != winrt::Visibility::Visible || IsVirtualized())
    {
        return false;
    }

    // The cell wrapper is a Grid, so IsEnabled lives on the owning row, not on the cell.
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        return false;
    }
    return !row || (row.IsEnabled() && winrt::get_self<TableViewRow>(row)->IsCellActionableInternal(cell));
}

void TableViewCellAutomationPeer::SetFocusCore()
{
    auto const row = m_row.get();
    if (!row || !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const cell = row ? GetRealizedCellFromRow(row) : nullptr;

    if (row && cell)
    {
        // Only an actionable cell takes focus; never leave the row armed with no focusable level.
        auto const rowImpl = winrt::get_self<TableViewRow>(row);
        const int32_t column = rowImpl->GetVisibleCellIndexInternal(cell);
        if (column < 0 || !rowImpl->IsCellActionableInternal(cell))
        {
            throw winrt::hresult_error(UIA_E_INVALIDOPERATION);
        }
        // The exact cell or nothing: no fallback to the row, and a refused cell restores the level.
        const bool wasCellLevel = rowImpl->IsCellLevelInternal();
        rowImpl->SetCellLevelInternal(true);
        if (!cell.Focus(winrt::FocusState::Programmatic) || cell.FocusState() == winrt::FocusState::Unfocused)
        {
            rowImpl->SetCellLevelInternal(wasCellLevel);
            throw winrt::hresult_error(UIA_E_INVALIDOPERATION);
        }
        return;
    }

    __super::SetFocusCore();
}

void TableViewCellAutomationPeer::ShowContextMenuCore()
{
    // Same tracking rule as SetFocusCore: a recycled cell must not raise a request for another item.
    auto const row = m_row.get();
    auto const cell = row && GetTrackedTableForRow(row) ? GetRealizedCellFromRow(row) : nullptr;
    if (!cell)
    {
        ThrowElementNotAvailable();
    }

    if (cell == Owner())
    {
        __super::ShowContextMenuCore();
    }
    else if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(cell))
    {
        peer.ShowContextMenu();
    }
}

hstring TableViewCellAutomationPeer::GetNameCore()
{
    auto const row = m_row.get();
    auto const tableView = row ? GetTrackedTableForRow(row) : nullptr;
    if (row && !tableView)
    {
        ThrowElementNotAvailable();
    }

    UpdateNameItem(row && tableView
        ? row.DataContext() : nullptr);
    if (auto const cell = Owner().try_as<winrt::FrameworkElement>();
        GetCachedHasInteractiveCellContent(cell))
    {
        // The control names itself; naming the cell after it too would read that name twice.
        auto const name = GetColumnHeaderText(tableView);
        m_lastName = name;
        return name;
    }
    auto const name = ReadDisplayName(tableView);
    m_lastName = name;
    return name;
}

winrt::IVector<winrt::AutomationPeer> TableViewCellAutomationPeer::GetChildrenCore()
{
    if (auto const cell = Owner().try_as<winrt::FrameworkElement>())
    {
        PrepareAutomationContentView(cell);
    }

    return __super::GetChildrenCore();
}

void TableViewCellAutomationPeer::UpdateNameItem(winrt::IInspectable const& item)
{
    if (!TableView::SameInspectableIdentity(m_nameItem.get(), item))
    {
        ResetNameState();
        m_nameItem.set(item);
        m_item.Track(item);
        m_lastKnownRowIndex = -1;
        m_trackedItemOccurrence = -1;
        ResetAutomationContentViewCache();
    }
}

void TableViewCellAutomationPeer::ResetNameState()
{
    m_lastName.reset();
    m_nameItem.set(nullptr);
}

namespace
{
    // True when the cell shows nothing: only layout containers and empty text.
    bool ShowsNoCellContent(winrt::DependencyObject const& element, uint32_t depth)
    {
        auto const fe = element.try_as<winrt::FrameworkElement>();
        if (!fe || fe.Visibility() != winrt::Visibility::Visible)
        {
            return true;
        }
        if (auto const textBlock = fe.try_as<winrt::TextBlock>())
        {
            return textBlock.Text().empty();
        }
        if (auto const textBox = fe.try_as<winrt::TextBox>())
        {
            return textBox.Text().empty();
        }
        if (depth == 0 || !(fe.try_as<winrt::Panel>() || fe.try_as<winrt::Border>() || fe.try_as<winrt::ContentPresenter>()))
        {
            return false;
        }
        const auto count = winrt::VisualTreeHelper::GetChildrenCount(fe);
        for (int32_t i = 0; i < count; ++i)
        {
            if (!ShowsNoCellContent(winrt::VisualTreeHelper::GetChild(fe, i), depth - 1))
            {
                return false;
            }
        }
        return true;
    }
}

winrt::hstring TableViewCellAutomationPeer::ReadDisplayName(winrt::TableView const& table, winrt::FrameworkElement const& display)
{
    uint32_t remaining = 32;
    auto valueText = display
        ? GetCellContentName(display, true, 8, remaining)
        : GetCellDisplayText(Owner().try_as<winrt::FrameworkElement>());

    // Borrowing the header would read an empty cell like one whose value is the header text.
    if (valueText.empty())
    {
        // An unnamed visual (a ProgressBar, say) is not blank; name it by its column instead.
        auto const content = display ? display : GetCellContentElement(Owner().try_as<winrt::FrameworkElement>());
        if (content && !ShowsNoCellContent(content, 8))
        {
            return GetColumnHeaderText(table);
        }
        valueText = LocalizedOrFallbackForTableViewAutomation(SR_TableViewCellBlankName, L"blank");
    }

    // A reachable header item already gives the column context.
    if (IsColumnHeaderReachable(table))
    {
        return valueText;
    }

    const auto headerText = GetColumnHeaderText(table);
    if (headerText.empty())
    {
        return valueText;
    }

    return FormatLocalizedOrFallback(SR_TableViewCellNameFormat, L"%1!s!, %2!s!", headerText.c_str(), valueText.c_str(), L", ");
}

winrt::hstring TableViewCellAutomationPeer::GetColumnHeaderText(winrt::TableView const& table)
{
    return ResolveColumnAccessibleName(table, m_column.get());
}

bool TableViewCellAutomationPeer::IsColumnHeaderReachable(winrt::TableView const& table)
{
    auto const column = m_column.get();
    return table && IsVisibleColumn(column) &&
        winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == table &&
        winrt::get_self<TableView>(table)->AreColumnHeadersShownInternal();
}

std::optional<winrt::hstring> TableViewCellAutomationPeer::TakeLastNameInternal()
{
    auto lastName = std::move(m_lastName);
    m_lastName.reset();
    return lastName;
}

winrt::hstring TableViewCellAutomationPeer::GetCellValueText()
{
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const content = GetCellContentElement(Owner().try_as<winrt::FrameworkElement>());
    // ValuePattern describes editable text, not the accessibility label naming that text.
    if (auto const text = content.try_as<winrt::TextBlock>())
    {
        return text.Text();
    }
    if (auto const editor = content.try_as<winrt::TextBox>())
    {
        return editor.Text();
    }
    return GetCellDisplayText(Owner().try_as<winrt::FrameworkElement>());
}

void TableViewCellAutomationPeer::PrepareAutomationContentView(winrt::FrameworkElement const& cell)
{
    auto const content = GetCellAutomationContent(cell);
    if (!content)
    {
        ResetAutomationContentViewCache();
        return;
    }

    if (GetCachedHasInteractiveCellContent(cell))
    {
        SetAccessibilityViewIfNeeded(content, winrt::AccessibilityView::Content);
        SetInteractiveCellContentAccessibilityViewContent(content);
    }
    else
    {
        SetCellContentAccessibilityViewRaw(content);
    }
}

bool TableViewCellAutomationPeer::GetCachedHasInteractiveCellContent(winrt::FrameworkElement const& cell)
{
    auto const content = GetCellAutomationContent(cell);
    if (!content)
    {
        ResetAutomationContentViewCache();
        return false;
    }

    auto const item = GetCurrentAutomationContentItem();
    if (IsAutomationContentCacheValid(content, item))
    {
        return *m_hasInteractiveAutomationContent;
    }

    ResetAutomationContentViewCache();
    m_automationContent = winrt::make_weak(content);
    m_automationContentItem.Track(item);

    uint32_t remaining = 64;
    m_hasInteractiveAutomationContent =
        HasInteractiveCellContentAndRegisterCallbacks(content, 8, &remaining);
    return *m_hasInteractiveAutomationContent;
}

winrt::IInspectable TableViewCellAutomationPeer::GetCurrentAutomationContentItem()
{
    auto const row = m_row.get();
    if (!row)
    {
        return nullptr;
    }

    if (auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView())
    {
        return winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext());
    }

    return row.DataContext();
}

bool TableViewCellAutomationPeer::IsAutomationContentCacheValid(
    winrt::FrameworkElement const& content,
    winrt::IInspectable const& item) const
{
    auto const cachedContent = m_automationContent.get();
    return m_hasInteractiveAutomationContent.has_value() &&
        cachedContent &&
        TableView::SameInspectableIdentity(cachedContent, content) &&
        CachedAutomationContentItemMatches(item);
}

bool TableViewCellAutomationPeer::CachedAutomationContentItemMatches(winrt::IInspectable const& item) const
{
    return item ? m_automationContentItem.SameIdentityAs(item) : !m_automationContentItem.IsTracking();
}

bool TableViewCellAutomationPeer::HasInteractiveCellContentAndRegisterCallbacks(
    winrt::UIElement const& element,
    uint32_t depthBudget,
    uint32_t* remainingBudget)
{
    uint32_t localBudget = 64;
    auto budget = remainingBudget ? remainingBudget : &localBudget;
    if (!element || depthBudget == 0 || *budget == 0)
    {
        return false;
    }

    RegisterAutomationContentPropertyCallbacks(element);
    if (element.Visibility() != winrt::Visibility::Visible)
    {
        return false;
    }

    --(*budget);
    if (IsFocusableCellContent(element))
    {
        return true;
    }

    constexpr int32_t maxChildrenPerLevel = 32;
    auto const childCount = winrt::VisualTreeHelper::GetChildrenCount(element);
    for (int32_t i = 0; i < childCount && i < maxChildrenPerLevel && *budget > 0; ++i)
    {
        if (auto const child = winrt::VisualTreeHelper::GetChild(element, i).try_as<winrt::UIElement>())
        {
            if (HasInteractiveCellContentAndRegisterCallbacks(child, depthBudget - 1, budget))
            {
                return true;
            }
        }
    }

    return false;
}

void TableViewCellAutomationPeer::RegisterAutomationContentPropertyCallbacks(winrt::UIElement const& element)
{
    auto const weakThis = get_weak();
    auto const callback =
        [weakThis](winrt::DependencyObject const&, winrt::DependencyProperty const&)
        {
            if (auto const peer = weakThis.get())
            {
                peer->InvalidateAutomationContentViewCache();
            }
        };

    auto const observe =
        [&](winrt::DependencyObject const& target, winrt::DependencyProperty const& property)
        {
            m_automationContentPropertyChangedRevokers.emplace_back(
                target,
                property,
                target.RegisterPropertyChangedCallback(property, callback));
        };

    observe(element, winrt::UIElement::VisibilityProperty());

    if (auto const control = element.try_as<winrt::Control>())
    {
        observe(control, winrt::Control::IsEnabledProperty());
        observe(control, winrt::UIElement::IsTabStopProperty());
    }

    // The three properties above describe elements that already exist. They say nothing
    // about the subtree gaining or losing elements, which a TableViewTemplateColumn does
    // whenever an app-supplied CellTemplate swaps what sits inside an otherwise stable
    // presenter. Without this, a stale "not interactive" verdict would survive the arrival
    // of a focusable control and SetCellContentAccessibilityViewRaw would then hide it.
    // XAML raises no subtree-changed event, so observe the properties that drive the swap.
    if (auto const presenter = element.try_as<winrt::ContentPresenter>())
    {
        observe(presenter, winrt::ContentPresenter::ContentProperty());
        observe(presenter, winrt::ContentPresenter::ContentTemplateProperty());
    }

    if (auto const contentControl = element.try_as<winrt::ContentControl>())
    {
        observe(contentControl, winrt::ContentControl::ContentProperty());
        observe(contentControl, winrt::ContentControl::ContentTemplateProperty());
    }

    if (auto const itemsControl = element.try_as<winrt::ItemsControl>())
    {
        observe(itemsControl, winrt::ItemsControl::ItemsSourceProperty());
    }
}

void TableViewCellAutomationPeer::InvalidateAutomationContentViewCache()
{
    m_hasInteractiveAutomationContent.reset();
}

void TableViewCellAutomationPeer::ResetAutomationContentViewCache() noexcept
{
    m_hasInteractiveAutomationContent.reset();
    m_automationContent = nullptr;
    m_automationContentItem.Track(nullptr);
    m_automationContentPropertyChangedRevokers.clear();
}

hstring TableViewCellAutomationPeer::GetHelpTextCore()
{
    auto const row = m_row.get();
    if (row && !GetTrackedTableForRow(row))
    {
        ThrowElementNotAvailable();
    }

    auto const helpText = __super::GetHelpTextCore();
    if (helpText.empty())
    {
        return helpText;
    }

    auto const record = TableViewDetails::GetRecord(Owner().try_as<winrt::FrameworkElement>());

    // Resolved here, not at attach, where the cell's binding may not have produced a value yet.
    // Gated on the record so text the app set is never dropped.
    if (record && !record->PublishedHelpText.empty() &&
        helpText == record->PublishedHelpText &&
        helpText == GetCellValueText())
    {
        return {};
    }

    return helpText;
}

int32_t TableViewCellAutomationPeer::GetRowIndex()
{
    if (auto const row = m_row.get())
    {
        return GetRowIndex(row, GetTrackedTableForRow(row));
    }

    return -1;
}

int32_t TableViewCellAutomationPeer::GetRowIndex(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (tableView)
    {
        m_lastOwningTable = winrt::make_weak(tableView);

        // Use the row peer's coordinate basis and avoid a visual-tree walk per realized cell.
        if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
        {
            if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
            {
                m_lastKnownRowIndex = rowIndex;
                return rowIndex;
            }
        }
    }

    // TableView exposes no public row-index API, so fall back to the hosting ItemsRepeater for
    // a row whose owner is not resolvable yet.
    auto const rowItem = row.DataContext();
    if (!m_item.SameIdentityAs(rowItem))
    {
        return -1;
    }

    winrt::DependencyObject parent = winrt::VisualTreeHelper::GetParent(row);
    while (parent)
    {
        if (auto repeater = parent.try_as<winrt::ItemsRepeater>())
        {
            if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
            {
                m_lastKnownRowIndex = rowIndex;
                return rowIndex;
            }
        }
        parent = winrt::VisualTreeHelper::GetParent(parent);
    }

    return -1;
}

bool TableViewCellAutomationPeer::IsVirtualized()
{
    if (GetRowIndex() >= 0)
    {
        return false;
    }

    auto const tableView = m_lastOwningTable.get();
    if (!tableView || m_lastKnownRowIndex < 0)
    {
        return false;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    const auto rowIndex = GetTrackedItemIndex(tableView);
    if (rowIndex < 0 || rowIndex >= tableImpl->GetRowCountInternal())
    {
        return false;
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    const bool isVirtualized = repeater && !repeater.TryGetElement(rowIndex);
    if (isVirtualized)
    {
        m_lastKnownRowIndex = rowIndex;
    }
    return isVirtualized;
}

void TableViewCellAutomationPeer::Realize()
{
    if (!IsVirtualized())
    {
        return;
    }

    // IVirtualizedItemProvider::Realize is synchronous: on return the client immediately re-queries
    // this provider and expects the element realized, with VirtualizedItem no longer supported.
    // Deferring to the dispatcher hands the client a still-virtualized provider.
    RealizeCore();
}

void TableViewCellAutomationPeer::RealizeCore()
{
    auto const tableView = m_lastOwningTable.get();
    const auto rowIndex = tableView ? GetTrackedItemIndex(tableView) : -1;
    if (!tableView || rowIndex < 0)
    {
        ThrowElementNotAvailable();
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    if (rowIndex >= tableImpl->GetRowCountInternal())
    {
        ThrowElementNotAvailable();
    }

    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    if (!repeater)
    {
        ThrowElementNotAvailable();
    }

    winrt::UIElement element{ nullptr };
    try
    {
        element = repeater.TryGetElement(rowIndex);
        if (!element)
        {
            element = repeater.GetOrCreateElement(rowIndex);
        }
    }
    catch (...)
    {
        ThrowElementNotAvailable();
    }

    if (auto const row = element.try_as<winrt::TableViewRow>())
    {
        // The peer is bound to its own cell element; if the repeater realized the item into another
        // row, this provider cannot describe it, so fail rather than report a realized element.
        if (!IsTrackedRow(row, tableView) || row != m_row.get() || !IsOwnerCurrentCellOfRow(row))
        {
            ThrowElementNotAvailable();
        }

        if (auto const cell = GetRealizedCellFromRow(row))
        {
            if (auto const cellElement = cell.try_as<winrt::FrameworkElement>())
            {
                cellElement.StartBringIntoView();
                return;
            }
        }
    }

    if (auto const frameworkElement = element.try_as<winrt::FrameworkElement>())
    {
        frameworkElement.StartBringIntoView();
    }
}

winrt::TableView TableViewCellAutomationPeer::GetTrackedTableForRow(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return nullptr;
    }

    auto const tableView = winrt::get_self<TableViewRow>(row)->GetOwningTableView();
    if (tableView && IsTrackedRow(row, tableView))
    {
        m_lastOwningTable = winrt::make_weak(tableView);
        return tableView;
    }

    return nullptr;
}

winrt::IInspectable TableViewCellAutomationPeer::GetTrackedItem() const
{
    return m_item.Resolve();
}

int32_t TableViewCellAutomationPeer::GetTrackedItemIndex(winrt::TableView const& tableView)
{
    if (!tableView || !m_item.IsTracking())
    {
        return -1;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!view)
    {
        return -1;
    }

    const int32_t count = view.Count();
    if (m_lastKnownRowIndex >= 0 && m_lastKnownRowIndex < count)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(m_lastKnownRowIndex));
            m_item.SameIdentityAs(candidate))
        {
            return m_lastKnownRowIndex;
        }
    }

    int32_t occurrence = 0;
    for (int32_t index = 0; index < count; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(index));
            m_item.SameIdentityAs(candidate))
        {
            if (m_trackedItemOccurrence < 0 || occurrence == m_trackedItemOccurrence)
            {
                return index;
            }
            ++occurrence;
        }
    }

    return -1;
}

int32_t TableViewCellAutomationPeer::GetItemOccurrenceAtIndex(
    winrt::TableView const& tableView,
    winrt::IInspectable const& item,
    int32_t targetIndex)
{
    if (!tableView || !item || targetIndex < 0)
    {
        return -1;
    }

    auto const tableImpl = winrt::get_self<TableView>(tableView);
    auto const repeater = tableImpl->GetRowsRepeaterInternal();
    auto const view = repeater ? repeater.ItemsSourceView() : nullptr;
    if (!view || targetIndex >= view.Count())
    {
        return -1;
    }

    int32_t occurrence = 0;
    for (int32_t index = 0; index <= targetIndex; ++index)
    {
        if (auto const candidate = tableImpl->UnwrapDataItem(view.GetAt(index));
            candidate && TableView::SameInspectableIdentity(candidate, item))
        {
            if (index == targetIndex)
            {
                return occurrence;
            }
            ++occurrence;
        }
    }

    return -1;
}

bool TableViewCellAutomationPeer::IsTrackedRow(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (!row || !tableView)
    {
        return false;
    }

    auto const rowItem = winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext());
    if (!rowItem)
    {
        return false;
    }

    if (!m_item.IsTracking())
    {
        TrackRowItem(row, tableView);
        return true;
    }

    if (!m_item.SameIdentityAs(rowItem))
    {
        return false;
    }

    if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
    {
        if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
        {
            const auto occurrence = GetItemOccurrenceAtIndex(tableView, rowItem, rowIndex);
            if (m_trackedItemOccurrence >= 0 && occurrence != m_trackedItemOccurrence)
            {
                return false;
            }

            TrackRowItem(row, tableView);
        }
    }

    return true;
}

void TableViewCellAutomationPeer::TrackRowItem(winrt::TableViewRow const& row, winrt::TableView const& tableView)
{
    if (row && tableView)
    {
        if (auto const item = winrt::get_self<TableView>(tableView)->UnwrapDataItem(row.DataContext()))
        {
            auto const previousOccurrence = m_trackedItemOccurrence;
            auto const sameItem = m_item.SameIdentityAs(item);
            auto trackedItemOccurrence = -1;
            if (auto const repeater = winrt::get_self<TableView>(tableView)->GetRowsRepeaterInternal())
            {
                if (const auto rowIndex = repeater.GetElementIndex(row); rowIndex >= 0)
                {
                    m_lastKnownRowIndex = rowIndex;
                    trackedItemOccurrence = GetItemOccurrenceAtIndex(tableView, item, rowIndex);
                }
            }
            if (!sameItem || previousOccurrence != trackedItemOccurrence)
            {
                ResetAutomationContentViewCache();
            }
            m_item.Track(item);
            m_trackedItemOccurrence = trackedItemOccurrence;
        }
    }
}

winrt::UIElement TableViewCellAutomationPeer::GetRealizedCellFromRow(winrt::TableViewRow const& row)
{
    if (!row)
    {
        return nullptr;
    }

    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    if (!rowImpl)
    {
        return nullptr;
    }

    auto const cellsHost = rowImpl->GetCellsHostPanelInternal();
    if (!cellsHost)
    {
        return nullptr;
    }

    if (auto const column = m_column.get())
    {
        for (auto const& child : cellsHost.Children())
        {
            auto const cell = child.try_as<winrt::UIElement>();
            if (cell && rowImpl->GetCellOwningColumn(cell) == column)
            {
                return cell;
            }
        }
    }

    return rowImpl->GetVisibleCellInternal(m_columnIndex);
}

int32_t TableViewCellAutomationPeer::Row()
{
    auto const row = m_row.get();
    auto const tableView = row ? GetTrackedTableForRow(row) : nullptr;
    if (!tableView || !IsOwnerCurrentCellOfRow(row))
    {
        ThrowElementNotAvailable();
    }

    if (const auto rowIndex = GetRowIndex(row, tableView); rowIndex >= 0)
    {
        return rowIndex;
    }

    // Between recycle and re-prepare a live row has no repeater index; only an item that has left
    // the source makes this provider stale.
    if (const auto itemIndex = GetTrackedItemIndex(tableView); itemIndex >= 0)
    {
        return itemIndex;
    }
    ThrowElementNotAvailable();
}

int32_t TableViewCellAutomationPeer::GetRowIndexInternal()
{
    return GetRowIndex();
}

int32_t TableViewCellAutomationPeer::Column()
{
    auto const row = m_row.get();
    if (!row || !GetTrackedTableForRow(row) || !IsOwnerCurrentCellOfRow(row))
    {
        ThrowElementNotAvailable();
    }

    // Computed live, mirroring Row(): a cached index goes stale as soon as a column is hidden or
    // shown underneath a client holding this provider.
    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    if (auto const cellsHost = rowImpl->GetCellsHostPanelInternal())
    {
        auto const cell = Owner().try_as<winrt::UIElement>();
        auto const cellChildren = cellsHost.Children();
        const auto count = cellChildren.Size();
        int32_t visibleColumnIndex = 0;

        // Same walk and predicate as TableViewAutomationPeer::VisibleColumnToChildIndex and
        // the row peer's GetChildrenCore, so all three agree on the coordinate.
        for (uint32_t i = 0; i < count; ++i)
        {
            auto const child = cellChildren.GetAt(i).try_as<winrt::UIElement>();
            if (!child || !IsVisibleColumn(rowImpl->GetCellOwningColumn(child)))
            {
                continue;
            }

            if (child == cell)
            {
                return visibleColumnIndex;
            }
            ++visibleColumnIndex;
        }

        // Still hosted, but its column is collapsed: the cell has no visible-column coordinate.
        ThrowElementNotAvailable();
    }

    if (m_columnIndex < 0)
    {
        ThrowElementNotAvailable();
    }
    return m_columnIndex;
}

// A same-row rebuild (column reorder or removal, CellTemplate change) replaces the cells while the
// row and its item stay live, so a tracked row alone does not prove this cell is current.
bool TableViewCellAutomationPeer::IsOwnerCurrentCellOfRow(winrt::TableViewRow const& row)
{
    auto const rowImpl = winrt::get_self<TableViewRow>(row);
    auto const cellsHost = rowImpl->GetCellsHostPanelInternal();
    if (!cellsHost)
    {
        return true;
    }

    auto const cell = Owner().try_as<winrt::UIElement>();
    return cell &&
        winrt::VisualTreeHelper::GetParent(cell).try_as<winrt::Panel>() == cellsHost &&
        rowImpl->GetCellOwningColumn(cell) == m_column.get();
}

int32_t TableViewCellAutomationPeer::RowSpan()
{
    return 1;
}

int32_t TableViewCellAutomationPeer::ColumnSpan()
{
    return 1;
}

winrt::IRawElementProviderSimple TableViewCellAutomationPeer::ContainingGrid()
{
    // The containing grid is the owning TableView's automation peer.
    if (auto const row = m_row.get())
    {
        if (auto const owner = GetTrackedTableForRow(row))
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(owner))
            {
                return ProviderFromPeer(peer);
            }
        }
    }

    return nullptr;
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewCellAutomationPeer::GetRowHeaderItems()
{
    // TableView has no row-header concept.
    return {};
}

winrt::com_array<winrt::IRawElementProviderSimple> TableViewCellAutomationPeer::GetColumnHeaderItems()
{
    // Return the corresponding column header provider using the same peer construction path
    // as the table-level header enumeration.
    std::vector<winrt::IRawElementProviderSimple> headers;

    if (auto const column = m_column.get())
    {
        if (auto const row = m_row.get())
        {
            // Hidden headers are named into the cell instead, so reporting them would repeat the header.
            if (auto const owner = GetTrackedTableForRow(row); owner && IsColumnHeaderReachable(owner))
            {
                // Resolve the visual's peer even when the app supplies a custom table peer.
                auto const headerPeer = GetRealizedColumnHeaderPeer(owner, column);

                // A provider array must not contain nulls - UIA marshals every element. An empty
                // array correctly reports "this cell has no reachable column header".
                if (headerPeer)
                {
                    if (auto const provider = ProviderFromPeer(headerPeer))
                    {
                        headers.push_back(provider);
                    }
                }
            }
        }
    }

    return winrt::com_array(headers);
}

// ----- IValueProvider -----

winrt::hstring TableViewCellAutomationPeer::Value()
{
    return GetCellValueText();
}

// Editing is not part of this release, so the value is never settable.
bool TableViewCellAutomationPeer::IsReadOnly()
{
    return true;
}

void TableViewCellAutomationPeer::SetValue(winrt::hstring const& /*value*/)
{
    throw winrt::hresult_error(E_NOTIMPL, L"This cell is read-only.");
}
