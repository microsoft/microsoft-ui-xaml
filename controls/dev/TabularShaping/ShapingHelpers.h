// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

namespace ShapingHelpers
{
    // A group object that already knows its own stable identity.
    //
    // The key OBJECT is not a usable identity on its own. A key selector run twice can return
    // equal-but-not-identical objects, and a reference-typed key has no value form at all, so
    // re-deriving an identity from the key either flips it between shapes or yields nothing. A
    // group that minted an identity when it was built publishes it here, and consumers read it
    // instead of guessing.
    //
    // This lives in layer 1 because the layer that implements it and the layer that reads it are
    // siblings: neither may include the other's headers, so the shared contract has to sit below
    // both.
    struct __declspec(uuid("E74C4DC8-3FAC-4A24-AD34-01547B4672DF")) IGroupIdentity : ::IUnknown
    {
        virtual winrt::hstring StableGroupIdentity() const = 0;
    };

    struct ValueKey
    {
        static bool TryFormatPropertyValue(
            winrt::Windows::Foundation::IPropertyValue const& propertyValue,
            winrt::hstring& key,
            bool rejectEmptyString = false);

        static bool TryGetStablePropertyKey(
            winrt::IInspectable const& value,
            winrt::hstring& key,
            bool rejectEmptyString = false);

        static winrt::hstring ToString(winrt::IInspectable const& value);
        static winrt::hstring ToObjectLookupKey(winrt::IInspectable const& value, bool rejectEmptyString = false);
    };

    struct ValueComparer
    {
        static int Compare(winrt::IInspectable const& a, winrt::IInspectable const& b);

        // Overload for callers that can hoist the reference-key tiebreak string out of the
        // comparison loop. `ValueKey::ToString` may call into app code (IStringable), so
        // recomputing it per comparison is both O(n log n) app calls and a strict-weak-ordering
        // hazard when the app's ToString is not deterministic. Pass nullptr to compute on demand.
        static int Compare(
            winrt::IInspectable const& a,
            winrt::IInspectable const& b,
            winrt::hstring const* fallbackKeyA,
            winrt::hstring const* fallbackKeyB);

        // True when Compare would fall through to the ToString-based tiebreak for this value,
        // i.e. it is a non-null, non-IPropertyValue reference.
        static bool UsesFallbackKey(winrt::IInspectable const& value);
    };

    std::vector<winrt::IInspectable> EnumerateInspectableItems(
        winrt::IInspectable const& source,
        bool throwIfUnsupported = false);

    // Resolves ONCE what a collection is and how to read it.
    //
    // A XAML items source can arrive as any of half a dozen interfaces, and the stack needs three
    // different things from it: read it by index, enumerate all of it, and observe it changing.
    // Asking those questions with a separate try_as ladder at each call site is how the ladders
    // drift: one of them omitted IVectorView, so for an IVectorView-only source the count answered
    // while the indexed read refused, and the incremental path silently degraded. Resolving the
    // interface once and reading every answer off the same resolution is what makes the answers
    // agree by construction.
    //
    // This lives in layer 1 because both layer 2 (the live projection) and layer 3 (grouping, which
    // classifies each group's items) need it, and siblings may not include each other's headers.
    class CollectionAccessor
    {
    public:
        CollectionAccessor() = default;
        explicit CollectionAccessor(winrt::IInspectable const& source);

        // True when Count/GetAt are a direct call rather than an enumeration. An enumerable-only
        // source reports false; callers that need indexed access degrade instead of materializing
        // behind the caller's back.
        bool IsIndexable() const noexcept;
        // True when the source publishes changes through any supported notification interface.
        bool IsObservable() const noexcept { return m_notifyCollectionChanged || m_observableVector || m_bindableObservableVector; }

        // Read through to the live collection rather than a value captured at construction: this
        // classifies a source that goes on mutating, so a cached count would go stale.
        uint32_t Count() const;
        bool TryGetAt(uint32_t index, winrt::IInspectable& item) const;
        std::vector<winrt::IInspectable> Enumerate(bool throwIfUnsupported = false) const;

        // The resolved notification interfaces, so a subscriber binds to one without re-probing.
        // Precedence is the caller's: INotifyCollectionChanged carries per-change detail, the
        // vector interfaces carry an index and a verb.
        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged const& AsNotifyCollectionChanged() const noexcept { return m_notifyCollectionChanged; }
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> const& AsObservableVector() const noexcept { return m_observableVector; }
        winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector const& AsBindableObservableVector() const noexcept { return m_bindableObservableVector; }

