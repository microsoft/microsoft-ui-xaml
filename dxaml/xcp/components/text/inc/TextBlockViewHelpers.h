// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Utility functions for TextBlockView and RichTextBlockView

#pragma once

#include "Inline.h"

class TextBlockViewHelpers
{

public:

    static const int PlaceHolderPositionsForInlines = 2;
    static const int PlaceHolderOpenSpan = 1;
    static const int PlaceHolderCloseSpan = 1;

    // TextBlocks and RichTextBlocks contain several hidden characters. This function 
    // takes a number of visible characters (charCount) and translates it to the actual
    // character position (adjusted for hidden positions) in the block. 
    // Returns true if adjustment is complete, returns false if there is more to do. 
    static bool AdjustPositionByCharacterCount(
        _In_ CInlineCollection *inlines,
        _Inout_ int &charCount,
        _Inout_ int &adjustedPosition);

    // Takes a number of positions (visible and hidden) and returns the character index
    // at that position.
    static bool AdjustCharacterIndexByPosition(
        _In_ CInlineCollection *inlines,
        _Inout_ int &charCount,
        _Inout_ int &position);

    static bool IsCSpanType(KnownTypeIndex typeIndex);

    static bool FindIUCPositionInInlines(
        _In_ CInlineCollection* inlines,
        _In_ CInlineUIContainer* iuc,
        _Inout_ uint32_t &positionOfIUC);

};