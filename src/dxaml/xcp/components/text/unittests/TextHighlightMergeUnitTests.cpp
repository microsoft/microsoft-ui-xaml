// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <map>
#include <functional>
#include <memory>
#include "TextHighlightMerge.h"
#include "TextHighlightMergeUnitTests.h"
#include "HighlightRegion.h"

class CSolidColorBrush;

namespace
{
    struct TextRegion
    {
        int startIndex = 0;
        int endIndex = 0;
        size_t foregroundBrushValue = 0;
        size_t backgroundBrushValue = 0;
    };

    void RegionValidator(
        _In_ const std::vector<TextRegion>& inputRegions,
        _In_ const std::vector<TextRegion>& outputRegions
        )
    {
        TextHighlightMerge merge;

        for (const auto& region : inputRegions)
        {
            merge.AddRegion(
                std::make_unique<HighlightRegion>(
                    region.startIndex,
                    region.endIndex,
                    reinterpret_cast<CSolidColorBrush*>(region.foregroundBrushValue),
                    reinterpret_cast<CSolidColorBrush*>(region.backgroundBrushValue)));
        }

        auto outputRegionIter = outputRegions.begin();
        auto dividerRegionIter = merge.begin();
        while (dividerRegionIter != merge.end())
        {
            VERIFY_ARE_NOT_EQUAL(outputRegionIter, outputRegions.end());

            VERIFY_ARE_EQUAL(outputRegionIter->startIndex, dividerRegionIter->second->startIndex);
            VERIFY_ARE_EQUAL(outputRegionIter->endIndex, dividerRegionIter->second->endIndex);

            VERIFY_ARE_EQUAL(
                reinterpret_cast<CSolidColorBrush*>(outputRegionIter->foregroundBrushValue),
                dividerRegionIter->second->foregroundBrush);
            VERIFY_ARE_EQUAL(
                reinterpret_cast<CSolidColorBrush*>(outputRegionIter->backgroundBrushValue),
                dividerRegionIter->second->backgroundBrush);

            outputRegionIter++;
            dividerRegionIter++;
        }

        VERIFY_ARE_EQUAL(outputRegionIter, outputRegions.end());
    }

    // Defines for easier readability of the code
    // Even #'s are foreground
    // Odd #'s are background
    static const int FIRST_FG = 0x1;
    static const int FIRST_BG = 0x2;

    static const int SECOND_FG = 0x3;
    static const int SECOND_BG = 0x4;

    static const int THIRD_FG = 0x5;
    static const int THIRD_BG = 0x6;

    static const int FOURTH_FG = 0x7;
    static const int FOURTH_BG = 0x8;

    static const int FIFTH_FG = 0x9;
    static const int FIFTH_BG = 0xA;
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Text {

// NOTE: All CSolidColorBrush's are objects but this unit test exploits to use
//       the pointers as raw values for unit testing purposes.

void TextHighlightMergeUnitTests::NonOverlapped()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 0, 1, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 5, 6, SECOND_FG, SECOND_BG });

    RegionValidator(inputRegions, inputRegions);
}

void TextHighlightMergeUnitTests::OverlapNextPartial()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 3, 8, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 3, 8, SECOND_FG, SECOND_BG });
    outputRegions.push_back({ 9, 10, FIRST_FG, FIRST_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapPrevPartial()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG});
    inputRegions.push_back({ 8, 12, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 5, 7, FIRST_FG, FIRST_BG });
    outputRegions.push_back({ 8, 12, SECOND_FG, SECOND_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapNextFull()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 3, 12, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 3, 12, SECOND_FG, SECOND_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapEqualFull()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 5, 12, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 5, 12, SECOND_FG, SECOND_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapEqualPartial()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 5, 8, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 5, 8, SECOND_FG, SECOND_BG });
    outputRegions.push_back({ 9, 10, FIRST_FG, FIRST_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapEqualMultiple()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 5, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 11, 20, SECOND_FG, SECOND_BG });
    inputRegions.push_back({ 21, 30, THIRD_FG, THIRD_BG });
    inputRegions.push_back({ 5, 25, FOURTH_FG, FOURTH_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 5, 25, FOURTH_FG, FOURTH_BG });
    outputRegions.push_back({ 26, 30, THIRD_FG, THIRD_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapInner()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 0, 10, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 4, 8, SECOND_FG, SECOND_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 0, 3, FIRST_FG, FIRST_BG });
    outputRegions.push_back({ 4, 8, SECOND_FG, SECOND_BG });
    outputRegions.push_back({ 9, 10, FIRST_FG, FIRST_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::Overlap1()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 0, 2, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 4, 6, SECOND_FG, SECOND_BG });
    inputRegions.push_back({ 2, 4, THIRD_FG, THIRD_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 0, 1, FIRST_FG, FIRST_BG });
    outputRegions.push_back({ 2, 4, THIRD_FG, THIRD_BG });
    outputRegions.push_back({ 5, 6, SECOND_FG, SECOND_BG });

    RegionValidator(inputRegions, outputRegions);
}

void TextHighlightMergeUnitTests::OverlapMultiple()
{
    std::vector<TextRegion> inputRegions;
    inputRegions.push_back({ 0, 5, FIRST_FG, FIRST_BG });
    inputRegions.push_back({ 8, 12, SECOND_FG, SECOND_BG });
    inputRegions.push_back({ 14, 20, THIRD_FG, THIRD_BG });
    inputRegions.push_back({ 20, 30, FOURTH_FG, FOURTH_BG });
    inputRegions.push_back({ 2, 25, FIFTH_FG, FIFTH_BG });

    std::vector<TextRegion> outputRegions;
    outputRegions.push_back({ 0, 1, FIRST_FG, FIRST_BG });
    outputRegions.push_back({ 2, 25, FIFTH_FG, FIFTH_BG });
    outputRegions.push_back({ 26, 30, FOURTH_FG, FOURTH_BG });

    RegionValidator(inputRegions, outputRegions);
}

} } } } }
