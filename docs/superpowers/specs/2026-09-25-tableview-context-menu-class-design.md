# TableView Context Menu - Class Design

Date: 2026-09-25
Status: Approved (2026-09-25)
Scope: `controls\dev\TableView`

## 1. Intent and decisions

Translate the context-menu behavior spec into concrete class responsibilities and
internal contracts. This document describes a design, not an implementation.

Inputs:

- Behavior spec: `2026-09-23-tableview-context-menu-design.md` in this directory.
- Existing plan: `..\plans\2026-09-23-tableview-context-menu.md`.
- Existing declarations and event handling in `TableView.h`, `TableViewRow.h`,
  `TableViewColumn.h`, `TableView.cpp`, and `TableViewRow.cpp`.

Agreed decisions:

- Use thin row/header entry handlers and one shared pipeline on `TableView`.
- Put the pipeline in `TableView_ContextMenu.cpp`, not a separate controller object.
- A null flyout override permits native fallback. `Handled = true` suppresses
  the entire request, including native fallback.
- Preserve the proposed public API, selection behavior, and no-stamping approach.
- Suppress custom display if its target becomes stale during processing, rather
  than silently switching context. An explicit null override still permits native
  fallback.

The existing implementation plan distributes policy between row/header handlers.
It must be revised before implementation; its code snippets are not the class
contracts for this design. Neither the existing spec nor the plan is modified here.

## 2. Responsibility map

```text
TableViewColumn
  CellContextFlyout / HeaderContextFlyout
                       |
TableViewRow            |                  TableView
  row subscription      |                    header-host subscription
  body target lookup    |                    header target lookup
          |             |                            |
          +-------------|----------------------------+
                        v
              TableView::ProcessContextMenuRequest
                validate -> focus -> resolve -> notify
                         -> revalidate -> show
                                   |
                   TableViewContextFlyoutRequestedEventArgs
                                   |
                              application
```

| Type | Owns | Does not own |
|---|---|---|
| `TableViewColumn` | Cell/header flyout dependency properties | Event routing, target lifetime, realized-cell updates |
| `TableViewRow` | Row subscription, body target lookup, body visual validation | Flyout precedence, override notification, showing menus |
| `TableView` | Header subscription, shared policy, focus/current-cell coordination, notification, display | Direct traversal of row-owned cell internals |
| `TableViewDetails::ContextMenuTarget` | A synchronous target snapshot | Subscriptions, persistent state, public API |
| `TableViewDetails::ContextMenuResult` | Explicit routing outcome | Error transport or fallback flyout storage |
| `TableViewContextFlyoutRequestedEventArgs` | Public request context and application decision | Routed-event state or visual-tree references |

No new public cell container, service interface, virtual strategy hierarchy, or
per-row controller is needed.

## 3. Internal value contracts

The following declarations are proposed C++ contracts, not generated WinRT API.
Shared value types belong in `TableViewContextMenu.h`.

```cpp
namespace TableViewDetails
{
    enum class ContextMenuTargetKind
    {
        Body,
        Header,
    };

    struct ContextMenuTarget
    {
        ContextMenuTargetKind Kind{ ContextMenuTargetKind::Body };
        winrt::TableViewRow Row{ nullptr };
        winrt::IInspectable Item{ nullptr };
        winrt::TableViewColumn Column{ nullptr };
        winrt::FrameworkElement Anchor{ nullptr };
        winrt::Panel ScopeRoot{ nullptr };
    };

    enum class ContextMenuResult
    {
        Unhandled,
        Suppressed,
        Shown,
    };
}
```

Target invariants:

- Body: `Row` belongs to the receiving TableView; `Item` is the application data
  item resolved through the existing item-unwrapping convention. `ScopeRoot` is
  the row's current cells host.
- Header: `Row` and `Item` are null; `Column` identifies the header;
  `ScopeRoot` is the current header host.
- A body target may have a null column. It anchors to the row and can use
  `RowContextFlyout`. A valid cell target anchors to its existing wrapper.
- Header blank space is not a column target; it falls through natively.
- Columns must belong to the receiving TableView and be visible.
- Strong references keep objects alive for the synchronous operation, but do not
  prove they remain attached. Scope, item, and ownership must be revalidated.
- The target is never stored on the control, row, column, or event args.

`std::optional<ContextMenuTarget>` distinguishes no applicable target from a valid
body request without a column. Those cases must not collapse into the same null
column result.

`Unhandled` leaves the routed event untouched. `Suppressed` and `Shown` cause the
entry handler to set the routed event's `Handled` property. Exceptions are not
converted into one of these successful routing outcomes.

## 4. Class contracts

### TableViewColumn

Add the proposed `CellContextFlyout` and `HeaderContextFlyout` dependency properties
through `TableView.idl`. Defaults are null.

No property-changed callback is required: the pipeline reads the current values
when a request arrives. Changing either property must not rebuild cells or headers.

### TableViewRow

Proposed private methods:

```cpp
void OnContextRequested(winrt::ContextRequestedEventArgs const& args);

std::optional<TableViewDetails::ContextMenuTarget> ResolveContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args);
```

