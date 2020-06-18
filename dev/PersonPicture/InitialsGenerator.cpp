// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InitialsGenerator.h"
#include <cwctype>
#include <sstream>
#include <algorithm>

/// <summary>
/// Helper function which takes in a Contact object and produces initials
/// which represent that contact.
/// </summary>
/// <remarks>
/// INTENDED LOGIC BEHAVIOR:
/// - If the name contains glyphs, return a generic icon.
/// - If we have access to First Name vs. Last Name data
///   (First Name being set regionally - i.e. it equals Given Name or Family Name based on system setting),
///   -> If non-symbolic, use the first letter of the First Name and first letter of the Last Name
///   -> If symbolic, use the first character of the Given Name
/// - If we don't have access to First/Last or Given/Family Name, (i.e. long string Dr. Jordan von Hammerspike III)
///   -> If non-symbolic, break by space delimiter then follow the following rules:
///      --> Use the first letter of the first word and the first letter of the last word
///      --> If only one word, use a single letter
///   -> If symbolic, use the first one or two characters of the string (decided by how squished it looks visually)
///      --> Note: This branch of the logic falls apart in the Family Name, Given Name case.
/// </remarks>
winrt::hstring InitialsGenerator::InitialsFromContactObject(const winrt::Contact &contact)
{
    if (!contact)
    {
        return winrt::hstring(L"");
    }

    // Optimal case is we have a clearly defined First and Last name. If
    // available, that is the data which should be used.
    if (!contact.FirstName().empty() && !contact.LastName().empty())
    {
        CharacterType type = GetCharacterType(contact.FirstName());

        // We'll attempt to make initials only if we recognize a name in the Standard character set.
        if (type == CharacterType::Standard)
        {
            std::wstring firstName = static_cast<std::wstring>(contact.FirstName());
            std::wstring lastName = static_cast<std::wstring>(contact.LastName());

            std::wstring result = GetFirstFullCharacter(firstName.data());
            result.append(GetFirstFullCharacter(lastName.data()));

            std::transform(result.begin(), result.end(), result.begin(), ::towupper);

            return winrt::hstring(result);
        }
        else
        {
            // Return empty string. In our code-behind we will produce a generic glyph as a result.
            return winrt::hstring(L"");
        }
    }

    // If the supplied object does not contain granular name data, then we must
    // extract the correct initials from the DisplayName.
    if (!contact.DisplayName().empty())
    {
        return InitialsFromDisplayName(contact.DisplayName());
    }

    // Return empty string. In our code-behind we will produce a generic glyph as a result.
    return winrt::hstring(L"");
}

winrt::hstring InitialsGenerator::InitialsFromDisplayName(const wstring_view &contactDisplayName)
{
    const CharacterType type = GetCharacterType(contactDisplayName);

    // We'll attempt to make initials only if we recognize a name in the Standard character set.
    if (type == CharacterType::Standard)
    {
        std::wstring displayName(contactDisplayName.data());

        StripTrailingBrackets(displayName);

        std::vector<std::wstring> words = Split(displayName, L' ');

        if (words.size() == 1)
        {
            // If there's only a single long word, we'll show one initial.
            std::wstring firstWord = words.front();

            std::wstring result = GetFirstFullCharacter(firstWord);

            std::transform(result.begin(), result.end(), result.begin(), ::towupper);

            return winrt::hstring(result);
        }
        else if (words.size() > 1)
        {
            // If there's at least two words, we'll show two initials.
            // 
            // NOTE: Based on current implementation, we could be showing punctuation.
            // For example, "John -Smith" would be "J-".
            std::wstring firstWord = words.front();
            std::wstring lastWord = words.back();

            std::wstring result = GetFirstFullCharacter(firstWord);
            result.append(GetFirstFullCharacter(lastWord));

            std::transform(result.begin(), result.end(), result.begin(), ::towupper);

            return winrt::hstring(result);
        }
        else
        {
            // If there's only spaces in the name, we'll get a Vector size of 0.
            return winrt::hstring(L"");
        }
    }
    else
    {
        // Return empty string. In our code-behind we will produce a generic glyph as a result.
        return winrt::hstring(L"");
    }
}

std::wstring InitialsGenerator::GetFirstFullCharacter(const std::wstring &str)
{
    // Index should begin at the first desireable character.
    unsigned int start = 0;

    while (start < str.size())
    {
        const wchar_t character = str.at(start);

        // Omit ! " # $ % & ' ( ) * + , - . /
        if ((character >= 0x0021) && (character <= 0x002F))
        {
            start++;
            continue;
        }

        // Omit : ; < = > ? @
        if ((character >= 0x003A) && (character <= 0x0040))
        {
            start++;
            continue;
        }

        // Omit { | } ~
        if ((character >= 0x007B) && (character <= 0x007E))
        {
            start++;
            continue;
        }

        break;
    }

    // If no desireable characters exist, we'll start at index 1, as combining
    // characters begin only after the first character.
    if (start >= str.size())
    {
        start = 0;
    }

    // Combining characters begin only after the first character, so we should start
    // looking 1 after the start character.
    unsigned int index = start + 1;

    while (index < str.size())
    {
        const wchar_t character = str.at(index);

        // Combining Diacritical Marks -- Official Unicode character block
        if ((character < 0x0300) || (character > 0x036F))
        {
            break;
        }

        index++;
    }

    // Determine number of diacritics by adjusting for our initial offset.
    const unsigned int strLength = index - start;

    std::wstring result(&str.at(start), strLength);
    return result;
}

