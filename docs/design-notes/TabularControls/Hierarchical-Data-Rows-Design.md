# TableView — Hierarchical (Tree-Grid) Data Rows — Design

Status: **design proposal, not implemented.** No code for this feature exists yet.

## The feature

A **hierarchical data row** is a data row that is itself the root of a sub-tree of data rows. The
canonical example is Task Manager's **Apps** section: a `VSCode` row shows total CPU and memory in
its own cells, carries a chevron, and expands to reveal each child process as its own row —
indented, with the same columns, and itself expandable.

The defining property, and the whole reason this is not grouping: **every visible row is a data
row.** A parent is not a band of chrome above its children; it is a real item with real cells that
happens to own more items. "Parent-ness" is therefore row *metadata* — depth, has-children,
is-expanded — not a separate row type. Depth is arbitrary.

Grouping is the other flattening axis and stays distinct. It is a fixed two-level
`header → leaf items` projection whose parent is a *synthesized* bucket rendered as
`TableViewGroupHeader` chrome, and whose structure is *derived* by bucketizing a flat list through
a key selector. Hierarchy is *intrinsic*: the parent object already owns its children, and the
control's job is to walk and present that, not to compute it.

What an app gets from the feature:

- Arbitrary-depth expand/collapse whose state survives sorting, filtering and regrouping.
- Sorting and filtering applied within each sibling set, so a level is ordered among its peers and
  never against its own parent.
- Lazy subtrees: rows start collapsed, and a collapsed node's children are never enumerated, so a
  huge tree costs only what is on screen. Supplying `hasChildrenSelector` is what makes this strictly
  true — without it, proving a chevron is needed enumerates one level down (§4.3).
- Rollup values on the parent row, bound from the parent item's own properties.

## Public API

Three additions, and nothing else:

| API | Purpose |
| --- | --- |
| `TableViewSource.WithChildren(childrenSelector [, hasChildrenSelector])` | Declares intrinsic hierarchy: how to reach an item's children, and optionally how to answer "is it expandable" without enumerating them. `ClearHierarchy()` removes it. |
| `TableView.ExpandAllRows()` / `CollapseAllRows()` | Bulk expand/collapse of the row hierarchy, as an intent rather than a loop over live nodes. |
| `TableView.RowIndentSize` | Per-level indent applied to the primary cell. Defaults from a theme resource. |

Per-node expand/collapse is reached the way group expansion already is — the chevron, the keyboard,
and `IExpandCollapseProvider` — not through a public per-item method. §3 has the exact IDL.

---

## 1. Where the current stack actually stands

Grounded in the shipping code.

| Fact today | Evidence |
| --- | --- |
| Row axis is **materialized, not computed**. Rebuild walks groups and pushes every visible row into one `IObservableVector`, then one `ReplaceAll` → one Reset. | `GroupedSourceAdapter::Rebuild` (`GroupedSourceAdapter.cpp:79`), `m_entries` + `m_entriesView` (`GroupedSourceAdapter.h`) |
| Data rows are **raw app items** — no wrapper, no per-row allocation. Only headers allocate. | `Rebuild`: *"Data rows ARE the app items — no wrapper"* (`GroupedSourceAdapter.cpp:170-178`) |
| Single-node expand/collapse already splices **in place** instead of Reset. | `TryApplyExpansionSplice` (`GroupedSourceAdapter.cpp:301`) |
| Expansion intent is already **feature-neutral and depth-agnostic**, keyed by an arbitrary string. Its own header says hierarchy keys it by *node* identity. | `RowExpansionModel.h` (layer 1) |
| Row metadata already has the **hierarchy fields**, currently hardcoded. `Level` is `1` for grouped data rows, `ChildCount` is documented as *"becomes the child count when hierarchical (tree) rows land"*. | `TableViewRowInfo.h:21-28`, `RowMetadataProvider::GetRowInfo` (`RowMetadataProvider.cpp:199`) |
| Container-type selection is **item-based**, not index-based (`ElementFactoryGetArgs` carries no index). | `TableView::GetRowKindForItem` (`TableView_Grouping.cpp:53`), `TableViewRowTemplateSelector::SelectTemplateCore` |
| Chevron chrome, visual states, `ToggleRequested`, and `IExpandCollapseProvider` exist — but only on `TableViewGroupHeader`. | `TableView.idl:333-379`, `TableViewGroupHeaderAutomationPeer.cpp` |

### What is genuinely missing

1. **No children input.** The source contract has `Filter` / `GroupBy` / `Sort` only
   (`TableViewSource.idl`). Grouping *derives* buckets via `BucketizeToGroups`; hierarchy is
   *intrinsic* — the parent object owns its children.
2. **No depth producer.** `GetRowInfo` synthesizes `Level` from the grouped/flat switch; nothing
   computes real depth.
3. **Row identity is the item's object address** (`RowIdentity::MakeObjectIdentitySelector`) and is
   validated **unique across the projection** (`ValidateRowIdentities`, which *throws* on a repeat).
   A flat object-identity key cannot distinguish one row of a node from another, so expansion intent
   needs a per-position key. It also means a shared child (one object under two parents) stays
   unsupported in v1 — see §4.2.
