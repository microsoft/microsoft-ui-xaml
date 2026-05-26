// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "AccessKeysParser.h"
#include "AccessKey.h"
#include <winstring.h>

using namespace AccessKeys;

// Strings in this list will cause parsing to fail
constexpr std::wstring_view invalidStringList[] =
{
    L" ",
    L"\t",
    L"\r",
    L"\n",
    L"\x200b", // 0 width space character
};

bool AKParser::TryParseAccessKey(_In_ const std::wstring& accessString, _Inout_ AKAccessKey& accessKey)
{
    if (!IsValidAccessKey(accessString))
        return false;

    // Because accessString is valid (has valid length and character composition) set it to the accessKey
    accessKey = accessString;

    return true;
}

bool AKParser::IsValidAccessKey(_In_ const std::wstring& accessString)
{
    // Right now only allow access keys of length 6 or less.
    if (accessString.length() == 0 || accessString.length() > AccessKeys::maxAccessKeyLength)
        return false;

    // If the access string contains any invalid characters or substrings, it's an invalid access string
    if (ContainsInvalidSubstring(accessString))
        return false;

    return true;
}

bool AccessKeys::AKParser::ContainsInvalidSubstring(_In_ const std::wstring & accessString)
{
    // For each string in invalidStringList, check it's not a substring in accessString
    for (const auto& invalidString : invalidStringList)
    {
        if (accessString.find(invalidString) != std::wstring::npos) // npos is returned when the string is not found
            return true;
    }

    // A null character terminates the wstring input internally, but it cannot be one of the characters counted in the std::basic_string::length() method
    // Unfortunately, the find method seems to match the null character as a valid substring of non-null characters.  Will check for it explicitly here
    for (const wchar_t accessChar : accessString)
    {
        if (accessChar == L'\0')
            return true;
    }

    return false;
}

