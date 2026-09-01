// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TableViewRow.h"
#include "TableViewToolTipHelpers.h"
#include "TableView.h"
#include "TableViewColumn.h"
#include "TableViewCellsPanel.h"
#include "TableViewRowAutomationPeer.h"
#include "TVDiag.h"

static constexpr std::wstring_view s_CellsHostPartName{ L"PART_CellsHost"sv };

namespace
{
    constexpr winrt::Thickness s_verticalThickness{ 0, 0, 1, 0 };
    constexpr winrt::Thickness s_zeroThickness{ 0, 0, 0, 0 };

    // Shared transparent fill for cell wrappers. Cached because rows and cells are rebuilt on every
    // scroll, and a fresh brush per cell is pure allocation for a value that never varies.
    winrt::Brush TransparentBrush()
    {
        static const winrt::SolidColorBrush s_transparent{ winrt::Colors::Transparent() };
        return s_transparent;
    }

    bool WantsHorizontalLines(winrt::TableViewGridLinesVisibility visibility) noexcept
    {
        return visibility == winrt::TableViewGridLinesVisibility::Horizontal ||
            visibility == winrt::TableViewGridLinesVisibility::All;
    }

    bool WantsVerticalLines(winrt::TableViewGridLinesVisibility visibility) noexcept
    {
        return visibility == winrt::TableViewGridLinesVisibility::Vertical ||
            visibility == winrt::TableViewGridLinesVisibility::All;
    }

    // Slop allowed between the two presses of a double-click, in DIPs.
    //
    // SM_CXDOUBLECLK is in PHYSICAL pixels, so it must be divided by the element's rasterization
    // scale to be comparable with the DIP-space positions this gesture works in. Without that the
    // effective tolerance halves at 200% - a user whose two presses land 6 DIPs apart is inside the
    // system tolerance but outside ours, and the double-click is silently dropped.
    //
    // Read per press rather than cached: the metric changes with the mouse control panel and the
    // scale changes when the window moves between monitors, and a cached value never sees either.
    // GetSystemMetrics is a cheap cached read in user32, not a round trip.
    double GetDoubleClickSlop(winrt::UIElement const& element)
    {
        const auto cx = static_cast<double>(::GetSystemMetrics(SM_CXDOUBLECLK));
        const auto cy = static_cast<double>(::GetSystemMetrics(SM_CYDOUBLECLK));
        const double physical = (std::max)((std::max)(cx, cy), 4.0) / 2.0;

        double scale = 1.0;
        if (element)
        {
            if (auto const root = element.XamlRoot())
            {
                const auto rasterization = root.RasterizationScale();
                if (rasterization > 0.0)
                {
                    scale = rasterization;
                }
            }
        }

        return physical / scale;
    }

    // PointerPoint.Timestamp is in microseconds; the system double-click time is in milliseconds.
    // Not cached, so a change made in the mouse control panel takes effect immediately.
    uint64_t GetDoubleClickIntervalMicroseconds()
    {
        return static_cast<uint64_t>(::GetDoubleClickTime()) * 1000ull;
    }
}

TableViewRow::TableViewRow()
{
    SetDefaultStyleKey(this);

    auto weakRow = get_weak();

    // handledEventsToo: a row that participates in selection marks the press handled, which also
    // suppresses XAML's gesture recognizer - this registration is what keeps begin-edit reachable.
    m_editingPointerPressedHandler = winrt::PointerEventHandler(
        [weakRow](winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
        {
            if (auto strongRow = weakRow.get())
            {
                strongRow->OnPointerPressedForEditing(sender, args);
            }
        });
    AddHandler(winrt::UIElement::PointerPressedEvent(), winrt::box_value(m_editingPointerPressedHandler), true /* handledEventsToo */);

    // auto_revoke owns this self-event subscription until row destruction.
    m_dataContextChangedRevoker = DataContextChanged(
        winrt::auto_revoke,
        [weakRow](winrt::FrameworkElement const& sender, winrt::DataContextChangedEventArgs const& args)
        {
            if (auto strongRow = weakRow.get())
            {
                strongRow->OnDataContextChanged(sender, args);
            }
        });

    // Keep CommonStates VSM in sync with IsEnabled so the Disabled
    // state activates when consumers toggle row IsEnabled at runtime.
    // auto_revoke owns this self-event subscription until row destruction.
    m_isEnabledChangedRevoker = IsEnabledChanged(
        winrt::auto_revoke,
        [weakRow](winrt::IInspectable const& sender, winrt::DependencyPropertyChangedEventArgs const& args)
        {
            if (auto strongRow = weakRow.get())
            {
                strongRow->OnIsEnabledChanged(sender, args);
            }
        });
}

void TableViewRow::OnIsEnabledChanged(
    const winrt::IInspectable& /*sender*/,
    const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateVisualState(true /* useTransitions */);
}

void TableViewRow::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    m_cellsHost.set(GetTemplateChild(hstring{ s_CellsHostPartName }).try_as<winrt::Panel>());

    // Let the panel recognise this row's editing cell so it can keep it out of the Auto-width pass.
    if (auto const cellsPanel = m_cellsHost.get().try_as<winrt::TableViewCellsPanel>())
    {
        winrt::get_self<TableViewCellsPanel>(cellsPanel)->SetOwningRowInternal(*this);
    }

    RebuildCells();

    UpdateVisualState(false /* useTransitions */);
}

winrt::AutomationPeer TableViewRow::OnCreateAutomationPeer()
{
    return winrt::make<TableViewRowAutomationPeer>(*this);
}