4. **The expansion baseline is wrong for trees.** `RowExpansionModel::m_defaultExpanded` is `true`
   and intent is stored only as exceptions, so an untouched key reads as *expanded*. Correct for
   grouping; for hierarchy it would expand the entire tree on first rebuild (§4.3).
5. **Pruning is whole-set.** `RetainOnly(liveKeys)` is safe for grouping only because every group,
   collapsed or not, emits a header and therefore a live key. A lazy tree walk cannot produce a
   complete live-key set (§4.2).
6. **`TableViewRow` has no indent and no chevron.** Indentation is a control concern and nothing in
   the row visual reserves space for it.

**Conclusion:** the architecture is already the right shape. This feature is a **sibling adapter
plus a depth-aware metadata path**, not a re-architecture. Nothing in `ShapedItemsSource`'s
identity/subscription machinery has to be replaced; layer 1 needs one additive method
(`RetainOnlyUnder`) and one non-default configuration (a collapsed baseline).

---

## 2. Design principles carried over from the current stack

These are not new; they are the invariants the grouped path already holds, and the hierarchical
path must hold them too or the two will drift.

- **Materialize the visible rows; never compute the axis.** One `IObservableVector` whose object
  identity is stable for the adapter's life, wrapped once in an `ItemsSourceView`.
- **Data rows are raw app items.** No per-row COM wrapper, ever. Per-row *metadata* may be a POD
  struct in a parallel vector, which costs a few words per **visible** row.
- **Expansion is intent about a key, not state on an object.** Shaping re-mints structure; intent
  must outlive it (`RowExpansionModel`).
- **One Reset by default, splice for the single-node case.** Bulk operations stay a Reset.
- **Adapters know nothing about a control.** `GroupedSourceAdapter` lives outside `TableView` and
  so must its hierarchical sibling.
- **Fail fast on bad identity.** Unresolvable/colliding identity throws with a caller-facing
  diagnostic rather than silently degrading (see `RebuildGrouped`).

---

## 3. Public surface — exact IDL

### 3.1 Source contract — a children selector

```idl
[MUX_PREVIEW] [webhosthidden]
delegate Object TableViewChildrenSelector(Object item);   // returns a collection, or null

runtimeclass TableViewSource
{
    // Declares intrinsic hierarchy. childrenSelector: required, non-null (E_INVALIDARG when null;
    // use ClearHierarchy() to remove). Returning null / an empty collection means "leaf".
    [default_overload] [method_name("WithChildren")]
    TableViewSource WithChildren(TableViewChildrenSelector childrenSelector);

    // hasChildrenSelector is OPTIONAL and exists for LAZY trees: it answers "is this expandable"
    // without enumerating children, so a collapsed node never materializes its subtree.
    [method_name("WithChildrenAndHasChildren")]
    TableViewSource WithChildren(TableViewChildrenSelector childrenSelector, TableViewPredicate hasChildrenSelector);

    TableViewSource ClearHierarchy();
}
```

Overload metadata is frozen at declaration time for the same MIDL reason `GroupBy` documents.

**`WithChildren` + `GroupBy` compose.** `GroupBy` applies to the **roots** only. The row stream
becomes `[Header A] [rootA1] [rootA1's visible subtree…] [rootA2] … [Header B] [rootB1] …`. A
descendant is never re-bucketed: its depth is what makes it a descendant, and it cannot
simultaneously sit at depth 3 under its parent and at depth 0 under a header. Headers therefore
exist at the top level and nowhere else, and every non-header row remains a real data row at its
real depth — the invariant that separates this feature from grouping.

**Mechanism.** The roots are ordered bucket-by-bucket and handed to the hierarchy adapter, which
emits each root immediately followed by its visible subtree. Because a bucket's roots are
contiguous and a root's subtree follows it, the adapter's entries can be sliced per bucket in one
pass, cutting at every `Depth == 0` row. Each group's `Items` is its slice — roots *plus* every
currently-visible descendant, so a header's item count grows as subtrees open.

Two consequences worth stating plainly:

- **The root set must not be re-shaped by the per-level shaping callback** under grouping. Letting
  it sort the roots among themselves would interleave buckets and destroy the contiguity the slice
  depends on. Layer 2 owns root ordering; the adapter never re-shapes the root set.
- **A node toggle costs a grouped `Reset`, not an incremental splice.** Under grouping the
  presented axis is the grouped adapter's, so a hierarchy splice is invisible to it until the
  slices are recomputed and re-pushed. Accepted for v1; revisit if it measures badly.

**Index spaces diverge.** A data row's index on the grouped axis does not address the hierarchy
adapter, because headers interleave. The row's *item* is the only shared handle, which is sound
because one object occupies at most one visible row (layer 2 rejects duplicate objects, and the
walk rejects duplicate siblings).

**Default expansion state is collapsed.** `TableViewSource` sets the hierarchical adapter's
expansion baseline to collapsed at construction (§4.3). This is a deliberate divergence from
grouping, whose baseline is expanded, and it is load-bearing rather than cosmetic — see §4.3.

### 3.2 Control surface

```idl
// TableView
void ExpandAllRows();
void CollapseAllRows();
Double RowIndentSize { get; set; };   // per-level indent; default from theme resource
```

`ExpandAllGroups` / `CollapseAllGroups` stay group-only; the row verbs are separate because a
unified "expand everything" would mean two different things under the two axes.