An internal C++ accessor supports owner-driven validation without exposing the
cells panel to the pipeline:

```cpp
bool IsContextMenuTargetCurrent(
    TableViewDetails::ContextMenuTarget const& target) const;
```

The handler checks already-handled input and a live owner, resolves a target,
calls the owner's pipeline, and maps the result to routed-event handling.
It does not read flyout properties or raise the public override event.

Pointer/touch lookup reuses `ResolvePressedColumn` with host coordinates.
Its existing caller uses a point relative to the host, and its fallback calls
`FindElementsInHostCoordinates`; a row-relative point is not interchangeable.
Keyboard/gamepad lookup uses `CurrentColumn()` only when the request has no
position. A pointer miss must not substitute an unrelated current column.

The row finds the existing wrapper through `TableViewCellsPanel::CellForColumn`.
No new cell cache or local `DataContext` is introduced.

Target resolution must distinguish a foreign nested TableView request from a
legitimate row-only request. An ownership-filtered null column alone cannot make
that distinction. Foreign requests do not enter this TableView's custom pipeline;
they remain subject to normal native bubbling.

### TableView

The existing proposed `RowContextFlyout` DP and `ContextFlyoutRequested` event
remain on `TableView`. Their accessors and event source are generated from IDL.

Proposed internal C++ entry point, callable by `TableViewRow` through `get_self`:

```cpp
TableViewDetails::ContextMenuResult ProcessContextMenuRequest(
    TableViewDetails::ContextMenuTarget target,
    winrt::ContextRequestedEventArgs const& args);
```

Proposed private methods:

```cpp
void OnHeaderContextRequested(winrt::ContextRequestedEventArgs const& args);

std::optional<TableViewDetails::ContextMenuTarget> ResolveHeaderContextMenuTarget(
    winrt::ContextRequestedEventArgs const& args);

bool IsContextMenuTargetCurrent(
    TableViewDetails::ContextMenuTarget const& target) const;

winrt::FlyoutBase ResolveContextFlyout(
    TableViewDetails::ContextMenuTarget const& target);

winrt::TableViewContextFlyoutRequestedEventArgs RaiseContextFlyoutRequested(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& resolvedFlyout);

void ShowContextFlyout(
    TableViewDetails::ContextMenuTarget const& target,
    winrt::FlyoutBase const& flyout,
    winrt::ContextRequestedEventArgs const& args);
```

The header resolver walks to a tagged header cell within the current header host;
it must not accept an arbitrary descendant tag as a header or walk into another
TableView's ownership domain. Focused header descendants use the same lookup.

Body validation delegates to the row. Header validation checks the captured host,
column ownership/visibility, and the anchor's membership in the current header
structure. A non-null parent is insufficient.

Only `ProcessContextMenuRequest` orders policy operations. The private methods
remain implementation details, not independently callable pipelines.

### TableViewContextFlyoutRequestedEventArgs

Follow the existing header-only event-args pattern exemplified by
`TableViewCellEditEndingEventArgs.h`.

| Property | Contract |
|---|---|
| `Item` | Read-only application row item; null for headers |
| `Column` | Read-only resolved column; nullable for body |
| `IsHeader` | Read-only request kind |
| `ContextFlyout` | Initially the selected candidate; replaceable; null permits native fallback |
| `Handled` | Initially false; true suppresses all further display |

The event is synchronous, with no deferral. Retaining the args does not retain a
visual anchor; changing them after the callback returns has no effect.
Return the args object from the internal notification method rather than pairing
a flyout return value with a `bool&` output parameter.

## 5. Shared pipeline

1. Respect already-handled routed input. Reject an inapplicable or stale target
   before changing focus or current-cell state.
2. For body requests, return `Unhandled` while the owning TableView is editing.
   Headers do not use this editing gate.
3. Before changing body current-cell/focus state, capture whether this request has
   an applicable custom candidate (column cell flyout, then row flyout). This is
   participation only, not the final candidate selection. Establish the body
   current cell when a column is available, then focus the
   row programmatically. Do not call selection operations. A row-only request
   still moves focus but must not fabricate a current column. Revalidate between
   current-cell and focus operations if the first can invoke application code.
4. Revalidate after focus, which can invoke application code. Do not act on a
   recycled row or replaced template. If current-cell/focus callbacks invalidate
   the request or start body editing, return `Suppressed` only if the captured
   participation was custom; otherwise return `Unhandled` for native fallback,
   without raising the override event.
5. Resolve the candidate: body uses column cell flyout, then row flyout;
   header uses column header flyout. Do not read or manually show
   `TableView.ContextFlyout`. For a still-valid request, resolve the candidate
   afresh after focus so callback changes to flyout properties remain effective.
6. With no candidate, return `Unhandled` without raising the override event,
   following the behavior spec's handler sequence.
7. Raise `ContextFlyoutRequested` once. If its `Handled` is true, return
   `Suppressed`. Otherwise, if its flyout is null, return `Unhandled`.
