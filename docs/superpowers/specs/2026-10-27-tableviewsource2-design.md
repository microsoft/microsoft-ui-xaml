# TableViewSource2 and ShapedItemsSource2 — Design

Status: Draft for review
Date: 2026-10-27
Branch: `dipeshmsft-tableview-source-interface`

## 1. Purpose

`TableViewSource2` is a second TableView source variant, selectable at runtime through
`TableViewSourceRegistry` (`"v2"`), so the perf harness and API tests can compare variants
without rebuilding. It differs from `TableViewSource` (v1) in two ways:

1. **Row identity is external.** Every row is identified by an app-supplied key string, never by
   object address.
2. **One representation for every shape.** Flat, grouped, hierarchical, and grouped-hierarchical
   projections are all a single pre-order array with per-row depth. There are no separate grouped
   and hierarchical adapters.

v1 is unchanged and remains the perf baseline.

### Success criteria

- v2 binds to TableView purely through `::ITableViewSource` and `ITableViewShapingSource`.
- Existing API tests that exercise supported verbs pass when parameterized to `"v2"`.
- Identity violations fail fast with `E_INVALIDARG`; no address-based identity exists anywhere in v2.
- Perf harness can switch v1/v2 by variant name.

## 2. Scope

| Feature | v2 |
|---|---|
| `From(items, keySelector)` | Supported; key required |
| `From(items)` | Throws `E_INVALIDARG` |
| `Filter` / `ClearFilter` | Supported |
| `Sort` (both overloads) / `ClearSort` | Supported |
| `GroupBy` (both overloads) / `ClearGroupBy` | Supported, single level |
| `ParentBy(parentSelector)` (new) / `ClearParentBy` | Supported |
| `ParentBy(keySelector, parentKeySelector)` | Throws `E_NOTIMPL` |
| `GroupBy` + `ParentBy` together | Supported (roots-only grouping, see §6.4) |
| `ChildrenWith` (both) / `ClearChildrenWith` | Throw `E_NOTIMPL` |
| `IsLiveShaping` | Supported |
| Internal sort commands (`ReplaceSortAxis`, `ClearAllSortAxes`, `ClearSortAxis`, `ClearSortAxesExcept`, `ActiveSortAxisInfos`, `CanApplySort`) | Supported |
| Telemetry, TiP | Not implemented (disabled on this branch) |

Out of scope: multi-level grouping, `ChildrenWith`, incremental (non-rescan) filter patching,
Fenwick-tree visibility, source-provided pre-order fast path.

## 3. Public API changes

`TableViewSource.idl`:

```idl
// ITableViewShapingSource gains the identity-based ParentBy.
ITableViewShapingSource ParentBy(TableViewIdentitySelector parentSelector);

runtimeclass TableViewSource2 : [default] ITableViewShapingSource
{
    static TableViewSource2 From(Object items);                                    // throws E_INVALIDARG
    [method_name("FromWithKey")]
    static TableViewSource2 From(Object items, TableViewIdentitySelector keySelector);
}
```

- The new `ParentBy` differs in arity from the existing two-argument overload, so no
  `[default_overload]` is required.
- v1 `TableViewSource::ParentBy(parentSelector)` throws `E_NOTIMPL`.
- `TableViewSourceRegistry` gains `{ L"v2", ... }`, forwarding to `TableViewSource2::From(items, key)`.
  `Create("v2", items)` therefore throws `E_INVALIDARG`.

## 4. Identity rules

All identity is an `hstring`.

| Identity | Source | Rule |
|---|---|---|
| Row key | `keySelector` passed to `From` | Required. Null/empty or duplicate across the **entire source** (including filtered-out rows) → `E_INVALIDARG`. |
| Parent key | `ParentBy(parentSelector)` | Null/empty → root. Key not present in source → root (orphan). Self-parent or cycle → `E_INVALIDARG`. |
| Group key | `GroupByWithIdentity` identity selector, else the group value | Without an identity selector the group value must be a value type (string, number, GUID, …); an object value → `E_INVALIDARG`. |