---

## 4. Layer 3 — `HierarchicalSourceAdapter`

A new sibling of `GroupedSourceAdapter`, in `controls/dev/HierarchicalSourceAdapter/`. Same
lifetime, threading, and re-entrancy contract, copied deliberately: UI-thread-affine, weak
callbacks, `m_rebuildInFlight` / `m_pendingRebuild` coalescing, teardown on the owning thread
because every strong owner is a `ReferenceTracker`.

```cpp
class HierarchicalSourceAdapter : public std::enable_shared_from_this<HierarchicalSourceAdapter>
{
public:
    // The constructor sets the expansion baseline to COLLAPSED (§4.3). RowExpansionModel defaults
    // to expanded, which is right for grouping and fatal here.
    HierarchicalSourceAdapter();

    winrt::ItemsSourceView Entries() const;              // one view over m_entries, created once

    void Source(winrt::IInspectable const& roots);       // shaped ROOT rows, in order
    void ChildrenSelector(ChildrenFn fn);
    void HasChildrenSelector(PredicateFn fn);            // optional

    // Per-node intent, addressed by PATH key (see 4.2).
    bool  IsNodeExpanded(winrt::hstring const& pathKey) const;
    void  SetNodeExpanded(winrt::hstring const& pathKey, bool isExpanded);
    void  ExpandAll();                                   // moves the baseline; never splices
    void  CollapseAll();

    // Guards on the walk, because ChildrenSelector is app code over an arbitrary object graph.
    static constexpr int32_t c_maxHierarchyDepth = 256;

    // Index -> descriptor, for the row-metadata provider. O(1).
    struct NodeRow
    {
        winrt::IInspectable Item{ nullptr };
        winrt::hstring PathKey;
        int32_t Depth{ 0 };          // 0-based; TableViewRowInfo::Level is Depth + 1 (§6)
        int32_t ChildCount{ 0 };     // resolved children count; -1 == unrealized/unknown
        bool HasChildren{ false };
        bool IsExpanded{ false };
    };
    NodeRow const* TryGetNodeRow(int32_t index) const;
    bool TryGetIndexForPathKey(winrt::hstring const& pathKey, int32_t& index) const;
};
```

### 4.1 The two parallel vectors and one index

- `m_entries` — `IObservableVector<IInspectable>` of the **raw app items**, in visible order. This
  is what the repeater sees. Identical in kind to the grouped adapter's `m_entries`, minus headers.
- `m_descriptors` — `std::vector<NodeRow>`, index-aligned 1:1 with `m_entries`.
- `m_indexByPathKey` — `std::unordered_map<hstring, int32_t>`, backing `TryGetIndexForPathKey`.

Depth cannot ride on the row value (that would reintroduce the per-row wrapper the stack just got
rid of) and cannot be recomputed from the item (the same object may appear at two depths). A
POD side-vector is the only option that keeps both invariants. Cost is `O(visible rows)` — the same
cost TreeView's `ViewModel` already accepts — with zero COM allocations.

Every mutation of `m_entries` mutates `m_descriptors` in the same statement block. They are only
correct as a pair, the same way `RowIdentity`'s identity set and index map are.