    private:
        // The indexed read itself, with no bounds check. Enumerate() has already established the
        // range and would otherwise pay a Size() call per item to re-derive it.
        winrt::IInspectable GetAtUnchecked(uint32_t index) const;

        winrt::IInspectable m_source{ nullptr };

        // Read: at most one of these is set, in IVector > IVectorView > IBindableVector order.
        winrt::Windows::Foundation::Collections::IVector<winrt::IInspectable> m_vector{ nullptr };
        winrt::Windows::Foundation::Collections::IVectorView<winrt::IInspectable> m_vectorView{ nullptr };
        winrt::Microsoft::UI::Xaml::Interop::IBindableVector m_bindableVector{ nullptr };

        // Observe: independent of the read resolution, since a source can be indexable through one
        // interface and observable through another.
        winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged m_notifyCollectionChanged{ nullptr };
        winrt::Windows::Foundation::Collections::IObservableVector<winrt::IInspectable> m_observableVector{ nullptr };
        winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector m_bindableObservableVector{ nullptr };
    };

    // Canonical predicate filter shared by Tabular shaping engines. Retains items for which
    // predicate(item) returns true; a predicate that throws is treated as "exclude", which is the
    // fail-safe every caller wants: a broken predicate hides rows rather than failing the shape. In-place; preserves relative order of kept items.
    void ApplyPredicateFilter(
        std::vector<winrt::IInspectable>& items,
        std::function<bool(winrt::IInspectable const& item)> const& predicate);

    // Canonical multi-axis stable sort shared by every Tabular shaping engine.
    // The caller supplies key extraction
    // (item, axisIndex) -> key and the per-axis sort direction; the routine performs a
    // decorate-sort-undecorate stable sort keyed by ValueComparer, which guarantees a
    // strict-weak-ordering. Extracting key selection into a functor lets a delegate-based and a
    // property-name/reflection-based engine share one sort implementation.
    void StableSortByKeys(
        std::vector<winrt::IInspectable>& items,
        size_t axisCount,
        std::function<winrt::IInspectable(winrt::IInspectable const& item, size_t axisIndex)> const& extractKey,
        std::function<winrt::SortDirection(size_t axisIndex)> const& axisDirection);

    // One ordered group bucket carrying its representative key + identity, produced by
    // BucketizeToGroups. The Key is retained (not just the identity string) so grouping adapters
    // can drive per-group object caches and collision policy that need the original key instance.
    struct KeyedBucket
    {
        winrt::IInspectable Key{ nullptr };
        winrt::hstring Identity;
        std::vector<winrt::IInspectable> Items;
    };

    // Canonical keyed group-bucketization shared by grouping adapters. Walks `items` once,
    // resolving each item's group key and stable identity string, bucketizing by identity while
    // preserving first-seen group order (items keep input order within a group). Returns the
    // buckets WITH their keys so the adapter can keep its own
    // identity/collision policy and group-object cache Pure — no XAML/dispatcher — so it is headless-testable.
    //
    // Callbacks (all supplied by the adapter, which owns the WinRT-coupled policy):
    //   resolveKey(item)            -> the group key (adapter wraps its selector incl. throw->null).
    //   resolveIdentity(key,id,why) -> false signals the caller MUST degrade to a flat projection
    //                                  (e.g. unstable/unresolvable identity); *why is a static reason.
    //   keysConsideredEqual(a,b)    -> false on a genuine identity COLLISION (same identity string,
    //                                  logically-different keys) which also forces a flat degrade.
    // Returns true with outBuckets populated (first-seen order); returns false and sets
    // degradeReason when the adapter must fall back to flat.
    bool BucketizeToGroups(
        std::vector<winrt::IInspectable> const& items,
        std::function<winrt::IInspectable(winrt::IInspectable const& item)> const& resolveKey,
        std::function<bool(winrt::IInspectable const& key, winrt::hstring& identity, wchar_t const*& reason)> const& resolveIdentity,
        std::function<bool(winrt::IInspectable const& existingKey, winrt::IInspectable const& newKey)> const& keysConsideredEqual,
        std::vector<KeyedBucket>& outBuckets,
        wchar_t const*& degradeReason);

    // Upper-bound insertion index shared by the incremental fast-paths. Given an already-sorted
    // range of `count` items, returns the position where a newly-arrived item should be inserted
    // so it lands AFTER any equal-key items — matching the append order a stable_sort produces for
    // a new row. compareNewToExisting(i) compares the NEW item against the existing item at index i
    // (< 0 when the new item sorts strictly before it, >= 0 otherwise). Pure binary search — no
    // XAML/collection type — so every incremental fast path can share it and it
    // is directly headless-unit-testable.
    uint32_t UpperBoundInsertIndex(
        uint32_t count,
        std::function<int(uint32_t existingIndex)> const& compareNewToExisting);

