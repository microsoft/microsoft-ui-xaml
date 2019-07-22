// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Inclusive range [start, end]
struct IndexRange
{
public:
    IndexRange() = default;
    IndexRange(int begin, int end);
    [[nodiscard]] int Begin() const;
    [[nodiscard]] int End() const;
    [[nodiscard]] bool Contains(int index) const;
    bool Split(int splitIndex, IndexRange& before, IndexRange& after);
    [[nodiscard]] bool Intersects(const IndexRange& other) const;
    bool operator==(const IndexRange& rhs) const;

private:
    // Invariant: m_end >= m_begin
    int m_begin = -1;
    int m_end = -1;
};