# TableView Context Menu — Design Spec

Date: 2026-09-23
Status: Proposed
Scope: `controls/dev/TableView`

## 1. Problem

`TableView` has no context-menu support. A grep of `controls/dev/TableView` for
`ContextFlyout|ContextRequested|MenuFlyout|Flyout|RightTapped` returns zero matches.

Right-click on a cell today does nothing beyond the existing guards: `TableViewRow::OnPointerPressed`
returns early for non-primary buttons (`TableViewRow.cpp:363-371`), and the begin-edit path has the
same guard (`:1179-1186`). Both carry comments explicitly anticipating this feature ("a right-click
opens a context menu and must not select"). `TableView.idl:511-514` likewise defers the public
current-cell surface until "a context menu ... to justify them".

Apps need per-column context menus for cells and for column headers, with the menu able to reach the
row item and the column that was clicked.

## 2. Goals / Non-goals

**Goals**

- Plumbing only: let an app attach a `FlyoutBase` per column (cells and headers) and per row.
- Identical behavior for pointer, keyboard (Shift+F10 / Apps key / gamepad Menu), and touch-hold.
- Establish the current cell and move focus on right-click, so a menu can act on "the cell I clicked".
- Give the app a single override hook to swap or suppress the resolved flyout.
- Never clobber a flyout the app attached directly to an element.

**Non-goals**

- No built-in default menu (no Copy / Delete / Select All items). Commands are out of scope.
- No new public container type for cells. Cells remain plain `Border` wrappers.
- No public current-cell API. `SetCurrentCell` / `CurrentColumn` stay internal.
- No change to selection behavior.

## 3. Background: how the platform resolves context flyouts

Two facts from the XAML core drive the entire design.

### 3.1 Base auto-show beats subscribed handlers on the same element

`CEventManager::Raise` (`dxaml/xcp/core/dll/eventmgr.cpp:530-560`) processes a single element in this
order:

1. `RaiseControlEvents` / `RaiseUIElementEvents` — dispatches through
   `CControl::Delegates[...]` / `CUIElement::Delegates[...]` (`UIElement.g.cpp:22-62`), reaching
   `CUIElement::OnContextRequested` → `OnContextRequestedCore` (`uielement.cpp:12781-12875`). If the
   element has `ContextFlyout` set, it shows it and sets `m_bHandled = TRUE` (`:12874`).
2. *Then* `RaiseHelper(matches, ...)` (`:548-559`) — the handlers the app or control subscribed.

The listener list itself is a flat `std::vector<REQUEST>` appended in registration order
(`eventmgr.cpp:1288-1297`); there is no internal-vs-CLR partition. So the ordering above is the whole
story: **a subscribed `ContextRequested` handler can never pre-empt the same element's own
`ContextFlyout`.**

Consequence: any element we use as an interception point must be one that never carries an
app-set `ContextFlyout`.

### 3.2 Keyboard raises from the focused element, and the event bubbles

`ContextMenuProcessor::ProcessContextRequestOnKeyboardInput` (`ContextMenuProcessor.cpp:28-44`) fires
on Shift+F10, Apps key, and `VirtualKey_GamepadMenu`, calling `RaiseContextRequestedEvent(pSource,
{-1,-1}, false)`. That sets `put_Source(pSource)` and raises from `pSource` (`:54-68`), where
`pSource` is the **focused element**.

In TableView, focus lives on the **row** — `TableViewRow.cpp:377` (`FocusState::Pointer`) and `:1064`
(`FocusState::Programmatic`). Cells and `PART_CellsHost` are never focused.

`ContextRequested` bubbles *upward*. So from the row the route is row → parent → … → TableView, and
**the cell wrapper `Border` and `PART_CellsHost` are not on the keyboard route at all** — they are
descendants of the row.

Consequence: **attaching anything at or below the cell is unusable.** Pointer would find it and
keyboard would not, so the same cell would show different menus depending on input device. This rules
out both stamping a flyout on the cell wrapper and subscribing a handler there.

### 3.3 A shared flyout can still read per-cell context

`FlyoutBase::ForwardTargetPropertiesToPresenter` (`FlyoutBase_partial.cpp:1458-1484`) copies
`PlacementTarget.DataContext` onto the presenter on **every show**. So one `FlyoutBase` instance
shared by all cells of a column re-points per show, and its items can bind to the row item. This is
the WinUI analogue of WPF's `ContextMenu.PlacementTarget` coercion
(`PopupControlService.cs:1263-1284`).

### 3.4 Comparison to WPF DataGrid

WPF ships no built-in DataGrid context menu either. It resolves `ContextMenu` by **bubbling**, not by
inheritance: `ContextMenuService` registers a class handler on `UIElement` and stashes the first
ancestor that has an enabled menu into `e.TargetElement` (`ContextMenuService.cs:389-409`), which
`PopupControlService.RaiseContextMenuOpeningEvent` (`:962-1026`) then opens. Innermost wins, same
shape as `OnContextRequestedCore`'s handled-gate.

Two WPF properties do **not** transfer:

- WPF's row menu gets a dedicated hit zone via `DataGridRowHeader`, so row and cell menus never
  collide. TableView has no row-header equivalent, so precedence must be explicit.
- `DataGridCell` is focusable, so WPF's keyboard and pointer routes agree. TableView cells are not
  focusable, which is exactly the problem in §3.2.

WPF also selects on right-click (`DataGrid.OnContextMenuOpening`, `DataGrid.cs:6305-6349`). We follow
ListView instead: `ListViewBaseItem::OnRightTapped` (`ListViewBaseItem_Partial.cpp:1199-1222`) only
calls `FocusItem`. See §6.2.

## 4. Approach

**Central resolution, not stamping.** No flyout is ever pushed onto a cell or header at build time.
Instead, `TableView` subscribes `ContextRequested` at a small number of fixed points, resolves which
flyout applies when the event actually fires, and calls `ShowAt`.

Rejected alternative — stamping a flyout per cell during `RebuildCells`, mirroring how
`CellToolTipBinding` is applied (`TableViewRow.cpp:788-790`):

1. Fatal: broken for keyboard, per §3.2. Tooltips never hit this because hover is pointer-only.
2. The stamped flyout shows before any of our code runs, so there is no override event, no
   `SetCurrentCell`, no focus change.
3. Costs work on every cell realize *and* recycle — the virtualized scroll hot path — plus a re-stamp
   whenever the column DP changes. Central resolution costs nothing until an actual right-click.

## 5. API surface

New on `TableViewColumn`:

```idl
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase CellContextFlyout { get; set; };
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase HeaderContextFlyout { get; set; };
static Microsoft.UI.Xaml.DependencyProperty CellContextFlyoutProperty { get; };
static Microsoft.UI.Xaml.DependencyProperty HeaderContextFlyoutProperty { get; };
```

New on `TableView`:

```idl
Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase RowContextFlyout { get; set; };
static Microsoft.UI.Xaml.DependencyProperty RowContextFlyoutProperty { get; };

event Windows.Foundation.TypedEventHandler<TableView, TableViewContextFlyoutRequestedEventArgs> ContextFlyoutRequested;
```

New args type:

```idl
runtimeclass TableViewContextFlyoutRequestedEventArgs
{
    Object Item { get; };                  // row item; null for a header
    TableViewColumn Column { get; };       // resolved column; may be null
    Boolean IsHeader { get; };
    Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase ContextFlyout { get; set; };  // pre-set to the resolved flyout; settable to override
    Boolean Handled { get; set; };         // true suppresses the show entirely
};
```

**Why the row rung is `TableView.RowContextFlyout` and not `TableViewRow.ContextFlyout`:** the row is
the only element on both routes (§3.2), and by §3.1 a flyout set on the row would auto-show before our
handler could prefer the column's. Hanging the row rung off `TableView` instead means nothing ever
sets `ContextFlyout` on the row, the base no-ops there, and our handler always runs.

## 6. Behavior

### 6.1 Subscription points and precedence

| Rung | Subscribe `ContextRequested` on | Established in |
|---|---|---|
| Cells | `TableViewRow` itself (one per row) | `TableViewRow::OnApplyTemplate` (`TableViewRow.cpp:131`) |
| Headers | `m_headerHost` Panel (`TableView.cpp:481`) | `TableView::OnApplyTemplate` |
| Fallback | *nothing* — `TableView.ContextFlyout` base auto-show is the last rung | — |

`m_headerHost` is used rather than `TableView` because a focused header cell is *inside* it, putting
it on the header keyboard route while still beating `TableView`'s own base auto-show (§3.1).

Precedence:

- Body: `Column.CellContextFlyout` → `TableView.RowContextFlyout` → fall through unhandled, so the
  base shows `TableView.ContextFlyout`.
- Header: `Column.HeaderContextFlyout` → fall through unhandled.

Group-header rows are separate repeater containers on neither route, so they fall through to
`TableView.ContextFlyout`.

### 6.2 Handler sequence

On `ContextRequested` at the row:

1. If the owning TableView `IsEditing`, return unhandled. The editor is deeper in the route and its
   own base auto-show has already run, so the editor's flyout wins for free.
2. Resolve the column (§6.3).
3. `SetCurrentCell(item, column)` (`TableView_Editing.cpp:207`) and `Focus(FocusState::Programmatic)`
   on the row.
4. **Selection is never changed.** The existing right-click guard (`TableViewRow.cpp:363-371`) stays.
   Under `SelectionMode.None`, focus still moves.
5. Resolve the flyout by precedence. If none, return unhandled.
6. Raise `ContextFlyoutRequested`. If `Handled`, stop. Otherwise take `args.ContextFlyout`, which the
   app may have replaced; if it is now null, stop.
7. `ShowAt(cellWrapper, options)` with `Placement = Auto` and the event's point. For keyboard
   (`GlobalPoint == {-1,-1}`), `ShowAt(cellWrapper)` with no point so it centers.
8. `args.Handled(true)`.

On `ContextRequested` at `m_headerHost`, the sequence is the same minus steps 1, 3, and 4 — headers
do not edit, do not set the current cell, and do not take focus — and step 7 shows at the header
`Grid` rather than the cell wrapper.

### 6.3 Column resolution

**Body, pointer:** reuse `TableViewRow::ResolvePressedColumn(originalSource, position)`
(`TableViewRow.cpp:1258`) unchanged. It already walks up from `OriginalSource` to the tagged wrapper
`Border` — the `Tag` carries the column, set in `RebuildCells` and read by
`TableViewCellsPanel::ColumnForCell` (`TableViewCellsPanel.cpp:25-32`) — filters out columns owned by
a nested TableView (`:1262-1268`), and falls back to hit-testing the row subtree when the row itself
arrives as `OriginalSource`.

**Body, keyboard:** `GlobalPoint == {-1,-1}`, so skip hit-testing and use `CurrentColumn()`
(`TableView_Editing.cpp:180`). If null, drop to the row rung.

**Header:** walk `OriginalSource` up to the header `Grid` built in `TableView::RebuildHeaders`
(`TableView.cpp:1549-1575`) and read its column. Keyboard uses the focused header cell directly;
header cells are `IsTabStop` only when resizable (`:1556`), so when nothing in the header has focus
the event does not originate there at all.

### 6.4 Edge cases

- **Recycling / `ItemsSource` swap** — the subscription is per-row on the row itself, established once
  in `OnApplyTemplate`. Recycling needs no work: no per-cell token, and no
  `ReleaseCellToolTips`-style teardown (`TableViewRow.cpp:808`). This is a direct benefit of stamping
  nothing, so the ownership discipline in `TableViewToolTipHelpers.h` is not replicated here.
- **App-set `ContextFlyout` on cell content or on the row** — deeper in the route, or the same element,
  so its base auto-show wins and we never clobber it. Documented as an explicit bypass of resolution;
  consistent across pointer and keyboard.
- **Nested TableView** — covered by `ResolvePressedColumn`'s ownership filter.
- **Touch-hold** (500 ms, `ContextMenuProcessor.cpp:78-113`) and **gamepad Menu** (`:35`) arrive as
  ordinary `ContextRequested`; no extra work.
- **RTL** — `ShowAt` mirrors; nothing custom.
- **Column removed or hidden between resolve and show** — re-check the wrapper is still parented; if
  not, fall to the row rung.
- **`TableViewGroupHeader`** — falls through to `TableView.ContextFlyout`.

## 7. Testing

**Unit / API**

- Precedence: column beats row beats TableView, for cells and for headers.
- `ContextFlyoutRequested` can swap the flyout, and `Handled` suppresses the show.
- Null column falls back to the row rung.

**Interaction (TAEF)**

- Right-click a cell shows that column's `CellContextFlyout`.
- **The same cell via Shift+F10 shows the same flyout.** This is the regression the design exists to
  prevent (§3.2).
- Apps key and touch-hold behave identically.
- Right-click does not change selection, but does move focus and set the current cell.
- Right-click while editing shows the editor's own flyout, not ours.
- Right-click a column header shows `HeaderContextFlyout`.

**UIA**

- Flyout presenter exposes the correct `Name`.
- Focus returns to the row on dismiss.

## 8. Open items

- Confirm at implementation time that `CUIElement::OnContextRequested` and
  `Control::OnContextRequestedImpl` (`Control_Partial.cpp:1321-1339`) do not both fire for a `Control`
  subclass, which would double-show. Note `OnContextRequestedImpl` is a dxaml-internal virtual with no
  XamlOM model entry (`Control_Partial.h:139`), so `TableView` cannot override it and must subscribe
  to the event — which is what this design does regardless.
