// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextNavigationHelper.h"
#include "PlainTextPosition.h"
#include "TextRangeAdapter.h"
#include "TextAdapter.h"
#include "RichTextBlockView.h"

_Check_return_ HRESULT TextNavigationHelper::MoveByCharacter(
    _In_ int count,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    uint32_t moveCount = 0;
    IFCFAILFAST(CTextRangeAdapter::MoveByCharacter(count, &moveCount, plainTextPosition));

    // Change the gravity to always be LineForwardCharacterForward when moving by character.
    bool foundPosition;
    IFCFAILFAST(plainTextPosition->GetPositionAtOffset(0, LineForwardCharacterForward, &foundPosition, plainTextPosition));

    return S_OK;
}

_Check_return_ HRESULT TextNavigationHelper::MoveByWord(
    _In_ int count,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    CPlainTextPosition origPosition = *plainTextPosition;
    uint32_t origOffset;
    IFCFAILFAST(origPosition.GetOffset(&origOffset));

    uint32_t moveCount = 0;
    IFCFAILFAST(CTextRangeAdapter::MoveByWord(count, &moveCount, plainTextPosition));

    uint32_t newOffset;
    IFCFAILFAST(plainTextPosition->GetOffset(&newOffset));

    // If the position didn't move or only moved by one, then it is possible MoveByWord got stuck
    // on an explicit newline.  Call MoveByCharacter, which does a better job advancing in this case.
    uint32_t posCount = (newOffset > origOffset) ? (newOffset - origOffset) : (origOffset - newOffset);
    if (posCount <= 1)
    {
        *plainTextPosition = origPosition;
        IFCFAILFAST(MoveByCharacter(count > 0 ? 1 : -1, plainTextPosition));

        if (count > 0)
        {
            // When moving forward, skip any hidden characters in that direction.
            IFCFAILFAST(TextNavigationHelper::SkipHiddenCharacters(true /*isMoveForward*/, plainTextPosition));
            return S_OK;
        }
        else
        {
            // When moving backward, check if the MoveByCharacter call went farther than MoveByWord, and if it did,
            // then move back over the character and try MoveByWord again, to move to the start of the word now
            // that the hidden characters have been skipped by MoveByCharacter.
            uint32_t newOffsetByChar;
            IFCFAILFAST(plainTextPosition->GetOffset(&newOffsetByChar));
            if (newOffsetByChar < newOffset)
            {
                IFCFAILFAST(MoveByCharacter(1, plainTextPosition));
                IFCFAILFAST(CTextRangeAdapter::MoveByWord(count, &moveCount, plainTextPosition));
            }
        }
    }

    // CTextRangeAdapter::MoveByWord may move beyond placeholder positions at the start or end of the text.
    // Call SkipHiddenCharacters to move a character and back, starting in the opposite direction as count,
    // to ensure the position is tight to a real character, rather than out in a placeholder position.
    IFCFAILFAST(TextNavigationHelper::SkipHiddenCharacters(count < 0, plainTextPosition));

    // Change the gravity to always be LineForwardCharacterForward when moving by word.
    bool foundPosition;
    IFCFAILFAST(plainTextPosition->GetPositionAtOffset(0, LineForwardCharacterForward, &foundPosition, plainTextPosition));

    return S_OK;
}