// Typed accessor for the owning TableView.
winrt::TableView TableViewRow::GetOwningTableView()
{
    // Keep the owner weak; callers acquire a strong ref only for synchronous work.
    return m_owningTableView.get();
}

winrt::TableViewColumn TableViewRow::GetCellOwningColumn(const winrt::UIElement& cellElement) const
{
    if (auto cellFE = cellElement.try_as<winrt::FrameworkElement>())
    {
        if (auto column = cellFE.Tag().try_as<winrt::TableViewColumn>())
        {
            return column;
        }
    }

    return nullptr;
}

void TableViewRow::DetachColumnsSubscription()
{
    // Clearing the revoker detaches from the previously observed Columns vector.
    if (m_observedColumns.get())
    {
        m_columnsVectorChangedRevoker = {};
        m_observedColumns = nullptr;
    }
}

void TableViewRow::AttachColumnsSubscription(winrt::TableView const& owner)
{
    if (!owner)
    {
        return;
    }

    // Cast the ABI vector to its observable backing type for VectorChanged.
    if (auto observable = owner.Columns().try_as<winrt::IObservableVector<winrt::TableViewColumn>>())
    {
        auto weakRow = get_weak();
        m_columnsVectorChangedRevoker = observable.VectorChanged(
            winrt::auto_revoke,
            [weakRow](winrt::IObservableVector<winrt::TableViewColumn> const& sender, winrt::IVectorChangedEventArgs const& args)
            {
                if (auto strongRow = weakRow.get())
                {
                    strongRow->OnColumnsVectorChanged(sender, args);
                }
            });
        m_observedColumns = observable;
    }
}

void TableViewRow::SetOwningTableViewInternal(winrt::TableView const& owner)
{
    auto const currentOwner = GetOwningTableView();
    // Compare through the typed accessor so unchanged weak refs hit the no-op path.
    if (currentOwner == owner)
    {
        return;
    }

    // Single-writer: the recycle cycle clears the owner to null between tables; a direct
    // table-A -> table-B switch (no intervening clear) is a cross-table leak, so reject it.
    if (currentOwner && owner)
    {
        // Runs inside an ItemsRepeater callback; an escaping throw would failfast.
        // Assert in debug, no-op in retail (keep current owner, skip re-attach).
        MUX_ASSERT_MSG(false, "TableViewRow re-owned without an intervening clear");
        return;
    }

    DetachColumnsSubscription();

    if (owner)
    {
        m_owningTableView = winrt::make_weak(owner);
    }
    else
    {
        m_owningTableView = nullptr;
        // Reset transient interaction state so a row recycled while hovered/pressed
        // re-enters the pool in Normal state (ListViewItem parity), not a stale tint.
        m_isPointerOver = false;
        m_isPressed = false;
        m_selectOnPointerRelease = false;

        // Selection belongs to the item, not the container: a pooled row must not carry selected
        // chrome onto its next item. RefreshRowSelectionState restamps it on the way back in.
        IsSelected(false);

        // Begin-edit gesture state must go too. A row recycled away and back to the SAME item
        // inside the double-click interval would otherwise turn the next single click into an
        // edit, and the stale trackers keep the previous item and column alive.
        ResetPressState();

        UpdateVisualState(false);
    }

    // Observe the new owner's Columns vector (no-op on recycle-out when owner is null).
    AttachColumnsSubscription(owner);

    RebuildCells();
}

// Rewire realized rows when Columns changes but the owner identity does not.
void TableViewRow::RefreshColumnsSubscriptionInternal()
{
    DetachColumnsSubscription();

    auto owner = GetOwningTableView();
    if (!owner)
    {
        return;
    }

    AttachColumnsSubscription(owner);

    if (auto host = m_cellsHost.get())
    {
        host.Children().Clear();
    }

    // Rebuild cells against the new Columns vector.
    RebuildCells();
}

void TableViewRow::OnDataContextChanged(
    const winrt::FrameworkElement& /*sender*/,
    const winrt::DataContextChangedEventArgs& /*args*/)
{
    // On recycle ItemsRepeater updates the row's DataContext; cells pick up the new item reactively
    // via inheritance (they are not restamped). RebuildCells is still called so index-dependent visuals
    // (alternating-row banding, frozen pinning) refresh for the new position. RebuildCells guards
    // against re-entry from child DataContext propagation.
    RebuildCells();
}

void TableViewRow::OnColumnsVectorChanged(
    const winrt::IObservableVector<winrt::TableViewColumn>& /*sender*/,
    const winrt::IVectorChangedEventArgs& /*args*/)
{
    // Mutating Columns triggers a cell rebuild against the new column set. Coalesce a burst of column
    // changes into a single rebuild on the next tick. No explicit Children().Clear() is needed:
    // RebuildCells' restamp fast-path forces a full rebuild (which clears) whenever the column set
    // actually changed, and keeping the old cells until the tick avoids an empty-cell flash.
    QueueRebuildCells();
}

void TableViewRow::QueueRebuildCells()
{
    // No cell host yet (template not applied); the owner-set / ApplyTemplate path builds cells.
    if (!m_cellsHost.get())
    {
        return;
    }

    // A rebuild is already scheduled for this tick -- collapse the burst into one.
    if (m_rebuildCellsQueued)
    {
        return;
    }

    auto dispatcher = DispatcherQueue();
    if (!dispatcher)
    {
        // No dispatcher (teardown) -- rebuild synchronously so cells are not left stale.
        RebuildCells();
        return;
    }

    m_rebuildCellsQueued = true;
    auto weakRow = get_weak();
    if (!dispatcher.TryEnqueue([weakRow]()
        {
            if (auto strongRow = weakRow.get())
            {
                strongRow->m_rebuildCellsQueued = false;
                try
                {
                    strongRow->RebuildCells();
                }
                catch (...)
                {
                    // Coalesced cell rebuild is best-effort; never fail-fast the dispatcher.
                }
            }
        }))
    {
        // Enqueue failed -- fall back to a synchronous rebuild so cells are not left stale.
        m_rebuildCellsQueued = false;
        RebuildCells();
    }
}

