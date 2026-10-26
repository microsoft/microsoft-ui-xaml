# TableView Object-Address Identity Unification — Implementation Plan

**Spec / background:** [TableView-Row-Identity-and-Index-Maps.md](TableView-Row-Identity-and-Index-Maps.md)

**Goal:** Every map/set that keys on "which object is this?" computes the key through one
function and stores it in one type. Logical row identity (the identity selector string) stays a
separate, pluggable path.

**Principles agreed**

1. One address function everywhere: QI to canonical `::IUnknown`, take its pointer, `0` for null.
2. Logical identity (`EffectiveIdentitySelector` / `IdentitySelector` / `MakeObjectIdentitySelector`,
   group keys) is **not** changed. It may later become an app-supplied key; physical
   bookkeeping (subscriptions, snapshots, ranks, dirty rows, item→index lookups) must keep using
   the object address regardless. Node keys are unchanged until T12, which replaces them as the
   hierarchy row identity.
3. One representation: replace `void*`, `void const*`, `uintptr_t` keys with a single strong type.
4. State policy: a new object is a new row. Expansion and selection are kept for the same object
   (reshape, `Refresh()`), not for a re-created object with equal data.

**Ordering:** T1–T11 first. Then T12 (hierarchy on object-address identity). T13 (address-reuse
fix) is deliberately last: T1–T12 may surface more maps or lifetime paths it must cover.

**Layering constraint:** `TabularShaping` is the lowest layer (ShapedItemsSource and
HierarchicalSourceAdapter depend on it, not vice versa). The helper must live there.

---

## Current state (sites to change)

| Map | Owner | Key type today | Address computed by | Canonical IUnknown? |
|---|---|---|---|---|
| #1 `m_flatRowIdentityToIndex`, #2 `m_flatRowIdentities` | ShapedItemsSource | `hstring` | selector → `RowIdentity::GetObjectAddress` → `FormatObjectIdentity` | yes (`as`) — **leave (logical)** |
| #4 `m_indexByItem` | HierarchicalSourceAdapter | `void*` | `winrt::get_abi(item)` | **no** |
| #6 `m_rootBucketIndex` | ShapedItemsSource | `void*` | `winrt::get_abi(root)` | **no** |
| #7 `m_rankByIdentity` (+ `RankEntry::Identity`) | CustomSortRankAdapter | `void*` | `IdentityOf` (`try_as`) / inline `try_as` in `KeyFor` | yes |
| #8 `ParentStructure::KeyByItem` | ParentKeyIndex | `void*` | `winrt::get_abi(item)` | **no** |
| #12 `m_liveShapeSnapshots` | ShapedItemsSource | `void const*` | `LiveShapingKeyFor` | yes |
| #13 `m_subscriptions` (+ `RetainOnly` set) | LiveShapingTracker | `void const*` | `LiveShapingTracker::Identity` (`as`) | yes |
| #14 `m_flatItemPropertyChangedRevokers` | ShapedItemsSource | `uintptr_t` | `RowIdentity::GetObjectAddress` | yes |
| S3 `m_sortDirtyRows` | ShapedItemsSource | `uintptr_t` | captured from #14 lambda | yes |

Also using the same pattern ad hoc: `ShapingHelpers.cpp` `ValueKey::ToString` (`"o:%016llX"`),
`ValueKey::ToObjectLookupKey` (`"object:%p"`); `RowIdentity.cpp` `GroupKeysEqual`;
`ShapedItemsSource.cpp` structure compare (`get_abi(structure->Rows[i]) != get_abi(rows[i])`).

---

## TODO list

### T1 — Add shared helper in TabularShaping
**Files:** `controls\dev\TabularShaping\ShapingHelpers.h/.cpp`

