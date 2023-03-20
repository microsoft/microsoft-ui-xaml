// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SharedHelpers.h"

struct __declspec(novtable) ITrackerHandleManager
{
    virtual ~ITrackerHandleManager() = default;

    const ITrackerHandleManager* GetTrackerHandleManager() const
    {
        return this;
    }

    //
    // ITrackerPtrWrapperManager interface
    //

    void NewTrackerHandle(::TrackerHandle &handle)  const try
    {
#ifdef _DEBUG
        MUX_ASSERT_NOASSUME(m_wasEnsureCalled);
#endif
        winrt::check_hresult(m_trackerOwnerInnerNoRef->CreateTrackerHandle(&handle));
    }
    catch (...) {}

    void DeleteTrackerHandle(::TrackerHandle handle) const try
    {
#ifdef _DEBUG
        MUX_ASSERT_NOASSUME(m_wasEnsureCalled);
#endif
        winrt::check_hresult(m_trackerOwnerInnerNoRef->DeleteTrackerHandle(handle));
    }
    catch (...) {}

    void SetTrackerValue(::TrackerHandle handle, IUnknown* value) const try
    {
#ifdef _DEBUG
        MUX_ASSERT_NOASSUME(m_wasEnsureCalled);
#endif
        winrt::check_hresult(m_trackerOwnerInnerNoRef->SetTrackerValue(handle, value));
    }
    catch (...) {}

    bool GetTrackerValue(::TrackerHandle handle, _Outptr_result_maybenull_ IUnknown** value) const try
    {
#ifdef _DEBUG
        MUX_ASSERT_NOASSUME(m_wasEnsureCalled);
#endif
        return !!m_trackerOwnerInnerNoRef->TryGetSafeTrackerValue(handle, value);
    }
    catch (...) { return false; }

    bool ShouldFallbackToComPointers() const
    {
        // If we are running on an older OS then we need to fall back to simple AddRef/Release tracking for tracker_refs
        // and we detect this scenario by just checking if we were able to acquire the ITrackerOwner interface.
#ifdef _DEBUG
        MUX_ASSERT_NOASSUME(m_wasEnsureCalled);
#endif
        return (m_trackerOwnerInnerNoRef == nullptr);
    }

#ifdef _DEBUG
    void DEBUG_NotifyTrackerActive() const
    {
        _activeTrackerChildren++;
    }

    void DEBUG_NotifyTrackerInactive() const
    {
        MUX_ASSERT(_activeTrackerChildren > 0);
        _activeTrackerChildren--;
    }

    mutable int _activeTrackerChildren{};
    bool m_wasEnsureCalled{};
#endif

protected:
    ::ITrackerOwner* m_trackerOwnerInnerNoRef{ nullptr };
};

// tracker_ref holds a T but needs to pass an IUnknown* to ITrackerOwner. For winrt::IInspectable-based
// things, those pointer values are the same (winrt::IInspectable *is* an IUnknown*). But when T is a random
// COM implementor, in particular a winrt::implements guy, then T is not an IUnknown* so we need to do a QI
// to find it. But tracker_ref still needs to hold a T, so we have this helper to find an appropriate IUnknown
// to pass down to ITrackerOwner.

template <typename T, typename Enable = void>
struct IUnknownAccessor
{
    // NOTE: this returns com_ptr<IUnknown> instead of IUnknown* so the below specialization can return
    // a temporary that was QI'd for. This one can just reinterpret.
    static com_ptr<IUnknown> const& get(T const& value)
    {
        // This is known to be an IUnknown already, so reinterpret it
        return reinterpret_cast<com_ptr<IUnknown> const&>(value);
    }
};

template <typename T>
struct IUnknownAccessor<com_ptr<T>>
{
    static com_ptr<IUnknown> get(com_ptr<T> const& value)
    {
        // com_ptr<T> is not necessarily an IUnknown so QI for it.
        if (value)
        {
            return value.as<IUnknown>();
        }

        return nullptr;
    }
};

enum class TrackerRefFallback
{
    None,
    FallbackToComPtrBeforeRS4
};

template<typename T, TrackerRefFallback fallback = TrackerRefFallback::None, typename RawStorageT = ::IUnknown*>
class tracker_ref sealed
{
    using IUnknownAccessorT = typename IUnknownAccessor<T>;
public:
    explicit tracker_ref(const ITrackerHandleManager* owner)
    {
        SetOwner(owner);
    }

    explicit tracker_ref(const ITrackerHandleManager* owner, const T& value)
    {
        SetOwner(owner);
        set(value);
    }

