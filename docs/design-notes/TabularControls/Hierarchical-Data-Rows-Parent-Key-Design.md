# TableView — Hierarchical Data Rows from a Flat Source (Parent Key) — Design

Status: **implemented** (decisions in §13).

## 1. Motivation

`TableView` reads hierarchy **from a relation inside a flat collection**: every item is a row
candidate, and two key selectors say "this is my key" and "this is my parent's key".

```csharp
var byManager = TableViewSource.From(employees).ParentBy(
    item => ((Employee)item).Id, item => ((Employee)item).ManagerId);
var byMentor  = TableViewSource.From(employees).ParentBy(
    item => ((Employee)item).Id, item => ((Employee)item).MentorId);
```

Why:

- **One source, many trees.** The parent relation is a *view* decision, not a data-shape decision.
  The same collection shows up as an org chart, a mentoring tree or a flat list, depending on the
  selector. A nested model bakes one hierarchy into the object graph. This is the main reason for
  the change.
- **This is how data arrives.** Rows from a database, a REST endpoint or a CSV are flat, with a
  parent id column. Apps would otherwise have to build and maintain a nested object graph just to
  feed the grid.
- **One collection to observe.** Adding, removing or reparenting a row is one change on one
  collection. Per-node child-collection subscriptions go away entirely.
- **It is the established flat-mode API.** Every major tree-grid supports it:

| Framework | Control | Flat (self-reference) API |
| --- | --- | --- |
| DevExtreme | `TreeList` | `dataStructure: 'plain'` + `keyExpr` + `parentIdExpr` |
| Kendo UI | `TreeList` | `idField` + `parentIdField` |
| Syncfusion (web) | `TreeGrid` | `idMapping` + `parentIdMapping` |
| Syncfusion (WPF/WinUI) | `SfTreeGrid` | `ParentPropertyName` + `SelfRelationRootValue` |
| DevExpress (WPF) | `TreeListControl` | `TreeDerivationMode="Selfreference"` + `KeyFieldName` + `ParentFieldName` |
| Telerik (WPF) | `RadTreeListView` | `TableRelation` with `IsSelfReference` |
| AG Grid | `AgGrid` | `treeData` + `getDataPath`, or `treeDataParentIdField` |

A nested source mode, where each parent object owns a child collection, is not part of this design.
It can be added later as a separate verb over the same adapter (§11).

## 2. Public API

```idl
// Existing delegate, reused for both selectors. No new delegate types.
delegate Object TableViewKeySelector(Object item);

runtimeclass TableViewSource
{
    // Declares the source a HIERARCHY defined by a parent-key relation over its own items.
    //
    // keySelector:       the item's own key.
    // parentKeySelector: the key of the item's parent. Null means "root".
    //
    // Both required and non-null (E_INVALIDARG otherwise; use ClearParentBy() to remove).
    TableViewSource ParentBy(TableViewKeySelector keySelector, TableViewKeySelector parentKeySelector);

    TableViewSource ClearParentBy();

    // Opt-in (default false). Reshapes when an item's PropertyChanged moves a value a verb reads.
    Boolean IsLiveShaping { get; set; };
}

runtimeclass TableView
{
    // Bulk node expansion, separate from the group verbs.
    void ExpandAllRows();
    void CollapseAllRows();
}
```

Rows are presented through the `TableViewRowIndentSize` and `TableViewRowExpanderSize` theme
resources, the `TableViewRow` `Level` / `IsExpandable` / `IsExpanded` DPs, the row template's
expander gutter and states, and the `TableViewRowAutomationPeer` `IExpandCollapseProvider`.

The names follow the rule the source already uses: the verb is named after the thing being
configured, and the removal verb mirrors it (`Filter`/`ClearFilter`, `GroupBy`/`ClearGroupBy`).
`ParentBy` takes the `GroupBy` form: it reads as "parent the rows by these keys", and
`ClearParentBy` removes it as `ClearGroupBy` removes `GroupBy`.

`IsLiveShaping` applies to every shaping verb, flat or hierarchical:

- **Default `false`.** Without it, an in-place property edit takes effect at the next collection
  change or reshape.
- **What is observed.** Each source item that implements `INotifyPropertyChanged` gets one
  subscription (held weakly where the item supports weak references). Per item, the source keeps a
  snapshot of what the declared verbs read: the filter result, the group key, the sort keys, and,
  under `ParentBy`, the node key and parent key. A `PropertyChanged` that leaves the snapshot
  unchanged is ignored.
- **When it reshapes.** A change that moves the snapshot posts one reshape to the source's
  dispatcher; further changes in the same turn coalesce into it. A change raised on another thread
  is marshaled to the source's thread first; if that post is refused, the change is picked up by
  the next live change on the source's thread. The reshape re-applies the whole pipeline: filter,
  sort, group and parent relation.
- **Edits.** While a cell edit is in progress the reshape is held, and it runs once the edit
  closes, so live data never ends the user's edit.
- **Turning it on** subscribes every item and posts one catch-up reshape, because edits made while
  it was off were not tracked. Turning it off revokes every subscription.
- **Cost.** One subscription and one snapshot per item, kept in step with collection changes, plus
  one snapshot capture per changed item while no reshape is pending. A reshape recaptures only the
  snapshots of the items that raised a change; it re-reads every item only after a reset, a
  `VectorChanged`-only source change, a spec change, or a change it could not attribute to an item.
  Under `ParentBy` it also re-shapes only the sibling sets those items sort in (§6.2); a flat or
  grouped reshape still re-filters and re-sorts the whole source.