- [ ] Add to `namespace ShapingHelpers`:
  ```cpp
  // Physical object identity: address of the canonical IUnknown. Never dereferenced; callers
  // that need it to stay unique must keep the object alive (or hold a weak ref and re-check).
  enum class ObjectAddress : uintptr_t { Null = 0 };

  ObjectAddress GetObjectAddress(winrt::IInspectable const& item) noexcept;
  ```
  (`std::hash` for enums is standard since C++14 — no custom hasher needed.)
- [ ] Implement with `try_as<::IUnknown>()`, `Null` on null/QI failure. Must be `noexcept`
  (today `RowIdentity::GetObjectAddress` uses `as<>`, which throws).
- [ ] Add a `FormatObjectAddress(ObjectAddress)` only if T6 needs it; otherwise keep formatting in RowIdentity.

### T2 — Route RowIdentity through the helper
**Files:** `controls\dev\ShapedItemsSource\RowIdentity.h/.cpp`

- [ ] `RowIdentity::GetObjectAddress` → delegate to `ShapingHelpers::GetObjectAddress` (or delete
  it and update callers; prefer delete to avoid two names).
- [ ] `FormatObjectIdentity` takes `ObjectAddress`; output string unchanged (`"0x%zx"`) so #1/#2
  and RowMetadataProvider identities are byte-identical.
- [ ] `MakeObjectIdentitySelector` unchanged in behaviour. Add comment: "logical row identity;
  may be replaced by an app key — do not use for physical bookkeeping".
- [ ] `GroupKeysEqual` (RowIdentity.cpp ~318): use helper for the object-compare branch.

### T3 — #7 CustomSortRankAdapter
**Files:** `controls\dev\TabularShaping\ShapingHelpers.h` (~225–330), `ShapingHelpers.cpp` (~1114, ~1228, ~1360)

- [ ] `RankEntry::Identity` → `ObjectAddress`; `m_rankByIdentity` → `unordered_map<ObjectAddress, int32_t>`.
- [ ] Delete `IdentityOf`; call `GetObjectAddress`.
- [ ] `KeyFor` (~1228): replace inline `try_as<::IUnknown>` lookup with `GetObjectAddress`.
- [ ] `EvictEntry` (~1360): `identity != ObjectAddress::Null && entry.Identity == identity`.

### T4 — #12 live-shape snapshots and #13 LiveShapingTracker
**Files:** `controls\dev\ShapedItemsSource\ShapedItemsSource.h` (324, 419), `ShapedItemsSource.cpp` (~2562, 2654–2832);
`controls\dev\ShapedItemsSource\LiveShapingTracker.h/.cpp`

- [ ] Delete `LiveShapingKeyFor` and `LiveShapingTracker::Identity`; use `GetObjectAddress`.
- [ ] `m_liveShapeSnapshots`, local `snapshots` → `unordered_map<ObjectAddress, LiveShapeSnapshot>`.
- [ ] `m_subscriptions` → keyed by `ObjectAddress`; `RetainOnly(std::unordered_set<ObjectAddress> const&)`;
  local `live` set (cpp ~2654) same type.
- [ ] Null checks `if (!identity)` → `== ObjectAddress::Null`.
- [ ] Keep weak-ref recycle check in `Subscribe` unchanged.
- [ ] Verify (while here) that `m_liveShapeSnapshots` entries are erased on remove (~2727) — the map
  holds no ref, so a missed erase allows address reuse to hit a stale snapshot. Record finding in doc.

