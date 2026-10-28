# TableViewSource6 and ShapedItemsSource6 — Design

Status: Draft for review
Date: 2026-10-28
Branch: `dipeshmsft-tableview-source-interface`

## 1. Purpose

`TableViewSource6` is a TableView source variant, selectable through `TableViewSourceRegistry`
(`"v6"`), for comparing projection models in the perf harness. It differs from every earlier
variant in how the visible rows are represented:

- **No flattened row list.** v1/v4/v5 materialize visible rows into an observable vector; v2/v3
  keep a pre-order array plus a `visible` index array. v6 keeps neither.
- **The model's hierarchy is the index.** Each item gets one lightweight wrapper node holding links,
  expansion state and a count of visible rows in its subtree. `GetAt(i)` walks down the tree,
  skipping whole branches by their counts. All visible rows are counted (parents and leaves alike),
  and there is no second visible-only tree.
- **Object identity.** Rows are identified by the address of their `IUnknown`. There is no
  external key.

v1–v5 are unchanged.

### Success criteria

- v6 binds to TableView purely through `::ITableViewSource` and `ITableViewShapingSource`.
- Sort, filter, GroupBy and ChildrenWith work, alone and combined, and match v1's presented rows
  for the same declarations.
- Expand and collapse cost O(depth · log k) and raise one range notification, however many rows
  they show or hide.
- No structure proportional to the number of visible rows is built or kept.
- The perf harness can select `"v6"` with the `Children` API build.

## 2. Scope

| Feature | v6 |
|---|---|
| `From(items)` | Supported |
| `From(items, keySelector)` | Null key → same as `From(items)`; non-null → `E_NOTIMPL` |
| `Filter` / `ClearFilter` | Supported |
| `Sort` (both overloads) / `ClearSort` | Supported |
| `GroupBy` (both overloads) / `ClearGroupBy` | Supported, single level |
| `ChildrenWith` (both overloads) / `ClearChildrenWith` | Supported |
| `GroupBy` + `ChildrenWith` together | Supported; roots only are grouped (§6.5) |
| `ParentBy` (both overloads) / `ClearParentBy` | Throw `E_NOTIMPL` |
| `IsLiveShaping` | Supported |
| Internal sort commands (`ReplaceSortAxis`, `ClearAllSortAxes`, `ClearSortAxis`, `ClearSortAxesExcept`, `ActiveSortAxisInfos`, `CanApplySort`) | Supported |
| Telemetry, TiP | Not implemented (disabled on this branch) |

Out of scope: multi-level grouping, `ParentBy`, external keys, sharing a node between two parents.

## 3. Public API changes

`TableViewSource.idl`:

```idl
// Rows are identified by object identity and kept in the model's own hierarchy: visible rows are
// found by walking wrapper nodes that count their visible descendants, never by a flattened list.
// ParentBy throws E_NOTIMPL; use ChildrenWith for hierarchy.
[MUX_PREVIEW]
[webhosthidden]
runtimeclass TableViewSource6 : [default] ITableViewShapingSource
{
    static TableViewSource6 From(Object items);
    // A null keySelector is the same as From(items); a non-null one throws E_NOTIMPL.
    [method_name("FromWithKey")]
    static TableViewSource6 From(Object items, TableViewIdentitySelector keySelector);
}
```

`TableViewSourceRegistry` gains `{ L"v6", ... }`.

## 4. Identity rules

| Rule | Behaviour |
|---|---|
| Identity | `winrt::get_abi(item.as<IUnknown>())`. The node holds a strong reference to the item, so the address stays valid while the node lives. |
| Identity string (row metadata) | `"p:"` + hex address for data rows; `"group:"` + group key for group headers. |
| Null item | `E_INVALIDARG` |
| Same object reachable twice (duplicate in one list, the same child under two parents, or an object that is its own ancestor) | `E_INVALIDARG` |
| Group key | As v2: the `GroupByWithIdentity` identity selector, else the group value, which must be a value type (string, number, GUID, …). An object value → `E_INVALIDARG`. |

