// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <utility>
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

    // A group's count of IMMEDIATE children. Over a hierarchy a group's rows include visible
    // descendants, so only the projection that sliced the group knows the root count.
    struct __declspec(uuid("9D3B0F7A-6B2E-4E67-93A2-1C8A9F5E4B10")) IGroupChildCount : ::IUnknown
    {
        virtual int32_t GroupChildCount() const = 0;
    };

    // Value keys compare numerically equal floats as equal: -0 folds into +0 and every NaN into one
    // canonical NaN, so the key is not the raw bit pattern.
    inline uint32_t CanonicalFloatBits(float value) noexcept
    {
        if (std::isnan(value)) return 0x7FC00000u;
        if (value == 0.0f) return 0u;
        return std::bit_cast<uint32_t>(value);
    }

    inline uint64_t CanonicalDoubleBits(double value) noexcept
    {
        if (std::isnan(value)) return 0x7FF8000000000000ull;
        if (value == 0.0) return 0ull;
        return std::bit_cast<uint64_t>(value);
    }

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

    // A boxed WinRT enum (IPropertyValue of OtherType), normalized to its runtime type and underlying integer value, so
    // two boxes of one enum value compare and key equal. Values above INT64_MAX are held in Unsigned.
    struct EnumValue
    {
        winrt::hstring TypeName;
        bool IsUnsigned{ false };
        int64_t Signed{ 0 };
        uint64_t Unsigned{ 0 };

        // False for any box that is not an enum (no integer getter succeeds). Never throws. The type name is read only
        // when asked for (identity keys need it; ordering does not).
        static bool TryGet(winrt::Windows::Foundation::IPropertyValue const& propertyValue, EnumValue& value, bool includeTypeName = true) noexcept;
        // Orders by underlying value alone, whatever the enum types: equal values of two types compare equal.
        static int Compare(EnumValue const& a, EnumValue const& b) noexcept;
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

    // Whose collection is read. A read of the caller's collection that throws is rethrown tagged as the caller's
    // (RethrowAsCallerError), except running out of memory while enumerating a collection with no indexed read, which
    // may be shaping's own allocation; a read of shaping's own collections, such as the group list it builds, never is.
    enum class CollectionOwner { Shaping, Caller };

    std::vector<winrt::IInspectable> EnumerateInspectableItems(
        winrt::IInspectable const& source,
        bool throwIfUnsupported = false,
        CollectionOwner owner = CollectionOwner::Shaping);

    // The kind a CallerError carries when the caller's collection or contract failed. An owner maps kinds onto its own
    // fault vocabulary; TableViewTip::AppFaultKind::Data is this value.
    constexpr uint32_t c_callerDataFault = 6;

    // An exception that left the caller's code: its collection, a broken contract, or a callback an owner ran. It
    // keeps the original HRESULT and error info, so the caller sees the same error, and records whose fault it was,
    // so an owner attributes a failure by the exception's type instead of by matching HRESULTs.
    struct CallerError final : winrt::hresult_error
    {
        CallerError(winrt::hresult_error const& error, uint32_t kind) noexcept : winrt::hresult_error(error), m_kind(kind) {}
        uint32_t Kind() const noexcept { return m_kind; }

    private:
        uint32_t m_kind;
    };

    // The caller's code ran out of memory (C++/WinRT throws std::bad_alloc for E_OUTOFMEMORY). Still a std::bad_alloc,
    // so it leaves as E_OUTOFMEMORY without allocating, and records whose fault it was like CallerError.
    struct CallerOutOfMemory final : std::bad_alloc
    {
        explicit CallerOutOfMemory(uint32_t kind) noexcept : m_kind(kind) {}
        uint32_t Kind() const noexcept { return m_kind; }

    private:
        uint32_t m_kind;
    };

    // In a catch: rethrows the current exception tagged with `kind`, as a CallerError, or a CallerOutOfMemory when the
    // caller's code ran out of memory. A tagged exception keeps its kind; any other exception leaves unchanged.
    [[noreturn]] void RethrowAsCallerError(uint32_t kind);
    // In a catch: the current exception's kind when it is tagged, else 0.
    uint32_t CurrentCallerErrorKind() noexcept;

    // Caller delegates that threw and were contained by design (a predicate excludes its row, a key selector yields a
    // null key, a comparer calls the pair equal): how many, and the first one's HRESULT. An owner takes the tally
    // after a verb, so it can report the caller's fault without failing the verb.
    struct SwallowedFaults
    {
        uint32_t count{};
        HRESULT firstError{ S_OK };

        // In a catch. Reads only the first fault's HRESULT.
        void Note() noexcept;
    };

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
        explicit CollectionAccessor(winrt::IInspectable const& source, CollectionOwner owner);

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
        bool m_callerOwned{};

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

}
