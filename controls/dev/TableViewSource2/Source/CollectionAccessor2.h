// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Interop.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Foundation.h>

#include "tracker_ref.h"

// TableViewSource2 copy of ShapingHelpers::CollectionAccessor (TabularShaping/ShapingHelpers.h).

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
// Every reference to the source collection is held through a tracker_ref owned by the enclosing
// ReferenceTracker (TableViewSource2), matching InspectingDataSource so .NET GC can trace cycles
// when the collection references its owner.
//
// Storage (decided, B4): two tracker_refs, not one per interface. The constructor resolves one
// read interface and one notification interface, stores each resolved interface pointer in an
// IInspectable-typed tracker_ref, and records which one it is in a kind. Readers cast the pointer
// back by kind (the reinterpret InspectingDataSource::OnBindableVectorChanged already relies on),
// so no read pays a QI. Unset tracker_refs cost storage only, but every set one is a GC tracker
// handle, and there is one accessor per root source plus one per ChildFeed: 2 handles and about
// 50 B each, instead of 3-5 handles and 168 B with seven typed members.
class CollectionAccessor2
{
public:
    // How the source is read. Resolved once, first match wins, in this order.
    enum class ReadKind : uint8_t
    {
        None,               // Not a collection (or no source): Count 0, Enumerate empty or throws.
        Vector,             // IVector<IInspectable>
        VectorView,         // IVectorView<IInspectable>
        BindableVector,     // IBindableVector
        Iterable,           // IIterable<IInspectable>: enumerable only, not indexable. Enumerate
                            // keeps v1's CLR retry: an E_NOINTERFACE from the iterator re-queries
                            // m_read for IBindableIterable (one QI, on that failure path only).
        BindableIterable,   // IBindableIterable: enumerable only, not indexable
    };

    // How the source reports changes. Resolved once, first match wins, in this order:
    // INotifyCollectionChanged carries per-change detail; the vector interfaces carry only an
    // index and a verb. Callers subscribe to exactly this one.
    enum class ObserveKind : uint8_t
    {
        None,
        NotifyCollectionChanged,    // INotifyCollectionChanged
        ObservableVector,           // IObservableVector<IInspectable>
        BindableObservableVector,   // IBindableObservableVector
    };

    explicit CollectionAccessor2(ITrackerHandleManager const* owner);
    CollectionAccessor2(
        ITrackerHandleManager const* owner,
        winrt::Windows::Foundation::IInspectable const& source);

    ReadKind GetReadKind() const noexcept { return m_readKind; }
    ObserveKind GetObserveKind() const noexcept { return m_observeKind; }

    // True when Count/GetAt are a direct call rather than an enumeration (Vector, VectorView,
    // BindableVector). An enumerable-only source reports false; callers that need indexed access
    // degrade instead of materializing behind the caller's back.
    bool IsIndexable() const noexcept;

    // True when the source publishes changes through a supported notification interface.
    bool IsObservable() const noexcept { return m_observeKind != ObserveKind::None; }

    // Read through to the live collection rather than a value captured at construction: this
    // classifies a source that goes on mutating, so a cached count would go stale.
    uint32_t Count() const;
    bool TryGetAt(uint32_t index, winrt::Windows::Foundation::IInspectable& item) const;
    std::vector<winrt::Windows::Foundation::IInspectable> Enumerate(bool throwIfUnsupported = false) const;

    // The resolved notification interface, typed, so a subscriber binds to it without re-probing.
    // Each returns m_observe cast back to its interface when GetObserveKind() matches, and null
    // otherwise; the caller switches on GetObserveKind(). Pass useSafeGet = true on teardown paths
    // where the target may already have been collected by the GC.
    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged AsNotifyCollectionChanged(bool useSafeGet = false) const;
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> AsObservableVector(bool useSafeGet = false) const;
    winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector AsBindableObservableVector(bool useSafeGet = false) const;

private:
    // The indexed read itself, with no bounds check. Enumerate() has already established the
    // range and would otherwise pay a Size() call per item to re-derive it.
    winrt::Windows::Foundation::IInspectable GetAtUnchecked(uint32_t index) const;

    // The resolved read interface pointer (the try_as result for m_readKind, not the source's
    // IInspectable). Null when ReadKind::None. Independent of m_observe: a source can be read
    // through one interface and observed through another.
    tracker_ref<winrt::Windows::Foundation::IInspectable> m_read;

    // The resolved notification interface pointer for m_observeKind. Null when ObserveKind::None.
    tracker_ref<winrt::Windows::Foundation::IInspectable> m_observe;

    ReadKind m_readKind{ ReadKind::None };
    ObserveKind m_observeKind{ ObserveKind::None };
};
