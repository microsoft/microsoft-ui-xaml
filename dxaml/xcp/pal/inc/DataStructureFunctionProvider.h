// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StaticAssertFalse.h"

enum class KnownPropertyIndex : UINT16;

// and defaults only.
// This is for declarations of core (language) types
//
// Add template specializations for your specific classes in their own header file
// not in here.

template<class T>
struct DataStructureFunctionProvider
{
    static XUINT32 Hash(const T& data)
    {
        // You must provide a template specialization for your types.
        static_assert_false("DataStructureFunctionProvider_Hash___No_Specialization_Provided");
    }

    static bool AreEqual(const T& lhs, const T& rhs)
    {
        static_assert_false("DataStructureFunctionProvider_AreEqual___No_Specialization_Provided");
    }
};

// Default provider for ints
template<>
struct DataStructureFunctionProvider<XUINT32>
{
    static XUINT32 Hash(const XUINT32& data)
    {
        return data;
    }

    static bool AreEqual(const XUINT32& lhs, const XUINT32& rhs)
    {
        return lhs == rhs;
    }
};

// Default provider for KnownPropertyIndexes
template<>
struct DataStructureFunctionProvider<KnownPropertyIndex>
{
    static XUINT32 Hash(const KnownPropertyIndex& data)
    {
        return static_cast<XUINT32>(data);
    }

    static bool AreEqual(const KnownPropertyIndex& lhs, const KnownPropertyIndex& rhs)
    {
        return lhs == rhs;
    }
};

// Default provider for void*
template<>
struct DataStructureFunctionProvider<void*>
{
    static XUINT32 Hash(const void* const& data)
    {
        // fold the pointer into 32 bits
        return (XUINT32)((XUINT64)(size_t)data) ^ (((XUINT64)(size_t)data) >> 32);
    }

    static bool AreEqual(const void* const& lhs, const void* const& rhs)
    {
        return lhs == rhs;
    }
};

template<>
struct DataStructureFunctionProvider<xstring_ptr>
{
    static XUINT32 Hash(const xstring_ptr& data)
    {
        XUINT32 count, hash = 0;
        const WCHAR* buffer = data.GetBufferAndCount(&count);

        for (XUINT32 i = 0; i < count; i++)
        {
            hash = static_cast<XUINT32>(buffer[i]) + (hash << 6) + (hash << 16) - hash;
        }

        return hash;
    }

    static bool AreEqual(const xstring_ptr& lhs, const xstring_ptr& rhs)
    {
        return ((lhs.GetCount() == rhs.GetCount()) && lhs.Equals(rhs));
    }
};