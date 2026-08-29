// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <map>
#include <functional>
#include <memory>
#include "TextHighlightMerge.h"
#include "HighlightRegion.h"

void TextHighlightMerge::AddRegion(
    _In_ std::shared_ptr<HighlightRegion> highlightRegion
    )
{
    // The most recent region always wins.
    ASSERT(highlightRegion->endIndex >= highlightRegion->startIndex);

    // OPTIMIZE: Doing a find which is logn followed by an emplace which is logn
    //                     is not the most optimal way of doing things.  The reason it is done this way
    //                     is because emplace will remove the previous entry if there is a collision.  See
    //                     about making an emplace that will return the previous entry.
    auto emplaceIter = m_regions.find(highlightRegion->startIndex);

    if (emplaceIter == m_regions.end())
    {
        // Element doesn't exist yet, emplace it
        auto emplaceRet = m_regions.emplace(highlightRegion->startIndex, std::move(highlightRegion));
        emplaceIter = emplaceRet.first;
        auto emplacedHighlightRegion = emplaceIter->second.get();

        // First look at the item immediately before this one.
        // Since nothing can overlap the previous item, only one previous item
        // needs to be checked.
        if (emplaceIter != m_regions.begin())
        {
            auto prevIter = emplaceIter;
            prevIter--;

            auto currentHighlightRegion = prevIter->second.get();

            // Check for partial overlap case
            if (emplacedHighlightRegion->startIndex <= currentHighlightRegion->endIndex)
            {
                // Handle partial overlap case
                // === Example ===
                // BEFORE: ...BBB...
                // AFTER:  ...BBRR..

                if (emplacedHighlightRegion->endIndex < currentHighlightRegion->endIndex)
                {
                    // The new range bisects the previous range so another range is necessary after
                    // the new range is complete.  It is safe to add to the map here since it will
                    // be placed after prevIter due to key ordering for std::map.
                    // === Example ===
                    // BEFORE: ...BBBB...
                    // AFTER:  ...BRRB...
                    auto newRegion = std::make_shared<HighlightRegion>(
                        emplacedHighlightRegion->endIndex + 1,
                        currentHighlightRegion->endIndex,
                        currentHighlightRegion->foregroundBrush,
                        currentHighlightRegion->backgroundBrush);

                    m_regions.emplace(newRegion->startIndex, std::move(newRegion));
                }

                currentHighlightRegion->endIndex = emplacedHighlightRegion->startIndex - 1;

                ASSERT(currentHighlightRegion->endIndex >= currentHighlightRegion->startIndex);
            }
        }
    }
    else
    {
        auto emplacedHighlightRegion = emplaceIter->second.get();

        // There as an index collision so split the node if necessary.
        // === Example ===
        // BEFORE: ...BBB...
        // AFTER:  ...RRB...
        if (highlightRegion->endIndex < emplacedHighlightRegion->endIndex)
        {
            auto newRegion = std::make_shared<HighlightRegion>(
                highlightRegion->endIndex + 1,
                emplacedHighlightRegion->endIndex,
                emplacedHighlightRegion->foregroundBrush,
                emplacedHighlightRegion->backgroundBrush);

            m_regions.emplace(newRegion->startIndex, std::move(newRegion));
        }

        // Move the new highlight region into the found node
        emplaceIter->second = std::move(highlightRegion);
    }

    // Next look after and do any necessary region splits
    auto emplacedHighlightRegion = emplaceIter->second.get();

    auto nextIter = emplaceIter;
    nextIter++;
    while (nextIter != m_regions.end())
    {
        auto currentHighlightRegion = nextIter->second.get();

        if (emplacedHighlightRegion->endIndex >= currentHighlightRegion->endIndex)
        {
            // The highlighter completely occludes this region, so add it to the removal list.
            // === Example ===
            // BEFORE: ...BBB...
            // AFTER:  ..RRRRR..
            nextIter = m_regions.erase(nextIter);
        }
        else
        {
            // Check for partial overlap case
            // === Example ===
            // BEFORE: ...BBB...
            // AFTER:  ..RRBB...
            if (emplacedHighlightRegion->endIndex >= currentHighlightRegion->startIndex)
            {

                auto newRegion = std::make_shared<HighlightRegion>(
                    emplacedHighlightRegion->endIndex + 1,
                    currentHighlightRegion->endIndex,
                    currentHighlightRegion->foregroundBrush,
                    currentHighlightRegion->backgroundBrush);
                
                // Replace the current entry with a new insertion
                m_regions.erase(nextIter);
                m_regions.emplace(newRegion->startIndex, std::move(newRegion));

                // NOTE: It is not possible to bi-sect an element that follows after because the
                //       following regions are guaranteed to be greater based on the incremental
                //       nature of the algorithm guaranteeing no overlaps.
            }

            // End the iteration since endIndex cannot intersect anymore ranges
            break;
        }
    }
}