void TableViewRow::OnPointerPressed(winrt::PointerRoutedEventArgs const& args)
{
    if (args.Handled())
    {
        return;
    }

    auto pointerPoint = args.GetCurrentPoint(*this);
    auto props = pointerPoint.Properties();

    // Mouse/pen: primary button only - a right-click opens a context menu and must not select.
    // Touch reports no pressed button, so it is admitted on device type instead, matching the
    // begin-edit gesture. Without this a touch press never arms the release-time selection.
    const auto deviceType = args.Pointer().PointerDeviceType();
    if (deviceType != winrt::Microsoft::UI::Input::PointerDeviceType::Touch &&
        !props.IsLeftButtonPressed())
    {
        return;
    }

    m_isPressed = true;
    UpdateVisualState(true);

    // Move keyboard focus to the row so the next keyboard interaction targets it.
    Focus(winrt::FocusState::Pointer);

    // Selection state lives on the control; the row is just where the press lands. Left unhandled
    // so the begin-edit handler for this same press still runs. Commits on RELEASE for every
    // pointer type (ListViewBaseItem parity) - committing on press would select the row a pan
    // started on, or one the user drags away from and cancels.
    m_selectOnPointerRelease = true;
    // Remember WHICH pointer armed it: with two contacts on the same row, the second one's release
    // or cancel must not commit (or discard) the first one's pending selection.
    m_selectPointerId = args.Pointer().PointerId();
}

void TableViewRow::OnPointerEntered(winrt::PointerRoutedEventArgs const& /*args*/)
{
    m_isPointerOver = true;
    UpdateVisualState(true);
}

void TableViewRow::OnPointerExited(winrt::PointerRoutedEventArgs const& args)
{
    m_isPointerOver = false;
    m_isPressed = false;

    // The contact left the row, so a deferred selection is no longer a tap on it. Only the pointer
    // that armed it may disarm it: with two contacts on the same row, the other one leaving must
    // not cancel this one's pending selection. Matches the check in OnPointerReleased.
    if (args.Pointer().PointerId() == m_selectPointerId)
    {
        m_selectOnPointerRelease = false;
    }

    UpdateVisualState(true);
}

void TableViewRow::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    m_isPressed = false;

    // Deferred selection: the pointer came up on this row without a pan or a capture loss taking
    // it away, so it was a tap. Only the pointer that armed it can commit or disarm it.
    const bool isArmingPointer = args.Pointer().PointerId() == m_selectPointerId;
    const bool selectNow = m_selectOnPointerRelease && isArmingPointer;
    if (isArmingPointer)
    {
        m_selectOnPointerRelease = false;
    }

    if (selectNow)
    {
        if (auto const owner = GetOwningTableView())
        {
            winrt::get_self<TableView>(owner)->OnRowPointerSelect(*this);
        }
    }

    UpdateVisualState(true);
}

void TableViewRow::OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args)
{
    m_isPressed = false;

    // A ScrollViewer took the pointer for a pan; the gesture was a scroll, not a tap. Only the
    // arming pointer disarms, so a second contact panning does not cancel the first one's tap.
    if (args.Pointer().PointerId() == m_selectPointerId)
    {
        m_selectOnPointerRelease = false;
    }

    UpdateVisualState(true);
}

void TableViewRow::OnPointerCanceled(winrt::PointerRoutedEventArgs const& args)
{
    // Palm rejection or a system gesture. Can arrive without a preceding PointerCaptureLost, which
    // would otherwise leave the latch set and let the next unrelated release select this row.
    m_isPressed = false;

    if (args.Pointer().PointerId() == m_selectPointerId)
    {
        m_selectOnPointerRelease = false;
    }

    UpdateVisualState(true);
}

void TableViewRow::UpdateVisualState(bool useTransitions)
{
    // One GoToState into one group: the selected states share CommonStates so nothing depends on
    // call order (TreeViewItem / ItemContainer pattern). Disabled wins, for ListViewItem parity.
    std::wstring_view state;
    if (IsSelected())
    {
        state = !IsEnabled()      ? L"SelectedDisabled"sv
              : m_isPressed       ? L"SelectedPressed"sv
              : m_isPointerOver   ? L"SelectedPointerOver"sv
                                  : L"Selected"sv;
    }
    else
    {
        state = !IsEnabled()      ? L"Disabled"sv
              : m_isPressed       ? L"Pressed"sv
              : m_isPointerOver   ? L"PointerOver"sv
                                  : L"Normal"sv;
    }

    winrt::VisualStateManager::GoToState(*this, hstring{ state }, useTransitions);
}

void TableViewRow::SetIsSelectedInternal(bool isSelected)
{
    if (IsSelected() == isSelected)
    {
        return;
    }

    IsSelected(isSelected);
    UpdateVisualState(true /* useTransitions */);
}

void TableViewRow::RefreshDensity()
{
    // Re-read density metrics and restamp built-in cell padding.
    RebuildCells();
}