## 3. Key semantics

### 3.1 What a key is

A key is any object the selector returns. It is converted to a string identity once per item per
rebuild (`MakeNodeKey`, built on `ShapingHelpers::ValueKey::ToObjectLookupKey`):

- **Value types** — anything boxed as an `IPropertyValue`: strings, the numeric types, `Boolean`,
  `Char16`, `Guid`, `DateTime`, `TimeSpan`, `Point`, `Size`, `Rect` and WinRT enums — are compared
  **by value**. `42` from `Id` and `42` from `ManagerId` match. This is the database-id case. A
  fresh box per selector call is fine.
- **Any other object** is compared **by reference identity**. So
  `ParentBy(item => item, item => ((Employee)item).Manager)` works: the item is its own key and the parent selector
  returns the parent object. This covers the object-reference case without a second overload. Both
  selectors must return the same instance for a link to form.

The value form keeps a type tag, so mixed types do not match: an `Int32` key never matches an
`Int64` parent key, and an enum never matches an enum of another type or a plain integer. This is
documented, not coerced. Floating-point keys (`Single`, `Double`, and the components of `Point`,
`Size` and `Rect`) compare as numbers: `-0` equals `+0`, and every NaN equals every other NaN of the
same type.

An enum declared in app code that is not a WinRT type (a plain C# `enum`) is not boxed as an
`IPropertyValue`; it reaches the control as an opaque object and is therefore compared by reference
identity, which a fresh box never satisfies. Apps convert such keys to their integer value.

Every key object is held for as long as its node is in the retained structure. An object key's
lookup form is its address, and a released temporary's address could otherwise be reused by a later
selector result and alias it.

### 3.2 Roots, orphans, and invalid data

Evaluated over the **whole unfiltered source** on every rebuild.

| Case | Behaviour | Rationale |
| --- | --- | --- |
| Parent key is null, or an empty string | Root | The usual "no parent" encoding. |
| Parent key matches no item's key (**orphan**) | Root | Matches Syncfusion, DevExpress WPF (default) and AG Grid. DevExtreme hides orphans instead (§14). Treating them as roots also covers "root value" conventions (`ParentId = 0` or `-1`) without a `SelfRelationRootValue` knob, and keeps partially loaded data visible. |
| Two items with the same key | Throw `E_INVALIDARG` naming the key | A key must name one parent. Either winning silently would reparent rows at random. |
| Key selector returns null or an empty string | Throw `E_INVALIDARG` | An item with no key cannot be anyone's parent, and expansion intent cannot be stored for it. |
| Item is its own parent | Throw `E_INVALIDARG` (cycle) | |
| Cycle (`A → B → A`) | Throw `E_INVALIDARG` naming one key on the cycle | Items on a cycle are unreachable from any root and would silently disappear. |
| Selector throws | Treat the result as null. For the key selector that means the throw above. | Matches how `RebuildGrouped` guards `m_groupSelector`. |

Bad identity fails fast: a throw leaves the previously published projection intact. The relation
stays declared after a throw, so every later reshape (another verb or a source collection change)
re-validates and throws again until the data is fixed or `ClearParentBy()` is called.

When invalid data arrives through a source collection change rather than through a verb, the engine
throws `E_INVALIDARG` from its collection-changed handler. Whether that reaches the app's own
mutation call depends on how the collection raises its events: a .NET `ObservableCollection`
propagates it, while C++/WinRT observable vectors swallow handler exceptions. The previous projection
stays on screen either way.

**Cycle detection is a reachability count, not a path walk.** After the index is built, walk from
every root and count the reached items. Because orphans already became roots, every non-root item
has a parent inside the set, so any item that is not reached is on a cycle, or below one. This costs
O(n) and needs no depth limit. To name a key that is really on the cycle, the error walks the parent
chain from the first unreached item until a node repeats; that node is on the cycle.

### 3.3 Duplicate objects

A flat source that contains the same object twice already fails in `ValidateRowIdentities` with the
existing diagnostic. Every hierarchical row is a source row, so that one check covers the whole tree.

## 4. Shaping semantics

### 4.1 Sort: within each sibling set

Bucket the flat list by parent in source order, then sort **each sibling list among its own peers**
with `ApplySort`. No depth is ever compared with another. The sort is stable and each list goes in
in source order, so ties break exactly as one sort over the whole source would. Sorting k siblings
costs O(k log k), never more in total than one O(n log n) sort over the source.

### 4.2 Filter: keep matches and their ancestors

With a flat source the whole tree is already in memory, so retaining ancestors costs no extra
materialization. v1 filters **with ancestors**:

- **kept** = items the predicate matches, plus every ancestor of a match.
- A matched item's descendants are **not** kept automatically. They must match on their own.
- A kept ancestor that did not match is a **context row**. It renders normally and its children are
  restricted to the kept set.

This is the default of DevExtreme (`filterMode: 'withAncestors'`) and Syncfusion (`Parent`). It is
the only mode where a match deep in the tree is actually reachable. With match-node-only filtering, a
match under a non-matching parent is simply lost.

Cost: one parent-chain walk per match, stopping at the first ancestor already marked kept. O(n)
overall.

Other modes (match-only, full branch) can be added later as an additive `TableViewFilterMode`
parameter. Not in v1.

**Filter and expansion.** Once filtered, context ancestors of a match must be expanded, or the match
stays hidden under a collapsed root (the baseline is collapsed, §5.2). Rule:

> While a filter is active, a context row is presented expanded unless the user collapsed it under
> this filter. Its persistent intent is not consulted, and a toggle on it never writes intent: user
> toggles on context rows are overlay-only and are discarded when the filter changes or clears, so
> the tree the user left comes back.

Mechanism: the adapter receives the set of context keys with the index. `Emit` computes
`isExpanded = HasChildren && (isContext ? !m_filterOverlayCollapsed.contains(key) : m_expansion.IsExpanded(key))`.
A collapse on a context row inserts into `m_filterOverlayCollapsed` and an expand erases from it;
the set is cleared when the filter changes. `ExpandSubtree` erases context keys from the overlay and
writes intent only for non-context keys; `ExpandAll` clears the overlay and `CollapseAll` fills it
with every context key. Persistent intent stays in `RowExpansionModel` and is never written from a
context row's individual toggle.

**Bulk commands are the exception, deliberately.** `ExpandAllRows` / `CollapseAllRows` are an
explicit whole-tree change of intent, not a toggle on a row. They move the persistent baseline for
**every** node, context rows included (§5.2), as well as the overlay above. So after
`CollapseAllRows` under a filter, clearing the filter shows the whole tree collapsed, and after
`ExpandAllRows` it shows the whole tree expanded; the per-row state from before the filter is not
restored. Individual toggles on context rows remain overlay-only.

### 4.3 Grouping: buckets the roots

`GroupBy` buckets the **roots** only, each root's visible
subtree follows it, and group headers exist at depth 0 and nowhere else. Under a filter, a context
root is bucketed like any other root. Roots are ordered bucket by bucket, and the adapter must not
re-sort them. Bucket (header) order follows plain `GroupBy` exactly: roots are bucketed in source
order with only the sorts declared before `GroupBy` applied, and sorts declared after it order the
roots within each bucket. Descendant sibling sets keep the full sort (§4.1).

## 5. Expansion

### 5.1 Node identity is the item object

```
nodeId       = canonical IUnknown of the item
row identity = "node:0x" + <hex address of nodeId>
```

The key selector only resolves parent keys to parent items; it is not the row identity:

- The node id is flat, O(1) in size, and costs no selector call.
- It survives re-sort, re-filter, regroup and **reparenting**. A node the user expanded stays
  expanded when it moves.
- It does **not** survive the app re-creating the item object, even with the same key: the new
  object is a new, collapsed, unselected row. This is the flat and grouped contract too.
- It is also the row identity for the metadata provider (`GetIdentity`), so selection re-anchors on
  it too.
- The address is meaningful only while the item is alive. The structure holds every indexed item,
  and intent is pruned against it on every publish (§5.3), so a recycled address never inherits a
  departed node's intent.
- Selection follows the presented row. When a collapse hides the selected row, the selection is
  cleared rather than held for the hidden row, and neither expanding again nor a later reshape
  restores it. Collapsing a group behaves the same way.

### 5.2 Baseline, bulk verbs, and redeclaration

- The baseline is **collapsed** (`m_expansion.SetAllExpanded(false)` in the adapter constructor). Any
  source can hold more rows than should be realized on first paint.
- `ExpandAllRows` / `CollapseAllRows` move the baseline. They call no app code: the index already
  exists, so the cost is only the size of the visible axis. Moving the baseline changes persistent
  intent for every node, including filter context rows, so the result outlives the filter (§4.2).
- **Last writer wins.** A second `ParentBy` call *replaces* the previous relation. It does not
  stack or compose with it, which matches how `GroupBy` replaces a previous `GroupBy`. The replaced
  hierarchy is torn down as if `ClearParentBy()` had been called, and its expansion intent is
  cleared with it. A different relation is a different tree: "Bob is expanded" in the org chart
  says nothing about Bob in the mentoring tree.

### 5.3 Pruning: the node set is complete

A flat source always produces the full set of nodes, so intent is pruned with a plain whole-set
retain. After every publish (§6.1):

```
m_expansion.RetainIf(key -> index->ContainsNode(TryParseNodeIdentity(key)))
```

The prune set is the **unfiltered** source, so a node hidden by a filter keeps its intent. This is
the grouping rule, and it is correct for the same reason: the live-node set is complete. The prune
runs on every publish, not only for a new structure, because a patched structure (§6.2) can lose a
node whose address a later item may reuse.

## 6. Architecture

The feature is split into four layers, each depending only on the ones above it.

```
Layer 1  TabularShaping      ParentStructure + ParentKeyIndex (pure, headless): validate; filter + sort
Layer 2  ShapedItemsSource   owns source observation; builds/reuses the structure and the index; bucketizes roots
Layer 3  HierarchicalSourceAdapter   walks the index -> visible rows + descriptors; splices toggles
Layer 4  RowMetadataProvider / TableView / TableViewRow / peers   read row metadata; drive expansion
```

### 6.1 Layer 1 — `ParentStructure` and `ParentKeyIndex`

Two stages. The **structure** is the validated tree over the unfiltered source and is independent
of filter and sort, so reshapes reuse it, and source changes patch it. The **index** is one
filtered, sorted reading of it, assembled from a cache of shaped sibling lists.

```cpp
namespace ShapingHelpers
{
    using NodeId = void*;                                               // canonical IUnknown of the item

    struct ParentStructure
    {
        std::vector<winrt::IInspectable> Rows;                          // unfiltered, source order
        std::unordered_map<NodeId, ParentNode> Nodes;                   // item, key objects, lookup keys, Parent, Rank
        std::unordered_map<std::wstring, NodeId> NodeByKey;             // lookup key -> node (parent resolution only)
        std::unordered_map<std::wstring, std::vector<NodeId>> Orphans;  // unknown parent key -> waiting roots
        std::unordered_map<NodeId, std::vector<NodeId>> Children;       // per parent, Rank order
        std::vector<NodeId> Roots;
        uint64_t Epoch;                                                 // unique per build
        // Change log consumed by the next index build: DirtyLists, DirtyNodes, RemovedItems, EdgesChanged.
    };

    struct ParentKeyIndex
    {
        std::vector<winrt::IInspectable> Roots;                              // shaped order
        std::shared_ptr<const ListMap> Children;                             // parent node -> shaped children (shared with the cache)
        std::shared_ptr<const std::unordered_set<NodeId>> ContextNodes;      // filter-retained ancestors (§4.2)
        std::shared_ptr<const ParentStructure> Structure;
    };

    // Runs both selectors once per row. False with `error` on duplicate/null key, self-parent or cycle.
    bool BuildParentStructure(rows, keySelector, parentKeySelector, ParentStructure& out, winrt::hstring& error);

    // Patches mirroring a source change; run no selector (callers resolve new items first with
    // ResolveParentNode). False = the structure is unusable and must be rebuilt.
    bool InsertParentNodes(structure, index, resolvedNodes);
    bool RemoveParentNodes(structure, index, count, expectedItems);
    bool MoveParentNodes(structure, oldIndex, newIndex, count);
    bool UpdateParentNode(structure, resolvedNode);                     // live shaping: re-read one item's edge

    // Runs no selector. Rebuilds only what the change log and the filter/sort generations invalidate.
    void BuildParentKeyIndex(structure, ParentShapeCache&, filter, filterGeneration, sort, sortGeneration, bool sortRoots, ParentKeyIndex& out);
}
```

`BuildParentStructure`, all O(n):

1. Evaluate the key selector once per item, holding every returned key object (§3.1). Reject a
   null or duplicate key.
2. Evaluate the parent selector; resolve it to a node, or leave the node a root for a null or empty
   parent key, or an orphan root (recorded in `Orphans`) for an unknown one. Reject a self-parent.
3. Bucket child lists in source order, then check reachability from the roots (§3.2). Reject cycles.

Patches keep the same semantics. An inserted node adopts the orphans waiting for its key (a lookup,
not a scan) and links to its parent; a removed node's children become orphan roots waiting for its
key. A new edge is checked for a cycle by walking up from the new parent only. `Rank` (the row's
source position) is the sibling tiebreak and is renumbered from the change point.

`BuildParentKeyIndex` keeps a `ParentShapeCache` per structure epoch:

1. If there is a filter, re-read the verdict of every node when the filter generation moved, else
   of the dirty nodes only. Recompute the kept set (matches plus ancestors) and `ContextNodes` in
   one flat pass only when a verdict, an edge or the filter moved; a node entering or leaving the
   kept set dirties its parent's list.
2. Re-emit and re-sort (stably, through the `SiblingSorter` callback, §4.1) only the dirty sibling
   lists, or all of them when the sort generation, filter presence or `sortRoots` changed. The list
   map is copied on write, so a published index never sees a later build. Roots are left in source
   order when `sortRoots` is false, so layer 2 can bucket them for grouping (§4.3).

A published index is immutable; its structure is patched in place between publications, on the UI
thread only and never during a refresh, so it can run ahead of the index until the next publish.

This is the testable core of the feature and has no dependency on XAML, a dispatcher or the adapter.

### 6.2 Layer 2 — `ShapedItemsSource`

- `SetParent(key, parentKey)` / `ClearParentBy()` set `m_hierarchyAxisDirty`. The hierarchy axis has
  no description in the pipeline spec, so without the flag a verb-only change would diff as a no-op
  and never rebuild. Both clear expansion intent (§5.2).
- `ClearParentBy()`, and every rebuild into a projection that is not hierarchical, releases the
  adapter: its index and intent are cleared and the engine drops its reference, so the last visible
  rows are not kept alive. The previous row-metadata provider shares ownership until the projection
  swap replaces it; the next declaration creates a fresh adapter.
- `RebuildHierarchical(rows)`: reuse the retained `ParentStructure` when allowed (see below), or
  build one with `BuildParentStructure` (throw on failure with the diagnostic); then
  `BuildParentKeyIndex(structure, filter, sort, sortRoots = true)`, which sorts each sibling list
  through `ApplySort`; then `m_hierarchicalAdapter->SetIndex(index)`. Rows stay unfiltered and in
  source order up to the index build, because ancestor retention needs the unfiltered parent chain.
- `RebuildGroupedHierarchical(rows)`: the same, with `sortRoots = false`; the roots are then
  bucketized as `GroupBy` does and the bucket-ordered roots replace `index.Roots` before the index
  is handed over.
- **Source changes.** Layer 2 already observes the source. Every change re-indexes the tree (the
  flat incremental fast paths bail out while a parent relation is declared), but an
  `INotifyCollectionChanged` Add, Remove, Replace or Move first **patches** the retained structure
  (§6.1), calling the selectors for the new items only. A Reset, a `VectorChanged`-only change, a
  change deferred behind a running refresh, or a patch that would leave the tree invalid drops the
  structure; the next refresh rebuilds it and reports any error.
- **Retained tree structure.** The resolved parent structure (keys, parents, child lists) is kept
  across reshapes and source changes, and dropped by re-declaring the relation or by the cases
  above. A refresh reuses it when it saw every source change and the materialized rows are the same
  objects in the same order (an O(n) pointer compare that catches sources without change
  notifications; no selector runs). Keys are otherwise **trusted**:
  - **Live shaping off**: an item whose key or parent key is edited in place keeps its position,
    even across a re-sort or re-filter, until a source change touches it or the relation is
    re-declared. The same holds for sort and filter inputs of sibling lists the reshape does not
    otherwise touch.
  - **Live shaping on**: each item that raised a `PropertyChanged` is re-read on the next reshape
    (`UpdateParentNode`): a moved key or parent key re-links that one node, with a local cycle
    check, and its sibling list is re-shaped. A reshape that reconciles every snapshot instead (see
    §2) compares each node and parent key with the previous pass and drops the structure on any
    difference. Turning live shaping on drops the structure, because edits made while it was off
    were never tracked, and posts one reshape so those edits show without waiting for the next
    change.
- **Property changes.** With `IsLiveShaping` on, an item's
  key and parent key are observed and a change reparents the row on the next dispatcher turn. A
  change raised on another thread is handed to the source's own thread first. While a cell editor
  is open, the reshape waits and runs once the edit closes, so live data never ends a user's edit.
  Otherwise reparenting through `INotifyPropertyChanged` on `ManagerId` takes effect at the next
  collection change or reshape, the same contract as sort keys. To reparent immediately, an app
  replaces the item or removes and re-inserts it.
- **Custom comparer columns.** A header sort on a `CustomSortComparer` column ranks the rows once
  and sorts by those ranks. With `IsLiveShaping` on, each changed item is taken out of the ranking
  and compared back in (O(n) comparer calls), and the reshape runs when any row's rank moved. A
  burst of changes that already has a reshape pending only drops ranks; once more items wait than
  log2(n), the next key read re-ranks everything in one merge sort. Turning `IsLiveShaping` on
  re-ranks everything once (edits made while it was off were not tracked), and an item that
  arrives while it is on is compared back in rather than keeping a rank from before it left.
- `Rows()` is the snapshot of the visible rows.

### 6.3 Layer 3 — `HierarchicalSourceAdapter`

- **Input.** `SetIndex(shared_ptr<const ParentKeyIndex>, rootSegments)`. The adapter observes
  nothing; layer 2 is the only listener on the source. One `SetIndex` produces exactly one
  `Rebuild`, and intent is pruned only after the new projection is live, so no intermediate state
  can prune intent against an empty tree.
- **Walk.** Iterative, with an explicit stack. No app calls and no cycle checks, because the index is
  validated. No depth limit.
- **Output.** `m_entries` + `m_entriesView` (one stable `ItemsSourceView`), the `NodeRow`
  side-vector, the node key → index map and the lazy item → index cache. `InsertRows` /
  `RemoveRows` keep those structures consistent as a set. `ProjectionChanged` is raised once per
  coherent publish; rebuilds requested during a publish are coalesced (`m_rebuildInFlight`). The
  baseline is collapsed.
- **Pruning.** After every publish, the adapter retains only the intent of nodes in
  `index->Structure` (§5.3).
- **Threading.** The adapter owns an `ItemsSourceView` and must be created on a thread with a
  `DispatcherQueue` (`RPC_E_WRONG_THREAD` otherwise); unlike layer 1 it is not headless.

`NodeRow`, filled during the walk at no extra cost:

```cpp
struct NodeRow
{
    winrt::IInspectable Item{ nullptr };
    winrt::hstring NodeKey;
    winrt::hstring ParentKey;   // empty for roots; backs Left-to-parent (§8)
    int32_t Depth{ 0 };         // 0-based
    int32_t ChildCount{ 0 };    // always known: Children[key].size()
    int32_t SiblingIndex{ 0 };  // 1-based within the shaped sibling set -> PositionInSet
    int32_t SiblingCount{ 0 };  // -> SizeOfSet
    bool HasChildren{ false };
    bool IsExpanded{ false };
    bool IsContext{ false };    // retained only as a filter ancestor (§4.2)
};
```

`SiblingIndex` / `SiblingCount` make the peer's `PositionInSet` / `SizeOfSet` O(1). Under grouping a
root's sibling set is its group bucket.

**Splice.** Expanding reads `index->Children[key]` and emits that subtree, honouring nested intent.
Collapsing removes the contiguous run of deeper rows. Neither calls app code, so neither can throw
part-way. Rows are spliced one at a time with each descriptor inserted (or removed) immediately
before its row, so the side-table always matches the published rows when a notification fires; the
node-key map is rebuilt lazily from the descriptors. A run longer than `c_maxSpliceRows` (64) falls
back to one Rebuild and a single Reset.

### 6.4 Layer 4 — consumers

- `RowMetadataProvider`: `GetIdentity` returns `NodeKey`. `IsNodeExpansionKey` checks the `node:`
  prefix. `GetRowInfo` reports `ChildCount` directly. `CreateForHierarchicalRows` and
  `CreateForGroupedHierarchicalRows` build the two hierarchical providers.
- `TableView`, `TableViewRow`, the XAML template and `TableViewRowAutomationPeer` read row level,
  expandability and expansion from the provider. `ShapedGroup` / `IGroupChildCount` report a group
  header's count as its number of roots.

## 7. Components

| Component | Role |
| --- | --- |
| `TableViewSource.idl` | `ParentBy` / `ClearParentBy`. |
| `ParentKeyIndex` / `ParentStructure` (layer 1) | Validate the relation; filter and sort it (§6.1). |
| `ShapedItemsSource` hierarchy paths | Index build, filter inside the build, incremental bail-out, intent reset on redeclaration, adapter release on retraction (§6.2). |
| `HierarchicalSourceAdapter` | Walk, splice and publish the visible rows (§6.3). |
| `ShapedGroup` + `IGroupChildCount`, `ResliceGroupsFromHierarchy` | Grouped composition (§4.3). |
| `RowMetadataProvider` hierarchical kinds | Row metadata and expansion endpoints for the control. |
| `TableView` bulk verbs, restamp-on-reset, indent cache | Control-level expansion and row stamping. |
| `TableViewRow` DPs, template parts and states, gutter, indent | Row presentation (§7.1). |
| `TableViewRowAutomationPeer` | Expand/collapse pattern, level, O(1) sibling position. |
| `TableViewSampleApp` `HierarchyData` / `HierarchyPage` | Flat data with two relations (§10). |

### 7.1 Expander gutter and frozen columns

`PART_RowExpanderGutter` overlays the row, while the indent padding is on the lead cell. When the
lead column is frozen and the table scrolls horizontally, the gutter follows the same translation
`ApplyFrozenColumnLayout` applies to the lead cell, so the chevron stays with its row text. The
chevron width is read from the `TableViewRowExpanderSize` resource.

The gutter toggles on a primary-button press (as the group header does); a secondary or middle
press is left unhandled for the row. A press on a leaf's gutter is also left unhandled, so it
selects the row.

**Template contract.** `PART_RowExpanderGutter` and `PART_RowExpanderIcon` are optional, but a
template declares them together with the `RowHierarchyStates`, `RowExpandabilityStates` and
`RowExpansionStates` groups or not at all: the groups are entered on every row (flat and grouped
tables included) and the default template's states resolve `PART_RowExpanderIcon` by name, so
groups without the part fail at state application, while neither is fine. Code null-checks the
gutter, so omitting it loses only the pointer gesture and visible chevron (keyboard and the
ExpandCollapse pattern still work); the lead cell's chevron reservation is not tied to it. In the
default template the gutter sits inside `PART_CellForegroundPresenter`, after the cells host for
hit-testing, and the icon sets no `Foreground` of its own so it inherits the selected/High Contrast
foreground. Recolouring it from the `Selected*` states would reference it from the required
`CommonStates` group and make it required.

`TableViewRowAutomationPeer` advertises ExpandCollapse only for rows of a hierarchical source;
reporting `LeafNode` on every row of a flat or grouped table would present it as a collapsed tree.

## 8. Keyboard

On a focused hierarchical row (row container focused, not a cell editor):

| Key | Collapsed expandable | Expanded | Leaf |
| --- | --- | --- | --- |
| `Right` (mirrored in RTL) | Expand | Focus the first cell | Focus the first cell |
| `Left` (mirrored in RTL) | Move focus to the parent | Collapse | Move focus to the parent |
| `*` (Multiply) | Expand this subtree | Expand this subtree | Not handled |

This is the ARIA treegrid pattern layered on TableView's row/cell drill
(`TableView::TryHandleRowLevelDrillKey`): `Right` on a row that has nothing to expand drills into its
cells, and `Left` on the first cell returns to the row. `Left` on a root row is consumed, so focus
does not leave the table. `Left`/`Right` are handled in TableView rather than on the row, because
TableView also processes keys a focused row already handled; taking them in both places would run
two actions for one press. The tree actions need Ctrl, Shift and Alt up; with Shift the plain drill
applies, and Ctrl/Alt chords fall through. A move to the parent selects that row, as Up/Down do
(selection follows the keyboard cursor). Down reaches the first child of an expanded row.

"Move to parent" resolves `NodeRow::ParentKey` to an index through the key map, which is O(1).

The index knows the whole subtree, so `*` is cheap: the adapter's `ExpandSubtree(nodeKey)` records
intent for the node and every descendant with children in one batch, then runs one `Rebuild`.

A key the row does not act on is **not** marked handled.

## 9. Cost model

| Operation | Cost |
| --- | --- |
| Rebuild (source change, re-declared relation, edge change) | O(n) key/parent selectors + O(n log n) per-sibling sorts + O(n) index + O(visible) walk. n = source size. |
| Reshape (sort/filter/group over an unchanged source) | Structure reused. Live shaping off: O(n) key/parent selectors to confirm it, no key table or validation. Live shaping on: no extra selectors. Then per-sibling sorts + O(n) index + O(visible) walk. |
| Single toggle (ungrouped) | O(run) emit + O(visible) index-shift sweep |
| Single toggle (grouped) | Every group is re-sliced and the grouped axis rebuilds (§11) |
| `ExpandAllRows` | O(n) visible rows, no app calls |
| Metadata / automation lookups | O(1) |

Collapsed subtrees are not lazy: a flat source is materialized by definition, so there is nothing
to defer. Grouping over a flat source already pays O(n) per rebuild, so this adds no new class of
cost.

The perf gate is the **100k perf** button on the sample's Hierarchical rows page
(`Samples/TableViewSampleApp/Pages/HierarchyPage.xaml.cs`, `RunPerfAsync`), which reports its results in
the page status text. It generates 100,000 flat employees
(branching 3) and times `ParentBy`, `Sort(Name)` and `ExpandAllRows` against a `GroupBy(Dept)`,
`Sort(Name)`, `ExpandAllGroups` baseline over the same rows. Groups start expanded, so the baseline
collapses them (untimed) before the timed `ExpandAllGroups`; both expand steps then realize the same
100k rows. Each step includes a synchronous layout pass. Target: total within about 2x the baseline.

Measured (median of 3 runs after one discarded warm-up; product chk/amd64chk with the sample built
Debug, so absolute values are inflated; times in ms):

| Step | Hierarchy | GroupBy(Dept) baseline |
| --- | --- | --- |
| `ParentBy` / `GroupBy(Dept)` | 1577.9 | 620.9 |
| `Sort(Name)` | 3644.4 | 2893.1 |
| `ExpandAllRows` / `ExpandAllGroups` | 390.0 | 221.7 |
| **Sum of step medians** | **5612.2** | **3735.7** |

Ratio (sum of step medians) **1.50x**, inside the 2x target; the median of per-run totals gives
5647.0 / 3735.0 = 1.51x. Sort dominates both columns. The hierarchy side is slower by about
957 ms in ParentBy, 751 ms in sort and 168 ms in expand; this probe does not attribute those
differences further.

## 10. Sample

`TableViewSampleApp`: one `ObservableCollection<Employee>` with `Id`, `ManagerId`, `MentorId`,
`Name`, `Title` and `Dept`. Two `TableView`s side by side over the **same** collection, one using
`ParentBy(Id, ManagerId)` and the other `ParentBy(Id, MentorId)`. Commands to add an employee,
reparent one (replace the item), filter by name (showing context ancestors), sort, group roots by
department, and expand or collapse all. The **100k perf** button times the 100k-row case (§9).

## 11. Open issues and future work

- **Nested or lazy source mode.** A mode where each parent owns a child collection, possibly loaded
  on demand, would feed the same adapter through a second index builder. The adapter currently
  consumes the eager, fully built `ParentKeyIndex` (`Roots` / `Children`), so a lazy or nested source
  needs a child-provider seam behind the adapter: an interface the adapter asks for a node's
  children (and whether it has any) when the node is expanded, with the eager index as one
  implementation.
- **Incremental source changes.** The structure and the shaped sibling lists are patched per change
  (§6.1, §6.2), but every change still re-materializes the source, re-ranks rows after the change
  point and re-walks the visible axis. A leaf added or removed below a collapsed parent could skip
  the walk.
- **Grouped toggle cost.** Under `GroupBy`, one node toggle re-slices every group, and each
  `ShapedGroup::SetItems` makes the grouped adapter rebuild the grouped axis. Batch the re-slice:
  update only the slices that changed, then rebuild once.
- **Row-axis descriptor.** `ProjectionKind` is a closed enumeration switched on in
  `ShapedItemsSource`, `TableViewSource` and `RowMetadataProvider`, so a new kind of source (for
  example virtualized or asynchronous) means editing all three. The engine could publish a row-axis
  descriptor instead — source view, row-metadata provider, grouping status and expansion endpoints —
  keeping the enumeration only as diagnostic state.
- **Metadata interface.** `RowMetadataProvider` depends on the concrete `HierarchicalSourceAdapter`.
  Extracting the subset it uses (`TryGetNodeRow`, `TryGetNodeRowForItem`, `TryGetIndexForNodeKey`,
  `IsNodeExpanded`, `SetNodeExpanded`, `ExpandAll`, `CollapseAll`, `ExpandSubtree`) into an interface
  would let another adapter plug in; it pairs naturally with the child-provider seam above.
- **Headless tests.** `ParentStructure` / `ParentKeyIndex` have no dependency on XAML or a
  dispatcher, and the adapter needs only a UI-thread `DispatcherQueue` (§6.3), but both are
  exercised only through the public API and UI (§12).
  Unit tests for index building, filtering and splice/descriptor coherence would be a cheaper
  regression net.
- **App-defined enum keys.** A non-WinRT enum is compared by reference identity (§3.1). Keying it by
  value would need the projection to box it as an `IPropertyValue`.
- **Load on demand** (Kendo/DevExtreme remote "has children"): needs a way to show a chevron with no
  children present yet; depends on the child-provider seam above.
- **Filter modes** other than with-ancestors.
- **Per-row programmatic expansion.** v1 has no app-callable expand or collapse for a single row;
  `TableViewRow.IsExpanded` is read-only and the only app verbs are `ExpandAllRows` /
  `CollapseAllRows`. Users toggle individual rows by chevron, keyboard and UI Automation. Restoring
  saved expansion, expanding to a search hit, or expanding to the selection needs a keyed verb on
  `TableView` (for example `ExpandRow(item)` / `CollapseRow(item)`), which the adapter already
  supports internally (`SetNodeExpanded`, `ExpandSubtree`). Out of scope for v1, additive post-v1.
- **Matching ancestors under a filter.** Auto-expand (§4.2) applies to context rows only. A row that
  matches keeps its own expansion state, so a match below a collapsed matching ancestor is reached
  by expanding that ancestor. Expanding every ancestor of a match would also expand the whole tree
  under a broad filter; a filter mode (above) is the place to offer that.
- **Observing key property changes without live shaping** (without `IsLiveShaping`, an in-place
  reparent waits for a collection change that touches the item, a reset or a re-declaration; §6.2).
- Also not covered: declarative aggregation, nested grouping, cascading selection, drag-reparent,
  and a keyed data source for container preservation.

## 12. Tests

TAEF coverage lives in `TableView_Hierarchy_APITests`, `TableView_LiveShaping_APITests`, the
hierarchy cases in `TableView_AutomationPeer_APITests`, and `TableView_Hierarchy_InteractionTests`
(the test app's Hierarchy pivot). The per-test intent is in the
[test plan](Hierarchical-Rows-and-Live-Shaping-Test-Plan.md). Perf is checked with the sample's
perf button (§9). The list below is the coverage those map to (layers 1 and 3 are exercised
through the public API rather than headless):

1. **`BuildParentStructure` / `BuildParentKeyIndex`**: roots (null, empty string, orphan, `0` with no key 0); value vs.
   object keys; WinRT enum and `Point` keys boxed afresh per call; fresh object keys (identity, never
   a spurious duplicate); app-defined enum keys; `Int32` vs `Int64` mismatch; duplicate key (naming
   the value), null key, self-parent and cycle all throw, and the cycle error names a key on the
   cycle even when a descendant of the cycle comes first; stable sibling order after sort.
2. **Filter**: a deep match pulls in its ancestors; context rows are marked; a match's unmatched
   descendants are excluded; an empty result.
3. **Adapter**: visible order, `Level == Depth + 1`, `ChildCount`, `SiblingIndex` / `SiblingCount`,
   and the key map exact after each splice. Collapse a grandchild, collapse its parent, expand the
   parent: the grandchild stays collapsed. Intent survives re-sort, re-filter and regroup, but not a
   same-key replacement of the item. Intent
   is cleared by redeclaration. `SetIndex` causes exactly one Reset.
4. **Layer 2**: add, remove, replace and move on the flat source with the hierarchy on (including
   the unsorted, unfiltered case, which must not take the flat incremental path); a sort does not
   reset expansion; grouping buckets roots only; `ClearParentBy` releases the rows the hierarchy
   presented.
5. **UI / UIA**: chevron and indent, the frozen lead column keeps the chevron (§7.1), the keyboard
   table in §8, and `LeafNode`, `Level`, `PositionInSet` and `SizeOfSet` values.

## 13. Decisions

| # | Question | Decision |
| --- | --- | --- |
| 1 | Names | `ParentBy` / `ClearParentBy`. |
| 2 | Orphans | Shown as roots. This matches Syncfusion, DevExpress WPF (default) and AG Grid, and no row is silently lost (§3.2). |
| 3 | Filter default | Matches plus their ancestors, the majority default (§4.2). |
| 4 | Auto-expand on filter | On. While a filter is active, context ancestors are expanded as a temporary overlay; the user can collapse or re-expand them, but those toggles are overlay-only. The user's prior state is restored when the filter changes or clears, which avoids DevExtreme's reset (§4.2). |
| 5 | Second `ParentBy` call | Replaces the previous relation (last writer wins) and clears its expansion intent (§5.2). |
| 6 | Row identity | The item object, as in flat and grouped sources. A same-key replacement is a new row and loses expansion and selection (§5.1). |
| 7 | Key edits with live shaping off | Trusted: only a source change touching the item, a reset or a re-declaration re-reads keys (§6.2). |

## 14. Prior art behind decisions 2–4

Sources: each product's public documentation. "Not documented" means the documentation does not
state the behaviour.

**Orphans (a parent key that matches no item).**

| Framework | Root rule | Orphan |
| --- | --- | --- |
| DevExtreme TreeList | `parentId == rootValue` (default `0`); null is treated as `rootValue` | Hidden, with its subtree |
| Kendo TreeList | `parentId` is null, or the type's default value | Not documented |
| Syncfusion EJ2 TreeGrid | Null parent | Root |
| Syncfusion SfTreeGrid | `SelfRelationRootValue`, or a parent that matches no key | Root |
| DevExpress WPF TreeListView | `RootValue`, default null, meaning "matches no key" | Root while `RootValue` is null; hidden once it is set |
| AG Grid (`treeDataParentIdField`) | Null parent | Root, with a console warning |

**Default filter scope.**

| Framework | Setting (default) | Rows shown |
| --- | --- | --- |
| DevExtreme | `filterMode` (`withAncestors`) | Matches + ancestors |
| Kendo | none | Matches + ancestors |
| Syncfusion EJ2 | `filterSettings.hierarchyMode` (`Parent`) | Matches + ancestors |
| Syncfusion SfTreeGrid | `FilterLevel` (`Root` / `All` / `Extended`) | `Extended` = matches + ancestors |
| DevExpress WPF | `FilteringMode` (`Nodes`) | Matches only, re-parented to the nearest matching ancestor |
| AG Grid | `excludeChildrenWhenTreeDataFiltering` (`false`) | Matches + descendants + ancestors |

**Auto-expand when filtering.**

| Framework | Auto-expands ancestors of matches | Prior expansion after the filter clears |
| --- | --- | --- |
| DevExtreme | Yes: `expandNodesOnFiltering`, default `true` | Lost (resets to collapsed) |
| Telerik WPF RadTreeListView | Yes: `AutoExpandItemsAfterFilter`, default `true` | Not documented |
| DevExpress WPF | Opt-in: `ExpandNodesOnFiltering` | Not documented |
| Kendo, Syncfusion EJ2, AG Grid | Not documented | Not documented |