**Index-map cost, stated rather than hidden.** `TryGetIndexForPathKey` is an O(1) *lookup*, but
keeping the map exact is not free: a subtree splice at index `i` shifts every index after the run,
which is an O(visible rows) sweep of integer compares — the identical cost, for the identical
reason, that `RowIdentity.h` already documents for `ShiftTrackedFlatRowIndicesForInsert` /
`ForRemove` ("a hash map keyed by identity has no way to reach 'every entry at or after this
position' without visiting all of them"). v1 accepts it, because the alternative is an O(n) ABI
scan per lookup and lookups are the frequent operation. Two mitigations carry over verbatim: a
splice at the *tail* skips the sweep entirely, and the indices must be exact rather than
approximate, so any detected desync falls back to `Rebuild` instead of being repaired lazily.

**Path-key cost.** Each visible row holds one `hstring` of `O(depth)` characters, re-minted on
every `Rebuild`; hashing and comparison are likewise `O(depth)`. So `Rebuild` is
`O(visible rows × depth)` in string work, not `O(visible rows)`. At the §10 perf-gate shape
(100k visible rows, depth ~10) that is the dominant allocation cost of the feature and is what the
harness must measure. If it does not hold, the fix is a interned path-segment id rather than a
string — deferred until the benchmark says so.

### 4.1.1 Bounding the walk

Two guards, because `ChildrenSelector` is app code returning an arbitrary object graph and neither
`Emit`'s recursion nor the tree's finiteness is something the control can assume:

- **Cycle detection.** `Emit` carries the set of object identities on the current root-to-node path.
  An item whose identity is already on its own ancestor path is a cycle; the adapter throws
  `E_INVALIDARG` with a caller-facing diagnostic naming the repeated item, per §2's *fail fast on
  bad identity*. It does not silently truncate, because a silently truncated tree is a data bug the
  app never learns about. Note this is a *path*-local check: the same object under two *different*
  parents is not a cycle and stays legal.
- **Depth cap.** `Emit` is recursive, so an adversarially deep (but acyclic) chain would overflow
  the stack before any allocator complained. A hard cap (`c_maxHierarchyDepth`, 256) throws the same
  way. Both guards exist for the same reason: `ExpandAll()` moves the baseline to expanded for keys
  that do not exist yet, so a single public call can drive the walk into the whole graph.
- **Sibling uniqueness.** A path key must identify exactly one row, so the same object appearing
  twice in one child set throws `E_INVALIDARG` too — the same duplicate-identity rejection
  `ShapedItemsSource` already performs, applied at the level that can name the offending parent.
  Allowing it would map one key to one of two rows in the index, silently addressing the wrong row
  on every metadata lookup and expansion toggle.

**What a throw leaves behind.** All three guards fire *during* the walk, so the walk is written to
touch no published state until it has completed: rows, descriptors, the index map and the node
subscriptions are all built into locals and adopted in one step at the end. A throw discards the
locals, revokes the subscriptions accumulated so far, and leaves the previously published projection
exactly as it was — intact and self-consistent, rather than half-replaced. The splice path applies
the same rule and additionally *declines* instead of propagating, so the caller's authoritative
`Rebuild` re-raises the diagnostic from a single place: one recovery policy rather than two that
have to agree.

One consequence worth stating, because it is otherwise a trap: a failed expand leaves intent saying
*expanded* while the projection shows *collapsed*. `SetNodeExpanded` therefore does not early-return
purely on intent equality — if the node is materialized and its descriptor disagrees, it reconciles.
Without that, the user's first failed expand would make the node permanently unexpandable.

### 4.2 Path identity

```
pathKey = "node:" + id(root) + "/" + id(child) + "/" + ... + "/" + id(node)
```

where `id(x)` is the existing object identity from
`RowIdentity::MakeObjectIdentitySelector` (its canonical `IUnknown` pointer), the same primitive
flat and grouped rows already use.

Why the path and not the node:

- The same object may legitimately appear under two parents. Object identity alone would make the
  two rows indistinguishable and would trip `ValidateRowIdentities`' uniqueness check.
- Expansion intent must survive re-sort / re-filter at **every** level, which is exactly the
  regression the identity-string key fixed for groups — lifted to N levels.

**What "the same object under two parents" actually costs, and what v1 does about it.** The path key
makes the two rows distinguishable *inside the adapter*, and that is all it does. Two shipping
behaviours sit above it and are not fixed by it:

1. `ShapedItemsSource` throws on a projection whose rows are not object-unique — literally
   *"The same item object appears in the source more than once… Use a distinct object per row."*
   (`ShapedItemsSource.cpp`, the `c_duplicateObjectReason` branch). §5.4 keeps `Rows()` as the flat
   visible projection, so a shared child would reach exactly that throw.
2. `SelectedItems` is a collection of items. Two rows backed by one object cannot be told apart in
   it, whatever the row axis knows.

So v1's contract is: **the adapter is built on path identity and does not itself assume uniqueness,
but `RebuildHierarchical` runs the same object-uniqueness validation as every other shaped
projection, and a shared child throws with that existing diagnostic.** Path identity is therefore
justified by the second bullet — surviving reshape at N levels — not by DAG support. Real DAG
support means an identity contract that is not the object address and a `SelectedItems` that is
row-addressed, which is §9 work; the path key is the half of it that can be paid for now, and
building the adapter on it means the later half does not re-architect the adapter.

Intent is pruned at the end of every `Rebuild`, but **not** against the visible-key set — that
would be a bug. The walk deliberately does not descend into collapsed nodes (§4.3), so a collapsed
node's descendants produce no keys that pass, and pruning against visible keys would delete the
user's intent for every subtree hidden behind a collapse. The observable symptom would be: collapse
a grandchild, collapse its parent, re-expand the parent — and the grandchild returns expanded,
because its intent was pruned while it was invisible.

Grouping does not have this hazard, and that is why the rule cannot be copied verbatim:
`GroupedSourceAdapter::Rebuild` emits a header for *every* group including collapsed ones, so its
live-key set is genuinely complete.

v1's rule: **prune a path key only when a strict ancestor of it was walked and did not produce it.**
Concretely, `Rebuild` collects the set of path prefixes it actually enumerated (the expanded nodes,
whose child sets it saw in full) alongside the live keys; a stored key is dropped only if its parent
prefix is in that enumerated set and the key itself is not live. Keys under an unenumerated prefix
are untouched. This still bounds the store — a subtree that genuinely vanishes is pruned the next
time its parent is expanded and walked — while making collapse non-destructive.

Inherited limitation, documented not fixed: object identity is not stable across an app
**re-creating** its item objects, so a rebuilt tree loses expansion and selection anchoring. That is
the same contract `TableViewSource` already states for rows.

### 4.3 Rebuild — visible-only walk

```
Rebuild():
  assert UI thread; guard re-entrancy
  built, descriptors, indexByPathKey, liveKeys, enumeratedPrefixes, subscriptions := {}  // all LOCAL
  enumeratedPrefixes.insert("")                      // the root sibling set IS walked in full
  for each root r (in shaped order):  Emit(r, depth: 0, parentPath: "", ancestorIds: {})
  // --- nothing above has touched published state; a throw unwinds to the old projection ---
  UnsubscribeFromAllNodes(); adopt subscriptions     // old handlers die only once the walk succeeds
  m_expansion.RetainOnlyUnder(liveKeys, enumeratedPrefixes, ParentPathOf)   // §4.2
  publish descriptors + indexByPathKey
  m_entries.ReplaceAll(built)          // one Reset

Emit(item, depth, parentPath, ancestorIds):
  if depth >= c_maxHierarchyDepth:      throw E_INVALIDARG (depth cap, §4.1.1)
  if id(item) in ancestorIds:           throw E_INVALIDARG (cycle, §4.1.1)
  path = parentPath + "/" + id(item)
  hasChildren = HasChildrenSelector ? HasChildrenSelector(item)
                                    : (ChildrenSelector(item) yields >= 1 item)
  isExpanded  = hasChildren && m_expansion.IsExpanded(path)   // baseline is COLLAPSED, see below
  push row {item, path, depth, hasChildren, isExpanded, childCount}
  if not liveKeys.insert(path):         throw E_INVALIDARG (duplicate sibling, §4.1.1)
  if isExpanded:
      childrenCollection = ChildrenSelector(item)   // ONE call: the walk and the
      children = ShapeSiblings(enumerate(childrenCollection))   // §5 -- subscription need the same object
      SubscribeToNode(path, childrenCollection)
      enumeratedPrefixes.insert(path)       // this child set was seen in FULL — safe to prune under
      for each c in children: Emit(c, depth + 1, path, ancestorIds + id(item))
```

**The expansion baseline must be flipped to collapsed, and it is not the default.**
`RowExpansionModel` stores intent only where it *differs* from `DefaultExpanded()`, and that default
is `true` — correct for grouping, where an unexpanded-by-default grid of headers would hide all
data. Taken as-is for hierarchy it is fatal: `IsExpanded(path)` answers `true` for every path nobody
has touched, so `Emit` descends into everything and the visible-only walk degenerates to a full
`O(tree)` realization on the very first `Rebuild` — exactly the cost this section claims to avoid.
`HierarchicalSourceAdapter`'s constructor therefore calls `m_expansion.SetDefaultExpanded(false)`,
and *that*, not the walk shape alone, is what makes the next paragraph true. The baseline is
adapter-owned rather than app-settable in v1; `ExpandAllRows()` moves it, which is precisely
`SetAllExpanded(true)`.

**Given a collapsed baseline, the walk only descends into expanded nodes.** A collapsed node's
children are never enumerated. So a 1M-node tree with the roots collapsed costs `O(roots)`, not
`O(tree)` — this is what makes a materialized axis viable for hierarchy at all, and it is why
`HasChildren` is a separate optional selector rather than `children.Count > 0`. Note the residual
cost when no `HasChildrenSelector` is supplied: proving `hasChildren` enumerates one level below
every visible row, so a collapsed 1M-node tree costs `O(roots + children-of-roots)`. That is the
concrete reason the optional selector exists, and the sample's perf case must cover both shapes.

**Unrealized children.** With `HasChildrenSelector` supplied and children not yet loaded,
`ChildCount` is `-1` and the row still presents a chevron. Expanding calls `ChildrenSelector`,
which the app may populate asynchronously; the resulting collection-changed notification rebuilds
that subtree. This is TreeView's `HasUnrealizedChildren` modelled as a selector instead of a node
flag.

### 4.4 Subtree splice on single-node toggle

The direct analogue of `TryApplyExpansionSplice`, and the reason expand/collapse does not destroy
scroll position, selection and focus:

- **Collapse**: the node's visible descendants are a **contiguous run** starting at `index + 1` —
  walk forward while `descriptor.Depth > node.Depth` to find the run length, then `RemoveAt` that
  range from both vectors and flip the node's `IsExpanded`.
- **Expand**: `Emit` the node's subtree into a temp vector (recursively honouring nested intent,
  and carrying the same cycle/depth guards as §4.1.1), then `InsertAt` the run.

