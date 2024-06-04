// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is holds the reference count logic for weakref_ptr
// Used for intrusively ref-counted objects that aren't COM, like CDependencyObject

#pragma once

#include <atomic>
#include <type_traits>
#include <wil\common.h>

namespace xref
{
    namespace details
    {
        // Holds the ref counts for weakref_ptr
        class control_block
        {
        public:
            control_block(uint32_t strong_count) WI_NOEXCEPT
                : m_strong(strong_count)
                , m_weak(1)
                , m_destroying(false)
            {}

            // Underlying strong count implementation is atomic, so both versions
            // route to the same call
            uint32_t AddStrong() WI_NOEXCEPT
            {
                ASSERT(!m_destroying);
                return ++m_strong;
            }
            uint32_t ThreadSafeAddStrong() WI_NOEXCEPT
            {
                ASSERT(!m_destroying);
                return ++m_strong;
            }
            uint32_t AddWeak() WI_NOEXCEPT { return ++m_weak; }

            uint32_t ReleaseStrong() WI_NOEXCEPT
            {
                ASSERT(m_strong > 0);
                return --m_strong;
            }
            uint32_t ThreadSafeReleaseStrong() WI_NOEXCEPT
            {
                ASSERT(m_strong > 0);
                return --m_strong;
            }
            // We delete ourselves if the weakref count hits zero, but we don't actively manage
            // the strong lifetime of the object itself. Since it's intrusively ref-counted, it has its
            // own "delete this" statement in Release().
            uint32_t ReleaseWeak() WI_NOEXCEPT
            {
                ASSERT(m_weak > 0);
                auto result = --m_weak;
                if (result == 0)
                {
                    ASSERT(m_strong == 0);
                    delete this;
                }
                return result;
            }

            // Used for weakref_ptr::lock()
            bool AddStrong_nz() WI_NOEXCEPT
            {
                // ASSERT(!m_destroying); - no longer needed.
                // Attempt to increment the strong refcount only if it's nonzero
                // Be careful... We don't just want to blindly increment and accidentally
                // resurrect an object after its refcount already went to zero
                uint32_t count = m_strong;
                while (count != 0)
                {
                    if (m_strong.compare_exchange_weak(count, count + 1))
                    {
                        return true;
                    }
                }
                return false;
            }

            uint32_t use_count() const WI_NOEXCEPT
            {
                return m_strong;
            }

            bool expired() const WI_NOEXCEPT
            {
                return m_strong == 0;
            }

            void start_destroying() WI_NOEXCEPT
            {
                m_destroying = true;
            }

            void end_destroying() WI_NOEXCEPT
            {
                m_destroying = false;
            }

            bool is_destroying() const WI_NOEXCEPT
            {
                return m_destroying;
            }

            // This method exists solely to service CDependencyObject's behavior
            // of re-adding a ref to CPopup (from 0) once it's started its destruction path
            // Nobody else should call this! Everybody else should get the assert
            // that prevents them from reviving a dying object
            uint32_t AddRefDuringDuringDestroy() WI_NOEXCEPT
            {
                return ++m_strong;
            }

            // Disallow copying/moving
            control_block(const control_block&) = delete;
            control_block(control_block&&) = delete;
            control_block& operator=(const control_block&) = delete;
            control_block& operator=(control_block&&) = delete;

        private:
            std::atomic<uint32_t> m_strong;
            std::atomic<uint32_t> m_weak;

            // CDependencyObject has some odd possibilities for reentrancy during cleanup
            // When the ref count hits zero, it performs some cleanup before its destructor runs
            // and this cleanup can cause other objects to resolve their weakref_ptr to this.
            // In lieu of rewriting all of that craziness to get the object under destruction to
            // "push" its updates to the world, we allow them to continue to resolve the weakref_ptr
            // but we also try to prevent them from changing the ref count

            // This flag is set when CDO::ReleaseImpl starts going down the cleanup path, and cleared in
            // its destructor. While the flag is set, clients can resolve the weakref_ptr without changing
            // the strong ref count. Any attempt to do so carries the potential for somebody to hold a dangling pointer
            // so we assert if the strong ref count gets modified during this temporary state of affairs.
            bool m_destroying;
        };

