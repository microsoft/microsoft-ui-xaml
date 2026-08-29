// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Experimental compile-time hashing

namespace ct_hash
{
    namespace details
    {
        constexpr uint32_t c_offset = 2166136261ULL;
        constexpr uint32_t c_prime = 16777619ULL;

        constexpr uint32_t hash_byte(uint32_t last, uint8_t byte)
        {
            return (static_cast<uint64_t>(last ^ byte) * c_prime) & static_cast<uint32_t>(-1);
        }

        template <size_t N>
        constexpr uint32_t hash(const wchar_t* arr)
        {
            uint32_t val = hash_byte(hash<N - 1>(arr), arr[N - 1] & 0xff);
            return hash_byte(val, arr[N - 1] >> 8);
        }

        template <>
        constexpr uint32_t hash<0>(const wchar_t* arr)
        {
            return c_offset;
        }
    }

    // Compile-time hash for strings.

    template <size_t N>
    constexpr uint32_t hash(const wchar_t(&arr)[N])
    {
        return details::hash<N - 1>(arr);
    }

    // Corresponding runtime hash function.

    inline uint32_t hash(const wchar_t* buf, size_t count)
    {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(buf);
        uint32_t val = details::c_offset;

        count *= sizeof(wchar_t);

        for (; count > 0; --count, ++bytes)
        {
            val ^= *bytes;
            val *= details::c_prime;
        }

        return val;
    }
}