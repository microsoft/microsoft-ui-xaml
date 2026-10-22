# TableView Row Identity Reuse — Spec

**Status:** Draft for review. All design questions are resolved (D1–D27); an implementation plan
follows once this spec is approved.

**Background:**
[TableView-Row-Identity-and-Index-Maps.md](TableView-Row-Identity-and-Index-Maps.md),
[TableView-Object-Address-Unification-Plan.md](TableView-Object-Address-Unification-Plan.md)

**Goal:** One shared source-tracking system: a table of records for the source rows plus their
lookups, owned in one place. Every component that needs to know "which row is this?" (engine,
grouped and hierarchy adapters, metadata provider, selection, live shaping, ranks) uses it,
instead of computing identity itself or keeping its own copy.

Fewer QIs and string formats are a result of that sharing, not the target. A solution that still
does an occasional QI is fine; a solution where each component keeps its own identity derivation
or map is not.

---

## Problem

The object-address unification (T1–T13) gave every "which object is this?" lookup one function
(`GetObjectAddress`) and one key type (`ObjectAddress`). It did not cache the result. The same
surviving object is still processed many times:

- **Full identity derivation** (QI to canonical `IUnknown` → format `"0x…"` → box → unbox) runs
  several times for one row in one shaped Refresh:
  - `ValidateRowIdentities` derives every row's identity.
  - `RebuildFlatRowIdentityTracking` derives every row's identity again.
  - A sort-only in-place reorder derives every row's identity again, even though no row changed.
  - Incremental remove/replace derives the affected row's identity, then derives the projected
    row's identity again to check the cached index.
  - `RowMetadataProvider::GetIdentity` derives it again, per request, for flat rows (without a
    key-index mapping) and grouped data rows.
  - The dirty-sort path turns an `ObjectAddress` back into a string with `FormatObjectIdentity`
    before looking it up.
- **Address-only lookups** (one QI, no string) repeat for the same object across the root-bucket
  index, custom-sort ranks, live snapshots, live subscriptions, flat sort subscriptions, selection
  and focus reconciliation, and hierarchy item lookups.
- Hierarchy is the exception: `ParentStructure` caches `{Address, Identity}` per source row.
  But that cache is private to one retained structure, so flat and grouped paths get no benefit
  from it.

None of this is wrong; the values match every time. It is repeated work, and it grows with row
count × number of consumers × number of rebuilds.

## Agreed principles

1. **One shared source of row identity.** An item's address and identity are worked out in one
   place and stored there. Every consumer reads those stored values instead of computing its own
   or keeping a private copy. A Refresh or rebuild normally QIs an item at most once as a result,
   but that is a consequence, not a hard rule.

2. **The source table is the authoritative in-memory snapshot; reuse across `Reset`.** The source
   collection is materialized on `Start()` and re-materialized only on a `Reset` notification (or
   when an index-only notification cannot read the item). Shaping-verb changes, live-shaping
   restores, and non-`Reset` collection changes run directly off the source table without
   re-enumerating the source (D27). When a `Reset` does re-materialize the source, stored values
   are reused for surviving items by position, then by raw pointer, then by address (D17). Only
   genuinely new items (or items returned through a different COM wrapper) are QI'd.

3. **Two separate concepts, both kept:**
   - **Address** (physical): the canonical `IUnknown` pointer. Subscriptions, ranks, snapshots
     and lifetime cleanup always need it, whatever the identity policy is.
   - **Identity** (logical): the string used for selection, expansion and identity → index
     lookups. Today it is derived from the address (`"0x…"`). Later it may come from values the
     app provides.

4. **Stability.**
   - An address stays the same for the whole life of the object. Editing properties, sorting,
     filtering, expanding, collapsing or moving the item does not change it. Replacing the source
     collection does not change it for objects that survive the swap.
   - An identity derived from the address has the same stability.
   - An identity provided by the app **can change without any collection change**: for example,
     when the `Id` property is edited. Any cached app-provided identity needs either per-item
     invalidation or a contract that it must not change while the item is in the source.

5. **Identity reuse is not index reuse.** Identity → index maps must still be rebuilt when
   positions change (sort, filter, insert, remove). The goal is to stop regenerating identity
   *values*, not to stop recomputing *positions*.

6. **Lifetime rule (from T13) still applies.** A cached address or identity is valid only while the
   engine keeps the object alive. Cached entries are purged before the engine releases its last
   reference, so a recycled address can never match a stale entry.

## Scope

**In scope**

- A shared source-tracking system (record table plus lookups) with one owner, used by every
  component that needs row identity or address.