std::vector<std::wstring> InitialsGenerator::Split(const std::wstring &source, wchar_t delim, int maxIterations)
{
    std::vector<std::wstring> result;

    std::wistringstream buffer(source);

    int i = 0;

    for (std::wstring token; std::getline(buffer, token, delim);)
    {
        if (!token.empty())
        {
            result.push_back(token);
        }
        if (++i >= maxIterations)
        {
            break;
        }
    }

    return result;
}

void InitialsGenerator::StripTrailingBrackets(std::wstring &source)
{
    // Guidance from the world readiness team is that text within a final set of brackets
    // can be removed for the purposes of calculating initials. ex. John Smith (OSG)
    std::wstring_view delimiters[] = { L"{}"sv, L"()"sv, L"[]"sv };

    if (source.empty())
    {
        return;
    }

    for (const auto delimiter : delimiters)
    {
        if (source[source.length() - 1] != delimiter[1])
        {
            continue;
        }

        const auto start = source.find_last_of(delimiter[0]);
        if (start == std::string::npos)
        {
            continue;
        }

        source.erase(start);
        return;
    }
}

CharacterType InitialsGenerator::GetCharacterType(const wstring_view &str)
{
    // Since we're doing initials, we're really only interested in the first
    // few characters. If the first three characters aren't a glyph then
    // we don't need to count it as such because we won't be changing meaning
    // by truncating to one or two.
    CharacterType result = CharacterType::Other;

    for (int i = 0; i < 3; i++)
    {
        // Break on null character. 0xFEFF is a terminating character which appears as null.
        if ((str.data()[i] == '\0') || (str.data()[i] == 0xFEFF))
        {
            break;
        }

        const wchar_t character = str.data()[i];
        const CharacterType evaluationResult = GetCharacterType(character);

        // In mix-match scenarios, we'll want to follow this order of precedence:
        // Glyph > Symbolic > Roman
        switch (evaluationResult)
        {
        case CharacterType::Glyph:
            result = CharacterType::Glyph;
            break;
        case CharacterType::Symbolic:
            // Don't override a Glyph state with a Symbolic State.
            if (result != CharacterType::Glyph)
            {
                result = CharacterType::Symbolic;
            }
            break;
        case CharacterType::Standard:
            // Don't override a Glyph or Symbolic state with a Latin state.
            if ((result != CharacterType::Glyph) && (result != CharacterType::Symbolic))
            {
                result = CharacterType::Standard;
            }
            break;
        default:
            // Preserve result's current state (if we never got data other 
            // than "Other", it'll be set to other anyway).
            break;
        }
    }

    return result;
}

