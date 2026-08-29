// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace CommonUtilities
{
    // Helper for aggregating hash values.  Adopted from boost::hash_combine.

    template <class T>
    inline void hash_combine(_Inout_ std::size_t& seed, _In_ const T& v)
    {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}