    // A pairwise ordering supplied by app code: returns < 0, 0, or > 0. The engine never hands one
    // to a std:: sort - it is untrusted and may be non-transitive, throwing, or reentrant, any of
    // which is undefined behaviour inside optimized sort internals. CustomSortRankAdapter is the
    // one place that ever invokes it, and it does so defensively.
    using PairwiseComparer =
        std::function<int(winrt::IInspectable const& a, winrt::IInspectable const& b)>;

    // Adapts an untrusted pairwise comparer into stable integer sort keys, so the key-based engine
    // can order a comparer column as an ordinary axis instead of ever touching the comparer. The
    // comparer is run ONCE over the rows and each item's ordinal becomes its key; every later
    // reshape reads the frozen integer keys. Memory-safe under ANY comparer: a non-transitive,
    // throwing, or reentrant comparer yields a wrong-but-valid order, never framework corruption.
    //
    // This is the rank-system adapter that used to live in the control. It stays out of the pure
    // key engine proper (StableSortByKeys never sees a comparer) but belongs in the shaping
    // substrate rather than in TableView, so any key-based consumer can reduce a pairwise comparer
    // to a sort key the same way.
    class CustomSortRankAdapter
    {
    public:
        // Runs `comparer` once over `rows` through a memory-safe stable merge sort and freezes
        // dense integer ranks. Equal items share a rank, so the projection imposes no order on a
        // tie the comparer called equal. Replaces any prior ranking and adopts `comparer` as the
        // one used to place late-arriving rows.
        void Rank(PairwiseComparer const& comparer, std::vector<winrt::IInspectable> const& rows);

        // The sort key for an item, boxed as int32. O(1) for a row the rank pass saw; a row added
        // afterwards is located among the existing ranks by the comparer and inserted, shifting the
        // ranks above it. Returns nullptr when no comparer is set, or when the state was cleared
        // underneath a reentrant call.
        winrt::IInspectable KeyFor(winrt::IInspectable const& item);

        // Drops the comparer and all ranks. The adapter object itself is retained so a key selector
        // that closed over it by shared_ptr stays valid.
        void Reset();

        bool HasComparer() const noexcept { return static_cast<bool>(m_comparer); }

    private:
        struct RankEntry
        {
            winrt::IInspectable Item{ nullptr };
            int32_t Rank{};
        };

        // App code, so it never escapes: a throwing comparer degrades to "equal", which keeps the
        // merge stable rather than random. Also normalizes the result to -1 / 0 / 1.
        int32_t SafeCompare(winrt::IInspectable const& left, winrt::IInspectable const& right) const;

        // Orders two row indices under the comparer, breaking ties by source index so the sort is
        // stable. Sets staleState and returns false when the ranks were cleared mid-merge (a
        // reentrant comparer callback).
        bool IndexComesBefore(
            std::vector<winrt::IInspectable> const& rows,
            uint64_t generation,
            size_t leftIndex,
            size_t rightIndex,
            bool& staleState) const;

        // Deliberately not std::stable_sort: that would hand an app comparer to optimized library
        // internals where a non-strict-weak-ordering is UB. Every access here is loop-bound, so a
        // bad comparer can only produce a strange order, never memory corruption, at the same
        // O(n log n) comparer calls. Returns false when the ranks were cleared mid-merge.
        bool StableMergeSortOrder(
            std::vector<size_t>& order,
            std::vector<winrt::IInspectable> const& rows,
            uint64_t generation) const;

        // Bumped every time the ranks are cleared. A comparer callback can synchronously re-enter
        // and clear this state, so anything that resumes after app code checks the generation
        // before trusting ranks written for a pass that is no longer current.
        void ClearRanks();

        PairwiseComparer m_comparer{ nullptr };
        std::vector<RankEntry> m_ranks;
        // O(1) identity -> rank lookup for the common (reference-type item) case; falls back to the
        // comparer scan on a miss, e.g. a boxed value type whose CCW churned.
        std::unordered_map<void*, int32_t> m_rankByIdentity;
        uint64_t m_generation{ 0 };
        // Reentrancy guard for the app comparer. Both rank paths iterate m_ranks and invoke the
        // comparer inside that iteration; a comparer that re-enters lands in the same vector and
        // reallocates it under the outer loop's iterators - UB, in practice a crash. A reentrant
        // comparer is an app bug (a sort predicate must be a pure function of its inputs), so fail
        // fast in chk and no-op safely in fre. WinUI is single-threaded, so a plain bool suffices.
        bool m_comparerActive{ false };
    };

}