void TableViewRow::RefreshCells()
{
    if (auto host = m_cellsHost.get())
    {
        host.Children().Clear();
    }

    // Regenerate realized cells after runtime cell-content changes.
    RebuildCells();
}

void TableViewRow::RefreshColumnVisibility(const winrt::TableViewColumn& column, winrt::Visibility visibility)
{
    if (!column)
    {
        return;
    }

    if (auto cell = TableViewCellsPanel::CellForColumn(m_cellsHost.get(), column))
    {
        cell.Visibility(visibility);
    }
}

double TableViewRow::MeasuredWidthForColumn(const winrt::TableViewColumn& column) const
{
    if (auto host = m_cellsHost.get())
    {
        if (auto cellsPanel = host.try_as<winrt::TableViewCellsPanel>())
        {
            return winrt::get_self<TableViewCellsPanel>(cellsPanel)->MeasuredWidthForColumn(column);
        }
    }

    return 0.0;
}

void TableViewRow::InvalidateCells()
{
    if (auto host = m_cellsHost.get())
    {
        host.InvalidateMeasure();
    }
}

void TableViewRow::RebuildCells()
{
    auto host = m_cellsHost.get();
    if (!host)
    {
        return;
    }

    // Re-entry guard. See OnDataContextChanged. Cell tooltips no longer call into app code from
    // this path - they are bindings evaluated by the framework - so a plain guard is enough.
    if (m_isRebuildingCells)
    {
        return;
    }
    m_isRebuildingCells = true;
    // Synchronous RAII guard: captures this only until RebuildCells returns.
    auto rebuildGuard = wil::scope_exit([this]() { m_isRebuildingCells = false; });

    auto owner = GetOwningTableView();
    if (!owner)
    {
        // Keep realized cells on recycle-out so the next ElementPrepared restamps
        // instead of re-generating every cell (the restamp fast-path exists for this).
        return;
    }

    // Density: apply the owning TableView's row min-height (Fluent Standard = 40,
    // Compact = 30, Comfortable = 48; resolved from the TableViewRowMinHeight* resources).
    const auto ownerImpl = winrt::get_self<TableView>(owner);
    const auto rowMinHeight = ownerImpl->GetDensityRowMinHeight();
    MinHeight(rowMinHeight);

    auto columns = owner.Columns();
    auto isOwnedColumn = [&owner](winrt::TableViewColumn const& column)
    {
        return column && winrt::get_self<TableViewColumn>(column)->GetOwningTableView() == owner;
    };
    if (!columns)
    {
        // Columns went away entirely. Clearing here without closing the edit first would rip a live
        // editor out of the tree while the control still believed it was editing - and it wedges
        // permanently: the next pass sees zero children AND zero columns, takes the restamp
        // fast-path below, and returns before ever reaching the teardown. IsEditing would stay true
        // for the lifetime of the control, with no editor and no way for the app to recover.
        if (m_editingCellWrapper.get())
        {
            if (auto const editOwner = GetOwningTableView())
            {
                winrt::get_self<TableView>(editOwner)->TerminateEditWithoutVisualRestore(true /* insideLayoutPass */);
            }
            else
            {
                AbandonCellEdit();
            }

            // Refused to close (an edit still in its Beginning window). Leave the cells alone
            // rather than orphan the editor; the next pass retries once the edit has closed.
            if (m_editingCellWrapper.get())
            {
                return;
            }
        }

        host.Children().Clear();
        return;
    }

    auto dataContext = DataContext();
    winrt::IInspectable dataItem = dataContext;
    const auto children = host.Children();

    uint32_t nonNullColumnCount = 0;
    for (auto const& column : columns)
    {
        if (isOwnedColumn(column))
        {
            ++nonNullColumnCount;
        }
    }

    bool canRestampCells = children.Size() == nonNullColumnCount;

    // The fast-path reuses cells and deliberately does not close an open edit. That is only sound
    // while the editor is still one of those cells. If it has been orphaned - its wrapper is no
    // longer a child - reusing them would leave the control editing an element outside the tree,
    // so force the rebuild path, which tears the edit down.
    if (canRestampCells)
    {
        if (auto const editingWrapper = m_editingCellWrapper.get())
        {
            bool stillAttached = false;
            for (uint32_t i = 0; i < children.Size(); ++i)
            {
                if (children.GetAt(i) == editingWrapper)
                {
                    stillAttached = true;
                    break;
                }
            }

            if (!stillAttached)
            {
                canRestampCells = false;
            }
        }
    }

    uint32_t childIndex = 0;
    if (canRestampCells)
    {
        for (auto const& column : columns)
        {
            if (!isOwnedColumn(column))
            {
                continue;
            }

            auto cellWrapper = children.GetAt(childIndex).try_as<winrt::Border>();
            if (!cellWrapper || cellWrapper.Tag().try_as<winrt::TableViewColumn>() != column)
            {
                canRestampCells = false;
                break;
            }
            ++childIndex;
        }
    }

    if (canRestampCells)
    {
        const auto cellPadding = ownerImpl->GetDensityCellPadding();
        childIndex = 0;
        for (auto const& column : columns)
        {
            if (!isOwnedColumn(column))
            {
                continue;
            }

            auto cellWrapper = children.GetAt(childIndex).as<winrt::Border>();
            // Do NOT re-push data here. Cells inherit the row's DataContext (ItemsRepeater updates it
            // on recycle) and bind to it reactively (TextColumn Text, TemplateColumn Content), so a
            // recycled row's *data* updates without setting DataContext/Content on a live, in-tree cell
            // during the ItemsRepeater measure pass -- that data mutation (the value always changes on
            // recycle and is layout-affecting) is what re-entered framework layout and tripped a
            // re-entrancy assertion (0xc0000420) on scroll. Only per-column / per-density visuals are
            // refreshed below; on a pure scroll-recycle these are equal-valued no-ops (columns and
            // density unchanged), so they do not re-invalidate layout. Keep it that way -- if any of
            // these is ever made to vary per data item, restore an off-tree update to avoid re-entry.
            cellWrapper.Visibility(column.Visibility());
            cellWrapper.MinHeight(rowMinHeight);

            if (auto cellElement = cellWrapper.Child().try_as<winrt::FrameworkElement>())
            {
                if (auto textBlock = cellElement.try_as<winrt::TextBlock>())
                {
                    textBlock.Padding(cellPadding);
                }
            }

            ++childIndex;
        }

        // Pin immediately so recycled frozen cells do not wait for the next scroll.
        if (auto owningView = GetOwningTableView())
        {
            winrt::get_self<TableView>(owningView)->PinFrozenColumnsForRow(*this);
        }

        RefreshGridLines();
        RefreshRowBackground();
        return;
    }

    // Column adds/removes/reorders or template changes rebuild cells; width-only changes are picked
    // up automatically by the cell panel's arrange (it reads each column's resolved ActualWidth), so
    // no cell rebuild is needed for those.
    //
    // This path destroys every cell, so an open editor cannot survive it. The restamp fast-path above
    // deliberately does NOT tear the edit down: it reuses the cells, so an editor opened on this row
    // stays valid through the routine re-measure that installing it provokes.
    //
    // The teardown must not restore the display child - RebuildCells can run inside the repeater's
    // measure pass. See AbandonCellEdit.
    if (m_editingCellWrapper.get())
    {
        if (auto const editOwner = GetOwningTableView())
        {
            winrt::get_self<TableView>(editOwner)->TerminateEditWithoutVisualRestore(true /* insideLayoutPass */);
        }
        else
        {
            AbandonCellEdit();
        }

        // If the edit still refuses to close, clearing the children would rip the live editor out
        // of the tree while the control still believes it is editing. Leave the cells alone; the
        // rebuild happens on the next pass once the edit has actually closed.
        if (m_editingCellWrapper.get())
        {
            return;
        }
    }

    host.Children().Clear();

    for (auto const& column : columns)
    {
        // Skip entries this TableView rejected so a half-owned column cannot realize cells here.
        if (!isOwnedColumn(column))
        {
            continue;
        }

        // Cell wrapper root.
        winrt::Border cellWrapper;
        cellWrapper.Tag(column);
        cellWrapper.Visibility(column.Visibility());
        cellWrapper.MinHeight(rowMinHeight);

        // A Border with a null Background does not hit-test, so without this only the generated
        // content itself (a TextBlock, which is as wide as its text) would respond to a press. A
        // click anywhere in the cell's padding resolved no column at all: no current cell, and
        // double-click-to-edit silently did nothing on most of the cell's area. Transparent keeps
        // the cell invisible while making the whole cell rectangle pressable.
        cellWrapper.Background(TransparentBrush());
        // No Width binding: TableViewCellsPanel arranges cells at the column's ActualWidth; an explicit
        // Width would defeat the panel's unconstrained Auto measured-width measurement.

        // No local DataContext: the cell inherits the row's DataContext once appended, so recycled
        // rows update reactively via inheritance instead of a live per-recycle push. This is a
        // load-bearing invariant: nothing on the cell path (wrapper Border, PART_CellsHost, or the
        // built-in cell elements) may set a local DataContext, or it would shadow inheritance and the
        // cell would show stale data after recycle. Custom columns (overridable GenerateElementCore)
        // must likewise bind reactively to the inherited DataContext rather than baking in the initial
        // dataItem, since recycled rows are no longer restamped.
        if (auto cellElement = column.GenerateElement(dataItem))
        {
            AttachCellContent(cellWrapper, cellElement);
        }

        // Opt-in per-cell tooltip. Set once: the binding evaluates against the row's inherited
        // DataContext, so a recycled row re-resolves with no realization-time work.
        if (auto const toolTipBinding = column.CellToolTipBinding())
        {
            TableViewDetails::ApplyCellToolTipBinding(cellWrapper, toolTipBinding);
        }

        host.Children().Append(cellWrapper);
    }

    // Pin immediately so rebuilt frozen cells do not wait for the next scroll.
    if (auto owningView = GetOwningTableView())
    {
        winrt::get_self<TableView>(owningView)->PinFrozenColumnsForRow(*this);
    }

    RefreshGridLines();
    RefreshRowBackground();
}

