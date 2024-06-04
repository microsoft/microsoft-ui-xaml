// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LinguisticsHelper.h"

#include <regex>
#include <stack>
#include <unordered_map>
#include <strsafe.h>
#include <wil/result.h>
#include <WinNls.h>

bool LinguisticsHelper::IsClosingPunctuation(wchar_t c) const
{
    static constexpr PCWSTR closingPunctuationCharacters = L".,;)]】>}\"'”’»?/\\!:*°%©®™…@、。〜〟』」♪‧——";
    return (wcschr(closingPunctuationCharacters, c) != nullptr) || IsFullWidth(c);
}

bool LinguisticsHelper::IsOpeningPunctuation(wchar_t c) const
{
    static constexpr PCWSTR openingPunctuationCharacters = L"([【<{\"'“‘«¿/\\¡…@、。〜〝『「‧——　$";
    return (wcschr(openingPunctuationCharacters, c) != nullptr) || IsFullWidth(c);
}

bool LinguisticsHelper::HasCorrespondingOpeningPunctuation(const wchar_t closing, const std::wstring& string) const
{
    const std::unordered_map<wchar_t, wchar_t> openToClose =
    {
        {L'\"', L'\"'},
        {L'\'', L'\''},
        {L'“', L'”'},
        {L'‘', L'’'},
        {L'<', L'>'},
        {L'{', L'}'},
        {L'[', L']'},
        {L'«', L'»'},
        {L'『', L'』'},
        {L'「', L'」'}
    };

    wchar_t opening = L'\0';
    for (const auto& entry : openToClose)
    {
        if (entry.second == closing)
        {
            opening = entry.first;
            break;
        }
    }

    if (opening == L'\0')
    {
        // The closing character was not found in the openClose set of matched open / close characters,
        // so it does not have a matched opening character.
        return false;
    }

    std::stack<wchar_t> unmatchedChars;
    for (const wchar_t curChar : string)
    {
        if ((!unmatchedChars.empty()) && (curChar == openToClose.at(unmatchedChars.top())))
        {
            unmatchedChars.pop();
        }
        else if (openToClose.find(curChar) != openToClose.end()) //disregard extra closing markers
        {
            unmatchedChars.push(curChar);
        }
    }
    return (!unmatchedChars.empty()) && (unmatchedChars.top() == opening);
}

bool LinguisticsHelper::IsMatchedClosingPunctuation(const wchar_t c, const std::wstring& previousStr) const
{
    return IsClosingPunctuation(c) && HasCorrespondingOpeningPunctuation(c, previousStr);
}

bool LinguisticsHelper::MayBeEmailAddressPrefix(const std::wstring& str) const
{
    // Is in the format [\w_.]*@(\w+.)*\w+
    // the * for the username is because we might have already committed the username to text.
    // this method is used to determine whether a period should not carry a space after it,
    // so don't include the final .com part which doesn't end in a period
    const std::wregex emailRegex(LR"#([\w.]*@(\w+\.)*$)#");
    return std::regex_search(str, emailRegex);
}

void LinguisticsHelper::SetInputLanguage(PCWSTR bcp47Tag)
{
    DecimalSeparator = GetCharFromLocaleData(LOCALE_SDECIMAL, bcp47Tag);
    ThousandSeparator = GetCharFromLocaleData(LOCALE_STHOUSAND, bcp47Tag);
    SpaceCharacter = IsEastAsianTag(bcp47Tag) ? L"\u3000" : L" ";
    InputLanguage = bcp47Tag;
}

bool LinguisticsHelper::ShouldInsertSpace(const std::wstring& beforeString, const std::wstring& afterString) const
{
    if (IsEastAsianTag(InputLanguage.c_str()))
    {
        return false;
    }

    if (beforeString.empty() || afterString.empty())
    {
        return false;
    }

    if (iswspace(beforeString.back()) || iswspace(afterString.front()))
    {
        return false;
    }

    if ((IsOpeningPunctuation(beforeString.back())) &&
        (!IsMatchedClosingPunctuation(beforeString.back(), beforeString.substr(0, beforeString.size() - 1))))
    {
        return false;
    }

    // If this is an opening punctuation mark that is not also a closing punctuation mark with a matching a previous opening mark,
    // insert a space.
    if ((IsOpeningPunctuation(afterString.front())) &&
        (!IsMatchedClosingPunctuation(afterString.front(), beforeString)))
    {
        return true;
    }

    if (iswpunct(afterString.front()))
    {
        return false;
    }

    if (iswpunct(beforeString.back()))
    {
        return true;
    }

    return true;
}

/*static*/
bool LinguisticsHelper::StartsWith(const std::wstring& str, const std::wstring& prefix)
{
    return (str.length() >= prefix.length()) &&
        (wcsncmp(str.c_str(), prefix.c_str(), prefix.length()) == 0);
}

/*static*/
bool LinguisticsHelper::EndsWith(const std::wstring& str, const std::wstring& suffix)
{
    return (str.length() >= suffix.length()) &&
        (wcsncmp(&(str.c_str()[str.length() - suffix.length()]), suffix.c_str(), suffix.length()) == 0);
}

/*static*/
std::wstring LinguisticsHelper::GetCharFromLocaleData(LCTYPE type, _In_ PCWSTR bcp47Tag) const
{
    wchar_t delim[5] = {};
    if (!GetLocaleInfoEx(bcp47Tag, type, delim, ARRAYSIZE(delim)))
    {
        delim[0] = L'.';
    }

    FAIL_FAST_IF(delim[1] != L'\0'); // We have a delimiter longer than one character?!

    return delim;
}

/*static*/
bool LinguisticsHelper::IsEastAsianTag(_In_ PCWSTR bcp47Tag) const
{
    PCWSTR EastAsianLanguages[] = { L"ja", L"zh", L"ko" };
    for (auto lang : EastAsianLanguages)
    {
        const auto len = wcslen(lang);
        if (CompareStringOrdinal(bcp47Tag, static_cast<int>(std::min(wcslen(bcp47Tag), len)), lang, static_cast<int>(len), TRUE) == CSTR_EQUAL)
        {
            return true;
        }
    }
    return false;
}

bool LinguisticsHelper::IsFullWidth(wchar_t c) const
{
    return ((c >= 0xff01) && (c <= 0xff60)) ||  // normal full-width
        ((c >= 0xffe0) && (c <= 0xffe6));       // full-width currency symbols
}

bool LinguisticsHelper::IsEastAsianInputLanguage() const
{
    return IsEastAsianTag(InputLanguage.c_str());
}
