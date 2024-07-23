// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>
#include "StaticAssertFalse.h"

namespace ext
{
    namespace details
    {
        // Bit operation helpers

        inline void zero_bits(uintptr_t* addr, uintptr_t mask)
        {
            *addr &= ~mask;
        }

        inline void copy_bits(const uintptr_t* from, uintptr_t* to, uintptr_t mask)
        {
            *to &= ~mask;
            *to |= *from & mask;
        }

        inline void move_bits(uintptr_t* from, uintptr_t* to, uintptr_t mask)
        {
            *to &= ~mask;
            *to |= *from & mask;
            *from &= ~mask;
        }

        template <typename T, size_t N>
        void zero_all(T (&arr)[N])
        {
            for (int i = 0; i < N; ++i)
            {
                arr[i] = 0;
            }
        }

        constexpr size_t _log2(size_t value)
        {
            return ((value < 2) ? 0 : 1 + _log2(value / 2));
        }

        // Storage layouts for heap and stack allocated pointers.
        // NT allocates heap on 8-byte boundary, therefore we have an extra bit for storage.

        struct align_to_8_params
        {
            static constexpr size_t c_freeBits      = 3;
            static constexpr size_t c_prefixBits    = 0;
            static constexpr size_t c_suffixBits    = sizeof(uintptr_t) * 8 - c_freeBits;
        };

        template <typename T>
        struct align_to_type_params
        {
            static constexpr size_t c_freeBits      = _log2(alignof(T));
            static constexpr size_t c_prefixBits    = 0;
            static constexpr size_t c_suffixBits    = sizeof(uintptr_t) * 8 - c_freeBits;
        };

        template <template<typename> typename ExtData>
        struct heap_alloc
        {
            using params = align_to_8_params;
            using overlay_t = ExtData<params>;
        };

        template <template<typename> typename ExtData, typename T>
        struct typealigned_alloc
        {
            using params = align_to_type_params<T>;
            using overlay_t = ExtData<params>;
        };

        // Overlay layout type providing basic bit operations on data / pointer.

        template <size_t, typename...> struct overlay {};

        template <typename T>
        struct overlay<0, T>
        {
            using overlay_t = typename T::overlay_t;

            using params = typename T::params;
            static constexpr uintptr_t c_freeBitsMask = ((1 << params::c_freeBits) - 1) << params::c_prefixBits;

            struct layout
            {
                overlay_t m_value;
            };

