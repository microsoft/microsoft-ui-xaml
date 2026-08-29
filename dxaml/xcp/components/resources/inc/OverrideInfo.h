// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <array>
#include <xstring_ptr.h>
#include <Indexes.g.h>

namespace Resources { namespace ScopedResources
{
    class OverrideInfo
    {
    public:
        static constexpr size_t c_maxOverridesPerType = 2;

        void Add(
            KnownPropertyIndex index,
            const xstring_ptr& key);

        bool Has(
            KnownPropertyIndex index) const;

        xstring_ptr Get(
            KnownPropertyIndex index) const;

        auto At(
            size_t index) const
        {
            return std::make_pair(m_indices[index], m_keys[index]);
        }

        void Clear(
            size_t index);

        size_t Count() const;

    private:
        std::array<KnownPropertyIndex, c_maxOverridesPerType> m_indices =
        {
            KnownPropertyIndex::UnknownType_UnknownProperty,
            KnownPropertyIndex::UnknownType_UnknownProperty
        };

        std::array<xstring_ptr, c_maxOverridesPerType> m_keys;
    };
} }