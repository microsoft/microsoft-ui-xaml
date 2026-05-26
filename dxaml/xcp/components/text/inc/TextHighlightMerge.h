// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>
#include <functional>
#include <memory>

class CSolidColorBrush;
class HighlightRegion;

// This algorithm is meant to take a series of linear text ranges and remove any overlap such
// that all text ranges do not start or end with an intersection.  This is done iteratively by
// adding regions to the class and then iterating over the map when complete.  The reason for doing
// this is so that foreground color highlighting chooses the correct color to generate for the
// glyph primitive.  There are multiple cases to consider which are tested in the unit tests
// and commented in the algorithm.
//
// Algorithmic decisions:
//      - The most recent region addition always occludes previous regions.
//      - All ranges are inclusive [Start,End] since it makes more conceptual sense.
//        For instance: [1,2] would highlight text index 1 and 2.  [1,1] would highlight text index 1.
//
// This algorithm is specifically built with std::map because it uses a red-black tree to
// insert items by key in sorted order and supports iterating over elements.  This allows
// the algorithm to work in amortized O(NlogN) time where N is the number of regions.  It also
// needs to be balanced in case all highlights are added in sequential order.
// TODO: This could be switched to std::set which might clean things up a bit.
//
// It is currently specific to TextHighlighter regions but it could be extended and templatized
// in the future with little work.
class TextHighlightMerge
{
public:

    // Types
    using MapType = std::map<int, std::shared_ptr<HighlightRegion>>;

    // Methods
    void AddRegion(_In_ std::shared_ptr<HighlightRegion> highlightRegion);

    // Iteration
    MapType::const_iterator begin() { return m_regions.begin(); }
    MapType::const_iterator end() { return m_regions.end(); }

private:

    MapType m_regions;
};