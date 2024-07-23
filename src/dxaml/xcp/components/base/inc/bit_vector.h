// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <stdint.h>

namespace containers
{
    // Bit vector class allowing addition and removal of elements at runtime (dynamically sized).
    // It has heap utilization advantage over std::vector<bool>.  STLs vector has constant overhead of
    // 4 pointers and a heap allocation for storing any bits.
    // bit_vector can store 26 / 57 bits (32-bit / 64-bit respectively) in one pointer without heap
    // allocation and if number of elements is larger than these thresholds, it will allocate and use heap.

    class bit_vector
    {
    public:
        using bit_count_t = std::size_t;

        bit_vector();
        bit_vector(bit_count_t bit_count);
        bit_vector(const bit_vector& other);
        bit_vector(bit_vector&& other) noexcept;

        bit_vector& operator=(const bit_vector& other);
        bit_vector& operator=(bit_vector&& other) noexcept;

        ~bit_vector();

        // Retrieve value at index.  Does not bounds-check at runtime.
        bool operator[](bit_count_t index) const;

        // Set value at index.  Does not bounds-check at runtime.
        void set(bit_count_t index, bool value);

        // Adds value to the end of vector.
        void push_back(bool value);

        // Removes last value from the vector.  Does not check if there is a value to be removed at runtime.
        void pop_back();

        // Discards contents of bit_vector and resizes it to bit_count (values initialized to false).
        void reset(bit_count_t bit_count = 0);

        // Erases element at index.
        void erase(bit_count_t index);

        bit_count_t size() const;
        bool empty() const;

        // Tests if all values in bit_vector are false.  For empty bit_vector returns true.
        bool all_false() const;

    private:
        template <bit_count_t> struct size_bits {};

        template <> struct size_bits<32>
        {
            static constexpr bit_count_t value = 5;
        };

        template <> struct size_bits<64>
        {
            static constexpr bit_count_t value = 6;
        };

        using int_chunk_type = uintptr_t;
        using ext_chunk_type = uint32_t;

        static constexpr bit_count_t s_ext_chunk_bytes      = sizeof(ext_chunk_type);
        static constexpr bit_count_t s_ext_chunk_bits       = s_ext_chunk_bytes << 3;
        static constexpr bit_count_t s_int_state_bytes      = sizeof(int_chunk_type);
        static constexpr bit_count_t s_int_state_bits       = s_int_state_bytes << 3;
        static constexpr bit_count_t s_int_size_bits        = size_bits<s_int_state_bits>::value;
        static constexpr bit_count_t s_int_bits             = s_int_state_bits - s_int_size_bits - 1;

        struct external_storage
        {
            bit_count_t     m_size;     // Number of valid bits in external storage.
            bit_count_t     m_capacity; // Maximum number of bits which can be stored in allocated chunk.
            ext_chunk_type  m_bits;     // First chunk of bit values.
        };

        union state_type
        {
            struct
            {
                int_chunk_type m_is_internal     : 1;                   // LSB indicating type of storage being used: 0 - external, 1 - internal.
                int_chunk_type m_size            : s_int_size_bits;     // Number of valid bits in internal storage.
                int_chunk_type m_bits            : s_int_bits;          // Bit values.
            };

            external_storage* m_external;
        }
        m_state;

        void init();
        void destroy();
        void copy_from(const bit_vector& other);
        void resize_internal(bit_count_t bit_count);

        bool get_from_internal_storage(bit_count_t index) const;
        void set_in_internal_storage(bit_count_t index, bool value);
        bool get_from_external_storage(bit_count_t index) const;
        void set_in_external_storage(bit_count_t index, bool value);

        // Return block of memory able to store passed number of bits.
        // Header is initialized, while bit values are not and need to be initialized by caller.
        static external_storage* alloc_external_uninitialized(bit_count_t bit_count);

        // Return number of bytes needed to store passed number of bits.
        // Includes space for header.
        static std::size_t alloc_size(bit_count_t bit_count);

        template <typename T>
        static T erase_bit(const T& chunk, bit_count_t index)
        {
            T rotate_mask = static_cast<T>(-1) << index;
            return (chunk & ~rotate_mask) | ((chunk >> 1) & rotate_mask);
        }
    };

    static_assert(alignof(bit_vector) >= 2, "Violated alignment assumption");
}