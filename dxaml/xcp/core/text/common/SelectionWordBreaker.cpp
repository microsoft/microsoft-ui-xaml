// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "UcdProperties.h"
#include "TextBoxHelpers.h"
#include "TextBlockViewHelpers.h"
#include <windows.data.text.h>
#include <windows.globalization.h>

using namespace RichTextServices;

//  Returns whether the passed character is considered a punctuation or symbol character
_Check_return_ XCP_FORCEINLINE XINT32 IsXamlPunctuationOrSymbol(XUINT32 character)
{
    auto category = UcdGetGeneralCategory(character);
    // This encompasses the following categories:
    // Pc, Pd, Pe, Pf, Pi, Po, Ps, Sc, Sk, Sm, So
    return (category >= GeneralCategoryPc) && (category <= GeneralCategorySo);
}


// The text backing store navigator is used to retrieve text content by walking the content tree.
// The class supports navigation of both the TextBox and RichText backing stores.
class CTextBackingStoreNavigator
{
public:
    CTextBackingStoreNavigator(_In_ ISimpleTextBackend *pTextBackend)
        : m_pTextBackendNoRef(pTextBackend)
    {
    }

    ~CTextBackingStoreNavigator()
    {
        m_pTextBackendNoRef = nullptr;
    }

    // Gets a text character from the backing store or
    // indicate that the position is at a breaking symbol.
    wchar_t GetCharacter()
    {
        wchar_t character = UNICODE_NEXT_LINE;

        uint32_t offset;
        HRESULT hr = m_navigatorPosition.GetOffset(&offset);
        if (SUCCEEDED(hr))
        {
            character = m_pTextBackendNoRef->GetCharacter(offset);
        }

        return character;
    }

    // Attempts to navigate to the next insertion position such that
    // a text character follows that new position.
    bool MoveNext()
    {
        bool fMoved = false;
        bool fIsAtEndOfContainer = false;
        HRESULT       hr = S_OK;
        CTextPosition newPosition;

        IGNOREHR(m_navigatorPosition.GetNextInsertionPosition(&fMoved, &newPosition));

        if (fMoved)
        {
            m_navigatorPosition = newPosition;
        }
        else
        {
            // GetNextInsertionPosition may return the same position we currently are located even if we're not at the last position, due to an issue in the
            // integration with line services (see BLUE:440033)
            // To work around this, we move to the last position of the block if fMoved is FALSE and we're not there already
            hr = IsAtOrAfterLastPositionOfContainer(&fIsAtEndOfContainer);
            if (!fIsAtEndOfContainer)
            {
                hr = MoveToLastPositionOfContainer();
            }

            if (FAILED(hr))
            {
                FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hr);
            }

            fMoved = !fIsAtEndOfContainer;
        }

        return fMoved;
    }

    // Attempts to navigate to the previous insertion position such that
    // a text character follows that new position.
    bool MovePrevious()
    {
        bool fMoved = false;
        CTextPosition newPosition;

        IGNOREHR(m_navigatorPosition.GetPreviousInsertionPosition(&fMoved, &newPosition));

        if (fMoved)
        {
            m_navigatorPosition = newPosition;
        }

        return fMoved;
    }

    // Return current navigator offset.
    CTextPosition& GetPosition()
    {
        return m_navigatorPosition;
    }

    // Move navigator position to new offset.
    void MoveTo(_In_ const CTextPosition &newPosition)
    {
        m_navigatorPosition = newPosition;
    }

private:
    // Check whether the navigator is located at or after the last position of the text container
    HRESULT IsAtOrAfterLastPositionOfContainer(_Out_ bool* atEnd)
    {
        uint32_t origOffset = 0;
        uint32_t positionCount = 0;
        const CPlainTextPosition& origPlainPosition = m_navigatorPosition.GetPlainPosition();
        ITextContainer* pNoRefContainer = origPlainPosition.GetTextContainer();

        IFC_RETURN(origPlainPosition.GetOffset(&origOffset));
        pNoRefContainer->GetPositionCount(&positionCount);

        *atEnd = ((origOffset + 1) >= positionCount) ? TRUE : FALSE;

        return S_OK;
    }

    // Move the navigator position to the last character of the text container
    HRESULT MoveToLastPositionOfContainer()
    {
        uint32_t positionCount = 0;
        TextGravity origGravity;
        const CPlainTextPosition& origPlainPosition = m_navigatorPosition.GetPlainPosition();
        ITextContainer* pNoRefContainer = origPlainPosition.GetTextContainer();

        pNoRefContainer->GetPositionCount(&positionCount);
        IFC_RETURN(origPlainPosition.GetGravity(&origGravity));

        ASSERT(positionCount > 0); // Because IsAtOrAfterLastPositionOfContainer will return TRUE and this function should then not be called
        m_navigatorPosition = CPlainTextPosition(pNoRefContainer, positionCount - 1, origGravity);

        return S_OK;
    }

