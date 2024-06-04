// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is the weak-reference counterpart to xref_ptr
// Used for intrusively ref-counted objects that aren't COM, like CDependencyObject

#pragma once

#include "weakref_count.h"
#include <xref_ptr.h>
#include <type_traits>
//#include <CDependencyObject.h>

class CDependencyObject;

namespace xref
{
    template <typename Ty>
    class weakref_ptr;

    template <typename Ty>
    weakref_ptr<Ty> get_weakref(_In_opt_ Ty* pTargetObject);

    // RAII template wrapper for weak references to core objects (deriving from CDependencyObject)
    // Obtain via xref::get_weakref(Ty* pTargetObject)
    // Example: CUIElement* pElement = ...; weakref_ptr<CUIElement> = get_weakref(pElement);
    template <typename Ty>
    class weakref_ptr
    {
    public:
        // Assuming only CDependencyObject-derived types for the sake of simplicity.
        // I can't think of any non-COM, non-CDO, intrusive ref-counted types that couldn't
        // just be converted to use non-intrusive counting with std::shared_ptr/weak_ptr
        //static_assert(std::is_base_of<CDependencyObject, Ty>::value, "weakref_ptr only supported for types derived from CDependencyObject");
        typedef xref::details::control_block ref_count_type;

        template <typename Ty>
            friend weakref_ptr<Ty> get_weakref(_In_opt_ Ty* pTargetObject);

        weakref_ptr() WI_NOEXCEPT
        {}

        weakref_ptr(const weakref_ptr& other) WI_NOEXCEPT
        {
            // Defer to the reset method to ensure the other refcount gets incremented before we take ownership
            private_reset(other.m_ptr, other.m_ref_count);
        }