// Recycle-out. Always walks: the restamp fast-path revives tooltips through the binding without
// going through cell creation, so a cached "row has tooltips" flag would go stale and strand app
// content in the recycle pool. Cells with no tooltip cost one attached-property read.
void TableViewRow::ReleaseCellToolTips()
{
    if (auto const host = m_cellsHost.get())
    {
        ClearOwnedCellToolTips(host);
    }
}

void TableViewRow::ClearOwnedCellToolTips(const winrt::Panel& host)
{
    auto const children = host.Children();
    const uint32_t count = children.Size();
    for (uint32_t i = 0; i < count; ++i)
    {
        if (auto const cellWrapper = children.GetAt(i).try_as<winrt::Border>())
        {
            TableViewDetails::ClearOwnedToolTip(cellWrapper);
        }
    }
}

// Installs a generated display element as a cell's content, including the ContentPresenter wiring a
// template column needs. Shared by the cell rebuild and by the post-commit refresh, because
// GenerateElement alone is NOT a complete cell - forgetting the second half leaves a template
// column's Content unbound and the cell blank.
void TableViewRow::AttachCellContent(const winrt::Border& cellWrapper, const winrt::FrameworkElement& cellElement)
{
    if (!cellWrapper || !cellElement)
    {
        return;
    }

    cellWrapper.Child(cellElement);

    // A ContentPresenter cell (built-in TemplateColumn) needs its Content wired to the row item.
    // Bind Content to the WRAPPER Border's inherited DataContext -- which tracks the item across
    // recycle -- rather than the presenter's own DataContext: ContentPresenter pins its DataContext
    // to its Content, so a self-referential binding would freeze after the first item and show stale
    // content on recycled rows. This binding persists across recycles (the restamp fast-path reuses
    // the cell), so no Content is pushed during the measure pass.
    if (auto presenter = cellElement.try_as<winrt::ContentPresenter>())
    {
        if (presenter.ContentTemplate())
        {
            winrt::Microsoft::UI::Xaml::Data::Binding contentBinding;
            contentBinding.Source(cellWrapper);
            contentBinding.Path(winrt::PropertyPath{ L"DataContext" });
            winrt::BindingOperations::SetBinding(
                presenter,
                winrt::ContentPresenter::ContentProperty(),
                contentBinding);
        }
    }
}

