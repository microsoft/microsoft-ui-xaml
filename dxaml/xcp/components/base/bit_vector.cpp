// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "bit_vector.h"
#include <string.h>

namespace containers
{
    bit_vector::bit_vector()
    {
        init();
    }

    bit_vector::bit_vector(bit_count_t bit_count)
    {
        resize_internal(bit_count);
    }

    bit_vector::bit_vector(const bit_vector& other)
    {
        copy_from(other);
    }

    bit_vector::bit_vector(bit_vector&& other) noexcept
    {
        m_state = other.m_state;
        other.init();
    }

    bit_vector::~bit_vector()
    {
        destroy();
    }

    bit_vector& bit_vector::operator=(const bit_vector& other)
    {
        if (this != &other)
        {
            destroy();
            copy_from(other);
        }

        return *this;
    }

    bit_vector& bit_vector::operator=(bit_vector&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            m_state = other.m_state;
            other.init();
        }

        return *this;
    }

    bool bit_vector::operator[](bit_count_t index) const
    {
        ASSERT(index < size());

        if (m_state.m_is_internal)
        {
            return get_from_internal_storage(index);
        }
        else
        {
            return get_from_external_storage(index);
        }
    }

    void bit_vector::set(bit_count_t index, bool value)
    {
        ASSERT(index < size());

        if (m_state.m_is_internal)
        {
            set_in_internal_storage(index, value);
        }
        else
        {
            set_in_external_storage(index, value);
        }
    }

    void bit_vector::push_back(bool value)
    {
        if (m_state.m_is_internal)
        {
            bit_count_t index = m_state.m_size;

            if (index < s_int_bits)
            {
                ++m_state.m_size;
                set_in_internal_storage(index, value);
            }
            else
            {
                // crossing over from internal to external.

                int_chunk_type bits = m_state.m_bits; // copy to local variable to save unshifted value.
                m_state.m_external = alloc_external_uninitialized(index + 1);
                m_state.m_external->m_size = index + 1;
                std::memcpy(&m_state.m_external->m_bits, &bits, sizeof(bits));
                set_in_external_storage(index, value);

                ASSERT(!m_state.m_is_internal);
            }
        }
        else
        {
            external_storage* external = m_state.m_external;
            bit_count_t index = external->m_size;

            if (index < external->m_capacity)
            {
                ++external->m_size;
            }
            else
            {
                // resizing external

                external_storage* new_storage = alloc_external_uninitialized(index + 1);
                new_storage->m_size = index + 1;
                std::memcpy(&new_storage->m_bits, &external->m_bits, external->m_capacity >> 3);    // copy full blocks.
                (&new_storage->m_bits)[(index + 1) / s_ext_chunk_bits] = 0;                         // and initialize newly allocated one to 0.
                destroy();
                m_state.m_external = new_storage;
            }

            set_in_external_storage(index, value);

            ASSERT(!m_state.m_is_internal);
        }
    }

    void bit_vector::pop_back()
    {
        ASSERT(size() > 0);

        if (m_state.m_is_internal)
        {
            --m_state.m_size;
        }
        else
        {
            --m_state.m_external->m_size;
        }
    }

    void bit_vector::reset(bit_count_t bit_count)
    {
        destroy();
        resize_internal(bit_count);
    }

    void bit_vector::erase(bit_count_t index)
    {
        ASSERT(index < size());

        if (m_state.m_is_internal)
        {
            m_state.m_bits = erase_bit(m_state.m_bits, index);
            --m_state.m_size;
        }
        else
        {
            ext_chunk_type* chunks = &m_state.m_external->m_bits;
            int chunk_count = static_cast<int>((m_state.m_external->m_size / s_ext_chunk_bits) + ((m_state.m_external->m_size % s_ext_chunk_bits) > 0));
            int chunk_index = static_cast<int>(index / s_ext_chunk_bits);
            bool carry = false;

            for (int i = chunk_count - 1; i > chunk_index; --i)
            {
                bool temp_carry = chunks[i] & 1;
                chunks[i] >>= 1;
                chunks[i] |= (carry << (s_ext_chunk_bits - 1));
                carry = temp_carry;
            }

            chunks[chunk_index] = erase_bit(chunks[chunk_index], index % s_ext_chunk_bits);
            chunks[chunk_index] |= (carry << (s_ext_chunk_bits - 1));

            --m_state.m_external->m_size;
        }
    }

    bit_vector::bit_count_t bit_vector::size() const
    {
        if (m_state.m_is_internal)
        {
            return m_state.m_size;
        }
        else
        {
            return m_state.m_external->m_size;
        }
    }

    bool bit_vector::empty() const
    {
        return size() == 0;
    }

    bool bit_vector::all_false() const
    {
        if (m_state.m_is_internal)
        {
            return (m_state.m_bits & ~(static_cast<int_chunk_type>(-1) << m_state.m_size)) == 0;
        }
        else
        {
            external_storage* external = m_state.m_external;
            ext_chunk_type* chunk = &external->m_bits;
            int full_chunk_count = static_cast<int>(external->m_size / s_ext_chunk_bits);

            // no need for masking, as all bits in full chunk are valid.
            for (int i = 0; i < full_chunk_count; ++i)
            {
                if (*chunk != 0)
                {
                    return false;
                }

                ++chunk;
            }

            // in partial chunk not all bits are valid, so set others to 0 before comparison.
            int partial_chunk_bits = external->m_size % s_ext_chunk_bits;
            return (*chunk & ~(static_cast<ext_chunk_type>(-1) << partial_chunk_bits)) == 0;
        }
    }

    void bit_vector::init()
    {
        m_state = {};
        m_state.m_is_internal = true;
    }

    void bit_vector::destroy()
    {
        if (!m_state.m_is_internal)
        {
            delete[] reinterpret_cast<uint8_t*>(m_state.m_external);
        }
    }

    void bit_vector::copy_from(const bit_vector& other)
    {
        if (other.m_state.m_is_internal)
        {
            m_state = other.m_state;
        }
        else
        {
            external_storage* other_external = other.m_state.m_external;
            m_state.m_external = alloc_external_uninitialized(other_external->m_capacity);
            m_state.m_external->m_size = other_external->m_size;
            std::memcpy(&m_state.m_external->m_bits, &other_external->m_bits, other_external->m_capacity >> 3);

            ASSERT(!m_state.m_is_internal);
        }
    }

    void bit_vector::resize_internal(bit_count_t bit_count)
    {
        if (bit_count <= s_int_bits)
        {
            init();
            m_state.m_size = bit_count;
        }
        else
        {
            m_state.m_external = alloc_external_uninitialized(bit_count);
            m_state.m_external->m_size = bit_count;
            std::memset(&m_state.m_external->m_bits, 0, m_state.m_external->m_capacity >> 3);

            ASSERT(!m_state.m_is_internal);
        }
    }

    bool bit_vector::get_from_internal_storage(bit_count_t index) const
    {
        int_chunk_type bit = static_cast<int_chunk_type>(1) << index;
        return (m_state.m_bits & bit) != 0;
    }

    void bit_vector::set_in_internal_storage(bit_count_t index, bool value)
    {
        m_state.m_bits &= ~(static_cast<int_chunk_type>(1) << index);
        m_state.m_bits |= static_cast<int_chunk_type>(value) << index;
    }

    bool bit_vector::get_from_external_storage(bit_count_t index) const
    {
        ext_chunk_type bit = static_cast<ext_chunk_type>(1) << (index % s_ext_chunk_bits);
        ext_chunk_type* chunks = &m_state.m_external->m_bits;
        return (chunks[index / s_ext_chunk_bits] & bit) != 0;
    }

    void bit_vector::set_in_external_storage(bit_count_t index, bool value)
    {
        ext_chunk_type& chunk = (&m_state.m_external->m_bits)[index / s_ext_chunk_bits];
        bit_count_t shift = index % s_ext_chunk_bits;
        chunk &= ~(static_cast<ext_chunk_type>(1) << shift);
        chunk |= static_cast<ext_chunk_type>(value) << shift;
    }

    bit_vector::external_storage* bit_vector::alloc_external_uninitialized(bit_count_t bit_count)
    {
        static_assert(sizeof(external_storage) - s_ext_chunk_bytes > 0, "sizes do not match...");
        std::size_t capacity_in_bits = alloc_size(bit_count);
        // allocate size of header struct, bytes for bitfields minus one chunk, as it's included in declaration of header.
        external_storage* storage = reinterpret_cast<external_storage*>(new uint8_t[sizeof(external_storage) + (capacity_in_bits >> 3) - s_ext_chunk_bytes]);
        storage->m_size = 0;
        storage->m_capacity = static_cast<bit_count_t>(capacity_in_bits);
        return storage;
    }

    std::size_t bit_vector::alloc_size(bit_count_t bit_count)
    {
        constexpr auto all_on = static_cast<std::size_t>(s_ext_chunk_bits - 1);
        return (static_cast<std::size_t>(bit_count) + all_on) & ~all_on;
    }
}