private:
    ISimpleTextBackend *m_pTextBackendNoRef;
    CTextPosition       m_navigatorPosition;
};


// Given a potential break position and list of valid breaks, returns
// whether a break for text selection is allowed at that position.
bool CSelectionWordBreaker::IsSelectionBreak(
    wchar_t prevChar,
    wchar_t breakChar,
    CTextBackingStoreNavigator navigator,
    FindBoundaryType direction,
    const std::vector<uint32_t>& breakIndexes)
{
    WCHAR curChar = (direction == FindBoundaryType::Backward) ? prevChar : breakChar;
    bool canBreak = IsXamlNewline(curChar) || IsXamlPunctuationOrSymbol(curChar);

    if (!canBreak)
    {
        canBreak = CanBreak(navigator, breakIndexes);
    }

    return canBreak;
}

bool CSelectionWordBreaker::IsNavigationBreak(
    CTextBackingStoreNavigator navigator,
    CTextBackingStoreNavigator prevNavigator,
    bool fPrevSpace,
    const std::vector<uint32_t>& breakIndexes,
    bool isNonSpaced)
{
    if (!isNonSpaced)
    {
        // For English and any other language that uses spaces to separate words
        if (   IsPunctuationAtEndOfWord(navigator.GetCharacter(), fPrevSpace)
            || IsWordStartingWithPunctuation(navigator.GetCharacter(), fPrevSpace, prevNavigator.GetCharacter())
            || !CanBreak(navigator, breakIndexes) )
        {
            return false;
        }
    }
    else
    {
        // For Japanese, Chinese, and any other language that does not use spaces to separate words
        if (!CanBreak(navigator, breakIndexes))
        {
            return false;
        }
    }
    return true;
}

// Given a potential break position and list of valid breaks, returns
// whether a break is allowed at that position.
// TODO: make this more efficient than having the vector walked from the
// beginning every time the navigator is checking a break position.
bool CSelectionWordBreaker::CanBreak(
    CTextBackingStoreNavigator navigator,
    const std::vector<uint32_t>& breakIndexes)
{
    const CPlainTextPosition& plainPosition = navigator.GetPosition().GetPlainPosition();

    uint32_t curOffsetPosition = 0;
    IFCFAILFAST(plainPosition.GetOffset(&curOffsetPosition));

    int characterIndex = plainPosition.GetTextView()->GetCharacterIndex(curOffsetPosition);

    for (uint32_t i = 0; i < breakIndexes.size(); ++i)
    {
        if (breakIndexes[i] == characterIndex)
        {
            return true;
        }
    }
    return false;
}