Both cases also fix up `m_indexByPathKey`: the spliced run's own keys are inserted or erased, and
every index after the run shifts by the run length. That sweep is `O(visible rows)` (§4.1), so a
splice is not `O(run)` overall — it is cheaper than a `Rebuild` in *container* work, which is the
cost that actually matters here, not in bookkeeping. A splice at the tail skips the sweep.

Returns `false` — and the caller falls back to authoritative `Rebuild` — when the path key is not
resolvable, the descriptor/entry counts disagree, or a rebuild is already in flight. Bulk
`ExpandAll` / `CollapseAll` and baseline moves never splice; they are one Reset, matching the
grouped adapter.

**Inherited cost, stated rather than implied:** `m_entries` is a plain `IObservableVector`, so the
repeater reaches it through `InspectingDataSource`, which reports **no key mapping** — the
container-preservation cost `GroupedSourceAdapter` already documents. Splicing therefore preserves
scroll offset, selection and focus *state*, but containers below the splice point are still
re-prepared. Toggling a node near the top of a long grid costs a re-prepare of everything under it.
Fixing that means a keyed data source, which is a layer-below change and out of scope here.

### 4.5 Subscriptions

One subscription per **expanded, visible** node's children collection, held in a vector of tokens
and dropped wholesale at the top of `Rebuild` — the same `SubscribeToGroup` /
`UnsubscribeFromAllGroups` shape. Subscription count is therefore bounded by visible rows, not by
tree size — a recursive per-node subscription over the whole tree would leak and scale with tree
size instead.