Failure policy is the same as v2: throw wherever detected, with no faulted state. Detection points
are `From`, each verb's rebuild, child materialization, collection-changed handlers and the
live-shaping handler. A failure inside a verb keeps the previous declaration. App delegate
exceptions are rethrown through `RethrowAsCallerError`.

## 5. Architecture

```
TableView ──::ITableViewSource──▶ TableViewSource6 ──▶ ShapedItemsSource6 ──▶ NodeTree6
App ──ITableViewShapingSource──▶        │                    │                    ▲
                                        ▼                    ├─ CollectionAccessor │
                         RowMetadataProvider6                ├─ ShapingPipeline    │
                                        │                    ├─ LiveShapingTracker │
                                        └──────▶ VisibleRowList6 ──────────────────┘
```

| Unit | Responsibility | Location |
|---|---|---|
| `TableViewSource6` | WinRT class. Implements `ITableViewShapingSource` and `::ITableViewSource`, tracks the owner, translates verbs, publishes the `ItemsSourceView` and the row metadata. Same structure as `TableViewSource2` (ReferenceTracker, UI-thread final release). | `controls\dev\TableView\TableViewSource6.h/.cpp` |
| `ShapedItemsSource6` | Engine. Owns the source and children subscriptions, verb state, identity validation, live shaping, and drives `NodeTree6`. No knowledge of TableView. | `controls\dev\ShapedItemsSource\ShapedItemsSource6.h/.cpp` |
| `NodeTree6` | Pure data structure: nodes, sibling lists, Fenwick counts, `GetAt`, `IndexOf`, expand/collapse, splice. Unit-testable without WinRT collections. | `controls\dev\ShapedItemsSource\NodeTree6.h/.cpp` |
| `VisibleRowList6` | The presented collection. Implements `IBindableVector`, `IBindableIterable` and `INotifyCollectionChanged` over `NodeTree6`. Raises range notifications. | `controls\dev\ShapedItemsSource\VisibleRowList6.h/.cpp` |
| `RowMetadataProvider6` | `ITableViewRowMetadataProvider` over `NodeTree6`: level, kind, expandable, expanded, child count, position in set, identity, expansion commands. | `controls\dev\TableView\RowMetadataProvider6.h/.cpp` |

Reused as they are: `CollectionAccessor`, `ShapingPipeline`, `ApplyPredicateFilter`,
`StableSortByKeys`, `ValueComparer`, `CallerError` / `RethrowAsCallerError`, `SwallowedFaults`,
`LiveShapingTracker` (already keyed by object address), `ShapedGroup` / `GroupedEntry`.

## 6. Data model

### 6.1 Node

```cpp
struct Node
{
    winrt::IInspectable Item;          // null for the virtual root and group headers
    Node* Parent;
    int32_t IndexInParent;             // position in Parent->Kids; -1 when not included
    int32_t Depth;
    std::vector<Node*> All;            // children in source order (unshaped)
    std::vector<Node*> Kids;           // included children in shaped (sorted) order
    Fenwick KidVisible;                // prefix sums of Kids[k]->Visible
    int32_t Visible;                   // 1 + (Expanded ? sum of Kids' Visible : 0); root: 0 + sum
    std::vector<winrt::IInspectable> SortKeys;
    winrt::IInspectable ChildrenSource;  // the object ChildrenWith returned, observed while materialized
    revokers;                          // collection-changed revokers for ChildrenSource
    bool Expanded, Materialized, SelfMatch, Included, HasChildrenHint;
};
```

- Nodes live in a pool (`std::deque<Node>` plus a free list) and are found through
  `std::unordered_map<void*, Node*>`.
- A **virtual root** holds the top-level rows: the source's items, or group headers when grouped.
  It is never presented.
- `Visible` counts the node itself plus every visible descendant. A collapsed node counts 1.
  Excluded nodes are not in any `Kids` list, so they contribute nothing.