// Given a position, find the next "by word" navigation boundary in the given direction.
//
// It returns the position right after the break (ie, the word boundary is in between
// the returned position and the previous one)
//
// NOTE: Wordbreaking is split between this function (for TextPattern) and
// GetAdjacentWordSelectionBoundary (for Text Selection). Changes in one of them might need to
// be reflected in the other function, depending on the nature of the change
_Check_return_ HRESULT CSelectionWordBreaker::GetAdjacentWordNavigationBoundary(
    _In_ CTextPosition currentPosition,
    _In_ ISimpleTextBackend *pTextBackend,
    _In_ FindBoundaryType findType,
    _Out_ CTextPosition *pBoundaryPosition)
{
    ASSERT(findType != FindBoundaryType::ForwardExact);

    HRESULT hr         = S_OK;

    // fPrevSpace gets set to TRUE when there is one or more whitespace characters between
    // the two consecutive non-whitespace characters we are considering as a break location
    bool fPrevSpace = false;
    bool fMoved = false;

    CTextBackingStoreNavigator navigator(pTextBackend);
    CTextBackingStoreNavigator prevNavigator(pTextBackend);

    navigator.MoveTo(currentPosition);

    // Get the container's text
    uint32_t totalCharacters = 0;
    const wchar_t *characters = nullptr;
    bool takeOwnershipOfContainersText = false;

    // Get the container's text
    IFC_RETURN(currentPosition.GetPlainPosition().GetTextContainer()->GetText(
        false /* insertNewlines */,
        &totalCharacters,
        &characters,
        &takeOwnershipOfContainersText));

    auto extraCleanup = wil::scope_exit([&] {
        if (takeOwnershipOfContainersText)
        {
            delete[] characters;
        }
    });

    HSTRING containerText = wrl::Wrappers::HStringReference(characters, totalCharacters).Get();

    const std::vector<uint32_t>& breakOffsets = GetTextSegments(containerText, currentPosition.GetPlainPosition().GetTextContainer());
    bool isNonSpaced = IsNonSpaceDelimitedLanguage(currentPosition.GetPlainPosition().GetTextContainer());

    if (IsForwardDirection(findType))
    {
        // Forwards
        prevNavigator.MoveTo(navigator.GetPosition());
        fMoved = navigator.MoveNext();

        // Move navigator to first non-whitespace character
        // Newlines are considered word breaks
        while (    fMoved
               && !IsXamlNewline(navigator.GetCharacter())
               &&  IsXamlWhitespace(navigator.GetCharacter()))
        {
            fPrevSpace = TRUE;
            fMoved = navigator.MoveNext();
        }

        // Consider this position for the line break, and keep going forward if not acceptable.
        // We're always checking two consecutive non-whitespace characters.
        while (     fMoved
               &&  !IsXamlNewline(navigator.GetCharacter())
               &&  !IsNavigationBreak(navigator, prevNavigator, fPrevSpace, breakOffsets, isNonSpaced)  )
        {
            // Try next position
            prevNavigator.MoveTo(navigator.GetPosition());
            fPrevSpace = FALSE;
            fMoved = navigator.MoveNext(); // Could hit EOL and return FALSE

            // Move navigator to first non-whitespace character
            // Newlines are considered word breaks
            while (    fMoved
                   && !IsXamlNewline(navigator.GetCharacter())
                   &&  IsXamlWhitespace(navigator.GetCharacter()))
            {
                fPrevSpace = TRUE;
                fMoved = navigator.MoveNext(); // Could hit EOL and return FALSE
            }
        }
    }
    else
    {
        // Backwards
        fMoved = navigator.MovePrevious();

        // Move navigator to first non-whitespace character
        // Newlines are considered word breaks
        while (    fMoved
               && !IsXamlNewline(navigator.GetCharacter())
               &&  IsXamlWhitespace(navigator.GetCharacter()))
        {
            fMoved = navigator.MovePrevious();    // Could hit BOL and return FALSE
        }

        if (    fMoved
            && !IsXamlNewline(navigator.GetCharacter()))
        {
            prevNavigator.MoveTo(navigator.GetPosition());
            fMoved = prevNavigator.MovePrevious();
            fPrevSpace = FALSE;

            // Consider this position for the line break, and keep going back if not acceptable.
            // We're always checking two consecutive non-whitespace characters.
            while (     fMoved
                   &&  !IsXamlNewline(prevNavigator.GetCharacter())
                   &&  !IsNavigationBreak(navigator, prevNavigator, fPrevSpace, breakOffsets, isNonSpaced)  )
            {
                // Try previous position
                navigator.MoveTo(prevNavigator.GetPosition());

                // We need to move navigator to a non-whitespace position, else we can have navigator in a whitespace character,
                // which means CanBreak would return non-desired breaks when there is an intervening whitespace
                // Both navigator and prevNavigator should be at non-whitespace characters
                while (    fMoved
                       &&  IsXamlWhitespace(navigator.GetCharacter()))
                {
                    fMoved = navigator.MovePrevious();    // Could hit BOL and return FALSE
                }

                fPrevSpace = FALSE;
                prevNavigator.MoveTo(navigator.GetPosition());
                fMoved = prevNavigator.MovePrevious();

                // Move prevNavigator to first non-whitespace character
                // Newlines are considered word breaks
                while (    fMoved
                       && !IsXamlNewline(prevNavigator.GetCharacter())
                       &&  IsXamlWhitespace(prevNavigator.GetCharacter()))
                {
                    fPrevSpace = TRUE;
                    fMoved = prevNavigator.MovePrevious();    // We could hit BOL and return FALSE
                }
            }
        }
    }

    *pBoundaryPosition = navigator.GetPosition();

    return hr;
}

