# TableViewSource2 header and IDL changes during implementation

This log tracks every non-comment change made to a header (`.h`) or IDL (`.idl`) file after the
reviewed TableViewSource2 design was committed (baseline `5f6617e792`, "TableViewSource2: review
fixes for headers and spec"). Comment-only edits are not listed.

Add an entry whenever an implementation commit changes a declaration, signature, member, base
class, include or inline definition in a header or IDL. Each entry gives the change, the reason,
and the commit that made it.

## IDL

No IDL changes so far.

## Headers

### `controls/dev/TableViewSource2/View/ShapedCollectionView.h`

| Change | Reason | Commit |
|---|---|---|
| No longer derives from `ItemsSourceView` (dropped `#include "ItemsSourceView.h"` and `ForwardRefToBaseReferenceTracker`). Now `winrt::implements<ShapedCollectionView, IBindableVector, IBindableVectorView, IBindableIterable, INotifyCollectionChanged, IKeyIndexMapping>`. | `ItemsSourceView`'s implementation type lives in the Controls DLL; TableViewSource2 builds into the Tabular DLL and cannot override its `*Core` virtuals across the DLL boundary. Option A kept (wrap with the public `winrt::ItemsSourceView{ collection }` constructor); alternatives (implement `IItemsSourceView` natively, move the view into the Controls DLL) were considered and deferred. | `8ebf18bcbe` (M1) |
| `ItemsSourceView` overrides replaced by interface members: `GetSizeCore` → `uint32_t Size()`, `GetAtCore(int)` → `GetAt(uint32_t)`, `IndexOfCore` → `bool IndexOf(value, uint32_t&)`, `KeyFromIndexCore` → `KeyFromIndex(int32_t)`, `IndexFromKeyCore` → `int32_t IndexFromKey(hstring)`. `HasKeyIndexMappingCore` removed (implied by implementing `IKeyIndexMapping`). | Follows the base-class change. | `8ebf18bcbe` (M1) |
| Added `GetView()`, `First()`, the read-only mutators `SetAt`, `InsertAt`, `RemoveAt`, `Append`, `RemoveAtEnd`, `Clear` (each throws `E_ILLEGAL_METHOD_CALL`), and the `CollectionChanged` add/remove pair. | Required by `IBindableVector` / `IBindableIterable` / `INotifyCollectionChanged`. | `8ebf18bcbe` (M1) |
| Constructor is now `noexcept`. | It only stores the back-reference. | `8ebf18bcbe` (M1) |
| Added private `Raise(args)` and `winrt::event<NotifyCollectionChangedEventHandler> m_collectionChanged`. | Event storage previously came from the `ItemsSourceView` base. | `8ebf18bcbe` (M1) |

### `controls/dev/TableViewSource2/View/ShapedRowQueries.h`

| Change | Reason | Commit |
|---|---|---|
| Added `winrt::hstring KeyAt(uint32_t index) const` and `static winrt::hstring KeyOf(ShapedNode const& node)`. | One row-key helper shared by `ShapedCollectionView::KeyFromIndex` and `ShapedRowMetadata::GetIdentity`, so the two key spaces cannot drift. | `8ebf18bcbe` (M1) |

### `controls/dev/TableViewSource2/Shaping/ShapingManager.h`

| Change | Reason | Commit |
|---|---|---|
| `View()` returns `winrt::Microsoft::UI::Xaml::Interop::IBindableVector` instead of `winrt::ItemsSourceView`; `m_view` is `std::optional<tracker_ref<IBindableVector>>`. | Follows the `ShapedCollectionView` base-class change; `TableViewSource2` does the `ItemsSourceView` wrap. | `8ebf18bcbe` (M1) |
| Added `ShapedCollectionView* m_viewImpl`. | Lets the `noexcept` tree listener reach the implementation without a QI. | `987774dc29` (M1) |
| Added private forward declaration `class CommitScope;`. | RAII scope around `BeginCommit` / `EndCommit`, defined in the `.cpp`. | `987774dc29` (M1) |
| Added `void SetCommitEndedHandler(std::function<void()>) noexcept`, `bool IsApplying() const noexcept` and member `std::function<void()> m_commitEnded`. `SetOwner(nullptr)` no longer throws during a commit. | Lets an owner release from inside its own callbacks; owner-released waiters run at commit end. | `6cc4e48ed4` (M1) |
| Removed `bool m_resyncPosted`. | The posted retry now goes through `SourceTable::PostReread`, guarded by its `m_rereadPosted`. | `db396c693c` (M2) |

### `controls/dev/TableViewSource2/Source/SourceTable.h`

| Change | Reason | Commit |
|---|---|---|
| `void PostReread() noexcept` moved from private to public. | `ShapingManager::ResyncSource` uses it for its posted retry. | `b3ae0903cc` (M2) |
| Added private `void PostRereadIfOwed() noexcept`. | `Refresh()` epilogue: posts a re-read owed by its own dispatch unless a freeze or hold will report it. | `b3ae0903cc` (M2) |
| `SubscribeRecordPropertyChanged` is now `noexcept`. | A subscribe failure must not unwind a commit and leave freed records in the lookups; the item is marked not observable instead. | `5f3bba7ebc` (M2) |
| Added private `void RaiseEdit(SourceChangeKind, uint32_t, uint32_t, std::vector<SourceRecord*> const&, std::vector<SourceRecordPtr>&)`. | The edits' `RaiseSourceChanged`; a listener throw still posts any owed re-read before rethrowing. | `f9f7b0326c` (M2) |
| Added private `static void ThrowIfOffBoundThread(ThreadBinding const&)`. | A bound table rejects a root collection notification raised on another thread with `hresult_wrong_thread`. | `f9f7b0326c` (M2) |

### `controls/dev/TableViewSource2/TableViewSource2.h`

| Change | Reason | Commit |
|---|---|---|
| Added `#include <optional>`, internal `winrt::ItemsSourceView GetItemsSourceView()`, and cache members `std::optional<tracker_ref<winrt::ItemsSourceView>> m_itemsSourceView` and `void* m_itemsSourceViewCollection`. | Wraps `ShapingManager::View()` once in a `winrt::ItemsSourceView` for TableView's rows repeater, as v1 `TableViewSource::GetItemsSourceView()`. | `987774dc29` (M1) |
| Added private `void RunOwnerReleasedWaiters()`. `TrySetOwner` no longer removes the claiming owner's waiter (the caller does). | A release during a commit defers waiters to the commit's end. | `6cc4e48ed4` (M1) |

### `controls/dev/TableViewSource2/Source/SourceRecord.h`

| Change | Reason | Commit |
|---|---|---|
| Added `static ObjectAddress AddressOf(IInspectable const& item)`. | Canonical COM identity (the `IUnknown` address) for lookups when a managed item arrives through a different interface pointer. | `dbc625e395` (M1) |

### `controls/dev/TableViewSource2/Engine/FenwickTree.h`

| Change | Reason | Commit |
|---|---|---|
| Added the inline definition of `template <typename CountOf> void FenwickTree::Build(uint32_t, CountOf&&)` (linear O(n) build). | Template body must live in the header. | `4dfb4995a8` (M1) |

### `controls/dev/TableView/TableView.h`

| Change | Reason | Commit |
|---|---|---|
| Added `#include "Shaping/IShapingOwner.h"`, `tracker_ref<winrt::TableViewSource2> m_activeSource2`, nested `struct SourceOwnerSink final : IShapingOwner` (empty callbacks) and member `m_sourceOwnerSink`. | TableView accepts a `TableViewSource2` as `ItemsSource` and registers itself as its owner. | `1e83acaf9c` (M1) |
| Added `bool m_activeSource2Released`, `winrt::weak_ref<winrt::TableViewSource2> m_awaitedSource2`, `void StopAwaitingSource2()` and `void AdoptTableViewSource2(winrt::TableViewSource2)`. | Owner arbitration for a shared `TableViewSource2`, mirroring the v1 `m_activeSource` members. | `1e83acaf9c` (M1) |