- `Size` = `root.Visible`.

`Fenwick` is a plain binary indexed tree over `int32_t` with `Add(k, delta)`, `Prefix(k)`,
`LowerBound(i)` and `Rebuild(values)`.

### 6.2 Reads

**`GetAt(i)`**

```
n = root
loop:
    if n != root: if i == 0 return n; i -= 1
    k = n.KidVisible.LowerBound(i)       // first child whose cumulative Visible > i
    i -= n.KidVisible.Prefix(k)          // rows before child k
    n = n.Kids[k]
```

Cost O(depth · log k). A one-entry cursor `(lastIndex, lastNode)` serves `GetAt(last ± 1)` in O(1)
amortized through next/previous-visible stepping, which is the pattern of the repeater's
realization. Any structural change invalidates the cursor.

**`IndexOf(item)`**: map lookup; -1 if not found, not included, or under a collapsed ancestor.
Otherwise:

```
idx = 0
for (n = node; n.Parent; n = n.Parent):
    idx += n.Parent.KidVisible.Prefix(n.IndexInParent) + (n.Parent != root ? 1 : 0)
```

Cost O(depth · log k).

**Iteration** walks the tree in pre-order, skipping collapsed subtrees.

### 6.3 Expansion

- `SetExpanded(n, e)`: delta = sum of `n.Kids`' `Visible`, negated for a collapse. Set the flag and
  apply `n.Visible += delta`. Then for each ancestor `a` with child `c` on the path:
  `a.KidVisible.Add(c.IndexInParent, delta)` and `a.Visible += delta`, stopping above the first
  collapsed ancestor.
- If every ancestor is expanded, raise one `Add` or `Remove` at `IndexOf(n) + 1` with `|delta|`
  items. Otherwise there is no notification: the change is hidden.
- `ExpandAll` / `CollapseAll` / `ExpandSubtree`: set flags in one post-order pass, recompute
  `Visible` and the Fenwicks bottom-up, raise one `Reset`.
- Default state matches v1: nodes start collapsed and groups start expanded. Expansion state lives
  on the node and is lost when the node is released. Group expansion is kept by group key across
  regrouping.
- Filter context rows (included only because a descendant matches) present expanded while the
  filter is active, unless the user collapses them. Those collapses are recorded in an overlay set
  that is cleared when the filter changes (v2 behaviour).

### 6.4 Materialization

- **`ChildrenWith(selector)`: eager.** The whole tree is built at declaration by walking the
  selector, and every node is `Materialized`.
- **`ChildrenWith(selector, hasChildren)`: lazy.** A node's children are read only on its first
  expand, under an active filter (which needs the whole tree), or on `ExpandAll` /
  `ExpandSubtree`. Until then `hasChildren(item)` sets `HasChildrenHint`, which drives the
  expand chevron.
- Materializing a node calls the selector, resolves the result through `CollectionAccessor`,
  creates child nodes (with the duplicate checks of §4), evaluates them (filter, sort keys) and
  subscribes when the collection is observable.
- `ClearChildrenWith` drops every non-root node, then rebuilds flat.

### 6.5 Grouping

- Single level. Group headers are nodes whose `Item` is a `GroupedEntry` over a cached
  `ShapedGroup`, inserted between the virtual root and the roots. Group expansion is the same
  `Expanded` flag.
- **With ChildrenWith, only roots are grouped.** Descendants stay under their root whatever their
  own group value.
- A header with no included roots is excluded. Its `ChildCount` is its count of included roots.
- Header order follows v1/v2: sorts declared before `GroupBy` order the headers by each group's
  first root; with none, headers follow first-appearance source order. Sorts declared after
  `GroupBy` order roots within each group and every sibling list below.
- `IsGrouped` is true while a group is declared and the projection has headers.

### 6.6 Filter and sort