Failure policy: **throw wherever detected, no mitigation.** Detection points are `From`, any verb's
rebuild, the source collection-changed handler, and the live-shaping `PropertyChanged` handler. The
engine holds no faulted state; the next change or verb re-validates.

A key selector that throws is rethrown as the caller's error (`RethrowAsCallerError`).

Group headers are never placed in the row-key map, so a group key cannot collide with a row key.

## 5. Architecture

```
TableView ──::ITableViewSource──▶ TableViewSource2 ──▶ ShapedItemsSource2 ──▶ PreorderProjection
App ──ITableViewShapingSource──▶        │                    │
                                        ▼                    ├─ CollectionAccessor (reused)
                         RowMetadataProvider (v2 overload)   ├─ ShapingPipeline / StableSortByKeys /
                                        ▲                    │  ApplyPredicateFilter (reused)
                         ProjectionAdapter2 ◀────────────────┘─ KeyedLiveShapingTracker (new)
```

| Unit | Responsibility | Location |
|---|---|---|
| `TableViewSource2` | WinRT class; implements `ITableViewShapingSource` and `::ITableViewSource`; owner tracking; translates verbs; publishes `ItemsSourceView` and row metadata. Mirrors v1's structure (ReferenceTracker, UI-thread final release). | `controls\dev\TableView\TableViewSource2.h/.cpp` |
| `ShapedItemsSource2` | Engine: owns source subscription, identity validation, verb state, live shaping, and the projection. No knowledge of TableView. | `controls\dev\ShapedItemsSource2\` |
| `PreorderProjection` | Pure data structure (§6): pre-order array, depth, subtree end, scans, splices. Unit-testable without WinRT collections. | `controls\dev\ShapedItemsSource2\` |
| `ProjectionAdapter2` | Exposes the visible rows as an observable `ItemsSourceView`; expand/collapse by node or group key; raises range notifications from visible diffs. Replaces both v1 adapters. | `controls\dev\ShapedItemsSource2\` |
| `KeyedLiveShapingTracker` | Per-item `PropertyChanged` subscriptions keyed by row key (v1's `LiveShapingTracker` is address-keyed). | `controls\dev\ShapedItemsSource2\` |
| Row metadata | `RowMetadataProvider` overload that reads depth / header / expansion from `ProjectionAdapter2` and identity from the row key. | `controls\dev\TableView\RowMetadataProvider.*` |

Reused unchanged: `CollectionAccessor`, `ShapingPipeline`, `ApplyPredicateFilter`,
`StableSortByKeys`, `ValueComparer`, `CallerError`/`RethrowAsCallerError`, `SwallowedFaults`,
`ShapedGroup`.

Projection kinds published by v2: `None`, `Flat`, `Grouped`, `Hierarchical`,
`GroupedHierarchical`. There is no `Unshaped` kind: every row always has an identity.

## 6. Data model

### 6.1 Arrays

| Array | Meaning |
|---|---|
| `R[pos]` | Pre-order rows: item or group header, depth, `End` (exclusive end of subtree), `Included`, `SelfMatch` |
| `idToSrc` | row key → source index |
| `srcKeys` | source index → row key (lets remove-by-index find the key without the removed item) |
| `srcToPos` | source index → position in `R` |
| `parentSrc` | source index → parent source index or root (hierarchy only) |
| `offset`, `kids` | CSR child lists: children of node *n* are `kids[offset[n] .. offset[n+1])`; virtual root holds top-level rows |
| `visible` | increasing list of positions in `R` currently shown |
| `groupToHeader` | group key → header row (cached `ShapedGroup`) |
| `expanded` | set of node / group keys that are expanded |

The *cached tree* is `parentSrc`, depth, `offset`/`kids`. It is reused across filter and sort verbs
and dropped by any source change or live key/parent change that alters structure.

### 6.2 Build

1. Resolve each row's parent through `idToSrc`; compute depth by memoized parent-chain walk.
   Re-entering a node on the current walk is a cycle → `E_INVALIDARG`.
2. Build CSR child lists (count, prefix-sum offsets, fill in source order).
3. Sort each sibling run with `StableSortByKeys` (ties keep source order).
4. Depth-first walk emits `R`, `End`, `srcToPos`.

Cost O(n log s). Path-key sorting was rejected: O(n log n · depth) and per-row key allocations.

| Shape | Depth layout | Top-level run |
|---|---|---|
| Flat | all rows depth 0 | rows |
| Grouped | headers 0, items 1 | headers |
| Hierarchical | parent-chain depth | roots |
| Grouped + hierarchical | headers 0, roots 1, tree depth + 1 | headers |

### 6.3 Filter and visibility

- **Reverse scan** over `R`: `Included = SelfMatch || childHit[d+1]`; reset `childHit[d+1]`;
  `childHit[d] |= Included`. Keeps ancestors of matches (context rows). Group headers never match
  themselves, so empty groups disappear. A parallel `childCount[d]` yields each header's
  `GroupChildCount` in the same pass.
- **Forward scan** builds `visible`, skipping subtrees that are excluded or collapsed
  (`hideDeeperThan`).
- Old vs new `visible` are merge-diffed (both increasing) into minimal range insert/remove
  notifications; no Reset for ordinary changes.

Reads: `GetAt(k) = R[visible[k]]` O(1); `IndexOf(item)` = key → `srcToPos` → binary search in
`visible`, O(log n).

Expand/collapse of position *p*: collapse removes `visible` entries with position in `(p, End[p])`
as one range; expand runs the forward scan over `[p+1, End[p])` and splices the result after *p*.
Expansion state persists by key across rebuilds and is pruned for keys that disappear. Default
expansion and filter-context expansion behaviour match v1's `HierarchicalSourceAdapter`.

### 6.4 Grouping

- Single level. Each distinct group key gets one header row whose children are its members.
- **Header order matches v1:** sorts declared before `GroupBy` (pipeline `GroupOrder`) order
  headers by each group's first item; with none, headers follow first-appearance source order.
  Sorts declared after `GroupBy` order members within each group.
- Header objects are cached `ShapedGroup` instances keyed by group key; `GroupItems` is refilled
  only for groups whose range changed.
- `IsGrouped` is true for `Grouped` and `GroupedHierarchical`.

**Grouped + hierarchical (v1 semantics):** only roots are grouped. Descendants stay under their
root regardless of their own group value. Headers are virtual parents of roots. Post-`GroupBy`
sorts order roots within each group and every sibling run below.

### 6.5 Change handling

| Change | Handling |
|---|---|
| Filter set/cleared | Reverse + forward scan only; `R` unchanged |
| Sort changed | Re-sort sibling runs, DFS, scans; cached tree reused |
| GroupBy / ParentBy set or cleared | Full rebuild |
| Source insert (leaf) | Validate key; binary search in parent's (or group's) sibling run; shift `R`, `End` fixups for ancestors; scans. Creates a header if the group is new. |
| Source remove (leaf) | Erase range; ancestor `End` fixups; scans. Removes an emptied header. |
| Source replace, same key | Update in place; treated as a live change (below) |
| Source replace, different key | Remove + insert |
| Remove of a node with children; insert of a missing parent of existing orphans; Move; Reset | Full rebuild |
| Live: sort value | Rotate the row's subtree range within its parent's sibling run (binary search over sibling starts found via `End`); scans |
| Live: filter result | Scans only |
| Live: group value of a flat item or root | Move subtree range to the other header's run; create/delete headers |
| Live: group value of a descendant | Ignored |
| Live: parent key | Cycle check (new parent position must lie outside the moved range, O(1)) → `E_INVALIDARG`; move range; apply depth delta; `End` fixups on old and new ancestors; scans. Root ↔ child transitions leave or join a group. |
| Live: own row key | Re-key `idToSrc`, `srcKeys`, tracker, snapshot; clash → `E_INVALIDARG`. Children referencing the old key become orphans → full rebuild. |

Many dirty items in one turn fall back to a full refresh (threshold tuned with the harness).

## 7. Live shaping

- While `IsLiveShaping` is true, every source item (including filtered-out rows) is subscribed via
  `KeyedLiveShapingTracker`.
- Per-key snapshot: sort values, filter result, group key, parent key, row key. A `PropertyChanged`
  that leaves the snapshot unchanged is ignored.
- First relevant change in a UI-thread turn marks dirty and posts one restore to the owner's
  `DispatcherQueue`; N changes cost one reshape.
- Copied from v1: edit hold (`SetLiveShapingHold` / `ResumeHeldLiveShaping`), `LiveItemChangedHook`,
  `RowsReleasedHook`, `LiveItemReshapeHandler`, off-thread `PropertyChanged` marshalling and the
  dropped-change flag.
- When live shaping is off but a sort is active, sort-value changes are still observed (v1
  behaviour) through the same tracker.
- After a live restore, `ShapingChanged(reorderOnly = true)` only if every change was a move within
  its sibling run; otherwise `false`.

## 8. Threading and lifetime

Same contract as v1: verbs and projection on the owner (UI) thread; `TableViewSource2` final
release marshalled to the UI thread; single owning TableView with owner-released waiters.

## 9. Testing

- **API tests:** parameterize existing `TableViewSource` tests over registry variants (`"default"`,
  `"v2"`). Tests for unsupported v2 verbs (`ChildrenWith`, two-argument `ParentBy`, keyless `From`)
  are skipped for v2 or assert the documented exception.
- **v2-specific tests:** missing key; duplicate key at `From`, on insert, on replace, and on live key
  change; null/empty key; cycle and self-parent; orphan becomes root; group ordering parity with v1;
  grouped-hierarchical roots-only grouping; new `ParentBy` throws `E_NOTIMPL` on v1.
- **`PreorderProjection` unit tests:** build, scans, diff notifications, subtree moves, reparent,
  expand/collapse splices.
- **Perf:** harness selects the variant by registry name; compare v1 and v2 on existing scenarios.

## 10. Order of work

Each step builds clean before the next.

1. Flat: `TableViewSource2`, `ShapedItemsSource2`, `PreorderProjection` (depth 0), filter, sort,
   incremental source changes, identity validation, registry entry, IDL (including the new
   `ParentBy` throwing in v1).
2. GroupBy: headers, `ProjectionAdapter2`, row metadata overload.
3. ParentBy(parentSelector), including GroupBy + ParentBy.
4. Live shaping.
5. Parameterized API tests and harness variant switch.

## 11. Decisions log

| # | Decision | Alternatives rejected |
|---|---|---|
| D1 | Keyless `From` throws | Fall back to object identity |
| D2 | Identity violations throw, no faulted/recovery mode | Keep last good projection and recover |
| D3 | v2 supports only identity-based `ParentBy(parentSelector)` | Also support two-argument form with value-type keys |
| D4 | One pre-order engine for all shapes | Plain vector for flat, tree engine only when grouped/hierarchical |
| D5 | Pre-order built via CSR + per-sibling sort | Path-key lexicographic sort; materialized path strings; nested sets |
| D6 | Group header order follows v1 (pre-GroupBy sorts) | Headers sorted by group value |
| D7 | GroupBy + ParentBy groups roots only | Each row in its own group with repeated ancestor context |
| D8 | Live shaping in scope, keyed by row key | Defer to a later phase |
| D9 | `visible` array with rescans | Fenwick tree over visibility bits |
