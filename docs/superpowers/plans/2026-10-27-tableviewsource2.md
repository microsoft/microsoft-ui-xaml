# TableViewSource2 — Implementation Plan

Spec: `docs/superpowers/specs/2026-10-27-tableviewsource2-design.md`.
Execution: native, in this session (executing-plans). Each step must build clean before the next.

## Global constraints

- Use `::ITableViewSource` with the global qualifier. TableView binds every variant through it.
- Reuse `CollectionAccessor`, `ShapingPipeline`, `ValueComparer`, `ValueKey`, `RowExpansionModel`,
  `LiveShapingTracker`, `ShapedGroup`, `GroupedEntry` and `SafeRevoke` without changes.
- The engine (`ShapedItemsSource2`) knows nothing about TableView. The row-metadata provider for v2
  lives on the TableView side, in `TableViewSource2.cpp`.
- Identity faults throw `E_INVALIDARG`, tagged as caller errors. The engine has no faulted mode.

## Implementation decisions (refine the spec; no behaviour change)

1. **Per-row cache with a structure rebuild.** The engine caches each source row's delegate outputs:
   row key, parent key, group value and key, sort keys, and filter result. A source change re-runs
   delegates only for the rows that changed. The pre-order array is then rebuilt from the cache in
   O(n log s) with **no app calls**. In-place leaf splicing of `R` is an optimisation, deferred
   until the harness shows rebuild cost matters.
2. **Visible diff by identity.** Old and new visible rows are compared by row identity: the row key,
   or the header's group key.
   - The common prefix and suffix are kept, and any row in them whose object changed gets a `SetAt`.
     That covers a header re-minted with a new count or expansion state, and an item replaced under
     the same key.
   - The middle is applied as removes and inserts, or as `ReplaceAll` when it is large.
   - This covers filter, expand/collapse, insert/remove and live moves through one path.
3. **Subscriptions reuse `LiveShapingTracker`.** Subscriptions are per object, which is the
   tracker's model. An `item → source index` map routes each event, and snapshots are compared
   through the row-key cache. No new tracker class is needed.
4. **Files.**
   - The engine is added to the existing `ShapedItemsSource` shared-items project as
     `ShapedItemsSource2.h/.cpp`; a separate project would need dll wiring for no benefit.
   - `PreorderProjection` stays inside the engine as a private section, since it is only meaningful
     with the cache.

## Steps

### Step 1 — Flat
- IDL:
  - add `ParentBy(TableViewIdentitySelector)` as `[method_name("ParentByIdentity")]`;
  - add `runtimeclass TableViewSource2`;
  - v1's implementation of the new `ParentBy` throws `E_NOTIMPL`.
- `TableViewSource2.h/.cpp` and `Generated\TableViewSource2.properties.cpp`, plus vcxitems entries.
- `ShapedItemsSource2`:
  - source subscription;
  - row cache and identity validation;
  - filter and sort;
  - projection build, scans and diff;
  - `ItemsSourceView`.
- v2 row-metadata provider (flat).
- Registry `"v2"`.
- Build the tabular DLL.

### Step 2 — GroupBy
- Group cache: `ShapedGroup` by group key, with header order that matches v1.
- `GroupedEntry` headers with re-mint on change.
- Group expansion.
- Provider: headers and `TryGetGroupMembership`.

### Step 3 — ParentBy(parentSelector)
- Parent resolution, cycle detection, CSR child lists and sibling sort.
- Node expansion, defaulting to collapsed, with a filter-context overlay.
- Grouped + hierarchical: only roots are grouped.
- Provider: node keys, level, parent identity, and position or size in set.

### Step 4 — Live shaping
- Subscriptions while live shaping is on, or while a sort is active.
- Snapshot comparison and one coalesced restore per turn.
- Hold/resume and the owner hooks.
- Off-thread marshalling.

### Step 5 — Tests and harness
- Parameterize the API tests by registry variant.
- Add v2-specific identity tests.
- Harness variant switch.
- Build the test app.

## Review focus
- The identity rules: from, insert, replace, live change, and cycle.
- Header order parity with v1.
- Diff correctness when a header is re-minted and when a key is replaced.
- The engine's lifetime under off-thread events: weak `this` everywhere.