v1 responds to any child-collection change with a full `Rebuild`. That is what the grouped adapter
does for inner-group changes today, and it keeps the first version honest; a ranged
subtree-local update is a later optimization with a real benchmark behind it.

---

## 5. Layer 2 — shaping a hierarchy

`ShapedItemsSource` gains a third projection kind and a third rebuild path, parallel to the
existing two:

```cpp
enum class ProjectionKind { None, Unshaped, Flat, Grouped, Hierarchical };
void RebuildHierarchical(std::vector<winrt::IInspectable>& rows);
```

`RebuildHierarchical` is deliberately thin, because per-level shaping cannot be done once up front:

1. `InvalidateShapingState()` and `ClearFlatRowIdentityTracking()` — the flat incremental fast path
   does not apply, same as `RebuildGrouped`.
2. Shape the **root** sibling set (`ApplyFilter` + `ApplySort`) and hand it to the adapter as
   `Source`.
3. Hand the adapter a `ShapeSiblings` callback that applies the same filter and sort to **each
   child sibling set as it is walked**. Per-sibling-set shaping is the same principle as the
   per-bucket `ApplySort(bucket.Items, GroupOrder, -1)` in `RebuildGrouped`, and matches WPF
   shaping each level's `CollectionView` independently.

   Because this callback runs *inside* the adapter's walk — a hazard the grouped path does not have,
   since a group's members arrive already shaped — it must not call back into the adapter. The
   adapter does not merely document that: it publishes "a shaping callback is on the stack" and
   every mutator asserts on it in chk, so a violation is found at authoring time rather than
   surviving as an invisible redundant rebuild.
4. Maintain `Rows()` as the flat shaped projection of **visible** rows, for the same coherence
   reason `RebuildGrouped` writes `flatRows`, and run the same `ValidateRowIdentities` check over it
   — a hierarchical projection is object-unique like every other one, and a shared child throws the
   existing duplicate-object diagnostic (§4.2).

**Filter semantics (v1): ancestor retention.** A node is kept if it matches the predicate **or any
descendant matches**, so a match is never orphaned and never silently invisible. The cost is that
filtering forces a **full tree walk** (descendants of collapsed nodes must be tested), which
defeats §4.3's visible-only walk. v1 therefore states the cost plainly: *filtering a hierarchical
source is `O(tree)`.* The alternative — match-node-only — is cheaper but hides matches under
collapsed parents and was rejected.

**Filter × lazy hierarchy: the direct contradiction, and v1's answer.** Ancestor retention has to
test descendants; `HasChildrenSelector` exists precisely so the control will *not* enumerate
descendants; and §4.3 allows those children to arrive **asynchronously**, so "walk the whole tree"
is not merely expensive but not answerable synchronously at all. The two features cannot both hold.
v1 resolves it rather than leaving it to discovery:

- **`Filter` + `WithChildren`** → supported, and the filter is **match-node-only**: a node that
  fails the predicate is dropped even when a descendant would have matched, because that descendant
  is reachable only through the parent just removed. This holds with or without a
  `hasChildrenSelector`, and it is the one shape that stays `O(visible)` and never realizes a
  subtree the app asked not to realize.
- **Ancestor retention** — keeping a non-matching parent alive because something beneath it matches
  — is **deferred**, not silently approximated. It is inherently `O(tree)`: the only way to know
  whether a collapsed subtree contains a match is to walk it, which defeats the lazy contract the
  `hasChildrenSelector` overload exists to uphold. §9 carries it, waiting on an async-aware,
  incrementally-realizing filter.

**Sort semantics:** sorts apply within each sibling set; sibling order never mixes depths. A sort
comparing a parent against its own child is meaningless and is not expressible.

---

## 6. Layer 4 — row metadata

`RowMetadataProvider` gains a third `SourceKind` and one factory:

```cpp
enum class SourceKind { Flat, Grouped, Hierarchical };

static TableViewRowMetadataProvider CreateForHierarchicalRows(
    HierarchicalSourceAdapterPtr const& adapter,
    ItemKeySelector const& itemKeySelector = {});
```

- `GetRowInfo(index)` → `{ Kind: Data, Level: d.Depth + 1, IsExpandable: d.HasChildren,
  IsExpanded: d.IsExpanded, ChildCount: d.ChildCount }`. **`Kind` stays `Data` for every tree row**
  — parent-ness is metadata, not a row type. This is what keeps `GetRowKindForItem`,
  `TableViewRowTemplateSelector` and the whole two-container-types path untouched: a hierarchical
  source produces exactly one container type.
- `GetIdentity(index)` → the descriptor's **path key**, not the item key. The existing
  `m_identityToIndex` map then works unchanged and stays unique across reshapes at every level.
- `TryGetIndexForIdentity` → `adapter->TryGetIndexForPathKey`: an O(1) lookup, no scan, against the
  index the adapter maintains and pays an O(visible rows) sweep to keep exact (§4.1).
- `Expand` / `Collapse` / `Toggle(key)` → `adapter->SetNodeExpanded(key, ...)`. The `"node:"` prefix
  distinguishes a node key from the existing `"group:"` / `"identity:"` keys, mirroring
  `IsGroupExpansionKey`.