        // CDependencyObject gets created a LOT in our code base, so it's difficult
        // to justify adding another flag or pointer to it, especially when most instances
        // will never have a weakref. For that reason, I'm cramming the existing ref count
        // and the pointer to the control block into a single union, and wrapping it in
        // a class that manages the lifetime and delegation responsibilities.
        // For almost any other class, this sort of trickery would be absurd, but this is
        // CDependencyObject, and little differences can have large impacts here.
        class optional_ref_count
        {
        public:
            typedef control_block control_block_type;

            optional_ref_count() WI_NOEXCEPT
                : m_count_and_flag(1 << flag_width)
            {
                // Default to a local ref count
                set_local_flag();
            }

            ~optional_ref_count() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    // If we are delegating to a control block, release our weak reference
                    // to allow cleanup if no other weak refs exist
                    block->ReleaseWeak();
                }
            }

            _Ret_maybenull_ control_block_type*
                get_control_block() const WI_NOEXCEPT
            {
                control_block_type* result = nullptr;
                if (has_control_block())
                {
                    result = m_control_block;
                }
                return result;
            }

            _Ret_notnull_ control_block_type*
                ensure_control_block()
            {
                auto result = get_control_block();
                if (!result)
                {
                    uint32_t temp_count = m_count_and_flag >> flag_width;
                    result = new control_block_type(temp_count);
                    m_control_block = result;
                    set_delegated_flag();
                }
                return result;
            }

            uint32_t AddRef() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->AddStrong();
                }
                else
                {
                    return (m_count_and_flag += (1 << flag_width)) >> flag_width;
                }
            }

            uint32_t ThreadSafeAddRef() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->ThreadSafeAddStrong();
                }
                else
                {
                    return static_cast<uint32_t>((::_InterlockedExchangeAdd(&m_count_and_flag, 1 << flag_width) >> flag_width) + 1);
                }
            }

            // This method exists solely to service CDependencyObject's behavior
            // of re-adding a ref to CPopup (from 0) once it's started its destruction path
            // Nobody else should call this!
            uint32_t AddRefDuringDuringDestroy() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->AddRefDuringDuringDestroy();
                }
                else
                {
                    return (m_count_and_flag += (1 << flag_width)) >> flag_width;
                }
            }

            uint32_t Release() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->ReleaseStrong();
                }
                else
                {
                    return (m_count_and_flag -= (1 << flag_width)) >> flag_width;
                }
            }

            uint32_t ThreadSafeRelease() WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->ThreadSafeReleaseStrong();
                }
                else
                {
#pragma warning (suppress : 26450) // Arithmetic overflow: '<<' operation causes overflow at compile time. Use a wider type to store the operands (io.1).
                    constexpr unsigned int inc = ~0u << flag_width;
                    auto temp = ::_InterlockedExchangeAdd(&m_count_and_flag, inc);
                    return (temp >> flag_width) - 1;
                }
            }

            uint32_t GetRefCount() const WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    return block->use_count();
                }
                else
                {
                    return m_count_and_flag >> flag_width;
                }
            }

            void start_destroying() const WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    block->start_destroying();
                }
            }

            void end_destroying() const WI_NOEXCEPT
            {
                auto block = get_control_block();
                if (block)
                {
                    block->end_destroying();
                }
            }

        private:
            void set_local_flag() WI_NOEXCEPT { m_ptr_as_int |= flag_mask; }
            void set_delegated_flag() WI_NOEXCEPT { m_ptr_as_int &= count_mask; }

            bool has_control_block() const WI_NOEXCEPT { return (m_ptr_as_int & flag_mask) == 0; }

            // Overlaying the local ref count and the delegating control block in the same memory
            // We differentiate between the two cases by co-opting the LSbit of the address.
            // Safe because we can guarantee that the control block will be aligned on a 4 or 8 byte boundary
            // Technically, this means we only have 31 bits for the ref count now, but that seems unlikely to be problematic
            union
            {
                uint32_t m_count_and_flag;
                control_block_type* m_control_block;
                uintptr_t m_ptr_as_int; // For bit-masking trickery
            };
            static constexpr uintptr_t flag_width = 1;
            static constexpr uintptr_t flag_mask = (1 << flag_width) - 1;
            static constexpr uintptr_t count_mask = ~flag_mask;
            static_assert(std::alignment_of<control_block>::value >= (1 << flag_width), "Violated alignment assumption for safely packing control_block<T>*");
        };
    }
}