            static void zero(uintptr_t* arr, bool flip)
            {
                zero_bits(arr, (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            static void copy(const uintptr_t* from, uintptr_t* to, bool flip)
            {
                copy_bits(from, to, (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            static void move(uintptr_t* from, uintptr_t* to, bool flip)
            {
                move_bits(from, to, (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            template <size_t Index_RTL>
            static void sanitized_ptr(const uintptr_t* from, uintptr_t* to)
            {
                static_assert_false("Index overflow.");
            }

            template <>
            static void sanitized_ptr<0>(const uintptr_t* from, uintptr_t* to)
            {
                copy_bits(from, to, ~c_freeBitsMask);
            }

            static_assert(sizeof(overlay_t) == sizeof(uintptr_t), "Not enough bits on pointer type.");
        };

        template <size_t N, typename T, typename... Args>
        struct overlay<N, T, Args...>
        {
            using overlay_t = typename T::overlay_t;
            using base = overlay<N - 1, Args...>;

            using params = typename T::params;
            static constexpr uintptr_t c_freeBitsMask = ((1 << params::c_freeBits) - 1) << params::c_prefixBits;

            struct layout
            {
                typename base::layout m_prev;
                overlay_t             m_value;
            };

            static void zero(uintptr_t* arr, bool flip)
            {
                base::zero(arr, flip);
                zero_bits(&arr[N], (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            static void copy(const uintptr_t* from, uintptr_t* to, bool flip)
            {
                base::copy(from, to, flip);
                copy_bits(&from[N], &to[N], (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            static void move(uintptr_t* from, uintptr_t* to, bool flip)
            {
                base::move(from, to, flip);
                move_bits(&from[N], &to[N], (flip) ? ~c_freeBitsMask : c_freeBitsMask);
            }

            template <size_t Index_RTL>
            static void sanitized_ptr(const uintptr_t* from, uintptr_t* to)
            {
                base::sanitized_ptr<Index_RTL - 1>(from, to);
            }

            template <>
            static void sanitized_ptr<0>(const uintptr_t* from, uintptr_t* to)
            {
                copy_bits(from, to, ~c_freeBitsMask);
            }

            static_assert(sizeof(overlay_t) == sizeof(uintptr_t), "Not enough bits on pointer type.");
        };

        template <typename type, typename value_type, size_t value_index_LTR, typename... overlays>
        struct type_with_overlay
        {
            using raw_storage_t = uintptr_t[sizeof(type) / sizeof(uintptr_t)];
            static constexpr size_t c_overlayCount = sizeof...(overlays);
            static constexpr size_t c_lastOverlayIndex= c_overlayCount - 1;

            using overlay = overlay<c_lastOverlayIndex, overlays...>;

            // This solely exists to provide types for debugging visualizations.

            union layout
            {
                raw_storage_t            m_raw;
                typename overlay::layout m_overlay;
            } m_storage;

            static_assert(sizeof(layout) == sizeof(type), "Size mismatch.");

            template <size_t Index_LTR>
            constexpr auto& _data()
            {
                return datapriv<Index_LTR>().m_value;
            }

            template <size_t Index_LTR>
            constexpr const auto& _data() const
            {
                return datapriv<Index_LTR>().m_value;
            }

            static constexpr size_t _data_count()
            {
                return c_overlayCount;
            }

            void _zero_overlay()
            {
                overlay::zero(_as_raw(), false);
            }

            void _remove_overlay(raw_storage_t& state)
            {
                overlay::move(_as_raw(), state, false);
            }

            void _merge_overlay(const raw_storage_t& state)
            {
                overlay::copy(state, _as_raw(), false);
            }

            template <typename... Args>
            void _call_ctor(Args&&... args)
            {
                new (&_as_type())type(std::forward<Args>(args)...);
            }

            void _call_dtor()
            {
                _as_type().~type();
            }

            raw_storage_t& _as_raw()
            {
                return m_storage.m_raw;
            }

            const raw_storage_t& _as_raw() const
            {
                return m_storage.m_raw;
            }

            type& _as_type()
            {
                return reinterpret_cast<type&>(m_storage.m_raw);
            }

            const type& _as_type() const
            {
                return reinterpret_cast<const type&>(m_storage.m_raw);
            }

            value_type* _sanitized_ptr() const
            {
                uintptr_t result = 0;

                overlay::sanitized_ptr<c_lastOverlayIndex - value_index_LTR>(
                    &(_as_raw()[value_index_LTR]),
                    &result);

                return reinterpret_cast<value_type*>(result);
            }

        private:
            template <size_t Index_LTR>
            constexpr auto& datapriv()
            {
                return datapriv<Index_LTR - 1>().m_prev;
            }

            template <>
            constexpr auto& datapriv<0>()
            {
                return m_storage.m_overlay;
            }

            template <size_t Index_LTR>
            constexpr const auto& datapriv() const
            {
                return datapriv<Index_LTR - 1>().m_prev;
            }

            template <>
            constexpr const auto& datapriv<0>() const
            {
                return m_storage.m_overlay;
            }
        };

        // Raw pointer with extra data.

        template <typename type, typename overlay>
        class raw_ptr
            : protected type_with_overlay<type*, type, 0, overlay>
        {
            using base = type_with_overlay<type*, type, 0, overlay>;
            using raw_storage_t = typename base::raw_storage_t;

            void safe_assign(const type* ptr)
            {
                overlay::copy(
                    reinterpret_cast<const uintptr_t*>(&ptr),
                    _as_raw(),
                    true);
            }

        public:

            // ctors

            raw_ptr() noexcept
            {
                zero_all(_as_raw());
            }

            raw_ptr(nullptr_t) noexcept
            {
                zero_all(_as_raw());
            }

            raw_ptr(type* ptr) noexcept
            {
                _as_type() = ptr;
            }

            // does not copy extra bits
            raw_ptr(const raw_ptr& other) noexcept
            {
                _as_type() = other._sanitized_ptr();
            }

            // does not move extra bits
            raw_ptr(raw_ptr&& other) noexcept
            {
                _as_type() = other._sanitized_ptr();
                overlay::zero(other._as_raw(), true);
            }

            // assign

            raw_ptr& operator=(type* other)
            {
                safe_assign(other);
                return *this;
            }

            // does not assign extra bits
            raw_ptr& operator=(const raw_ptr& other)
            {
                if (&other != this)
                {
                    safe_assign(other._sanitized_ptr());
                }

                return *this;
            }

            // does not assign extra bits
            raw_ptr& operator=(raw_ptr&& other) noexcept
            {
                if (&other != this)
                {
                    safe_assign(other._sanitized_ptr());
                    overlay::zero(other._as_raw(), true);
                }

                return *this;
            }

            operator type*() const
            {
                return _sanitized_ptr();
            }

            type* operator->() const
            {
                return _sanitized_ptr();
            }

            type& operator*() const
            {
                return *_sanitized_ptr();
            }

            template <size_t Index_LTR>
            constexpr auto& data()
            {
                return _data<Index_LTR>();
            }

            template <size_t Index_LTR>
            constexpr const auto& data() const
            {
                return _data<Index_LTR>();
            }

            void data_copy(const raw_ptr& other)
            {
                overlay::copy(other._as_raw(), _as_raw(), false);
            }

            static constexpr size_t data_count()
            {
                return _data_count();
            }
        };

        // Smart pointer with extra data.  Provides ephemeral_ptr method for getting a clean instance of the object.

        template <typename type, typename value_type, size_t value_index_LTR, typename... overlays>
        class smart_ptr
            : protected type_with_overlay<type, value_type, value_index_LTR, overlays...>
        {
            using base = type_with_overlay<type, value_type, value_index_LTR, overlays...>;
            using raw_storage_t = typename base::raw_storage_t;

            template <typename T>
            void ctor(T other, std::false_type)
            {
                static_assert_false("Invalid way to construct type.");
            }

            void ctor(const smart_ptr& other, std::true_type)
            {
                raw_storage_t saved_overlay{};
                const_cast<smart_ptr&>(other)._remove_overlay(saved_overlay);
                _call_ctor(other._as_type());
                const_cast<smart_ptr&>(other)._merge_overlay(saved_overlay);
            }

            void ctor(smart_ptr&& other, std::true_type)
            {
                raw_storage_t saved_overlay{};
                other._remove_overlay(saved_overlay);
                _call_ctor(std::move(other._as_type()));
                other._merge_overlay(saved_overlay);
            }

            template <typename T>
            void assign(T other, std::false_type)
            {
                static_assert_false("Invalid way to assign type.");
            }

            void assign(const type& other, std::true_type)
            {
                if (&_as_type() != &other)
                {
                    raw_storage_t saved_overlay{};
                    _remove_overlay(saved_overlay);
                    _as_type() = other;
                    _merge_overlay(saved_overlay);
                }
            }

            void assign(type&& other, std::true_type)
            {
                if (&_as_type() != &other)
                {
                    raw_storage_t saved_overlay{};
                    _remove_overlay(saved_overlay);
                    _as_type() = std::move(other);
                    _merge_overlay(saved_overlay);
                }
            }

            void assign(const smart_ptr& other, std::true_type)
            {
                if (this != &other)
                {
                    raw_storage_t my_saved_overlay{};
                    raw_storage_t other_saved_overlay{};
                    _remove_overlay(my_saved_overlay);
                    const_cast<smart_ptr&>(other)._remove_overlay(other_saved_overlay);
                    _as_type() = other._as_type();
                    _merge_overlay(my_saved_overlay);
                    const_cast<smart_ptr&>(other)._merge_overlay(other_saved_overlay);
                }
            }

            void assign(smart_ptr&& other, std::true_type)
            {
                if (this != &other)
                {
                    raw_storage_t my_saved_overlay{};
                    raw_storage_t other_saved_overlay{};
                    _remove_overlay(my_saved_overlay);
                    const_cast<smart_ptr&>(other)._remove_overlay(other_saved_overlay);
                    _as_type() = std::move(other._as_type());
                    _merge_overlay(my_saved_overlay);
                    const_cast<smart_ptr&>(other)._merge_overlay(other_saved_overlay);
                }
            }

            raw_storage_t& as_raw()
            {
                return _as_raw();
            }

            const raw_storage_t& as_raw() const
            {
                return _as_raw();
            }

            type& as_type()
            {
                return _as_type();
            }

            const type& as_type() const
            {
                return _as_type();
            }

        public:
            // ctors

            template <typename... Args>
            smart_ptr(Args&&... args)
            {
                _call_ctor(std::forward<Args>(args)...);
            }

            smart_ptr(const smart_ptr& other)
            {
                ctor(other, std::is_copy_constructible<type>());
            }

            smart_ptr(smart_ptr&& other)
            {
                ctor(std::move(other), std::is_move_constructible<type>());
            }

            // dtor

            ~smart_ptr()
            {
                _zero_overlay();
                _call_dtor();
                zero_all(_as_raw());
            }

            // assign

            smart_ptr& operator=(const type& other)
            {
                assign(other, std::is_copy_assignable<type>());
                return *this;
            }

            smart_ptr& operator=(type&& other)
            {
                assign(std::move(other), std::is_move_assignable<type>());
                return *this;
            }

            smart_ptr& operator=(const smart_ptr& other)
            {
                assign(other, std::is_copy_assignable<type>());
                return *this;
            }

            smart_ptr& operator=(smart_ptr&& other)
            {
                assign(std::move(other), std::is_move_assignable<type>());
                return *this;
            }

            class ephemeral_t
            {
                friend class smart_ptr;

                void construct(std::true_type)
                {
                    raw_storage_t saved_overlay{};
                    m_owner._remove_overlay(saved_overlay);
                    m_value._call_ctor(std::move(m_owner._as_type()));
                    m_owner._merge_overlay(saved_overlay);
                }

                void destroy()
                {
                    raw_storage_t saved_overlay{};
                    m_owner._remove_overlay(saved_overlay);
                    m_owner._as_type() = std::move(m_value._as_type());
                    m_owner._merge_overlay(saved_overlay);
                }

                void construct(std::false_type)
                {
                    static_assert_false("Type needs to be move-constructible.");
                }

                base       m_value;
                smart_ptr& m_owner;

            public:
                ephemeral_t(smart_ptr& owner)
                    : m_owner(owner)
                {
                    construct(std::is_move_constructible<type>());
                }

                ~ephemeral_t()
                {
                    destroy();
                }

                type& operator*()
                {
                    return m_value._as_type();
                }

                const type& operator*() const
                {
                    return m_value._as_type();
                }

                type* operator->()
                {
                    return &m_value._as_type();
                }

                const type* operator->() const
                {
                    return &m_value._as_type();
                }
            };

            ephemeral_t ephemeral_ptr()
            {
                return ephemeral_t(*this);
            }

            // const version of temporary instance getter returns const so only const methods can be called.
            // still, it will write-back to backing object hence const_cast is needed.
            // state of the object should not be changed, so conceptually this can be still const.
            const ephemeral_t ephemeral_ptr() const
            {
                return ephemeral_t(const_cast<smart_ptr&>(*this));
            }

            value_type* operator->() const
            {
                return _sanitized_ptr();
            }

            value_type& operator*() const
            {
                return *_sanitized_ptr();
            }

            template <size_t Index_LTR>
            constexpr auto& data()
            {
                return _data<Index_LTR>();
            }

            template <size_t Index_LTR>
            constexpr const auto& data() const
            {
                return _data<Index_LTR>();
            }

            void data_copy(const smart_ptr& other)
            {
                overlay::copy(other._as_raw(), _as_raw(), false);
            }

            static constexpr size_t data_count()
            {
                return _data_count();
            }
        };
    }
}