- `ExpandAllGroups` / `CollapseAllGroups` stay no-ops under `Hierarchical`; the row-level bulk verbs
  are separate entry points.

**`Level` is 1-based on the wire, 0-based in the descriptor, and the conversion happens here — once.**
`NodeRow::Depth` is 0 for roots because indent is `Depth * RowIndentSize` and roots must not be
indented. `TableViewRowInfo::Level` is already 1-based in shipping code (`RowMetadataProvider`
assigns `Level = 1` to grouped data rows), and UIA's `AutomationProperties.Level` is 1-based by
definition. Emitting raw `Depth` would therefore give roots `Level = 0`, silently contradicting the
grouped path and handing screen readers an invalid level. So: descriptors stay 0-based, `GetRowInfo`
emits `Depth + 1`, §7.2's indent uses `Level - 1` (equivalently the descriptor's `Depth`), and §8's
automation passes `Level` straight through.

`TableViewRowInfo` needs **no new fields** — `Level`, `IsExpandable`, `IsExpanded`, `ChildCount`
were reserved for exactly this and stop being hardcoded.

`RowExpansionModel` needs **one** addition: `RetainOnlyUnder(liveKeys, enumeratedPrefixes,
parentPrefixOf)`, the prefix-scoped prune §4.2 requires. The prefix extractor is a caller-supplied
callback, so layer 1 keeps knowing nothing about paths, groups or rows. Existing `RetainOnly` stays
as-is for the grouped caller, whose live-key set really is complete.

The prune walks **all the way up** the ancestor chain, not just to the immediate parent, and drops a
key as soon as some ancestor-or-self has an enumerated parent prefix while not itself being live.
Stopping at the immediate parent leaks without bound: if `A` was enumerated and its child `B` has
since vanished, nothing will ever enumerate `A/B` again, so intent stored for `A/B/C` would survive
every future prune. Walking up reaches `A` — enumerated, with `B` absent — which proves the whole
branch died. The walk is bounded by a constant so a malformed key format that never reduces to a
fixed point cannot spin.

---

## 7. Rendering

### 7.1 `TableViewRow`

Three new internal-set DPs, mirroring `TableViewGroupHeader`'s: `Level`, `IsExpandable`,
`IsExpanded`, plus visual state groups `ExpansionStates (Expanded|Collapsed)` and
`ExpandabilityStates (Expandable|NotExpandable)` so the glyph is driven by state, not code —
the pattern `TableViewGroupHeader.cpp:142` already establishes.

They are pushed from `PrepareRowElement` using `GetRowInfo(index)`, the same place and the same way
`PrepareGroupHeaderElement` pushes header state. This is sound precisely because *template
selection* is item-based (needs no depth) while *element preparation* is index-based (has depth).

### 7.2 Indent + chevron live in the primary cell

The expander gutter and the indent are applied **inside the first visible column's cell content**,
by `TableViewCellsPanel`, not by padding the row:

- Column widths, frozen-column layout (`RefreshFrozenColumnLayout`) and grid lines stay
  authoritative; indent consumes *content* width within the cell, never column width.
- The gutter is reserved (`PART_ExpanderGutter`-style) even for leaves at the same depth so sibling
  text stays aligned — the alignment reason `TableView.idl:350` already gives for headers.
- Effective indent = `(Level - 1) * RowIndentSize` — i.e. the descriptor's 0-based `Depth`, so a
  root row is flush (§6) — defaulted from a `TabularSurfaces_themeresources.xaml`
  resource and RTL-mirrored via the existing cells-panel flow-direction handling.

Editing interaction: the chevron hit-test wins over cell edit-on-click within the gutter only;
outside it, existing `OnPointerPressedForEditing` behaviour is unchanged.

### 7.3 Toggle plumbing

`TableViewRow` raises `ToggleRequested` → `TableView::RequestRowExpansion(container, desired)` →
`m_tableViewSourceRowMetadata->Toggle(identity)` — the exact path
`RequestGroupExpansion` already takes, with `GetIdentity(index)` supplying the key.

---

## 8. Cross-cutting behaviour

These are the parts that actually decide whether the feature ships.

**Selection.** Selection is anchored by row identity, which is now the path key, so a node keeps
selection across re-sort/re-filter. Collapsing a selected node's subtree **retains** selection
intent for the hidden descendants (they return selected on expand) but they are not in the row axis
while hidden, so `SelectedItems` reports only visible selected rows. Rationale: dropping intent
makes collapse destructive; reporting hidden rows as selected makes `SelectedItems` disagree with
the visible grid. Selecting a parent does **not** select its descendants in v1 — no tri-state
cascade.