- Caching the address and the address-derived identity per source row in that system.
- Reusing those values across successive Refresh and rebuild passes.
- **Migrating existing maps now** where they duplicate what the shared system holds, so the
  component draws from it or the map goes away. Which maps qualify is decided per map (see
  [Map migration](#map-migration)).
- A seam for a future app-provided identity: write down its contract and how it would be
  invalidated, but don't build it.

**Out of scope**

- Changing identity semantics, other than D2. A new object is still a new row. D2 is a deliberate
  exception: duplicate object instances and null items are now rejected in unshaped sources too
  (previously only shaped sources rejected duplicates).
- Building app-provided identity selectors.
- Hierarchy parent-link keys. They come from app values and are evaluated by their own path.
- **Deferred to a follow-up spec** (more complex, not excluded): group identity reuse, including
  #10/#11 overlap and per-item group-key derivation. See [Grouping: deferred, not excluded](#grouping-deferred-not-excluded).
- Sort keys and comparer ranks as values. Only the address lookups they do for bookkeeping are in
  scope.
- Maps that describe a projection or per-consumer state rather than the source, where the shared
  system would add nothing (decided per map).

## Known constraints

- **Boxed value-type items:** a .NET boxed value can be handed out with a different wrapper on
  each read, so its address is not stable between reads. Reusing values for it must not assume
  that reading the same index again gives the same object.
- **No duplicate object instances:** a source cannot hold the same object instance (same
  `ObjectAddress`) more than once, whether shaped or unshaped (D2).
- **Non-observable sources** are snapshotted once on `Start()`. Unnotified mutations to a
  non-observable collection are not supported and are never polled on shaping changes (D27). This
  is a TableView choice and a behaviour change, not alignment with another collection view.
- **Reentrancy:** app code (selectors, comparers, event handlers) can start a new rebuild in the
  middle of a flow. Cached values must stay consistent with the generation guards added in T13.

## Proposed source record

One record per source row. It holds only values that depend on the object alone and live exactly as
long as the item stays in the source. It stays generic: the same record works for flat, grouped and
hierarchy sources, with or without live shaping.

```cpp
struct SourceRecord
{
    winrt::IInspectable Item;                    // strong; the record owns the item's lifetime
    ObjectAddress       Address;                 // canonical IUnknown, one QI
    winrt::hstring      Identity;                // logical identity, created lazily (D16); "0x..." today
    winrt::event_token  PropertyChangedToken{};  // 0 = not subscribed
    WatchState          Watch;                   // Unchecked | NotObservable | Subscribed
    bool                Retired{ false };        // set on retirement; rejects late callbacks (D21)
};
```

### Why each field is there

| Field | Reason |
|---|---|
| `Item` | Replaces `m_sourceItems` as the owner. Under the T13 rule the record's lifetime is the item's lifetime, so no record outlives its object. The raw ABI pointer for positional reuse is `get_abi(Item)`, so it needs no field of its own. |
| `Address` | Physical identity: forms the address-derived identity, detects one object reached through two raw pointers, and serves the rare item-only lookup when `rawLookup` misses (D13). |
| `Identity` | Logical identity. Formatting the string costs more than the QI, so it is cached rather than recreated. |
| `PropertyChangedToken` | One `PropertyChanged` subscription per item, used by live shaping (the only remaining subscriber; see [Decisions](#decisions)). The record holds the item strongly, so revoking is `Item.as<INPC>().PropertyChanged(token)` before release; no weak reference or `auto_revoke` is needed. Items with and without `IWeakReferenceSource` go through the same path. |
| `Watch` | `NotObservable` caches "not INPC", so a non-INPC item is not QI'd again on every Refresh. Kept per item because an `IVector<IInspectable>` can hold mixed types. |
| `Retired` | Set when the record leaves the source. A retired record can stay alive while a holder keeps it (D10), so late callbacks check this flag (D21). |

### Kept outside the record

| State | Owner | Reason |
|---|---|---|
| Live-shape snapshot (#12) | Live shaping, keyed by `SourceRecord*` (D13) | Exists only while live shaping is on; the values depend on the current verbs. |
| Source index, projected index | Vector positions | Change on every insert, remove or reshape. |
| Sort keys, filter result, group key | Pipeline | Depend on the current verbs. |
| Custom rank | Comparer adapter | Belongs to the current sort spec. |
| Expansion and selection state | Keyed by identity | Lets a future app-provided identity carry state across a replaced object. |
| Hierarchy node and parent keys | `ParentStructure`, live snapshot | Come from app data that can change; existing invalidation stays as is. |

Side maps key on `SourceRecord*` (D13), so they no longer QI. They still follow the
purge-before-release rule.

### What this changes

- The live subscription map #13 (`LiveShapingTracker::m_subscriptions`) goes away; the record
  holds the subscription.
- Sort-key tracking with live shaping off (#14, S3) is removed entirely; see [Decisions](#decisions).
- With live shaping off, the subscription slot stays empty for every record.

## Decisions

### D1. Remove sort-key tracking when live shaping is off

**Decided:** accept a stale sort order until the next Refresh.

**What exists today.** PR #12190 (commit `166972a99`) added a `PropertyChanged` subscription per
projected INPC row while the table is sorted and live shaping is off
(`m_flatItemPropertyChangedRevokers`, #14). A change to a sort-key property marks the row dirty
(`m_sortDirtyRows`, S3). The subscriptions never re-sort on the edit itself. On the next single-item
collection change, `TryReshapeStaleSortedFlatProjection` checks the dirty rows (or the whole list
above 256 rows), re-sorts if needed, and only then lets the sorted incremental path binary-search
its insert position.

**Why remove it.**
- Live shaping off means "edits do not re-sort". The only job of this machinery is to make
  the binary-search splice exact after an edit the app chose not to have re-shaped.
- It costs one subscription per projected INPC row, a dirty set, a property-name filter, a
  subscription-ID scheme to reject queued callbacks (added in T13), and stale-repair code on every
  collection change.
- Without it, the record's subscription slot serves only live shaping, and every row stays
  unsubscribed while live shaping is off.

**Behaviour after removal.** With live shaping off, a sort-key edit leaves the row where it is,
matching WPF's `ListCollectionView` when `IsLiveSorting` is off. A later single-item insert
binary-searches the current, possibly unsorted, list (`AdjustBefore` / `BinarySearch` in WPF). In a
stale list the search can return an unexpected position. That is accepted: the order is corrected by
the next `Reset`, a sort change, or enabling live shaping. Single-item `Remove`, `Replace`
(outgoing lookup), and `Move` remain exact even when sort keys are stale because they locate the
existing row via the shared projection index (D7, equivalent to WPF's `InternalList.IndexOf(item)`)
rather than binary search.

**Removed:** #14 `m_flatItemPropertyChangedRevokers`, S3 `m_sortDirtyRows`, `m_sortDirtyOverflow`,
`m_sortKeyPropertyNames`, `m_anyPropertyMayAffectSort`, `m_nextFlatItemSubscriptionId`, the
live-off branch of `TryReshapeStaleSortedFlatProjection`, and their tracking/untracking calls.

**Tests asserting the removed behaviour (verify and update during implementation):**
- `TableViewSource_APITests.cs`: `VerifySortKeyPropertyChangeDoesNotMoveTheRowUntilACollectionChange`,
  `VerifyStaleSortRepairIsReportedAsAReshape`, `VerifyNonSortPropertyChangeDoesNotMarkTheSortStale`,
  possibly `VerifyFilterAndSortAfterFilterKeyPropertyChangeAndAdd`.
- `TableView_LiveShaping_APITests.cs`: `VerifyDisableHandsStalenessBackToTheCollectionChangeRepair`.

**Note:** #12190's release notes say a row with a changed sort key "moves to its correct position by
itself". With live shaping off it only moved on the next collection change. After this decision it
moves only on the next Refresh.

### D2. One record per unique source object; duplicates rejected upfront

**Decided:** duplicate object instances (two positions with the same `ObjectAddress`) and null items
are rejected when building or updating the `SourceRecord` table, in **both shaped and unshaped**
modes.

- Every `SourceRecord` in the table has a distinct `Address` and `get_abi(Item)`, so one object
  means at most one record and one row everywhere.
- Duplicate detection happens once when records enter the source table (`Start()`,
  `ReloadFromSource()`, and incremental `Add` / `Replace`), before any record retirement, table
  mutation, or shaping pipeline runs. A duplicate or null item throws `E_INVALIDARG` atomically,
  leaving the existing source table and lookup maps untouched.
- Retiring a record means its object has completely left the source: owner-state cleanup (D24)
  never has to check whether another position still holds the same `Address`.
- **Public behaviour change.** Unshaped tables that today show duplicate rows or null rows will
  throw instead. `ItemsRepeater` itself accepts both, so this needs a release note.
- **Cost for unshaped tables:** every row is now QI'd once for its `Address` so uniqueness can be
  checked (D6). Previously the unshaped path skipped this. It is paid once per object and reused
  across `Reset`s (D17); identity strings stay lazy (D16).
- **Tests asserting the previous duplicate/null behaviour (update during implementation):**
  - `TableView_DataBinding_APITests.cs`: `VerifyDuplicateItemInstancesProduceIndependentRows`
  - `TableView_Sorting_APITests.cs`: `VerifySortOverDuplicateItemFailsSafely`,
    `VerifySortByColumnReturnsFalseForDuplicateRows`, `VerifySortOverNullItemFailsSafely`
  - `TableViewSource_APITests.cs`: `VerifyDuplicateItemObjectThrowsWhenShaped` (now throws on
    `From(items)` rather than waiting for `Filter(...)`).
  - `TableView_Columns_APITests.cs`: check `VerifyGenerateElementWithNullDataItemDoesNotCrash`;
    update only if it routes a null item through the source.

### D3. Records on the heap; projection holds pointers

**Decided:** the source table is a vector of record handles in source order (`shared_ptr` since
D10; originally `unique_ptr`). The projection is `vector<SourceRecord*>`, aligned with the rows on
screen (`m_rows`).

- Records never move, so a `SourceRecord*` is a stable handle across vector growth and mid-source
  inserts. Callbacks, ranks and lookups can hold one.
- A source insert or remove shifts only the pointers in the source table; projection entries need
  no renumbering.
- `GetIdentity(i)` on a projected row becomes `projection[i]->Identity`.
- Cost: one heap allocation per record, comparable to the identity `hstring` allocation already
  paid per row.
- Rejected alternative: records stored by value with the projection holding source indices. That
  saves allocations, but records move on growth and insert, nothing can hold a pointer, and every
  mid-source edit renumbers the projection in O(n).

### D4. Pipeline stays on items; records found through a persistent raw-pointer lookup

**Decided:** filter, sort, group and hierarchy keep working on `vector<IInspectable>`. Their output is
mapped back to records through `rawLookup` (`get_abi(item)` → `SourceRecord*`).

- No QI: the pipeline only copies pointers it received from the source table, so every output
  pointer matches a record's `get_abi(Item)` exactly.
- The lookup is persistent. An entry is added when a record is created and removed when the record
  is retired, before the item is released. It is not rebuilt per Refresh.
- A shaping-verb change (sort, filter, group) therefore does no QI and no lookup build; it does one
  lookup per projected row.
- The key is safe to keep because the record holds the item strongly, so the pointer cannot be
  freed and reused while the entry exists.
- Group buckets and hierarchy nodes map back the same way. `ParentStructure` can take
  `{Address, Identity}` from records instead of computing them.
- Rejected alternative: carrying `SourceRecord*` through the pipeline. It avoids the map-back but
  changes every shaping layer (`ShapingState`, `KeyedBucket`, `ParentKeyIndex`, sort helpers, ranks,
  live shaping, the incremental sorted path) at once. It remains possible later, layer by layer.

### D5. Two lookup maps: raw pointer and identity

**Decided:**

```text
rawLookup:     void* (get_abi(Item)) → SourceRecord*   // pipeline map-back, item → record, no QI
identityIndex: hstring Identity      → SourceRecord*   // logical lookups
addressSet:    ObjectAddress set                       // duplicate-object check only (D6)
```

- `identityIndex` serves identity → row lookups: selection and focus restore, metadata
  `IndexFromKey`, hierarchy expansion. It does not depend on the identity policy. Parsing `"0x…"`
  back to an address works only while identity is the address; once identity comes from app values
  there is no address to parse.
- The keys are `hstring`s shared with the record (an `AddRef`, no copy). String hashing is fine
  because these lookups happen per restore or request, not per row per Refresh.
- An identity key can't serve the pipeline map-back: looking up an item by identity would first
  require computing its identity, which is the work this design removes.
- No canonical-address *lookup* index (address → record) for now. Physical side maps (live
  snapshots, ranks) key on `SourceRecord*` (D13). The only address-keyed structure is `addressSet`,
  which exists solely to reject duplicates (D6).
- Rare path: the app sets `SelectedItem` with an arbitrary item → try `rawLookup`; on a miss,
  compute the identity once and use `identityIndex`.
- Rejected alternative: one map keyed by canonical address plus raw pointer, parsing identity back
  to an address. It uses fewer entries, but it ties logical lookups to address-derived identity.

### D6. `rawLookup` is always maintained; `identityIndex` is built when needed

**Decided:** because duplicate objects are rejected upfront in all modes (D2), keys are always
1:1 with records.

- `rawLookup` (`void*` → `SourceRecord*`) is maintained unconditionally as records are added and
  retired. It requires no QI or string formatting.
- `identityIndex` (`hstring Identity` → `SourceRecord*`) is maintained while at least one shaping
  verb is active (so unshaped tables still avoid formatting `Identity` strings, preserving D16).
  Turning the first verb on builds `identityIndex` in O(n) from the existing records with no QI;
  turning the last verb off drops it.
- Duplicate-object detection when records enter the table checks `rawLookup` first (0 QI if the
  exact `IInspectable*` pointer is repeated), then checks the new record's `Address` against
  `addressSet` (D5), so one COM object reached through two different `IInspectable*` pointers is
  also rejected. `addressSet` is maintained unconditionally alongside `rawLookup`: added on record
  creation, removed on retirement.

### D7. One shared projection index for flat sources

**Decided:** the engine's identity → projected index (#1) and the metadata provider's identity →
row index (#2) merge into one projection index, owned by the shared system.

- For flat sources both maps answer the same question over the same rows; today it is built twice,
  each time deriving identity again.
- Built from `projection[i]->Identity`. The engine keeps it current on incremental edits (as #1
  does today); the provider only reads it.
- The `m_flatRowIdentities` set goes away: membership is answered by `identityIndex` (D5).
- Grouped sources keep a provider-owned map, because the provider's row space includes group
  headers. Its data-row entries take identity from records.

### D8. Hierarchical sources: provider uses the adapter's index

**Decided:** for hierarchical sources, `RowMetadataProvider` delegates identity → row lookups to the
hierarchy adapter's visible index (#3) instead of building its own copy.

- Both maps cover the same visible rows with the same identities (the provider reads
  `NodeRow::Identity` from the adapter to build its copy).
- Grouped-hierarchy sources keep a provider-owned map, because the provider's row space includes
  group headers.

### D9. Types in TabularShaping; one shared-owned table instance in ShapedItemsSource

**Decided:** `SourceRecord` and the source-table type (`SourceRecordTable`: records, `rawLookup`,
`identityIndex`, projection index) are defined in `TabularShaping`, the lowest layer.
`ShapedItemsSource` owns the single instance via `std::shared_ptr<SourceRecordTable>`.

- Lower-layer structures (`ParentStructure`, `CustomSortRankAdapter`) can hold `SourceRecord*`, so
  #7 and #8 can migrate.
- `ShapedItemsSource` hands `std::shared_ptr<SourceRecordTable>` to the grouped and hierarchy
  adapters and, through `TableViewSource`, to `RowMetadataProvider` and selection. Sharing the
  table via `shared_ptr` ensures a `RowMetadataProvider` cached on `TableView` never dangles if it
  briefly outlives a `TableViewSource` detach or `ShapedItemsSource` teardown.
- **Teardown:** on source swap, detach or engine teardown, the engine retires every record (sets
  `Retired`, revokes subscriptions, clears the record vector and lookup maps) before releasing its
  reference, even though the table object may survive in a provider. A surviving provider then
  sees an empty table, and no app item is kept alive by it (T13).
- Fits the existing layering rule: `TabularShaping` depends on nothing above it.
- Rejected alternative: types in `ShapedItemsSource`. `TabularShaping` would stay record-unaware,
  so `ParentStructure` and ranks would keep their own per-row caches.

### D10. Records are shared-owned

**Decided:** the table holds `shared_ptr<SourceRecord>` (created with `make_shared`, so still one
allocation per record). Long-lived holders keep their records alive.

- **Long-lived holders:** a retained `ParentStructure` (structure reuse, plus the old index kept
  until publication), rank entries, and late callbacks can each outlive a Refresh. With shared
  ownership none of them can dangle.
- **Retirement vs destruction:** retiring a record removes it from the table and every lookup map
  and revokes its subscription. The object itself is freed when the last holder lets go.
- **T13 still holds:** a retired record that is still referenced also keeps its item alive, so the
  item's address cannot be recycled while any holder can see it.
- **Late callbacks** capture `weak_ptr<SourceRecord>` plus check `Retired` on the UI thread (D21);
  a retired-and-released record fails to lock, and a retired-but-still-referenced record is
  rejected by `Retired`.
- The projection inside `SourceRecordTable` keeps raw `SourceRecord*`: the table owns the
  `shared_ptr<SourceRecord>`s and therefore always outlives its own projection vector. Consumers
  that can outlive `ShapedItemsSource` (such as `RowMetadataProvider`) hold
  `shared_ptr<SourceRecordTable>` (D9, D14).
- **Cost:** refcount operations and a small control block per record.
- **Changes D3:** `unique_ptr` becomes `shared_ptr`.
- Rejected alternative: raw pointers with a strict retirement order (build new structure → publish
  → release old holders → retire records). No refcounts, but every path (Refresh, incremental edit,
  source swap, detach) must get the order right.

### D11. `ParentStructure` holds records

**Decided:**

```text
Rows            vector<IInspectable>        →  Records        vector<shared_ptr<SourceRecord>>
RowIdentities   vector<{Address,Identity}>  →  (removed; read from records)
IndexByAddress  ObjectAddress → position    →  IndexByRecord  SourceRecord* → position
Children (#9)   parent ObjectAddress → kids →  parent SourceRecord* → kids
NodeKeys, IndexByKey, ParentIndex, ChildIndices, RootIndices  (unchanged; app-key parent links)
```

- Item → node: `rawLookup` → record → `IndexByRecord`. No QI.
- Identity → node: `identityIndex` → record → `IndexByRecord`. No parsing of `"0x…"`, so it keeps
  working once identity is app-provided.
- Structure reuse compares record pointers (`structure.Records[i] == table[i]`), which is stricter
  than the current raw-ABI comparison.
- The structure holds `shared_ptr`s (D10), so a retained or not-yet-replaced structure never
  dangles.

### D12. Custom-sort ranks hold records

**Decided:**

```text
RankEntry       { shared_ptr<SourceRecord> Record; int32_t Rank; }
m_rankByRecord  SourceRecord* → rank          (replaces m_rankByIdentity, ObjectAddress-keyed)
KeyFor(item)    rawLookup → record → m_rankByRecord   (no QI, no weak resolve)
comparer        called with record->Item
```

- Removed: `WeakItem`, `StrongItem`, the per-entry `ObjectAddress`, and the T13 check that a weak
  reference still resolves to the same object.
- The existing retirement callback purges a record's entry when it leaves the source. A missed
  purge keeps a retired item alive (a leak, never a wrong rank).
- **Requirement:** an item's record exists before its sort key is read; every insert path creates
  the record first.
- Rejected alternative: `weak_ptr` entries. A missed purge couldn't leak, but every comparer pass
  and `KeyFor` would have to lock and handle failure.

### D13. Side maps are keyed by `SourceRecord*`

**Decided:** maps that stay with their owners (#6 root buckets, #7 ranks, #9 `Children`, #12 live
snapshots) use `SourceRecord*` as the key.

- One key convention everywhere; lookups come straight from `rawLookup` / `identityIndex` results,
  with no QI and no address.
- Same safety rule as address keys today: entries are purged when the record is retired, before its
  memory can be reused.
- `SourceRecord::Address` stays. It is still needed to form the address-derived identity, to detect
  one object reached through two different raw pointers (two items with the same address), and for
  the rare item-only lookup when `rawLookup` misses.

### D14. Metadata provider reads identity from the shared table

**Decided:** `TableViewSource` passes `std::shared_ptr<SourceRecordTable>` to `RowMetadataProvider`
instead of the item identity selector (`MakeIdentitySelector`). Holding a `shared_ptr` to the table
guarantees the provider's reads are safe even if `TableView` holds onto a cached provider across a
source detach. The provider derives no identity itself.

| Source kind | `GetIdentity(i)` | Identity → row |
|---|---|---|
| Unshaped flat | No `RowMetadataProvider` (`nullptr`, as today: `TableViewSource` creates none for an unshaped projection); `TableView` reads `table[i]` directly when needed | scan (D6) |
| Shaped flat | `projection[i]->Identity` | shared projection index (D7) |
| Grouped | header: group key (unchanged); data row: `rawLookup(item)->Identity` | provider-owned map, data rows from records |
| Hierarchical | `NodeRow::Identity` (descriptors filled from records) | adapter's index (D8) |
| Grouped-hierarchy | header: group key; data row: `NodeRow::Identity` | provider-owned map |

- `MakeIdentitySelector` and the engine's boxed `IdentitySelector()` bridge are no longer used for
  rows.
- Rejected alternatives: implementing `IKeyIndexMapping` on the rows views (public WinRT surface,
  and it changes `ItemsRepeater` recycling across a Reset); per-kind native accessors (more API
  than the shared table needs).

### D15. Selection and focus anchors stay object-based; callbacks pass records

**Decided:** `TableView`'s sticky/pending selection and pending focus anchors keep their current
form (item plus identity). The T13 retain/remove callbacks pass `SourceRecord`s instead of items,
so reconciliation compares record addresses without QI'ing each item. (D24 later replaces the
retain callback with per-record retirement.)

- An anchor follows the *object*, so it survives a Refresh even when the record was recreated (for
  example, the object moved and positional reuse missed it).
- Rejected alternative: anchors holding `weak_ptr<SourceRecord>`. Direct and QI-free, but a
  recreated record for the same object would lose the anchor, unlike today.

### D16. Identity is created lazily

**Decided:** `SourceRecord::Identity` is filled on first use, through one accessor, and then stored
in the record.

- Unshaped tables rarely ask for identity (today's `RebuildUnshapedRows` deliberately skips it), so
  they never pay for the `swprintf` and `hstring` allocation.
- The address-derived identity is fully determined by `Address`, so creating it later gives the
  same value.
- A future app-provided identity would call the app's selector on first use through the same
  accessor.
- Consumers never read the field directly; they go through the accessor.
- Building `identityIndex` (D5) or the projection index (D7) forces identity for the rows it
  covers.
- Rejected alternative: eager creation with the record. Simpler, but every row of every table pays
  for formatting, including tables that never shape.

### D17. `Reset` reuses records by position, then by raw pointer, then by address

**Decided:** after duplicate and null validation upfront (D2), reconciling a new materialization
with the previous source table on `ReloadFromSource()` runs in three steps, staged before retiring
unmatched old records so a validation failure leaves the previous table intact:

1. **By position:** if `get_abi(newItems[i]) == get_abi(previous[i]->Item)`, reuse that record. No
   QI.
2. **By raw pointer (`get_abi`),** for positions the first pass didn't match (for example, after an
   app-side sort or a batch insert/remove at the front that shifted indices and raised `Reset`):
   look up `get_abi(newItems[i])` in `rawLookup` (always maintained, D6). Reuse if found. **No QI.**
3. **By address,** only for positions that still missed both raw-pointer checks (genuinely new items,
   or rare COM objects that returned a different interface tear-off/wrapper on re-read): QI the item
   once (needed anyway for a new record) and look it up among any remaining unmatched previous
   records by `Address`. Reuse if found; otherwise create a new record.

Records left unmatched after all passes are retired once the new record list is validated.

- A moved or index-shifted object keeps its record without a QI, preserving its custom rank (D12),
  live snapshot, and subscription (D13). A reordering or batch-edit `Reset` does zero QIs for
  surviving items whose `IInspectable` pointer is unchanged.
- Duplicate and null items in `newItems` are rejected upfront with `E_INVALIDARG` (D2), so this
  reconciliation layer never handles duplicate occurrences.
- Rejected alternative: positional match only, or skipping raw-pointer match and going straight to
  address QI (which would QI every shifted or reordered item on a `Reset`).

### D18. Incremental source edits

**Decided:** every non-`Reset` collection change updates the source table at the notified index
first, regardless of which shaping verbs are active (flat, filtered, sorted, grouped, or
hierarchical).

| Action at source index `i` | Records | QI |
|---|---|---|
| **Add** | Validate non-null and non-duplicate (D2, D6) before mutating state; then create a record per new item, insert into the table at `i`, add to `rawLookup` and `addressSet` (and `identityIndex` if shaped). If live shaping is on, subscribe. The record exists before any sort key is read (D12). | 1 per new item |
| **Remove** | Retire `table[i]`: remove it from the lookup maps, revoke its subscription, call the owner callback with the record (D15), drop it from the projection. Freed when the last holder lets go (D10). | 0 |
| **Replace** | Same raw pointer → keep the record (self-replacement). Different raw pointer → QI the new item; same address → keep the record; otherwise retire the old record and create a new one. | 0 or 1 |
| **Move** | Move the handle from `i` to `j` in the table; the record is unchanged. | 0 |
| **Reset** | Reload from source (D17, D27). | Moved and new items only |

- Order within each action: update the table and maps, retire (purge before release), then either
  splice the projection in place (unfiltered flat/sorted fast paths) or run `Reshape()` from the
  updated source table (grouped, filtered, hierarchical, or multi-item edits) without
  re-enumerating the source (D27).
- If an index-only notification (`IVectorChangedEventArgs`) is out of range or `TryGetAt` fails,
  fall back to `ReloadFromSource()` (D27).
- The generation guards added in T13 still apply around every call into app code.

### D19. Measurement by timing benchmarks only

**Decided:** the improvement is measured with timing benchmarks only. No counters or test hooks are
added to the product.

- Benchmarks compare before and after for: Refresh with no source change, sort/filter/group
  changes, a reordering Reset, and single-item edits, at large row counts.
- Correctness is covered by the existing TableView API and interaction suites (640 cases at the
  time of writing) plus behaviour tests for the decisions that change behaviour (D1).
- Known limit: timing can show the gain but can't prove that a specific value was reused rather
  than recomputed; results are noisy and need repeated runs.

### D20. App-provided identity seam: fixed while in the source

**Decided (seam only, not built):** a future app-provided identity (for example, an `Id` property
read by a selector) must stay fixed while its item is in the source. To change it, the app replaces
the item or resets the source.

- **Why it would exist:** address identity loses selection and expansion whenever the app recreates
  objects (for example, reloading from a server). An app-provided identity lets a new object with
  the same `Id` inherit that state.
- **What the design already provides:** identity is read through one accessor (D16), and lookups,
  expansion and selection are keyed by identity, not by object (D5, D7, D15).
- **Contract violation:** if the app edits the identity in place, every stored copy (record,
  `identityIndex`, projection and hierarchy indexes, expansion intent, selection and focus anchors,
  `ContextIdentities`) stays on the old value. This is documented behaviour, not detected.
- **Duplicate identity:** two items in a shaped source with the same identity is an error, like a
  duplicate object today.
- **Replacement with the same identity:** the new object gets a new record (records follow objects);
  identity-keyed state carries over to it.
- **Possible later improvement:** on each Refresh, call the identity selector again for reused
  records and treat a changed value as retire + re-add. This costs one selector call per row per
  Refresh, needs no subscriptions, and fits the record layout without changes.
- Rejected alternative: watching `PropertyChanged` for the identity property and updating every
  stored copy. Always correct, but it needs a subscription on every row even with live shaping off,
  plus update logic in every place identity is stored.

### D21. Live-shaping callbacks capture a weak record handle

**Decided:** the `PropertyChanged` handler for a record captures `weak_ptr<SourceRecord>`. On the
UI thread it locks the handle; a failed lock or a set `Retired` flag means the callback is stale and
is ignored.

- **Why callbacks can be late:** a change raised on a worker thread is queued to the UI thread, and
  the record may be retired before the queued work runs. Revoking also doesn't stop a raise already
  in progress on another thread.
- **Why a `Retired` flag is needed:** a retired record can still be alive while a `ParentStructure`
  or rank entry holds it (D10), so a successful lock alone doesn't prove the record is current.
  Retirement sets the flag.
- **Threading rule:** lock only on the UI thread. A lock on a worker thread could take the last
  reference and destroy the record, releasing the app's item there. The worker-thread part of the
  handler only queues the weak handle.
- No lookup is needed to find the record; the snapshot (D13) is found by `SourceRecord*`.
- Replaces today's approach (QI the sender and check its address in the snapshot map), which no
  longer fits: snapshots are keyed by record and there is no address → record index (D5).
- Rejected alternative: a raw record pointer plus a subscription ID checked against the snapshot
  map. Works, and never dereferences a freed pointer, but needs an ID per snapshot entry and a
  lookup per callback.

### D22. Hierarchy expansion pruning checks records directly

**Decided:** when a new hierarchy structure is published, expansion intent (S1) is pruned by
checking each stored identity against the shared `identityIndex`, instead of first copying every
node identity into a temporary set.

- Today (`HierarchicalSourceAdapter.cpp`, `SetIndex`) each new structure builds a
  `liveIdentities` set of n `hstring`s just to call `RowExpansionModel::RetainOnly`.
- `identityIndex` already holds every source row, including filtered-out ones, which is exactly
  the complete live set pruning needs. Hierarchy is a shaping verb, so the index exists (D6).
- `RowExpansionModel::RetainOnly` takes a predicate (or the index) instead of a set; its keys and
  meaning are unchanged.
- S1 and S2 stay keyed by identity, so a future app-provided identity can carry expansion to a
  replacement object (D20).

### Grouping: deferred, not excluded

Grouping has the same kind of repetition, but it is more complex, so it is left for a follow-up
spec rather than ruled out of scope:

- **#10 duplicates #11.** The grouped adapter is created only by the engine, so its groups are
  always `ShapedGroup`s that already cache their identity. #10 rebuilds identity → group on every
  adapter Rebuild, and its ambiguous-key set can't fire because the engine rejects group-identity
  collisions. Removing it needs a layering answer: the adapter is meant to read any
  `ICollectionViewGroup` source, not the engine's cache.
- **Group identity is derived per item, per Refresh.** `BucketizeGroupsOrThrow` runs the group
  selector and canonicalizes the key for every row, and `CaptureLiveShapeSnapshot` repeats it with
  live shaping on. The value depends on the GroupBy verb, so it can't live in the source record; it
  would need a group-level equivalent with its own invalidation.

### D23. Hierarchy descriptors hold records

**Decided:** `HierarchicalSourceAdapter::NodeRow` (one per visible tree row, `m_descriptors`) holds
`SourceRecord*` for the row and for its parent, instead of its own copies of `Item`, `Address`,
`Identity` and `ParentIdentity`.

- Removes duplicated values and the `hstring` copies per visible row.
- Item, address and identity are read from the record; `ParentIdentity` is the parent record's
  identity. Depth, child count and sibling position stay in `NodeRow` (they depend on the current
  tree and filter).
- Records outlive the descriptors: the structure that produced them holds the records (D10, D11).

### D24. Consumers purge retired records instead of re-scanning the source

**Decided:** the engine reports exactly which records were retired, and each consumer purges only
those. The "retain against the full item list" passes are removed.

- **Today:** every Refresh hands the whole item list to `TableView::RetainSourceItemState` (ranks,
  sticky/pending selection, pending focus). Each consumer QIs every item and builds a set of live
  addresses. The S1 hierarchy prune and `LiveShapingTracker::RetainOnly` repeat the pattern.
- **After:** D17 already knows which records were left unmatched after a `Reset`, and D18 knows
  which record each Remove/Replace retires. The engine raises one notification per retired record
  (batched per `Reset`); `TableView` drops the rank entry and clears a selection or focus anchor
  whose address matches it. Because duplicates are rejected upfront (D2), no other record in the
  table can share that address.
- Replaces the T13 retain callback; the T13 remove callback becomes the single retirement path.
- D22 (hierarchy expansion pruning) stays as decided: expansion intent is keyed by identity and
  pruned against the complete `identityIndex`, not per retirement.

### D25. Finding a data item's row uses the shared lookups

**Decided:** `TableView::FindEntryIndexForDataItem` (`TableView_Sort.cpp`) stops QI'ing every row.

- **Today:** a linear scan that QIs the target and then every row in the view (about 2n QIs per
  call), used for example to restore selection after a re-sort.
- **Shaped flat:** `rawLookup` → record → shared projection index (D7), O(1). On a `rawLookup` miss
  (item reached through a different interface pointer), QI the target once and fall back to the
  scan below.
- **Other source kinds:** scan comparing each row's record address with the target's address
  (one QI for the target, none per row). Grouped and hierarchy rows reach their record through
  `rawLookup` / descriptors (D23).

### D26. Keyboard focus checks compare record addresses

**Decided:** the checks in `TableView_Keyboard.cpp` that confirm a row still shows the expected item
(pending body focus, expected item at an index) compare the row's record address with the expected
item's address, instead of QI'ing both sides on every check.

### D27. Authoritative source table: split `ReloadFromSource()` from `Reshape()`

**Decided:** the source record table is the authoritative in-memory snapshot of the source
collection. `Refresh()` splits into two distinct operations:

1. **`ReloadFromSource()`** — calls `Materialize(m_source)`, reconciles against the previous
   source record table via D17, updates lookup maps and live-shaping subscriptions, and then runs
   `Reshape()`. Called **only** on:
   - Initial `Start()`.
   - A collection `Reset` notification (or an untyped collection change with no action/index).
   - Fallback when an incremental notification is out of bounds against the source table or
     `IVectorChangedEventArgs` cannot read the item via `TryGetAt`.
2. **`Reshape()`** — runs the shaping pipeline (filter, sort, group, hierarchy) directly off the
   items already held in the source record table (`record->Item`), with **zero source reads or
   re-enumeration**. Called on:
   - Every shaping-verb change (`Filter`, `Sort`, `GroupBy`, `ParentBy`, `Clear*`) that cannot be
     satisfied by the in-place sort delta path.
   - Live-shaping restores (`PropertyChanged` on a tracked item).
   - Non-`Reset` collection changes (`Add`, `Remove`, `Replace`, `Move`) after updating the source
     record table via D18, whenever the projection cannot be spliced in place (for example, while
     grouped, filtered, or hierarchical).

- **Non-observable sources (TableView's own choice):** sources without `INotifyCollectionChanged`
  or `IObservableVector` (`IIterable`, plain `IList`/`IVector`) are snapshotted once on `Start()`
  and never re-enumerated on shaping changes. Mutating such a source without notifications is
  unsupported; the app reassigns `ItemsSource` to pick up changes.
  - This is **not** alignment with other collection views. WinUI's `InspectingDataSource` reads an
    `IVector`/`IBindableVector` live and copies only an `IIterable`; WPF's
    `ListCollectionView.Refresh()` re-reads its source.
  - **Behaviour change (release note):** today every shaping-verb change re-materializes the
    source, so an unnotified mutation shows up on the next sort/filter/group. After D27 it does
    not. TableView has no public `Refresh()`, so reassigning `ItemsSource` is the only way to pick
    it up. No existing test depends on the old behaviour.
- **Removed defensive source probes:** `TryReuseHierarchyStructure` no longer scans all rows to
  compare raw ABI pointers for silent mutations; comparing `m_hierarchyStructureSourceStamp ==
  m_sourceChangeStamp` is sufficient. Incremental paths no longer probe `m_sourceAccessor.Count()`
  against the source.

## Map migration

**Confirmed map by map (D1, D7–D15, D21–D27).** Numbering follows
[TableView-Row-Identity-and-Index-Maps.md](TableView-Row-Identity-and-Index-Maps.md).

| Map | Today | Proposal |
|---|---|---|
| #1 `m_flatRowIdentities` + `m_flatRowIdentityToIndex` | Identity set and identity → projected index, rederived from items | **Migrate (D7).** The set is replaced by `identityIndex`. The identity → projected index becomes the shared projection index, built from `projection[i]->Identity`. Its index check compares against the record's identity instead of rederiving it. |
| #2 `RowMetadataProvider::m_identityToIndex` | Identity → provider row, derived per row | **Migrate (D7).** Flat: uses the shared projection index. Grouped: keeps its own map (headers in its row space), data rows filled from records. Hierarchical: delegates to the adapter's index (D8). |
| #3 Hierarchy `m_indexByIdentity` | Identity → visible tree row | **Keep**, now also serving the provider (D8). Specific to the visible tree; descriptor identities come from records. |
| #5 `ParentStructure::IndexByKey` | App key → source position | **Keep.** Parent-link resolution (out of scope). |
| #6 `m_rootBucketIndex` | Address → group bucket | **Keep**, keyed by `SourceRecord*` (D13). |
| #7 `m_rankByIdentity` + `RankEntry` | Address → rank; entries hold weak/strong items | **Migrate (D12).** |
| #8 `ParentStructure::RowIdentities` + `IndexByAddress` | Per-row address/identity cache + address → position | **Migrate (D11).** |
| #9 `Children` + `ContextIdentities` | Parent address → children; context identities | **Keep**, `Children` keyed by parent `SourceRecord*` (D11); context identities read from records. |
| #10/#11 Group maps | Group identity | **Deferred** to a follow-up spec ([Grouping: deferred, not excluded](#grouping-deferred-not-excluded)). |
| #12 Live snapshots | Address → snapshot | **Keep**, keyed by `SourceRecord*` (D13). |
| #13 Live subscriptions | Address → subscription | **Remove.** Moves into the record. |
| #14 + S3 | Flat sort tracking | **Remove** (D1). |
| S1 / S2 expansion intent | Identity strings | **Keep**, keyed by identity (D20 carry-over). Hierarchy pruning of S1 checks `identityIndex` instead of building a temporary set (D22). |
| Selection/focus anchors (`TableView`) | Item + identity; reconciled by QI scan | **Keep** the anchors; purged per retired record, no QI scan (D15, D24). |
| Hierarchy `m_descriptors` (`NodeRow`) | Per visible row: own copy of item, address, identity, parent identity | **Migrate (D23):** holds row and parent `SourceRecord*`. |
| Retain passes (`RetainSourceItemState`, `LiveShapingTracker::RetainOnly`) | Full item list each Refresh; QI every item, build a live set | **Remove (D24):** consumers purge only retired records. |
| `FindEntryIndexForDataItem` | Linear scan, QI per row | **Migrate (D25):** shared lookups; address comparison when scanning. |
| Keyboard focus item checks | QI both sides per check | **Migrate (D26):** compare record addresses. |

## Open questions

All resolved. Kept for traceability:

1. ~~**Watch coverage** when sorted without live shaping~~ — resolved by D1: live shaping is the
   only subscriber and covers every source record.
2. ~~**Stale callback rejection**~~ — resolved by D21: weak record handle plus `Retired` flag,
   locked on the UI thread.
3. ~~**Duplicate objects in sources**~~ — resolved by D2: one record per unique object; duplicate
   object instances rejected upfront in all modes.
4. ~~**Storage and access**~~ — resolved by D3–D7, D9, and D14.
5. ~~**Identity creation**~~ — resolved by D16: lazily, through one accessor.
6. ~~**Incremental edits**~~ — resolved by D18 and D27.
7. ~~**Sorters and filters**~~ — resolved by D4: they keep receiving bare items.
8. ~~**`Reset` reuse beyond position**~~ — resolved by D17: by position, then by raw pointer, then
   by address.
9. ~~**App-provided identity seam**~~ — resolved by D20: fixed while in the source.
10. ~~**Measurement and tests**~~ — resolved by D19: timing benchmarks only.
