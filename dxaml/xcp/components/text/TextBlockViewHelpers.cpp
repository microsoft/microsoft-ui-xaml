// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBlockViewHelpers.h"
#include "Inline.h"
#include "DOPointerCast.h"
#include "InlineUIContainer.h"

bool TextBlockViewHelpers::AdjustPositionByCharacterCount(
    _In_ CInlineCollection *inlines,
    _Inout_ int &charCount,
    _Inout_ int &adjustedPosition)
{
    auto inlineCollection = inlines->GetCollection();
    for (auto inl : inlineCollection)
    {
        int curInlineLength;
        do_pointer_cast<CInline>(inl)->GetPositionCount(reinterpret_cast<unsigned int *>(&curInlineLength));

        auto inlType = inl->GetTypeIndex();
        // Look at Runs, LineBreaks (inlines that can't be nested)
        // to see if the position is contained in them
        if (inlType == DependencyObjectTraits<CRun>::Index ||
            inlType == DependencyObjectTraits<CLineBreak>::Index)
        {
            if (charCount == 0)
            {
                adjustedPosition += PlaceHolderPositionsForInlines;
                return true;
            }
            else if (charCount < (curInlineLength - PlaceHolderPositionsForInlines))
            {
                // Position is somewhere in this inline
                adjustedPosition += charCount + PlaceHolderPositionsForInlines;
                return true;
            }
            else
            {
                // Position is in another inline
                charCount -= curInlineLength - PlaceHolderPositionsForInlines;
                // Line separator character is a character, but not accounted for in curInlineLength
                if (inl->GetTypeIndex() == DependencyObjectTraits<CLineBreak>::Index)
                {
                    charCount -= 1;
                }
                adjustedPosition += curInlineLength;
            }
        }
        else if (inl->GetTypeIndex() == DependencyObjectTraits<CInlineUIContainer>::Index)
        {
            // InlineUIContainer does not have any characters when getting the text from the inline collection.
            // InlineUIContainer only has 2 positions - Open/Close.
            adjustedPosition += curInlineLength;
        }
        else
        {
            // Spans can contain more spans, but eventually will contain a run
            ASSERT(IsCSpanType(inl->GetTypeIndex()));
            adjustedPosition += PlaceHolderOpenSpan;
            bool adjustedPositionFound =
                AdjustPositionByCharacterCount(
                    static_cast<CSpan*>(inl)->GetInlineCollection(),
                    /* Inout */ charCount,
                    /* Inout */ adjustedPosition);
            if (adjustedPositionFound)
            {
                return true;
            }
            else
            {
                adjustedPosition += PlaceHolderCloseSpan;
            }
        }
    }

    // If the desired position is the very end of the inlineCollection, 
    // we don't yet realize we've found it, but we have.
    uint32_t totalPositions = 0;
    inlines->GetPositionCount(&totalPositions);
    if (charCount == 0 && adjustedPosition == (totalPositions - PlaceHolderPositionsForInlines))
    {
        return true;
    }

    return false;
}

bool TextBlockViewHelpers::AdjustCharacterIndexByPosition(
    _In_ CInlineCollection *inlines,
    _Inout_ int &charCount,
    _Inout_ int &position)
{
    auto inlineCollection = inlines->GetCollection();
    for (auto inl : inlineCollection)
    {
        int curInlineLength;
        static_cast<CInline*>(inl)->GetPositionCount(reinterpret_cast<unsigned int *>(&curInlineLength));

        auto inlType = inl->GetTypeIndex();
        // Look at Runs, LineBreaks (inlines that can't be nested)
        // to see if the character index is contained in them
        if (inlType == DependencyObjectTraits<CRun>::Index ||
            inlType == DependencyObjectTraits<CLineBreak>::Index)
        {
            if (position <= PlaceHolderPositionsForInlines)
            {
                // Return charCount as is
                return true;
            }
            else if (position <= curInlineLength)
            {
                // Character index is somewhere in this inline
                charCount += position - PlaceHolderPositionsForInlines;
                return true;
            }
            else
            {
                // Character index is in another inline
                position -= curInlineLength;
                charCount += (curInlineLength - PlaceHolderPositionsForInlines);
                // Line separator character is a character, but not accounted for in curInlineLength
                if (inl->GetTypeIndex() == DependencyObjectTraits<CLineBreak>::Index)
                {
                    charCount += 1;
                }
            }
        }
        else if (inlType == DependencyObjectTraits<CInlineUIContainer>::Index)
        {
            // InlineUIContainer does not have any characters when getting the text from the inline collection.
            // It has 2 positions - Open/Close.
            position -= curInlineLength;
        }
        else
        {
            // Spans can contain more spans, but eventually will contain a run
            ASSERT(IsCSpanType(inlType));
            position -= PlaceHolderOpenSpan;
            bool charIndexFound =
                AdjustCharacterIndexByPosition(
                    static_cast<CSpan*>(inl)->GetInlineCollection(),
                    /* Inout */ charCount,
                    /* Inout */ position);
            if (charIndexFound)
            {
                return true;
            }
            else
            {
                position -= PlaceHolderCloseSpan;
            }
        }
    }
    return false;
}

bool TextBlockViewHelpers::IsCSpanType(KnownTypeIndex typeIndex)
{
    if (typeIndex == DependencyObjectTraits<CSpan>::Index ||
        typeIndex == DependencyObjectTraits<CUnderline>::Index ||
        typeIndex == DependencyObjectTraits<CItalic>::Index ||
        typeIndex == DependencyObjectTraits<CBold>::Index ||
        typeIndex == DependencyObjectTraits<CHyperlink>::Index)
    {
        return true;
    }
    return false;
}

bool TextBlockViewHelpers::FindIUCPositionInInlines(
    _In_ CInlineCollection* inlines,
    _In_ CInlineUIContainer* iuc,
    _Inout_ uint32_t &positionOfIUC)
{
    auto inlineCollection = inlines->GetCollection();
    for (auto inl : inlineCollection)
    {
        int curInlineLength;
        do_pointer_cast<CInline>(inl)->GetPositionCount(reinterpret_cast<unsigned int *>(&curInlineLength));

        auto inlType = inl->GetTypeIndex();
        // Count positions in Runs, LineBreaks (inlines that can't be nested)
        if (inlType == DependencyObjectTraits<CRun>::Index ||
            inlType == DependencyObjectTraits<CLineBreak>::Index)
        {
            positionOfIUC += curInlineLength;
        }
        else if (inl->GetTypeIndex() == DependencyObjectTraits<CInlineUIContainer>::Index)
        {
            // Found the InlineUIContainer we're looking for
            if (do_pointer_cast<CInlineUIContainer>(inl) == iuc)
            {
                return true;
            }

            // InlineUIContainer does not have any characters when getting the text from the inline collection.
            // InlineUIContainer only has 2 positions - Open/Close
            positionOfIUC += curInlineLength;
        }
        else
        {
            // Spans can contain more spans
            ASSERT(IsCSpanType(inl->GetTypeIndex()));
            positionOfIUC += PlaceHolderOpenSpan;

            bool iucFound = FindIUCPositionInInlines(
                                static_cast<CSpan*>(inl)->GetInlineCollection(),
                                iuc,
                                /* Inout */ positionOfIUC);
            if (iucFound)
            {
                return true;
            }
            else
            {
                positionOfIUC += PlaceHolderCloseSpan;
            }
        }
    }
    return false;
}