Selection *state* stays index-based internally (`TableView_Selection`'s existing "by INDEX, not item
identity" rule is unchanged); the path key is the anchor used to re-find a row after a reshape, not
a replacement storage key. Because §4.2 keeps the projection object-unique, `SelectedItems` has no
new ambiguity to resolve.

**Keyboard.** On a tree row: `Right` expands (then moves to first child), `Left` collapses (then
moves to parent), `*` expands the current subtree — TreeView's contract. These bind only when the
row is expandable and the current cell is the primary cell, so they do not steal caret navigation
inside an editable cell.

**Automation.** `TableViewRowAutomationPeer` implements `IExpandCollapseProvider` (leaf rows report
`LeafNode`, exactly as `TableViewGroupHeaderAutomationPeer.cpp:121` does), sets
`AutomationProperties.Level` from `RowInfo.Level`, and raises `ExpandCollapseState` property-changed
on toggle. `SizeOfSet` / `PositionInSet` are reported **within the sibling set**, not within the
flat row axis, or a screen reader announces "row 4,217 of 900,000" for the second child of a node.
Grid coordinates (`TableViewCellAutomationPeer`) continue to use flat row indices — UIA Grid is a
flat coordinate space by definition.

**Virtualization.** Unchanged: the repeater virtualizes containers over a materialized vector of
visible rows. The vector is `O(visible rows)` where "visible" means *not hidden behind a collapsed
ancestor* — fully expanding a huge tree materializes the whole thing, which is the same exposure
`ExpandAllGroups` has today. `ExpandAllRows()` on a large or lazy tree is therefore the single most
expensive call in the API: it moves the baseline (§4.3), so the next `Rebuild` realizes every node
the children selector can reach, bounded only by the §4.1.1 guards. Documented as such on the
method, not silently mitigated.

---

## 9. Deferred, with the reason

- **Declarative column aggregation.** v1 is app-provided rollups: the parent object exposes its own
  totals and TableView binds the parent's cells — which is exactly how Task Manager works, and
  needs zero engine work. Declarative `Aggregate(column, items => …)` over descendants is the only
  genuinely new compute in the feature and it needs incremental-recompute design of its own.
- **`GroupBy` + `WithChildren` at every level.** v1 composes the two axes by grouping the **roots**
  only (§4). What remains deferred is *nested* grouping — synthetic headers bucketing siblings at
  depth — which would re-parent rows under headers and break the "every non-header row is a real
  data row at its real depth" invariant. It also needs a decision on what a sort means across a
  mixed axis. Also deferred: the incremental splice, since a node toggle under grouping currently
  costs a grouped `Reset`.
- **Ranged subtree updates on child-collection change.** v1 rebuilds, matching grouping's current
  behaviour; optimize against a benchmark.
- **`Filter` + a lazy `hasChildrenSelector`.** v1 throws (§5). Unblocking it needs a filter that can
  realize subtrees incrementally and answer asynchronously — a different shape of filter than the
  synchronous predicate the shaping stack has today.
- **True DAG support (one object under two parents).** The path key is built for it, but
  `ShapedItemsSource`'s object-uniqueness validation and an item-based `SelectedItems` are not
  (§4.2). Needs a row-addressed selection API and an identity contract that is not the object
  address.
- **Keyed data source for container preservation across a splice.** `InspectingDataSource` reports
  no key mapping, so containers below a toggle are re-prepared (§4.4). Shared with grouping; fix
  once, below both adapters.
- **Tri-state / cascading selection.** Product decision, not an architectural one.
- **Drag-reparent.** Out of scope.

---

## 10. Delivery plan

| PR | Content | Gate |
| --- | --- | --- |
| 0 | `RowExpansionModel::RetainOnlyUnder(liveKeys, enumeratedPrefixes, parentPrefixOf)` — prefix-scoped prune (§4.2). Layer 1 only, no consumer changes. | Unit tests: intent under an unenumerated prefix survives; intent under an enumerated prefix that vanished is pruned; intent whose *grandparent* was enumerated but whose parent vanished is pruned (the ancestor walk, not just the immediate parent); a throwing or non-reducing extractor prunes nothing; existing `RetainOnly` behaviour unchanged for the grouped caller |
| 1 | `HierarchicalSourceAdapter` + `NodeRow` + path identity + collapsed baseline + cycle/depth guards + `Rebuild` + splice + `m_indexByPathKey`. No control changes. | Unit tests on the adapter alone (no host, no dispatcher — the same testability `RowExpansionModel` has). Must include: collapsed 1M-node tree walks `O(roots)`; collapse-grandchild → collapse-parent → expand-parent preserves the grandchild's collapse; a cyclic children selector throws rather than overflowing; index map stays exact across splices |
| 2 | `ShapedItemsSource::RebuildHierarchical` / `RebuildGroupedHierarchical`, `ProjectionKind::Hierarchical` and `GroupedHierarchical`, per-sibling-set shaping | Shaping tests: sort/filter per level, match-node-only filtering, `GroupBy`+`WithChildren` buckets roots and keeps subtrees under their roots, a node toggle updates the grouped slices, shared child hits the duplicate-object diagnostic |
| 3 | `TableViewSource.WithChildren` IDL + wiring; `RowMetadataProvider::CreateForHierarchicalRows` | API review; metadata tests for `Level` (roots report 1, not 0) / `ChildCount` / path identity |
| 4 | `TableViewRow` indent + chevron + visual states + cells-panel gutter; toggle plumbing | Visual verification incl. RTL, frozen columns, density; roots render flush |
| 5 | Keyboard, selection semantics, automation peer (`IExpandCollapseProvider`, `Level`, sibling-scoped `SizeOfSet`) | TAEF + UIA harness (`tableview-uia-harness.ps1`) |
| 6 | `TableViewSampleApp` hierarchy page (Task-Manager-shaped data, lazy children, 100k-node perf case) | Perf harness (`Diagnostics/ShapingPerfHarness.cs`) |