    explicit tracker_ref(nullptr_t)
    {
        static_assert_false("tracker_ref should only be used in ReferenceRuntimeClass-derived field members and 'this' should be passed for the owner");
    }

    ~tracker_ref()
    {
        if (m_owner)
        {
            if (!ShouldFallbackToComPointers())
            {
                if (m_handle)
                {
                    MUX_ASSERT(m_owner);
                    m_owner->DeleteTrackerHandle(m_handle);
                }
            }
            else
            {
                // In fallback mode, the field actually holds a COM reference so treat it as a T instead of RawStorageT
                // to go through the Release() of the COM reference.
                reinterpret_cast<T&>(m_valueNoRef) = nullptr;
            }

#ifdef _DEBUG
            m_owner->DEBUG_NotifyTrackerInactive();
#endif
        }
        else
        {
            // Despite the guards in the constructor, owner may be null if we are destroying a moved-from tracker_ref.
            // Just make sure that m_valueNoRef is null in that case.
            MUX_ASSERT(!m_valueNoRef);
        }
    }

    // Move semantics are available for tracker_ref<T> to be used
    // carefully so that we don't end up with two tracker_ref instances
    // with the same data.
    // Move constructor.
    tracker_ref(tracker_ref&& other) noexcept
        : m_owner(std::move(other.m_owner))
        , m_handle(std::move(other.m_handle))
        , m_valueNoRef(std::move(other.m_valueNoRef))
    {
        other.m_owner = nullptr;
        other.m_handle = nullptr;
        other.m_valueNoRef = nullptr;
    }
    // Move assignment operator.
    tracker_ref& operator=(tracker_ref&& other) noexcept
    {
        if (this != std::addressof(other))
        {
            tracker_ref(std::move(other)).Swap(*this);
        }
        return *this;
    }

    tracker_ref(const tracker_ref& other)
    {
        SetOwner(other.m_owner);
        set(other.get());
    }
    tracker_ref& operator=(const tracker_ref& other)
    {
        if (!m_owner)
        {
            SetOwner(other.m_owner);
        }
        else
        {
            MUX_ASSERT_MSG(m_owner == other.m_owner, "When assigning, tracker_ref needs to either have its owner not set or same owner");
        }
        set(other.get());
        return *this;
    }

    void Swap(tracker_ref& other)
    {
        std::swap(m_owner, other.m_owner);
        std::swap(m_handle, other.m_handle);
        std::swap(m_valueNoRef, other.m_valueNoRef);
    }

    void set(const T& value)
    {
        MUX_ASSERT(m_owner);

        if (!ShouldFallbackToComPointers())
        {
            m_valueNoRef = reinterpret_cast<RawStorageT>(winrt::get_abi(value));

            // Optimization: only create the handle if value isn't null or,
            // if it is, we are already set to a non-null value.
            if (!m_handle && value)
            {
                m_owner->NewTrackerHandle(m_handle);
            }
            if (m_handle)
            {
                m_owner->SetTrackerValue(m_handle, IUnknownAccessorT::get(value).get());
            }
        }
        else
        {
            // In fallback mode, the field actually has a reference, treat it as T instead of RawStorageT
            // to go through the Release of the old value and AddRef of the new value.
            reinterpret_cast<T&>(m_valueNoRef) = value;
        }
    }

    const T& get() const
    {
#if _DEBUG
        // Do some debug validation to make sure that m_valueNoRef and the GetTrackerValue result don't
        // get out of sync. Also if GetTrackerValue returns false it means that the caller should have been
        // using safe_get instead because we're being called during finalization and the target already got 
        // collected.
        auto succeeded = false;

        if (m_handle)
        {
            com_ptr<IUnknown> unknown;
            succeeded = m_owner->GetTrackerValue(m_handle, unknown.put());
            MUX_ASSERT_MSG(succeeded, "GetTrackerValue returned false, should have called safe_get instead?");

            // Check if the pointers are identical or, if not, that their IUnknowns QI to the same thing
            MUX_ASSERT(
                unknown.as<winrt::IUnknown>() == (reinterpret_cast<const T&>(m_valueNoRef)).as<winrt::IUnknown>());
        }
#endif
        return reinterpret_cast<const T &>(m_valueNoRef);
    }

