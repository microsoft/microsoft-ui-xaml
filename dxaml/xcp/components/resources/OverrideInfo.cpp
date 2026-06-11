// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <OverrideInfo.h>
#include <algorithm>

using namespace Resources::ScopedResources;

void OverrideInfo::Add(
    KnownPropertyIndex index,
    const xstring_ptr& key)
{
    ASSERT(!Has(index) || Get(index).Equals(key));

    for (size_t i = 0; i < m_indices.size(); ++i)
    {
        if (m_indices[i] == KnownPropertyIndex::UnknownType_UnknownProperty)
        {
            m_indices[i] = index;
            m_keys[i] = key;
            return;
        }
    }

    XAML_FAIL_FAST(); // did not find an empty spot...
}

bool OverrideInfo::Has(
    KnownPropertyIndex index) const
{
    return std::find(
        std::begin(m_indices),
        std::end(m_indices),
        index) != std::end(m_indices);
}

xstring_ptr OverrideInfo::Get(
    KnownPropertyIndex index) const
{
    for (size_t i = 0; i < m_indices.size(); ++i)
    {
        if (m_indices[i] == index)
        {
            return m_keys[i];
        }
    }

    return xstring_ptr::NullString();
}

void OverrideInfo::Clear(
    size_t index)
{
    m_indices[index] = KnownPropertyIndex::UnknownType_UnknownProperty;
}

size_t OverrideInfo::Count() const
{
    return std::count_if(
        std::begin(m_indices),
        std::end(m_indices),
        [](KnownPropertyIndex index) -> bool
        {
            return index != KnownPropertyIndex::UnknownType_UnknownProperty;
        });
}