// Grid lines only: the row's horizontal bottom line and the vertical per-cell separators (driven by
// GridLinesVisibility). Row background / alternating banding lives in RefreshRowBackground.
void TableViewRow::RefreshGridLines()
{
    auto owner = GetOwningTableView();
    if (!owner)
    {
        return;
    }

    const auto visibility = owner.GridLinesVisibility();
    if (WantsHorizontalLines(visibility))
    {
        ClearValue(winrt::Control::BorderThicknessProperty());
    }
    else
    {
        BorderThickness(s_zeroThickness);
    }

    auto host = m_cellsHost.get();
    if (!host)
    {
        return;
    }

    const bool wantVertical = WantsVerticalLines(visibility);
    winrt::Brush gridLineBrush{ nullptr };
    if (wantVertical)
    {
        gridLineBrush = winrt::get_self<TableView>(owner)->GetGridLineBrush();
    }

    const auto children = host.Children();
    const uint32_t childCount = children.Size();
    for (uint32_t i = 0; i < childCount; ++i)
    {
        if (auto cellWrapper = children.GetAt(i).try_as<winrt::Border>())
        {
            if (wantVertical)
            {
                cellWrapper.BorderThickness(s_verticalThickness);
                cellWrapper.BorderBrush(gridLineBrush);
            }
            else
            {
                cellWrapper.ClearValue(winrt::Border::BorderThicknessProperty());
                cellWrapper.ClearValue(winrt::Border::BorderBrushProperty());
            }
        }
    }
}

// Row background / alternating banding only. Index-dependent (parity), so it must refresh when the
// row's position changes on recycle, and when RowBackground / AlternatingRowBackground change.
void TableViewRow::RefreshRowBackground()
{
    auto owner = GetOwningTableView();
    if (!owner)
    {
        return;
    }

    // Clear first so recycled rows do not keep stale banding fills.
    ClearValue(winrt::Control::BackgroundProperty());
    if (owner.RowBackground() != nullptr || owner.AlternatingRowBackground() != nullptr)
    {
        auto rowIndex = -1;
        if (auto repeater = winrt::get_self<TableView>(owner)->GetRowsRepeaterInternal())
        {
            rowIndex = repeater.GetElementIndex(*this);
        }
        if (rowIndex >= 0)
        {
            // RowBackground is the base for every row; AlternatingRowBackground overrides
            // odd rows only when set (WPF DataGrid parity). Setting RowBackground alone
            // must fill all rows uniformly, not stripe odd rows transparent.
            auto background = owner.RowBackground();
            if ((rowIndex % 2) != 0 && owner.AlternatingRowBackground() != nullptr)
            {
                background = owner.AlternatingRowBackground();
            }
            if (background)
            {
                Background(background);
            }
        }
    }
}

void TableViewRow::RefreshFrozenColumnLayout(double horizontalOffset, double leadingFrozenWidth)
{
    if (auto host = m_cellsHost.get())
    {
        TableViewCellsPanel::ApplyFrozenColumnLayout(host, horizontalOffset, leadingFrozenWidth);
    }
}

// ----- Editing -----
//
// The row owns its cells, so the control delegates the display/editor swap here. The swap replaces
// Child on the existing cell wrapper Border: keeping the wrapper means column width, visibility,
// frozen pinning and grid lines keep applying while the cell is edited, with no layout re-plumbing.

