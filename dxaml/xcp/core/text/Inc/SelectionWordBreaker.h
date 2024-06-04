// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef NO_HEADER_INCLUDES
#include "PlainTextPosition.h"
#endif

#include <fwd/windows.data.text.h>

class CTextPosition;
class CTextBackingStoreNavigator;

//------------------------------------------------------------------------
//  Summary:
//      Simple interface that can retrieve characters from a text backend (like an ITextContainer)
//      Exposes the minimal functionality needed by CSelectionWordBreaker
//------------------------------------------------------------------------
class ISimpleTextBackend
{
public:
    virtual wchar_t GetCharacter(_In_ XUINT32 offset) = 0;
};

// Backward and ForwardIncludeTrailingWhitespace navigate the "proper" word boundaries,
// which include trailing whitespaces:
//
// "[word1  ][word2 ][word3][,][word4   ]"
//
// ForwardExact will, if the position doesn't start at a whitespace, find the ForwardIncludeTrailingWhitespace
// boundary and move back to trim all the trailing whitespaces.
//
enum class FindBoundaryType
{
    Backward = 0,
    ForwardIncludeTrailingWhitespace,
    ForwardExact
};

inline bool IsForwardDirection(_In_ FindBoundaryType findType)
{
    return (findType != FindBoundaryType::Backward);
}

//------------------------------------------------------------------------
//
// CSelectionWordBreaker
//
// A short term substitute for replacing TextBox word navigation algorithm,
// that used to live in CTextView/CTextLineRider classes.
// Needs more work before this can be shared by TextBox/RichTextBox backends.
//
//------------------------------------------------------------------------
class CSelectionWordBreaker
{
private:

    static bool CanBreak(
        CTextBackingStoreNavigator navigator,
        const std::vector<uint32_t>& breakIndexes);

    static bool IsSelectionBreak(
        wchar_t prevChar,
        wchar_t breakChar,
        CTextBackingStoreNavigator navigator,
        FindBoundaryType direction,
        const std::vector<uint32_t>& breakIndexes);

    static bool IsNavigationBreak(
        CTextBackingStoreNavigator navigator,
        CTextBackingStoreNavigator prevNavigator,
        bool fPrevSpace,
        const std::vector<uint32_t>& breakIndexes,
        bool isNonSpaced);

    static wrl::ComPtr<wda::Text::ISelectableWordsSegmenter> GetWordsSegmenter(_In_ ITextContainer *pTextContainer);
    static void GetLanguage(_In_ ITextContainer *pTextContainer, _Out_ xstring_ptr &language);
    static bool IsNonSpaceDelimitedLanguage(_In_ ITextContainer *pTextContainer);
    static std::vector<uint32_t> GetTextSegments(_In_ HSTRING containerText, _In_ ITextContainer *pTextContainer);

    static bool IsWordStartingWithPunctuation(wchar_t character, bool fPrevSpace, wchar_t prevCharacter);
    static bool IsPunctuationAtEndOfWord(wchar_t character, bool fPrevSpace);



public:
    // Given a position, find the next "by word" navigation boundary in
    // the given direction.
    _Check_return_ HRESULT static GetAdjacentWordNavigationBoundary(
        _In_ CTextPosition       currentPosition,
        _In_ ISimpleTextBackend *pTextBackend,
        _In_ FindBoundaryType    findType,
        _Out_ CTextPosition     *pBoundaryPosition);

    // Given a position, find the next word boundary for selection in
    // the given direction.
    _Check_return_ HRESULT static GetAdjacentWordSelectionBoundary(
        _In_ CTextPosition       currentPosition,
        _In_ ISimpleTextBackend *pTextBackend,
        _In_ FindBoundaryType    findType,
        _Out_ CTextPosition     *pBoundaryPosition);

    _Check_return_ HRESULT static GetAdjacentNonWhitespaceCharacter(
        _In_ CTextPosition       currentPosition,
        _In_ ISimpleTextBackend *pTextBackend,
        _In_ FindBoundaryType    findType,
        _Out_ CTextPosition     *pBoundaryPosition);
};