CharacterType InitialsGenerator::GetCharacterType(wchar_t character)
{
    // To ensure predictable behavior, we're currently operating on an allowed list of character sets.
    //
    // Each block below is a HEX range in the official Unicode spec, which defines a set
    // of Unicode characters. Changes to the character sets would only be made by Unicode, and
    // are highly unlikely (as it would break virtually every modern text parser).
    // Definitions available here: http://www.unicode.org/charts/
    //
    // GLYPH
    //
    // IPA Extensions
    if ((character >= 0x0250) && (character <= 0x02AF))
    {
        return CharacterType::Glyph;
    }

    // Arabic
    if ((character >= 0x0600) && (character <= 0x06FF))
    {
        return CharacterType::Glyph;
    }

    // Arabic Supplement
    if ((character >= 0x0750) && (character <= 0x077F))
    {
        return CharacterType::Glyph;
    }

    // Arabic Extended-A
    if ((character >= 0x08A0) && (character <= 0x08FF))
    {
        return CharacterType::Glyph;
    }

    // Arabic Presentation Forms-A
    if ((character >= 0xFB50) && (character <= 0xFDFF))
    {
        return CharacterType::Glyph;
    }

    // Arabic Presentation Forms-B
    if ((character >= 0xFE70) && (character <= 0xFEFF))
    {
        return CharacterType::Glyph;
    }

    // Devanagari
    if ((character >= 0x0900) && (character <= 0x097F))
    {
        return CharacterType::Glyph;
    }

    // Devanagari Extended
    if ((character >= 0xA8E0) && (character <= 0xA8FF))
    {
        return CharacterType::Glyph;
    }

    // Bengali
    if ((character >= 0x0980) && (character <= 0x09FF))
    {
        return CharacterType::Glyph;
    }

    // Gurmukhi
    if ((character >= 0x0A00) && (character <= 0x0A7F))
    {
        return CharacterType::Glyph;
    }

    // Gujarati
    if ((character >= 0x0A80) && (character <= 0x0AFF))
    {
        return CharacterType::Glyph;
    }

    // Oriya
    if ((character >= 0x0B00) && (character <= 0x0B7F))
    {
        return CharacterType::Glyph;
    }

    // Tamil
    if ((character >= 0x0B80) && (character <= 0x0BFF))
    {
        return CharacterType::Glyph;
    }

    // Telugu
    if ((character >= 0x0C00) && (character <= 0x0C7F))
    {
        return CharacterType::Glyph;
    }

    // Kannada
    if ((character >= 0x0C80) && (character <= 0x0CFF))
    {
        return CharacterType::Glyph;
    }

    // Malayalam
    if ((character >= 0x0D00) && (character <= 0x0D7F))
    {
        return CharacterType::Glyph;
    }

    // Sinhala
    if ((character >= 0x0D80) && (character <= 0x0DFF))
    {
        return CharacterType::Glyph;
    }

    // Thai
    if ((character >= 0x0E00) && (character <= 0x0E7F))
    {
        return CharacterType::Glyph;
    }

    // Lao
    if ((character >= 0x0E80) && (character <= 0x0EFF))
    {
        return CharacterType::Glyph;
    }

    // SYMBOLIC
    //
    // CJK Unified Ideographs
    if ((character >= 0x4E00) && (character <= 0x9FFF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Unified Ideographs Extension 
    if ((character >= 0x3400) && (character <= 0x4DBF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Unified Ideographs Extension B
    if ((character >= 0x20000) && (character <= 0x2A6DF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Unified Ideographs Extension C
    if ((character >= 0x2A700) && (character <= 0x2B73F))
    {
        return CharacterType::Symbolic;
    }

    // CJK Unified Ideographs Extension D
    if ((character >= 0x2B740) && (character <= 0x2B81F))
    {
        return CharacterType::Symbolic;
    }

    // CJK Radicals Supplement
    if ((character >= 0x2E80) && (character <= 0x2EFF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Symbols and Punctuation
    if ((character >= 0x3000) && (character <= 0x303F))
    {
        return CharacterType::Symbolic;
    }

    // CJK Strokes
    if ((character >= 0x31C0) && (character <= 0x31EF))
    {
        return CharacterType::Symbolic;
    }

    // Enclosed CJK Letters and Months
    if ((character >= 0x3200) && (character <= 0x32FF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Compatibility
    if ((character >= 0x3300) && (character <= 0x33FF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Compatibility Ideographs
    if ((character >= 0xF900) && (character <= 0xFAFF))
    {
        return CharacterType::Symbolic;
    }

    // CJK Compatibility Forms
    if ((character >= 0xFE30) && (character <= 0xFE4F))
    {
        return CharacterType::Symbolic;
    }

    // CJK Compatibility Ideographs Supplement
    if ((character >= 0x2F800) && (character <= 0x2FA1F))
    {
        return CharacterType::Symbolic;
    }

    // Greek and Coptic
    if ((character >= 0x0370) && (character <= 0x03FF))
    {
        return CharacterType::Symbolic;
    }

    // Hebrew
    if ((character >= 0x0590) && (character <= 0x05FF))
    {
        return CharacterType::Symbolic;
    }

    // Armenian
    if ((character >= 0x0530) && (character <= 0x058F))
    {
        return CharacterType::Symbolic;
    }

    // LATIN
    //
    // Basic Latin
    if ((character > 0x0000) && (character <= 0x007F))
    {
        return CharacterType::Standard;
    }

    // Latin-1 Supplement
    if ((character >= 0x0080) && (character <= 0x00FF))
    {
        return CharacterType::Standard;
    }

    // Latin Extended-A
    if ((character >= 0x0100) && (character <= 0x017F))
    {
        return CharacterType::Standard;
    }

    // Latin Extended-B
    if ((character >= 0x0180) && (character <= 0x024F))
    {
        return CharacterType::Standard;
    }

    // Latin Extended-C
    if ((character >= 0x2C60) && (character <= 0x2C7F))
    {
        return CharacterType::Standard;
    }

    // Latin Extended-D
    if ((character >= 0xA720) && (character <= 0xA7FF))
    {
        return CharacterType::Standard;
    }

    // Latin Extended-E
    if ((character >= 0xAB30) && (character <= 0xAB6F))
    {
        return CharacterType::Standard;
    }

    // Latin Extended Additional
    if ((character >= 0x1E00) && (character <= 0x1EFF))
    {
        return CharacterType::Standard;
    }

    // Cyrillic
    if ((character >= 0x0400) && (character <= 0x04FF))
    {
        return CharacterType::Standard;
    }

    // Cyrillic Supplement
    if ((character >= 0x0500) && (character <= 0x052F))
    {
        return CharacterType::Standard;
    }

    // Combining Diacritical Marks
    if ((character >= 0x0300) && (character <= 0x036F))
    {
        return CharacterType::Standard;
    }

    return CharacterType::Other;
}