### T5 — #14 and S3 flat sort-key tracking
**Files:** `ShapedItemsSource.h` (~390, #14 decl, `OnFlatItemPropertyChanged` decl), `ShapedItemsSource.cpp` (~938, ~1050–1180)

- [ ] `m_flatItemPropertyChangedRevokers` key, `m_sortDirtyRows`, local `dirtyRows`, and
  `OnFlatItemPropertyChanged(itemAddress, …)` param → `ObjectAddress`.
- [ ] Replace `RowIdentity::GetObjectAddress` calls (1090, 1138, 1173) with helper.
- [ ] `RowIdentity::FormatObjectIdentity(address)` at ~958 takes `ObjectAddress` (from T2).

### T6 — Hierarchy raw `get_abi` sites (behaviour change: IInspectable* → canonical IUnknown*)
**Files:** `controls\dev\HierarchicalSourceAdapter\HierarchicalSourceAdapter.h` (~167) / `.cpp` (702, 727);
`controls\dev\TabularShaping\ParentKeyIndex.h` (~24) / `.cpp` (168, 217);
`ShapedItemsSource.h` (249) / `.cpp` (2084, 2093, 2403)

- [ ] #4 `m_indexByItem` → `unordered_map<ObjectAddress, int32_t>`; lookups/emplace via helper.
- [ ] #8 `KeyByItem` → `unordered_map<ObjectAddress, std::wstring>`; `TryGetKey` and build via helper.
- [ ] #6 `m_rootBucketIndex` + local `rootBucketIndex` → `ObjectAddress` keys.
- [ ] Note the semantic fix: today two different interface pointers for the same object miss each
  other; after the change they match. Today these are fed from the same vector, so expected no
  visible change — tests must confirm.
- [ ] Perf note: each lookup now does a QI (AddRef/Release). Hot loops: `EnsureItemIndex`,
  `BuildParentStructure`. Accept (O(n) cheap QI) unless profiling says otherwise.

### T7 — Leave as-is (documented decisions)
- [ ] Structure compare (ShapedItemsSource.cpp ~2229): keep raw `get_abi` pointer equality. Both
  sides come from the same source vector; a false "changed" only costs a rebuild; QI per row would
  slow the fast path it exists for. Add one-line comment saying why.
- [ ] `ValueKey::ToString` / `ToObjectLookupKey` (ShapingHelpers.cpp ~565, ~588): optional — switch
  the `try_as` to `GetObjectAddress` but **keep string formats** (`"o:%016llX"`, `"object:%p"`),
  since those strings are group/sort keys. Do only if trivially safe.
- [ ] Identity selector path (#1/#2, RowMetadataProvider, `TableViewSource::MakeIdentitySelector`): unchanged.

### T8 — Optional naming cleanup
**Files:** `ShapedItemsSource.h` (~161, ~282)
- [ ] Consider merging `EffectiveIdentitySelector()` into `IdentitySelector()` (pure pass-through).
  Separate commit; skip if it widens the diff.

### T9 — Build
- [ ] `.\initrun.ps1 msb /q "controls\dev\dll\Microsoft.UI.Xaml.Controls.vcxproj"` (if "Run a full init first": `.\init.ps1`).
- [ ] Zero new warnings (enum class will flag any missed `void*` mixing at compile time — that is the point).

### T10 — Tests
**Existing TAEF API tests** in `controls\dev\TableView\APITests\` — run via
`.\initrun.ps1 .\tools\run-tests-on-vm.ps1 -VMName <vm> <testname>`:
- [ ] `*TableView_LiveShaping*` (incl. `VerifyReplacedItemOldOutNewIn`, `VerifyComparerReinsertedRowIsReplaced`, `VerifyObjectKeyReparent`)
- [ ] `*TableView_Sorting*` (incl. `VerifyDuplicateSortMemberPathsTrackColumnIdentity`)
- [ ] `*TableView_Hierarchy*` (incl. `VerifyObjectKeysLinkByIdentity`, `VerifyReplaceReparents`)
- [ ] `*TableView_Grouping*`, `*TableViewSource*` (incl. `VerifyReplaceWhileShapedReevaluatesTheNewItem`), `*TableView_Selection*`

**New tests (add if feasible from C#):**
- [ ] Custom-comparer sort + replace item with a *new* object having equal sort value → new row ranked, old rank evicted (#7).
- [ ] Flat sorted source, mutate sort key on an INPC item → row moves; remove item then mutate it → no move/crash (#14/S3).
- [ ] Hierarchy: same object reachable via two projections (if a C# scenario can produce different IInspectable pointers) — otherwise document as not testable from managed code.

### T11 — Docs
- [ ] Update `TableView-Row-Identity-and-Index-Maps.md` §3/§5: single `ObjectAddress` type, single
  function, logical vs physical identity rule, T7 decisions, T4 snapshot-erase finding.

### T12 — Hierarchy rows on object-address identity (after T1–T11)
**Goal:** a hierarchy node's identity becomes `"0x…"` (`FormatObjectIdentity(GetObjectAddress(item))`),
the same rule as flat rows. App node keys (`ParentBy(key, parentKey)`) are used only to *resolve
parent links* during the build; they stop being the row identity. This also prepares the tree
for a future children-mode source, which has no app key and produces the same address-based tree.

**Files:** `controls\dev\TabularShaping\ParentKeyIndex.h/.cpp`;
`controls\dev\HierarchicalSourceAdapter\HierarchicalSourceAdapter.h/.cpp` (+ `RowExpansionModel` use);
`controls\dev\ShapedItemsSource\ShapedItemsSource.cpp` (hierarchy structure / reuse check, live reparent);
`controls\dev\TableView\RowMetadataProvider.cpp` (`GetIdentity` Hierarchical / GroupedHierarchical);
`controls\dev\TableView\TableView_Selection.cpp` (`m_stickySelectedIdentity`).

Model:
```
Node { ObjectAddress Address; hstring Identity /* "0x…" */; std::wstring NodeKey /* build-only data */; ... }
AddressIndex : ObjectAddress → node   // structure, object-driven events (#4, #8)
IdentityIndex: hstring "0x…" → node   // API, expansion, selection (#3 renamed)
```

- [ ] **Build** (`BuildParentStructure`): pass 1 computes `key(item)` → #5 (key → source position);
  pass 2 resolves `parentKey(item)` → parent index via #5 and links nodes by index/address. Each
  node gets `Address` and `Identity`.
- [ ] **#5 `ParentStructure::IndexByKey` is kept** (not discarded after the build, not merged with
  #8), until the design matures. Reason: in ParentBy mode a child carries only its parent's key
  value, so key → row is the only way to resolve the parent. It is no longer row identity; it is
  build/link data. Revisit only after children-mode exists (that mode needs no #5).
- [ ] **#5 vs #8 roles after T12** (they do not merge):
  - #5 key → source position: kept, used for parent resolution (and re-resolution on live key edits).
  - #8 item → key: loses its identity role (identity is now the address). Keep only as the
    old-key snapshot live shaping needs to detect key/parentKey edits; otherwise remove.
  - #3 + #4: become one visible-tree index keyed by address / `"0x…"`.
- [ ] #9 `ParentKeyIndex::Children`: key by parent node index/address instead of parent key string.
  `ContextKeys` → set of addresses (or identities).
- [ ] #3 `m_indexByNodeKey` → `m_indexByIdentity` (`"0x…"` → visible index); fold #4 into it. Rename
  `TryGetIndexForNodeKey`, `IsNodeExpanded`, `SetNodeExpanded`, `TryApplyExpansionSplice` params to identity.
- [ ] S1 hierarchy `RowExpansionModel` and S2 `m_filterOverlayCollapsed`: key by `"0x…"`.
- [ ] `RowMetadataProvider::GetIdentity`: Hierarchical and GH rows return node `Identity`; the GH
  "fallback to item key mid-reshape" becomes the same value, so the fallback can be removed.
- [ ] Hierarchy sticky selection (`m_stickySelectedIdentity`) now holds `"0x…"`.
- [ ] Review `IsObjectNodeKey` / `"node:object:…"` handling: no longer an identity concern; keep only
  if parent resolution still needs it.
- [ ] Rules to keep: same object twice in the source is still an error (duplicate address as well as
  duplicate key); missing parent / orphan handling unchanged; key or parentKey edit under live shaping
  re-resolves links and keeps the node's state (same object = same identity).
- [ ] **Behaviour change (intended, per policy 4):** expansion and selection no longer survive the app
  replacing objects with new ones that have equal keys. Find and update API tests asserting the old
  behaviour (`TableView_Hierarchy_APITests.cs`, `TableView_LiveShaping_APITests.cs`,
  `TableView_Selection_APITests.cs`; e.g. replace/reset with same keys).
- [ ] New tests: (a) `Refresh()` / sort / filter keeps expansion and selection; (b) replacing a node
  object with a new one with the same key resets that node's expansion and selection; (c) editing an
  item's key/parentKey under live shaping keeps its expansion.
- [ ] Build (T9 command) and run `*TableView_Hierarchy*`, `*TableView_LiveShaping*`,
  `*TableView_Selection*`, `*TableView_Grouping*` (GH).
- [ ] Update design doc §1, §2 (Hierarchy), §3, §4 Key column.

### T13 — Address-reuse fix (final change; scope may grow from T1–T12)
**Problem:** an address key is only unique while its object is alive. If an entry outlives its object,
a new object allocated at the same address matches the stale entry (ghost expansion, selection,
snapshot, rank).

**Rule (option C, purge before release):** no address-keyed entry holds its own strong ref to an app
item (app owns item lifetime). The engine's projection (`m_rows`, `ParentStructure::Rows`, walked
child collections) is the only strong ref. Every entry for an item is erased **before** the engine
releases that ref, so the address cannot be reused while a stale entry exists.

Three release paths, each must purge every address map:

| Path | Trigger | Required order |
|---|---|---|
| Incremental remove | Remove / Replace on the flat incremental path | erase item from all maps → remove from `m_rows` |
| Full rebuild | Reset, `Refresh()`, grouped/hierarchy rebuild | `old = move(rows)` → build new → `RetainOnly(newLive)` on every map → destroy `old` |
| Source swap / detach | `ItemsSource` set / cleared, unload | clear every map → release rows |

- [ ] Re-inventory address-keyed maps/sets after T12 (list may have grown or shrunk).
- [ ] Audit each map against all three paths. Known gaps to fix:
  - [ ] #12 `m_liveShapeSnapshots`: no ref; confirm erase on remove and `RetainOnly` on rebuild.
  - [ ] S3 `m_sortDirtyRows`: erase in `UntrackFlatItemPropertyChanged` (stale entry is harmless today, fix for consistency).
  - [ ] S1 / S2 / sticky identity (`"0x…"` after T12): `RetainOnly(liveIdentities)` on every rebuild; clear sticky identity when its row leaves.
- [ ] Audit existing extra strong refs for post-removal leaks: `RankEntry::Item`,
  `FlatItemPropertyChangedSubscription::Item`, `m_stickySelectedItem`. Each must be dropped when the row
  leaves; replace with weak ref + verify (option B, as in `LiveShapingTracker`) where an entry must
  reach the object.
- [ ] Verify every rebuild follows build → purge → release (as `RebuildFlatItemPropertyChangedTracking` does).
- [ ] Silent sources: confirm the next rebuild purges items removed without notification.
- [ ] Tests: remove an item, force GC, assert it is collected (`WeakReference`) for flat, grouped,
  hierarchy, live shaping, custom sort; remove + add a new item, assert no inherited expansion/selection.
- [ ] Record any additional work found here before implementing it.
- [ ] Update design doc §5 with the lifetime rule.

### Suggested commit split
1. T1+T2 (helper + RowIdentity) 2. T3 3. T4 4. T5 5. T6 6. T7 comments + T11 docs 7. T8 (optional)
8. T12 (may split: build/structure, then identity/state + tests) 9. T13 (last).
Each commit must build on its own.