// This help us group punctuation that's separate from a word as its own word.
// For example, in "a !!! b", "!!!" should be treated as its own word.
// This is different than punctuation at the end of a word, where it should be grouped with the word.
// For example: "This is a sentence." The period should be grouped with "sentence".
bool CSelectionWordBreaker::IsPunctuationAtEndOfWord(wchar_t character, bool fPrevSpace)
{
    return IsXamlPunctuationOrSymbol(character) && fPrevSpace == FALSE;
}

// This helps group puntuation at the start of a word with that word.
// For example, " *typo " or " ?Que "
bool CSelectionWordBreaker::IsWordStartingWithPunctuation(wchar_t character, bool fPrevSpace, wchar_t prevCharacter)
{
    // This check should only happen when navigators are on non-whitespace characters.
    ASSERT(!IsXamlWhitespace(character));

    if (!IsXamlPunctuationOrSymbol(prevCharacter) || fPrevSpace == TRUE)
    {
        return false;
    }
    if (IsXamlPunctuationOrSymbol(character) || IsXamlNewline(character))
    {
        return false;
    }
    return true;
}

// Given a position, find the next word boundary for selection in the given direction.
//
// It returns the position right after the break (ie, the word boundary is in between
// the returned position and the previous one)
//
// NOTE: Wordbreaking is split between this function (for Text Selection) and
// GetAdjacentWordNavigationBoundary (for TextPattern). Changes in one of them might need to
// be reflected in the other function, depending on the nature of the change
_Check_return_ HRESULT CSelectionWordBreaker::GetAdjacentWordSelectionBoundary(
    _In_ CTextPosition currentPosition,
    _In_ ISimpleTextBackend *pTextBackend,
    _In_ FindBoundaryType findType,
    _Out_ CTextPosition *pBoundaryPosition)
{
    HRESULT hr         = S_OK;

    // fPrevSpace gets set to TRUE when there is one or more whitespace characters between
    // the two consecutive non-whitespace characters we are considering as a break location
    bool fPrevSpace = false;
    bool fMoved = true;

    CTextBackingStoreNavigator navigator(pTextBackend);
    CTextBackingStoreNavigator prevNavigator(pTextBackend);

    navigator.MoveTo(currentPosition);

    // Get the container's text
    uint32_t totalCharacters = 0;
    const wchar_t *characters = nullptr;
    bool takeOwnershipOfContainersText = false;

    // Get the container's text
    IFC_RETURN(currentPosition.GetPlainPosition().GetTextContainer()->GetText(
        false /* insertNewlines */,
        &totalCharacters,
        &characters,
        &takeOwnershipOfContainersText));

    auto extraCleanup = wil::scope_exit([&] {
        if (takeOwnershipOfContainersText)
        {
            delete[] characters;
        }
    });

    HSTRING containerText = wrl::Wrappers::HStringReference(characters, totalCharacters).Get();

    const std::vector<uint32_t>& breakOffsets = GetTextSegments(containerText, currentPosition.GetPlainPosition().GetTextContainer());

    if (IsForwardDirection(findType))
    {
        // Forwards
        if (IsXamlPunctuationOrSymbol(navigator.GetCharacter()))
        {
            // A contiguous sequence of punctuation signs should be considered a whole word
            while (    fMoved
                   &&  IsXamlPunctuationOrSymbol(navigator.GetCharacter()))
            {
                fMoved = navigator.MoveNext();
            }
        }
        else
        {
            prevNavigator.MoveTo(navigator.GetPosition());

            fMoved = navigator.MoveNext();

            // Move navigator to first non-whitespace character
            // Newlines are considered word breaks
            while (    fMoved
                   && !IsXamlNewline(navigator.GetCharacter())
                   &&  IsXamlWhitespace(navigator.GetCharacter()))
            {
                fPrevSpace = TRUE;
                fMoved = navigator.MoveNext();
            }

            // Consider this position for the word break, and keep going
            // forward if not acceptable. We're always checking two consecutive non-whitespace characters (except in the first iteration if we started at whitespace)
            while (     fMoved
                    &&  !IsSelectionBreak(
                            prevNavigator.GetCharacter(),
                            navigator.GetCharacter(),
                            navigator,
                            findType,
                            breakOffsets))
            {
                // Try next position
                prevNavigator.MoveTo(navigator.GetPosition());
                fPrevSpace = FALSE;
                fMoved = navigator.MoveNext(); // Could hit EOL and return FALSE

                // Move navigator to first non-whitespace character
                // Newlines are considered word breaks
                while (    fMoved
                       && !IsXamlNewline(navigator.GetCharacter())
                       &&  IsXamlWhitespace(navigator.GetCharacter()))
                {
                    fPrevSpace = TRUE;
                    fMoved = navigator.MoveNext(); // Could hit EOL and return FALSE
                }
            }

            // If we want an exact boundary, move to right after prevNavigator if it isn't in whitespace
            // (because it is the last non-whitespace before the break if it isn't whitespace)
            // (if it is whitespace, it is because we started at it and we want to stop at the actual break)
            if ((findType == FindBoundaryType::ForwardExact) && !IsXamlWhitespace(prevNavigator.GetCharacter()))
            {
                navigator.MoveTo(prevNavigator.GetPosition());
                navigator.MoveNext();
            }
        }
    }
    else
    {
        // Backwards
        if (IsXamlPunctuationOrSymbol(navigator.GetCharacter()))
        {
            // A contiguous sequence of punctuation signs should be considered a whole word
            while (    fMoved
                   &&  IsXamlPunctuationOrSymbol(navigator.GetCharacter()))
            {
                fMoved = navigator.MovePrevious();
            }

            if (fMoved)
            {
                navigator.MoveNext();
            }
        }
        else
        {
            // Move navigator to first non-whitespace character
            // Newlines are considered word breaks
            while (    fMoved
                   && !IsXamlNewline(navigator.GetCharacter())
                   &&  IsXamlWhitespace(navigator.GetCharacter()))
            {
                fMoved = navigator.MovePrevious();    // Could hit BOL and return FALSE
            }

            if (    fMoved
                && !IsXamlNewline(navigator.GetCharacter()))
            {
                prevNavigator.MoveTo(navigator.GetPosition());
                fMoved = prevNavigator.MovePrevious();
                fPrevSpace = FALSE;

                // Consider this position for the word break, and keep going
                // back if not acceptable. We're always checking two consecutive non-whitespace characters
                while (     fMoved
                        &&  !IsSelectionBreak(
                                prevNavigator.GetCharacter(),
                                navigator.GetCharacter(),
                                navigator,
                                findType,
                                breakOffsets))
                {
                    // Try previous position
                    navigator.MoveTo(prevNavigator.GetPosition());
                    fPrevSpace = FALSE;
                    fMoved = prevNavigator.MovePrevious();

                    // Move navigator to first non-whitespace character
                    // Newlines are considered word breaks
                    while (    fMoved
                           && !IsXamlNewline(prevNavigator.GetCharacter())
                           &&  IsXamlWhitespace(prevNavigator.GetCharacter()))
                    {
                        fPrevSpace = TRUE;
                        fMoved = prevNavigator.MovePrevious();    // We could hit BOL and return FALSE
                    }
                }
            }
        }
    }

    *pBoundaryPosition = navigator.GetPosition();

    return hr;
}