bool TableViewRow::BeginCellEdit(const winrt::TableViewColumn& column, const winrt::IInspectable& dataItem)
{
    if (!column)
    {
        return false;
    }

    // An edit already open on this row is closed first. Committing it is the control's job, not
    // the row's; by this point the control has already ended it, so anything still open here is
    // stale visual state.
    EndCellEdit(winrt::TableViewEditAction::Cancel);

    auto const host = m_cellsHost.get();
    if (!host)
    {
        return false;
    }

    winrt::Border cellWrapper{ nullptr };
    for (auto const& child : host.Children())
    {
        if (auto const border = child.try_as<winrt::Border>())
        {
            if (border.Tag().try_as<winrt::TableViewColumn>() == column)
            {
                cellWrapper = border;
                break;
            }
        }
    }

    if (!cellWrapper)
    {
        return false;
    }

    auto const editingElement = winrt::get_self<TableViewColumn>(column)->GenerateEditingElement(dataItem);
    if (!editingElement)
    {
        // The column declined the edit (no Binding, no editing template, or a base column).
        return false;
    }

    // No local DataContext on the editing element, for the same reason the display cell sets none:
    // it inherits from the wrapper, which tracks the item across row recycle.
    m_editingDisplayElement.set(cellWrapper.Child());
    cellWrapper.Child(editingElement);

    m_editingColumn.set(column);
    m_editingCellWrapper.set(cellWrapper);
    // An editor owns its cell; a tooltip over a live text box is noise.
    TableViewDetails::ClearOwnedToolTip(cellWrapper);
    m_editingElement.set(editingElement);

    // The column decides how its editor is primed - focus, caret, selection are editor-specific,
    // and the row has no business knowing that a TextBox wants SelectAll. The control calls it
    // (TableView::BeginEdit) so it can keep the returned pre-edit value for cancel.

    return true;
}

void TableViewRow::EndCellEdit(winrt::TableViewEditAction action)
{
    auto const cellWrapper = m_editingCellWrapper.get();
    if (!cellWrapper)
    {
        m_editingColumn.set(nullptr);
        m_editingElement.set(nullptr);
        m_editingDisplayElement.set(nullptr);
        return;
    }

    // Move focus off the editor before it leaves the tree. Dropping a focused element causes XAML
    // to fall back to whatever it can find, which can scroll the list; the row is the correct
    // landing spot and is where keyboard navigation expects focus to be.
    if (auto const editingElement = m_editingElement.get())
    {
        bool editorHasFocus = false;
        if (auto const root = XamlRoot())
        {
            if (auto const focused = winrt::FocusManager::GetFocusedElement(root).try_as<winrt::DependencyObject>())
            {
                auto current = focused;
                while (current)
                {
                    if (current == editingElement)
                    {
                        editorHasFocus = true;
                        break;
                    }
                    current = winrt::VisualTreeHelper::GetParent(current);
                }
            }
        }

        if (editorHasFocus)
        {
            Focus(winrt::FocusState::Programmatic);
        }
    }

    // Regenerate the display cell on commit rather than re-parenting the one that was parked when
    // the edit opened.
    //
    // The parked element still holds the OneWay binding it was created with, and a OneWay binding
    // only re-reads its source when that source raises PropertyChanged. For an item that does not
    // implement INotifyPropertyChanged the committed value would therefore never appear - the user
    // types, presses Enter, the item is updated, and the cell keeps showing the old text. A freshly
    // generated element evaluates its binding immediately against the current value, so the commit
    // is visible for plain POCOs too. This is what WPF's DataGrid does for the same reason.
    //
    // Only on Commit: a cancel wrote nothing, so the parked element is still correct and
    // regenerating it would be pure churn.
    winrt::UIElement displayElement = m_editingDisplayElement.get();
    if (action == winrt::TableViewEditAction::Commit)
    {
        if (auto const column = m_editingColumn.get())
        {
            try
            {
                if (auto const refreshed = column.GenerateElement(DataContext()))
                {
                    AttachCellContent(cellWrapper, refreshed);
                    displayElement = nullptr;
                }
            }
            catch (...)
            {
                // A column that throws while regenerating must not strand the row in edit mode;
                // fall back to the parked element, which is stale but present.
                displayElement = m_editingDisplayElement.get();
            }
        }
    }

    if (displayElement)
    {
        cellWrapper.Child(displayElement);
    }

    m_editingColumn.set(nullptr);
    m_editingCellWrapper.set(nullptr);
    m_editingElement.set(nullptr);
    m_editingDisplayElement.set(nullptr);

    // Beginning the edit retracted this cell's tooltip. The bound value did not change, so nothing
    // else would bring it back now that the cell is a display cell again.
    TableViewDetails::RefreshOwnedToolTip(cellWrapper);
}

void TableViewRow::AbandonCellEdit()
{
    // Restores the display child, but deliberately does NOT touch focus. Callers run inside a layout
    // pass, where moving focus re-enters the framework and trips the re-entrancy guard. Replacing
    // the child does not - and it must happen, or the row keeps showing a TextBox after the edit
    // closed, and over a different item once recycled.
    auto const editedWrapper = m_editingCellWrapper.get();
    if (editedWrapper)
    {
        editedWrapper.Child(m_editingDisplayElement.get());
    }

    m_editingColumn.set(nullptr);
    m_editingCellWrapper.set(nullptr);
    m_editingElement.set(nullptr);
    m_editingDisplayElement.set(nullptr);

    // The cell is a display cell again; restore the tooltip the edit retracted.
    TableViewDetails::RefreshOwnedToolTip(editedWrapper);
}

// Pointer entry point for editing, and the only place a pointer establishes the current cell.
//
// Lives on the row because the row owns its cells: resolving which cell a press landed on is a
// question only the row can answer cheaply. The control keeps the edit state machine, so this
// handler translates a gesture into SetCurrentCell / BeginEdit and nothing more. Mirrors WPF,
// where DataGridCell handles the gesture and calls DataGrid.BeginEdit.
//
// PointerPressed with click counting, not DoubleTapped: marking a press handled suppresses XAML's
// gesture recognizer entirely, and a row that participates in selection must mark it handled. A
// DoubleTapped handler would work today and silently break when selection lands.
void TableViewRow::ResetPressState()
{
    m_lastPressTimestamp = 0;
    m_lastPressPosition = {};
    m_lastPressColumn.set(nullptr);
    m_lastPressItem.set(nullptr);
}

