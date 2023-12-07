// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <limits>
#include <cstdint>

template <std::uint16_t Size, typename T>
class CircularMemoryLogger
{
    static constexpr std::uint16_t c_emptySentinel = std::numeric_limits<std::uint16_t>::max();
    static_assert(Size > 0 && Size < c_emptySentinel, "Size parameter is out of valid range.");

public:
    CircularMemoryLogger() = default;
    CircularMemoryLogger(const CircularMemoryLogger&) = delete;
    CircularMemoryLogger& operator=(const CircularMemoryLogger&) = delete;

    void Log(const T& item)
    {
        // On the first call m_index + 1 will cause overflow and be equal to 0.
        m_index = (m_index + 1) % Size;
        m_log[m_index] = item;
    }

    const T* Last() const
    {
        if (m_index != c_emptySentinel)
        {
            return &m_log[m_index];
        }
        else
        {
            return nullptr;
        }
    }

private:
    std::array<T, Size> m_log {};
    std::uint16_t m_index = c_emptySentinel;
};