- **Filter verb.** Evaluate `SelfMatch` on every node (materializing lazy nodes). One post-order
  pass sets `Included = SelfMatch || any child Included`, rebuilds each `Kids` from `All`
  (preserving shaped order), the Fenwicks and `Visible`. Raise `Reset`. Groups never match
  themselves.
- **Sort verb.** Re-evaluate `SortKeys`, re-sort every materialized `Kids` with `StableSortByKeys`
  (ties keep source order), refresh `IndexInParent`, the Fenwicks and `Visible`. Raise `Reset`.
  Lazy nodes are sorted when they materialize.
- `CanApplySort` is false only for a null item; duplicates already throw.

### 6.7 Change handling

Every observed collection (the root source and each materialized children collection) is handled
by the same routine against its owning node `p`.

| Change | Handling |
|---|---|
| Insert | Create the nodes, evaluate them, insert into `p.All`, and binary-search each included node into `p.Kids` by sort keys. Rebuild `p.KidVisible` (O(k)), then propagate the `Visible` delta up the ancestors. If `p` became included, re-propagate inclusion upward. One `Add` range per contiguous visible run. |
| Remove | Raise `Remove` for the visible range first, then erase from `p.All` and `p.Kids`, release the subtree (map entries, revokers, live subscriptions), rebuild the Fenwick, propagate. Exclude ancestors that kept no match (filter). Remove an emptied group header. |
| Replace (same object) | Treated as a live change. |
| Replace (different object) | Remove + insert. |
| Move | Remove + insert of the same node (state kept). |
| Reset | Reconcile `p`'s children by object identity: survivors keep their nodes, subtrees, subscriptions and expansion; only departed objects are released and only new ones created. All survivors are re-evaluated, then shaped. The presented rows before and after are compared by identity: no change raises nothing, a one-sided change raises one `Add` or `Remove` range, anything else raises `Reset`. |
| Live: sort value | Re-splice the node within `p.Kids`; Fenwick rebuild of `p`; one `Remove` + `Add` (or nothing when its position is unchanged). |
| Live: filter result | Update `Included` up the ancestors; splice in or out; range notifications. |
| Live: group value of a root | Move it to another header, creating or removing headers. |
| Live: group value of a descendant | Ignored. |

Notifications are raised after the tree is consistent, so a handler that calls `GetAt` reads the
new state. A `Remove`'s count is captured before the subtree is detached.

If handling a source change throws part-way, the engine marks its cache invalid, raises `Reset`, and posts a deferred reload to the owner's `DispatcherQueue`. The exception is swallowed only when the post succeeds; otherwise it propagates. An insert whose attach cannot be done in place (for example, it needs a new group header) falls back to a full shape and one `Reset`.

## 7. Projection: `VisibleRowList6`

- Implements `IBindableVector`, `IBindableIterable` and `INotifyCollectionChanged`.
  `InspectingDataSource` prefers `INotifyCollectionChanged`, which carries ranges, so a collapse of
  any size is one `Remove(start, count)`, not N single events or a Reset.
- `NewItems` / `OldItems` are count-only views: an `IBindableVector` of the right `Size` whose
  `GetAt` returns null. The repeater reads only `Size()`, so nothing allocates per row. `Reset`
  carries null items.
- Mutating members (`Append`, `SetAt`, …) throw `E_NOTIMPL`: the list is read-only.
- One instance lives for the engine's lifetime; verbs raise `Reset` instead of swapping it.

## 8. Live shaping

Same contract as v2, keyed by object address through `LiveShapingTracker`: every node's item is
subscribed while `IsLiveShaping` is true; changes are coalesced into one restore per UI turn on the
owner's `DispatcherQueue`; the edit hold, `LiveItemChangedHook`, `RowsReleasedHook`,
`LiveItemReshapeHandler`, off-thread marshalling and the dropped-change flag are kept. A
`PropertyChanged` that leaves sort keys, filter result and group key unchanged is ignored.
`ShapingChanged(reorderOnly = true)` is raised only when every change was a move within a sibling
list. When GroupBy is active together with sort axes, a live sort change falls back to a full
rebuild, because the group header order may change too.