void TableViewRow::OnPointerPressedForEditing(
    const winrt::IInspectable& /*sender*/,
    const winrt::PointerRoutedEventArgs& args)
{
    auto const owner = GetOwningTableView();
    if (!owner)
    {
        return;
    }

    auto ownerImpl = winrt::get_self<TableView>(owner);

    // Editing is opt-in and read-only by default, so a read-only table pays nothing beyond this.
    if (ownerImpl->IsReadOnly())
    {
        return;
    }

    auto const pointerPoint = args.GetCurrentPoint(*this);
    if (!pointerPoint)
    {
        return;
    }

    // Mouse/pen: primary button only - a right-click opens a context menu and must not begin an
    // edit. Touch reports no pressed button, so it is admitted on device type instead.
    const auto deviceType = args.Pointer().PointerDeviceType();
    if (deviceType != winrt::Microsoft::UI::Input::PointerDeviceType::Touch &&
        !pointerPoint.Properties().IsLeftButtonPressed())
    {
        return;
    }

    auto const column = ResolvePressedColumn(args.OriginalSource(), args.GetCurrentPoint(nullptr).Position());
    if (!column)
    {
        return;
    }

    auto const item = ownerImpl->UnwrapEditingDataItem(DataContext());
    if (!item)
    {
        return;
    }

    const auto timestamp = pointerPoint.Timestamp();
    const auto position = pointerPoint.Position();

    const double slop = GetDoubleClickSlop(*this);

    // Device type is part of the repeat identity so a mouse click following a pen tap is not read
    // as one double-click. Pointer ID deliberately is NOT: every touch contact gets a fresh id, so
    // requiring equality would mean the second tap never matches and touch could never begin an
    // edit at all.
    const bool isRepeatPress =
        m_lastPressColumn.get() == column &&
        TableView::SameInspectableIdentity(m_lastPressItem.get(), item) &&
        m_lastPressDeviceType == deviceType &&
        timestamp >= m_lastPressTimestamp &&
        (timestamp - m_lastPressTimestamp) <= GetDoubleClickIntervalMicroseconds() &&
        std::abs(position.X - m_lastPressPosition.X) <= slop &&
        std::abs(position.Y - m_lastPressPosition.Y) <= slop;

    // A press inside the cell already being edited belongs to the editor: it is the user placing
    // the caret, and must not be read as a navigation move or a fresh edit.
    const bool pressInsideOpenEdit =
        ownerImpl->IsEditing() &&
        m_editingColumn.get() == column &&
        m_editingElement.get() != nullptr;

    if (!pressInsideOpenEdit)
    {
        // Move the current cell even when the edit is refused (read-only, or BeginningEdit cancels):
        // the user pointed here, so a later F2 must not edit a different cell.
        //
        // This is also what makes keyboard editing reachable at all - pointer focus lands on the
        // row, not a tagged cell, so without this the current column stays null.
        ownerImpl->SetCurrentCell(item, column);

        if (isRepeatPress)
        {
            ownerImpl->BeginEdit(item, column);

            // Consume the press so a third click starts a fresh first press rather than
            // re-entering begin-edit on a cell that is already open.
            m_lastPressTimestamp = 0;
            m_lastPressColumn.set(nullptr);
            m_lastPressItem.set(nullptr);
                return;
        }
    }

    m_lastPressTimestamp = timestamp;
    m_lastPressPosition = position;
    m_lastPressColumn.set(column);
    m_lastPressItem.set(item);
    m_lastPressDeviceType = deviceType;
}

// Which of this row's cells a press landed on. Walks up from OriginalSource to the cell wrapper
// Border, whose Tag carries the owning column (set in RebuildCells). Once the row itself has focus
// a press can arrive with the row as OriginalSource and no tagged Border on the chain, so fall back
// to hit-testing this row's subtree.
winrt::TableViewColumn TableViewRow::ResolvePressedColumn(
    const winrt::IInspectable& originalSource,
    const winrt::Point& hostPoint)
{
    // A nested TableView's press bubbles through this row, and the walk would reach the INNER
    // table's tagged Border before it ever reached this row - so filter on ownership rather than
    // trying to stop the walk. A column this table does not own is not ours to act on.
    auto const owner = GetOwningTableView();
    auto const ownedByThisTable = [&owner](winrt::TableViewColumn const& candidate)
    {
        if (!candidate || !owner)
        {
            return false;
        }

        if (auto const columns = owner.Columns())
        {
            for (auto const& column : columns)
            {
                if (column == candidate)
                {
                    return true;
                }
            }
        }

        return false;
    };

    auto current = originalSource.try_as<winrt::DependencyObject>();
    while (current)
    {
        if (auto const border = current.try_as<winrt::Border>())
        {
            if (auto const tagged = border.Tag().try_as<winrt::TableViewColumn>())
            {
                return ownedByThisTable(tagged) ? tagged : nullptr;
            }
        }

        current = winrt::VisualTreeHelper::GetParent(current);
    }

    for (auto const& hit : winrt::VisualTreeHelper::FindElementsInHostCoordinates(hostPoint, *this))
    {
        if (auto const border = hit.try_as<winrt::Border>())
        {
            if (auto const tagged = border.Tag().try_as<winrt::TableViewColumn>())
            {
                return ownedByThisTable(tagged) ? tagged : nullptr;
            }
        }
    }

    return nullptr;
}