8. For a non-null flyout, revalidate the request after the application callback.
   If it no longer identifies the same live target, apply the safety rule below.
9. Show the selected flyout at the final anchor. Use `TryGetPosition(anchor, point)`
   for pointer/touch positioning with `Placement = Auto`. For keyboard/gamepad,
   show without an explicit point and let platform placement apply.
10. Return `Shown` only after `ShowAt` returns successfully.

| Situation | Pipeline outcome |
|---|---|
| A direct element flyout already handled the event | No custom processing |
| Body request while editing | `Unhandled` |
| No configured column/row candidate | `Unhandled`; no override event |
| Callback sets `Handled = true` | `Suppressed`, regardless of flyout value |
| Callback sets flyout to null without handling | `Unhandled`; native fallback remains possible |
| Callback supplies a flyout and target stays valid | `Shown` after successful display |
| Current-cell/focus callback invalidates a request with no applicable custom candidate before that callback | `Unhandled`; no override event |
| Target becomes stale before notification with custom participation, or while custom display remains requested | `Suppressed`, as specified below |

**Approved safety refinement:** if focus or the override callback
changes the row item, replaces the relevant template host, removes/hides the
column, or invalidates the anchor, consume the request without showing a menu
while custom display remains requested. An explicit null override still permits
native fallback; it is the application's decision to relinquish this request.
A native-only request invalidated by current-cell/focus callbacks also remains
unhandled. Determine participation from flyout configuration, not item nullness;
a realized null data item can still have an applicable custom candidate.
Do not restart resolution or raise the override event again for different context.
This deliberately refines the original spec's instruction to fall to the row rung
when a column disappears between resolution and display. A missing/invalid column
at initial resolution can still produce a row-only request; invalidation after
processing starts must not silently replace the context the app just observed.

Use a single scoped, per-TableView processing guard to suppress synchronous
reentry into this custom pipeline. Restore the guard on every return or exception.
This is not a controller lifetime or a retained request; nested TableView instances
have independent guards.

## 6. Subscription and lifetime rules

- `TableViewRow` owns a `ContextRequested_revoker` on itself. Install it in
  `OnApplyTemplate`, revoking any previous registration before replacing it.
- `TableView` owns a header-host `ContextRequested_revoker`. Revoke it before
  replacing template parts, then subscribe to the new host if present.
- Use weak captures, matching existing event patterns. Acquire strong references
  only for synchronous dispatch.
- Do not subscribe per cell or header cell. Do not request handled events merely
  to compete with directly assigned `ContextFlyout` values.
- Row recycling changes neither flyout assignments nor subscriptions. Each
  invocation reads the row's current owner/item/cells.
- Direct element flyouts retain native priority wherever that element is on the
  routed-event path. This is not a promise that a cell-content flyout participates
  when keyboard focus is on the row.
- Binding context comes from the placement target on show; never stamp a local
  `DataContext` onto cell wrappers.
- Unexpected notification, focus, or display failures propagate through existing
  event error handling. Do not broadly catch them or report `Shown` on failure.

## 7. File organization

| File | Proposed change |
|---|---|
| `TableView.idl` | New column/row flyout DPs, event, and event-args runtimeclass |
| `TableViewContextMenu.h` | Internal target/result types |
| `TableViewContextFlyoutRequestedEventArgs.h` | Public event-args implementation |
| `TableView.h` | Pipeline contracts, header revoker, scoped-processing flag |
| `TableView_ContextMenu.cpp` | Header resolution and shared pipeline implementation |
| `TableView.cpp` | Header subscription lifecycle only |
| `TableViewRow.h` / `TableViewRow.cpp` | Thin body handler, target lookup/validation, row revoker |
| `TableView.vcxitems` | Register new source/header files |

Retain generated DP/event accessors in their generated files. Do not hand-edit
generated files or add a generic helpers header for unrelated utilities.
Public additions retain the TableView preview/visibility attributes.

## 8. Validation design

Exercise the same policy matrix through both row and header entry points:

- Pointer, Shift+F10, Apps key, touch-hold, and gamepad invoke equivalent applicable
  menus; pointer misses do not reuse the keyboard current column.
- Body precedence is column, row, native fallback. Header precedence is column,
  native fallback.
- Replacement, null override, and suppression produce distinct routing outcomes.
  No-candidate requests do not raise the override event.
- Focus moves without selection changes, including `SelectionMode.None`.
- Editing body requests and directly assigned element flyouts retain native behavior.
- Recycling, retemplating, column visibility changes, nested TableViews, and
  synchronous callback mutations cannot show a stale menu or duplicate a request.
- Shared flyouts obtain the correct row binding context on successive shows.
- Dismissal, accessibility name, and keyboard focus behavior follow the platform.
- API coverage includes DP defaults, event args, and generated metadata.

These are required scenarios, not a claim that a TableView test harness already
exists. Confirm the available Tabular test infrastructure when revising the plan;
the existing plan documents a build plus sample-app validation path.

Before implementation, also resolve the original spec's platform-dispatch open
item: verify that base event handling and the subscription cannot double-show.
The class design does not assume that source-level question has been settled.
