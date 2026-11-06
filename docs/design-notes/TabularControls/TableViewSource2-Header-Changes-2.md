# TableViewSource2 header changes and design drift: fresh implementation

This log covers the fresh implementation on branch `user/dipesh/tableview/tvs-3-implementation-2`,
which starts again from the reviewed design (baseline `5f6617e792`, "TableViewSource2: review fixes
for headers and spec"). It lists only what is implemented on this branch.
`TableViewSource2-Header-Changes.md` is the log of the first implementation
(`user/dipesh/tableview/tvs-3-implementation`); it is kept unchanged as a reference, and its row
numbers are reused here so entries can be traced back.

Implemented so far: `CollectionAccessor2`, `FenwickTree` and `ShapedNode`, `SourceRecord`, the
flat `SourceTable`, and the shape spec (`ShapeSpec`, `ShapeDelegates`, `PathEvaluator`).

The **First implementation** column says how each entry compares with the first implementation:

- **Same**: the same decision or change as the first implementation's row of the same number.
- **Differs**: the first implementation made this change or decision differently; the cell says how.
- **New**: the first implementation has no such entry.

"pending" in the Commit column means not yet committed.

## Decisions that drift from the spec or headers

The spec (`Shaping-Architecture-Redesign.md`) is frozen. Every decision that departs from it, or
from a declaration in the reviewed headers, or that settles behaviour the spec leaves open, is
recorded here.

| # | Spec / header says | Implementation does | Why | First implementation | Commit |
|---|---|---|---|---|---|
| 4 | Spec is silent on a hold ending inside a freeze. | `EndHoldResets` performs its deferred reset only when no freeze is open (`m_freezeDepth == 0`); otherwise `EndFreeze` reports it. | Symmetric with `EndFreeze`; avoids a reset raised mid-freeze. | Same | 7da9913263 |
| 5 | Global constraint: app exceptions propagate. | `SubscribeRecordPropertyChanged` is `noexcept`: a subscribe failure marks the item not observable, with no retry. | A throw inside a commit would unwind with freed records left in the lookups. Cost: that item's property edits are not live-shaped. | Same | 7da9913263 |
| 6 | Spec covers off-thread item `PropertyChanged` (marshalled) but not an off-thread root collection notification. | A bound table throws `hresult_wrong_thread` (and asserts in debug) for a root `CollectionChanged` / `VectorChanged` raised on another thread, before taking any lock. | TableViewSource2 builds no shadow copy of the source. WPF can accept off-thread changes only with `EnableCollectionSynchronization`, which marshals them against a shadow collection under the app's lock; without one, applying an index-based change after marshalling would apply it against a collection that has moved on. | Same behaviour. The first implementation's reason ("matches XAML `ItemsControl` behaviour") is replaced by the shadow-copy reason. | 7da9913263 |
| 7 | Spec is silent on a failed posted re-read. | The posted re-read callback does not catch: a `Refresh` throw propagates to the dispatcher (`Application.UnhandledException`). The flags follow `Refresh()`'s failure rules, and the callback does not re-post. | A swallowed failure would leave the table silently out of sync. Posting is kept so a `Reset` handler that edits the source cannot loop synchronously. | Differs: the first implementation swallowed the throw and did not re-post, leaving the table owed until the next notification. | 7da9913263 |
| 74 | Storage (decided, B4): one tracked `CollectionAccessor2` per root source plus one per ChildFeed; the header listed `ChildCollectionsHierarchy::m_trackerOwner` for the accessor `CollectIncoming()` enumerates through. | `CollectionAccessor2` has a static `Enumerate(source, throwIfUnsupported)`: it resolves the read interface with the same precedence, the same CLR `IBindableIterable` retry and the same errors as the tracked accessor, and holds nothing. The member enumeration shares its body. Not implemented yet: its ChildrenBy callers and the removal of `ChildCollectionsHierarchy::m_trackerOwner` (they come with `ChildrenBy`). | A throwaway tracked accessor would set and destroy two tracker handles per node. Enumeration semantics are unchanged. | Same for the accessor part. | 147963955c |
| 76 | `DelegateId` is `uint32_t`; `DelegateRef::Version` and the `ShapeDelegates` counters are `uint32_t`. | `DelegateId`, `DelegateRef::Version`, `m_nextId`, `m_nextVersion` and `NextVersion()` are `uint64_t`. Ids and Versions never wrap. | A `uint32_t` id could wrap in a long-running app that passes a fresh lambda on every verb, and a new delegate could then reuse an id still in use. `DelegateRef` grows from 8 to 16 bytes. | New: the first implementation kept `uint32_t` and ignored overflow. | pending |
| 77 | `ShapeDelegates::Retain` and `PathEvaluator::Retain` release what no spec in use refers to; the spec does not say whether they may fail. | Both scan the specs in place without allocating, so neither can throw after an apply has committed. `ShapeDelegates::Retain` erases the address map first (no app code), then releases unused entries one at a time, restarting the scan after each; only ids below `m_nextId` at entry are candidates, so a delegate registered by app code during a release is kept. `PathEvaluator::Retain` stops at the first matching path level. | A throw from `Retain` would make a verb report failure after its rows had changed. Releasing a delegate can run app code, which must not invalidate the scan. The registry and spec lists hold a handful of entries, so the nested scan is cheaper than a hash set. | New: the first implementation built an `unordered_set` of used ids (which can throw) and erased while iterating; its `PathEvaluator::Retain` had no early exit. | pending |
| 78 | Spec: a path that cannot be evaluated gives a `null` key, no throw; `PathEvaluator` is silent on re-entrancy. | `PathEvaluator::Value` guards re-entrancy with `m_isResolving`: a nested `Value` gives `nullptr`, `Retain` does nothing, and `Clear` is deferred (`m_clearPending`) to the end of the outermost `Value`. Allocation failures (a resolver for a new path) propagate. | `Resolve` runs app code (property getters) through the binding engine. A nested read on the same path would overwrite and then clear the resolver's probe `DataContext`, so the outer read would return the wrong key. Keys are read in the engine's compute phase, so an allocation failure discards the unit of work cleanly. | New: the first implementation had no re-entrancy guard. | pending |
| 79 | Spec is silent on `ShapeDelegates` invocation with an id that has no entry, or an entry of another kind, and on `Register` failures. | `Passes`, `Key` and `Compare` assert, then throw `E_UNEXPECTED`. `Register` may throw (`QueryInterface`, allocation); it builds the `tracker_ref` before touching either map and undoes the entry if the address insert throws. The verb calls `Register` before it edits the desired spec. | A bad id is an internal bug; throwing takes the same compute-before-commit path as a throwing app delegate. Registering first keeps the desired spec unchanged on failure; an entry orphaned by a later failure in the verb is released by the next `Retain`. | New entry. The bad-id rule is the same as the first implementation; the `Register` undo differs (the first implementation reserved both maps, which does not cover node allocation). | pending |

## Headers

Non-comment changes to headers since the baseline. Comment-only edits are not listed.

### `controls/dev/TableViewSource2/Source/CollectionAccessor2.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| Added public `static std::vector<IInspectable> Enumerate(IInspectable const& source, bool throwIfUnsupported)`; private `static ReadKind ResolveRead(IInspectable const&, IInspectable&)` and `static std::vector<IInspectable> EnumerateResolved(ReadKind, IInspectable const&, bool)`; removed private `GetAtUnchecked(uint32_t) const` (now a file-local helper over the resolved read interface). | One-shot enumeration without tracker handles; the member `Enumerate` shares the body (drift #74). | Same | 147963955c |

### `controls/dev/TableViewSource2/Engine/FenwickTree.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| Added the inline definition of `template <typename CountOf> void FenwickTree::Build(uint32_t, CountOf&&)` (linear O(n) build). | Template body must live in the header. | Same | aab9218bea |

### `controls/dev/TableViewSource2/Source/SourceRecord.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| Added `static ObjectAddress AddressOf(IInspectable const& item)`. | Canonical COM identity (the `IUnknown` address) for lookups when a managed item arrives through a different interface pointer. | Same | abfc580414 |

### `controls/dev/TableViewSource2/Source/SourceTable.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| `void PostReread() noexcept` moved from private to public. | `ShapingManager::ResyncSource` uses it for its posted retry. | Same | 7da9913263 |
| Added private `void PostRereadIfOwed() noexcept`. | `Refresh()` epilogue and `RaiseSourceChanged`'s listener-fault path: posts a re-read owed by its own dispatch unless a freeze or hold will report it. | Same | 7da9913263 |
| `SubscribeRecordPropertyChanged` is now `noexcept`. | A subscribe failure must not unwind a commit and leave freed records in the lookups; the item is marked not observable instead (drift #5). | Same | 7da9913263 |
| Added private `static void ThrowIfOffBoundThread(ThreadBinding const&)`. | A bound table rejects a root collection notification raised on another thread with `hresult_wrong_thread` (drift #6). | Same | 7da9913263 |
| Removed `struct PrivateToken` and the public `SourceTable(PrivateToken, ITrackerHandleManager const*)`; added private `explicit SourceTable(ITrackerHandleManager const* owner)`. | `Create()` owns the result through `std::shared_ptr(new ...)`, so the constructor can be private: two allocations instead of `make_shared`'s one, once per table. | New: the first implementation kept `PrivateToken` and `make_shared`. | 7da9913263 |

### `controls/dev/TableViewSource2/Spec/ShapeSpec.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| `enum class DelegateId` underlying type is `uint64_t` (was `uint32_t`); `DelegateRef::Version` is `uint64_t` (was `uint32_t`). | Ids and Versions never wrap (drift #76). | New | pending |

### `controls/dev/TableViewSource2/Spec/ShapeDelegates.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| `NextVersion()` returns `uint64_t`; `m_nextId` and `m_nextVersion` are `uint64_t` (were `uint32_t`). | Drift #76. | New | pending |

### `controls/dev/TableViewSource2/Spec/PathEvaluator.h`

| Change | Reason | First implementation | Commit |
|---|---|---|---|
| Added private members `bool m_isResolving` and `bool m_clearPending`. | Re-entrancy guard for `Value`, `Retain` and `Clear` (drift #78). | New | pending |
