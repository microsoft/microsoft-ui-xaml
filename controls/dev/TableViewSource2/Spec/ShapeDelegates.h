// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>

#include <winrt/Windows.Foundation.h>

#include "ShapeSpec.h"

// Registry of the app delegates a ShapeSpec refers to (declarations only).
// See docs/design-notes/TabularControls/Shaping-Architecture-Redesign.md, "5. Shape spec".
//
// The spec says WHAT is applied by DelegateId; this registry holds HOW. It is the only place in the
// shape spec layer that holds app objects, so it is the only place with tracker_ref and teardown
// rules.
//
// Identity: delegates are deduplicated by their IUnknown address. Registering the same delegate
// object again returns the same DelegateId, which is how a repeated Sort(key) or Sort(comparer)
// call is matched to its existing level. Two separately created lambdas are different objects and
// get different ids, even with identical code. Every Register call also returns a new Version, so
// a re-declared delegate still reads as a change in the spec.
//
// Lifetime: DelegateId is a plain value and does not keep its delegate alive. Retain() is called
// with every spec still in use (the applied one and the desired one); anything
// they no longer refer to is released. So a delegate the applied spec still uses is never released
// while a newer spec is being built.
//
// Paths: a SortSpec of kind Path has no delegate and no entry here; PathEvaluator reads its keys.
//
// Reference tracking: m_trackerOwner points to the owning TableViewSource2. Every delegate goes
// through tracker_ref bound to it. ~tracker_ref dereferences the owner, so entries are
// std::optional and Clear() destroys them while the owner is alive; teardown uses safe_get only.
//
// Threading: UI-thread affine, like SourceTable (tracker_ref and app delegates).
class ShapeDelegates
{
public:
    explicit ShapeDelegates(ITrackerHandleManager const* trackerOwner) noexcept;
    ~ShapeDelegates();

    ShapeDelegates(ShapeDelegates const&) = delete;
    ShapeDelegates& operator=(ShapeDelegates const&) = delete;

    // Registration. Null delegates are rejected by the verb before reaching here. Returns the
    // existing Id when the same delegate object is already registered, and a new Version always.
    DelegateRef Register(winrt::TableViewPredicate2 const& predicate);
    DelegateRef Register(winrt::TableViewKeySelector2 const& keySelector);
    DelegateRef Register(winrt::TableViewComparer2 const& comparer);

    // Keeps only the delegates referenced by `specsInUse`; releases the rest.
    void Retain(std::span<ShapeSpec const* const> specsInUse);

    // Invocation, used by the engine and live shaping. A predicate that throws rejects the item
    // (as v1's PassesFilter). Error handling for key selectors, comparers and paths follows the
    // engine's error rule (open, see the spec).
    bool Passes(DelegateId predicate, winrt::IInspectable const& item) const;
    winrt::IInspectable Key(DelegateId keySelector, winrt::IInspectable const& item) const;
    int32_t Compare(DelegateId comparer, winrt::IInspectable const& x, winrt::IInspectable const& y) const;

    // Teardown: destroys every tracker_ref and nulls m_trackerOwner. Must run on
    // the UI thread while the owner is alive. The registry is empty and inert afterwards.
    void Clear() noexcept;

private:
    enum class DelegateKind : uint8_t
    {
        Predicate,
        KeySelector,
        Comparer,
    };

    struct Entry
    {
        DelegateKind Kind{ DelegateKind::Predicate };

        // The delegate as registered, held through tracker_ref; queried for the typed delegate
        // on invocation. Optional so Clear() can destroy it while the owner is alive.
        std::optional<tracker_ref<winrt::Windows::Foundation::IUnknown>> Delegate;
    };

    DelegateId NextId() noexcept;
    uint32_t NextVersion() noexcept;

    ITrackerHandleManager const* m_trackerOwner{ nullptr };

    std::unordered_map<DelegateId, Entry> m_entries;

    // Dedupe: canonical IUnknown address -> id. Address only, never dereferenced.
    std::unordered_map<void const*, DelegateId> m_idByAddress;

    uint32_t m_nextId{ 1 };
    uint32_t m_nextVersion{ 1 };
};
