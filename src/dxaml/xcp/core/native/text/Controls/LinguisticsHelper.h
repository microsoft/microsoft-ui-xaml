// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <string>

struct LinguisticsHelper final
{
    bool IsClosingPunctuation(wchar_t c) const;
    bool IsOpeningPunctuation(wchar_t c) const;
    bool HasCorrespondingOpeningPunctuation(wchar_t closing, const std::wstring& string) const;
    bool MayBeEmailAddressPrefix(const std::wstring& str) const;
    void SetInputLanguage(PCWSTR bcp47Tag);

    std::wstring DecimalSeparator;
    std::wstring ThousandSeparator;
    std::wstring InputLanguage;
    std::wstring SpaceCharacter;

    bool ShouldInsertSpace(const std::wstring& beforeString, const std::wstring& afterString) const;
    static bool StartsWith(const std::wstring& str, const std::wstring& prefix);
    static bool EndsWith(const std::wstring& str, const std::wstring& suffix);
    bool IsEastAsianInputLanguage() const;

private:
    std::wstring GetCharFromLocaleData(LCTYPE type, _In_ PCWSTR bcp47Tag) const;

    bool IsMatchedClosingPunctuation(wchar_t c, const std::wstring& previousStr) const;
    bool IsEastAsianTag(_In_ PCWSTR bcp47Tag) const;
    bool IsFullWidth(wchar_t c) const;
};