// Given a position, find the next non-whitespace character in
// the given direction. Newlines here will also be considered whitespace
_Check_return_ HRESULT CSelectionWordBreaker::GetAdjacentNonWhitespaceCharacter(
    _In_ CTextPosition currentPosition,
    _In_ ISimpleTextBackend *pTextBackend,
    _In_ FindBoundaryType findType,
    _Out_ CTextPosition *pBoundaryPosition)
{
    bool fMoved = true;
    CTextBackingStoreNavigator navigator(pTextBackend);
    navigator.MoveTo(currentPosition);

    if (IsForwardDirection(findType))
    {
        // Forwards
        while (    fMoved
               &&  (IsXamlNewline(navigator.GetCharacter())
                 || IsXamlWhitespace(navigator.GetCharacter())))
        {
            fMoved = navigator.MoveNext();
        }
    }
    else
    {
        // Backwards
        while (    fMoved
               &&  (IsXamlNewline(navigator.GetCharacter())
                 || IsXamlWhitespace(navigator.GetCharacter())))
        {
            fMoved = navigator.MovePrevious();
        }
    }

    *pBoundaryPosition = navigator.GetPosition();

    return S_OK; // RRETURN_REMOVAL
}

void CSelectionWordBreaker::GetLanguage(_In_ ITextContainer *pTextContainer, _Out_ xstring_ptr &language)
{
    CValue value;
    IFCFAILFAST(pTextContainer->GetOwnerUIElement()->GetValueByIndex(KnownPropertyIndex::FrameworkElement_Language, &value));

    language = value.AsString();
    if (language.IsNullOrEmpty())
    {
        // If we can't determine a language, use "und" (undetermined) to use Unicode Standard language-neutral rules.
        IFCFAILFAST(xstring_ptr::CloneBuffer(L"und", &language));
    }
}

