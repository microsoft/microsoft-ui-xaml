// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "IndexRange.h"

IndexRange::IndexRange(int begin, int end)
{
    // Accept out of order begin/end pairs, just swap them.
    if (begin > end)
    {
        const int temp = begin;
        begin = end;
        end = temp;
    }

    MUX_ASSERT(begin <= end);

    m_begin = begin;
    m_end = end;
}

int IndexRange::Begin() const
{
    return m_begin;
}

int IndexRange::End() const
{
    return m_end;
}

bool IndexRange::Contains(int index) const
{
    return index >= m_begin && index <= m_end;
}

bool IndexRange::Split(int splitIndex, IndexRange& before, IndexRange& after)
{
    MUX_ASSERT(Contains(splitIndex));

    bool afterIsValid;

    before = IndexRange(m_begin, splitIndex);
    if (splitIndex < m_end)
    {
        after = IndexRange(splitIndex + 1, m_end);
        afterIsValid = true;
    }
    else
    {
        after = IndexRange();
        afterIsValid = false;
    }

    return afterIsValid;
}

bool IndexRange::Intersects(const IndexRange& other) const
{
    return ((m_begin <= other.End()) && (m_end >= other.Begin()));
}

bool IndexRange::operator==(const IndexRange& rhs) const
{
    return m_begin == rhs.m_begin && m_end == rhs.m_end;
}