        // Convertible type ctor
        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        weakref_ptr(const weakref_ptr<Ty2>& other) WI_NOEXCEPT
        {
            // EVIL gotcha. For a conventional derived-to-base assignment, the compiler knows what to
            // do at compile-time and adjusts by a constant offset. However, with virtual bases,
            // the offset must be determined at runtime via its vptr
            // Because of that, we can't afford to let the other weakref-owned object die before we
            // try to perform the conversion, so we lock() it into xref_ptr temporarily.
            // If it's already dead, then the xref_ptr will hold a null, and that's just fine.
            // Not sure if we'd ever end up with a virtual base in XAML, but the potential bug is
            // difficult enough to diagnose, so let's just be paranoid and handle it in advance.

            // PS: This kind of subtlety is exactly why we should be trying to use standard STL as much as possible
            safe_assign(other.lock().get());
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        explicit weakref_ptr(const xref_ptr<Ty2>& other)
        {
            safe_assign(other.get());
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        explicit weakref_ptr(Ty2* other)
        {
            safe_assign(other);
        }

        // The non-converting move constructor can be nice and fast
        weakref_ptr(weakref_ptr&& other) WI_NOEXCEPT
            : m_ptr(other.m_ptr)
            , m_ref_count(other.m_ref_count)
        {
            other.m_ptr = nullptr;
            other.m_ref_count = nullptr;
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        weakref_ptr(weakref_ptr<Ty2>&& other) WI_NOEXCEPT
        {
            // See above for why the lock() is needed
            safe_assign(other.lock().get());
        }

        ~weakref_ptr() WI_NOEXCEPT
        {
            if (m_ref_count)
            {
                m_ref_count->ReleaseWeak();
            }
        }

        weakref_ptr& operator=(const weakref_ptr& right) WI_NOEXCEPT
        {
            // No need to check against == *this, private_reset will simply inc/dec
            private_reset(right.m_ptr, right.m_ref_count);
            return *this;
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        weakref_ptr& operator=(const weakref_ptr<Ty2>& right) WI_NOEXCEPT
        {
            safe_assign(right.lock().get());
            return *this;
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        weakref_ptr& operator=(const xref_ptr<Ty2>& right)
        {
            safe_assign(right.get());
            return *this;
        }

        weakref_ptr& operator=(weakref_ptr&& right) WI_NOEXCEPT
        {
            if (&right != this)
            {
                reset();
                m_ptr = right.m_ptr;
                m_ref_count = right.m_ref_count;

                right.m_ptr = nullptr;
                right.m_ref_count = nullptr;
            }
            return *this;
        }

        template <typename Ty2,
            class = typename std::enable_if<std::is_convertible<Ty2*, Ty*>::value, void>::type>
        weakref_ptr& operator=(weakref_ptr<Ty2>&& right) WI_NOEXCEPT
        {
            safe_assign(right.lock().get());
            return *this;
        }

        void reset() WI_NOEXCEPT
        {
            private_reset<Ty>(nullptr, nullptr);
        }

        void swap(weakref_ptr& other) WI_NOEXCEPT
        {
            std::swap(m_ptr, other.m_ptr);
            std::swap(m_ref_count, other.m_ref_count);
        }

        bool expired() const WI_NOEXCEPT
        {
            return !m_ref_count || m_ref_count->expired();
        }

        xref_ptr<Ty> lock() const WI_NOEXCEPT
        {
            xref_ptr<Ty> result;
            if (m_ref_count && m_ref_count->AddStrong_nz())
            {
                result.attach(m_ptr);
            }
            return result;
        }

        // CDependencyObject::ResetReferencesFromChildren() is unfortunate.
        // It's called from ReleaseImpl when the strong refcount hits zero, which
        // triggers other code that expects to be able to resolve the weakref
        // while the strong refcount is zero, pre-destructor call
        // TODO: Change the code downstream from ResetReferenceFromChildren() to
        //       no longer rely on resolving this weakref
        Ty* lock_noref() const WI_NOEXCEPT
        {
            if (m_ref_count && (m_ref_count->is_destroying() || !m_ref_count->expired()))
            {
                return m_ptr;
            }
            return nullptr;
        }

        explicit operator bool() const WI_NOEXCEPT
        {
            return m_ref_count != nullptr;
        }

        bool operator<(const weakref_ptr& rhs) const WI_NOEXCEPT
        {
            return m_ref_count < rhs.m_ref_count;
        }

        bool operator==(const Ty* rhs) const WI_NOEXCEPT
        {
            return m_ptr == rhs;
        }

        bool PointerEquals(const weakref_ptr& rhs) const WI_NOEXCEPT
        {
            return m_ptr == rhs.m_ptr;
        }

        // Deceptive - m_ref_count isn't a raw number, but a pointer to a control block. This is comparing whether
        // the two weakref_ptrs use the same control block.
        bool operator==(const weakref_ptr& rhs) const WI_NOEXCEPT
        {
            return m_ref_count == rhs.m_ref_count;
        }

        std::size_t hash() const WI_NOEXCEPT
        {
            return std::hash<ref_count_type*>()(m_ref_count);
        }

    private:
        template <typename Ty2>
        void private_reset(_In_opt_ Ty2* other_ptr, _In_opt_ ref_count_type* other_count) WI_NOEXCEPT
        {
            if (other_count) {
                other_count->AddWeak();
            }
            if (m_ref_count) {
                m_ref_count->ReleaseWeak();
            }
            m_ref_count = other_count;
            m_ptr = other_ptr;
        }

        template <typename Ty2>
        void safe_assign(_In_opt_ Ty2* other_ptr) WI_NOEXCEPT
        {
            // Obtain an exactly-typed weakref_ptr which we can then move-assign with no conversion issues
            *this = get_weakref<Ty>(other_ptr);
        }

        Ty* m_ptr = nullptr;
        ref_count_type* m_ref_count = nullptr;
    };

    template <typename Ty>
    bool operator!=(const weakref_ptr<Ty>& lhs, const weakref_ptr<Ty>& rhs) { return !(lhs == rhs); }

    template <typename Ty>
    weakref_ptr<Ty> get_weakref(_In_opt_ Ty* pTargetObject)
    {
        weakref_ptr<Ty> result;
        if (pTargetObject)
        {
            result.private_reset(pTargetObject, pTargetObject->EnsureControlBlock());
        }
        return result;
    }

    template <typename Ty>
    weakref_ptr<Ty> get_weakref(_In_ const xref_ptr<Ty>& spTargetObject)
    {
        return get_weakref(spTargetObject.get());
    }

    // Non-member swap function so ADL can find it, e.g. from STL algorithms
    template <typename Ty>
    void swap(weakref_ptr<Ty>& lhs, weakref_ptr<Ty>& rhs) WI_NOEXCEPT
    {
        lhs.swap(rhs);
    }
}

// Specialize std::hash for ease of use in unordered collections
namespace std {
    template <typename Ty>
    struct hash<xref::weakref_ptr<Ty>>
    {
        std::size_t operator()(const xref::weakref_ptr<Ty>& ptr) const WI_NOEXCEPT
        {
            return ptr.hash();
        }
    };
}