## 9. Threading and lifetime

Same as v2: verbs and projection on the owner UI thread; `TableViewSource6` final release
marshalled to the UI thread; single owning TableView with owner-released waiters; engine callbacks
hold weak references.

## 10. Complexity

N = all nodes, k = siblings, d = depth, m = rows shown or hidden.

| Operation | v6 | v2/v3 (pre-order array) |
|---|---|---|
| `GetAt(i)` | O(d log k); O(1) sequential | O(1) |
| `IndexOf(item)` | O(d log k) | O(log n) |
| Expand/collapse | O(d log k), one range event | O(m) scan + splice |
| Insert/remove of a child | O(k + d log k) | O(n) array shift |
| Filter / sort verb | O(N) / O(N log k) | O(N) / O(N log k) |
| Memory beyond the model | One node per materialized item | Pre-order array + visible array + maps |

## 11. Testing

- **API tests** (`TableViewSource6_APITests.cs`): `From` with null and non-null key; `ParentBy`
  throws; flat sort/filter; GroupBy; ChildrenWith eager and lazy; GroupBy + ChildrenWith groups
  roots only; expand/collapse counts and indices; duplicate object and self-ancestor throw;
  insert/remove in a nested children collection; live sort change.
- **Parity:** the presented rows for representative declarations equal v1's (`"default"`).
- **`NodeTree6` unit checks:** `GetAt` / `IndexOf` round-trip over random trees and expansion
  states, delta propagation through collapsed ancestors, the cursor.
- **Perf:** a `TableViewSrcV6` bench built with the `Children` API and the registry name `"v6"`,
  run on the same 21 hierarchy scenarios as v3–v5.

## 12. Order of work

Each step builds clean before the next.

1. `NodeTree6` and `VisibleRowList6`; flat rows; IDL, registry and `TableViewSource6` glue;
   `RowMetadataProvider6`.
2. Sort and filter verbs; column-header sort commands.
3. ChildrenWith, eager and lazy; expansion; nested collection changes.
4. GroupBy, including GroupBy + ChildrenWith.
5. Live shaping.
6. API tests, VM run, bench.

## 13. Decisions log

| # | Decision | Alternatives rejected |
|---|---|---|
| D1 | Object identity via `IUnknown` address | External key (v2) |
| D2 | `ParentBy` throws; ChildrenWith is the only hierarchy | Support `ParentBy` by building wrappers from parent keys |
| D3 | Visible rows are resolved by walking wrapper nodes with visible-subtree counts | Flat visible list (v1/v4/v5); pre-order array + visible index (v2/v3); separate visible-only tree |
| D4 | Per-sibling-list Fenwick for counts | Linear scan over siblings (O(k) per step); lazily rebuilt prefix arrays (O(k) after every change in a large list); one global order-statistics tree |
| D5 | `INotifyCollectionChanged` range notifications with lazy item views | `IObservableVector` single-item events; Reset on every expansion |
| D6 | Eager tree for `ChildrenWith(sel)`, lazy for `ChildrenWith(sel, has)` | Always eager; always lazy |
| D7 | GroupBy + ChildrenWith groups roots only | Group every sibling set; throw |
| D8 | Null item or an object reachable twice throws | Allow DAGs with per-path nodes |
| D9 | Expansion state on the node | Separate expansion model keyed by identity string |
| D10 | Change args carry count-only item views | Lazy views that resolve rows through the tree |
| D11 | A failed source change raises `Reset` and posts a deferred reload | Faulted state; rethrow and leave the tree inconsistent |
| D12 | A grouped live re-splice with sort axes forces a full rebuild | Re-sorting the header run in place |
| D13 | A source `Reset` reconciles nodes by identity and raises only the presented difference | Release and recreate every node, then raise `Reset` |
