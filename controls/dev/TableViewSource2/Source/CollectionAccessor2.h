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
class CollectionAccessor2
{
public:
    explicit CollectionAccessor2(ITrackerHandleManager const* owner);
    CollectionAccessor2(
        ITrackerHandleManager const* owner,
        winrt::Windows::Foundation::IInspectable const& source);

    // True when Count/GetAt are a direct call rather than an enumeration. An enumerable-only
    // source reports false; callers that need indexed access degrade instead of materializing
    // behind the caller's back.
    bool IsIndexable() const noexcept;
    // True when the source publishes changes through any supported notification interface.
    bool IsObservable() const noexcept
    {
        return static_cast<bool>(m_notifyCollectionChanged)
            || static_cast<bool>(m_observableVector)
            || static_cast<bool>(m_bindableObservableVector);
    }

    // Read through to the live collection rather than a value captured at construction: this
    // classifies a source that goes on mutating, so a cached count would go stale.
    uint32_t Count() const;
    bool TryGetAt(uint32_t index, winrt::Windows::Foundation::IInspectable& item) const;
    std::vector<winrt::Windows::Foundation::IInspectable> Enumerate(bool throwIfUnsupported = false) const;

    // The resolved notification interfaces, so a subscriber binds to one without re-probing.
    // Precedence is the caller's: INotifyCollectionChanged carries per-change detail, the
    // vector interfaces carry an index and a verb. Pass useSafeGet = true on teardown paths
    // where the target may already have been collected by the GC.
    winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged AsNotifyCollectionChanged(bool useSafeGet = false) const
    {
        return m_notifyCollectionChanged.safe_get(useSafeGet);
    }
    winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable> AsObservableVector(bool useSafeGet = false) const
    {
        return m_observableVector.safe_get(useSafeGet);
    }
    winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector AsBindableObservableVector(bool useSafeGet = false) const
    {
        return m_bindableObservableVector.safe_get(useSafeGet);
    }

private:
    // The indexed read itself, with no bounds check. Enumerate() has already established the
    // range and would otherwise pay a Size() call per item to re-derive it.
    winrt::Windows::Foundation::IInspectable GetAtUnchecked(uint32_t index) const;

    tracker_ref<winrt::Windows::Foundation::IInspectable> m_source;

    // Read: at most one of these is set, in IVector > IVectorView > IBindableVector order.
    tracker_ref<winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable>> m_vector;
    tracker_ref<winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable>> m_vectorView;
    tracker_ref<winrt::Microsoft::UI::Xaml::Interop::IBindableVector> m_bindableVector;

    // Observe: independent of the read resolution, since a source can be indexable through one
    // interface and observable through another.
    tracker_ref<winrt::Microsoft::UI::Xaml::Interop::INotifyCollectionChanged> m_notifyCollectionChanged;
    tracker_ref<winrt::Windows::Foundation::Collections::IObservableVector<winrt::Windows::Foundation::IInspectable>> m_observableVector;
    tracker_ref<winrt::Microsoft::UI::Xaml::Interop::IBindableObservableVector> m_bindableObservableVector;
};