_Check_return_ HRESULT TextNavigationHelper::MoveToStartOrEndOfLine(
    _In_ bool isMoveToEnd,
    _In_ CDependencyObject* textOwnerObject,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    CREATEPARAMETERS cp(textOwnerObject->GetContext());
    cp.m_value.WrapObjectNoRef(textOwnerObject);
    xref_ptr<CTextAdapter> textAdapter;
    xref_ptr<CTextRangeAdapter> textRangeAdapter;
    IFC_RETURN(CTextAdapter::Create((CDependencyObject**)textAdapter.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(textAdapter->GetDocumentRange(textRangeAdapter.ReleaseAndGetAddressOf()));
    CTextRangeAdapter::MoveToLine(isMoveToEnd, textRangeAdapter, plainTextPosition);
    return S_OK;
}

_Check_return_ HRESULT TextNavigationHelper::MoveToStartOrEndOfContent(
    _In_ bool isMoveToEnd,
    _In_ CDependencyObject* textOwnerObject,
    _Inout_ CPlainTextPosition* plainTextPosition
)
{
    if (textOwnerObject)
    {
        xref_ptr<CTextPointerWrapper> textPointerWrapper;
        if (isMoveToEnd)
        {
            if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
            {
                CTextBlock* textObject = do_pointer_cast<CTextBlock>(textOwnerObject);
                IFCFAILFAST(textObject->GetContentEnd(textPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
            {
                CRichTextBlock* textObject = do_pointer_cast<CRichTextBlock>(textOwnerObject);
                CRichTextBlockOverflow *overflow = nullptr;
                CRichTextBlockOverflow *targetUIElement = nullptr;
                overflow = static_cast<CRichTextBlockOverflow *>(textObject->GetNext());
                while (overflow != nullptr) // if richtextblockoverflow exist go to the last overflow
                {
                    targetUIElement = overflow;
                    overflow = static_cast<CRichTextBlockOverflow *>(overflow->GetNext());
                }
                if (targetUIElement != nullptr)  // if richtextblockoverflow exist
                {
                    IFCFAILFAST(targetUIElement->GetContentEnd(textPointerWrapper.ReleaseAndGetAddressOf()));
                }
                else   //only richtextblock
                {
                    IFCFAILFAST(textObject->GetContentEnd(textPointerWrapper.ReleaseAndGetAddressOf()));
                }
            }
            else if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
            {
                CRichTextBlockOverflow* textObject = do_pointer_cast<CRichTextBlockOverflow>(textOwnerObject);
                CRichTextBlockOverflow* targetUIElement = nullptr;
                while (textObject != nullptr)     // if richtextblockoverflow exist go to the last overflow
                {
                    targetUIElement = textObject;
                    textObject = static_cast<CRichTextBlockOverflow *>(textObject->GetNext());
                }
                IFCFAILFAST(targetUIElement->GetContentEnd(textPointerWrapper.ReleaseAndGetAddressOf()));
            }
            *plainTextPosition = textPointerWrapper->GetPlainTextPosition();
            IFCFAILFAST(TextNavigationHelper::SkipHiddenCharacters(false, plainTextPosition));
        }
        else
        {
            if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::TextBlock)
            {
                CTextBlock* textObject = do_pointer_cast<CTextBlock>(textOwnerObject);
                IFCFAILFAST(textObject->GetContentStart(textPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::RichTextBlock)
            {
                CRichTextBlock* textObject = do_pointer_cast<CRichTextBlock>(textOwnerObject);
                IFCFAILFAST(textObject->GetContentStart(textPointerWrapper.ReleaseAndGetAddressOf()));
            }
            else if (textOwnerObject->GetTypeIndex() == KnownTypeIndex::RichTextBlockOverflow)
            {
                CRichTextBlockOverflow* textObject = do_pointer_cast<CRichTextBlockOverflow>(textOwnerObject);
                RichTextServices::ILinkedTextContainer* currentUIElement = textObject;
                RichTextServices::ILinkedTextContainer* targetUIElement = nullptr;
                while (currentUIElement != nullptr)     // if richtextblockoverflow, go the the beginning richtextblock
                {
                    targetUIElement = currentUIElement;
                    currentUIElement = currentUIElement->GetPrevious();
                }
                IFCFAILFAST(static_cast<CRichTextBlock*>(targetUIElement)->GetContentStart(textPointerWrapper.ReleaseAndGetAddressOf()));
            }
            *plainTextPosition = textPointerWrapper->GetPlainTextPosition();
            IFCFAILFAST(TextNavigationHelper::SkipHiddenCharacters(true, plainTextPosition));

            // Change the gravity to be LineForwardCharacterForward for this beginning of selection.
            bool foundPosition;
            IFCFAILFAST(plainTextPosition->GetPositionAtOffset(0, LineForwardCharacterForward, &foundPosition, plainTextPosition));
        }
    }
    return S_OK;
}

// This function would move text position up or down by a line in that textview.
// It will keep in the same X coordinate and move Y corridinate.
// if no line above or below, it won't move.
_Check_return_ HRESULT TextNavigationHelper::MoveToLineUpOrDownPosition(
    _In_ bool isMoveDown,
    _In_ CDependencyObject* textOwnerObject,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    XRECTF currentRect;
    EmptyRectF(&currentRect);
    XUINT32 textOffset;
    TextGravity gravity;

    CPlainTextPosition origPosition = *plainTextPosition;

    // It's possible the given text position is past the end of the visible area, which will cause us to not
    // be able to find the owning object.  Thus we move the text position back if this is the case.
    IFCFAILFAST(plainTextPosition->GetOffset(&textOffset));
    if (textOffset > 0)
    {
        bool found;
        IFCFAILFAST(plainTextPosition->IsAtInsertionPosition(&found));
        if (!found)
        {
            IFCFAILFAST(plainTextPosition->GetPreviousInsertionPosition(&found, plainTextPosition));
        }
    }

    textOwnerObject = FindOwnerForPosition(textOwnerObject, plainTextPosition);
    if (textOwnerObject != nullptr)
    {
        XFLOAT currentLineOffset;
        IFCFAILFAST(plainTextPosition->GetOffset(&textOffset));
        IFCFAILFAST(plainTextPosition->GetGravity(&gravity));
        ITextView* textView = CTextAdapter::GetTextView(textOwnerObject);
        IFCFAILFAST(textView->TextPositionToPixelPosition(
            textOffset,
            gravity,
            &currentRect.X,
            nullptr,
            nullptr,
            &currentRect.Y,
            &currentRect.Height,
            nullptr,
            &currentLineOffset));
        currentRect.X += currentLineOffset;
    }
    else
    {
        IFCFAILFAST(E_FAIL);
    }

    if (isMoveDown)
    {
        //move to the end of this line
        IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfLine(true, textOwnerObject, plainTextPosition));
        //move one to the beginning of nextline. This is used to extract the Y coordinate of that line.
        IFCFAILFAST(TextNavigationHelper::MoveByCharacter(1, plainTextPosition));
    }
    else
    {
        IFCFAILFAST(TextNavigationHelper::MoveToStartOrEndOfLine(false, textOwnerObject, plainTextPosition));
        IFCFAILFAST(TextNavigationHelper::MoveByCharacter(-1, plainTextPosition));
    }

    // Update to the offset/gravity being used for the new position
    textOwnerObject = FindOwnerForPosition(textOwnerObject, plainTextPosition);
    if (textOwnerObject != nullptr)
    {
        IFCFAILFAST(plainTextPosition->GetGravity(&gravity));
        IFCFAILFAST(plainTextPosition->GetOffset(&textOffset));
        XRECTF newRect;
        EmptyRectF(&newRect);
        XFLOAT newLineOffset;
        ITextView* textView = CTextAdapter::GetTextView(textOwnerObject);
        IFCFAILFAST(textView->TextPositionToPixelPosition(
            textOffset,
            gravity,
            &newRect.X,
            nullptr,
            nullptr,
            &newRect.Y,
            &newRect.Height,
            nullptr,
            &newLineOffset));

        XPOINTF pixelCoordinate;
        pixelCoordinate.x = currentRect.X;
        pixelCoordinate.y = newRect.Y + newRect.Height/2;
        XUINT32 resultTextOffset;
        IFCFAILFAST(textView->PixelPositionToTextPosition(pixelCoordinate, TRUE, &resultTextOffset, &gravity));

        *plainTextPosition = CPlainTextPosition(plainTextPosition->GetTextContainer(), resultTextOffset, gravity);
    }
    else    // if not find the position, don't move
    {
        *plainTextPosition = origPosition;
    }
    return S_OK;
}

// Given a potential "owner" for a text position, determine the actual owner, searching across
// linked RichTextBlockOverflows if necessary.
_Ret_maybenull_ CDependencyObject* TextNavigationHelper::FindOwnerForPosition(
    _In_ CDependencyObject* textOwnerCandidate,
    _In_ CPlainTextPosition* plainTextPosition
    )
{
    XUINT32 textOffset;
    TextGravity gravity;
    IFCFAILFAST(plainTextPosition->GetOffset(&textOffset));
    IFCFAILFAST(plainTextPosition->GetGravity(&gravity));

    // Start with RichTextBlock if the candidate is an overflow.
    CDependencyObject* master = textOwnerCandidate;
    if (textOwnerCandidate->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        master = static_cast<CRichTextBlockOverflow*>(textOwnerCandidate)->GetMaster();
    }

    // See if we can early detect a hit in this "master" element.
    ITextView* textView = CTextAdapter::GetTextView(master);
    bool found = false;
    IFCFAILFAST(textView->ContainsPosition(textOffset, gravity, &found));
    if (found)
    {
        return master;
    }

    if (textOwnerCandidate->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        // If we get here, we've already searched the TextBlock, which has no overflows, no hit.
        return nullptr;
    }

    // Now search RichTextBlock's linked overflows
    ASSERT(master->OfTypeByIndex<KnownTypeIndex::RichTextBlock>());
    CRichTextBlock *richTextBlock = static_cast<CRichTextBlock*>(master);
    CRichTextBlockOverflow *overflow = static_cast<CRichTextBlockOverflow *>(richTextBlock->GetNext());

    while (overflow != nullptr)
    {
        textView = static_cast<ITextView*>(overflow->GetSingleElementTextView());
        IFCFAILFAST(textView->ContainsPosition(textOffset, gravity, &found));

        if (found)
        {
            return overflow;
        }

        overflow = static_cast<CRichTextBlockOverflow *>(overflow->GetNext());
    }

    return nullptr;
}

_Check_return_ HRESULT TextNavigationHelper::SkipHiddenCharacters(
    _In_ bool isMoveForward,
    _Inout_ CPlainTextPosition* plainTextPosition
    )
{
    uint32_t moveCount = 0;
    if (isMoveForward)
    {
        // move forward by one char, this will skip the hidden characters
        IFCFAILFAST(CTextRangeAdapter::MoveByCharacter(1, &moveCount, plainTextPosition));
        // if the above call moved, then move back by one char, the result of these two steps skips the hidden characters.
        if (moveCount > 0)
        {
            IFCFAILFAST(CTextRangeAdapter::MoveByCharacter(-1, &moveCount, plainTextPosition));
        }
    }
    else
    {
        IFCFAILFAST(CTextRangeAdapter::MoveByCharacter(-1, &moveCount, plainTextPosition));
        // if the above call moved, then move back by one char, the result of these two steps skips the hidden characters.
        if (moveCount > 0)
        {
            IFCFAILFAST(CTextRangeAdapter::MoveByCharacter(1, &moveCount, plainTextPosition));
        }
    }
    return S_OK;
}