bool CSelectionWordBreaker::IsNonSpaceDelimitedLanguage(_In_ ITextContainer *pTextContainer)
{
    xstring_ptr language;
    GetLanguage(pTextContainer, language);

    if (language.StartsWith(XSTRING_PTR_EPHEMERAL(L"ja"), xstrCompareCaseInsensitive)  || // Japanese
        language.StartsWith(XSTRING_PTR_EPHEMERAL(L"zh"), xstrCompareCaseInsensitive)  || // Chinese
        language.StartsWith(XSTRING_PTR_EPHEMERAL(L"th"), xstrCompareCaseInsensitive)  || // Thai
        language.StartsWith(XSTRING_PTR_EPHEMERAL(L"ko-"), xstrCompareCaseInsensitive) || // Korean -- Need to differentiate from Konkani (kok)
        language.Equals(XSTRING_PTR_EPHEMERAL(L"ko"), xstrCompareCaseInsensitive))
    {
        return true;
    }
    return false;
}

wrl::ComPtr<wda::Text::ISelectableWordsSegmenter> CSelectionWordBreaker::GetWordsSegmenter(_In_ ITextContainer *pTextContainer)
{
    wrl::ComPtr<wda::Text::ISelectableWordsSegmenterFactory> wordsSegmenterFactory;
    wrl::ComPtr<wda::Text::ISelectableWordsSegmenter> wordsSegmenter;

    wrl_wrappers::HString hsClassName;
    IFCFAILFAST(hsClassName.Set(RuntimeClass_Windows_Data_Text_SelectableWordsSegmenter));

    IFCFAILFAST(wf::GetActivationFactory(
        hsClassName.Get(),
        &wordsSegmenterFactory));

    xstring_ptr language;
    GetLanguage(pTextContainer, language);

    IFCFAILFAST(wordsSegmenterFactory->CreateWithLanguage(
        wrl::Wrappers::HStringReference(language.GetBuffer()).Get(),
        &wordsSegmenter));

    return wordsSegmenter;
}

// Uses a SelectableWordsSegmenter to break the text into selectable segments,
// then adds each character offset where there can be a break to a list.
// A SelectableWordsSegmenter includes the trailing space after a word, whereas
// a WordsSegmenter would not. This is the behavior we want in both navigation and selection.
std::vector<uint32_t> CSelectionWordBreaker::GetTextSegments(
    _In_ HSTRING containerText,
    _In_ ITextContainer *pTextContainer)
{
    wrl::ComPtr<wda::Text::ISelectableWordsSegmenter> wordsSegmenter = GetWordsSegmenter(pTextContainer);

    wrl::ComPtr<wfc::IVectorView<wda::Text::SelectableWordSegment*>> selectableWordSegments;
    IFCFAILFAST(wordsSegmenter->GetTokens(containerText, &selectableWordSegments));
    ASSERT(selectableWordSegments != nullptr);

    uint32_t size = 0;
    IFCFAILFAST(selectableWordSegments->get_Size(&size));

    std::vector<uint32_t> breakOffsets;
    breakOffsets.push_back(0);
    for (uint32_t i = 0; i < size; ++i)
    {
        wda::Text::TextSegment textSegment;
        Microsoft::WRL::ComPtr<wda::Text::ISelectableWordSegment> segment;
        IFCFAILFAST(selectableWordSegments->GetAt(i, &segment));
        IFCFAILFAST(segment->get_SourceTextSegment(&textSegment));

        breakOffsets.push_back(textSegment.StartPosition + textSegment.Length);
    }

    return breakOffsets;
}

