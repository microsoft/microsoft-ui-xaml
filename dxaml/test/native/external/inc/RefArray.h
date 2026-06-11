// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>

// Raw/native arrays of ref class "hat pointers" leak, due to a compiler bug in Dev11
// This is still a useful construct to have, though, and the bug can be mitigated
// by setting each element of the array to null before it goes out of scope.
// That's the purpose of this RAII wrapper around std::array

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    template <class Ty, size_t Size>
    struct ref_array
    {
        std::array<Ty, Size> m_array;
        ~ref_array() { m_array.fill(nullptr); }

        // Wrappers to more easily facilitate things like range-based for
        auto begin() noexcept -> decltype(m_array.begin())
        {
            return m_array.begin();
        }
        auto begin() const noexcept -> decltype(m_array.begin())
        {
            return m_array.begin();
        }
        auto end() noexcept -> decltype(m_array.end())
        {
            return m_array.end();
        }
        auto end() const noexcept -> decltype(m_array.end())
        {
            return m_array.end();
        }
        auto rbegin() noexcept -> decltype(m_array.rbegin())
        {
            return m_array.rbegin();
        }
        auto rbegin() const noexcept -> decltype(m_array.rbegin())
        {
            return m_array.rbegin();
        }
        auto rend() noexcept -> decltype(m_array.rend())
        {
            return m_array.rend();
        }
        auto rend() const noexcept -> decltype(m_array.rend())
        {
            return m_array.rend();
        }
        auto cbegin() const noexcept -> decltype(m_array.cbegin())
        {
            return m_array.cbegin();
        }
        auto cend() const noexcept -> decltype(m_array.cend())
        {
            return m_array.cend();
        }
        auto crbegin() const noexcept -> decltype(m_array.crbegin())
        {
            return m_array.crbegin();
        }
        auto crend() const noexcept -> decltype(m_array.crend())
        {
            return m_array.crend();
        }
    };

} } } } }