    template<typename V = T>
    V safe_get(bool useSafeGet = true) const
    {
        if (m_valueNoRef == nullptr)
        {
            return nullptr;
        }
        else
        {
            if (useSafeGet && !ShouldFallbackToComPointers())
            {
                com_ptr<IUnknown> unknown;
                if (m_owner->GetTrackerValue(m_handle, unknown.put()))
                {
                    auto value = unknown.as<V>();
                    return value;
                }

                return nullptr;
            }
            else
            {
                // Always safe to use values in fallback mode or "!useSafeGet".
                // useSafeGet is an optimization for callers who mostly want to use the faster get() version but the 
                // call site may be called during the destructor path.
                return get().as<V>();
            }
        }
    }

    //
    // Helpers
    //
    explicit operator bool() const
    {
        return (m_valueNoRef != nullptr);
    }

    bool operator==(const tracker_ref& rhs) const
    {
        return get() == rhs.get();
    }

    bool operator!=(const tracker_ref& rhs) const
    {
        return !(*this == rhs);
    }

    bool operator==(const T& rhs) const
    {
        return get() == rhs;
    }

    bool operator!=(const T& rhs) const
    {
        return !(*this == rhs);
    }

    template <typename U>
    U as() const
    {
        return get().as<U>();
    }

    template <typename U>
    U try_as() const
    {
        return get().try_as<U>();
    }

private:
    void SetOwner(const ITrackerHandleManager* owner)
    {
        m_owner = owner;
#ifdef _DEBUG
        MUX_ASSERT(m_owner);
        m_owner->DEBUG_NotifyTrackerActive();
#endif
    }

    bool ShouldFallbackToComPointers() const
    {
        // On pre-RS4 builds we sometimes hit a crash when using tracker ref:
        // Bug 13904947: AV during ResetReferencesFromSparcePropertyValues in QUIAffinityReleaseQueue::DoCleanup
        // In these cases, we fallback to using a standard com pointer.
        return (fallback == TrackerRefFallback::FallbackToComPtrBeforeRS4 && !SharedHelpers::IsRS4OrHigher()) || m_owner->ShouldFallbackToComPointers();
    }

private:
    RawStorageT m_valueNoRef{ nullptr };
    ::TrackerHandle m_handle{ nullptr };
    const ITrackerHandleManager* m_owner{ nullptr };
};

template <typename T>
bool operator<(const tracker_ref<T>& lhs, const tracker_ref<T>& rhs)
{
    return lhs.get() < rhs.get();
}

template<typename T>
using tracker_com_ref = tracker_ref<com_ptr<T>, TrackerRefFallback::None, T*>;

// Specialization of tracker_ptr for hstring just so containers don't have to do something special.
// HSTRING doesn't need to be tracked but without this specialization it's hard for something like Vector<T>
// to always use tracker_ref<T> as its storage type.
template<>
struct tracker_ref<winrt::hstring, TrackerRefFallback::None, IUnknown*> : public winrt::hstring
{
    explicit tracker_ref(const ITrackerHandleManager* /*owner*/)
    {
    }

    explicit tracker_ref(const ITrackerHandleManager* /*owner*/, const winrt::hstring& value)
    {
        set(value);
    }

    void set(const winrt::hstring& value)
    {
        winrt::hstring::operator=(value);
    }

    const winrt::hstring& get() const
    {
        return *this;
    }

    const winrt::hstring& safe_get(bool) const
    {
        return *this;
    }
};

// Base for containers like event_source.
class ReferenceTrackerContainerBase
{
public:
    ReferenceTrackerContainerBase(ITrackerHandleManager* owner)
        : m_owner{ owner }
    {
#if _DEBUG
        m_owner->DEBUG_NotifyTrackerActive();
#endif
    }


    ~ReferenceTrackerContainerBase()
    {
#if _DEBUG
        m_owner->DEBUG_NotifyTrackerInactive();
#endif
    }

    const ITrackerHandleManager* GetTrackerHandleManager() const
    {
        return m_owner;
    }

private:
    ITrackerHandleManager* m_owner;
};


template <typename ImplT, typename T>
struct ReferenceTrackerStorageHelper
{
    auto wrap(const T& value) const
    {
        tracker_ref<T> holder{ Impl()->GetTrackerHandleManager() };
        holder.set(value);
        return holder;
    }

    T unwrap(const tracker_ref<T>& holder) const
    {
        return holder.get();
    }

private:
    const ImplT* Impl() const { return static_cast<const ImplT*>(this); }
};

template <typename T>
struct NonReferenceTrackerStorageHelper
{
    auto wrap(const T& value) const
    {
        return value;
    }

    T unwrap(const T& value) const
    {
        return value;
    }
};
